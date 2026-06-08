// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AISurvivorArena : ModuleRules
{
	public AISurvivorArena(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"AISurvivorArena",
			"AISurvivorArena/Variant_Platforming",
			"AISurvivorArena/Variant_Platforming/Animation",
			"AISurvivorArena/Variant_Combat",
			"AISurvivorArena/Variant_Combat/AI",
			"AISurvivorArena/Variant_Combat/Animation",
			"AISurvivorArena/Variant_Combat/Gameplay",
			"AISurvivorArena/Variant_Combat/Interfaces",
			"AISurvivorArena/Variant_Combat/UI",
			"AISurvivorArena/Variant_SideScrolling",
			"AISurvivorArena/Variant_SideScrolling/AI",
			"AISurvivorArena/Variant_SideScrolling/Gameplay",
			"AISurvivorArena/Variant_SideScrolling/Interfaces",
			"AISurvivorArena/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
