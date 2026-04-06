#include "PhysicsLocomotionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "SoccerSim/SoccerSim.h"
#include "SoccerPhysicsPawn.h"

UPhysicsLocomotionComponent::UPhysicsLocomotionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

void UPhysicsLocomotionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache skeletal mesh reference
    ASoccerPhysicsPawn* Owner = Cast<ASoccerPhysicsPawn>(GetOwner());
    if (Owner && Owner->MeshComp)
    {
        MeshComp = Owner->MeshComp;

        // Verify pelvis bone exists in physics asset
        if (UPhysicsAsset* PhysAsset = MeshComp->GetPhysicsAsset())
        {
            bool bFound = false;
            for (int32 i = 0; i < PhysAsset->SkeletalBodySetups.Num(); ++i)
            {
                if (PhysAsset->SkeletalBodySetups[i] &&
                    PhysAsset->SkeletalBodySetups[i]->BoneName == PelvisBoneName)
                {
                    PelvisBodyIndex = i;
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                UE_LOG(LogSoccerSim, Warning, TEXT("PhysicsLocomotion: Pelvis bone '%s' not found in physics asset. Root drive will not work."), *PelvisBoneName.ToString());
            }
        }
    }
}

void UPhysicsLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!MeshComp || PelvisBodyIndex == INDEX_NONE) return;

    ApplyRootDrive(DeltaTime);
    ApplyGroundSpring(DeltaTime);
    ApplyRotationDrive(DeltaTime);
}

void UPhysicsLocomotionComponent::ApplyRootDrive(float DeltaTime)
{
    // Get desired velocity from input
    float TargetSpeed = bDesiredSprint ? SprintSpeed : JogSpeed;
    FVector DesiredVelocity(DesiredInput.X * TargetSpeed, DesiredInput.Y * TargetSpeed, 0.0f);

    if (DesiredInput.IsNearlyZero())
    {
        DesiredVelocity = FVector::ZeroVector;
    }

    // Get current pelvis velocity (horizontal only)
    FVector CurrentVelocity = MeshComp->GetBoneLinearVelocity(PelvisBoneName);
    CurrentVelocity.Z = 0.0f;

    // Spring-damper toward desired velocity
    FVector VelocityError = DesiredVelocity - CurrentVelocity;
    FVector DriveForce = DriveSpring * VelocityError - DriveDamping * CurrentVelocity;

    // Apply force to pelvis and all bodies below it (which is just the pelvis itself)
    MeshComp->AddForceToAllBodiesBelow(DriveForce, PelvisBoneName, false, true);
}

void UPhysicsLocomotionComponent::ApplyGroundSpring(float DeltaTime)
{
    FVector PelvisPos = MeshComp->GetSocketLocation(PelvisBoneName);
    float HeightError = StandingPelvisHeight - PelvisPos.Z;

    FVector PelvisVel = MeshComp->GetBoneLinearVelocity(PelvisBoneName);
    float UpForce = HeightError * GroundSpringK - PelvisVel.Z * GroundDampingK;

    MeshComp->AddForceToAllBodiesBelow(FVector(0.0f, 0.0f, UpForce), PelvisBoneName, false, true);
}

void UPhysicsLocomotionComponent::ApplyRotationDrive(float DeltaTime)
{
    if (DesiredInput.IsNearlyZero()) return;

    // Target facing = movement direction
    FVector MoveDir(DesiredInput.X, DesiredInput.Y, 0.0f);
    float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(MoveDir.Y, MoveDir.X));

    // Current pelvis yaw
    FQuat PelvisQuat = MeshComp->GetSocketQuaternion(PelvisBoneName);
    FRotator PelvisRot(PelvisQuat);
    float CurrentYaw = PelvisRot.Yaw;

    // Angular difference (shortest path)
    float AngleDiff = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);

    // Get angular velocity from FBodyInstance via ForEachBodyBelow
    FVector AngularVelRad = FVector::ZeroVector;
    MeshComp->ForEachBodyBelow(PelvisBoneName, true, false, [&](FBodyInstance* BI)
    {
        AngularVelRad = BI->GetUnrealWorldAngularVelocityInRadians();
    });

    // Convert angular velocity to degrees for damping calculation
    float AngularVelYawDeg = FMath::RadiansToDegrees(AngularVelRad.Z);
    float TorqueZDeg = TurnSpringK * AngleDiff - TurnDampingK * AngularVelYawDeg;

    // Apply torque in radians
    float TorqueZRad = FMath::DegreesToRadians(TorqueZDeg);
    MeshComp->ForEachBodyBelow(PelvisBoneName, true, false, [&](FBodyInstance* BI)
    {
        BI->AddTorqueInRadians(FVector(0.0f, 0.0f, TorqueZRad), false, false);
    });
}
