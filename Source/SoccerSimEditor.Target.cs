using UnrealBuildTool;
using System.Collections.Generic;

public class SoccerSimEditorTarget : TargetRules
{
    public SoccerSimEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SoccerSim");
    }
}
