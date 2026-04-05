#include "SoccerAnimInstance.h"
#include "SoccerSim/Player/SoccerPlayerPawn.h"
#include "GameFramework/CharacterMovementComponent.h"

void USoccerAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    if (APawn* Pawn = TryGetPawnOwner())
    {
        SoccerPawn = Cast<ASoccerPlayerPawn>(Pawn);
    }
}

void USoccerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // Cache previous frame's ball state
    bHadBallLastFrame = bHasBall;

    if (!SoccerPawn.IsValid())
    {
        if (APawn* Pawn = TryGetPawnOwner())
        {
            SoccerPawn = Cast<ASoccerPlayerPawn>(Pawn);
        }
        if (!SoccerPawn.IsValid()) return;
    }

    ASoccerPlayerPawn* Pawn = SoccerPawn.Get();
    if (!Pawn) return;

    // Get velocity and calculate speed
    FVector Velocity = Pawn->GetVelocity();
    Velocity.Z = 0.0f;
    MovementSpeed = Velocity.Size();
    
    // Check if moving
    bIsMoving = MovementSpeed > MovingSpeedThreshold;

    // Update time in state
    if (bIsMoving)
    {
        TimeInState += DeltaSeconds;
    }
    else
    {
        TimeInState = 0.0f;
    }

    // Calculate normalized velocity blend weight (0-1)
    VelocityBlendWeight = FMath::Clamp(MovementSpeed / MaxSpeedForNormalization, 0.0f, 1.0f);

    // Calculate movement direction in local space
    if (MovementSpeed > MovingSpeedThreshold)
    {
        FVector Forward = Pawn->GetActorForwardVector();
        Forward.Z = 0.0f;
        Forward.Normalize();
        
        FVector Right = Pawn->GetActorRightVector();
        Right.Z = 0.0f;
        Right.Normalize();
        
        FVector VelDir = Velocity.GetSafeNormal();

        // Local space direction (-1 to 1)
        MovementDirection.X = FVector::DotProduct(Forward, VelDir);  // Forward/Back
        MovementDirection.Y = FVector::DotProduct(Right, VelDir);    // Left/Right

        // Convert to degrees (-180 to 180)
        // 0 = Forward, 90 = Right, -90 = Left, 180/-180 = Backward
        MovementDirectionDegrees = FMath::RadiansToDegrees(FMath::Atan2(MovementDirection.Y, MovementDirection.X));

        // Blendspace input: use raw velocity mapped to local space
        // This gives a more natural blend for 8-directional movement
        BlendSpaceInput.X = MovementDirection.X * VelocityBlendWeight;
        BlendSpaceInput.Y = MovementDirection.Y * VelocityBlendWeight;
    }
    else
    {
        MovementDirection = FVector2D::ZeroVector;
        MovementDirectionDegrees = 0.0f;
        BlendSpaceInput = FVector2D::ZeroVector;
    }

    // Sprinting state
    bIsSprinting = Pawn->GetCurrentStamina() > 0.0f && 
                    Pawn->IsSprinting() && 
                    MovementSpeed > 500.0f;

    // Ball possession state
    bHasBall = Pawn->HasBall();
}
