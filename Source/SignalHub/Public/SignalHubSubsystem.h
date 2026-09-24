#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "SignalHubTypes.h"
#include "SignalKey.h"
#include "SignalPayload.h"
#include "SignalSubscription.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SignalHubSubsystem.generated.h"

class UObject;

UCLASS()
class SIGNALHUB_API USignalHubSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual ~USignalHubSubsystem() override;
	virtual void Initialize(FSubsystemCollectionBase& InCollection) override;
	virtual void Deinitialize() override;

	template <typename TPayload, typename TCallback>
	FSignalSubscribeOutcome Subscribe(const FSignalKey& InKey, UObject* InOwner, TCallback&& InCallback)
	{
		using FPayloadType = std::decay_t<TPayload>;
		TFunction<void(const FSignalPayload&, const FSignalContext&)> callback = [fn = Forward<TCallback>(InCallback)](const FSignalPayload& payload, const FSignalContext& context)
		{
			if (const FPayloadType* value = payload.TryGet<FPayloadType>())
			{
				fn(*value, context);
			}
		};
		return SubscribeBoxed(InKey, TSignalTypeTraits<FPayloadType>::Get(), InOwner, MoveTemp(callback));
	}

	template <typename TPayload, typename TKey, typename TCallback, std::enable_if_t<!std::is_same_v<std::decay_t<TKey>, FSignalKey>, int> = 0>
	FSignalSubscribeOutcome Subscribe(TKey&& InKey, UObject* InOwner, TCallback&& InCallback)
	{
		return Subscribe<TPayload>(MakeSignalKey(Forward<TKey>(InKey)), InOwner, Forward<TCallback>(InCallback));
	}

	template <typename TPayload, typename TKey, typename TObject>
	FSignalSubscribeOutcome Subscribe(TKey&& InKey, TObject* InObject, void (TObject::*InCallback)(const TPayload&, const FSignalContext&))
	{
		if (!InObject || !InCallback) return { ESignalSubscribeResult::InvalidCallback, {} };
		return Subscribe<TPayload>(Forward<TKey>(InKey), InObject, [InObject, InCallback](const TPayload& payload, const FSignalContext& context) { (InObject->*InCallback)(payload, context); });
	}

	template <typename TPayload>
	ESignalPublishResult Publish(const FSignalKey& InKey, const TPayload& InPayload)
	{
		using FPayloadType = std::decay_t<TPayload>;
		if (!HasListeners(InKey, TSignalTypeTraits<FPayloadType>::Get()))
		{
			return ESignalPublishResult::NoListeners;
		}
		return PublishBoxed(InKey, MakeSignalPayload(InPayload));
	}

	template <typename TKey, typename TPayload, std::enable_if_t<!std::is_same_v<std::decay_t<TKey>, FSignalKey>, int> = 0>
	ESignalPublishResult Publish(TKey&& InKey, const TPayload& InPayload)
	{
		return Publish(MakeSignalKey(Forward<TKey>(InKey)), InPayload);
	}

	bool Unsubscribe(FSignalSubscriptionHandle InHandle);
	void UnsubscribeAll(UObject* InOwner);
	bool IsBound(const FSignalKey& InKey) const;
	FSignalHubLimits GetLimits() const { return Limits; }
	FSignalHubDiagnostics GetDiagnostics() const;

private:
	FSignalSubscribeOutcome SubscribeBoxed(const FSignalKey& InKey, const FSignalTypeId& InPayloadType, UObject* InOwner, TFunction<void(const FSignalPayload&, const FSignalContext&)>&& InCallback);
	ESignalPublishResult PublishBoxed(const FSignalKey& InKey, const FSignalPayload& InPayload);
	bool HasListeners(const FSignalKey& InKey, const FSignalTypeId& InPayloadType) const;
	bool Tick(float InDeltaTime);
	void Dispatch(const FSignalKey& InKey, const FSignalPayload& InPayload, int64 InSequence, int32 InDepth);
	void DispatchOne(const FSignalKey& InKey, const FSignalPayload& InPayload, int64 InSequence, int32 InDepth);

	struct FImpl;
	TSharedPtr<FImpl> Impl;
	FTSTicker::FDelegateHandle TickHandle;
	FSignalHubLimits Limits;
	TAtomic<bool> bAcceptingPublishes = false;
	bool bDispatching = false;
	int32 CurrentDispatchDepth = 0;
};
