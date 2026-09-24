#pragma once

#include "CoreMinimal.h"
#include "SignalKey.h"
#include "SignalPayload.h"
#include "SignalHubBlueprintTypes.generated.h"
#include "SingalHubBlueprintTypes.generated.h"

class FProperty;

UENUM(BlueprintType)
enum class ESignalBlueprintValueKind : uint8
{
	Invalid,
	Bool,
	Byte,
	Int32,
	Int64,
	UInt32,
	Float,
	Double,
	Name,
	String,
	Struct
};

USTRUCT(BlueprintType)
struct SIGNALHUB_API FSignalBlueprintType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Hub")
	ESignalBlueprintValueKind Kind = ESignalBlueprintValueKind::Invalid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Signal Hub")
	TObjectPtr<UScriptStruct> StructType = nullptr;

	bool IsValid() const { return Kind != ESignalBlueprintValueKind::Invalid && (Kind != ESignalBlueprintValueKind::Struct || StructType != nullptr); }
};

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
