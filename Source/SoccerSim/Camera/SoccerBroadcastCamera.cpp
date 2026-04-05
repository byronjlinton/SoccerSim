#include "SoccerBroadcastCamera.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformFile.h"

#define SOCCERSIM_AGENT_DEBUG 0  // Set to 1 to write debug-67637b.log for diagnostics

ASoccerBroadcastCamera::ASoccerBroadcastCamera()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComp->SetupAttachment(RootComponent);
    CameraComp->SetFieldOfView(BroadcastFOV);
}

void ASoccerBroadcastCamera::BeginPlay()
{
    Super::BeginPlay();

    if (CameraComp)
    {
        CameraComp->SetFieldOfView(BroadcastFOV);
    }
    FindBall();

    CurrentCameraPos = FVector(0.0f, SidelineOffset, CameraHeight);
    // Look at field center on the game plane (Z=GamePlaneZ), not world origin, so we don't aim below the pitch
    FVector LookAtFieldCenter(0.0f, 0.0f, SoccerField::GamePlaneZ);
    CurrentCameraRot = (LookAtFieldCenter - CurrentCameraPos).Rotation();
    SetActorLocationAndRotation(CurrentCameraPos, CurrentCameraRot);

#if SOCCERSIM_AGENT_DEBUG
    const FString DebugLogPath = FPaths::Combine(FPaths::GetPath(FPaths::ProjectDir()), TEXT("debug-67637b.log"));
    const int64 Timestamp = (int64)(FPlatformTime::Seconds() * 1000.0);
    const FString Line = FString::Printf(TEXT("{\"sessionId\":\"67637b\",\"location\":\"BroadcastCamera:BeginPlay\",\"message\":\"Initial camera state\",\"data\":{\"hasTrackedBall\":%s,\"posY\":%.0f,\"posZ\":%.0f,\"pitch\":%.1f,\"yaw\":%.1f},\"timestamp\":%lld,\"hypothesisId\":\"H2\"}\n"),
        TrackedBall ? TEXT("true") : TEXT("false"), CurrentCameraPos.Y, CurrentCameraPos.Z, CurrentCameraRot.Pitch, CurrentCameraRot.Yaw, Timestamp);
    FFileHelper::SaveStringToFile(Line, *DebugLogPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
#endif

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->SetViewTarget(this);
    }
}

void ASoccerBroadcastCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!TrackedBall) { FindBall(); return; }

    FVector BallPos = TrackedBall->GetActorLocation();
    FVector BallVel = TrackedBall->GetBallVelocity();
    float BallSpeed = BallVel.Size();

    if (!bFixedAngle)
    {
        FVector DesiredPos;
        DesiredPos.X = BallPos.X;
        DesiredPos.Y = SidelineOffset;
        DesiredPos.Z = CameraHeight;

        float FollowSpeed = FMath::GetMappedRangeValueClamped(
            FVector2D(0.0f, SpeedThresholdForMaxFollow),
            FVector2D(MinFollowSpeed, MaxFollowSpeed),
            BallSpeed);

        CurrentCameraPos = FMath::VInterpTo(CurrentCameraPos, DesiredPos, DeltaTime, FollowSpeed);
    }

    FVector LookTarget = BallPos;
    if (BallSpeed > 50.0f)
    {
        FVector BallDir = BallVel.GetSafeNormal();
        LookTarget += BallDir * LookAheadDistance;
    }

    FRotator DesiredRot = (LookTarget - CurrentCameraPos).Rotation();
    CurrentCameraRot = FMath::RInterpTo(CurrentCameraRot, DesiredRot, DeltaTime, RotationInterpSpeed);

    SetActorLocationAndRotation(CurrentCameraPos, CurrentCameraRot);
}

void ASoccerBroadcastCamera::FindBall()
{
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        TrackedBall = GM->MatchBall;
    }
}
