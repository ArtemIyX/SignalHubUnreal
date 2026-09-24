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
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	#endif

	UPROPERTY(Config, EditAnywhere, Category = "Limits", meta = (ToolTip = "Runtime queue and dispatch limits applied to each new SignalHub game-instance subsystem."))
	FSignalHubLimits Limits;
};