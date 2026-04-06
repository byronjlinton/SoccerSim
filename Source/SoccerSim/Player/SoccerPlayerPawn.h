#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerPlayerPawn.generated.h"

class ASoccerBall;
class UCapsuleComponent;
class UCharacterMovementComponent;
class UStaticMeshComponent;

UCLASS()
class SOCCERSIM_API ASoccerPlayerPawn : public ACharacter
{
    GENERATED_BODY()

public:
    ASoccerPlayerPawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    void InitializePlayer(ETeamId Team, int32 SlotIndex, const FFormationSlot& Slot);

    /** Move pawn to formation position (e.g. after goal or kick-off). Clears velocity. */
    void RepositionToFormation(FVector WorldPos, FRotator Rot);

    /** Updates placeholder mesh color from TeamId. Call when team is set (e.g. from InitializePlayer). */
    void UpdatePlaceholderAppearance();

    void SetMovementInput(FVector2D Input);
    void SetSprinting(bool bSprint);
    void AttemptKick(EKickType KickType);
    void AttemptShot(float ChargeTime);
    void AttemptTackle();
    void BeginShotCharge();

    /** Called from AnimNotify_KickContact at the kick contact frame. Applies pending kick. */
    void ExecuteKickFromNotify();

    void OnPlayerControlAcquired();
    void OnPlayerControlReleased();

    UFUNCTION(BlueprintPure, Category = "Player")
    ETeamId GetTeamId() const { return TeamId; }

    UFUNCTION(BlueprintPure, Category = "Player")
    EPlayerPosition GetPlayerPosition() const { return FormationSlot.Position; }

    UFUNCTION(BlueprintPure, Category = "Player")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, Category = "Player")
    const FFormationSlot& GetFormationSlot() const { return FormationSlot; }

    UFUNCTION(BlueprintPure, Category = "Player")
    bool IsHumanControlled() const { return bIsHumanControlled; }

    class ASoccerAIController* GetAssignedAIController() const { return AssignedAIController; }
    void SetAssignedAIController(class ASoccerAIController* AIC) { AssignedAIController = AIC; }

    UFUNCTION(BlueprintPure, Category = "Player")
    bool HasBall() const { return bHasBall; }

    UFUNCTION(BlueprintPure, Category = "Player")
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category = "Player")
    float GetCurrentStamina() const { return CurrentStamina; }

    UFUNCTION(BlueprintPure, Category = "Player")
    float GetStaminaPercent() const { return CurrentStamina / MaxStamina; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Data")
    FPlayerData PlayerData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Data")
    FFormationSlot FormationSlot;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Player")
    ETeamId TeamId = ETeamId::None;

    UPROPERTY(BlueprintReadOnly, Category = "Player")
    int32 SlotIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Player")
    bool bIsHumanControlled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Player")
    bool bIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category = "Player")
    bool bHasBall = false;

    UPROPERTY(BlueprintReadOnly, Category = "Player")
    bool bIsChargingShot = false;

    /** AI controller that owns this pawn when not human-controlled. Set by GameMode in SpawnTeam. */
    UPROPERTY(Transient)
    TObjectPtr<class ASoccerAIController> AssignedAIController;

    FVector2D CurrentMovementInput = FVector2D::ZeroVector;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float JogSpeed = PlayerMovement::JogSpeed;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float SprintSpeed = PlayerMovement::SprintSpeed;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float MoveAcceleration = PlayerMovement::Acceleration;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float MoveDeceleration = PlayerMovement::Deceleration;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float TurnRateStanding = PlayerMovement::TurnRateStanding;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float TurnRateSprinting = PlayerMovement::TurnRateSprinting;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float MaxStamina = PlayerMovement::MaxStamina;

    UPROPERTY(BlueprintReadOnly, Category = "Player|Stamina")
    float CurrentStamina = PlayerMovement::MaxStamina;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float SprintStaminaDrain = PlayerMovement::SprintStaminaDrain;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float JogStaminaDrain = PlayerMovement::JogStaminaDrain;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float StaminaRecoveryRate = PlayerMovement::StaminaRecovery;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Ball")
    float KickRange = PlayerMovement::KickRange;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Ball")
    float DribbleDistance = PlayerMovement::DribbleDistance;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Ball")
    float DribbleSpringStrength = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Ball")
    float DribbleDamping = 5.0f;

    /** Optional: set to a MetaHuman (or other) skeletal mesh asset path to use instead of default. e.g. /Game/MetaHumans/.../SK_... */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Appearance", meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    FSoftObjectPath MetaHumanMeshPath;

    /** Body of human-like placeholder when no skeletal mesh is set. */
    UPROPERTY(VisibleAnywhere, Category = "Player|Appearance")
    UStaticMeshComponent* PlaceholderBody = nullptr;

    /** Head of human-like placeholder when no skeletal mesh is set. */
    UPROPERTY(VisibleAnywhere, Category = "Player|Appearance")
    UStaticMeshComponent* PlaceholderHead = nullptr;

    /** Applies team-colored material to the skeletal mesh (if loaded). */
    void ApplyTeamMeshMaterial();

    void UpdateMovement(float DeltaTime);
    void UpdateStamina(float DeltaTime);
    void UpdateDribble(float DeltaTime);
    void UpdateBallProximity();

    ASoccerBall* FindBall() const;
    class ASoccerPlayerPawn* GetBestPassTarget() const;
    FVector CalculateKickDirection(EKickType KickType) const;
    float CalculateKickPower(EKickType KickType, float ChargeTime = 0.0f) const;
    FVector CalculateSpinFromApproach(FVector KickDir) const;

    UPROPERTY(Transient)
    ASoccerBall* CachedBall = nullptr;

    /** Team-colored material for PlaceholderBody/PlaceholderHead when no skeletal mesh is set. */
    UPROPERTY(Transient)
    class UMaterialInstanceDynamic* PlaceholderMaterial = nullptr;

    float DistToBall = TNumericLimits<float>::Max();

    /** Pending kick applied when AnimNotify_KickContact fires (or immediately if not using montage) */
    bool bPendingKick = false;
    EKickType PendingKickType = EKickType::ShortPass;
    float PendingChargeTime = 0.0f;

    /** If true, AttemptKick/AttemptShot only set pending; ExecuteKickFromNotify (e.g. from montage) applies. */
    UPROPERTY(EditDefaultsOnly, Category = "Player|Animation")
    bool bKickDeferredByMontage = false;

    // ========== Animation Montage Support ==========

    /** Kick animation montage to play when executing a kick. Set in Blueprint or config. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Animation", meta = (AllowedClasses = "/Script/Engine.AnimMontage"))
    FSoftObjectPath KickMontagePath;

    /** Header animation montage to play when executing a header. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Animation", meta = (AllowedClasses = "/Script/Engine.AnimMontage"))
    FSoftObjectPath HeaderMontagePath;

    /** Spin/celebration animation montage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Animation", meta = (AllowedClasses = "/Script/Engine.AnimMontage"))
    FSoftObjectPath CelebrateMontagePath;

    /** Cached kick montage (loaded on demand). */
    UPROPERTY(Transient)
    class UAnimMontage* KickMontage = nullptr;

    /** Cached header montage (loaded on demand). */
    UPROPERTY(Transient)
    class UAnimMontage* HeaderMontage = nullptr;

    /** Cached celebrate montage (loaded on demand). */
    UPROPERTY(Transient)
    class UAnimMontage* CelebrateMontage = nullptr;

    /** True when playing an action montage (kick, header, celebrate). */
    UPROPERTY(BlueprintReadOnly, Category = "Player|Animation")
    bool bIsPlayingActionMontage = false;

    /** Plays the kick animation montage. Returns true if montage started. */
    UFUNCTION(BlueprintCallable, Category = "Player|Animation")
    bool PlayKickMontage();

    /** Plays the header animation montage. Returns true if montage started. */
    UFUNCTION(BlueprintCallable, Category = "Player|Animation")
    bool PlayHeaderMontage();

    /** Plays the celebration animation montage. */
    UFUNCTION(BlueprintCallable, Category = "Player|Animation")
    bool PlayCelebrateMontage();

    /** Called when an action montage finishes (via montage delegate). */
    UFUNCTION()
    void OnActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    /** Loads animation montages from paths (called in BeginPlay). */
    void LoadAnimationMontages();
};
