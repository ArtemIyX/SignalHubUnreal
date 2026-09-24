#include "SignalHubBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "SignalHubSubsystem.h"
#include "SignalHubBlueprintTypes.h"
#include "UObject/Stack.h"
#include "UObject/UnrealType.h"

FSignalKey USignalHubBlueprintLibrary::MakeNameSignalKey(FName InValue)
{
	return MakeSignalKey(InValue);
}

bool USignalHubBlueprintLibrary::IsNameSignalBound(const UObject* WorldContextObject, FName InKey)
{
	if (USignalHubSubsystem* hub = ResolveHub(WorldContextObject)) return hub->IsBound(MakeSignalKey(InKey));
	return false;
}

FSignalHubDiagnostics USignalHubBlueprintLibrary::GetSignalHubDiagnostics(const UObject* WorldContextObject)
{
	if (USignalHubSubsystem* hub = ResolveHub(WorldContextObject)) return hub->GetDiagnostics();
	return {};
}

ESignalPublishResult USignalHubBlueprintLibrary::PublishSignalWildcard(const UObject* WorldContextObject, const int32& InKey, const int32& InPayload)
{
	return ESignalPublishResult::InvalidKey;
}

DEFINE_FUNCTION(USignalHubBlueprintLibrary::execPublishSignalWildcard)
{
	P_GET_OBJECT(UObject, worldContextObject);
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* keyProperty = Stack.MostRecentProperty;
	const void* keyAddress = Stack.MostRecentPropertyAddress;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* payloadProperty = Stack.MostRecentProperty;
	const void* payloadAddress = Stack.MostRecentPropertyAddress;
	P_FINISH;
	P_NATIVE_BEGIN;
	const FSignalKeyBuildResult key = BuildSignalKey(keyProperty, keyAddress);
	const FSignalPayloadBuildResult payload = BuildSignalPayload(payloadProperty, payloadAddress);
	USignalHubSubsystem* hub = ResolveHub(worldContextObject);
	*(ESignalPublishResult*)RESULT_PARAM = !key.IsSuccess() ? ESignalPublishResult::InvalidKey : !payload.IsSuccess() ? ESignalPublishResult::PayloadTypeMismatch : hub ? hub->PublishPayload(key.Key, payload.Payload) : ESignalPublishResult::InvalidWorldContext;
	P_NATIVE_END;
}

FSignalKey USignalHubBlueprintLibrary::MakeSignalKeyWildcard(const int32& InValue)
{
	return {};
}

DEFINE_FUNCTION(USignalHubBlueprintLibrary::execMakeSignalKeyWildcard)
{
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* property = Stack.MostRecentProperty;
	const void* address = Stack.MostRecentPropertyAddress;
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FSignalKey*)RESULT_PARAM = BuildSignalKey(property, address).Key;
	P_NATIVE_END;
}

USignalHubSubsystem* USignalHubBlueprintLibrary::ResolveHub(const UObject* WorldContextObject)
{
	if (!WorldContextObject || !GEngine) return nullptr;
	UWorld* world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* gameInstance = world ? world->GetGameInstance() : nullptr;
	return gameInstance ? gameInstance->GetSubsystem<USignalHubSubsystem>() : nullptr;
}
