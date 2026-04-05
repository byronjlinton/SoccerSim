#include "SoccerGoalkeeperAI.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

USoccerGoalkeeperAI::USoccerGoalkeeperAI()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

void USoccerGoalkeeperAI::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ASoccerPlayerPawn* GK = Cast<ASoccerPlayerPawn>(GetOwner());
    if (!GK || GK->IsHumanControlled()) return;

    ASoccerBall* Ball = nullptr;
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM) Ball = GM->MatchBall;
    if (!Ball) return;

    // Get ideal position
    FVector IdealPos = GetIdealPosition(GK, Ball);

    // Check for diving
    FVector DiveDir = ShouldDive(GK, Ball);
    if (!DiveDir.IsNearlyZero())
    {
        // Execute dive - move GK rapidly in dive direction
        FVector DiveTarget = GK->GetActorLocation() + DiveDir * MaxDiveDistance;
        GK->SetActorLocation(FMath::Lerp(GK->GetActorLocation(), DiveTarget, DeltaTime * 8.0f));
        return; // Skip normal positioning during dive
    }

    // Check for rush out
    if (ShouldRushOut(GK, Ball))
    {
        FVector RushTarget = Ball->GetActorLocation();
        RushTarget.Z = SoccerField::GamePlaneZ;
        FVector Dir = (RushTarget - GK->GetActorLocation()).GetSafeNormal();
        GK->SetMovementInput(FVector2D(Dir.X, Dir.Y));
        GK->SetSprinting(true);
        return;
    }

    // Normal positioning: move to ideal position
    FVector ToIdeal = IdealPos - GK->GetActorLocation();
    float DistToIdeal = ToIdeal.Size2D();

    if (DistToIdeal > 30.0f)
    {
        FVector Dir = ToIdeal.GetSafeNormal2D();
        float Speed = FMath::Min(DistToIdeal, GKPositioningSpeed) * DeltaTime;
        FVector NewPos = GK->GetActorLocation() + Dir * Speed;
        NewPos.Z = SoccerField::GamePlaneZ;
        GK->SetActorLocation(NewPos);
    }

    // Face the ball
    FVector ToBall = (Ball->GetActorLocation() - GK->GetActorLocation());
    ToBall.Z = 0.0f;
    if (!ToBall.IsNearlyZero())
    {
        FRotator TargetRot = ToBall.Rotation();
        GK->SetActorRotation(FMath::RInterpTo(GK->GetActorRotation(), TargetRot, DeltaTime, 5.0f));
    }
}

FVector USoccerGoalkeeperAI::GetIdealPosition(ASoccerPlayerPawn* GK, ASoccerBall* Ball) const
{
    if (!GK || !Ball) return FVector::ZeroVector;

    ETeamId Team = GK->GetTeamId();
    FVector GoalCenter = GetGoalCenter(Team);
    GoalCenter.Z = 0.0f;

    FVector BallPos = Ball->GetActorLocation();
    BallPos.Z = 0.0f;

    // Direction from goal to ball
    FVector GoalToBall = (BallPos - GoalCenter).GetSafeNormal();

    // ── Depth: how far out from goal line ──

    float BallDistToGoal = FVector::Dist(BallPos, GoalCenter);
    float DepthRatio = FMath::Clamp(BallDistToGoal / (SoccerField::HalfLength * 2.0f), 0.0f, 1.0f);
    float Depth = FMath::Lerp(BaseDepth, MaxDepth, DepthRatio);

    // When ball is in opponent's half, push up slightly
    FVector OppGoal = (Team == ETeamId::Home)
        ? SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
    OppGoal.Z = 0.0f;

    float BallDistToOppGoal = FVector::Dist(BallPos, OppGoal);
    if (BallDistToOppGoal < SoccerField::HalfLength)
    {
        Depth += AdvancedPositioningPush;
    }

    // Base position: on the line between goal center and ball, at Depth from goal
    FVector BasePos = GoalCenter + GoalToBall * Depth;

    // ── Lateral: narrow the angle ──

    // Bisecting angle between goal posts and ball
    float HalfGoalW = SoccerField::GoalWidth / 2.0f;
    FVector LeftPost = GoalCenter + FVector(0.0f, -HalfGoalW, 0.0f);
    FVector RightPost = GoalCenter + FVector(0.0f, HalfGoalW, 0.0f);

    FVector ToBallFromLeft = (BallPos - LeftPost).GetSafeNormal();
    FVector ToBallFromRight = (BallPos - RightPost).GetSafeNormal();

    // Center of the two angles - position GK to cover center of goal mouth from ball's perspective
    FVector AngleCenter = (ToBallFromLeft + ToBallFromRight).GetSafeNormal();

    // Shift laterally toward ball Y, scaled by AngleNarrowing
    float BallY = BallPos.Y;
    float ShiftedY = GoalCenter.Y + (BallY - GoalCenter.Y) * AngleNarrowing;

    // Combine depth and lateral shift
    FVector IdealPos = GoalCenter + FVector(GoalToBall.X * Depth, ShiftedY - GoalCenter.Y, 0.0f);

    // Clamp to reasonable bounds (don't go too wide or too deep)
    float MaxY = HalfGoalW + 100.0f;
    IdealPos.Y = FMath::Clamp(IdealPos.Y, GoalCenter.Y - MaxY, GoalCenter.Y + MaxY);

    // Don't go behind goal line
    if (Team == ETeamId::Home)
    {
        IdealPos.X = FMath::Max(IdealPos.X, GoalCenter.X - Depth); // Keep in front of goal
    }
    else
    {
        IdealPos.X = FMath::Min(IdealPos.X, GoalCenter.X + Depth);
    }

    IdealPos.Z = SoccerField::GamePlaneZ;
    return IdealPos;
}

FVector USoccerGoalkeeperAI::ShouldDive(ASoccerPlayerPawn* GK, ASoccerBall* Ball) const
{
    if (!GK || !Ball) return FVector::ZeroVector;

    float BallSpeed = Ball->GetBallSpeed();

    // Ball must be moving fast (it's a shot)
    if (BallSpeed < ShotSpeedThreshold) return FVector::ZeroVector;

    FVector BallPos = Ball->GetActorLocation();
    FVector BallVel = Ball->GetBallVelocity();
    FVector GKPos = GK->GetActorLocation();
    GKPos.Z = 0.0f;

    ETeamId Team = GK->GetTeamId();
    FVector GoalCenter = GetGoalCenter(Team);
    GoalCenter.Z = 0.0f;

    // Is ball heading toward our goal?
    FVector BallToGoal = (GoalCenter - BallPos).GetSafeNormal();
    float DotWithGoal = FVector::DotProduct(BallVel.GetSafeNormal(), BallToGoal);
    if (DotWithGoal < 0.5f) return FVector::ZeroVector; // Not heading toward goal

    // Predict where ball will cross the goal line
    float TimeToGoalLine;
    if (Team == ETeamId::Home)
    {
        float DistToGoalLine = FMath::Abs(BallPos.X - GoalCenter.X);
        if (BallVel.X < 0.0f) return FVector::ZeroVector; // Moving away
        TimeToGoalLine = (BallVel.X > 10.0f) ? DistToGoalLine / BallVel.X : 999.0f;
    }
    else
    {
        float DistToGoalLine = FMath::Abs(BallPos.X - GoalCenter.X);
        if (BallVel.X > 0.0f) return FVector::ZeroVector; // Moving away
        TimeToGoalLine = (BallVel.X < -10.0f) ? DistToGoalLine / -BallVel.X : 999.0f;
    }

    if (TimeToGoalLine > 2.0f || TimeToGoalLine <= 0.0f) return FVector::ZeroVector; // Too far or wrong direction

    // Predict ball Y at goal line
    float PredictedY = BallPos.Y + BallVel.Y * TimeToGoalLine;

    // Is it within the goal?
    float HalfGoalW = SoccerField::GoalWidth / 2.0f;
    if (FMath::Abs(PredictedY - GoalCenter.Y) > HalfGoalW) return FVector::ZeroVector; // Going wide

    // Is GK already in position to save it?
    float GKDistToPredicted = FMath::Abs(GKPos.Y - PredictedY);
    if (GKDistToPredicted < 80.0f) return FVector::ZeroVector; // Already in position

    // Can GK reach it?
    if (GKDistToPredicted > MaxDiveDistance) return FVector::ZeroVector; // Too far

    // Dive direction: toward the predicted Y
    float DiveDirY = FMath::Sign(PredictedY - GKPos.Y);
    return FVector(0.0f, DiveDirY, 0.0f);
}

bool USoccerGoalkeeperAI::ShouldRushOut(ASoccerPlayerPawn* GK, ASoccerBall* Ball) const
{
    if (!GK || !Ball) return false;

    FVector BallPos = Ball->GetActorLocation();
    BallPos.Z = 0.0f;

    ETeamId Team = GK->GetTeamId();
    FVector GoalCenter = GetGoalCenter(Team);
    GoalCenter.Z = 0.0f;

    float BallDistFromGoal = FVector::Dist(BallPos, GoalCenter);
    float GKDistFromGoal = FVector::Dist(GK->GetActorLocation(), GoalCenter);

    // Ball must be close enough to rush
    if (BallDistFromGoal > RushActivationDistance) return false;

    // Ball must not be too close to goal (then just stay on line)
    if (BallDistFromGoal < RushMinBallDistFromGoal) return false;

    // Ball must be relatively slow (don't rush a fast ball)
    if (Ball->GetBallSpeed() > 800.0f) return false;

    // Ball must be in front of goal (not behind)
    float BallX = BallPos.X;
    float GoalX = GoalCenter.X;
    bool bBallInFront = (Team == ETeamId::Home) ? (BallX > GoalX) : (BallX < GoalX);
    if (!bBallInFront) return false;

    // No opponent closer to the ball than us
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return false;

    float GKDistToBall = FVector::Dist(GK->GetActorLocation(), BallPos);

    TArray<ASoccerPlayerPawn*>& Opponents = (Team == ETeamId::Home) ? GM->AwayPlayers : GM->HomePlayers;
    for (ASoccerPlayerPawn* Opp : Opponents)
    {
        if (!Opp) continue;
        float OppDistToBall = FVector::Dist(Opp->GetActorLocation(), BallPos);
        if (OppDistToBall < GKDistToBall * 0.7f) return false; // Opponent much closer
    }

    return true;
}

FVector USoccerGoalkeeperAI::GetGoalCenter(ETeamId Team) const
{
    return (Team == ETeamId::Home)
        ? SoccerField::HomeGoalCenter()
        : SoccerField::AwayGoalCenter();
}

void USoccerGoalkeeperAI::GetPostAngles(FVector GKPos, FVector GoalCenter, float& OutLeftAngle, float& OutRightAngle) const
{
    float HalfGoalW = SoccerField::GoalWidth / 2.0f;
    FVector LeftPost = GoalCenter + FVector(0.0f, -HalfGoalW, 0.0f);
    FVector RightPost = GoalCenter + FVector(0.0f, HalfGoalW, 0.0f);

    FVector ToLeft = (LeftPost - GKPos).GetSafeNormal();
    FVector ToRight = (RightPost - GKPos).GetSafeNormal();

    // Angle between the two vectors
    float CosAngle = FVector::DotProduct(ToLeft, ToRight);
    float TotalAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAngle, -1.0f, 1.0f)));

    OutLeftAngle = -TotalAngle / 2.0f;
    OutRightAngle = TotalAngle / 2.0f;
}

bool USoccerGoalkeeperAI::IsBallHeadingForGoal(ASoccerBall* Ball, FVector GoalCenter) const
{
    if (!Ball) return false;

    FVector BallPos = Ball->GetActorLocation();
    BallPos.Z = 0.0f;
    FVector BallVel = Ball->GetBallVelocity();
    BallVel.Z = 0.0f;

    if (BallVel.Size() < ShotSpeedThreshold) return false;

    // Simple trajectory check: will ball cross goal line within goal width?
    FVector BallToGoal = (GoalCenter - BallPos);
    BallToGoal.Z = 0.0f;
    float Dot = FVector::DotProduct(BallVel.GetSafeNormal(), BallToGoal.GetSafeNormal());

    return Dot > 0.7f; // Heading generally toward goal
}
