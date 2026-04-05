#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SoccerSimTypes.generated.h"

// ============================================================
// COLLISION CHANNELS
// These must match the DefaultEngine.ini configuration above
// ============================================================
#define COLLISION_BALL          ECC_GameTraceChannel1
#define COLLISION_PLAYER_BODY   ECC_GameTraceChannel2
#define COLLISION_FIELD_BOUNDARY ECC_GameTraceChannel3
#define COLLISION_GOAL_TRIGGER  ECC_GameTraceChannel4

// ============================================================
// ENUMS
// ============================================================

UENUM(BlueprintType)
enum class ETeamId : uint8
{
    Home    UMETA(DisplayName = "Home"),
    Away    UMETA(DisplayName = "Away"),
    None    UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class EFormation : uint8
{
    F_442       UMETA(DisplayName = "4-4-2"),
    F_433       UMETA(DisplayName = "4-3-3"),
    F_4231      UMETA(DisplayName = "4-2-3-1"),
    F_352       UMETA(DisplayName = "3-5-2"),
    F_343       UMETA(DisplayName = "3-4-3"),
    F_4141      UMETA(DisplayName = "4-1-4-1"),
    F_532       UMETA(DisplayName = "5-3-2"),
    F_4222      UMETA(DisplayName = "4-2-2-2"),
    F_4411      UMETA(DisplayName = "4-4-1-1"),
    F_4321      UMETA(DisplayName = "4-3-2-1")
};

UENUM(BlueprintType)
enum class EPlayerPosition : uint8
{
    GK      UMETA(DisplayName = "Goalkeeper"),
    LB      UMETA(DisplayName = "Left Back"),
    CB      UMETA(DisplayName = "Center Back"),
    RB      UMETA(DisplayName = "Right Back"),
    LWB     UMETA(DisplayName = "Left Wing Back"),
    RWB     UMETA(DisplayName = "Right Wing Back"),
    CDM     UMETA(DisplayName = "Central Defensive Mid"),
    CM      UMETA(DisplayName = "Central Midfielder"),
    CAM     UMETA(DisplayName = "Central Attacking Mid"),
    LM      UMETA(DisplayName = "Left Midfielder"),
    RM      UMETA(DisplayName = "Right Midfielder"),
    LW      UMETA(DisplayName = "Left Winger"),
    RW      UMETA(DisplayName = "Right Winger"),
    ST      UMETA(DisplayName = "Striker"),
    CF      UMETA(DisplayName = "Center Forward")
};

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    PreMatch        UMETA(DisplayName = "Pre Match"),
    KickOff         UMETA(DisplayName = "Kick Off"),
    FirstHalf       UMETA(DisplayName = "First Half"),
    HalfTime        UMETA(DisplayName = "Half Time"),
    SecondHalfKickOff UMETA(DisplayName = "Second Half Kick Off"),
    SecondHalf      UMETA(DisplayName = "Second Half"),
    FullTime        UMETA(DisplayName = "Full Time"),
    ExtraFirstHalf  UMETA(DisplayName = "Extra Time First Half"),
    ExtraSecondHalf UMETA(DisplayName = "Extra Time Second Half"),
    Penalties       UMETA(DisplayName = "Penalty Shootout")
};

UENUM(BlueprintType)
enum class EBallState : uint8
{
    InPlay          UMETA(DisplayName = "In Play"),
    OutForThrowIn   UMETA(DisplayName = "Out - Throw In"),
    OutForGoalKick  UMETA(DisplayName = "Out - Goal Kick"),
    OutForCorner    UMETA(DisplayName = "Out - Corner"),
    Goal            UMETA(DisplayName = "Goal"),
    FreeKick        UMETA(DisplayName = "Free Kick"),
    PenaltyKick     UMETA(DisplayName = "Penalty"),
    Dead            UMETA(DisplayName = "Dead Ball")
};

UENUM(BlueprintType)
enum class EKickType : uint8
{
    ShortPass       UMETA(DisplayName = "Short Pass"),
    DrivenPass      UMETA(DisplayName = "Driven Pass"),
    LobPass         UMETA(DisplayName = "Lob Pass"),
    ThroughBall     UMETA(DisplayName = "Through Ball"),
    Shot            UMETA(DisplayName = "Shot"),
    LobShot         UMETA(DisplayName = "Lob Shot"),
    Cross           UMETA(DisplayName = "Cross"),
    FreeKickShot    UMETA(DisplayName = "Free Kick Shot"),
    GoalKick        UMETA(DisplayName = "Goal Kick"),
    ThrowIn         UMETA(DisplayName = "Throw In"),
    Header          UMETA(DisplayName = "Header")
};

UENUM(BlueprintType)
enum class EDefensiveStyle : uint8
{
    Zonal           UMETA(DisplayName = "Zonal"),
    ManMark         UMETA(DisplayName = "Man Marking"),
    Hybrid          UMETA(DisplayName = "Hybrid")
};

UENUM(BlueprintType)
enum class EPlayerAIState : uint8
{
    Idle                UMETA(DisplayName = "Idle"),
    ChasingBall         UMETA(DisplayName = "Chasing Ball"),
    HoldingPosition     UMETA(DisplayName = "Holding Position"),
    MakingRun           UMETA(DisplayName = "Making Run"),
    Dribbling           UMETA(DisplayName = "Dribbling"),
    PreparingPass       UMETA(DisplayName = "Preparing Pass"),
    PreparingShot       UMETA(DisplayName = "Preparing Shot"),
    Pressing            UMETA(DisplayName = "Pressing"),
    CoveringLane        UMETA(DisplayName = "Covering Lane"),
    TrackingRunner      UMETA(DisplayName = "Tracking Runner"),
    Tackling            UMETA(DisplayName = "Tackling"),
    Celebrating         UMETA(DisplayName = "Celebrating"),
    TakingSetPiece      UMETA(DisplayName = "Taking Set Piece"),
    ReturnToPosition    UMETA(DisplayName = "Returning to Position")
};

// ============================================================
// STRUCTS
// ============================================================

USTRUCT(BlueprintType)
struct FPlayerStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Pace = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Shooting = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Passing = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Dribbling = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Defending = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Physical = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "99"))
    int32 Goalkeeping = 10;

    // Derived helpers
    float GetSpeedMultiplier() const { return 0.7f + (Pace / 99.0f) * 0.3f; }
    float GetShotPowerMultiplier() const { return 0.7f + (Shooting / 99.0f) * 0.3f; }
    float GetPassAccuracy() const { return 0.6f + (Passing / 99.0f) * 0.4f; }
    float GetDribbleControl() const { return 0.5f + (Dribbling / 99.0f) * 0.5f; }
    float GetTackleSuccess() const { return 0.4f + (Defending / 99.0f) * 0.6f; }
    float GetStaminaRate() const { return 0.7f + (Physical / 99.0f) * 0.3f; }
};

USTRUCT(BlueprintType)
struct FPlayerData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PlayerName = TEXT("Player");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 JerseyNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPlayerPosition PreferredPosition = EPlayerPosition::CM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FPlayerStats Stats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Height = 180.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight = 75.0f; // kg
};

USTRUCT(BlueprintType)
struct FFormationSlot
{
    GENERATED_BODY()

    // Normalized position on pitch: X = 0 (own goal line) to 1 (opponent goal line)
    // Y = 0 (left touchline) to 1 (right touchline)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D NormalizedPosition = FVector2D(0.5f, 0.5f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPlayerPosition Position = EPlayerPosition::CM;

    // How much this slot shifts toward the ball (0 = stays home, 1 = follows ball fully)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BallAttraction = 0.3f;

    // Positional zone limits (normalized). Player AI will not leave this zone.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D ZoneMin = FVector2D(0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D ZoneMax = FVector2D(1.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct FFormationData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFormation Formation = EFormation::F_442;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName = TEXT("4-4-2");

    // Exactly 11 slots (index 0 = GK always)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FFormationSlot> Slots;
};

USTRUCT(BlueprintType)
struct FTacticalSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefensiveLineHeight = 0.4f; // 0 = deep, 1 = high

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Compactness = 0.5f; // 0 = spread, 1 = tight

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PressingIntensity = 0.5f; // 0 = drop back, 1 = gegenpress

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PassingDirectness = 0.5f; // 0 = short/possession, 1 = long/direct

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AttackingWidth = 0.5f; // 0 = narrow, 1 = wide

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.5", ClampMax = "2.0"))
    float TempoMultiplier = 1.0f; // AI decision speed

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDefensiveStyle DefensiveStyle = EDefensiveStyle::Zonal;
};

// ============================================================
// FIELD CONSTANTS
// ============================================================
namespace SoccerField
{
    // All values in centimeters (UE units)
    /** World Z for the playing surface so content appears above default level floor (e.g. grey plane at 0). */
    constexpr float GamePlaneZ = 100.0f;

    constexpr float PitchLength = 10500.0f;         // 105m
    constexpr float PitchWidth = 6800.0f;            // 68m
    constexpr float HalfLength = PitchLength / 2.0f;
    constexpr float HalfWidth = PitchWidth / 2.0f;

    constexpr float PenaltyBoxLength = 1650.0f;      // 16.5m from goal line
    constexpr float PenaltyBoxWidth = 4032.0f;        // 40.32m wide (centered)
    constexpr float GoalBoxLength = 550.0f;           // 5.5m from goal line
    constexpr float GoalBoxWidth = 1832.0f;           // 18.32m wide (centered)

    constexpr float GoalWidth = 732.0f;               // 7.32m between posts
    constexpr float GoalHeight = 244.0f;              // 2.44m crossbar height
    constexpr float GoalDepth = 200.0f;               // Net depth behind goal line
    constexpr float GoalPostRadius = 6.0f;            // 12cm diameter posts

    constexpr float CenterCircleRadius = 915.0f;      // 9.15m
    constexpr float PenaltySpotDistance = 1100.0f;     // 11m from goal line
    constexpr float CornerArcRadius = 100.0f;          // 1m

    constexpr float LineWidth = 12.0f;                 // 12cm line markings

    // Goal positions (center of goal mouth)
    // Pitch is centered at world origin (0,0,0)
    // X axis = length, Y axis = width, Z = up
    inline FVector HomeGoalCenter() { return FVector(-HalfLength, 0.0f, GoalHeight / 2.0f); }
    inline FVector AwayGoalCenter() { return FVector(HalfLength, 0.0f, GoalHeight / 2.0f); }

    // Convert normalized formation position (0-1, 0-1) to world position
    // For HOME team: own goal at -X, attacking toward +X
    inline FVector NormalizedToWorld_Home(FVector2D Normalized)
    {
        float X = FMath::Lerp(-HalfLength, HalfLength, Normalized.X);
        float Y = FMath::Lerp(-HalfWidth, HalfWidth, Normalized.Y);
        return FVector(X, Y, GamePlaneZ);
    }

    // For AWAY team: own goal at +X, attacking toward -X (mirror)
    inline FVector NormalizedToWorld_Away(FVector2D Normalized)
    {
        float X = FMath::Lerp(HalfLength, -HalfLength, Normalized.X);
        float Y = FMath::Lerp(HalfWidth, -HalfWidth, Normalized.Y);
        return FVector(X, Y, GamePlaneZ);
    }

    inline FVector NormalizedToWorld(FVector2D Normalized, ETeamId Team)
    {
        return (Team == ETeamId::Home) ? NormalizedToWorld_Home(Normalized) : NormalizedToWorld_Away(Normalized);
    }
}

// ============================================================
// BALL PHYSICS CONSTANTS (defaults, overridden by DataAsset)
// ============================================================
namespace BallPhysics
{
    constexpr float Mass = 0.430f;                     // kg (FIFA: 410-450g)
    constexpr float Radius = 11.0f;                    // cm (FIFA size 5: ~22cm diameter)
    constexpr float Restitution_Grass = 0.35f;
    constexpr float Restitution_Post = 0.75f;
    constexpr float Restitution_Net = 0.05f;
    constexpr float Friction_Grass = 0.45f;
    constexpr float Friction_Post = 0.30f;
    constexpr float Friction_Net = 0.90f;

    constexpr float LinearDamping = 0.3f;              // Approximates air resistance
    constexpr float AngularDamping = 0.15f;            // Spin decay

    constexpr float MagnusCoefficient = 0.00045f;      // Curve intensity
    constexpr float DragCoefficient = 0.25f;           // Cd for textured sphere
    constexpr float AirDensity = 0.00001225f;          // 1.225 kg/m³ → kg/cm³
    constexpr float CrossSectionArea = 380.13f;        // π * r² in cm²
    constexpr float RollingFrictionCoeff = 0.015f;     // Grass rolling resistance

    constexpr float GroundCheckDistance = 2.0f;        // cm, for ground detection
}

// ============================================================
// PLAYER MOVEMENT CONSTANTS (defaults)
// ============================================================
namespace PlayerMovement
{
    constexpr float JogSpeed = 550.0f;                 // cm/s (~20 km/h)
    constexpr float RunSpeed = 750.0f;                 // cm/s (~27 km/h)
    constexpr float SprintSpeed = 950.0f;              // cm/s (~34 km/h)
    constexpr float Acceleration = 2000.0f;            // cm/s²
    constexpr float Deceleration = 2400.0f;            // cm/s²
    constexpr float TurnRateStanding = 720.0f;         // deg/s
    constexpr float TurnRateSprinting = 280.0f;        // deg/s

    constexpr float MaxStamina = 100.0f;
    constexpr float SprintStaminaDrain = 8.0f;         // per second
    constexpr float JogStaminaDrain = 2.0f;
    constexpr float StaminaRecovery = 4.0f;            // per second, standing still

    constexpr float KickRange = 150.0f;                // cm, max dist to ball for kick
    constexpr float DribbleDistance = 80.0f;            // cm, ball offset when dribbling
    constexpr float HeaderHeight = 0.85f;              // fraction of capsule height
    constexpr float VolleyHeightMin = 0.10f;
    constexpr float VolleyHeightMax = 0.40f;

    constexpr float ShortPassPower = 800.0f;           // cm/s
    constexpr float DrivenPassPower = 1400.0f;
    constexpr float LobPassPower = 1000.0f;
    constexpr float ThroughBallPower = 1200.0f;
    constexpr float ShotPowerMin = 1500.0f;
    constexpr float ShotPowerMax = 3500.0f;            // ~126 km/h
    constexpr float CrossPower = 1100.0f;

    constexpr float CapsuleRadius = 34.0f;             // cm
    constexpr float CapsuleHalfHeight = 94.0f;         // cm (188cm total)
}
