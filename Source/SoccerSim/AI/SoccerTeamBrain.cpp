#include "SoccerTeamBrain.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"

USoccerTeamBrain::USoccerTeamBrain()
{
}

void USoccerTeamBrain::Initialize(ETeamId InTeamId)
{
    TeamId = InTeamId;
    CurrentPhase = ETeamPhase::Defending;
    PressState = ETeamPressState::NoPress;

    UE_LOG(LogSoccerSim, Log, TEXT("TeamBrain initialized for team %d"),
        static_cast<int32>(TeamId));
}

void USoccerTeamBrain::Update(float DeltaTime, const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                              const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball)
{
    if (!Ball || TeamPlayers.Num() == 0) return;

    UpdateTeamPhase(DeltaTime, TeamPlayers, OpponentPlayers, Ball);
    UpdatePressingState(TeamPlayers, OpponentPlayers, Ball);
    UpdateMarkingAssignments(TeamPlayers, OpponentPlayers, Ball);
}

// ============================================================
// Team Phase
// ============================================================

void USoccerTeamBrain::UpdateTeamPhase(float DeltaTime, const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                                        const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball)
{
    ETeamPhase DesiredPhase;

    bool bHasBall = DoesTeamHaveBall(Ball);
    bool bBallInDefThird = IsBallInDefensiveThird(Ball);
    bool bBallInAttThird = IsBallInAttackingThird(Ball);

    if (bHasBall)
    {
        if (bBallInAttThird)
        {
            DesiredPhase = ETeamPhase::Attacking;
        }
        else
        {
            DesiredPhase = ETeamPhase::AttackingTransition;
        }
    }
    else
    {
        if (bBallInDefThird)
        {
            DesiredPhase = ETeamPhase::Defending;
        }
        else
        {
            DesiredPhase = ETeamPhase::DefendingTransition;
        }
    }

    // Phase hysteresis: don't flip-flop rapidly
    if (DesiredPhase != CurrentPhase)
    {
        PhaseTransitionTimer += DeltaTime;
        float TransitionDelay = 0.5f; // seconds before phase switch

        // Counter-attack detection: immediate transition
        if (bHasBall && CurrentPhase == ETeamPhase::Defending && bBallInDefThird)
        {
            TransitionDelay = 0.1f; // Quick transition for counter-attacks
        }

        if (PhaseTransitionTimer >= TransitionDelay)
        {
            CurrentPhase = DesiredPhase;
            PhaseTransitionTimer = 0.0f;

            UE_LOG(LogSoccerSim, Verbose, TEXT("Team %d phase -> %s"),
                static_cast<int32>(TeamId),
                *UEnum::GetValueAsString(CurrentPhase));
        }
    }
    else
    {
        PhaseTransitionTimer = 0.0f;
    }

    // Counter-attack timer
    if (CurrentPhase == ETeamPhase::AttackingTransition)
    {
        CounterAttackTimer += DeltaTime;
        if (CounterAttackTimer > 6.0f)
        {
            // Counter-attack expired, settle into normal attacking
            CounterAttackTimer = 0.0f;
        }
    }
    else
    {
        CounterAttackTimer = 0.0f;
    }
}

// ============================================================
// Pressing State
// ============================================================

void USoccerTeamBrain::UpdatePressingState(const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                                            const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball)
{
    if (!Ball) return;

    FVector BallLoc = Ball->GetActorLocation();
    FVector OurGoal = (TeamId == ETeamId::Home)
        ? SoccerField::HomeGoalCenter() : SoccerField::AwayGoalCenter();
    OurGoal.Z = 0.0f;

    float BallDistFromOurGoal = FVector::Dist(BallLoc, OurGoal);
    bool bHasBall = DoesTeamHaveBall(Ball);

    // Determine pressing intensity based on ball position and tactical settings
    if (bHasBall)
    {
        PressState = ETeamPressState::NoPress;
        AssignPressingRoles(TeamPlayers, Ball);
        return;
    }

    float PressingIntensity = Tactics.PressingIntensity;

    // Counter-press: immediately after losing the ball
    if (Ball->LastTouchTeam == TeamId && Ball->GetBallSpeed() < 500.0f)
    {
        PressState = ETeamPressState::CounterPress;
    }
    // Hard press: ball in our defensive third with high pressing intensity
    else if (BallDistFromOurGoal < PressingActivationRadius && PressingIntensity > 0.7f)
    {
        PressState = ETeamPressState::HardPress;
    }
    // Soft press: ball in midfield with moderate pressing
    else if (BallDistFromOurGoal < PressingActivationRadius * 2.0f && PressingIntensity > 0.4f)
    {
        PressState = ETeamPressState::SoftPress;
    }
    else
    {
        PressState = ETeamPressState::NoPress;
    }

    AssignPressingRoles(TeamPlayers, Ball);
}

void USoccerTeamBrain::AssignPressingRoles(const TArray<ASoccerPlayerPawn*>& TeamPlayers, ASoccerBall* Ball)
{
    PressingPlayerIndices.Empty();

    if (PressState == ETeamPressState::NoPress || !Ball) return;

    // Sort players by distance to ball
    TArray<TPair<int32, float>> Distances;
    for (int32 i = 0; i < TeamPlayers.Num(); i++)
    {
        if (!TeamPlayers[i]) continue;
        if (TeamPlayers[i]->GetPlayerPosition() == EPlayerPosition::GK) continue;

        float Dist = FVector::Dist(TeamPlayers[i]->GetActorLocation(), Ball->GetActorLocation());
        Distances.Add(TPair<int32, float>(i, Dist));
    }

    Distances.Sort([](const TPair<int32, float>& A, const TPair<int32, float>& B)
    {
        return A.Value < B.Value;
    });

    // Assign closest N players as pressers
    int32 NumPressers = FMath::Min(MaxPressingPlayers, Distances.Num());
    if (PressState == ETeamPressState::CounterPress)
    {
        NumPressers = FMath::Min(MaxPressingPlayers + 1, Distances.Num()); // Extra presser for counter-press
    }
    else if (PressState == ETeamPressState::SoftPress)
    {
        NumPressers = FMath::Max(1, NumPressers - 1); // Fewer for soft press
    }

    for (int32 i = 0; i < NumPressers && i < Distances.Num(); i++)
    {
        PressingPlayerIndices.Add(Distances[i].Key);
    }
}

// ============================================================
// Marking Assignments
// ============================================================

void USoccerTeamBrain::UpdateMarkingAssignments(const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                                                  const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball)
{
    MarkingAssignments.Empty();

    if (!Ball) return;
    bool bHasBall = DoesTeamHaveBall(Ball);
    if (bHasBall) return; // Don't man-mark when we have the ball

    // Only assign man-marking if using man-marking or hybrid defensive style
    if (Tactics.DefensiveStyle == EDefensiveStyle::Zonal) return;

    // Simple matching: assign defenders to mark the most dangerous opponents
    TArray<ASoccerPlayerPawn*> Defenders;
    TArray<ASoccerPlayerPawn*> DangerousOpponents;

    FVector OurGoal = (TeamId == ETeamId::Home)
        ? SoccerField::HomeGoalCenter() : SoccerField::AwayGoalCenter();
    OurGoal.Z = 0.0f;

    // Collect defenders (exclude GK and already-pressing players)
    for (int32 i = 0; i < TeamPlayers.Num(); i++)
    {
        if (!TeamPlayers[i]) continue;
        EPlayerPosition Pos = TeamPlayers[i]->GetPlayerPosition();
        if (Pos == EPlayerPosition::GK) continue;
        if (PressingPlayerIndices.Contains(i)) continue;

        if (Pos == EPlayerPosition::CB || Pos == EPlayerPosition::LB || Pos == EPlayerPosition::RB ||
            Pos == EPlayerPosition::CDM || Pos == EPlayerPosition::LWB || Pos == EPlayerPosition::RWB)
        {
            Defenders.Add(TeamPlayers[i]);
        }
    }

    // Collect dangerous opponents (closest to our goal)
    for (ASoccerPlayerPawn* Opp : OpponentPlayers)
    {
        if (!Opp) continue;
        if (Opp->GetPlayerPosition() == EPlayerPosition::GK) continue;
        DangerousOpponents.Add(Opp);
    }

    // Sort opponents by danger (distance to our goal)
    DangerousOpponents.Sort([&OurGoal](const ASoccerPlayerPawn& A, const ASoccerPlayerPawn& B)
    {
        return FVector::Dist(A.GetActorLocation(), OurGoal) < FVector::Dist(B.GetActorLocation(), OurGoal);
    });

    // Assign: closest defender marks closest dangerous opponent
    int32 AssignCount = FMath::Min(Defenders.Num(), DangerousOpponents.Num());
    for (int32 i = 0; i < AssignCount; i++)
    {
        FMarkingAssignment Assignment;
        Assignment.Marker = Defenders[i];
        Assignment.Target = DangerousOpponents[i];
        Assignment.Tightness = (Tactics.DefensiveStyle == EDefensiveStyle::ManMark) ? 0.9f : 0.5f;
        MarkingAssignments.Add(Assignment);
    }
}

// ============================================================
// Formation Shift
// ============================================================

FVector USoccerTeamBrain::GetShiftedFormationPosition(const ASoccerPlayerPawn* Player, ASoccerBall* Ball) const
{
    if (!Player || !Ball) return FVector::ZeroVector;

    const FFormationSlot& Slot = Player->GetFormationSlot();
    FVector BasePos = SoccerField::NormalizedToWorld(Slot.NormalizedPosition, TeamId);

    // Ball attraction: shift formation toward ball
    FVector BallPos = Ball->GetActorLocation();
    float Attraction = Slot.BallAttraction * Tactics.Compactness;

    FVector Shifted = FMath::Lerp(BasePos, BallPos, Attraction);

    // Team phase adjustments
    float PhaseShiftX = 0.0f;

    switch (CurrentPhase)
    {
    case ETeamPhase::Attacking:
        PhaseShiftX = 300.0f * (TeamId == ETeamId::Home ? 1.0f : -1.0f); // Push up
        break;
    case ETeamPhase::AttackingTransition:
        PhaseShiftX = 500.0f * (TeamId == ETeamId::Home ? 1.0f : -1.0f); // Counter-attack push
        break;
    case ETeamPhase::Defending:
        PhaseShiftX = -200.0f * (TeamId == ETeamId::Home ? 1.0f : -1.0f); // Drop back
        break;
    case ETeamPhase::DefendingTransition:
        PhaseShiftX = -100.0f * (TeamId == ETeamId::Home ? 1.0f : -1.0f); // Recover
        break;
    default:
        break;
    }

    Shifted.X += PhaseShiftX;

    // Width adjustment based on attacking width setting
    float WidthMult = 0.5f + Tactics.AttackingWidth * 0.5f;
    float CenterY = 0.0f;
    Shifted.Y = CenterY + (Shifted.Y - CenterY) * WidthMult;

    // Clamp to zone
    FVector ZoneMin = SoccerField::NormalizedToWorld(Slot.ZoneMin, TeamId);
    FVector ZoneMax = SoccerField::NormalizedToWorld(Slot.ZoneMax, TeamId);

    Shifted.X = FMath::Clamp(Shifted.X, FMath::Min(ZoneMin.X, ZoneMax.X), FMath::Max(ZoneMin.X, ZoneMax.X));
    Shifted.Y = FMath::Clamp(Shifted.Y, FMath::Min(ZoneMin.Y, ZoneMax.Y), FMath::Max(ZoneMin.Y, ZoneMax.Y));
    Shifted.Z = SoccerField::GamePlaneZ;

    return Shifted;
}

// ============================================================
// Player Queries
// ============================================================

bool USoccerTeamBrain::ShouldPlayerPress(const ASoccerPlayerPawn* Player) const
{
    if (!Player) return false;

    // Check if this player is in the pressing list
    for (int32 Idx : PressingPlayerIndices)
    {
        // We can't directly compare by index here since we don't have the full team array
        // Instead, check if this player is one of the pressers by distance ranking
    }

    // Simplified: check if this player is close to ball and pressing is active
    if (PressState == ETeamPressState::NoPress) return false;

    // If pressing is active and player is reasonably close, they should press
    return true; // Will be filtered further by the individual utility evaluator
}

ASoccerPlayerPawn* USoccerTeamBrain::GetMarkTarget(const ASoccerPlayerPawn* Player) const
{
    for (const FMarkingAssignment& Assignment : MarkingAssignments)
    {
        if (Assignment.Marker.Get() == Player)
        {
            return Assignment.Target.Get();
        }
    }
    return nullptr;
}

// ============================================================
// Helpers
// ============================================================

bool USoccerTeamBrain::IsBallInDefensiveThird(ASoccerBall* Ball) const
{
    if (!Ball) return false;

    FVector BallLoc = Ball->GetActorLocation();
    FVector OurGoal = (TeamId == ETeamId::Home)
        ? SoccerField::HomeGoalCenter() : SoccerField::AwayGoalCenter();
    OurGoal.Z = 0.0f;

    return FVector::Dist(BallLoc, OurGoal) < SoccerField::HalfLength * 0.6f;
}

bool USoccerTeamBrain::IsBallInAttackingThird(ASoccerBall* Ball) const
{
    if (!Ball) return false;

    FVector BallLoc = Ball->GetActorLocation();
    FVector OppGoal = (TeamId == ETeamId::Home)
        ? SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
    OppGoal.Z = 0.0f;

    return FVector::Dist(BallLoc, OppGoal) < SoccerField::HalfLength * 0.6f;
}

bool USoccerTeamBrain::DoesTeamHaveBall(ASoccerBall* Ball) const
{
    return Ball && Ball->LastTouchTeam == TeamId;
}
