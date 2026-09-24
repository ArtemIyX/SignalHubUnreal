#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SignalHubBlueprintTypes.h"
#include "SignalSubscription.h"
#include "SignalHubSubscriptionProxy.generated.h"

class USignalHubSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSignalHubBlueprintSignal, const FSignalBlueprintEnvelope&, InEnvelope);

UCLASS(BlueprintType)
class SIGNALHUB_API USignalHubSubscription final : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Signal Hub")
	FSignalHubBlueprintSignal OnSignal;

	virtual void Activate() override;
	UFUNCTION(BlueprintCallable, Category = "Signal Hub", meta = (ToolTip = "Stops this subscription. Safe to call repeatedly."))
	bool Cancel();

	UFUNCTION(BlueprintPure, Category = "Signal Hub", meta = (ToolTip = "Returns true while this object owns an active SignalHub subscription."))
	bool IsActive() const;

	void Initialize(USignalHubSubsystem* InHub, FSignalSubscriptionHandle InHandle);
	void Invalidate();
	void Deliver(const FSignalPayload& InPayload, const FSignalContext& InContext);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<USignalHubSubsystem> Hub;

	FSignalSubscriptionHandle Handle;
};
