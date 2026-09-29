using UnrealBuildTool;
using System.Collections.Generic;

public class BlackTideEditorTarget : TargetRules
{
	public BlackTideEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BlackTide");
	}
}
