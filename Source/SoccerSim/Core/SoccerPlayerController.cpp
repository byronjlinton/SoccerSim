#include "SoccerPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SoccerSim/Player/SoccerPhysicsPawn.h"
#include "SoccerSim/SoccerSim.h"

ASoccerPlayerController::ASoccerPlayerController()
{
}

void ASoccerPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
        else
        {
            UE_LOG(LogSoccerSim, Warning, TEXT("DefaultMappingContext is not set on PlayerController!"));
        }
    }
}

void ASoccerPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EIC)
    {
        UE_LOG(LogSoccerSim, Error, TEXT("Failed to get EnhancedInputComponent!"));
        return;
    }

    if (IA_Move)
        EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASoccerPlayerController::HandleMove);
    if (IA_Sprint)
    {
        EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleSprintStarted);
        EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ASoccerPlayerController::HandleSprintCompleted);
    }
}

ASoccerPhysicsPawn* ASoccerPlayerController::GetPhysicsPawn() const
{
    return Cast<ASoccerPhysicsPawn>(GetPawn());
}

void ASoccerPlayerController::HandleMove(const FInputActionValue& Value)
{
    FVector2D MovementInput = Value.Get<FVector2D>();
    if (ASoccerPhysicsPawn* PhysPawn = GetPhysicsPawn())
    {
        PhysPawn->SetMovementInput(MovementInput);
    }
}

void ASoccerPlayerController::HandleSprintStarted(const FInputActionValue& Value)
{
    if (ASoccerPhysicsPawn* PhysPawn = GetPhysicsPawn())
    {
        PhysPawn->SetSprinting(true);
    }
}

void ASoccerPlayerController::HandleSprintCompleted(const FInputActionValue& Value)
{
    if (ASoccerPhysicsPawn* PhysPawn = GetPhysicsPawn())
    {
        PhysPawn->SetSprinting(false);
    }
}