#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "SoccerPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ASoccerPhysicsPawn;

UCLASS()
class SOCCERSIM_API ASoccerPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASoccerPlayerController();

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Move;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Sprint;

private:
    void HandleMove(const FInputActionValue& Value);
    void HandleSprintStarted(const FInputActionValue& Value);
    void HandleSprintCompleted(const FInputActionValue& Value);

    ASoccerPhysicsPawn* GetPhysicsPawn() const;
};
