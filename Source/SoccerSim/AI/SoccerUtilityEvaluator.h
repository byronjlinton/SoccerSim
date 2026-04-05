#pragma once

#include "CoreMinimal.h"
#include "SoccerSim/AI/SoccerAction.h"
#include "SoccerSim/AI/SoccerPerception.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerUtilityEvaluator.generated.h"

class ASoccerPlayerPawn;
class ASoccerBall;
class ASoccerGameMode;

// ============================================================
// Utility Evaluator Weights (per-position overrides in data asset)
// ============================================================

USTRUCT(BlueprintType)
struct FSoccerActionWeights
{
    GENERATED_BODY()

    /** Base score for pass action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float PassWeight = 1.0f;

    /** Base score for shot */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float ShotWeight = 1.0f;

    /** Base score for dribble */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float DribbleWeight = 1.0f;

    /** Base score for tackle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float TackleWeight = 1.0f;

    /** Base score for holding position */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float HoldPositionWeight = 0.6f;

    /** Base score for making a run */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float MakeRunWeight = 0.8f;

    /** Base score for pressing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float PressWeight = 0.9f;

    /** Base score for covering a defensive lane */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float CoverLaneWeight = 0.7f;

    /** Base score for crossing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float CrossWeight = 1.0f;

    /** Base score for clearing the ball */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"))
    float ClearWeight = 0.8f;

    // Position preset weights
    static FSoccerActionWeights ForwardWeights()
    {
        FSoccerActionWeights W;
        W.ShotWeight = 1.5f;
        W.MakeRunWeight = 1.2f;
        W.PassWeight = 0.8f;
        W.CoverLaneWeight = 0.3f;
        return W;
    }

    static FSoccerActionWeights MidfielderWeights()
    {
        FSoccerActionWeights W;
        W.PassWeight = 1.3f;
        W.MakeRunWeight = 1.0f;
        W.DribbleWeight = 1.1f;
        return W;
    }

    static FSoccerActionWeights DefenderWeights()
    {
        FSoccerActionWeights W;
        W.TackleWeight = 1.4f;
        W.CoverLaneWeight = 1.3f;
        W.ClearWeight = 1.2f;
        W.PassWeight = 0.7f;
        W.MakeRunWeight = 0.3f;
        W.ShotWeight = 0.2f;
        return W;
    }

    static FSoccerActionWeights GoalkeeperWeights()
    {
        FSoccerActionWeights W;
        W.ClearWeight = 1.5f;
        W.HoldPositionWeight = 1.0f;
        W.PassWeight = 0.5f;
        W.TackleWeight = 0.3f;
        W.ShotWeight = 0.0f;
        W.MakeRunWeight = 0.0f;
        return W;
    }
};

// ============================================================
// Soccer Utility Evaluator
// ============================================================

UCLASS(BlueprintType)
class SOCCERSIM_API USoccerUtilityEvaluator : public UObject
{
    GENERATED_BODY()

public:
    USoccerUtilityEvaluator();

    /** Update perception data from current game state */
    void UpdatePerception(ASoccerPlayerPawn* Pawn, ASoccerBall* Ball, float DeltaTime);

    /** Evaluate all possible actions and return the best one */
    FSoccerAction EvaluateBestAction(ASoccerPlayerPawn* Pawn);

    /** Get current perception data (read-only) */
    const FSoccerPerception& GetPerception() const { return Perception; }

    /** Action weights - tuneable per position */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Utility")
    FSoccerActionWeights Weights;

    /** Perception radius for scanning nearby players */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
    float PerceptionRadius = 2500.0f;

    /** Maximum pass distance (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Passing")
    float MaxPassDistance = 3500.0f;

    /** Minimum pass distance (cm) - too close = bad pass */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Passing")
    float MinPassDistance = 300.0f;

    /** Shot distance range (cm). Beyond this, shot utility drops. */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Shooting")
    float MaxShotDistance = 2500.0f;

    /** Minimum shot angle (degrees) for a viable shot */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Shooting")
    float MinShotAngle = 5.0f;

private:
    /** Cached perception data */
    UPROPERTY(Transient)
    FSoccerPerception Perception;

    // ── Individual action scorers ──

    FSoccerAction ScorePass(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreShot(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreDribble(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreTackle(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreHoldPosition(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreMakeRun(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScorePress(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreCoverLane(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreCross(ASoccerPlayerPawn* Pawn);
    FSoccerAction ScoreClear(ASoccerPlayerPawn* Pawn);

    // ── Helpers ──

    /** Update the nearby players lists */
    void ScanNearbyPlayers(ASoccerPlayerPawn* Pawn, ASoccerGameMode* GM);

    /** Check if passing lane is clear to a target location */
    bool CheckPassingLaneClear(ASoccerPlayerPawn* Pawn, FVector Target) const;

    /** Calculate positional value (how good a position is for attacking) */
    float CalculatePositionalValue(FVector Location, ETeamId Team) const;
};
