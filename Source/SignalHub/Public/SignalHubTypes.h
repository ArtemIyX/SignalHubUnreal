#pragma once

#include "CoreMinimal.h"
#include "SignalHubTypes.generated.h"

UENUM(BlueprintType)
enum class ESignalPublishResult : uint8
{
	Delivered,
	Queued,
	QueuedReentrant,
	NoListeners,
	InvalidKey,
	PayloadTypeMismatch,
	WrongThreadForPayload,
	QueueFull,
	CascadeLimit,
	ShuttingDown,
	InvalidWorldContext
};

UENUM(BlueprintType)
enum class ESignalSubscribeResult : uint8
{
	Bound,
	InvalidKey,
	PayloadTypeMismatch,
	InvalidCallback,
	WrongThread,
	ShuttingDown
};

USTRUCT(BlueprintType)
struct SIGNALHUB_API FSignalHubLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Hub|Limits", meta = (ClampMin = "1", ToolTip = "Maximum worker-thread signals waiting for game-thread delivery."))
	int32 MaxQueuedSignals = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Hub|Limits", meta = (ClampMin = "1", ToolTip = "Maximum queued signals delivered during one game-thread tick."))
	int32 MaxSignalsPerTick = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Hub|Limits", meta = (ClampMin = "1", ToolTip = "Maximum causal nested publication depth."))
	int32 MaxCascadeDepth = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Hub|Limits", meta = (ClampMin = "1", ToolTip = "Maximum deliveries caused by one root publication."))
	int32 MaxDispatchesPerRoot = 4096;

	void Clamp();
};

USTRUCT(BlueprintType)
struct SIGNALHUB_API FSignalContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Signal Hub", meta = (ToolTip = "Monotonically increasing accepted publication sequence."))
	int64 Sequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Signal Hub", meta = (ToolTip = "Causal depth of this delivery."))
	int32 CascadeDepth = 0;
};

DECLARE_LOG_CATEGORY_EXTERN(LogSignalHub, Log, All);
