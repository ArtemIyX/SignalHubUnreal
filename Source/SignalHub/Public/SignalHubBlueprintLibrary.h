#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SignalKey.h"
#include "SignalHubBlueprintLibrary.generated.h"

class USignalHubSubsystem;

UCLASS()
class SIGNALHUB_API USignalHubBlueprintLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Signal Hub|Key", meta = (ToolTip = "Creates an opaque runtime-only SignalHub key from an exact name."))
	static FSignalKey MakeNameSignalKey(FName InValue);

	UFUNCTION(BlueprintPure, Category = "Signal Hub", meta = (WorldContext = "WorldContextObject", ToolTip = "Returns whether the exact name key has a current listener in this game instance."))
	static bool IsNameSignalBound(const UObject* WorldContextObject, FName InKey);

	UFUNCTION(BlueprintPure, Category = "Signal Hub|Diagnostics", meta = (WorldContext = "WorldContextObject", ToolTip = "Returns a read-only snapshot for this game instance."))
	static FSignalHubDiagnostics GetSignalHubDiagnostics(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Signal Hub", meta = (BlueprintInternalUseOnly = "true", CustomStructureParam = "Key,Payload", WorldContext = "WorldContextObject"))
	static ESignalPublishResult PublishSignalWildcard(const UObject* WorldContextObject, const int32& InKey, const int32& InPayload);
	DECLARE_FUNCTION(execPublishSignalWildcard);

	UFUNCTION(BlueprintPure, CustomThunk, Category = "Signal Hub|Key", meta = (BlueprintInternalUseOnly = "true", CustomStructureParam = "Value"))
	static FSignalKey MakeSignalKeyWildcard(const int32& InValue);
	DECLARE_FUNCTION(execMakeSignalKeyWildcard);

	static USignalHubSubsystem* ResolveHub(const UObject* WorldContextObject);
};
