#pragma once

#include "CoreMinimal.h"
#include "SignalHubTypes.h"
#include "SignalSubscription.generated.h"

USTRUCT(BlueprintType)
struct SIGNALHUB_API FSignalSubscriptionHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Signal Hub")
	int64 Id = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Signal Hub")
	int32 Generation = 0;

	bool IsValid() const { return Id != 0; }

	void Reset()
	{
		Id = 0;
		Generation = 0;
	}
};

struct SIGNALHUB_API FSignalSubscribeOutcome
{
	ESignalSubscribeResult Result = ESignalSubscribeResult::InvalidCallback;
	FSignalSubscriptionHandle Handle;
	bool IsBound() const { return Result == ESignalSubscribeResult::Bound && Handle.IsValid(); }
};