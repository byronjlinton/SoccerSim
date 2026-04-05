using UnrealBuildTool;
using System.Collections.Generic;

public class SoccerSimTarget : TargetRules
{
    public SoccerSimTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SoccerSim");
    }
}
