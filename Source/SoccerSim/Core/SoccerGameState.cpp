#include "SoccerGameState.h"
#include "SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"

ASoccerGameState::ASoccerGameState()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASoccerGameState::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bClockRunning)
    {
        MatchClockSeconds += DeltaTime;
    }

    AGameModeBase* GM = GetWorld()->GetAuthGameMode();
    if (!GM) return;

    float HalfDuration = 270.0f;
    if (const ASoccerGameMode* SoccerGM = Cast<ASoccerGameMode>(GM))
    {
        HalfDuration = SoccerGM->HalfDurationSeconds;
    }

    if (bClockRunning && MatchClockSeconds >= HalfDuration)
    {
        if (CurrentPhase == EMatchPhase::FirstHalf)
        {
            SetMatchPhase(EMatchPhase::HalfTime);
        }
        else if (CurrentPhase == EMatchPhase::SecondHalf)
        {
            SetMatchPhase(EMatchPhase::FullTime);
        }
    }
}

void ASoccerGameState::SetMatchPhase(EMatchPhase NewPhase)
{
    if (CurrentPhase == NewPhase) return;

    CurrentPhase = NewPhase;

    switch (NewPhase)
    {
    case EMatchPhase::FirstHalf:
    case EMatchPhase::SecondHalf:
    case EMatchPhase::ExtraFirstHalf:
    case EMatchPhase::ExtraSecondHalf:
        bClockRunning = true;
        break;
    case EMatchPhase::KickOff:
    case EMatchPhase::SecondHalfKickOff:
        bClockRunning = false;
        break;
    case EMatchPhase::HalfTime:
        bClockRunning = false;
        break;
    case EMatchPhase::FullTime:
        bClockRunning = false;
        break;
    default:
        bClockRunning = false;
        break;
    }

    OnMatchPhaseChanged.Broadcast(NewPhase);
}

void ASoccerGameState::ResetMatchClock()
{
    MatchClockSeconds = 0.0f;
}

FString ASoccerGameState::GetMatchClockDisplay() const
{
    int32 TotalSeconds = FMath::FloorToInt(MatchClockSeconds);
    int32 Minutes = TotalSeconds / 60;
    int32 Seconds = TotalSeconds % 60;
    return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}
