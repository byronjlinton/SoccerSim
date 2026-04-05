#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoccerBroadcastCamera.generated.h"

class UCameraComponent;
class ASoccerBall;

UCLASS()
class SOCCERSIM_API ASoccerBroadcastCamera : public AActor
{
    GENERATED_BODY()

public:
    ASoccerBroadcastCamera();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> CameraComp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Position")
    float SidelineOffset = -6000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Position")
    float CameraHeight = 2200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Position", meta = (ClampMin = "15", ClampMax = "90"))
    float BroadcastFOV = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Position")
    bool bFixedAngle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Position")
    float LookAheadDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
    float MinFollowSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
    float MaxFollowSpeed = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
    float RotationInterpSpeed = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing")
    float SpeedThresholdForMaxFollow = 3000.0f;

protected:
    UPROPERTY(Transient)
    ASoccerBall* TrackedBall;

    FVector CurrentCameraPos;
    FRotator CurrentCameraRot;

    void FindBall();
};
