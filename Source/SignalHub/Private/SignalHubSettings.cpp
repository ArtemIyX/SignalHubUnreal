#include "SignalHubSettings.h"

#define LOCTEXT_NAMESPACE "SignalHubSettings"

FName USignalHubSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName USignalHubSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName USignalHubSettings::GetSectionName() const
{
	return TEXT("SignalHub");
}

#if WITH_EDITOR
FText USignalHubSettings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Signal Hub");
}

FText USignalHubSettings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Configures queue and dispatch limits for per-game-instance SignalHub routing.");
}
#endif

#undef LOCTEXT_NAMESPACE
