using UnrealBuildTool;

public class SignalHubEditor : ModuleRules
{
	public SignalHubEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "SignalHub", "BlueprintGraph", "KismetCompiler", "UnrealEd" });
	}
}
