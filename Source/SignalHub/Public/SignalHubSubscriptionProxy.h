#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SignalSubscription.h"
#include "SignalHubSubscriptionProxy.generated.h"

class USignalHubSubsystem;

UCLASS(BlueprintType)
class SIGNALHUB_API USignalHubSubscription final : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override;
	UFUNCTION(BlueprintCallable, Category = "Signal Hub", meta = (ToolTip = "Stops this subscription. Safe to call repeatedly."))
	bool Cancel();

	UFUNCTION(BlueprintPure, Category = "Signal Hub", meta = (ToolTip = "Returns true while this object owns an active SignalHub subscription."))
	bool IsActive() const;

	void Initialize(USignalHubSubsystem* InHub, FSignalSubscriptionHandle InHandle);
	void Invalidate();

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<USignalHubSubsystem> Hub;

	FSignalSubscriptionHandle Handle;
};
