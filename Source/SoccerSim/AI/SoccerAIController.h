#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerSim/AI/SoccerAction.h"
#include "SoccerAIController.generated.h"

class ASoccerPlayerPawn;
class ASoccerBall;
class USoccerUtilityEvaluator;

UCLASS()
class SOCCERSIM_API ASoccerAIController : public AAIController
{
    GENERATED_BODY()

public:
    ASoccerAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void Tick(float DeltaTime) override;

    /** How often (seconds) the AI re-evaluates its decision */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float DecisionInterval = 0.25f;

    /** Distance threshold for chasing ball (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float ChaseDistanceThreshold = 500.0f;

    /** Distance threshold for holding formation position (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    float PositionTolerance = 100.0f;

    /** When true, uses the new utility AI system. Falls back to simple state machine when false. */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    bool bUseUtilityAI = true;

protected:
    UPROPERTY(Transient)
    TObjectPtr<ASoccerPlayerPawn> SoccerPawn;

    UPROPERTY(Transient)
    TObjectPtr<ASoccerBall> Ball;

    /** Utility evaluator instance */
    UPROPERTY(Transient)
    TObjectPtr<USoccerUtilityEvaluator> UtilityEvaluator;

    /** Current AI state (used by fallback state machine) */
    EPlayerAIState CurrentState = EPlayerAIState::HoldingPosition;

    /** Currently executing action (from utility evaluator) */
    FSoccerAction CurrentAction;

    float TimeSinceLastDecision = 0.0f;

    // ── Decision Making ──

    void MakeDecision(float DeltaTime);
    void MakeDecision_UtilityAI(float DeltaTime);
    void MakeDecision_StateMachine(float DeltaTime);

    // ── Action Execution ──

    void ExecuteAction(const FSoccerAction& Action, float DeltaTime);
    void ExecuteState(EPlayerAIState State, float DeltaTime);

    // ── Movement Helpers ──

    void MoveToLocation(FVector Target, float AcceptanceRadius = 100.0f);
    void MoveTowardBall();
    void MoveToFormationPosition();

    // ── Perception Helpers ──

    bool AmINearestTeammateToBall() const;
    FVector GetFormationWorldPosition() const;
    FVector GetShiftedPosition() const;

    /** Smoothed movement input to prevent jittery direction changes */
    FVector2D SmoothedMovementInput = FVector2D::ZeroVector;

    ASoccerBall* FindBall() const;
};
