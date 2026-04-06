#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "SoccerPhysicsPawn.generated.h"

class UPhysicsLocomotionComponent;
class UPhysicsMuscleComponent;
class UCapsuleComponent;

/**
 * Physics-driven player pawn using active ragdoll (Euphoria-style).
 * No CharacterMovementComponent — the physics bodies ARE the animation.
 * Designed to coexist with ASoccerPlayerPawn (only one physics pawn on pitch).
 */
UCLASS()
class SOCCERSIM_API ASoccerPhysicsPawn : public APawn
{
    GENERATED_BODY()

public:
    ASoccerPhysicsPawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void PostInitializeComponents() override;

    // -- Input interface (mirrors ASoccerPlayerPawn for AI/HUD compatibility) --
    void SetMovementInput(FVector2D Input);
    void SetSprinting(bool bSprint);

    UFUNCTION(BlueprintPure, Category = "PhysicsPawn")
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category = "PhysicsPawn")
    bool HasMovementInput() const { return !CurrentMovementInput.IsNearlyZero(); }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PhysicsPawn|Mesh", meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    FSoftObjectPath SkeletalMeshPath;

    /** Physics asset to apply to the skeletal mesh. If empty, uses the mesh's default. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PhysicsPawn|Mesh", meta = (AllowedClasses = "/Script/Engine.PhysicsAsset"))
    FSoftObjectPath PhysicsAssetPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicsPawn|Components")
    USkeletalMeshComponent* MeshComp = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicsPawn|Components")
    UPhysicalAnimationComponent* PhysAnimComp = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicsPawn|Components")
    UPhysicsLocomotionComponent* LocomotionComp = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicsPawn|Components")
    UPhysicsMuscleComponent* MuscleComp = nullptr;

    /** Game-level collision capsule (follows pelvis). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PhysicsPawn|Components")
    UCapsuleComponent* CapsuleComp = nullptr;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float RootDriveSpring = 4000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float RootDriveDamping = 300.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float GroundSpring = 8000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float GroundDamping = 500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float TurnSpring = 1500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float TurnDamping = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float StandingHeight = 95.0f; // pelvis Z when standing (cm)

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float JogSpeed = 400.0f; // cm/s

    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|RootDrive")
    float SprintSpeed = 750.0f; // cm/s

    FVector2D CurrentMovementInput = FVector2D::ZeroVector;
    bool bIsSprinting = false;

    /** Name of the pelvis bone in the physics asset. */
    UPROPERTY(EditDefaultsOnly, Category = "PhysicsPawn|Setup")
    FName PelvisBoneName = FName(TEXT("Hips"));

private:
    void InitPhysics();
    void UpdateCapsuleFollow();
};
