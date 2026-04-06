#include "SoccerPlayerPawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreMisc.h"
#include "SoccerSim/Ball/SoccerBall.h"
#include "SoccerSim/Core/SoccerGameMode.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"

ASoccerPlayerPawn::ASoccerPlayerPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(PlayerMovement::CapsuleRadius, PlayerMovement::CapsuleHalfHeight);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("SoccerPlayer"));

    UCharacterMovementComponent* MC = GetCharacterMovement();
    MC->MaxWalkSpeed = PlayerMovement::JogSpeed;
    MC->MaxAcceleration = PlayerMovement::Acceleration;
    MC->BrakingDecelerationWalking = PlayerMovement::Deceleration;
    MC->bOrientRotationToMovement = true;
    MC->RotationRate = FRotator(0.0f, TurnRateStanding, 0.0f);
    MC->bUseControllerDesiredRotation = false;
    MC->GravityScale = 1.0f;
    MC->bCanWalkOffLedges = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(GetCapsuleComponent());
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderBody->SetVisibility(false);

    PlaceholderHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderHead"));
    PlaceholderHead->SetupAttachment(GetCapsuleComponent());
    PlaceholderHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderHead->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        PlaceholderBody->SetStaticMesh(CylinderMesh.Object);
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        PlaceholderHead->SetStaticMesh(SphereMesh.Object);
    }
}

void ASoccerPlayerPawn::BeginPlay()
{
    Super::BeginPlay();
    CurrentStamina = MaxStamina;

    // Config-driven animation montage paths
    if (GConfig && GGameIni.Len() > 0)
    {
        FString ConfigMontagePath;
        if (!KickMontagePath.IsValid() && GConfig->GetString(TEXT("/Script/SoccerSim.SoccerPlayerPawn"), TEXT("DefaultKickMontagePath"), ConfigMontagePath, GGameIni) && !ConfigMontagePath.IsEmpty())
        {
            KickMontagePath.SetPath(ConfigMontagePath);
        }
        FString ConfigDeferred;
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerPlayerPawn"), TEXT("bKickDeferredByMontage"), ConfigDeferred, GGameIni) && !ConfigDeferred.IsEmpty())
        {
            bKickDeferredByMontage = (ConfigDeferred == TEXT("True") || ConfigDeferred == TEXT("1"));
        }
    }

    LoadAnimationMontages();

    // Optional default mesh path from config (DefaultGame.ini [/Script/SoccerSim.SoccerPlayerPawn] DefaultMetaHumanMeshPath="...")
    if (!MetaHumanMeshPath.IsValid() && GConfig && GGameIni.Len() > 0)
    {
        FString ConfigPath;
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerPlayerPawn"), TEXT("DefaultMetaHumanMeshPath"), ConfigPath, GGameIni) && !ConfigPath.IsEmpty())
        {
            MetaHumanMeshPath.SetPath(ConfigPath);
        }
    }
    // Optional MetaHuman (or other) skeletal mesh: if path is set, use it for the character mesh
    if (MetaHumanMeshPath.IsValid() && GetMesh())
    {
        if (USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(MetaHumanMeshPath.TryLoad()))
        {
            GetMesh()->SetSkeletalMesh(LoadedMesh);
            // Optional: skin material override so characters show skin instead of topology/wireframe
            if (GConfig && GGameIni.Len() > 0)
            {
                FString SkinPath;
                if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerPlayerPawn"), TEXT("DefaultMetaHumanSkinMaterialPath"), SkinPath, GGameIni) && !SkinPath.IsEmpty())
                {
                    if (UMaterialInterface* SkinMat = Cast<UMaterialInterface>(FSoftObjectPath(SkinPath).TryLoad()))
                    {
                        const int32 NumSlots = GetMesh()->GetNumMaterials();
                        for (int32 i = 0; i < NumSlots; ++i)
                        {
                            GetMesh()->SetMaterial(i, SkinMat);
                        }
                    }
                }
            }
            // Optional: set Anim Instance class from config so MetaHuman skeleton is driven by retargeted run/sprint/kick
            if (GConfig && GGameIni.Len() > 0)
            {
                FString AnimPath;
                if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerPlayerPawn"), TEXT("DefaultMetaHumanAnimBlueprintPath"), AnimPath, GGameIni) && !AnimPath.IsEmpty())
                {
                    FSoftObjectPath AnimRef(AnimPath);
                    if (UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(AnimRef.TryLoad()))
                    {
                        if (AnimBP->GeneratedClass)
                        {
                            GetMesh()->SetAnimInstanceClass(AnimBP->GeneratedClass);
                        }
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogSoccerSim, Warning, TEXT("MetaHuman mesh failed to load: %s. Check DefaultMetaHumanMeshPath in DefaultGame.ini and use Copy Reference from the Skeletal Mesh (SK_...) in Content Browser."), *MetaHumanMeshPath.ToString());
        }
    }

    // Optional default player mesh from config (e.g. Mannequin migrated from Third Person template)
    if (GetMesh() && !GetMesh()->GetSkeletalMeshAsset() && GConfig && GGameIni.Len() > 0)
    {
        FString ConfigPath;
        if (GConfig->GetString(TEXT("/Script/SoccerSim.SoccerPlayerPawn"), TEXT("DefaultPlayerMeshPath"), ConfigPath, GGameIni) && !ConfigPath.IsEmpty())
        {
            FSoftObjectPath Path(ConfigPath);
            if (USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(Path.TryLoad()))
            {
                GetMesh()->SetSkeletalMesh(LoadedMesh);
            }
        }
    }

    // When no skeletal mesh is set, show a human-like silhouette (body + head) so all 22 players are visible
    if (GetMesh() && !GetMesh()->GetSkeletalMeshAsset() && PlaceholderBody && PlaceholderHead)
    {
        // Body: cylinder ~40cm wide, ~80cm tall; engine cylinder is 50 radius, 100 height (units = cm)
        PlaceholderBody->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.8f));
        PlaceholderBody->SetRelativeLocation(FVector(0.0f, 0.0f, -54.0f)); // stand on capsule base, center at half body height

        // Head: sphere ~25cm; engine sphere radius 50. Place above body.
        PlaceholderHead->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
        PlaceholderHead->SetRelativeLocation(FVector(0.0f, 0.0f, -1.5f));

        // Load material with BaseColor parameter (supports SetVectorParameterValue)
        UMaterialInterface* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/M_DynamicColor.M_DynamicColor"));
        if (!BaseMat)
            BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial"));
        if (BaseMat)
        {
            PlaceholderMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
            if (PlaceholderMaterial)
            {
                PlaceholderMaterial->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.5f, 0.5f, 0.5f));
                PlaceholderBody->SetMaterial(0, PlaceholderMaterial);
                PlaceholderHead->SetMaterial(0, PlaceholderMaterial);
            }
        }
        PlaceholderBody->SetVisibility(true);
        PlaceholderHead->SetVisibility(true);
    }
}

void ASoccerPlayerPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateBallProximity();
    UpdateMovement(DeltaTime);
    UpdateStamina(DeltaTime);

    if (bHasBall)
    {
        UpdateDribble(DeltaTime);
    }
}

void ASoccerPlayerPawn::InitializePlayer(ETeamId Team, int32 InSlotIndex, const FFormationSlot& Slot)
{
    TeamId = Team;
    SlotIndex = InSlotIndex;
    FormationSlot = Slot;

    PlayerData.JerseyNumber = InSlotIndex + 1;
    PlayerData.PreferredPosition = Slot.Position;
    PlayerData.PlayerName = FString::Printf(TEXT("%s Player %d"),
        Team == ETeamId::Home ? TEXT("Home") : TEXT("Away"), InSlotIndex + 1);

    UpdatePlaceholderAppearance();
    ApplyTeamMeshMaterial();

    if (Slot.Position == EPlayerPosition::GK)
    {
        PlayerData.Stats.Goalkeeping = 75;
        PlayerData.Stats.Pace = 40;
    }

    UE_LOG(LogSoccerSim, Verbose, TEXT("Initialized %s at slot %d, position %d"),
        *PlayerData.PlayerName, SlotIndex, static_cast<int32>(Slot.Position));
}

void ASoccerPlayerPawn::RepositionToFormation(FVector WorldPos, FRotator Rot)
{
    SetActorLocation(WorldPos);
    SetActorRotation(Rot);
    if (UCharacterMovementComponent* MC = GetCharacterMovement())
    {
        MC->Velocity = FVector::ZeroVector;
    }
}

void ASoccerPlayerPawn::UpdatePlaceholderAppearance()
{
    if (!PlaceholderMaterial) return;
    bool bPlaceholderVisible = (PlaceholderBody && PlaceholderBody->IsVisible()) || (PlaceholderHead && PlaceholderHead->IsVisible());
    if (!bPlaceholderVisible) return;

    FLinearColor TeamColor(0.5f, 0.5f, 0.5f);
    switch (TeamId)
    {
    case ETeamId::Home: TeamColor = FLinearColor(0.85f, 0.2f, 0.2f); break;  // red
    case ETeamId::Away: TeamColor = FLinearColor(0.2f, 0.35f, 0.85f); break; // blue
    default: break;
    }
    PlaceholderMaterial->SetVectorParameterValue(FName("BaseColor"), TeamColor);
}

void ASoccerPlayerPawn::SetMovementInput(FVector2D Input)
{
    CurrentMovementInput = Input;
}

void ASoccerPlayerPawn::SetSprinting(bool bSprint)
{
    bIsSprinting = bSprint && (CurrentStamina > MaxStamina * 0.05f);
}

void ASoccerPlayerPawn::ApplyTeamMeshMaterial()
{
    if (TeamId == ETeamId::None) return;
    if (!GetMesh() || !GetMesh()->GetSkeletalMeshAsset()) return;

    // Determine team color
    FLinearColor TeamColor;
    switch (TeamId)
    {
    case ETeamId::Home: TeamColor = FLinearColor(0.85f, 0.15f, 0.15f); break;  // red
    case ETeamId::Away: TeamColor = FLinearColor(0.15f, 0.3f, 0.85f); break;   // blue
    default: return;
    }

    // Create a dynamic material instance from M_DynamicColor and apply to all mesh slots
    UMaterialInterface* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/M_DynamicColor.M_DynamicColor"));
    if (!BaseMat) return;

    UMaterialInstanceDynamic* TeamMI = UMaterialInstanceDynamic::Create(BaseMat, this);
    if (!TeamMI) return;

    TeamMI->SetVectorParameterValue(FName("BaseColor"), TeamColor);
    const int32 NumSlots = GetMesh()->GetNumMaterials();
    for (int32 i = 0; i < NumSlots; ++i)
    {
        GetMesh()->SetMaterial(i, TeamMI);
    }
}

void ASoccerPlayerPawn::UpdateMovement(float DeltaTime)
{
    if (CurrentMovementInput.IsNearlyZero())
    {
        return;
    }

    float CameraYaw = 0.0f;
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (AActor* ViewTarget = PC->GetViewTarget())
        {
            CameraYaw = ViewTarget->GetActorRotation().Yaw;
        }
    }

    FRotator CameraRot(0.0f, CameraYaw, 0.0f);
    FVector WorldDirection = CameraRot.RotateVector(FVector(CurrentMovementInput.X, CurrentMovementInput.Y, 0.0f));
    WorldDirection.Z = 0.0f;
    WorldDirection.Normalize();

    float TargetSpeed = bIsSprinting ? SprintSpeed : JogSpeed;
    TargetSpeed *= PlayerData.Stats.GetSpeedMultiplier();

    if (CurrentStamina < MaxStamina * 0.25f)
    {
        TargetSpeed *= 0.85f;
    }

    GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;

    float CurrentSpeed = GetVelocity().Size();
    float SpeedRatio = FMath::Clamp(CurrentSpeed / SprintSpeed, 0.0f, 1.0f);
    float CurrentTurnRate = FMath::Lerp(TurnRateStanding, TurnRateSprinting, SpeedRatio);
    GetCharacterMovement()->RotationRate = FRotator(0.0f, CurrentTurnRate, 0.0f);

    // Directly set velocity instead of AddMovementInput.
    // AddMovementInput uses ConsumeInputVector which routes through
    // ReplicatedInputVector for non-locally-controlled pawns (AI),
    // causing AI characters to never actually receive movement input.
    FVector CurrentVel = GetCharacterMovement()->Velocity;
    GetCharacterMovement()->Velocity = FVector(
        WorldDirection.X * TargetSpeed,
        WorldDirection.Y * TargetSpeed,
        CurrentVel.Z);
}

void ASoccerPlayerPawn::UpdateStamina(float DeltaTime)
{
    float DrainRate = 0.0f;

    if (bIsSprinting && !GetVelocity().IsNearlyZero())
    {
        DrainRate = SprintStaminaDrain;
    }
    else if (!GetVelocity().IsNearlyZero())
    {
        DrainRate = JogStaminaDrain;
    }
    else
    {
        DrainRate = -StaminaRecoveryRate;
    }

    float StaminaMod = PlayerData.Stats.GetStaminaRate();
    if (DrainRate > 0)
        DrainRate /= StaminaMod;
    else
        DrainRate *= StaminaMod;

    CurrentStamina = FMath::Clamp(CurrentStamina - DrainRate * DeltaTime, 0.0f, MaxStamina);

    if (CurrentStamina <= 0.0f)
    {
        bIsSprinting = false;
    }
}

void ASoccerPlayerPawn::UpdateBallProximity()
{
    CachedBall = FindBall();
    if (!CachedBall)
    {
        DistToBall = TNumericLimits<float>::Max();
        bHasBall = false;
        return;
    }

    DistToBall = FVector::Dist(GetActorLocation(), CachedBall->GetActorLocation());

    bool bInRange = DistToBall < (DribbleDistance + 50.0f);
    bool bBallSlow = CachedBall->GetBallSpeed() < 300.0f;
    FVector RelVel = CachedBall->GetBallVelocity() - GetVelocity();
    bool bMovingWithUs = RelVel.Size() < 200.0f;

    bHasBall = bInRange && (bBallSlow || bMovingWithUs);

    if (bHasBall)
    {
        CachedBall->LastTouchTeam = TeamId;
        CachedBall->LastTouchPlayerIndex = SlotIndex;
    }
}

void ASoccerPlayerPawn::UpdateDribble(float DeltaTime)
{
    if (!CachedBall || !bHasBall) return;

    FVector DesiredBallPos = GetActorLocation() + GetActorForwardVector() * DribbleDistance;
    DesiredBallPos.Z = BallPhysics::Radius + 1.0f;

    FVector BallPos = CachedBall->GetActorLocation();
    FVector Correction = DesiredBallPos - BallPos;
    Correction.Z = 0.0f;

    FVector BallVel = CachedBall->GetBallVelocity();
    FVector Force = Correction * DribbleSpringStrength - BallVel * DribbleDamping;
    Force *= BallPhysics::Mass;

    CachedBall->ApplyKick(Force * DeltaTime, FVector::ZeroVector);

    float SkillNoise = (1.0f - PlayerData.Stats.GetDribbleControl()) * 15.0f;
    if (SkillNoise > 0.01f)
    {
        FVector Noise = FMath::VRand() * SkillNoise * DeltaTime;
        Noise.Z = 0.0f;
        CachedBall->ApplyKick(Noise, FVector::ZeroVector);
    }
}

ASoccerBall* ASoccerPlayerPawn::FindBall() const
{
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    return GM ? GM->MatchBall : nullptr;
}

ASoccerPlayerPawn* ASoccerPlayerPawn::GetBestPassTarget() const
{
    ASoccerGameMode* GM = Cast<ASoccerGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return nullptr;

    TArray<ASoccerPlayerPawn*>& Team = (TeamId == ETeamId::Home) ? GM->HomePlayers : GM->AwayPlayers;
    FVector MyLoc = GetActorLocation();
    FVector BallLoc = CachedBall ? CachedBall->GetActorLocation() : MyLoc;
    FVector Forward = GetActorForwardVector();

    ASoccerPlayerPawn* Best = nullptr;
    float BestScore = -TNumericLimits<float>::Max();

    for (ASoccerPlayerPawn* P : Team)
    {
        if (P == this) continue;
        if (P->GetPlayerPosition() == EPlayerPosition::GK) continue;

        FVector ToTarget = P->GetActorLocation() - BallLoc;
        float Dist = ToTarget.Size();
        if (Dist < 100.0f || Dist > 3000.0f) continue;

        ToTarget.Normalize();
        float Dot = FVector::DotProduct(Forward, ToTarget);
        if (Dot < 0.0f) continue;

        float Progression = (TeamId == ETeamId::Home) ? ToTarget.X : -ToTarget.X;
        float Score = Dot * 100.0f + Progression * 0.1f - Dist * 0.02f;

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = P;
        }
    }
    return Best;
}

void ASoccerPlayerPawn::AttemptKick(EKickType KickType)
{
    if (!CachedBall || DistToBall > KickRange) return;
    if (bIsPlayingActionMontage) return;

    bPendingKick = true;
    PendingKickType = KickType;
    PendingChargeTime = 0.0f;

    if (bKickDeferredByMontage)
    {
        PlayKickMontage();
    }
    else
    {
        // Execute kick immediately and play montage for visual feedback
        ExecuteKickFromNotify();
        PlayKickMontage();
    }
}

void ASoccerPlayerPawn::AttemptShot(float ChargeTime)
{
    if (!CachedBall || DistToBall > KickRange) return;
    if (bIsPlayingActionMontage) return;

    bIsChargingShot = false;
    bPendingKick = true;
    PendingKickType = EKickType::Shot;
    PendingChargeTime = ChargeTime;

    if (bKickDeferredByMontage)
    {
        PlayKickMontage();
    }
    else
    {
        // Execute kick immediately and play montage for visual feedback
        ExecuteKickFromNotify();
        PlayKickMontage();
    }
}

void ASoccerPlayerPawn::AttemptTackle()
{
    if (!CachedBall) return;
    if (DistToBall > KickRange * 1.5f) return;
    if (bIsPlayingActionMontage) return;

    FVector ToBall = CachedBall->GetActorLocation() - GetActorLocation();
    ToBall.Normalize();

    float TackleForce = 500.0f * PlayerData.Stats.GetTackleSuccess();
    FVector Impulse = -ToBall * TackleForce * BallPhysics::Mass;

    CachedBall->ApplyKick(Impulse, FVector::ZeroVector);

    UE_LOG(LogSoccerSim, Verbose, TEXT("%s tackled"), *PlayerData.PlayerName);
}

void ASoccerPlayerPawn::BeginShotCharge()
{
    bIsChargingShot = true;
}

void ASoccerPlayerPawn::ExecuteKickFromNotify()
{
    if (!bPendingKick || !CachedBall) return;
    if (DistToBall > KickRange * 1.5f) { bPendingKick = false; return; }

    FVector KickDir = CalculateKickDirection(PendingKickType);
    float KickPower = CalculateKickPower(PendingKickType, PendingChargeTime);
    FVector Spin = CalculateSpinFromApproach(KickDir);

    FVector Impulse = KickDir * KickPower * BallPhysics::Mass;
    CachedBall->ApplyKick(Impulse, Spin);
    CachedBall->LastTouchTeam = TeamId;
    CachedBall->LastTouchPlayerIndex = SlotIndex;

    bHasBall = false;
    bPendingKick = false;

    UE_LOG(LogSoccerSim, Verbose, TEXT("%s executed kick from notify (%s) power=%.0f"),
        *PlayerData.PlayerName, *UEnum::GetValueAsString(PendingKickType), KickPower);
}

void ASoccerPlayerPawn::OnPlayerControlAcquired()
{
    bIsHumanControlled = true;
}

void ASoccerPlayerPawn::OnPlayerControlReleased()
{
    bIsHumanControlled = false;
    CurrentMovementInput = FVector2D::ZeroVector;
    bIsSprinting = false;
}

FVector ASoccerPlayerPawn::CalculateKickDirection(EKickType KickType) const
{
    FVector Forward = GetActorForwardVector();

    switch (KickType)
    {
    case EKickType::ShortPass:
    case EKickType::DrivenPass:
    {
        ASoccerPlayerPawn* Target = GetBestPassTarget();
        if (Target)
        {
            FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            ToTarget.Z = 0.0f;
            return ToTarget;
        }
        return Forward;
    }

    case EKickType::ThroughBall:
        return Forward;

    case EKickType::LobPass:
    case EKickType::Cross:
    {
        FVector LobDir = Forward;
        LobDir.Z = 0.6f;
        LobDir.Normalize();
        return LobDir;
    }

    case EKickType::Shot:
    {
        FVector GoalPos = (TeamId == ETeamId::Home) ?
            SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
        FVector ToGoal = GoalPos - GetActorLocation();
        ToGoal.Normalize();

        float Inaccuracy = (1.0f - PlayerData.Stats.GetShotPowerMultiplier()) * 0.15f;
        ToGoal += FMath::VRand() * Inaccuracy;
        ToGoal.Normalize();

        return ToGoal;
    }

    case EKickType::LobShot:
    {
        FVector GoalPos = (TeamId == ETeamId::Home) ?
            SoccerField::AwayGoalCenter() : SoccerField::HomeGoalCenter();
        FVector ToGoal = GoalPos - GetActorLocation();
        ToGoal.Z = ToGoal.Size() * 0.3f;
        ToGoal.Normalize();
        return ToGoal;
    }

    default:
        return Forward;
    }
}

float ASoccerPlayerPawn::CalculateKickPower(EKickType KickType, float ChargeTime) const
{
    float BasePower = 0.0f;

    switch (KickType)
    {
    case EKickType::ShortPass:    BasePower = PlayerMovement::ShortPassPower; break;
    case EKickType::DrivenPass:   BasePower = PlayerMovement::DrivenPassPower; break;
    case EKickType::LobPass:      BasePower = PlayerMovement::LobPassPower; break;
    case EKickType::ThroughBall:  BasePower = PlayerMovement::ThroughBallPower; break;
    case EKickType::Cross:        BasePower = PlayerMovement::CrossPower; break;
    case EKickType::Shot:
    case EKickType::FreeKickShot:
    {
        float ChargeRatio = FMath::Clamp(ChargeTime / 1.5f, 0.0f, 1.0f);
        BasePower = FMath::Lerp(PlayerMovement::ShotPowerMin, PlayerMovement::ShotPowerMax, ChargeRatio);
        break;
    }
    case EKickType::LobShot:
        BasePower = PlayerMovement::ShotPowerMin * 0.8f; break;
    case EKickType::Header:
        BasePower = 600.0f; break;
    default:
        BasePower = PlayerMovement::ShortPassPower; break;
    }

    BasePower *= PlayerData.Stats.GetShotPowerMultiplier();

    if (CurrentStamina < MaxStamina * 0.25f)
    {
        BasePower *= 0.9f;
    }

    return BasePower;
}

FVector ASoccerPlayerPawn::CalculateSpinFromApproach(FVector KickDir) const
{
    FVector Forward = GetActorForwardVector();
    FVector Right = GetActorRightVector();

    float SideAngle = FVector::DotProduct(KickDir, Right);

    FVector SpinAxis = FVector::UpVector * SideAngle * 15.0f;

    if (KickDir.Z > 0.2f)
    {
        SpinAxis += Right * 8.0f;
    }

    return SpinAxis;
}

// ========== Animation Montage Support ==========

void ASoccerPlayerPawn::LoadAnimationMontages()
{
    if (KickMontagePath.IsValid())
    {
        KickMontage = Cast<UAnimMontage>(KickMontagePath.TryLoad());
    }
    if (HeaderMontagePath.IsValid())
    {
        HeaderMontage = Cast<UAnimMontage>(HeaderMontagePath.TryLoad());
    }
    if (CelebrateMontagePath.IsValid())
    {
        CelebrateMontage = Cast<UAnimMontage>(CelebrateMontagePath.TryLoad());
    }
}

bool ASoccerPlayerPawn::PlayKickMontage()
{
    if (!KickMontage) return false;

    UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInst) return false;

    float Duration = AnimInst->Montage_Play(KickMontage, 1.0f);
    if (Duration > 0.0f)
    {
        bIsPlayingActionMontage = true;
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &ASoccerPlayerPawn::OnActionMontageEnded);
        AnimInst->Montage_SetEndDelegate(EndDelegate, KickMontage);
        return true;
    }
    return false;
}

bool ASoccerPlayerPawn::PlayHeaderMontage()
{
    if (!HeaderMontage) return false;

    UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInst) return false;

    float Duration = AnimInst->Montage_Play(HeaderMontage, 1.0f);
    if (Duration > 0.0f)
    {
        bIsPlayingActionMontage = true;
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &ASoccerPlayerPawn::OnActionMontageEnded);
        AnimInst->Montage_SetEndDelegate(EndDelegate, HeaderMontage);
        return true;
    }
    return false;
}

bool ASoccerPlayerPawn::PlayCelebrateMontage()
{
    if (!CelebrateMontage) return false;

    UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInst) return false;

    float Duration = AnimInst->Montage_Play(CelebrateMontage, 1.0f);
    if (Duration > 0.0f)
    {
        bIsPlayingActionMontage = true;
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &ASoccerPlayerPawn::OnActionMontageEnded);
        AnimInst->Montage_SetEndDelegate(EndDelegate, CelebrateMontage);
        return true;
    }
    return false;
}

void ASoccerPlayerPawn::OnActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    bIsPlayingActionMontage = false;
}
