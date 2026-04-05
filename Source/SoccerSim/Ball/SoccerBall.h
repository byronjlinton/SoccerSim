#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerBall.generated.h"

class USphereComponent;

UCLASS()
class SOCCERSIM_API ASoccerBall : public AActor
{
    GENERATED_BODY()

public:
    ASoccerBall();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void ApplyKick(FVector Impulse, FVector SpinAxis = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "Ball")
    void ResetBall(FVector NewLocation);

    UFUNCTION(BlueprintPure, Category = "Ball")
    FVector GetBallVelocity() const;

    UFUNCTION(BlueprintPure, Category = "Ball")
    float GetBallSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Ball")
    bool IsBallOnGround() const { return bIsOnGround; }

    UFUNCTION(BlueprintPure, Category = "Ball")
    UStaticMeshComponent* GetBallMesh() const { return BallMesh; }

    UPROPERTY(BlueprintReadOnly, Category = "Ball")
    ETeamId LastTouchTeam = ETeamId::None;

    UPROPERTY(BlueprintReadOnly, Category = "Ball")
    int32 LastTouchPlayerIndex = -1;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BallMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|Physics")
    float Mass = BallPhysics::Mass;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|Physics")
    float Radius = BallPhysics::Radius;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|Physics")
    float BallLinearDamping = BallPhysics::LinearDamping;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|Physics")
    float BallAngularDamping = BallPhysics::AngularDamping;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|Physics")
    float BallRestitution = BallPhysics::Restitution_Grass;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|Physics")
    float BallFriction = BallPhysics::Friction_Grass;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|AdvancedPhysics")
    float MagnusCoefficient = BallPhysics::MagnusCoefficient;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|AdvancedPhysics")
    float DragCoefficient = BallPhysics::DragCoefficient;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|AdvancedPhysics")
    float AirDensity = BallPhysics::AirDensity;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|AdvancedPhysics")
    float CrossSectionArea = BallPhysics::CrossSectionArea;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|AdvancedPhysics")
    float RollingFrictionCoeff = BallPhysics::RollingFrictionCoeff;

    UPROPERTY(EditDefaultsOnly, Category = "Ball|AdvancedPhysics")
    float GroundCheckDistance = BallPhysics::GroundCheckDistance;

    /** Optional football material (white + black pattern). e.g. /Game/Art/Ball/M_Football. If empty or invalid, a white dynamic material is used. */
    UPROPERTY(EditDefaultsOnly, Category = "Ball|Appearance", meta = (AllowedClasses = "/Script/Engine.Material,/Script/Engine.MaterialInstance"))
    FSoftObjectPath FootballMaterialPath;

    bool bIsOnGround = false;

    void ApplyMagnusForce(float DeltaTime);
    void ApplyAirDrag(float DeltaTime);
    void ApplyRollingResistance(float DeltaTime);
    void CheckGroundContact();

    UFUNCTION()
    void OnBallHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                   FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void OnBallOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                            bool bFromSweep, const FHitResult& SweepResult);
};
