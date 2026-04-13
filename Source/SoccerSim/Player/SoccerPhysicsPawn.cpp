#include "SoccerPhysicsPawn.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
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

    // -- Spring Arm (offset behind and above the pawn) --
    USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(CapsuleComp);
    SpringArm->TargetArmLength = 300.0f;
    SpringArm->SocketOffset = FVector(0.0f, 0.0f, 100.0f);
    SpringArm->bUsePawnControlRotation = false;
    SpringArm->bDoCollisionTest = false;

    // -- Camera --
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
    CameraComp->SetupAttachment(SpringArm);

    // -- Skeletal Mesh (physics-driven, auto-detaches from capsule when physics starts) --
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
    else
    {
        UE_LOG(LogSoccerSim, Error, TEXT("SoccerPhysicsPawn: Failed to load skeletal mesh '/Game/Anim/Mixamo/Dribble'"));
    }

    static ConstructorHelpers::FObjectFinder<UPhysicsAsset> PhysAssetLoader(TEXT("/Game/Anim/Mixamo/Dribble_PhysicsAsset.Dribble_PhysicsAsset"));
    if (PhysAssetLoader.Succeeded())
    {
        MeshComp->SetPhysicsAsset(PhysAssetLoader.Object);
    }
    else
    {
        UE_LOG(LogSoccerSim, Error, TEXT("SoccerPhysicsPawn: Failed to load physics asset '/Game/Anim/Mixamo/Dribble_PhysicsAsset'"));
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
    if (!MeshComp || !MeshComp->GetSkeletalMeshAsset())
    {
        UE_LOG(LogSoccerSim, Error, TEXT("SoccerPhysicsPawn: No skeletal mesh assigned. Cannot init physics."));
        return;
    }

    if (!MeshComp->GetPhysicsAsset())
    {
        UE_LOG(LogSoccerSim, Warning, TEXT("SoccerPhysicsPawn: No PhysicsAsset set. Ragdoll physics will not work."));
        return;
    }

    // Log diagnostic info about the physics asset
    UPhysicsAsset* PhysAsset = MeshComp->GetPhysicsAsset();
    UE_LOG(LogSoccerSim, Log, TEXT("SoccerPhysicsPawn: PhysicsAsset '%s' has %d body setups"),
        *PhysAsset->GetName(), PhysAsset->SkeletalBodySetups.Num());
    for (int32 i = 0; i < PhysAsset->SkeletalBodySetups.Num(); ++i)
    {
        if (PhysAsset->SkeletalBodySetups[i])
        {
            UE_LOG(LogSoccerSim, Log, TEXT("  Body[%d]: BoneName='%s'"),
                i, *PhysAsset->SkeletalBodySetups[i]->BoneName.ToString());
        }
    }

    // Enable physics simulation on all bodies
    // This auto-detaches MeshComp from CapsuleComp
    MeshComp->SetSimulatePhysics(true);
    MeshComp->WakeAllRigidBodies();

    // Set physical animation to drive bodies toward animated pose (reference pose)
    if (PhysAnimComp)
    {
        PhysAnimComp->SetSkeletalMeshComponent(MeshComp);

        // CRITICAL: bIsLocalSimulation = true
        // This makes drives target LOCAL transforms (relative to parent body).
        // With false (previous), drives targeted WORLD-space transforms, causing
        // all bodies to be yanked back to their spawn positions when the root moves.
        FPhysicalAnimationData AnimData;
        AnimData.bIsLocalSimulation = true;
        AnimData.OrientationStrength = 300.0f;
        AnimData.AngularVelocityStrength = 30.0f;
        AnimData.PositionStrength = 200.0f;
        AnimData.VelocityStrength = 20.0f;
        AnimData.MaxAngularForce = 5000.0f;
        AnimData.MaxLinearForce = 3000.0f;

        // Apply to all bodies — pelvis PhysicalAnimation is intentionally left active.
        // The root drive (spring 4000) and ground spring (spring 8000) are much stronger
        // than the PhysicalAnimation position strength (200), so they win on the pelvis.
        // The PhysicalAnimation orientation strength (300) on pelvis actually helps
        // keep the body upright, supplementing the ground spring.
        PhysAnimComp->ApplyPhysicalAnimationSettingsBelow(NAME_None, AnimData, true);

        UE_LOG(LogSoccerSim, Log, TEXT("SoccerPhysicsPawn: PhysicalAnimation configured (LocalSpace, OrientStr=300, PosStr=200)"));
    }

    // NOTE: Do NOT call MuscleComp->ApplyStandingPose() here.
    // It calls ResetAllBodiesSimulatePhysics() which destroys the drives we just set up.
    // The bodies already start at the reference pose (default for no AnimBP).

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerPhysicsPawn: Physics initialized on %s"), *GetName());
}

void ASoccerPhysicsPawn::SetMovementInput_Implementation(FVector2D Input)
{
    CurrentMovementInput = Input;
}

void ASoccerPhysicsPawn::SetSprinting_Implementation(bool bSprint)
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
