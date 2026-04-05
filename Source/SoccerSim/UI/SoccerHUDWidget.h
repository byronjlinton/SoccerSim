#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerHUDWidget.generated.h"

class UTextBlock;
class UCanvasPanel;
class ASoccerGameState;

UCLASS()
class SOCCERSIM_API USoccerHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "HUD")
    int32 GetHomeScore() const;

    UFUNCTION(BlueprintCallable, Category = "HUD")
    int32 GetAwayScore() const;

    UFUNCTION(BlueprintCallable, Category = "HUD")
    FString GetMatchClockText() const;

    UFUNCTION(BlueprintCallable, Category = "HUD")
    FString GetCurrentPhaseText() const;

    UFUNCTION(BlueprintPure, Category = "HUD")
    ASoccerGameState* GetSoccerGameState() const;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> HomeScoreText;

    UPROPERTY()
    TObjectPtr<UTextBlock> AwayScoreText;

    UPROPERTY()
    TObjectPtr<UTextBlock> ClockText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PhaseText;

    bool bWidgetsCreated = false;
    int32 LastHomeScore = -1;
    int32 LastAwayScore = -1;
    FString LastClockText;
    FString LastPhaseText;
};
