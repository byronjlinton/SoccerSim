#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class ASoccerPlayerPawn;
class ASoccerBall;
class USoccerHUDWidget;

UCLASS()
class SOCCERSIM_API ASoccerPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASoccerPlayerController();

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Team")
    void SwitchToNearestPlayerToBall();

    UFUNCTION(BlueprintCallable, Category = "Team")
    void SwitchToPlayer(ASoccerPlayerPawn* NewPlayer);

    UFUNCTION(BlueprintPure, Category = "Team")
    ASoccerPlayerPawn* GetSoccerPawn() const;

    UPROPERTY(BlueprintReadOnly, Category = "Team")
    ETeamId ControlledTeam = ETeamId::Home;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Move;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Sprint;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Pass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Shoot;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ThroughBall;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Tackle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SwitchPlayer;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Lob;

    /** Toggle view between broadcast camera and possessed pawn. If not set, V key is used as fallback. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ViewToggle;

    /** Optional HUD widget shown on screen (score, clock, phase). Set in Blueprint to a WBP that parent class is SoccerHUDWidget. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
    TSubclassOf<USoccerHUDWidget> HUDWidgetClass;

    /** Optional pause menu widget (Resume / Quit to Menu). Assign in Blueprint; Escape toggles pause and shows this. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menus")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    /** Optional match end widget (score, Rematch / Main Menu). Shown when phase becomes FullTime. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menus")
    TSubclassOf<UUserWidget> MatchEndWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "Menus")
    void ResumeGame();

    UFUNCTION(BlueprintCallable, Category = "Menus")
    void QuitToMainMenu();

    UFUNCTION(BlueprintCallable, Category = "Menus")
    void Rematch();

    void HandleMove(const FInputActionValue& Value);
    void HandleSprintStarted(const FInputActionValue& Value);
    void HandleSprintCompleted(const FInputActionValue& Value);
    void HandlePass(const FInputActionValue& Value);
    void HandleShootStarted(const FInputActionValue& Value);
    void HandleShootCompleted(const FInputActionValue& Value);
    void HandleThroughBall(const FInputActionValue& Value);
    void HandleTackle(const FInputActionValue& Value);
    void HandleSwitchPlayer(const FInputActionValue& Value);
    void HandleLob(const FInputActionValue& Value);
    void HandleViewToggle(const FInputActionValue& Value);
    void HandleViewToggleKey(FKey Key);
    void HandlePauseToggle();

    float ShotChargeStartTime = 0.0f;
    bool bIsChargingShot = false;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> PauseMenuWidgetInstance;

    bool bMatchEndShown = false;
};
