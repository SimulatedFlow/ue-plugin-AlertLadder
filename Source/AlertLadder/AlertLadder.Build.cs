// Copyright 2026 Silvan Teufel. All Rights Reserved.

using UnrealBuildTool;

public class AlertLadder : ModuleRules
{
	public AlertLadder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			// AActor, UActorComponent and DrawDebugHelpers for the demo level.
			"Engine",

			// UAlertLadderSettings is a UDeveloperSettings, so the thresholds and timings sit
			// under Project Settings > Plugins > AlertLadder without an editor module.
			"DeveloperSettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// Deliberately NOT here:
		//   AIModule / PerceptionSystem - AlertLadder does not see, hear or smell anything. You
		//                                 report a stimulus from whatever perception you already
		//                                 use; the plugin decides what it means and for how long.
	}
}
