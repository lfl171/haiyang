using UnrealBuildTool;
using System.Collections.Generic;

public class BlackTideTarget : TargetRules
{
	public BlackTideTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BlackTide");
	}
}
