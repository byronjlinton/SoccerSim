#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerField.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UDynamicMeshComponent;

UCLASS()
class SOCCERSIM_API ASoccerField : public AActor
{
    GENERATED_BODY()

public:
    ASoccerField();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
    TObjectPtr<UStaticMeshComponent> PitchMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field|Markings")
    TObjectPtr<UDynamicMeshComponent> FieldMarkings;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field|Boundaries")
    TObjectPtr<UBoxComponent> LeftTouchline;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field|Boundaries")
    TObjectPtr<UBoxComponent> RightTouchline;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field|Boundaries")
    TObjectPtr<UBoxComponent> HomeGoalLine;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field|Boundaries")
    TObjectPtr<UBoxComponent> AwayGoalLine;

    /** Optional pitch material (e.g. Megascans grass). If set and valid, applied to PitchMesh; else green dynamic material. */
    UPROPERTY(EditDefaultsOnly, Category = "Field|Appearance", meta = (AllowedClasses = "/Script/Engine.Material,/Script/Engine.MaterialInstance"))
    FSoftObjectPath PitchMaterialPath;

    UPROPERTY(EditDefaultsOnly, Category = "Field|Physics")
    float GrassFriction = 0.55f;

    UPROPERTY(EditDefaultsOnly, Category = "Field|Physics")
    float GrassRestitution = 0.35f;

    UFUNCTION()
    void OnTouchlineOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                            bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnGoalLineOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                           bool bFromSweep, const FHitResult& SweepResult);

private:
    void CreateBoundaryTrigger(TObjectPtr<UBoxComponent>& OutComp, FName Name, FVector Location, FVector Extent);
    void GenerateFieldMarkings();
    void AddLineQuad(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                     FVector Start, FVector End, float Width, float Z);
    void AddArcStrip(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                     FVector Center, float Radius, float Width, float Z,
                     float StartAngleDeg, float EndAngleDeg, int32 Segments = 32);
    void AddFilledCircle(TArray<FVector>& Vertices, TArray<int32>& Triangles,
                         FVector Center, float Radius, float Z, int32 Segments = 16);
};
