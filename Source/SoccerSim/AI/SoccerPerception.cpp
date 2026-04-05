#include "SoccerPerception.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "Kismet/GameplayStatics.h"

FPerceivedPlayer FSoccerPerception::GetBestPassTarget() const
{
    FPerceivedPlayer Best;
    float BestScore = -1.0f;

    for (const FPerceivedPlayer& Teammate : NearbyTeammates)
    {
        if (!Teammate.Pawn || !Teammate.Pawn->IsValidLowLevel()) continue;
        if (Teammate.Pawn->GetPlayerPosition() == EPlayerPosition::GK) continue;

        float LaneScore = Teammate.bPassingLaneClear ? 1.0f : 0.15f;
        float DistScore = FMath::Clamp(1.0f - Teammate.Distance / 4000.0f, 0.0f, 1.0f);
        float Score = LaneScore * 0.5f + Teammate.PositionalValue * 0.3f + DistScore * 0.2f;

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Teammate;
        }
    }
    return Best;
}

bool FSoccerPerception::IsPassingLaneClearTo(FVector Target) const
{
    // Check if any opponent is near the line between ball and target
    for (const FPerceivedPlayer& Opp : NearbyOpponents)
    {
        if (!Opp.Pawn) continue;

        FVector OppLoc = Opp.Pawn->GetActorLocation();
        FVector BallToTarget = (Target - BallLocation).GetSafeNormal();
        FVector BallToOpp = (OppLoc - BallLocation);
        float DistAlongLane = FVector::DotProduct(BallToOpp, BallToTarget);

        if (DistAlongLane < 0.0f || DistAlongLane > (Target - BallLocation).Size()) continue;

        FVector ClosestPointOnLane = BallLocation + BallToTarget * DistAlongLane;
        float PerpDist = FVector::Dist(OppLoc, ClosestPointOnLane);

        if (PerpDist < 200.0f) return false; // 2m clearance needed
    }
    return true;
}

float FSoccerPerception::GetSpaceAhead() const
{
    // Default: large space if no opponents nearby
    float MinDist = 3000.0f;
    for (const FPerceivedPlayer& Opp : NearbyOpponents)
    {
        if (Opp.Distance < MinDist)
        {
            MinDist = Opp.Distance;
        }
    }
    return MinDist;
}

float FSoccerPerception::GetAngleToGoal(ETeamId MyTeam) const
{
    FVector GoalPos = (MyTeam == ETeamId::Home)
        ? SoccerField::AwayGoalCenter()
        : SoccerField::HomeGoalCenter();
    GoalPos.Z = 0.0f;

    // Angle between the two goal posts from ball position
    FVector BallPos2D = BallLocation;
    BallPos2D.Z = 0.0f;

    float HalfGoalW = SoccerField::GoalWidth / 2.0f;
    FVector GoalLeft = GoalPos + FVector(0.0f, -HalfGoalW, 0.0f);
    FVector GoalRight = GoalPos + FVector(0.0f, HalfGoalW, 0.0f);

    FVector ToLeft = (GoalLeft - BallPos2D).GetSafeNormal();
    FVector ToRight = (GoalRight - BallPos2D).GetSafeNormal();

    float CosAngle = FVector::DotProduct(ToLeft, ToRight);
    return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAngle, -1.0f, 1.0f)));
}

float FSoccerPerception::GetDistToGoal(ETeamId MyTeam) const
{
    FVector GoalPos = (MyTeam == ETeamId::Home)
        ? SoccerField::AwayGoalCenter()
        : SoccerField::HomeGoalCenter();
    GoalPos.Z = 0.0f;

    FVector BallPos2D = BallLocation;
    BallPos2D.Z = 0.0f;

    return FVector::Dist(BallPos2D, GoalPos);
}
