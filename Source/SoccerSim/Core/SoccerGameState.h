#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, int32, HomeScore, int32, AwayScore);

UCLASS()
class SOCCERSIM_API ASoccerGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ASoccerGameState();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Match")
    void SetMatchPhase(EMatchPhase NewPhase);

    /** Reset match clock to 0 (e.g. at start of second half). */
    UFUNCTION(BlueprintCallable, Category = "Match")
    void ResetMatchClock();

    UFUNCTION(BlueprintPure, Category = "Match")
    EMatchPhase GetMatchPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintPure, Category = "Match")
    float GetMatchClockSeconds() const { return MatchClockSeconds; }

    UFUNCTION(BlueprintPure, Category = "Match")
    FString GetMatchClockDisplay() const;

    UPROPERTY(BlueprintReadOnly, Category = "Match")
    int32 HomeScore = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Match")
    int32 AwayScore = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Match")
    EBallState CurrentBallState = EBallState::Dead;

    UPROPERTY(BlueprintReadOnly, Category = "Match")
    ETeamId LastTouchTeam = ETeamId::None;

    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnMatchPhaseChanged OnMatchPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnScoreChanged OnScoreChanged;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Match")
    EMatchPhase CurrentPhase = EMatchPhase::PreMatch;

    UPROPERTY(BlueprintReadOnly, Category = "Match")
    float MatchClockSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Match")
    bool bClockRunning = false;
};
