#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsMuscleComponent.generated.h"

/**
 * Manages joint motor strengths and target angles for the physics ragdoll.
 * Each joint group (spine, arms, legs, head) has configurable motor strength
 * that varies by state (Standing, Running, Stumbling).
 * Section 3 of the active ragdoll design spec.
 */
UCLASS(ClassGroup = (PhysicsPawn), meta = (BlueprintSpawnableComponent))
class SOCCERSIM_API UPhysicsMuscleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPhysicsMuscleComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // -- Motor Strengths per State --
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Standing")
    float SpineStrengthStanding = 0.8f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Standing")
    float ArmStrengthStanding = 0.4f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Standing")
    float LegStrengthStanding = 0.9f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Standing")
    float HeadStrengthStanding = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Running")
    float SpineStrengthRunning = 0.6f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Running")
    float ArmStrengthRunning = 0.3f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Running")
    float LegStrengthRunning = 0.7f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Running")
    float HeadStrengthRunning = 0.4f;

    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Stumbling")
    float SpineStrengthStumbling = 0.3f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Stumbling")
    float ArmStrengthStumbling = 0.15f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Stumbling")
    float LegStrengthStumbling = 0.4f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Stumbling")
    float HeadStrengthStumbling = 0.2f;

    // -- Blend Speeds --
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Blend")
    float ToRunningBlendSpeed = 5.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Blend")
    float ToStumblingBlendSpeed = 12.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Blend")
    float RecoveryBlendSpeed = 2.0f;

    // -- Force Clamps (Nm) --
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Clamps")
    float SpineMaxForce = 5000.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Clamps")
    float ArmMaxForce = 1000.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Clamps")
    float LegMaxForce = 4000.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Muscles|Clamps")
    float HeadMaxForce = 500.0f;

    /** Set standing-pose target angles on all joints. */
    void ApplyStandingPose();

private:
    USkeletalMeshComponent* MeshComp = nullptr;

    /** Configure physical animation drives on all bodies. */
    void SetupPhysicalAnimationDrives();
};
