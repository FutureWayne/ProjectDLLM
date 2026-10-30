// Copyright Ludens Studio. All Rights Reserved.

using UnrealBuildTool;

public class ArenaEditor : ModuleRules
{
	public ArenaEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"InputCore",
			"Arena",
			"CommonGame",
			"CommonUI",
			"DataRegistry",
			"AsyncMixin",
			"UMG",
			"UMGEditor",
			"MovieScene",
			"UnrealEd",
			"BlueprintGraph",
			"KismetCompiler",
			"GameplayAbilities",
			"GameplayTags",
			"AIModule",
			"GameplayMessageRuntime",
			"EngineSettings",
			"OnlineSubsystemUtils",
			"Niagara",
			"NiagaraEditor",
			"AssetRegistry",
			"RenderCore",
			"Slate",
            "SlateCore"
		});
	}
}
