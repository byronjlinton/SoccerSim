#include "SoccerUtilityEvaluator.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

USoccerUtilityEvaluator::USoccerUtilityEvaluator()
{
    Weights = FSoccerActionWeights::MidfielderWeights();
}

// ============================================================
// Perception Update
// ============================================================

void USoccerUtilityEvaluator::UpdatePerception(ASoccerPlayerPawn* Pawn, ASoccerBall* Ball, float DeltaTime)
{
    if (!Pawn || !Ball) return;

    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return;

    // Ball state
    Perception.BallLocation = Ball->GetActorLocation();
    Perception.BallVelocity = Ball->GetBallVelocity();
    Perception.BallSpeed = Ball->GetBallSpeed();
    Perception.DistToBall = FVector::Dist(Pawn->GetActorLocation(), Ball->GetActorLocation());
    Perception.bIHaveBall = Pawn->HasBall();

    // Team possession
    Perception.bMyTeamHasBall = (Ball->LastTouchTeam == Pawn->GetTeamId());

    // Am I nearest to ball?
    TArray<ASoccerPlayerPawn*>& Team = (Pawn->GetTeamId() == ETeamId::Home) ? GM->HomePlayers : GM->AwayPlayers;
    Perception.bAmINearestToBall = true;
    for (ASoccerPlayerPawn* P : Team)
    {
        if (P == Pawn || !P || P->GetPlayerPosition() == EPlayerPosition::GK) continue;
        if (FVector::Dist(P->GetActorLocation(), Ball->GetActorLocation()) < Perception.DistToBall)
        {
            Perception.bAmINearestToBall = false;
            break;
        }
    }

    // Formation position
    const FFormationSlot& Slot = Pawn->GetFormationSlot();
    Perception.FormationPosition = SoccerField::NormalizedToWorld(Slot.NormalizedPosition, Pawn->GetTeamId());
    Perception.ShiftedFormationPosition = Pawn->GetActorLocation(); // Will be updated by TeamBrain
    Perception.DistFromFormationSlot = FVector::Dist(Pawn->GetActorLocation(), Perception.FormationPosition);

    // Attacking half check
    float X = Pawn->GetActorLocation().X;
    Perception.bInAttackingHalf = (Pawn->GetTeamId() == ETeamId::Home) ? (X > 0.0f) : (X < 0.0f);

    // Personal state
    Perception.Stamina = Pawn->GetCurrentStamina();
    Perception.bIsGoalkeeper = (Pawn->GetPlayerPosition() == EPlayerPosition::GK);
    Perception.Position = Pawn->GetPlayerPosition();

    // Scan nearby players
    ScanNearbyPlayers(Pawn, GM);

    // Threat assessment (defensive)
    if (!Perception.bMyTeamHasBall)
    {
        FVector OurGoal = (Pawn->GetTeamId() == ETeamId::Home)
            ? SoccerField::HomeGoalCenter() : SoccerField::AwayGoalCenter();
        OurGoal.Z = 0.0f;

        Perception.Threat.BallCarrierDistToOurGoal = FVector::Dist(
            Ball->GetActorLocation(), OurGoal);

        Perception.Threat.OpponentsBehindDefLine = 0;
        Perception.Threat.DefendersBetweenBallAndGoal = 0;

        FVector BallToGoal = (OurGoal - Ball->GetActorLocation()).GetSafeNormal();
        for (const FPerceivedPlayer& Opp : Perception.NearbyOpponents)
        {
            if (Opp.Distance > 3000.0f) continue;
            FVector ToOppFromGoal = Opp.Pawn ? (Opp.Pawn->GetActorLocation() - OurGoal) : FVector::ZeroVector;
            // Is opponent between ball and goal?
            if (FVector::DotProduct(ToOppFromGoal, BallToGoal) > 0.0f)
            {
                Perception.Threat.OpponentsBehindDefLine++;
            }
        }

        // Simple threat scoring
        float ThreatFromDist = FMath::Clamp(1.0f - Perception.Threat.BallCarrierDistToOurGoal / 5000.0f, 0.0f, 1.0f);
        Perception.Threat.ThreatLevel = ThreatFromDist * 0.7f +
            FMath::Clamp(Perception.Threat.OpponentsBehindDefLine / 3.0f, 0.0f, 1.0f) * 0.3f;

        // Counter attack detection
        Perception.Threat.bIsCounterAttack = (Ball->GetBallSpeed() > 1500.0f &&
            FVector::Dist(Ball->GetBallVelocity().GetSafeNormal(), BallToGoal) < 0.3f);
    }
    else
    {
        Perception.Threat.ThreatLevel = 0.0f;
    }
}

void USoccerUtilityEvaluator::ScanNearbyPlayers(ASoccerPlayerPawn* Pawn, ASoccerGameMode* GM)
{
    Perception.NearbyTeammates.Empty();
    Perception.NearbyOpponents.Empty();

    FVector MyLoc = Pawn->GetActorLocation();

    TArray<ASoccerPlayerPawn*>& Team = (Pawn->GetTeamId() == ETeamId::Home) ? GM->HomePlayers : GM->AwayPlayers;
    TArray<ASoccerPlayerPawn*>& Opponents = (Pawn->GetTeamId() == ETeamId::Home) ? GM->AwayPlayers : GM->HomePlayers;

    // Scan teammates
    for (ASoccerPlayerPawn* P : Team)
    {
        if (P == Pawn || !P) continue;

        float Dist = FVector::Dist(MyLoc, P->GetActorLocation());
        if (Dist > PerceptionRadius) continue;

        FPerceivedPlayer Perceived;
        Perceived.Pawn = P;
        Perceived.Distance = Dist;
        Perceived.Direction = (P->GetActorLocation() - MyLoc).GetSafeNormal();
        Perceived.bPassingLaneClear = CheckPassingLaneClear(Pawn, P->GetActorLocation());
        Perceived.PositionalValue = CalculatePositionalValue(P->GetActorLocation(), Pawn->GetTeamId());

        Perception.NearbyTeammates.Add(Perceived);
    }

    // Scan opponents
    Perception.NearestOpponentDist = PerceptionRadius;
    Perception.OpponentsInPressRange = 0;

    for (ASoccerPlayerPawn* P : Opponents)
    {
        if (!P) continue;

        float Dist = FVector::Dist(MyLoc, P->GetActorLocation());
        if (Dist > PerceptionRadius) continue;

        FPerceivedPlayer Perceived;
        Perceived.Pawn = P;
        Perceived.Distance = Dist;
        Perceived.Direction = (P->GetActorLocation() - MyLoc).GetSafeNormal();

        Perception.NearbyOpponents.Add(Perceived);

        if (Dist < Perception.NearestOpponentDist)
        {
            Perception.NearestOpponentDist = Dist;
        }
        if (Dist < 500.0f)
        {
            Perception.OpponentsInPressRange++;
        }
    }

    // Sort by distance
    Perception.NearbyTeammates.Sort([](const FPerceivedPlayer& A, const FPerceivedPlayer& B)
    {
        return A.Distance < B.Distance;
    });
    Perception.NearbyOpponents.Sort([](const FPerceivedPlayer& A, const FPerceivedPlayer& B)
    {
        return A.Distance < B.Distance;
    });
}

bool USoccerUtilityEvaluator::CheckPassingLaneClear(ASoccerPlayerPawn* Pawn, FVector Target) const
{
    FVector MyLoc = Pawn->GetActorLocation();
    FVector ToTarget = Target - MyLoc;
    float PassDist = ToTarget.Size();

    for (const FPerceivedPlayer& Opp : Perception.NearbyOpponents)
    {
        if (!Opp.Pawn) continue;

        FVector OppLoc = Opp.Pawn->GetActorLocation();
        FVector ToOpp = OppLoc - MyLoc;
        float Along = FVector::DotProduct(ToOpp, ToTarget.GetSafeNormal());

        // Opponent behind us or past the target
        if (Along < 0.0f || Along > PassDist) continue;

        FVector ClosestPoint = MyLoc + ToTarget.GetSafeNormal() * Along;
        float PerpDist = FVector::Dist(OppLoc, ClosestPoint);

        if (PerpDist < 150.0f) return false; // 1.5m clearance
    }
    return true;
}

float USoccerUtilityEvaluator::CalculatePositionalValue(FVector Location, ETeamId Team) const
{
    // Higher value = more attacking position (0-1)
    float NormalizedX;
    if (Team == ETeamId::Home)
    {
        NormalizedX = (Location.X + SoccerField::HalfLength) / SoccerField::PitchLength;
    }
    else
    {
        NormalizedX = (-Location.X + SoccerField::HalfLength) / SoccerField::PitchLength;
    }

    // Value increases exponentially toward opponent goal
    // Also penalize being in own penalty area (bad for receiving passes)
    float Value = FMath::Pow(FMath::Clamp(NormalizedX, 0.0f, 1.0f), 1.5f);

    // Width bonus - being near the wings is slightly less valuable than central
    float NormalizedY = FMath::Abs(Location.Y) / SoccerField::HalfWidth;
    float WidthPenalty = FMath::Lerp(1.0f, 0.7f, FMath::Pow(NormalizedY, 2.0f));

    return Value * WidthPenalty;
}

// ============================================================
// Action Evaluation
// ============================================================

FSoccerAction USoccerUtilityEvaluator::EvaluateBestAction(ASoccerPlayerPawn* Pawn)
{
    if (!Pawn || !Pawn->IsValidLowLevel()) return FSoccerAction();

    TArray<FSoccerAction> Candidates;

    // Always evaluate hold position as baseline
    Candidates.Add(ScoreHoldPosition(Pawn));

    if (Perception.bIHaveBall)
    {
        // Attacking with ball: pass, shoot, dribble
        Candidates.Add(ScorePass(Pawn));
        Candidates.Add(ScoreShot(Pawn));
        Candidates.Add(ScoreDribble(Pawn));
        Candidates.Add(ScoreCross(Pawn));
        Candidates.Add(ScoreClear(Pawn));
    }
    else if (Perception.bMyTeamHasBall)
    {
        // Team has ball, I don't: make runs, support
        Candidates.Add(ScoreMakeRun(Pawn));
    }
    else
    {
        // Defending: press, tackle, cover
        if (Perception.bAmINearestToBall || Perception.DistToBall < 800.0f)
        {
            Candidates.Add(ScorePress(Pawn));
            Candidates.Add(ScoreTackle(Pawn));
        }
        Candidates.Add(ScoreCoverLane(Pawn));
    }

    // Sort by score descending
    Candidates.Sort([](const FSoccerAction& A, const FSoccerAction& B)
    {
        return A.Score > B.Score;
    });

    // Return highest-scored action (if valid)
    if (Candidates.Num() > 0 && Candidates[0].Score > 0.01f)
    {
        return Candidates[0];
    }

    // Fallback: hold position
    return ScoreHoldPosition(Pawn);
}

// ============================================================
// Individual Action Scorers
// ============================================================

FSoccerAction USoccerUtilityEvaluator::ScorePass(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Pass;
    float BestScore = 0.0f;
    ASoccerPlayerPawn* BestTarget = nullptr;

    for (const FPerceivedPlayer& Teammate : Perception.NearbyTeammates)
    {
        if (!Teammate.Pawn) continue;
        if (Teammate.Distance < MinPassDistance || Teammate.Distance > MaxPassDistance) continue;

        float LaneScore = Teammate.bPassingLaneClear ? 1.0f : 0.12f;
        float PositionScore = Teammate.PositionalValue;
        float DistScore = FMath::Clamp(1.0f - Teammate.Distance / MaxPassDistance, 0.2f, 1.0f);

        // Passing stat influence
        float PassSkill = Pawn->PlayerData.Stats.GetPassAccuracy();

        float Score = Weights.PassWeight * LaneScore * 0.5f
                     + PositionScore * 0.25f
                     + DistScore * 0.1f
                     + PassSkill * 0.15f;

        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Teammate.Pawn;
        }
    }

    if (BestTarget)
    {
        Action.Score = BestScore;
        Action.TargetTeammateIndex = BestTarget->GetSlotIndex();
        Action.TargetLocation = BestTarget->GetActorLocation();

        // Determine kick type based on distance
        float Dist = FVector::Dist(Pawn->GetActorLocation(), BestTarget->GetActorLocation());
        Action.KickType = (Dist > 1500.0f) ? EKickType::DrivenPass : EKickType::ShortPass;
        Action.PowerRatio = FMath::Clamp(Dist / PlayerMovement::DrivenPassPower, 0.3f, 1.0f);
    }

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreShot(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Shot;

    float DistToGoal = Perception.GetDistToGoal(Pawn->GetTeamId());
    float AngleToGoal = Perception.GetAngleToGoal(Pawn->GetTeamId());

    // Distance factor: closer = better
    float DistFactor = FMath::Clamp(1.0f - DistToGoal / MaxShotDistance, 0.0f, 1.0f);
    DistFactor = FMath::Pow(DistFactor, 1.5f); // Exponential dropoff

    // Angle factor: wider angle = better
    float AngleFactor = FMath::Clamp(AngleToGoal / 30.0f, 0.0f, 1.0f);
    if (AngleToGoal < MinShotAngle) AngleFactor = 0.0f;

    // Opponent pressure: fewer opponents near = better
    float PressureFactor = FMath::Clamp(1.0f - Perception.OpponentsInPressRange / 3.0f, 0.2f, 1.0f);

    // Shooting stat
    float SkillFactor = Pawn->PlayerData.Stats.GetShotPowerMultiplier();

    Action.Score = Weights.ShotWeight * DistFactor * 0.4f
                  + AngleFactor * 0.3f
                  + PressureFactor * 0.1f
                  + SkillFactor * 0.2f;

    // Determine kick type
    if (DistToGoal < 1500.0f)
    {
        Action.KickType = EKickType::Shot;
        Action.PowerRatio = 0.7f;
    }
    else
    {
        Action.KickType = EKickType::LobShot;
        Action.PowerRatio = 1.0f;
    }

    FVector GoalPos = (Pawn->GetTeamId() == ETeamId::Home)
        ? SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
    Action.TargetLocation = GoalPos;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreDribble(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Dribble;

    // Space ahead: more space = better dribble option
    float SpaceFactor = FMath::Clamp(Perception.NearestOpponentDist / 1000.0f, 0.0f, 1.0f);

    // Dribble skill
    float SkillFactor = Pawn->PlayerData.Stats.GetDribbleControl();

    // Stamina: less stamina = less dribble (more risk)
    float StaminaFactor = FMath::Clamp(Perception.Stamina / 50.0f, 0.3f, 1.0f);

    // Position: midfielders benefit most from dribbling
    float PositionBonus = 1.0f;
    if (Perception.bInAttackingHalf) PositionBonus = 1.3f;

    Action.Score = Weights.DribbleWeight * SpaceFactor * 0.4f
                  + SkillFactor * 0.3f
                  + StaminaFactor * 0.15f
                  + PositionBonus * 0.15f;

    // Dribble toward opponent goal
    FVector GoalDir = (Pawn->GetTeamId() == ETeamId::Home)
        ? FVector::ForwardVector : -FVector::ForwardVector;
    Action.TargetLocation = Pawn->GetActorLocation() + GoalDir * 500.0f;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreTackle(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Tackle;

    // Must be close to ball
    if (Perception.DistToBall > PlayerMovement::KickRange * 1.5f)
    {
        Action.Score = 0.0f;
        return Action;
    }

    // Tackling stat
    float SkillFactor = Pawn->PlayerData.Stats.GetTackleSuccess();

    // Threat: higher threat = more urgent tackle
    float ThreatFactor = FMath::Clamp(Perception.Threat.ThreatLevel, 0.2f, 1.0f);

    // Don't tackle if teammate is already tackling (prevent double commits)
    float TeammateFactor = 1.0f; // Could check if nearest teammate is also in tackle range

    Action.Score = Weights.TackleWeight * SkillFactor * 0.4f
                  + ThreatFactor * 0.3f
                  + TeammateFactor * 0.3f;

    Action.TargetLocation = Perception.BallLocation;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreHoldPosition(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::HoldPosition;

    // Base score: always somewhat viable
    float BaseScore = Weights.HoldPositionWeight;

    // Higher if close to formation slot (discourage wandering)
    float FormationFitness = FMath::Clamp(1.0f - Perception.DistFromFormationSlot / 1000.0f, 0.1f, 1.0f);

    // Lower if team has ball and we're in a good attacking position (should make runs instead)
    float AttackPenalty = Perception.bMyTeamHasBall ? 0.5f : 1.0f;

    Action.Score = BaseScore * FormationFitness * AttackPenalty;
    // Leave TargetLocation zero so ExecuteAction falls back to GetShiftedPosition(),
    // which correctly computes ball-attracted, zone-clamped formation position.
    Action.TargetLocation = FVector::ZeroVector;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreMakeRun(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::MakeRun;

    // Only relevant when team has ball
    if (!Perception.bMyTeamHasBall)
    {
        Action.Score = 0.0f;
        return Action;
    }

    // Space ahead
    float SpaceFactor = FMath::Clamp(Perception.NearestOpponentDist / 800.0f, 0.0f, 1.0f);

    // Position: attackers and midfielders benefit most
    float PositionBonus = 1.0f;
    EPlayerPosition Pos = Perception.Position;
    if (Pos == EPlayerPosition::ST || Pos == EPlayerPosition::CF || Pos == EPlayerPosition::LW || Pos == EPlayerPosition::RW)
    {
        PositionBonus = 1.4f;
    }
    else if (Pos == EPlayerPosition::CM || Pos == EPlayerPosition::CAM || Pos == EPlayerPosition::LM || Pos == EPlayerPosition::RM)
    {
        PositionBonus = 1.1f;
    }
    else if (Pos == EPlayerPosition::CB || Pos == EPlayerPosition::LB || Pos == EPlayerPosition::RB)
    {
        PositionBonus = 0.3f;
    }

    // Stamina
    float StaminaFactor = FMath::Clamp(Perception.Stamina / 40.0f, 0.0f, 1.0f);

    Action.Score = Weights.MakeRunWeight * SpaceFactor * 0.4f
                  + PositionBonus * 0.3f
                  + StaminaFactor * 0.3f;

    // Run toward opponent goal
    FVector GoalDir = (Pawn->GetTeamId() == ETeamId::Home)
        ? FVector::ForwardVector : -FVector::ForwardVector;
    Action.TargetLocation = Pawn->GetActorLocation() + GoalDir * 800.0f;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScorePress(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Press;

    // Must be reasonably close to ball
    if (Perception.DistToBall > 2000.0f)
    {
        Action.Score = 0.0f;
        return Action;
    }

    // Only press if nearest or close to nearest
    float ProximityBonus = Perception.bAmINearestToBall ? 1.0f : 0.4f;

    // Urgency from threat
    float ThreatFactor = FMath::Clamp(Perception.Threat.ThreatLevel + 0.2f, 0.0f, 1.0f);

    // Stamina cost
    float StaminaFactor = FMath::Clamp(Perception.Stamina / 40.0f, 0.1f, 1.0f);

    Action.Score = Weights.PressWeight * ProximityBonus * 0.4f
                  + ThreatFactor * 0.3f
                  + StaminaFactor * 0.3f;

    Action.TargetLocation = Perception.BallLocation;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreCoverLane(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::CoverLane;

    // Relevant when defending (team doesn't have ball)
    if (Perception.bMyTeamHasBall)
    {
        Action.Score = Weights.CoverLaneWeight * 0.2f; // Low score when attacking
        Action.TargetLocation = Perception.FormationPosition;
        return Action;
    }

    // Higher threat = more urgent to cover
    float ThreatFactor = Perception.Threat.ThreatLevel;

    // Defenders get bonus for covering
    float PositionBonus = 1.0f;
    EPlayerPosition Pos = Perception.Position;
    if (Pos == EPlayerPosition::CB || Pos == EPlayerPosition::LB || Pos == EPlayerPosition::RB ||
        Pos == EPlayerPosition::LWB || Pos == EPlayerPosition::RWB || Pos == EPlayerPosition::CDM)
    {
        PositionBonus = 1.4f;
    }

    // If far from formation slot, should get back
    float Urgency = FMath::Clamp(Perception.DistFromFormationSlot / 1000.0f, 0.0f, 1.0f);

    Action.Score = Weights.CoverLaneWeight * ThreatFactor * 0.4f
                  + PositionBonus * 0.3f
                  + Urgency * 0.3f;

    // Leave TargetLocation zero so ExecuteAction falls back to GetShiftedPosition()
    Action.TargetLocation = FVector::ZeroVector;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreCross(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Cross;

    // Must have ball and be in a wide attacking position
    if (!Perception.bIHaveBall) { Action.Score = 0.0f; return Action; }

    // Must be in wide position near opponent goal
    float AbsY = FMath::Abs(Pawn->GetActorLocation().Y);
    float NormalizedWidth = AbsY / SoccerField::HalfWidth;
    bool bIsWide = NormalizedWidth > 0.6f;

    if (!bIsWide) { Action.Score = 0.0f; return Action; }

    // Must be in attacking third
    if (!Perception.bInAttackingHalf) { Action.Score = 0.0f; return Action; }

    // Check for teammates in the box
    float TeammatesInBox = 0.0f;
    FVector OppGoal = (Pawn->GetTeamId() == ETeamId::Home)
        ? SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();

    for (const FPerceivedPlayer& Teammate : Perception.NearbyTeammates)
    {
        if (!Teammate.Pawn) continue;
        float DistToGoal = FVector::Dist(Teammate.Pawn->GetActorLocation(), OppGoal);
        if (DistToGoal < SoccerField::PenaltyBoxLength * 1.2f)
        {
            TeammatesInBox += 1.0f;
        }
    }

    if (TeammatesInBox < 1.0f) { Action.Score = 0.0f; return Action; }

    Action.Score = Weights.CrossWeight * FMath::Clamp(TeammatesInBox / 3.0f, 0.3f, 1.0f);
    Action.KickType = EKickType::Cross;
    Action.PowerRatio = 0.8f;
    Action.TargetLocation = OppGoal;

    return Action;
}

FSoccerAction USoccerUtilityEvaluator::ScoreClear(ASoccerPlayerPawn* Pawn)
{
    FSoccerAction Action;
    Action.Type = ESoccerActionType::Clear;

    // Must have ball
    if (!Perception.bIHaveBall) { Action.Score = 0.0f; return Action; }

    // Only consider clearing when under heavy pressure in own half
    FVector OurGoal = (Pawn->GetTeamId() == ETeamId::Home)
        ? SoccerField::HomeGoalCenter() : SoccerField::AwayGoalCenter();
    float DistToOurGoal = FVector::Dist(Pawn->GetActorLocation(), OurGoal);

    // Must be in own defensive third
    if (DistToOurGoal > 4000.0f) { Action.Score = 0.0f; return Action; }

    // Pressure: more opponents nearby = more urgent to clear
    float PressureFactor = FMath::Clamp(Perception.OpponentsInPressRange / 2.0f, 0.0f, 1.0f);

    // Threat level
    float ThreatFactor = Perception.Threat.ThreatLevel;

    Action.Score = Weights.ClearWeight * PressureFactor * 0.5f + ThreatFactor * 0.5f;
    Action.KickType = EKickType::DrivenPass;
    Action.PowerRatio = 1.0f;

    // Clear toward the nearest touchline side
    float Y = Pawn->GetActorLocation().Y;
    Action.TargetLocation = FVector(0.0f, FMath::Sign(Y) * SoccerField::HalfWidth, 0.0f);

    return Action;
}
