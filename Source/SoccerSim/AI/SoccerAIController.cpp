#include "SoccerAIController.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/AI/SoccerUtilityEvaluator.h"
#include "SoccerSim/AI/SoccerTeamBrain.h"
#include "SoccerSim/AI/SoccerGoalkeeperAI.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"

ASoccerAIController::ASoccerAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASoccerAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    SoccerPawn = Cast<ASoccerPlayerPawn>(InPawn);
    Ball = FindBall();

    // Create utility evaluator
    UtilityEvaluator = NewObject<USoccerUtilityEvaluator>(this);

    // Set position-appropriate weights
    if (SoccerPawn)
    {
        EPlayerPosition Pos = SoccerPawn->GetPlayerPosition();
        if (Pos == EPlayerPosition::GK)
        {
            UtilityEvaluator->Weights = FSoccerActionWeights::GoalkeeperWeights();
        }
        else if (Pos == EPlayerPosition::CB || Pos == EPlayerPosition::LB || Pos == EPlayerPosition::RB ||
                 Pos == EPlayerPosition::LWB || Pos == EPlayerPosition::RWB)
        {
            UtilityEvaluator->Weights = FSoccerActionWeights::DefenderWeights();
        }
        else if (Pos == EPlayerPosition::ST || Pos == EPlayerPosition::CF ||
                 Pos == EPlayerPosition::LW || Pos == EPlayerPosition::RW)
        {
            UtilityEvaluator->Weights = FSoccerActionWeights::ForwardWeights();
        }
        else
        {
            UtilityEvaluator->Weights = FSoccerActionWeights::MidfielderWeights();
        }
    }
}

void ASoccerAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!SoccerPawn || SoccerPawn->IsHumanControlled()) return;

    if (!Ball) Ball = FindBall();
    if (!Ball) return;

    // GK uses dedicated component
    if (SoccerPawn->GetPlayerPosition() == EPlayerPosition::GK)
    {
        // GK AI is handled by USoccerGoalkeeperAI component on the pawn
        // Just make sure they have the component
        if (!SoccerPawn->FindComponentByClass<USoccerGoalkeeperAI>())
        {
            SoccerPawn->AddComponentByClass(USoccerGoalkeeperAI::StaticClass(), false, FTransform(), false);
        }
        return;
    }

    MakeDecision(DeltaTime);
}

// ============================================================
// Decision Making
// ============================================================

void ASoccerAIController::MakeDecision(float DeltaTime)
{
    TimeSinceLastDecision += DeltaTime;

    if (TimeSinceLastDecision >= DecisionInterval)
    {
        TimeSinceLastDecision = 0.0f;

        if (bUseUtilityAI && UtilityEvaluator)
        {
            MakeDecision_UtilityAI(DeltaTime);
        }
        else
        {
            MakeDecision_StateMachine(DeltaTime);
        }
    }

    // Execute current action/state every tick (smooth movement)
    if (bUseUtilityAI && CurrentAction.IsValid())
    {
        ExecuteAction(CurrentAction, DeltaTime);
    }
    else
    {
        ExecuteState(CurrentState, DeltaTime);
    }
}

void ASoccerAIController::MakeDecision_UtilityAI(float DeltaTime)
{
    // Update perception
    UtilityEvaluator->UpdatePerception(SoccerPawn, Ball, DeltaTime);

    // Get best action
    CurrentAction = UtilityEvaluator->EvaluateBestAction(SoccerPawn);

#if ENABLE_DRAW_DEBUG
    // Debug: log action type periodically
    if (GFrameNumber % 60 == 0 && CurrentAction.IsValid())
    {
        UE_LOG(LogSoccerSim, Verbose, TEXT("%s: %s (score: %.2f)"),
            *SoccerPawn->PlayerData.PlayerName,
            *UEnum::GetValueAsString(CurrentAction.Type),
            CurrentAction.Score);
    }
#endif
}

void ASoccerAIController::MakeDecision_StateMachine(float DeltaTime)
{
    if (SoccerPawn->GetPlayerPosition() == EPlayerPosition::GK)
    {
        CurrentState = EPlayerAIState::HoldingPosition;
        return;
    }

    bool bTeamHasBall = (Ball->LastTouchTeam == SoccerPawn->GetTeamId());
    bool bIHaveBall = SoccerPawn->HasBall();

    if (bIHaveBall)
    {
        FVector GoalPos = (SoccerPawn->GetTeamId() == ETeamId::Home) ?
            SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
        float DistToGoal = FVector::Dist(SoccerPawn->GetActorLocation(), GoalPos);

        if (DistToGoal < 2500.0f)
        {
            SoccerPawn->AttemptShot(FMath::RandRange(0.5f, 1.2f));
            CurrentState = EPlayerAIState::PreparingShot;
        }
        else
        {
            CurrentState = EPlayerAIState::Dribbling;
        }
    }
    else if (bTeamHasBall)
    {
        CurrentState = EPlayerAIState::MakingRun;
    }
    else
    {
        if (AmINearestTeammateToBall())
        {
            CurrentState = EPlayerAIState::ChasingBall;
        }
        else
        {
            CurrentState = EPlayerAIState::HoldingPosition;
        }
    }
}

// ============================================================
// Action Execution (Utility AI)
// ============================================================

void ASoccerAIController::ExecuteAction(const FSoccerAction& Action, float DeltaTime)
{
    switch (Action.Type)
    {
    case ESoccerActionType::Pass:
    {
        // Pass to target teammate
        ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
        if (GM && Action.TargetTeammateIndex >= 0)
        {
            TArray<ASoccerPlayerPawn*>& Team = (SoccerPawn->GetTeamId() == ETeamId::Home)
                ? GM->HomePlayers : GM->AwayPlayers;
            if (Action.TargetTeammateIndex < Team.Num() && Team[Action.TargetTeammateIndex])
            {
                FVector TargetLoc = Action.TargetLocation;
                // Predict teammate movement
                ASoccerPlayerPawn* Target = Team[Action.TargetTeammateIndex];
                TargetLoc += Target->GetVelocity() * 0.3f; // Lead the pass

                FVector Dir = (TargetLoc - SoccerPawn->GetActorLocation()).GetSafeNormal();
                float Dist = FVector::Dist(SoccerPawn->GetActorLocation(), TargetLoc);

                EKickType KickType = (Dist > 1500.0f) ? EKickType::DrivenPass : EKickType::ShortPass;
                SoccerPawn->AttemptKick(KickType);
            }
        }
        // After kick, return to formation
        MoveToFormationPosition();
        break;
    }

    case ESoccerActionType::Shot:
    {
        SoccerPawn->AttemptShot(0.8f);
        break;
    }

    case ESoccerActionType::Dribble:
    {
        FVector Target = Action.TargetLocation;
        FVector Dir = (Target - SoccerPawn->GetActorLocation()).GetSafeNormal();
        SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        SoccerPawn->SetSprinting(false); // Controlled dribble
        break;
    }

    case ESoccerActionType::Tackle:
    {
        SoccerPawn->AttemptTackle();
        // Move toward ball for tackle
        MoveTowardBall();
        break;
    }

    case ESoccerActionType::HoldPosition:
    {
        FVector Target = Action.TargetLocation.IsNearlyZero()
            ? GetShiftedPosition() : Action.TargetLocation;
        FVector ToTarget = Target - SoccerPawn->GetActorLocation();
        float Dist = ToTarget.Size2D();

        if (Dist > PositionTolerance)
        {
            FVector Dir = ToTarget.GetSafeNormal();
            SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
            SoccerPawn->SetSprinting(Dist > 800.0f);
        }
        else
        {
            SoccerPawn->SetMovementInput(FVector2D::ZeroVector);
            SoccerPawn->SetSprinting(false);
        }
        break;
    }

    case ESoccerActionType::MakeRun:
    {
        FVector Target = Action.TargetLocation;
        FVector Dir = (Target - SoccerPawn->GetActorLocation()).GetSafeNormal();
        SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        SoccerPawn->SetSprinting(true);
        break;
    }

    case ESoccerActionType::Press:
    {
        MoveTowardBall();
        SoccerPawn->SetSprinting(true);

        // If close enough, attempt tackle
        if (SoccerPawn->HasBall() ||
            FVector::Dist(SoccerPawn->GetActorLocation(), Ball->GetActorLocation()) < PlayerMovement::KickRange * 1.2f)
        {
            SoccerPawn->AttemptTackle();
        }
        break;
    }

    case ESoccerActionType::CoverLane:
    {
        FVector Target = Action.TargetLocation.IsNearlyZero()
            ? GetShiftedPosition() : Action.TargetLocation;
        FVector ToTarget = Target - SoccerPawn->GetActorLocation();
        float Dist = ToTarget.Size2D();

        if (Dist > PositionTolerance)
        {
            FVector Dir = ToTarget.GetSafeNormal();
            SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
            SoccerPawn->SetSprinting(Dist > 600.0f);
        }
        else
        {
            SoccerPawn->SetMovementInput(FVector2D::ZeroVector);
            SoccerPawn->SetSprinting(false);

            // Face the ball while in position
            FVector ToBall = (Ball->GetActorLocation() - SoccerPawn->GetActorLocation());
            ToBall.Z = 0.0f;
            if (!ToBall.IsNearlyZero())
            {
                SoccerPawn->SetActorRotation(ToBall.Rotation());
            }
        }
        break;
    }

    case ESoccerActionType::Cross:
    {
        SoccerPawn->AttemptKick(EKickType::Cross);
        break;
    }

    case ESoccerActionType::Header:
    {
        // Headers are handled by physics contact - just position near ball
        MoveTowardBall();
        break;
    }

    case ESoccerActionType::Clear:
    {
        SoccerPawn->AttemptKick(EKickType::DrivenPass);
        break;
    }

    case ESoccerActionType::TrackRunner:
    default:
    {
        MoveToFormationPosition();
        break;
    }
    }
}

// ============================================================
// State Execution (Fallback State Machine)
// ============================================================

void ASoccerAIController::ExecuteState(EPlayerAIState State, float DeltaTime)
{
    switch (State)
    {
    case EPlayerAIState::ChasingBall:
        MoveTowardBall();
        break;

    case EPlayerAIState::HoldingPosition:
    {
        FVector TargetPos = GetShiftedPosition();
        FVector ToTarget = TargetPos - SoccerPawn->GetActorLocation();
        float Dist = ToTarget.Size2D();

        if (Dist > PositionTolerance)
        {
            FVector Dir = ToTarget.GetSafeNormal();
            SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
            SoccerPawn->SetSprinting(Dist > 800.0f);
        }
        else
        {
            SoccerPawn->SetMovementInput(FVector2D::ZeroVector);
            SoccerPawn->SetSprinting(false);
        }
        break;
    }

    case EPlayerAIState::MakingRun:
    {
        FVector BasePos = GetShiftedPosition();
        FVector GoalPos = (SoccerPawn->GetTeamId() == ETeamId::Home) ?
            SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
        FVector RunTarget = BasePos + (GoalPos - BasePos).GetSafeNormal() * 500.0f;
        FVector Dir = (RunTarget - SoccerPawn->GetActorLocation()).GetSafeNormal();
        SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        SoccerPawn->SetSprinting(true);
        break;
    }

    case EPlayerAIState::Dribbling:
    {
        FVector GoalPos = (SoccerPawn->GetTeamId() == ETeamId::Home) ?
            SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
        FVector Dir = (GoalPos - SoccerPawn->GetActorLocation()).GetSafeNormal();
        SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        break;
    }

    case EPlayerAIState::Pressing:
        MoveTowardBall();
        break;

    default:
    {
        FVector TargetPos = GetShiftedPosition();
        FVector ToTarget = TargetPos - SoccerPawn->GetActorLocation();
        float Dist = ToTarget.Size2D();
        if (Dist > PositionTolerance)
        {
            FVector Dir = ToTarget.GetSafeNormal();
            SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        }
        else
        {
            SoccerPawn->SetMovementInput(FVector2D::ZeroVector);
        }
        break;
    }
    }
}

// ============================================================
// Movement Helpers
// ============================================================

void ASoccerAIController::MoveToLocation(FVector Target, float AcceptanceRadius)
{
    FVector Dir = (Target - SoccerPawn->GetActorLocation()).GetSafeNormal();
    SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
}

void ASoccerAIController::MoveTowardBall()
{
    if (!Ball) return;
    FVector BallLoc = Ball->GetActorLocation();
    FVector Dir = (BallLoc - SoccerPawn->GetActorLocation()).GetSafeNormal();
    SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
    SoccerPawn->SetSprinting(true);

    float Dist = FVector::Dist(SoccerPawn->GetActorLocation(), BallLoc);
    if (Dist < PlayerMovement::KickRange * 1.2f)
    {
        SoccerPawn->AttemptTackle();
    }
}

void ASoccerAIController::MoveToFormationPosition()
{
    FVector TargetPos = GetShiftedPosition();
    FVector ToTarget = TargetPos - SoccerPawn->GetActorLocation();
    float Dist = ToTarget.Size2D();

    if (Dist > PositionTolerance)
    {
        FVector Dir = ToTarget.GetSafeNormal();
        SoccerPawn->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        SoccerPawn->SetSprinting(Dist > 800.0f);
    }
    else
    {
        SoccerPawn->SetMovementInput(FVector2D::ZeroVector);
        SoccerPawn->SetSprinting(false);
    }
}

// ============================================================
// Perception Helpers
// ============================================================

bool ASoccerAIController::AmINearestTeammateToBall() const
{
    if (!Ball || !SoccerPawn) return false;

    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return false;

    TArray<ASoccerPlayerPawn*>& Team = (SoccerPawn->GetTeamId() == ETeamId::Home) ?
        GM->HomePlayers : GM->AwayPlayers;

    float MyDist = FVector::Dist(SoccerPawn->GetActorLocation(), Ball->GetActorLocation());

    for (ASoccerPlayerPawn* P : Team)
    {
        if (P == SoccerPawn) continue;
        if (P->IsHumanControlled()) continue;
        if (P->GetPlayerPosition() == EPlayerPosition::GK) continue;

        float TheirDist = FVector::Dist(P->GetActorLocation(), Ball->GetActorLocation());
        if (TheirDist < MyDist)
        {
            return false;
        }
    }
    return true;
}

FVector ASoccerAIController::GetFormationWorldPosition() const
{
    if (!SoccerPawn) return FVector::ZeroVector;
    return SoccerField::NormalizedToWorld(
        SoccerPawn->GetFormationSlot().NormalizedPosition,
        SoccerPawn->GetTeamId());
}

FVector ASoccerAIController::GetShiftedPosition() const
{
    FVector HomePos = GetFormationWorldPosition();
    if (!Ball) return HomePos;

    FVector BallPos = Ball->GetActorLocation();
    float Attraction = SoccerPawn->GetFormationSlot().BallAttraction;

    FVector Shifted = FMath::Lerp(HomePos, BallPos, Attraction);

    const FFormationSlot& Slot = SoccerPawn->GetFormationSlot();
    FVector ZoneMin = SoccerField::NormalizedToWorld(Slot.ZoneMin, SoccerPawn->GetTeamId());
    FVector ZoneMax = SoccerField::NormalizedToWorld(Slot.ZoneMax, SoccerPawn->GetTeamId());

    float MinX = FMath::Min(ZoneMin.X, ZoneMax.X);
    float MaxX = FMath::Max(ZoneMin.X, ZoneMax.X);
    float MinY = FMath::Min(ZoneMin.Y, ZoneMax.Y);
    float MaxY = FMath::Max(ZoneMin.Y, ZoneMax.Y);

    Shifted.X = FMath::Clamp(Shifted.X, MinX, MaxX);
    Shifted.Y = FMath::Clamp(Shifted.Y, MinY, MaxY);
    Shifted.Z = 0.0f;

    return Shifted;
}

ASoccerBall* ASoccerAIController::FindBall() const
{
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    return GM ? GM->MatchBall : nullptr;
}
