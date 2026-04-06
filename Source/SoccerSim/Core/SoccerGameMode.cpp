#include "SoccerGameMode.h"
#include "SoccerPlayerController.h"
#include "SoccerSim/Player/SoccerPhysicsPawn.h"
#include "SoccerSim/SoccerSim.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASoccerGameMode::ASoccerGameMode()
{
    PlayerControllerClass = ASoccerPlayerController::StaticClass();

    // Use SpectatorPawn as default so PIE Login succeeds,
    // then we spawn the real physics pawn manually in StartPlay.
    DefaultPawnClass = ASpectatorPawn::StaticClass();
}

void ASoccerGameMode::StartPlay()
{
    Super::StartPlay();

    // Spawn the physics pawn manually
    FTransform SpawnTransform(FVector(0.0f, 0.0f, 95.0f));

    // Try BP first, then C++ class
    UClass* PawnClass = LoadClass<APawn>(nullptr,
        TEXT("/Game/Blueprints/BP_SoccerPhysicsPawn.BP_SoccerPhysicsPawn_C"));
    if (!PawnClass)
    {
        PawnClass = ASoccerPhysicsPawn::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnParams);
    if (SpawnedPawn)
    {
        UE_LOG(LogSoccerSim, Log, TEXT("Spawned physics pawn: %s"), *SpawnedPawn->GetName());

        // Possess it with the player controller
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            // Unpossess the spectator
            if (APawn* OldPawn = PC->GetPawn())
            {
                PC->UnPossess();
                OldPawn->Destroy();
            }
            PC->Possess(SpawnedPawn);
            UE_LOG(LogSoccerSim, Log, TEXT("PlayerController possessed physics pawn"));
        }
    }
    else
    {
        UE_LOG(LogSoccerSim, Error, TEXT("Failed to spawn physics pawn!"));
    }
}
