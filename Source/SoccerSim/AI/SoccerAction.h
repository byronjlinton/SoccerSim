#pragma once

#include "CoreMinimal.h"
#include "SoccerSim/Utils/SoccerSimTypes.h"
#include "SoccerAction.generated.h"

// ============================================================
// AI Action Types
// ============================================================

UENUM(BlueprintType)
enum class ESoccerActionType : uint8
{
    Pass            UMETA(DisplayName = "Pass"),
    Shot            UMETA(DisplayName = "Shot"),
    Dribble         UMETA(DisplayName = "Dribble"),
    Tackle          UMETA(DisplayName = "Tackle"),
    HoldPosition    UMETA(DisplayName = "Hold Position"),
    MakeRun         UMETA(DisplayName = "Make Run"),
    Press           UMETA(DisplayName = "Press"),
    CoverLane       UMETA(DisplayName = "Cover Lane"),
    Cross           UMETA(DisplayName = "Cross"),
    Header          UMETA(DisplayName = "Header"),
    Clear           UMETA(DisplayName = "Clear Ball"),
    TrackRunner     UMETA(DisplayName = "Track Runner")
};

// ============================================================
// Action Evaluation Result
// ============================================================

USTRUCT(BlueprintType)
struct FSoccerAction
{
    GENERATED_BODY()

    /** Type of action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESoccerActionType Type = ESoccerActionType::HoldPosition;

    /** Utility score (0-1). Higher = more desirable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Score = 0.0f;

    /** Target location for movement-based actions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    /** Target teammate for pass actions (index into team array) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetTeammateIndex = -1;

    /** Kick type for pass/shot actions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EKickType KickType = EKickType::ShortPass;

    /** Kick power multiplier (0-1, scaled to actual power) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PowerRatio = 0.5f;

    bool IsValid() const { return Score > 0.0f; }
};

// ============================================================
// Utility Response Curve
// ============================================================

USTRUCT(BlueprintType)
struct FSoccerUtilityCurve
{
    GENERATED_BODY()

    /** Input values (X-axis). Must be sorted ascending. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<float> InputKeys;

    /** Output values (Y-axis). Maps to InputKeys by index. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<float> OutputValues;

    FSoccerUtilityCurve()
    {
        // Default: linear response
        InputKeys = { 0.0f, 1.0f };
        OutputValues = { 0.0f, 1.0f };
    }

    /** Evaluate the curve at a given input value. Linear interpolation between key points. */
    float Evaluate(float Input) const
    {
        if (InputKeys.Num() == 0 || OutputValues.Num() == 0) return 0.0f;
        if (InputKeys.Num() != OutputValues.Num()) return 0.0f;
        if (InputKeys.Num() == 1) return OutputValues[0];

        float ClampedInput = FMath::Clamp(Input, InputKeys[0], InputKeys.Last());

        // Find the segment
        for (int32 i = 0; i < InputKeys.Num() - 1; i++)
        {
            if (ClampedInput >= InputKeys[i] && ClampedInput <= InputKeys[i + 1])
            {
                float Range = InputKeys[i + 1] - InputKeys[i];
                if (Range < KINDA_SMALL_NUMBER) return OutputValues[i];

                float Alpha = (ClampedInput - InputKeys[i]) / Range;
                return FMath::Lerp(OutputValues[i], OutputValues[i + 1], Alpha);
            }
        }
        return OutputValues.Last();
    }

    // Preset curves
    static FSoccerUtilityCurve Linear() { return FSoccerUtilityCurve(); }

    static FSoccerUtilityCurve Exponential()
    {
        FSoccerUtilityCurve Curve;
        Curve.InputKeys = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        Curve.OutputValues = { 0.0f, 0.06f, 0.25f, 0.56f, 1.0f };
        return Curve;
    }

    static FSoccerUtilityCurve Logarithmic()
    {
        FSoccerUtilityCurve Curve;
        Curve.InputKeys = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        Curve.OutputValues = { 0.0f, 0.5f, 0.75f, 0.9f, 1.0f };
        return Curve;
    }

    static FSoccerUtilityCurve Inverse()
    {
        FSoccerUtilityCurve Curve;
        Curve.InputKeys = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        Curve.OutputValues = { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f };
        return Curve;
    }

    static FSoccerUtilityCurve Threshold(float ThresholdValue = 0.5f)
    {
        FSoccerUtilityCurve Curve;
        Curve.InputKeys = { 0.0f, ThresholdValue - 0.01f, ThresholdValue, 1.0f };
        Curve.OutputValues = { 0.0f, 0.0f, 1.0f, 1.0f };
        return Curve;
    }
};
