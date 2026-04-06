#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsLocomotionComponent.generated.h"

/**
 * The "nervous system" for the active ragdoll. Orchestrates root drive,
 * balance, pose generation, stumble detection, and state machine.
 * Section 2 & 4 of the active ragdoll design spec.
 */
UCLASS(ClassGroup = (PhysicsPawn), meta = (BlueprintSpawnableComponent))
class SOCCERSIM_API UPhysicsLocomotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPhysicsLocomotionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void SetMovementInput(FVector2D Input) { DesiredInput = Input; }
    void SetSprinting(bool bSprint) { bDesiredSprint = bSprint; }

    // -- Root Drive Parameters --
    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float DriveSpring = 4000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float DriveDamping = 300.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float GroundSpringK = 8000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float GroundDampingK = 500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float TurnSpringK = 1500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float TurnDampingK = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float StandingPelvisHeight = 95.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float JogSpeed = 400.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RootDrive")
    float SprintSpeed = 750.0f;

    /** Name of the pelvis bone in the physics asset. */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    FName PelvisBoneName = FName(TEXT("Hips"));

    /** Name of the bone to use for root orientation tracking. */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    FName RootBoneName = FName(TEXT("Root"));

private:
    FVector2D DesiredInput = FVector2D::ZeroVector;
    bool bDesiredSprint = false;

    void ApplyRootDrive(float DeltaTime);
    void ApplyGroundSpring(float DeltaTime);
    void ApplyRotationDrive(float DeltaTime);

    // Cached references (set in BeginPlay)
    USkeletalMeshComponent* MeshComp = nullptr;
    int32 PelvisBodyIndex = INDEX_NONE;
};
