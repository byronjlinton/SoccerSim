#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SoccerAnimInstance.generated.h"

class ASoccerPlayerPawn;

/**
 * Animation instance for SoccerPlayerPawn.
 * Provides locomotion variables for blendspace-driven animation blueprints.
 * 
 * USAGE IN ANIMATION BLUEPRINT:
 * 1. Create ABP_SoccerPlayer with this as parent class
 * 2. In Anim Graph, use MovementSpeed and MovementDirectionDegrees for blendspace
 * 3. Use bIsSprinting to transition to sprint animations
 * 4. Use bHasBall to switch between dribble/normal locomotion
 */
UCLASS()
class SOCCERSIM_API USoccerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    /** Current movement speed in cm/s (0-600+). Use for blendspace vertical axis. */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    float MovementSpeed = 0.0f;

    /** Normalized movement direction in local space (-1 to 1). X=Forward/Back, Y=Left/Right. */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    FVector2D MovementDirection = FVector2D::ZeroVector;

    /** Movement direction in degrees (-180 to 180). 0=Forward, 90=Right, -90=Left, 180=Backward. Use for blendspace horizontal axis. */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    float MovementDirectionDegrees = 0.0f;

    /** True when player is sprinting (speed > 500 and stamina available). */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    bool bIsSprinting = false;

    /** True when player has possession of the ball. */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    bool bHasBall = false;

    /** True when player is moving (speed > threshold). */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    bool bIsMoving = false;

    /** Velocity magnitude normalized to 0-1 range based on max sprint speed. */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    float VelocityBlendWeight = 0.0f;

    /** 2D velocity for blendspace (forward/back, strafe left/right). */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    FVector2D BlendSpaceInput = FVector2D::ZeroVector;

    /** Time spent in current state (useful for transition rules). */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    float TimeInState = 0.0f;

    /** Previous frame's bHasBall for detecting ball acquisition events. */
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    bool bHadBallLastFrame = false;

protected:
    UPROPERTY(Transient)
    TWeakObjectPtr<ASoccerPlayerPawn> SoccerPawn;

    /** Maximum speed for normalization (cm/s). Adjust if sprint speed changes. */
    UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Config")
    float MaxSpeedForNormalization = 800.0f;

    /** Speed threshold to consider player as "moving". */
    UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Config")
    float MovingSpeedThreshold = 10.0f;
};
