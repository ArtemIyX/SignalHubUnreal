#include "SignalHubBlueprintTypes.h"

#include "UObject/UnrealType.h"

#include <cmath>

namespace
{
template <typename TValue>
FSignalKeyBuildResult MakeKeyResult(const TValue& InValue)
{
	return { ESignalValueBuildResult::Success, MakeSignalKey(InValue), {} };
}

template <typename TValue>
FSignalPayloadBuildResult MakePayloadResult(const TValue& InValue)
{
	return { ESignalValueBuildResult::Success, MakeSignalPayload(InValue), {} };
}
}

FSignalKeyBuildResult BuildSignalKey(const FProperty* InProperty, const void* InValueAddress)
{
	if (!InProperty || !InValueAddress) return { ESignalValueBuildResult::InvalidValue, {}, TEXT("Key property or value is null.") };
	if (const FNameProperty* property = CastField<FNameProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FStrProperty* property = CastField<FStrProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FBoolProperty* property = CastField<FBoolProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FIntProperty* property = CastField<FIntProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FInt64Property* property = CastField<FInt64Property>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FUInt32Property* property = CastField<FUInt32Property>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FFloatProperty* property = CastField<FFloatProperty>(InProperty))
	{
		float value = property->GetPropertyValue(InValueAddress);
		if (!FMath::IsFinite(value)) return { ESignalValueBuildResult::InvalidValue, {}, TEXT("SignalHub keys cannot contain NaN or infinity.") };
		if (value == 0.0f) value = 0.0f;
		return MakeKeyResult(value);
	}
	if (const FDoubleProperty* property = CastField<FDoubleProperty>(InProperty))
	{
		double value = property->GetPropertyValue(InValueAddress);
		if (!FMath::IsFinite(value)) return { ESignalValueBuildResult::InvalidValue, {}, TEXT("SignalHub keys cannot contain NaN or infinity.") };
		if (value == 0.0) value = 0.0;
		return MakeKeyResult(value);
	}
	if (const FStructProperty* property = CastField<FStructProperty>(InProperty))
	{
		FSignalKey key = MakeSignalStructKey(property->Struct, InValueAddress);
		return key.IsValid() ? FSignalKeyBuildResult { ESignalValueBuildResult::Success, MoveTemp(key), {} } : FSignalKeyBuildResult { ESignalValueBuildResult::InvalidValue, {}, TEXT("Struct key construction failed.") };
	}
	return { ESignalValueBuildResult::UnsupportedType, {}, FString::Printf(TEXT("Unsupported SignalHub key property '%s'."), *InProperty->GetClass()->GetName()) };
}

FSignalPayloadBuildResult BuildSignalPayload(const FProperty* InProperty, const void* InValueAddress)
{
	if (!InProperty || !InValueAddress) return { ESignalValueBuildResult::InvalidValue, {}, TEXT("Payload property or value is null.") };
	if (const FNameProperty* property = CastField<FNameProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FStrProperty* property = CastField<FStrProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FBoolProperty* property = CastField<FBoolProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FIntProperty* property = CastField<FIntProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FInt64Property* property = CastField<FInt64Property>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FUInt32Property* property = CastField<FUInt32Property>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FFloatProperty* property = CastField<FFloatProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FDoubleProperty* property = CastField<FDoubleProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
	if (const FStructProperty* property = CastField<FStructProperty>(InProperty))
	{
		FSignalPayload payload = MakeSignalStructPayload(property->Struct, InValueAddress);
		return payload.IsValid() ? FSignalPayloadBuildResult { ESignalValueBuildResult::Success, MoveTemp(payload), {} } : FSignalPayloadBuildResult { ESignalValueBuildResult::InvalidValue, {}, TEXT("Struct payload construction failed.") };
	}
	return { ESignalValueBuildResult::UnsupportedType, {}, FString::Printf(TEXT("Unsupported SignalHub payload property '%s'."), *InProperty->GetClass()->GetName()) };
}
