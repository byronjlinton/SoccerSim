#include "SoccerBall.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/ConfigCacheIni.h"
#include "SoccerSim/SoccerSim.h"
#include "UObject/ConstructorHelpers.h"

ASoccerBall::ASoccerBall()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(BallPhysics::Radius);
    CollisionSphere->SetCollisionProfileName(TEXT("Ball"));
    CollisionSphere->SetSimulatePhysics(true);
    CollisionSphere->SetEnableGravity(true);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionSphere->SetLinearDamping(BallPhysics::LinearDamping);
    CollisionSphere->SetAngularDamping(BallPhysics::AngularDamping);
    CollisionSphere->SetNotifyRigidBodyCollision(true);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->BodyInstance.bUseCCD = true;
    CollisionSphere->SetMassOverrideInKg(NAME_None, BallPhysics::Mass, true);
    SetRootComponent(CollisionSphere);

    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
    BallMesh->SetupAttachment(CollisionSphere);
    BallMesh->SetSimulatePhysics(false);
    BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BallMesh->SetRelativeScale3D(FVector(BallPhysics::Radius / 50.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        BallMesh->SetStaticMesh(SphereMesh.Object);
    }
}

void ASoccerBall::BeginPlay()
{
    Super::BeginPlay();

    // Optional default material path from config (DefaultGame.ini [/Script/SoccerSim.SoccerBall] DefaultFootballMaterialPath="...")
    if (!FootballMaterialPath.IsValid() && GConfig && GGameIni.Len() > 0)
    {
        FString ConfigPath;
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerBall"), TEXT("DefaultFootballMaterialPath"), ConfigPath, GGameIni) && !ConfigPath.IsEmpty())
        {
            FootballMaterialPath.SetPath(ConfigPath);
        }
    }

    // Football look: use asset from FootballMaterialPath if valid, else white dynamic material
    if (BallMesh)
    {
        UMaterialInterface* BallMat = nullptr;
        if (FootballMaterialPath.IsValid())
        {
            BallMat = Cast<UMaterialInterface>(FootballMaterialPath.TryLoad());
        }
        if (!BallMat)
        {
            UMaterialInterface* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/M_DynamicColor.M_DynamicColor"));
            if (!BaseMat)
                BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial"));
            if (BaseMat)
            {
                UMaterialInstanceDynamic* BallMID = UMaterialInstanceDynamic::Create(BaseMat, this);
                if (BallMID)
                {
                    BallMID->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.98f, 0.98f, 0.98f));
                    BallMat = BallMID;
                }
            }
        }
        if (BallMat)
        {
            BallMesh->SetMaterial(0, BallMat);
        }
    }

    UPhysicalMaterial* PhysMat = NewObject<UPhysicalMaterial>(this);
    PhysMat->Friction = BallFriction;
    PhysMat->Restitution = BallRestitution;
    PhysMat->RestitutionCombineMode = EFrictionCombineMode::Average;
    PhysMat->FrictionCombineMode = EFrictionCombineMode::Average;
    CollisionSphere->SetPhysMaterialOverride(PhysMat);

    CollisionSphere->SetMassOverrideInKg(NAME_None, Mass, true);
    CollisionSphere->SetLinearDamping(BallLinearDamping);
    CollisionSphere->SetAngularDamping(BallAngularDamping);

    CollisionSphere->OnComponentHit.AddDynamic(this, &ASoccerBall::OnBallHit);
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASoccerBall::OnBallOverlapBegin);

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerBall initialized. Mass: %.3f kg, Radius: %.1f cm, CCD: %s"),
        Mass, Radius, CollisionSphere->BodyInstance.bUseCCD ? TEXT("ON") : TEXT("OFF"));
}

void ASoccerBall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CheckGroundContact();
    ApplyMagnusForce(DeltaTime);
    ApplyAirDrag(DeltaTime);

    if (bIsOnGround)
    {
        ApplyRollingResistance(DeltaTime);
    }

    // Visual rotation: sync mesh rotation with physics angular velocity for visible spin
    if (BallMesh && CollisionSphere)
    {
        FVector AngVel = CollisionSphere->GetPhysicsAngularVelocityInRadians();
        if (!AngVel.IsNearlyZero())
        {
            FQuat DeltaRotation = FQuat(AngVel.GetSafeNormal(), AngVel.Size() * DeltaTime);
            BallMesh->AddRelativeRotation(DeltaRotation);
        }
    }
}

void ASoccerBall::ApplyKick(FVector Impulse, FVector SpinAxis)
{
    CollisionSphere->AddImpulse(Impulse, NAME_None, false);

    if (!SpinAxis.IsNearlyZero())
    {
        CollisionSphere->SetPhysicsAngularVelocityInRadians(SpinAxis, true);
    }
}

void ASoccerBall::ResetBall(FVector NewLocation)
{
    CollisionSphere->SetSimulatePhysics(false);
    SetActorLocation(NewLocation.IsNearlyZero() ?
        FVector(0.0f, 0.0f, Radius + 1.0f) : NewLocation);
    CollisionSphere->SetSimulatePhysics(true);
    CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
    CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

    LastTouchTeam = ETeamId::None;
    LastTouchPlayerIndex = -1;
}

FVector ASoccerBall::GetBallVelocity() const
{
    return CollisionSphere->GetPhysicsLinearVelocity();
}

float ASoccerBall::GetBallSpeed() const
{
    return CollisionSphere->GetPhysicsLinearVelocity().Size();
}

void ASoccerBall::CheckGroundContact()
{
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0.0f, 0.0f, Radius + GroundCheckDistance);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bIsOnGround = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
}

void ASoccerBall::ApplyMagnusForce(float DeltaTime)
{
    FVector Velocity = GetBallVelocity();
    FVector AngVelocity = CollisionSphere->GetPhysicsAngularVelocityInRadians();

    if (Velocity.IsNearlyZero() || AngVelocity.IsNearlyZero()) return;

    FVector MagnusForce = FVector::CrossProduct(AngVelocity, Velocity) * MagnusCoefficient;
    CollisionSphere->AddForce(MagnusForce, NAME_None, false);
}

void ASoccerBall::ApplyAirDrag(float DeltaTime)
{
    FVector Velocity = GetBallVelocity();
    float Speed = Velocity.Size();
    if (Speed < 1.0f) return;

    FVector DragForce = -Velocity.GetSafeNormal() * 0.5f * DragCoefficient * CrossSectionArea * AirDensity * Speed * Speed;
    CollisionSphere->AddForce(DragForce, NAME_None, false);
}

void ASoccerBall::ApplyRollingResistance(float DeltaTime)
{
    FVector Velocity = GetBallVelocity();
    FVector HorizontalVel = FVector(Velocity.X, Velocity.Y, 0.0f);
    float HorizontalSpeed = HorizontalVel.Size();

    if (HorizontalSpeed < 5.0f)
    {
        if (HorizontalSpeed > 0.1f)
        {
            CollisionSphere->SetPhysicsLinearVelocity(FVector(0.0f, 0.0f, Velocity.Z));
        }
        return;
    }

    float GravityMag = FMath::Abs(GetWorld()->GetGravityZ());
    FVector RollingForce = -HorizontalVel.GetSafeNormal() * RollingFrictionCoeff * Mass * GravityMag;
    CollisionSphere->AddForce(RollingForce, NAME_None, false);
}

void ASoccerBall::OnBallHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                            const FHitResult& Hit)
{
    float ImpactForce = NormalImpulse.Size();
    if (ImpactForce > 10.0f)
    {
        UE_LOG(LogSoccerSim, Verbose, TEXT("Ball hit %s with force %.1f"),
            *OtherActor->GetName(), ImpactForce);
    }
}

void ASoccerBall::OnBallOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                      bool bFromSweep, const FHitResult& SweepResult)
{
    UE_LOG(LogSoccerSim, Verbose, TEXT("Ball overlapped: %s"), *OtherActor->GetName());
}
