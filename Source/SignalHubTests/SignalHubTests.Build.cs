using UnrealBuildTool;

public class SignalHubTests : ModuleRules
{
	public SignalHubTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "SignalHub", "StructUtils" });
	}
}
