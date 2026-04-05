#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SoccerGameInstance.generated.h"

UCLASS()
class SOCCERSIM_API USoccerGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;

    /** Open the match level (call from main menu "Play" button). Reads MatchMapName from config or uses default. */
    UFUNCTION(BlueprintCallable, Category = "SoccerSim")
    void OpenMatchMap();

    /** Open the main menu level (call from pause "Quit to Menu" or match end "Main Menu"). */
    UFUNCTION(BlueprintCallable, Category = "SoccerSim")
    void OpenMainMenuMap();

    /** Open the current level again (rematch). */
    UFUNCTION(BlueprintCallable, Category = "SoccerSim")
    void Rematch();

    UFUNCTION(BlueprintPure, Category = "SoccerSim")
    FName GetMatchMapName() const { return MatchMapName; }

    UFUNCTION(BlueprintPure, Category = "SoccerSim")
    FName GetMainMenuMapName() const { return MainMenuMapName; }

protected:
    UPROPERTY(BlueprintReadOnly, Category = "SoccerSim")
    FName MatchMapName = FName(TEXT("Match"));

    UPROPERTY(BlueprintReadOnly, Category = "SoccerSim")
    FName MainMenuMapName = FName(TEXT("MainMenu"));
};
