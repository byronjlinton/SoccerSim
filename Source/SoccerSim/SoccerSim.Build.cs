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
            "AIModule",
            "GameplayTasks",
            "NavigationSystem",
            "Niagara",
            "UMG",
            "PhysicsCore",
            "Slate",
            "SlateCore",
            "GameplayTags",
            "GeometryFramework",
            "GeometryCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "ChaosCore",
            "ChaosSolverEngine",
            "DynamicMesh"
        });

        // Enable exceptions for physics math
        bEnableExceptions = true;
    }
}
