#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerGoalkeeperAI.generated.h"

class ASoccerPlayerPawn;
class ASoccerBall;

// ============================================================
// Goalkeeper AI Component
// ============================================================

UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class SOCCERSIM_API USoccerGoalkeeperAI : public UActorComponent
{
    GENERATED_BODY()

public:
    USoccerGoalkeeperAI();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    /** Get the ideal GK position based on ball location */
    FVector GetIdealPosition(ASoccerPlayerPawn* GK, ASoccerBall* Ball) const;

    /** Should the GK dive? Returns dive direction if yes, ZeroVector if no */
    FVector ShouldDive(ASoccerPlayerPawn* GK, ASoccerBall* Ball) const;

    /** Should the GK rush out (sweeper keeper)? */
    bool ShouldRushOut(ASoccerPlayerPawn* GK, ASoccerBall* Ball) const;

    // ── Tunable Parameters ──

    /** Base distance from goal line (cm). GK positions this far out. */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Positioning")
    float BaseDepth = 350.0f;

    /** Maximum distance from goal line (cm). GK won't come out further than this. */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Positioning")
    float MaxDepth = 800.0f;

    /** How much GK narrows the angle laterally (0 = stays centered, 1 = matches ball Y) */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Positioning")
    float AngleNarrowing = 0.9f;

    /** Ball speed threshold (cm/s) for diving - must be fast enough to be a shot */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Diving")
    float ShotSpeedThreshold = 1500.0f;

    /** Maximum distance (cm) GK can dive. Beyond this, can't reach. */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Diving")
    float MaxDiveDistance = 400.0f;

    /** Ball distance threshold (cm) for rushing out */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Rush")
    float RushActivationDistance = 1200.0f;

    /** Minimum ball distance from goal for rush out */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Rush")
    float RushMinBallDistFromGoal = 600.0f;

    /** How fast GK moves when not diving (cm/s) */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Movement")
    float GKPositioningSpeed = 400.0f;

    /** How much the GK positions forward when ball is in opponent's half */
    UPROPERTY(EditDefaultsOnly, Category = "AI|GK|Positioning")
    float AdvancedPositioningPush = 200.0f;

private:
    /** GK's team goal center */
    FVector GetGoalCenter(ETeamId Team) const;

    /** Get angle from GK to each post, for positioning */
    void GetPostAngles(FVector GKPos, FVector GoalCenter, float& OutLeftAngle, float& OutRightAngle) const;

    /** Check if ball trajectory will enter the goal */
    bool IsBallHeadingForGoal(ASoccerBall* Ball, FVector GoalCenter) const;
};
