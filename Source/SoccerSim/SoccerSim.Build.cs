using UnrealBuildTool;

public class SoccerSim : ModuleRules
{
    public SoccerSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "PhysicsCore",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "ChaosCore",
            "ChaosSolverEngine"
        });

        bEnableExceptions = true;
    }
}
