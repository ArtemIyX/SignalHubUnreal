#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SignalHubTypes.h"
#include "SignalHubSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Signal Hub"))
class SIGNALHUB_API USignalHubSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Limits", meta = (ToolTip = "Runtime queue and dispatch limits applied to each new SignalHub game-instance subsystem."))
	FSignalHubLimits Limits;
};
