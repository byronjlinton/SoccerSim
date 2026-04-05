#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerTeamBrain.generated.h"

class ASoccerPlayerPawn;
class ASoccerBall;
class USoccerUtilityEvaluator;

// ============================================================
// Team Tactical State
// ============================================================

UENUM(BlueprintType)
enum class ETeamPhase : uint8
{
    Defending             UMETA(DisplayName = "Defending"),
    DefendingTransition   UMETA(DisplayName = "Defensive Transition"),
    AttackingTransition   UMETA(DisplayName = "Attacking Transition"),
    Attacking             UMETA(DisplayName = "Attacking"),
    SetPieceDefensive     UMETA(DisplayName = "Set Piece (Defensive)"),
    SetPieceAttacking     UMETA(DisplayName = "Set Piece (Attacking)")
};

UENUM(BlueprintType)
enum class ETeamPressState : uint8
{
    NoPress       UMETA(DisplayName = "No Press"),
    SoftPress     UMETA(DisplayName = "Soft Press"),
    HardPress     UMETA(DisplayName = "Hard Press"),
    CounterPress  UMETA(DisplayName = "Counter-Press (Gegenpressing)")
};

// ============================================================
// Man-Marking Assignment
// ============================================================

USTRUCT()
struct FMarkingAssignment
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<ASoccerPlayerPawn> Marker = nullptr;

    UPROPERTY()
    TObjectPtr<ASoccerPlayerPawn> Target = nullptr;

    /** How tightly to mark (0 = loose/zonal, 1 = tight man-mark) */
    UPROPERTY()
    float Tightness = 0.5f;
};

// ============================================================
// Soccer Team Brain
// ============================================================

UCLASS(BlueprintType)
class SOCCERSIM_API USoccerTeamBrain : public UObject
{
    GENERATED_BODY()

public:
    USoccerTeamBrain();

    /** Initialize for a specific team */
    void Initialize(ETeamId Team);

    /** Main update tick - call from GameState Tick */
    void Update(float DeltaTime, const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball);

    // ── Team Phase Accessors ──

    ETeamPhase GetTeamPhase() const { return CurrentPhase; }
    ETeamPressState GetPressState() const { return PressState; }
    ETeamId GetTeamId() const { return TeamId; }

    /** Get the shifted formation position for a specific player (includes ball-attracted shift + team phase adjustments) */
    FVector GetShiftedFormationPosition(const ASoccerPlayerPawn* Player, ASoccerBall* Ball) const;

    /** Check if a specific player should be pressing */
    bool ShouldPlayerPress(const ASoccerPlayerPawn* Player) const;

    /** Get man-marking target for a player (nullptr = no assignment) */
    ASoccerPlayerPawn* GetMarkTarget(const ASoccerPlayerPawn* Player) const;

    /** Is this team in the counter-attack phase? */
    bool IsCounterAttacking() const { return CurrentPhase == ETeamPhase::AttackingTransition && PressState == ETeamPressState::CounterPress; }

    // ── Configuration ──

    UPROPERTY(EditDefaultsOnly, Category = "AI|Team")
    FTacticalSettings Tactics;

    /** How many players can press simultaneously */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Pressing")
    int32 MaxPressingPlayers = 3;

    /** Distance threshold (cm) for pressing activation - ball must be in this zone */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Pressing")
    float PressingActivationRadius = 3000.0f;

    /** Ball distance to own goal that triggers emergency defending */
    UPROPERTY(EditDefaultsOnly, Category = "AI|Defending")
    float EmergencyDefendingDistance = 2500.0f;

private:
    UPROPERTY()
    ETeamId TeamId = ETeamId::None;

    UPROPERTY()
    ETeamPhase CurrentPhase = ETeamPhase::Defending;

    UPROPERTY()
    ETeamPressState PressState = ETeamPressState::NoPress;

    UPROPERTY()
    TArray<FMarkingAssignment> MarkingAssignments;

    /** Indexes of players currently assigned to press */
    TArray<int32> PressingPlayerIndices;

    /** Timers */
    float PhaseTransitionTimer = 0.0f;
    float CounterAttackTimer = 0.0f;

    // ── Internal Methods ──

    void UpdateTeamPhase(float DeltaTime, const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                         const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball);
    void UpdatePressingState(const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                             const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball);
    void AssignPressingRoles(const TArray<ASoccerPlayerPawn*>& TeamPlayers, ASoccerBall* Ball);
    void UpdateMarkingAssignments(const TArray<ASoccerPlayerPawn*>& TeamPlayers,
                                  const TArray<ASoccerPlayerPawn*>& OpponentPlayers, ASoccerBall* Ball);

    /** Is the ball in our defensive third? */
    bool IsBallInDefensiveThird(ASoccerBall* Ball) const;

    /** Is the ball in our attacking third? */
    bool IsBallInAttackingThird(ASoccerBall* Ball) const;

    /** Does our team have the ball? */
    bool DoesTeamHaveBall(ASoccerBall* Ball) const;
};
