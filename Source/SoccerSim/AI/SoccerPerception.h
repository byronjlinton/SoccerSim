#pragma once

#include "CoreMinimal.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerPerception.generated.h"

class ASoccerPlayerPawn;
class ASoccerBall;

// ============================================================
// Perceived Nearby Player
// ============================================================

USTRUCT(BlueprintType)
struct FPerceivedPlayer
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<ASoccerPlayerPawn> Pawn = nullptr;

    /** Distance to this player in cm */
    UPROPERTY(BlueprintReadOnly)
    float Distance = 0.0f;

    /** Direction to this player (normalized, local to perceiver) */
    UPROPERTY(BlueprintReadOnly)
    FVector Direction = FVector::ForwardVector;

    /** Whether a passing lane to this player is clear (no opponents blocking) */
    UPROPERTY(BlueprintReadOnly)
    bool bPassingLaneClear = false;

    /** Positional value of this player (higher = more attacking/valuable position) */
    UPROPERTY(BlueprintReadOnly)
    float PositionalValue = 0.0f;
};

// ============================================================
// Spatial Threat Assessment
// ============================================================

USTRUCT(BlueprintType)
struct FThreatAssessment
{
    GENERATED_BODY()

    /** Distance from ball carrier to our goal (cm) */
    UPROPERTY(BlueprintReadOnly)
    float BallCarrierDistToOurGoal = 0.0f;

    /** Number of opponents behind our defensive line */
    UPROPERTY(BlueprintReadOnly)
    int32 OpponentsBehindDefLine = 0;

    /** Number of our defenders between ball and goal */
    UPROPERTY(BlueprintReadOnly)
    int32 DefendersBetweenBallAndGoal = 0;

    /** How dangerous the current situation is (0 = safe, 1 = critical) */
    UPROPERTY(BlueprintReadOnly)
    float ThreatLevel = 0.0f;

    /** Is the ball carrier sprinting toward our goal? */
    UPROPERTY(BlueprintReadOnly)
    bool bIsCounterAttack = false;

    /** Estimated time (seconds) for ball carrier to reach shooting position */
    UPROPERTY(BlueprintReadOnly)
    float TimeToThreat = 10.0f;
};

// ============================================================
// Cached Perception Data (updated per AI tick)
// ============================================================

USTRUCT(BlueprintType)
struct FSoccerPerception
{
    GENERATED_BODY()

    // ── Ball state ──

    /** Ball location */
    UPROPERTY(BlueprintReadOnly)
    FVector BallLocation = FVector::ZeroVector;

    /** Ball velocity */
    UPROPERTY(BlueprintReadOnly)
    FVector BallVelocity = FVector::ZeroVector;

    /** Ball speed in cm/s */
    UPROPERTY(BlueprintReadOnly)
    float BallSpeed = 0.0f;

    /** Distance from this player to ball (cm) */
    UPROPERTY(BlueprintReadOnly)
    float DistToBall = 0.0f;

    /** Am I the nearest teammate to the ball? */
    UPROPERTY(BlueprintReadOnly)
    bool bAmINearestToBall = false;

    /** Do I have possession? */
    UPROPERTY(BlueprintReadOnly)
    bool bIHaveBall = false;

    /** Does my team have the ball? */
    UPROPERTY(BlueprintReadOnly)
    bool bMyTeamHasBall = false;

    // ── Positional state ──

    /** My formation position (world space) */
    UPROPERTY(BlueprintReadOnly)
    FVector FormationPosition = FVector::ZeroVector;

    /** My shifted formation position (adjusted for ball) */
    UPROPERTY(BlueprintReadOnly)
    FVector ShiftedFormationPosition = FVector::ZeroVector;

    /** Distance from my position to my shifted formation slot */
    UPROPERTY(BlueprintReadOnly)
    float DistFromFormationSlot = 0.0f;

    /** Am I in the attacking half? */
    UPROPERTY(BlueprintReadOnly)
    bool bInAttackingHalf = false;

    // ── Nearby players ──

    /** Nearby teammates (sorted by distance) */
    UPROPERTY(BlueprintReadOnly)
    TArray<FPerceivedPlayer> NearbyTeammates;

    /** Nearby opponents (sorted by distance) */
    UPROPERTY(BlueprintReadOnly)
    TArray<FPerceivedPlayer> NearbyOpponents;

    /** Nearest opponent (distance) */
    UPROPERTY(BlueprintReadOnly)
    float NearestOpponentDist = 0.0f;

    /** Number of opponents within pressing range (500cm) */
    UPROPERTY(BlueprintReadOnly)
    int32 OpponentsInPressRange = 0;

    // ── Tactical state ──

    /** Is my team pressing? (from TeamBrain) */
    UPROPERTY(BlueprintReadOnly)
    bool bTeamIsPressing = false;

    /** Is my team counter-attacking? */
    UPROPERTY(BlueprintReadOnly)
    bool bTeamIsCounterAttacking = false;

    /** My assigned marking target (if any) */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<ASoccerPlayerPawn> MarkTarget = nullptr;

    /** Threat assessment for defensive decision-making */
    UPROPERTY(BlueprintReadOnly)
    FThreatAssessment Threat;

    // ── Personal state ──

    /** Current stamina (0-100) */
    UPROPERTY(BlueprintReadOnly)
    float Stamina = 100.0f;

    /** Am I the goalkeeper? */
    UPROPERTY(BlueprintReadOnly)
    bool bIsGoalkeeper = false;

    /** My position on the field */
    UPROPERTY(BlueprintReadOnly)
    EPlayerPosition Position = EPlayerPosition::CM;

    // ── Helpers ──

    /** Get the best pass target (highest combined score of lane clear + positional value + distance) */
    SOCCERSIM_API FPerceivedPlayer GetBestPassTarget() const;

    /** Check if a passing lane is clear to a specific location */
    SOCCERSIM_API bool IsPassingLaneClearTo(FVector Target) const;

    /** Calculate space available ahead (distance to nearest opponent in forward direction) */
    SOCCERSIM_API float GetSpaceAhead() const;

    /** Get angle to opponent goal in degrees */
    SOCCERSIM_API float GetAngleToGoal(ETeamId MyTeam) const;

    /** Get distance to opponent goal in cm */
    SOCCERSIM_API float GetDistToGoal(ETeamId MyTeam) const;
};
