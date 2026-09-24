#include "SignalHubSubsystem.h"
#include "SignalHubSettings.h"

#include "Containers/Ticker.h"
#include "Logging/StructuredLog.h"
#include "Misc/ScopeLock.h"

struct USignalHubSubsystem::FImpl
{
	struct FListener
	{
		int64 Id = 0;
		uint64 Serial = 0;
		bool bHasOwner = false;
		TWeakObjectPtr<UObject> Owner;
		TFunction<void(const FSignalPayload&, const FSignalContext&)> Callback;
	};

	struct FChannel
	{
		FSignalTypeId PayloadType;
		uint32 Generation = 1;
		uint64 NextListenerSerial = 1;
		TArray<FListener> Listeners;
	};

	struct FQueuedSignal
	{
		FSignalKey Key;
		FSignalPayload Payload;
		int64 Sequence = 0;
		int32 Depth = 0;
		uint32 ChannelGeneration = 0;
		uint64 ListenerSerialCutoff = 0;
	};

	mutable FCriticalSection Lock;
	TMap<FSignalKey, FChannel> Channels;
	TArray<FQueuedSignal> Pending;
	TArray<FQueuedSignal> Reentrant;
	uint64 NextSubscriptionId = 1;
	uint32 NextChannelGeneration = 1;
	uint64 NextSequence = 1;
	int32 QueueOverflows = 0;
	int32 CascadeRejections = 0;
	int32 PeakQueueDepth = 0;
	int32 PeakCascadeDepth = 0;
	bool bQueueOverflowLogged = false;

	void PruneExpiredOwners()
	{
		for (auto it = Channels.CreateIterator(); it; ++it)
		{
			it.Value().Listeners.RemoveAll([](const FListener& listener) { return listener.bHasOwner && !listener.Owner.IsValid(); });
			if (it.Value().Listeners.IsEmpty()) it.RemoveCurrent();
		}
	}

	void AddReferencedObjects(FReferenceCollector& InCollector)
	{
		FScopeLock lock(&Lock);
		for (const FQueuedSignal& signal : Pending) signal.Payload.AddReferencedObjects(InCollector);
		for (const FQueuedSignal& signal : Reentrant) signal.Payload.AddReferencedObjects(InCollector);
	}
};

USignalHubSubsystem::~USignalHubSubsystem() = default;

void USignalHubSubsystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& InCollector)
{
	USignalHubSubsystem* hub = CastChecked<USignalHubSubsystem>(InThis);
	if (hub->Impl) hub->Impl->AddReferencedObjects(InCollector);
	Super::AddReferencedObjects(InThis, InCollector);
}

void USignalHubSubsystem::Initialize(FSubsystemCollectionBase& InCollection)
{
	Super::Initialize(InCollection);
	Limits = GetDefault<USignalHubSettings>()->Limits;
	Limits.Clamp();
	Impl = MakeShared<FImpl>();
	bAcceptingPublishes.Store(true);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USignalHubSubsystem::Tick));
}

void USignalHubSubsystem::Deinitialize()
{
	bAcceptingPublishes.Store(false);
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	if (Impl)
	{
		FScopeLock lock(&Impl->Lock);
		Impl->Pending.Reset();
		Impl->Channels.Reset();
	}
	Impl.Reset();
	Super::Deinitialize();
}

FSignalSubscribeOutcome USignalHubSubsystem::SubscribeBoxed(const FSignalKey& InKey, const FSignalTypeId& InPayloadType, UObject* InOwner, TFunction<void(const FSignalPayload&, const FSignalContext&)>&& InCallback)
{
	if (!IsInGameThread()) return { ESignalSubscribeResult::WrongThread, {} };
	if (!bAcceptingPublishes.Load() || !Impl) return { ESignalSubscribeResult::ShuttingDown, {} };
	if (!InKey.IsValid()) return { ESignalSubscribeResult::InvalidKey, {} };
	if (!InCallback) return { ESignalSubscribeResult::InvalidCallback, {} };

	FScopeLock lock(&Impl->Lock);
	Impl->PruneExpiredOwners();
	FImpl::FChannel* channel = Impl->Channels.Find(InKey);
	if (!channel)
	{
		FImpl::FChannel created;
		created.PayloadType = InPayloadType;
		created.Generation = Impl->NextChannelGeneration++;
		channel = &Impl->Channels.Add(InKey, MoveTemp(created));
	}
	else if (channel->PayloadType != InPayloadType)
	{
		return { ESignalSubscribeResult::PayloadTypeMismatch, {} };
	}
	const FSignalSubscriptionHandle handle { Impl->NextSubscriptionId++, static_cast<int32>(channel->Generation) };
	channel->Listeners.Add({ handle.Id, channel->NextListenerSerial++, InOwner != nullptr, InOwner, MoveTemp(InCallback) });
	return { ESignalSubscribeResult::Bound, handle };
}

bool USignalHubSubsystem::HasListeners(const FSignalKey& InKey, const FSignalTypeId& InPayloadType) const
{
	if (!bAcceptingPublishes.Load() || !Impl || !InKey.IsValid()) return false;
	FScopeLock lock(&Impl->Lock);
	Impl->PruneExpiredOwners();
	const FImpl::FChannel* channel = Impl->Channels.Find(InKey);
	return channel && channel->PayloadType == InPayloadType && channel->Listeners.Num() > 0;
}

ESignalPublishResult USignalHubSubsystem::PublishBoxed(const FSignalKey& InKey, const FSignalPayload& InPayload)
{
	if (!bAcceptingPublishes.Load() || !Impl) return ESignalPublishResult::ShuttingDown;
	if (!InKey.IsValid()) return ESignalPublishResult::InvalidKey;
	if (!InPayload.IsValid()) return ESignalPublishResult::PayloadTypeMismatch;

	int64 sequence;
	uint32 channelGeneration;
	uint64 listenerSerialCutoff;
	{
		FScopeLock lock(&Impl->Lock);
		Impl->PruneExpiredOwners();
		FImpl::FChannel* channel = Impl->Channels.Find(InKey);
		if (!channel || channel->Listeners.Num() == 0) return ESignalPublishResult::NoListeners;
		if (channel->PayloadType != *InPayload.GetTypeId()) return ESignalPublishResult::PayloadTypeMismatch;
		sequence = Impl->NextSequence++;
		channelGeneration = channel->Generation;
		listenerSerialCutoff = channel->NextListenerSerial - 1;
		if (!IsInGameThread())
		{
			if (!InPayload.IsWorkerCopySafe()) return ESignalPublishResult::WrongThreadForPayload;
			if (Impl->Pending.Num() >= Limits.MaxQueuedSignals)
			{
				++Impl->QueueOverflows;
				if (!Impl->bQueueOverflowLogged)
				{
					Impl->bQueueOverflowLogged = true;
					UE_LOGFMT(LogSignalHub, Warning, "SignalHub worker queue reached its capacity of {Capacity}; subsequent worker publications are rejected until it drains.", Limits.MaxQueuedSignals);
				}
				return ESignalPublishResult::QueueFull;
			}
			Impl->Pending.Add({ InKey, InPayload, sequence, 0, channelGeneration, listenerSerialCutoff });
			Impl->PeakQueueDepth = FMath::Max(Impl->PeakQueueDepth, Impl->Pending.Num());
			return ESignalPublishResult::Queued;
		}
	}
	if (bDispatching)
	{
		const int32 depth = CurrentDispatchDepth + 1;
		if (depth > Limits.MaxCascadeDepth)
		{
			FScopeLock lock(&Impl->Lock);
			++Impl->CascadeRejections;
			return ESignalPublishResult::CascadeLimit;
		}
		FScopeLock lock(&Impl->Lock);
		Impl->Reentrant.Add({ InKey, InPayload, sequence, depth, channelGeneration, listenerSerialCutoff });
		return ESignalPublishResult::QueuedReentrant;
	}
	Dispatch(InKey, InPayload, sequence, 0, channelGeneration, listenerSerialCutoff);
	return ESignalPublishResult::Delivered;
}

bool USignalHubSubsystem::Unsubscribe(FSignalSubscriptionHandle InHandle)
{
	if (!IsInGameThread() || !Impl || !InHandle.IsValid()) return false;
	FScopeLock lock(&Impl->Lock);
	for (auto it = Impl->Channels.CreateIterator(); it; ++it)
	{
		FImpl::FChannel& channel = it.Value();
		if (channel.Generation != InHandle.Generation) continue;
		const int32 index = channel.Listeners.IndexOfByPredicate([InHandle](const FImpl::FListener& listener) { return listener.Id == InHandle.Id; });
		if (index == INDEX_NONE) continue;
		channel.Listeners.RemoveAt(index);
		if (channel.Listeners.IsEmpty()) it.RemoveCurrent();
		return true;
	}
	return false;
}

int32 USignalHubSubsystem::UnsubscribeAll(UObject* InOwner)
{
	if (!IsInGameThread() || !Impl || !InOwner) return 0;
	int32 removed = 0;
	FScopeLock lock(&Impl->Lock);
	for (auto it = Impl->Channels.CreateIterator(); it; ++it)
	{
		removed += it.Value().Listeners.RemoveAll([InOwner](const FImpl::FListener& listener) { return listener.bHasOwner && listener.Owner.Get() == InOwner; });
		if (it.Value().Listeners.IsEmpty()) it.RemoveCurrent();
	}
	return removed;
}

bool USignalHubSubsystem::IsBound(const FSignalKey& InKey) const
{
	if (!Impl || !InKey.IsValid()) return false;
	FScopeLock lock(&Impl->Lock);
	Impl->PruneExpiredOwners();
	const FImpl::FChannel* channel = Impl->Channels.Find(InKey);
	return channel && !channel->Listeners.IsEmpty();
}

ESignalPublishResult USignalHubSubsystem::PublishPayload(const FSignalKey& InKey, const FSignalPayload& InPayload)
{
	return PublishBoxed(InKey, InPayload);
}

FSignalSubscribeOutcome USignalHubSubsystem::SubscribePayload(const FSignalKey& InKey, const FSignalTypeId& InPayloadType, UObject* InOwner, TFunction<void(const FSignalPayload&, const FSignalContext&)>&& InCallback)
{
	return SubscribeBoxed(InKey, InPayloadType, InOwner, MoveTemp(InCallback));
}

FSignalHubDiagnostics USignalHubSubsystem::GetDiagnostics() const
{
	FSignalHubDiagnostics result;
	if (!Impl) return result;
	FScopeLock lock(&Impl->Lock);
	Impl->PruneExpiredOwners();
	result.ActiveChannels = Impl->Channels.Num();
	result.PendingSignals = Impl->Pending.Num();
	result.QueueOverflows = Impl->QueueOverflows;
	result.CascadeRejections = Impl->CascadeRejections;
	result.PeakQueueDepth = Impl->PeakQueueDepth;
	result.PeakCascadeDepth = Impl->PeakCascadeDepth;
	for (const TPair<FSignalKey, FImpl::FChannel>& pair : Impl->Channels)
	{
		result.ActiveListeners += pair.Value.Listeners.Num();
	}
	return result;
}

void USignalHubSubsystem::FlushPendingSignals()
{
	if (IsInGameThread()) Tick(0.0f);
}

bool USignalHubSubsystem::Tick(float InDeltaTime)
{
	if (!Impl || !bAcceptingPublishes.Load()) return true;
	TArray<FImpl::FQueuedSignal> batch;
	{
		FScopeLock lock(&Impl->Lock);
		const int32 count = FMath::Min(Limits.MaxSignalsPerTick, Impl->Pending.Num());
		if (count > 0)
		{
			batch.Append(Impl->Pending.GetData(), count);
			Impl->Pending.RemoveAt(0, count, EAllowShrinking::No);
		}
	}
	for (const FImpl::FQueuedSignal& signal : batch)
	{
		Dispatch(signal.Key, signal.Payload, signal.Sequence, signal.Depth, signal.ChannelGeneration, signal.ListenerSerialCutoff);
	}
	return true;
}

void USignalHubSubsystem::Dispatch(const FSignalKey& InKey, const FSignalPayload& InPayload, int64 InSequence, int32 InDepth, uint32 InChannelGeneration, uint64 InListenerSerialCutoff)
{
	if (!Impl || bDispatching) return;
	bDispatching = true;
	Impl->Reentrant.Reset();
	Impl->Reentrant.Add({ InKey, InPayload, InSequence, InDepth, InChannelGeneration, InListenerSerialCutoff });
	bool bDispatchLimitExceeded = false;
	for (int32 index = 0; index < Impl->Reentrant.Num(); ++index)
	{
		if (index >= Limits.MaxDispatchesPerRoot)
		{
			bDispatchLimitExceeded = true;
			break;
		}
		const FImpl::FQueuedSignal signal = Impl->Reentrant[index];
		CurrentDispatchDepth = signal.Depth;
		{
			FScopeLock lock(&Impl->Lock);
			Impl->PeakCascadeDepth = FMath::Max(Impl->PeakCascadeDepth, signal.Depth);
		}
		DispatchOne(signal.Key, signal.Payload, signal.Sequence, signal.Depth, signal.ChannelGeneration, signal.ListenerSerialCutoff);
	}
	if (bDispatchLimitExceeded)
	{
		FScopeLock lock(&Impl->Lock);
		++Impl->CascadeRejections;
		UE_LOGFMT(LogSignalHub, Warning, "SignalHub discarded descendants after reaching the per-root dispatch limit of {Limit}.", Limits.MaxDispatchesPerRoot);
	}
	Impl->Reentrant.Reset();
	CurrentDispatchDepth = 0;
	bDispatching = false;
}

void USignalHubSubsystem::DispatchOne(const FSignalKey& InKey, const FSignalPayload& InPayload, int64 InSequence, int32 InDepth, uint32 InChannelGeneration, uint64 InListenerSerialCutoff)
{
	if (!Impl) return;
	TArray<uint64> listenerIds;
	{
		FScopeLock lock(&Impl->Lock);
		if (const FImpl::FChannel* channel = Impl->Channels.Find(InKey); channel && channel->Generation == InChannelGeneration)
		{
			for (const FImpl::FListener& listener : channel->Listeners)
			{
				if (listener.Serial <= InListenerSerialCutoff) listenerIds.Add(listener.Id);
			}
		}
	}
	const FSignalContext context { InSequence, InDepth };
	for (uint64 listenerId : listenerIds)
	{
		TFunction<void(const FSignalPayload&, const FSignalContext&)> callback;
		{
			FScopeLock lock(&Impl->Lock);
			FImpl::FChannel* channel = Impl->Channels.Find(InKey);
			if (!channel || channel->Generation != InChannelGeneration) continue;
			const FImpl::FListener* listener = channel->Listeners.FindByPredicate([listenerId](const FImpl::FListener& value) { return value.Id == listenerId; });
			if (!listener || (listener->bHasOwner && !listener->Owner.IsValid())) continue;
			callback = listener->Callback;
		}
		if (callback) callback(InPayload, context);
	}
}
