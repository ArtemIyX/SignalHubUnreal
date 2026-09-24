#pragma once

#include "CoreMinimal.h"
#include "SignalKey.h"
#include "SignalPayload.h"

class FProperty;

USTRUCT(BlueprintType)
struct SIGNALHUB_API FSignalBlueprintEnvelope
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Signal Hub")
	FSignalContext Context;

	static FSignalBlueprintEnvelope Make(const FSignalPayload& InPayload, const FSignalContext& InContext);
	bool TryExtract(const FProperty* InProperty, void* OutValueAddress) const;

private:
	FSignalPayload Payload;
};

enum class ESignalValueBuildResult : uint8
{
	Success,
	InvalidValue,
	UnsupportedType
};

struct SIGNALHUB_API FSignalKeyBuildResult
{
	ESignalValueBuildResult Result = ESignalValueBuildResult::InvalidValue;
	FSignalKey Key;
	FString Error;

	bool IsSuccess() const { return Result == ESignalValueBuildResult::Success; }
};

struct SIGNALHUB_API FSignalPayloadBuildResult
{
	ESignalValueBuildResult Result = ESignalValueBuildResult::InvalidValue;
	FSignalPayload Payload;
	FString Error;

	bool IsSuccess() const { return Result == ESignalValueBuildResult::Success; }
};

SIGNALHUB_API FSignalKeyBuildResult BuildSignalKey(const FProperty* InProperty, const void* InValueAddress);
SIGNALHUB_API FSignalPayloadBuildResult BuildSignalPayload(const FProperty* InProperty, const void* InValueAddress);
SIGNALHUB_API bool ExtractSignalPayload(const FSignalPayload& InPayload, const FProperty* InProperty, void* OutValueAddress);
