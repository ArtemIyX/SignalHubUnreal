#include "SignalHubSubsystem.h"

#include "Containers/Ticker.h"
#include "Misc/ScopeLock.h"

struct USignalHubSubsystem::FImpl
{
	struct FListener
	{
		int64 Id = 0;
		TWeakObjectPtr<UObject> Owner;
		TFunction<void(const FSignalPayload&, const FSignalContext&)> Callback;
	};

	struct FChannel
	{
		FSignalTypeId PayloadType;
		uint32 Generation = 1;
		TArray<FListener> Listeners;
	};

	struct FQueuedSignal
	{
		FSignalKey Key;
		FSignalPayload Payload;
		int64 Sequence = 0;
	};

	mutable FCriticalSection Lock;
	TMap<FSignalKey, FChannel> Channels;
	TArray<FQueuedSignal> Pending;
	TArray<FQueuedSignal> Reentrant;
	uint64 NextSubscriptionId = 1;
	uint64 NextSequence = 1;
};

USignalHubSubsystem::~USignalHubSubsystem() = default;

void USignalHubSubsystem::Initialize(FSubsystemCollectionBase& InCollection)
{
	Super::Initialize(InCollection);
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
	FImpl::FChannel& channel = Impl->Channels.FindOrAdd(InKey);
	if (channel.Listeners.Num() == 0)
	{
		channel.PayloadType = InPayloadType;
	}
	else if (channel.PayloadType != InPayloadType)
	{
		return { ESignalSubscribeResult::PayloadTypeMismatch, {} };
	}
	const FSignalSubscriptionHandle handle { Impl->NextSubscriptionId++, channel.Generation };
	channel.Listeners.Add({ handle.Id, InOwner, MoveTemp(InCallback) });
	return { ESignalSubscribeResult::Bound, handle };
}

bool USignalHubSubsystem::HasListeners(const FSignalKey& InKey, const FSignalTypeId& InPayloadType) const
{
	if (!bAcceptingPublishes.Load() || !Impl || !InKey.IsValid()) return false;
	FScopeLock lock(&Impl->Lock);
	const FImpl::FChannel* channel = Impl->Channels.Find(InKey);
	return channel && channel->PayloadType == InPayloadType && channel->Listeners.Num() > 0;
}

ESignalPublishResult USignalHubSubsystem::PublishBoxed(const FSignalKey& InKey, const FSignalPayload& InPayload)
{
	if (!bAcceptingPublishes.Load() || !Impl) return ESignalPublishResult::ShuttingDown;
	if (!InKey.IsValid()) return ESignalPublishResult::InvalidKey;
	if (!InPayload.IsValid()) return ESignalPublishResult::PayloadTypeMismatch;

	int64 sequence;
	{
		FScopeLock lock(&Impl->Lock);
		FImpl::FChannel* channel = Impl->Channels.Find(InKey);
		if (!channel || channel->Listeners.Num() == 0) return ESignalPublishResult::NoListeners;
		if (channel->PayloadType != *InPayload.GetTypeId()) return ESignalPublishResult::PayloadTypeMismatch;
		sequence = Impl->NextSequence++;
		if (!IsInGameThread())
		{
			if (!InPayload.IsWorkerCopySafe()) return ESignalPublishResult::WrongThreadForPayload;
			if (Impl->Pending.Num() >= Limits.MaxQueuedSignals) return ESignalPublishResult::QueueFull;
			Impl->Pending.Add({ InKey, InPayload, sequence });
			return ESignalPublishResult::Queued;
		}
	}
	if (bDispatching)
	{
		FScopeLock lock(&Impl->Lock);
		Impl->Reentrant.Add({ InKey, InPayload, sequence });
		return ESignalPublishResult::QueuedReentrant;
	}
	Dispatch(InKey, InPayload, sequence, 0);
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

void USignalHubSubsystem::UnsubscribeAll(UObject* InOwner)
{
	if (!IsInGameThread() || !Impl || !InOwner) return;
	FScopeLock lock(&Impl->Lock);
	for (auto it = Impl->Channels.CreateIterator(); it; ++it)
	{
		it.Value().Listeners.RemoveAll([InOwner](const FImpl::FListener& listener) { return listener.Owner.Get() == InOwner; });
		if (it.Value().Listeners.IsEmpty()) it.RemoveCurrent();
	}
}

bool USignalHubSubsystem::IsBound(const FSignalKey& InKey) const
{
	if (!Impl || !InKey.IsValid()) return false;
	FScopeLock lock(&Impl->Lock);
	const FImpl::FChannel* channel = Impl->Channels.Find(InKey);
	return channel && !channel->Listeners.IsEmpty();
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
		Dispatch(signal.Key, signal.Payload, signal.Sequence, 0);
	}
	return true;
}

void USignalHubSubsystem::Dispatch(const FSignalKey& InKey, const FSignalPayload& InPayload, int64 InSequence, int32 InDepth)
{
	if (!Impl || InDepth >= Limits.MaxCascadeDepth) return;
	bDispatching = true;
	TArray<uint64> listenerIds;
	{
		FScopeLock lock(&Impl->Lock);
		if (const FImpl::FChannel* channel = Impl->Channels.Find(InKey))
		{
			for (const FImpl::FListener& listener : channel->Listeners) listenerIds.Add(listener.Id);
		}
	}
	const FSignalContext context { InSequence, InDepth };
	for (uint64 listenerId : listenerIds)
	{
		TFunction<void(const FSignalPayload&, const FSignalContext&)> callback;
		{
			FScopeLock lock(&Impl->Lock);
			FImpl::FChannel* channel = Impl->Channels.Find(InKey);
			if (!channel) continue;
			const FImpl::FListener* listener = channel->Listeners.FindByPredicate([listenerId](const FImpl::FListener& value) { return value.Id == listenerId; });
			if (!listener || (listener->Owner.IsValid() == false && listener->Owner.Get() != nullptr)) continue;
			callback = listener->Callback;
		}
		if (callback) callback(InPayload, context);
	}
	bDispatching = false;
	TArray<FImpl::FQueuedSignal> next;
	{
		FScopeLock lock(&Impl->Lock);
		next = MoveTemp(Impl->Reentrant);
		Impl->Reentrant.Reset();
	}
	int32 dispatches = 0;
	for (const FImpl::FQueuedSignal& signal : next)
	{
		if (++dispatches > Limits.MaxDispatchesPerRoot) break;
		Dispatch(signal.Key, signal.Payload, signal.Sequence, InDepth + 1);
	}
}
