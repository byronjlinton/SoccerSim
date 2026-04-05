#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerGoal.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class SOCCERSIM_API ASoccerGoal : public AActor
{
    GENERATED_BODY()

public:
    ASoccerGoal();

    void SetTeamId(ETeamId NewTeam) { TeamId = NewTeam; }
    ETeamId GetTeamId() const { return TeamId; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UStaticMeshComponent> LeftPost;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UStaticMeshComponent> RightPost;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UStaticMeshComponent> Crossbar;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UStaticMeshComponent> NetBack;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UStaticMeshComponent> NetLeft;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UStaticMeshComponent> NetRight;

    UPROPERTY(VisibleAnywhere, Category = "Goal")
    TObjectPtr<UBoxComponent> GoalTrigger;

    UPROPERTY(EditDefaultsOnly, Category = "Goal")
    ETeamId TeamId = ETeamId::None;

    UPROPERTY(EditDefaultsOnly, Category = "Goal|Physics")
    float PostRestitution = BallPhysics::Restitution_Post;

    UPROPERTY(EditDefaultsOnly, Category = "Goal|Physics")
    float PostFriction = BallPhysics::Friction_Post;

    UPROPERTY(EditDefaultsOnly, Category = "Goal|Physics")
    float NetRestitution = BallPhysics::Restitution_Net;

    UPROPERTY(EditDefaultsOnly, Category = "Goal|Physics")
    float NetFriction = BallPhysics::Friction_Net;

    /** Optional net material (translucent/grid so net reads as net). If set and valid, applied to net panels; else light grey. */
    UPROPERTY(EditDefaultsOnly, Category = "Goal|Appearance", meta = (AllowedClasses = "/Script/Engine.Material,/Script/Engine.MaterialInstance"))
    FSoftObjectPath NetMaterialPath;

    UFUNCTION()
    void OnGoalTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                              bool bFromSweep, const FHitResult& SweepResult);

private:
    UStaticMeshComponent* CreatePostMesh(FName Name, FVector Location, FVector Scale);
    UStaticMeshComponent* CreateNetMesh(FName Name, FVector Location, FVector Scale);
};
