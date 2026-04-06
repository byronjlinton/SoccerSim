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

    // Default drive settings for all bodies — strong orientation + position drives
    FPhysicalAnimationData DefaultDrive;
    DefaultDrive.bIsLocalSimulation = false;
    DefaultDrive.OrientationStrength = 500.0f;
    DefaultDrive.AngularVelocityStrength = 500.0f;
    DefaultDrive.PositionStrength = 500.0f;
    DefaultDrive.VelocityStrength = 500.0f;
    DefaultDrive.MaxAngularForce = SpineMaxForce;
    DefaultDrive.MaxLinearForce = 0.0f; // linear force not needed for joint motors

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
    // Phase 1: Set all bodies to their reference pose (zero offset from bind pose).
    // The PhysicalAnimationComponent drives bodies toward the reference pose.
    // Setting orientation strength makes bodies hold their bind-pose rotation.
    if (!MeshComp) return;

    // Reset all bodies to reference pose transform
    MeshComp->ResetAllBodiesSimulatePhysics();

    // Enable physics on all bodies again (ResetAllBodiesSimulatePhysics disables sim)
    MeshComp->SetAllBodiesSimulatePhysics(true);
    MeshComp->WakeAllRigidBodies();

    UE_LOG(LogSoccerSim, Log, TEXT("PhysicsMuscleComponent: Standing pose applied (reference pose)"));
}
