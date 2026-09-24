#pragma once

#include "CoreMinimal.h"
#include "SignalKey.h"
#include "SignalPayload.h"

class FProperty;

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