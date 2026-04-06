#include "SoccerPhysicsPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "SoccerSim/SoccerSim.h"
#include "PhysicsLocomotionComponent.h"
#include "PhysicsMuscleComponent.h"

ASoccerPhysicsPawn::ASoccerPhysicsPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // -- Capsule (game-level collision, follows pelvis) --
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    CapsuleComp->InitCapsuleSize(34.0f, 94.0f);
    CapsuleComp->SetCollisionProfileName(TEXT("SoccerPlayer"));
    CapsuleComp->SetSimulatePhysics(false);
    CapsuleComp->SetEnableGravity(false);
    RootComponent = CapsuleComp;

    // -- Skeletal Mesh (physics-driven, NOT attached to capsule — it follows pelvis) --
    MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PhysicsMesh"));
    MeshComp->SetupAttachment(CapsuleComp);
    MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
    MeshComp->SetSimulatePhysics(false); // enabled in BeginPlay after asset setup
    MeshComp->bBlendPhysics = true;
    // No AnimBP — physics drives the pose directly

    // -- Physical Animation Component --
    PhysAnimComp = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysAnimComp"));

    // -- Custom Components --
    LocomotionComp = CreateDefaultSubobject<UPhysicsLocomotionComponent>(TEXT("LocomotionComp"));
    MuscleComp = CreateDefaultSubobject<UPhysicsMuscleComponent>(TEXT("MuscleComp"));

    // -- Hardcoded mesh & physics asset for Phase 1 prototype --
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshLoader(TEXT("/Game/Anim/Mixamo/Dribble.Dribble"));
    if (MeshLoader.Succeeded())
    {
        MeshComp->SetSkeletalMesh(MeshLoader.Object);
    }
    static ConstructorHelpers::FObjectFinder<UPhysicsAsset> PhysAssetLoader(TEXT("/Game/Anim/Mixamo/Dribble_PhysicsAsset.Dribble_PhysicsAsset"));
    if (PhysAssetLoader.Succeeded())
    {
        MeshComp->SetPhysicsAsset(PhysAssetLoader.Object);
    }

    bReplicates = false;
}

void ASoccerPhysicsPawn::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Load skeletal mesh if path is set
    if (SkeletalMeshPath.IsValid())
    {
        if (USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(SkeletalMeshPath.TryLoad()))
        {
            MeshComp->SetSkeletalMesh(LoadedMesh);
        }
    }

    // Load physics asset if path is set
    if (PhysicsAssetPath.IsValid())
    {
        if (UPhysicsAsset* PhysAsset = Cast<UPhysicsAsset>(PhysicsAssetPath.TryLoad()))
        {
            MeshComp->SetPhysicsAsset(PhysAsset);
        }
    }
}

void ASoccerPhysicsPawn::BeginPlay()
{
    Super::BeginPlay();

    InitPhysics();
}

void ASoccerPhysicsPawn::InitPhysics()
{
    if (!MeshComp || !MeshComp->GetSkeletalMeshAsset()) return;

    // Must have a physics asset to simulate
    if (!MeshComp->GetPhysicsAsset())
    {
        UE_LOG(LogSoccerSim, Warning, TEXT("SoccerPhysicsPawn: No PhysicsAsset set. Ragdoll physics will not work."));
        return;
    }

    // Enable physics simulation on all bodies
    MeshComp->SetSimulatePhysics(true);
    MeshComp->WakeAllRigidBodies();

    // Set physical animation to drive bodies toward targets
    if (PhysAnimComp)
    {
        PhysAnimComp->SetSkeletalMeshComponent(MeshComp);

        // Apply physical animation to all bodies: drive orientation and position
        FPhysicalAnimationData AnimData;
        AnimData.bIsLocalSimulation = false;
        AnimData.OrientationStrength = 100.0f;
        AnimData.AngularVelocityStrength = 100.0f;
        AnimData.PositionStrength = 100.0f;
        AnimData.VelocityStrength = 100.0f;
        AnimData.MaxAngularForce = 5000.0f;
        AnimData.MaxLinearForce = 5000.0f;

        PhysAnimComp->ApplyPhysicalAnimationSettingsBelow(NAME_None, AnimData, true);
    }

    // Apply standing pose through muscle system
    if (MuscleComp)
    {
        MuscleComp->ApplyStandingPose();
    }

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerPhysicsPawn: Physics initialized on %s"), *GetName());
}

void ASoccerPhysicsPawn::SetMovementInput(FVector2D Input)
{
    CurrentMovementInput = Input;
}

void ASoccerPhysicsPawn::SetSprinting(bool bSprint)
{
    bIsSprinting = bSprint;
}

void ASoccerPhysicsPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Forward input to locomotion component
    if (LocomotionComp)
    {
        LocomotionComp->SetMovementInput(CurrentMovementInput);
        LocomotionComp->SetSprinting(bIsSprinting);
    }

    // Keep capsule following the pelvis
    UpdateCapsuleFollow();
}

void ASoccerPhysicsPawn::UpdateCapsuleFollow()
{
    if (!MeshComp || !CapsuleComp) return;

    // Get pelvis body transform
    FTransform PelvisTransform = MeshComp->GetSocketTransform(PelvisBoneName, RTS_World);
    FVector PelvisPos = PelvisTransform.GetTranslation();
    FQuat PelvisQuat = PelvisTransform.GetRotation();

    // Move capsule to pelvis XY, keep at ground + half height
    FVector CapsulePos(PelvisPos.X, PelvisPos.Y, StandingHeight);
    CapsuleComp->SetWorldLocation(CapsulePos);

    // Yaw-only rotation from pelvis
    FRotator PelvisRotator(PelvisQuat);
    FRotator CapsuleRot(0.0f, PelvisRotator.Yaw, 0.0f);
    CapsuleComp->SetWorldRotation(CapsuleRot);
}
