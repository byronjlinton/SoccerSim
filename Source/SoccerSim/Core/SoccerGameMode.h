#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoccerGameMode.generated.h"

UCLASS()
class SOCCERSIM_API ASoccerGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASoccerGameMode();

    virtual void StartPlay() override;
};
