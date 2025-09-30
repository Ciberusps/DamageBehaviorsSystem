// Pavel Penkov 2025 All Rights Reserved.

using UnrealBuildTool;

public class DamageBehaviorsSystem : ModuleRules
{
	public DamageBehaviorsSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",

				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks", 
				
				"DeveloperSettings",
				
				"PhysicsCore"
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AIModule"
			}
			);
	}
}
