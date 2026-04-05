#include "SoccerPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/Core/SoccerGameState.h"
#include "SoccerSim/Core/SoccerGameInstance.h"
#include "SoccerSim/AI/SoccerAIController.h"
#include "SoccerSim/Camera/SoccerBroadcastCamera.h"
#include "SoccerSim/UI/SoccerHUDWidget.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"

ASoccerPlayerController::ASoccerPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
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

    if (HUDWidgetClass)
    {
        if (USoccerHUDWidget* HUDWidget = CreateWidget<USoccerHUDWidget>(this, HUDWidgetClass))
        {
            HUDWidget->AddToViewport();
        }
    }
}

void ASoccerPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EIC)
    {
        UE_LOG(LogSoccerSim, Error, TEXT("Failed to get EnhancedInputComponent! Is Enhanced Input enabled in project settings?"));
        return;
    }

    if (IA_Move)
        EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASoccerPlayerController::HandleMove);
    if (IA_Sprint)
    {
        EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleSprintStarted);
        EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ASoccerPlayerController::HandleSprintCompleted);
    }
    if (IA_Pass)
        EIC->BindAction(IA_Pass, ETriggerEvent::Started, this, &ASoccerPlayerController::HandlePass);
    if (IA_Shoot)
    {
        EIC->BindAction(IA_Shoot, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleShootStarted);
        EIC->BindAction(IA_Shoot, ETriggerEvent::Completed, this, &ASoccerPlayerController::HandleShootCompleted);
    }
    if (IA_ThroughBall)
        EIC->BindAction(IA_ThroughBall, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleThroughBall);
    if (IA_Tackle)
        EIC->BindAction(IA_Tackle, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleTackle);
    if (IA_SwitchPlayer)
        EIC->BindAction(IA_SwitchPlayer, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleSwitchPlayer);
    if (IA_Lob)
        EIC->BindAction(IA_Lob, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleLob);
    if (IA_ViewToggle)
        EIC->BindAction(IA_ViewToggle, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleViewToggle);
    else
        InputComponent->BindKey(EKeys::V, EInputEvent::IE_Pressed, this, &ASoccerPlayerController::HandleViewToggleKey);

    InputComponent->BindKey(EKeys::Escape, EInputEvent::IE_Pressed, this, &ASoccerPlayerController::HandlePauseToggle);
}

void ASoccerPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bMatchEndShown && MatchEndWidgetClass)
    {
        ASoccerGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASoccerGameState>() : nullptr;
        if (GS && GS->GetMatchPhase() == EMatchPhase::FullTime)
        {
            bMatchEndShown = true;
            if (UUserWidget* Widget = CreateWidget<UUserWidget>(this, MatchEndWidgetClass))
            {
                Widget->AddToViewport(1);
            }
        }
    }
}

void ASoccerPlayerController::HandlePauseToggle()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (World->IsPaused())
    {
        ResumeGame();
    }
    else
    {
        if (PauseMenuWidgetClass)
        {
            PauseMenuWidgetInstance = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
            if (PauseMenuWidgetInstance)
            {
                PauseMenuWidgetInstance->AddToViewport(1);
            }
        }
        UGameplayStatics::SetGamePaused(World, true);
    }
}

void ASoccerPlayerController::ResumeGame()
{
    if (PauseMenuWidgetInstance)
    {
        PauseMenuWidgetInstance->RemoveFromParent();
        PauseMenuWidgetInstance = nullptr;
    }
    UWorld* World = GetWorld();
    if (World)
    {
        UGameplayStatics::SetGamePaused(World, false);
    }
}

void ASoccerPlayerController::QuitToMainMenu()
{
    ResumeGame();
    if (USoccerGameInstance* GI = GetGameInstance<USoccerGameInstance>())
    {
        GI->OpenMainMenuMap();
    }
}

void ASoccerPlayerController::Rematch()
{
    if (USoccerGameInstance* GI = GetGameInstance<USoccerGameInstance>())
    {
        GI->Rematch();
    }
}

ASoccerPlayerPawn* ASoccerPlayerController::GetSoccerPawn() const
{
    return Cast<ASoccerPlayerPawn>(GetPawn());
}

void ASoccerPlayerController::HandleMove(const FInputActionValue& Value)
{
    FVector2D MovementInput = Value.Get<FVector2D>();
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn)
    {
        SoccerPawn->SetMovementInput(MovementInput);
    }
}

void ASoccerPlayerController::HandleSprintStarted(const FInputActionValue& Value)
{
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->SetSprinting(true);
}

void ASoccerPlayerController::HandleSprintCompleted(const FInputActionValue& Value)
{
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->SetSprinting(false);
}

void ASoccerPlayerController::HandlePass(const FInputActionValue& Value)
{
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->AttemptKick(EKickType::ShortPass);
}

void ASoccerPlayerController::HandleShootStarted(const FInputActionValue& Value)
{
    bIsChargingShot = true;
    ShotChargeStartTime = GetWorld()->GetTimeSeconds();

    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->BeginShotCharge();
}

void ASoccerPlayerController::HandleShootCompleted(const FInputActionValue& Value)
{
    if (!bIsChargingShot) return;
    bIsChargingShot = false;

    float ChargeTime = GetWorld()->GetTimeSeconds() - ShotChargeStartTime;
    ChargeTime = FMath::Clamp(ChargeTime, 0.0f, 1.5f);

    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->AttemptShot(ChargeTime);
}

void ASoccerPlayerController::HandleThroughBall(const FInputActionValue& Value)
{
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->AttemptKick(EKickType::ThroughBall);
}

void ASoccerPlayerController::HandleTackle(const FInputActionValue& Value)
{
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->AttemptTackle();
}

void ASoccerPlayerController::HandleSwitchPlayer(const FInputActionValue& Value)
{
    SwitchToNearestPlayerToBall();
}

void ASoccerPlayerController::HandleLob(const FInputActionValue& Value)
{
    ASoccerPlayerPawn* SoccerPawn = GetSoccerPawn();
    if (SoccerPawn) SoccerPawn->AttemptKick(EKickType::LobPass);
}

void ASoccerPlayerController::HandleViewToggle(const FInputActionValue& Value)
{
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(GetWorld()->GetAuthGameMode());
    if (!GM || !GM->BroadcastCamera) return;

    AActor* CurrentView = GetViewTarget();
    if (CurrentView == GM->BroadcastCamera)
    {
        if (APawn* P = GetPawn())
        {
            SetViewTarget(P);
        }
    }
    else
    {
        SetViewTarget(GM->BroadcastCamera);
    }
}

void ASoccerPlayerController::HandleViewToggleKey(FKey Key)
{
    HandleViewToggle(FInputActionValue());
}

void ASoccerPlayerController::SwitchToNearestPlayerToBall()
{
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM || !GM->MatchBall) return;

    TArray<ASoccerPlayerPawn*>& MyTeam = (ControlledTeam == ETeamId::Home) ? GM->HomePlayers : GM->AwayPlayers;
    ASoccerPlayerPawn* CurrentPawn = GetSoccerPawn();
    FVector BallLoc = GM->MatchBall->GetActorLocation();

    ASoccerPlayerPawn* BestCandidate = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    for (ASoccerPlayerPawn* P : MyTeam)
    {
        if (P == CurrentPawn) continue;
        if (P->GetPlayerPosition() == EPlayerPosition::GK) continue;

        float Dist = FVector::Dist(P->GetActorLocation(), BallLoc);

        FVector ToBall = BallLoc - P->GetActorLocation();
        float ForwardDot = FVector::DotProduct(ToBall.GetSafeNormal(), P->GetActorForwardVector());
        float AnglePenalty = (ForwardDot < 0.0f) ? 200.0f : 0.0f;

        float Score = Dist + AnglePenalty;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestCandidate = P;
        }
    }

    if (BestCandidate && BestCandidate != CurrentPawn)
    {
        SwitchToPlayer(BestCandidate);
    }
}

void ASoccerPlayerController::SwitchToPlayer(ASoccerPlayerPawn* NewPlayer)
{
    if (!NewPlayer) return;

    ASoccerPlayerPawn* PreviousPawn = GetSoccerPawn();
    ASoccerAIController* PreviousAI = PreviousPawn ? PreviousPawn->GetAssignedAIController() : nullptr;

    if (PreviousPawn)
    {
        PreviousPawn->OnPlayerControlReleased();
    }

    UnPossess();
    Possess(NewPlayer);
    NewPlayer->OnPlayerControlAcquired();

    if (PreviousAI && PreviousPawn)
    {
        PreviousAI->Possess(PreviousPawn);
    }

    UE_LOG(LogSoccerSim, Verbose, TEXT("Switched control to player: %s"), *NewPlayer->GetName());
}
