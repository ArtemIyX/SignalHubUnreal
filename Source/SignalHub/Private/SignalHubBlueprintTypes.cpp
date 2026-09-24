#include "SignalHubBlueprintTypes.h"

#include "UObject/UnrealType.h"

#include <cmath>

FSignalBlueprintEnvelope FSignalBlueprintEnvelope::Make(const FSignalPayload& InPayload, const FSignalContext& InContext)
{
	FSignalBlueprintEnvelope result;
	result.Payload = InPayload;
	result.Context = InContext;
	return result;
}

bool FSignalBlueprintEnvelope::TryExtract(const FProperty* InProperty, void* OutValueAddress) const
{
	return ExtractSignalPayload(Payload, InProperty, OutValueAddress);
}

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
	if (const FByteProperty* property = CastField<FByteProperty>(InProperty)) return MakeKeyResult(property->GetPropertyValue(InValueAddress));
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
	if (const FByteProperty* property = CastField<FByteProperty>(InProperty)) return MakePayloadResult(property->GetPropertyValue(InValueAddress));
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

bool ExtractSignalPayload(const FSignalPayload& InPayload, const FProperty* InProperty, void* OutValueAddress)
{
	if (!InProperty || !OutValueAddress) return false;
	if (const FNameProperty* nameProperty = CastField<FNameProperty>(InProperty))
	{
		if (const FName* value = InPayload.TryGet<FName>()) { nameProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FStrProperty* stringProperty = CastField<FStrProperty>(InProperty))
	{
		if (const FString* value = InPayload.TryGet<FString>()) { stringProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FBoolProperty* boolProperty = CastField<FBoolProperty>(InProperty))
	{
		if (const bool* value = InPayload.TryGet<bool>()) { boolProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FByteProperty* byteProperty = CastField<FByteProperty>(InProperty))
	{
		if (const uint8* value = InPayload.TryGet<uint8>()) { byteProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FIntProperty* intProperty = CastField<FIntProperty>(InProperty))
	{
		if (const int32* value = InPayload.TryGet<int32>()) { intProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FInt64Property* int64Property = CastField<FInt64Property>(InProperty))
	{
		if (const int64* value = InPayload.TryGet<int64>()) { int64Property->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FUInt32Property* uint32Property = CastField<FUInt32Property>(InProperty))
	{
		if (const uint32* value = InPayload.TryGet<uint32>()) { uint32Property->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FFloatProperty* floatProperty = CastField<FFloatProperty>(InProperty))
	{
		if (const float* value = InPayload.TryGet<float>()) { floatProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FDoubleProperty* doubleProperty = CastField<FDoubleProperty>(InProperty))
	{
		if (const double* value = InPayload.TryGet<double>()) { doubleProperty->SetPropertyValue(OutValueAddress, *value); return true; }
	}
	else if (const FStructProperty* structProperty = CastField<FStructProperty>(InProperty))
	{
		if (const void* value = InPayload.TryGetStruct(structProperty->Struct)) { structProperty->Struct->CopyScriptStruct(OutValueAddress, value); return true; }
	}
	return false;
}
