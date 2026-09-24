#include "SignalHubBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "SignalHubSubsystem.h"

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

USignalHubSubsystem* USignalHubBlueprintLibrary::ResolveHub(const UObject* WorldContextObject)
{
	if (!WorldContextObject || !GEngine) return nullptr;
	UWorld* world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* gameInstance = world ? world->GetGameInstance() : nullptr;
	return gameInstance ? gameInstance->GetSubsystem<USignalHubSubsystem>() : nullptr;
}
