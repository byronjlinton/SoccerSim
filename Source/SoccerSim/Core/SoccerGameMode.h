#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerGameMode.generated.h"

class ASoccerBall;
class ASoccerField;
class ASoccerGoal;
class ASoccerPlayerPawn;
class ASoccerGameState;
class ASoccerBroadcastCamera;
class ASoccerAIController;

UCLASS()
class SOCCERSIM_API ASoccerGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASoccerGameMode();

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Match")
    void HandleGoalScored(ETeamId ScoringTeam);

    UFUNCTION(BlueprintCallable, Category = "Match")
    void HandleBallOutOfPlay(EBallState OutType, ETeamId LastTouchTeam);

    UFUNCTION(BlueprintCallable, Category = "Match")
    void ResetToKickOff();

    UPROPERTY(Transient)
    ASoccerBall* MatchBall;

    UPROPERTY(Transient)
    ASoccerField* MatchField;

    UPROPERTY(Transient)
    ASoccerGoal* HomeGoal;

    UPROPERTY(Transient)
    ASoccerGoal* AwayGoal;

    UPROPERTY(Transient)
    TArray<ASoccerPlayerPawn*> HomePlayers;

    UPROPERTY(Transient)
    TArray<ASoccerPlayerPawn*> AwayPlayers;

    UPROPERTY(Transient)
    ASoccerBroadcastCamera* BroadcastCamera;

    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    float HalfDurationSeconds = 270.0f;

    /** Duration of halftime before second half kick-off (seconds). */
    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    float HalftimeDurationSeconds = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    TSubclassOf<ASoccerBall> BallClass;

    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    TSubclassOf<ASoccerPlayerPawn> PlayerPawnClass;

    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    TSubclassOf<ASoccerField> FieldClass;

    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    TSubclassOf<ASoccerGoal> GoalClass;

    UPROPERTY(EditDefaultsOnly, Category = "Match|Config")
    TSubclassOf<ASoccerBroadcastCamera> BroadcastCameraClass;

    UFUNCTION()
    void OnMatchPhaseChanged(EMatchPhase NewPhase);

    void StartSecondHalf();

protected:
    void SpawnField();
    void SpawnStadiumLighting();
    void SpawnBall();
    void SpawnTeams();
    void SpawnTeam(ETeamId Team, const FFormationData& Formation, TArray<ASoccerPlayerPawn*>& OutPlayers);
};
