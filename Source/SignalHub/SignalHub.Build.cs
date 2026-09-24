using UnrealBuildTool;

public class SignalHub : ModuleRules
{
	public SignalHub(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "StructUtils", "DeveloperSettings" });
	}
}
