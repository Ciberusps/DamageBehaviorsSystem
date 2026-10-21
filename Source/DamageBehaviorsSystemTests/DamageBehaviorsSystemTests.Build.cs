// Tests for the DamageBehaviorsSystem plugin.

using UnrealBuildTool;

public class DamageBehaviorsSystemTests : ModuleRules
{
	public DamageBehaviorsSystemTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Module under test
			"DamageBehaviorsSystem"
		});
	}
}
