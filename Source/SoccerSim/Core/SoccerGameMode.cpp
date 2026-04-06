#include "SoccerGameMode.h"
#include "SoccerGameState.h"
#include "SoccerPlayerController.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Field/SoccerField.h"
#include "SoccerSim/Field/SoccerGoal.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "SoccerSim/Camera/SoccerBroadcastCamera.h"
#include "SoccerSim/AI/SoccerAIController.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformFile.h"

#define SOCCERSIM_AGENT_DEBUG 0  // Set to 1 to write debug-67637b.log for diagnostics

#if SOCCERSIM_AGENT_DEBUG
static void AgentDebugLog(const FString& Location, const FString& Message, const FString& DataJson, const FString& HypothesisId)
{
    const FString DebugLogPath = FPaths::Combine(FPaths::GetPath(FPaths::ProjectDir()), TEXT("debug-67637b.log"));
    const int64 Timestamp = (int64)(FPlatformTime::Seconds() * 1000.0);
    const FString Line = FString::Printf(TEXT("{\"sessionId\":\"67637b\",\"location\":\"%s\",\"message\":\"%s\",\"data\":%s,\"timestamp\":%lld,\"hypothesisId\":\"%s\"}\n"),
        *Location, *Message.Replace(TEXT("\""), TEXT("\\\"")), *DataJson, Timestamp, *HypothesisId);
    FFileHelper::SaveStringToFile(Line, *DebugLogPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}
#else
static void AgentDebugLog(const FString&, const FString&, const FString&, const FString&) {}
#endif

ASoccerGameMode::ASoccerGameMode()
{
    GameStateClass = ASoccerGameState::StaticClass();
    PlayerControllerClass = ASoccerPlayerController::StaticClass();
    DefaultPawnClass = nullptr;

    BallClass = ASoccerBall::StaticClass();
    PlayerPawnClass = ASoccerPlayerPawn::StaticClass();
    FieldClass = ASoccerField::StaticClass();
    GoalClass = ASoccerGoal::StaticClass();
    BroadcastCameraClass = ASoccerBroadcastCamera::StaticClass();
}

void ASoccerGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    UE_LOG(LogSoccerSim, Log, TEXT("SoccerGameMode::InitGame - Map: %s"), *MapName);
}

void ASoccerGameMode::StartPlay()
{
    // #region agent log
    UWorld* W = GetWorld();
    AgentDebugLog(TEXT("SoccerGameMode.cpp:StartPlay"), TEXT("StartPlay entered"),
        W ? FString(TEXT("{\"hasWorld\":true}")) : FString(TEXT("{\"hasWorld\":false}")),
        TEXT("H1"));
    // #endregion
    Super::StartPlay();

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerGameMode::StartPlay - Spawning match elements"));

    SpawnField();
    // #region agent log
    AgentDebugLog(TEXT("SoccerGameMode.cpp:AfterSpawnField"), TEXT("SpawnField done"),
        FString::Printf(TEXT("{\"MatchField\":%s,\"FieldClass\":%s}"), MatchField ? TEXT("true") : TEXT("false"), FieldClass ? TEXT("true") : TEXT("false")),
        TEXT("H2"));
    // #endregion
    SpawnStadiumLighting();
    SpawnStadiumGeometry();
    SpawnBall();
    // #region agent log
    AgentDebugLog(TEXT("SoccerGameMode.cpp:AfterSpawnBall"), TEXT("SpawnBall done"),
        FString::Printf(TEXT("{\"MatchBall\":%s,\"BallClass\":%s}"), MatchBall ? TEXT("true") : TEXT("false"), BallClass ? TEXT("true") : TEXT("false")),
        TEXT("H3"));
    // #endregion
    SpawnTeams();
    // #region agent log
    AgentDebugLog(TEXT("SoccerGameMode.cpp:AfterSpawnTeams"), TEXT("SpawnTeams done"),
        FString::Printf(TEXT("{\"HomeCount\":%d,\"AwayCount\":%d}"), HomePlayers.Num(), AwayPlayers.Num()),
        TEXT("H4"));
    // #endregion

    if (BroadcastCameraClass)
    {
        FActorSpawnParameters CamParams;
        CamParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        BroadcastCamera = GetWorld()->SpawnActor<ASoccerBroadcastCamera>(
            BroadcastCameraClass, FVector(0.0f, -6000.0f, 2200.0f), FRotator::ZeroRotator, CamParams);
        if (BroadcastCamera)
        {
            APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
            if (PC) PC->SetViewTarget(BroadcastCamera);
        }
    }

    // Broadcast-style post-process (unbounded)
    APostProcessVolume* PPVol = GetWorld()->SpawnActor<APostProcessVolume>(FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
    if (PPVol)
    {
        PPVol->bUnbound = true;
        PPVol->Settings.bOverride_AutoExposureBias = true;
        PPVol->Settings.AutoExposureBias = 0.3f;
        PPVol->Settings.bOverride_ColorSaturation = true;
        PPVol->Settings.ColorSaturation = FVector4(1.05f, 1.05f, 1.05f, 1.0f);
        PPVol->Settings.bOverride_ColorContrast = true;
        PPVol->Settings.ColorContrast = FVector4(1.1f, 1.1f, 1.1f, 1.0f);
        PPVol->Settings.bOverride_BloomIntensity = true;
        PPVol->Settings.BloomIntensity = 0.15f;
        PPVol->Settings.bOverride_VignetteIntensity = true;
        PPVol->Settings.VignetteIntensity = 0.3f;
        PPVol->Settings.bOverride_SceneFringeIntensity = true;
        PPVol->Settings.SceneFringeIntensity = 0.1f;
        PPVol->Settings.bOverride_MotionBlurAmount = true;
        PPVol->Settings.MotionBlurAmount = 0.2f;
    }

    ASoccerPlayerController* PC = Cast<ASoccerPlayerController>(
        UGameplayStatics::GetPlayerController(this, 0));
    if (PC && HomePlayers.Num() > 0)
    {
        ASoccerPlayerPawn* StartPlayer = HomePlayers.Last();
        PC->Possess(StartPlayer);
        UE_LOG(LogSoccerSim, Log, TEXT("Human player possessing: %s"), *StartPlayer->GetName());
        // Keep broadcast camera as view target (possession would otherwise switch view to the pawn)
        if (BroadcastCamera)
        {
            PC->SetViewTarget(BroadcastCamera);
            FTimerHandle ViewTargetTimer;
            GetWorld()->GetTimerManager().SetTimer(ViewTargetTimer, [this, PC]()
            {
                if (PC && BroadcastCamera)
                {
                    PC->SetViewTarget(BroadcastCamera);
                    // #region agent log
                    AActor* VT = PC->GetViewTarget();
                    bool bViewTargetIsBroadcast = (VT == BroadcastCamera);
                    FVector CamLoc = BroadcastCamera->GetActorLocation();
                    FRotator CamRot = BroadcastCamera->GetActorRotation();
                    AgentDebugLog(TEXT("GameMode:ViewTargetTimer"), TEXT("After SetViewTarget"),
                        FString::Printf(TEXT("{\"viewTargetIsBroadcast\":%s,\"camX\":%.0f,\"camY\":%.0f,\"camZ\":%.0f,\"camPitch\":%.1f,\"camYaw\":%.1f,\"camRoll\":%.1f}"),
                            bViewTargetIsBroadcast ? TEXT("true") : TEXT("false"), CamLoc.X, CamLoc.Y, CamLoc.Z, CamRot.Pitch, CamRot.Yaw, CamRot.Roll),
                        TEXT("H1"));
                    // #endregion
                }
            }, 0.1f, false);
        }
    }

    ASoccerGameState* GS = GetGameState<ASoccerGameState>();
    if (GS)
    {
        GS->SetMatchPhase(EMatchPhase::KickOff);
        GS->OnMatchPhaseChanged.AddDynamic(this, &ASoccerGameMode::OnMatchPhaseChanged);
        FTimerHandle KickOffTimer;
        GetWorld()->GetTimerManager().SetTimer(KickOffTimer, [this, GS]()
        {
            GS->SetMatchPhase(EMatchPhase::FirstHalf);
            GS->CurrentBallState = EBallState::InPlay;

            // Kickoff: give ball a small forward kick so play begins
            if (MatchBall)
            {
                FVector KickDir = FVector::ForwardVector;
                float KickPower = 400.0f;
                FVector Impulse = KickDir * KickPower * BallPhysics::Mass;
                MatchBall->ApplyKick(Impulse, FVector::ZeroVector);
            }
        }, 2.0f, false);
    }
}

void ASoccerGameMode::OnMatchPhaseChanged(EMatchPhase NewPhase)
{
    // Play whistle on key phase transitions
    if (NewPhase == EMatchPhase::KickOff || NewPhase == EMatchPhase::SecondHalfKickOff ||
        NewPhase == EMatchPhase::HalfTime || NewPhase == EMatchPhase::FullTime)
    {
        // Load and play whistle sound if configured
        if (WhistleSoundPath.IsValid())
        {
            if (USoundBase* Whistle = Cast<USoundBase>(WhistleSoundPath.TryLoad()))
            {
                UGameplayStatics::PlaySoundAtLocation(this, Whistle, FVector(0.0f, 0.0f, SoccerField::GamePlaneZ + 200.0f));
            }
        }
    }

    if (NewPhase == EMatchPhase::HalfTime)
    {
        FTimerHandle HalftimeTimer;
        GetWorld()->GetTimerManager().SetTimer(HalftimeTimer, this, &ASoccerGameMode::StartSecondHalf, HalftimeDurationSeconds, false);
    }
}

void ASoccerGameMode::StartSecondHalf()
{
    ASoccerGameState* GS = GetGameState<ASoccerGameState>();
    if (!GS) return;
    GS->ResetMatchClock();
    GS->SetMatchPhase(EMatchPhase::SecondHalfKickOff);
    FTimerHandle KickOffTimer;
    GetWorld()->GetTimerManager().SetTimer(KickOffTimer, [this]()
    {
        if (ASoccerGameState* GState = GetGameState<ASoccerGameState>())
        {
            GState->SetMatchPhase(EMatchPhase::SecondHalf);
        }
    }, 2.0f, false);
}

void ASoccerGameMode::HandleGoalScored(ETeamId ScoringTeam)
{
    ASoccerGameState* GS = GetGameState<ASoccerGameState>();
    if (!GS) return;

    if (ScoringTeam == ETeamId::Home)
        GS->HomeScore++;
    else
        GS->AwayScore++;

    GS->OnScoreChanged.Broadcast(GS->HomeScore, GS->AwayScore);
    UE_LOG(LogSoccerSim, Log, TEXT("GOAL! Score: Home %d - %d Away"), GS->HomeScore, GS->AwayScore);

    // Play goal celebration sound if configured
    if (GoalCelebrationSoundPath.IsValid())
    {
        if (USoundBase* GoalSound = Cast<USoundBase>(GoalCelebrationSoundPath.TryLoad()))
        {
            UGameplayStatics::PlaySoundAtLocation(this, GoalSound, FVector(0.0f, 0.0f, SoccerField::GamePlaneZ + 200.0f));
        }
    }

    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASoccerGameMode::ResetToKickOff, 3.0f, false);
}

void ASoccerGameMode::HandleBallOutOfPlay(EBallState OutType, ETeamId LastTouchTeam)
{
    UE_LOG(LogSoccerSim, Log, TEXT("Ball out of play: %d, Last touch: %d"),
        static_cast<int32>(OutType), static_cast<int32>(LastTouchTeam));

    if (!MatchBall) return;

    const float Inset = 80.0f;
    const float Z = SoccerField::GamePlaneZ + BallPhysics::Radius + 5.0f;
    FVector PlaceLoc;

    switch (OutType)
    {
    case EBallState::OutForThrowIn:
        PlaceLoc = FVector(0.0f, SoccerField::HalfWidth - Inset, Z);
        break;
    case EBallState::OutForGoalKick:
        if (LastTouchTeam == ETeamId::Away)
            PlaceLoc = FVector(SoccerField::HalfLength - 200.0f, 0.0f, Z);
        else
            PlaceLoc = FVector(-SoccerField::HalfLength + 200.0f, 0.0f, Z);
        break;
    case EBallState::OutForCorner:
        if (LastTouchTeam == ETeamId::Home)
            PlaceLoc = FVector(SoccerField::HalfLength - Inset, SoccerField::HalfWidth - Inset, Z);
        else
            PlaceLoc = FVector(-SoccerField::HalfLength + Inset, SoccerField::HalfWidth - Inset, Z);
        break;
    default:
        return;
    }

    MatchBall->ResetBall(PlaceLoc);
}

void ASoccerGameMode::ResetToKickOff()
{
    if (MatchBall)
    {
        MatchBall->ResetBall(FVector(0.0f, 0.0f, SoccerField::GamePlaneZ + BallPhysics::Radius + 5.0f));
    }

    ASoccerGameState* GS = GetGameState<ASoccerGameState>();
    if (GS)
    {
        GS->CurrentBallState = EBallState::Dead;
        GS->SetMatchPhase(EMatchPhase::KickOff);
    }

    const float PlayerZ = SoccerField::GamePlaneZ + PlayerMovement::CapsuleHalfHeight + 5.0f;
    const FRotator HomeRot(0.0f, 0.0f, 0.0f);
    const FRotator AwayRot(0.0f, 180.0f, 0.0f);

    for (ASoccerPlayerPawn* P : HomePlayers)
    {
        if (P)
        {
            FVector Pos = SoccerField::NormalizedToWorld(P->GetFormationSlot().NormalizedPosition, ETeamId::Home);
            Pos.Z = PlayerZ;
            P->RepositionToFormation(Pos, HomeRot);
        }
    }
    for (ASoccerPlayerPawn* P : AwayPlayers)
    {
        if (P)
        {
            FVector Pos = SoccerField::NormalizedToWorld(P->GetFormationSlot().NormalizedPosition, ETeamId::Away);
            Pos.Z = PlayerZ;
            P->RepositionToFormation(Pos, AwayRot);
        }
    }

    // After short delay, start play with a kickoff kick
    FTimerHandle KickOffTimer;
    GetWorld()->GetTimerManager().SetTimer(KickOffTimer, [this, GS]()
    {
        if (GS)
        {
            GS->SetMatchPhase(EMatchPhase::FirstHalf);
            GS->CurrentBallState = EBallState::InPlay;
        }
        if (MatchBall)
        {
            FVector KickDir = FVector::ForwardVector;
            float KickPower = 400.0f;
            FVector Impulse = KickDir * KickPower * BallPhysics::Mass;
            MatchBall->ApplyKick(Impulse, FVector::ZeroVector);
        }
    }, 2.0f, false);

    UE_LOG(LogSoccerSim, Log, TEXT("Reset to kick off"));
}

void ASoccerGameMode::SpawnField()
{
    if (FieldClass)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MatchField = GetWorld()->SpawnActor<ASoccerField>(FieldClass, FVector(0.0f, 0.0f, SoccerField::GamePlaneZ), FRotator::ZeroRotator, Params);
    }
    else
    {
        UE_LOG(LogSoccerSim, Warning, TEXT("FieldClass not set in GameMode defaults!"));
    }

    if (GoalClass)
    {
        HomeGoal = GetWorld()->SpawnActor<ASoccerGoal>(GoalClass,
            FVector(-SoccerField::HalfLength, 0.0f, SoccerField::GamePlaneZ), FRotator::ZeroRotator);
        if (HomeGoal) HomeGoal->SetTeamId(ETeamId::Home);

        AwayGoal = GetWorld()->SpawnActor<ASoccerGoal>(GoalClass,
            FVector(SoccerField::HalfLength, 0.0f, SoccerField::GamePlaneZ), FRotator(0.0f, 180.0f, 0.0f));
        if (AwayGoal) AwayGoal->SetTeamId(ETeamId::Away);
    }
}

void ASoccerGameMode::SpawnStadiumLighting()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Sky atmosphere for realistic outdoor lighting
    ASkyAtmosphere* SkyAtmo = World->SpawnActor<ASkyAtmosphere>(FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, Params);
    if (SkyAtmo)
    {
        USkyAtmosphereComponent* AtmoComp = SkyAtmo->GetComponent();
        if (AtmoComp)
        {
            AtmoComp->SetAtmosphereHeight(60.0f);
        }
    }

    // Broadcast-style key light: sun from above and slightly behind camera side
    ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 0.0f), FRotator(-40.0f, 30.0f, 0.0f), Params);
    if (Sun)
    {
        UDirectionalLightComponent* SunComp = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
        if (SunComp)
        {
            SunComp->SetIntensity(3.0f);
            SunComp->SetLightColor(FLinearColor(1.0f, 0.98f, 0.94f));
            SunComp->SetForwardShadingPriority(1);
            SunComp->SetAtmosphereSunLight(true);
            SunComp->SetAtmosphereSunLightIndex(0);
        }
    }

    ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, Params);
    if (Sky)
    {
        USkyLightComponent* SkyComp = Sky->GetLightComponent();
        if (SkyComp)
        {
            SkyComp->SetIntensity(1.0f);
            SkyComp->SetLightColor(FLinearColor(0.6f, 0.7f, 1.0f));
            SkyComp->RecaptureSky();
        }
    }

    World->SpawnActor<AExponentialHeightFog>(FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, Params);
}

void ASoccerGameMode::SpawnStadiumGeometry()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Load materials for stadium
    UMaterialInterface* ConcreteMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/M_DynamicColor.M_DynamicColor"));
    auto CreateStandMesh = [&](FVector Location, FVector Scale, FRotator Rotation)
    {
        AStaticMeshActor* Stand = World->SpawnActor<AStaticMeshActor>(Location, Rotation);
        if (Stand)
        {
            Stand->SetMobility(EComponentMobility::Movable);
            UStaticMeshComponent* MeshComp = Stand->GetStaticMeshComponent();
            if (MeshComp)
            {
                UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
                if (CubeMesh)
                {
                    MeshComp->SetStaticMesh(CubeMesh);
                }
                MeshComp->SetMobility(EComponentMobility::Movable);
                MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
                MeshComp->SetGenerateOverlapEvents(false);
                Stand->SetActorScale3D(Scale);

                if (ConcreteMat)
                {
                    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(ConcreteMat, MeshComp);
                    if (DynMat)
                    {
                        DynMat->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.25f, 0.25f, 0.28f));
                        MeshComp->SetMaterial(0, DynMat);
                    }
                }
            }
        }
        return Stand;
    };

    // Stand dimensions (in scaled units): each is a box
    // Side stands: 120m long x 15m deep x 10m tall, offset 15m from touchline
    const float StandHeight = 1200.0f;  // 12m (scaled by actor scale)
    const float SideStandLength = SoccerField::PitchLength + 1500.0f; // slightly longer than pitch
    const float StandDepth = 1500.0f;   // 15m deep
    const float SideStandOffset = SoccerField::HalfWidth + StandDepth * 0.5f + 500.0f;

    // Two sideline stands (Y offset)
    CreateStandMesh(
        FVector(0.0f, -SideStandOffset, SoccerField::GamePlaneZ + StandHeight * 0.4f),
        FVector(SideStandLength / 100.0f, StandDepth / 100.0f, StandHeight / 100.0f),
        FRotator(0.0f, 0.0f, 0.0f));
    CreateStandMesh(
        FVector(0.0f, SideStandOffset, SoccerField::GamePlaneZ + StandHeight * 0.4f),
        FVector(SideStandLength / 100.0f, StandDepth / 100.0f, StandHeight / 100.0f),
        FRotator(0.0f, 0.0f, 0.0f));

    // Two end stands (X offset)
    const float EndStandLength = SoccerField::PitchWidth + 1500.0f;
    const float EndStandOffset = SoccerField::HalfLength + StandDepth * 0.5f + 500.0f;

    CreateStandMesh(
        FVector(-EndStandOffset, 0.0f, SoccerField::GamePlaneZ + StandHeight * 0.4f),
        FVector(StandDepth / 100.0f, EndStandLength / 100.0f, StandHeight / 100.0f),
        FRotator(0.0f, 0.0f, 0.0f));
    CreateStandMesh(
        FVector(EndStandOffset, 0.0f, SoccerField::GamePlaneZ + StandHeight * 0.4f),
        FVector(StandDepth / 100.0f, EndStandLength / 100.0f, StandHeight / 100.0f),
        FRotator(0.0f, 0.0f, 0.0f));

    // Floodlight towers — 4 corner posts
    const float CornerX = SoccerField::HalfLength + 500.0f;
    const float CornerY = SoccerField::HalfWidth + 500.0f;
    const float TowerHeight = 3000.0f; // 30m

    auto CreateFloodlight = [&](FVector BaseLocation)
    {
        // Tower pole
        FVector PoleLoc = BaseLocation + FVector(0.0f, 0.0f, TowerHeight * 0.5f + SoccerField::GamePlaneZ);
        AStaticMeshActor* Pole = World->SpawnActor<AStaticMeshActor>(PoleLoc, FRotator::ZeroRotator);
        if (Pole)
        {
            Pole->SetMobility(EComponentMobility::Movable);
            UStaticMeshComponent* MeshComp = Pole->GetStaticMeshComponent();
            if (MeshComp)
            {
                UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
                if (CubeMesh)
                {
                    MeshComp->SetStaticMesh(CubeMesh);
                }
                MeshComp->SetMobility(EComponentMobility::Movable);
                MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Pole->SetActorScale3D(FVector(1.0f, 1.0f, TowerHeight / 100.0f));
                if (ConcreteMat)
                {
                    UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(ConcreteMat, MeshComp);
                    if (DynMat)
                    {
                        DynMat->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.15f, 0.15f, 0.17f));
                        MeshComp->SetMaterial(0, DynMat);
                    }
                }
            }
        }
    };

    CreateFloodlight(FVector(-CornerX, -CornerY, 0.0f));
    CreateFloodlight(FVector(-CornerX, CornerY, 0.0f));
    CreateFloodlight(FVector(CornerX, -CornerY, 0.0f));
    CreateFloodlight(FVector(CornerX, CornerY, 0.0f));
}

void ASoccerGameMode::SpawnBall()
{
    if (BallClass)
    {
        FVector SpawnLoc = FVector(0.0f, 0.0f, SoccerField::GamePlaneZ + BallPhysics::Radius + 5.0f);
        MatchBall = GetWorld()->SpawnActor<ASoccerBall>(BallClass, SpawnLoc, FRotator::ZeroRotator);
    }
    else
    {
        UE_LOG(LogSoccerSim, Warning, TEXT("BallClass not set in GameMode defaults!"));
    }
}

void ASoccerGameMode::SpawnTeams()
{
    // 5v5 formation: GK, CB, CB, CM, ST
    FFormationData DefaultFormation;
    DefaultFormation.Formation = EFormation::F_442;
    DefaultFormation.DisplayName = TEXT("1-2-1 (5v5)");
    DefaultFormation.Slots.SetNum(5);

    // Slot 0: Goalkeeper — stays deep, narrow zone
    DefaultFormation.Slots[0].NormalizedPosition = FVector2D(0.04f, 0.50f);
    DefaultFormation.Slots[0].Position = EPlayerPosition::GK;
    DefaultFormation.Slots[0].BallAttraction = 0.05f;
    DefaultFormation.Slots[0].ZoneMin = FVector2D(0.0f, 0.15f);
    DefaultFormation.Slots[0].ZoneMax = FVector2D(0.15f, 0.85f);

    // Slot 1: Left Centre-Back
    DefaultFormation.Slots[1].NormalizedPosition = FVector2D(0.25f, 0.30f);
    DefaultFormation.Slots[1].Position = EPlayerPosition::CB;
    DefaultFormation.Slots[1].BallAttraction = 0.20f;
    DefaultFormation.Slots[1].ZoneMin = FVector2D(0.05f, 0.0f);
    DefaultFormation.Slots[1].ZoneMax = FVector2D(0.60f, 0.55f);

    // Slot 2: Right Centre-Back
    DefaultFormation.Slots[2].NormalizedPosition = FVector2D(0.25f, 0.70f);
    DefaultFormation.Slots[2].Position = EPlayerPosition::CB;
    DefaultFormation.Slots[2].BallAttraction = 0.20f;
    DefaultFormation.Slots[2].ZoneMin = FVector2D(0.05f, 0.45f);
    DefaultFormation.Slots[2].ZoneMax = FVector2D(0.60f, 1.0f);

    // Slot 3: Central Midfielder — box-to-box
    DefaultFormation.Slots[3].NormalizedPosition = FVector2D(0.50f, 0.50f);
    DefaultFormation.Slots[3].Position = EPlayerPosition::CM;
    DefaultFormation.Slots[3].BallAttraction = 0.45f;
    DefaultFormation.Slots[3].ZoneMin = FVector2D(0.15f, 0.15f);
    DefaultFormation.Slots[3].ZoneMax = FVector2D(0.85f, 0.85f);

    // Slot 4: Striker — stays high
    DefaultFormation.Slots[4].NormalizedPosition = FVector2D(0.78f, 0.50f);
    DefaultFormation.Slots[4].Position = EPlayerPosition::ST;
    DefaultFormation.Slots[4].BallAttraction = 0.35f;
    DefaultFormation.Slots[4].ZoneMin = FVector2D(0.40f, 0.15f);
    DefaultFormation.Slots[4].ZoneMax = FVector2D(1.0f, 0.85f);

    SpawnTeam(ETeamId::Home, DefaultFormation, HomePlayers);
    SpawnTeam(ETeamId::Away, DefaultFormation, AwayPlayers);
}

void ASoccerGameMode::SpawnTeam(ETeamId Team, const FFormationData& Formation, TArray<ASoccerPlayerPawn*>& OutPlayers)
{
    if (!PlayerPawnClass)
    {
        UE_LOG(LogSoccerSim, Warning, TEXT("PlayerPawnClass not set in GameMode!"));
        return;
    }

    OutPlayers.Empty();
    for (int32 i = 0; i < Formation.Slots.Num(); i++)
    {
        const FFormationSlot& Slot = Formation.Slots[i];
        FVector SpawnPos = SoccerField::NormalizedToWorld(Slot.NormalizedPosition, Team);
        SpawnPos.Z = SoccerField::GamePlaneZ + PlayerMovement::CapsuleHalfHeight + 5.0f;

        FRotator SpawnRot = (Team == ETeamId::Home) ? FRotator(0.0f, 0.0f, 0.0f) : FRotator(0.0f, 180.0f, 0.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ASoccerPlayerPawn* NewPlayer = GetWorld()->SpawnActor<ASoccerPlayerPawn>(
            PlayerPawnClass, SpawnPos, SpawnRot, Params);

        if (NewPlayer)
        {
            NewPlayer->InitializePlayer(Team, i, Slot);
            OutPlayers.Add(NewPlayer);

            ASoccerAIController* AIC = GetWorld()->SpawnActor<ASoccerAIController>();
            if (AIC)
            {
                AIC->Possess(NewPlayer);
                NewPlayer->SetAssignedAIController(AIC);
            }

            UE_LOG(LogSoccerSim, Log, TEXT("Spawned %s player slot %d at %s"),
                Team == ETeamId::Home ? TEXT("Home") : TEXT("Away"), i, *SpawnPos.ToString());
        }
    }
}
