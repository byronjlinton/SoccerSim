#include "PhysicsMuscleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "SoccerSim/SoccerSim.h"
#include "SoccerPhysicsPawn.h"

UPhysicsMuscleComponent::UPhysicsMuscleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

void UPhysicsMuscleComponent::BeginPlay()
{
    Super::BeginPlay();

    ASoccerPhysicsPawn* Owner = Cast<ASoccerPhysicsPawn>(GetOwner());
    if (Owner && Owner->MeshComp)
    {
        MeshComp = Owner->MeshComp;
        SetupPhysicalAnimationDrives();
    }
}

void UPhysicsMuscleComponent::SetupPhysicalAnimationDrives()
{
    // Physical animation drives are set up through the PhysicalAnimationComponent
    // on the owning pawn. This is called once to configure initial drives.
    ASoccerPhysicsPawn* Owner = Cast<ASoccerPhysicsPawn>(GetOwner());
    if (!Owner || !Owner->PhysAnimComp || !MeshComp) return;

    UPhysicalAnimationComponent* PhysAnim = Owner->PhysAnimComp;
    PhysAnim->SetSkeletalMeshComponent(MeshComp);

    // CRITICAL: bIsLocalSimulation = true
    // Previous value (false) drove bodies toward world-space reference positions,
    // causing the entire skeleton to be yanked back to origin when the root moved.
    // With true, drives target local transforms relative to parent body — the
    // skeleton maintains its shape while the pelvis moves freely.
    FPhysicalAnimationData DefaultDrive;
    DefaultDrive.bIsLocalSimulation = true;
    DefaultDrive.OrientationStrength = 300.0f;
    DefaultDrive.AngularVelocityStrength = 30.0f;
    DefaultDrive.PositionStrength = 200.0f;
    DefaultDrive.VelocityStrength = 20.0f;
    DefaultDrive.MaxAngularForce = SpineMaxForce;
    DefaultDrive.MaxLinearForce = 3000.0f; // Was 0 — position drive was doing nothing

    PhysAnim->ApplyPhysicalAnimationSettingsBelow(NAME_None, DefaultDrive, true);
}

void UPhysicsMuscleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Phase 1: no dynamic strength changes — standing pose is static
    // Future phases will blend motor strengths based on locomotion state
}

void UPhysicsMuscleComponent::ApplyStandingPose()
{
    // Phase 1: Ensure bodies are awake and simulating.
    // Do NOT call ResetAllBodiesSimulatePhysics() — it resets all bodies
    // to reference pose AND disables physics simulation, which destroys the
    // PhysicalAnimation drives that were just configured. The bodies already
    // start at the reference pose (default for skeletal mesh with no AnimBP).
    if (!MeshComp) return;

    MeshComp->WakeAllRigidBodies();

    UE_LOG(LogSoccerSim, Log, TEXT("PhysicsMuscleComponent: Standing pose applied (bodies woken)"));
}
