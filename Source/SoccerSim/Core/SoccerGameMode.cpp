#include "SoccerGameMode.h"
#include "SoccerPlayerController.h"
#include "SoccerSim/Player/SoccerPhysicsPawn.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"

ASoccerGameMode::ASoccerGameMode()
{
    PlayerControllerClass = ASoccerPlayerController::StaticClass();
    DefaultPawnClass = ASoccerPhysicsPawn::StaticClass();
}

void ASoccerGameMode::StartPlay()
{
    Super::StartPlay();

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerGameMode::StartPlay - Physics sandbox"));
}