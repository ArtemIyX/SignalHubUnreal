#include "SignalHubBlueprintTypes.h"

#include "UObject/UnrealType.h"

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
	if (const FFloatProperty* property = CastField<FFloatProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
	if (const FDoubleProperty* property = CastField<FDoubleProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
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
