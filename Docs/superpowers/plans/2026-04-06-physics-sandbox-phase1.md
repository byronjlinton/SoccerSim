# Physics Sandbox Phase 1 — Nuclear Cleanup + Standing Ragdoll

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Strip the SoccerSim codebase down to a physics sandbox — one active ragdoll pawn on a ground plane, controlled by WASD/Shift. Everything else deleted.

**Architecture:** Keep `SoccerPhysicsPawn`, `PhysicsLocomotionComponent`, and `PhysicsMuscleComponent` unchanged. Rewrite `SoccerGameMode` and `SoccerPlayerController` to bare minimums. Delete all other modules. Update configs for new sandbox map.

**Tech Stack:** UE5.7 C++, Chaos physics, Enhanced Input system

---

## File Structure After Cleanup

```
Source/SoccerSim/
  SoccerSim.h                          — Log category (unchanged)
  SoccerSim.cpp                        — Module (unchanged)
  SoccerSim.Build.cs                   — Stripped dependencies
  Core/
    SoccerGameMode.h/.cpp              — Rewritten: spawns one PhysicsPawn only
    SoccerPlayerController.h/.cpp      — Rewritten: move + sprint input only
    SoccerGameState.h/.cpp             — DELETED
    SoccerGameInstance.h/.cpp          — DELETED
  Player/
    SoccerPhysicsPawn.h/.cpp           — UNCHANGED
    PhysicsLocomotionComponent.h/.cpp  — UNCHANGED
    PhysicsMuscleComponent.h/.cpp      — UNCHANGED
    SoccerPlayerPawn.h/.cpp            — DELETED
  Physics/                             — Keep (empty, .gitkeep)
  Input/                               — Keep (empty, .gitkeep)

DELETED DIRECTORIES:
  AI/, Animation/, Ball/, Camera/, Data/, Field/, Match/, UI/, Utils/
```

---

### Task 1: Delete soccer modules

**Files:**
- Delete: `Source/SoccerSim/AI/` (all 10 files)
- Delete: `Source/SoccerSim/Animation/` (all 4 files)
- Delete: `Source/SoccerSim/Ball/` (all 2 files)
- Delete: `Source/SoccerSim/Camera/` (all 2 files)
- Delete: `Source/SoccerSim/Data/` (all 2 files)
- Delete: `Source/SoccerSim/Field/` (all 4 files)
- Delete: `Source/SoccerSim/Match/` (if exists)
- Delete: `Source/SoccerSim/UI/` (all 2 files)
- Delete: `Source/SoccerSim/Utils/` (SoccerSimTypes.h)
- Delete: `Source/SoccerSim/Player/SoccerPlayerPawn.h`
- Delete: `Source/SoccerSim/Player/SoccerPlayerPawn.cpp`
- Delete: `Source/SoccerSim/Core/SoccerGameState.h`
- Delete: `Source/SoccerSim/Core/SoccerGameState.cpp`
- Delete: `Source/SoccerSim/Core/SoccerGameInstance.h`
- Delete: `Source/SoccerSim/Core/SoccerGameInstance.cpp`

- [ ] **Step 1: Delete all soccer module directories**

```bash
cd /c/Users/byron/Cursor/SoccerSim
rm -rf Source/SoccerSim/AI/
rm -rf Source/SoccerSim/Animation/
rm -rf Source/SoccerSim/Ball/
rm -rf Source/SoccerSim/Camera/
rm -rf Source/SoccerSim/Data/
rm -rf Source/SoccerSim/Field/
rm -rf Source/SoccerSim/UI/
rm -rf Source/SoccerSim/Utils/
rm -f Source/SoccerSim/Player/SoccerPlayerPawn.h
rm -f Source/SoccerSim/Player/SoccerPlayerPawn.cpp
rm -f Source/SoccerSim/Core/SoccerGameState.h
rm -f Source/SoccerSim/Core/SoccerGameState.cpp
rm -f Source/SoccerSim/Core/SoccerGameInstance.h
rm -f Source/SoccerSim/Core/SoccerGameInstance.cpp
# Check for Match directory
if [ -d "Source/SoccerSim/Match" ]; then rm -rf Source/SoccerSim/Match/; fi
```

- [ ] **Step 2: Verify only expected files remain**

```bash
find Source/SoccerSim -name "*.h" -o -name "*.cpp" | sort
```

Expected output:
```
Source/SoccerSim/Core/SoccerGameMode.cpp
Source/SoccerSim/Core/SoccerGameMode.h
Source/SoccerSim/Core/SoccerPlayerController.cpp
Source/SoccerSim/Core/SoccerPlayerController.h
Source/SoccerSim/Player/PhysicsLocomotionComponent.cpp
Source/SoccerSim/Player/PhysicsLocomotionComponent.h
Source/SoccerSim/Player/PhysicsMuscleComponent.cpp
Source/SoccerSim/Player/PhysicsMuscleComponent.h
Source/SoccerSim/Player/SoccerPhysicsPawn.cpp
Source/SoccerSim/Player/SoccerPhysicsPawn.h
Source/SoccerSim/SoccerSim.cpp
Source/SoccerSim/SoccerSim.h
```

- [ ] **Step 3: Commit the deletion**

```bash
git add -A
git commit -m "Nuclear cleanup: delete all soccer modules (AI, Ball, Field, Camera, UI, etc)"
```

---

### Task 2: Strip SoccerGameMode to sandbox

**Files:**
- Rewrite: `Source/SoccerSim/Core/SoccerGameMode.h`
- Rewrite: `Source/SoccerSim/Core/SoccerGameMode.cpp`

The old GameMode spawned fields, balls, teams, cameras, stadium geometry, post-processing, kickoff timers, AI controllers, and 22 players. The new one does one thing: spawn a single PhysicsPawn and let the player controller possess it.

- [ ] **Step 1: Rewrite SoccerGameMode.h**

Replace entire file with:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoccerGameMode.generated.h"

class ASoccerPhysicsPawn;

UCLASS()
class SOCCERSIM_API ASoccerGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASoccerGameMode();

    virtual void StartPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Sandbox|Config")
    TSubclassOf<APawn> PhysicsPawnClass;
};
```

- [ ] **Step 2: Rewrite SoccerGameMode.cpp**

Replace entire file with:

```cpp
#include "SoccerGameMode.h"
#include "SoccerPlayerController.h"
#include "SoccerSim/Player/SoccerPhysicsPawn.h"
#include "SoccerSim/SoccerSim.h"
#include "Kismet/GameplayStatics.h"

ASoccerGameMode::ASoccerGameMode()
{
    PlayerControllerClass = ASoccerPlayerController::StaticClass();
    DefaultPawnClass = ASoccerPhysicsPawn::StaticClass();
}

void ASoccerGameMode::StartPlay()
{
    Super::StartPlay();

    UE_LOG(LogSoccerSim, Log, TEXT("SoccerGameMode::StartPlay - Physics sandbox"));
}
```

- [ ] **Step 3: Verify it compiles mentally — no references to deleted modules**

The new GameMode has zero references to: SoccerBall, SoccerField, SoccerGoal, SoccerPlayerPawn, SoccerBroadcastCamera, SoccerAIController, SoccerGameState, SoccerSimTypes, formations, teams, match phases. It only references SoccerPhysicsPawn and SoccerPlayerController.

---

### Task 3: Strip SoccerPlayerController to move + sprint

**Files:**
- Rewrite: `Source/SoccerSim/Core/SoccerPlayerController.h`
- Rewrite: `Source/SoccerSim/Core/SoccerPlayerController.cpp`

The old controller handled: move, sprint, pass, shoot (with charging), through ball, tackle, switch player, lob, view toggle, pause menu, match end widget, HUD creation, player switching, camera toggling. The new one handles: move + sprint. It routes input to whatever pawn it possesses (which is ASoccerPhysicsPawn).

- [ ] **Step 1: Rewrite SoccerPlayerController.h**

Replace entire file with:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "SoccerPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ASoccerPhysicsPawn;

UCLASS()
class SOCCERSIM_API ASoccerPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASoccerPlayerController();

    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Move;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Sprint;

private:
    void HandleMove(const FInputActionValue& Value);
    void HandleSprintStarted(const FInputActionValue& Value);
    void HandleSprintCompleted(const FInputActionValue& Value);

    ASoccerPhysicsPawn* GetPhysicsPawn() const;
};
```

- [ ] **Step 2: Rewrite SoccerPlayerController.cpp**

Replace entire file with:

```cpp
#include "SoccerPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SoccerSim/Player/SoccerPhysicsPawn.h"
#include "SoccerSim/SoccerSim.h"

ASoccerPlayerController::ASoccerPlayerController()
{
}

void ASoccerPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
        else
        {
            UE_LOG(LogSoccerSim, Warning, TEXT("DefaultMappingContext is not set on PlayerController!"));
        }
    }
}

void ASoccerPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EIC)
    {
        UE_LOG(LogSoccerSim, Error, TEXT("Failed to get EnhancedInputComponent!"));
        return;
    }

    if (IA_Move)
        EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASoccerPlayerController::HandleMove);
    if (IA_Sprint)
    {
        EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ASoccerPlayerController::HandleSprintStarted);
        EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ASoccerPlayerController::HandleSprintCompleted);
    }
}

ASoccerPhysicsPawn* ASoccerPlayerController::GetPhysicsPawn() const
{
    return Cast<ASoccerPhysicsPawn>(GetPawn());
}

void ASoccerPlayerController::HandleMove(const FInputActionValue& Value)
{
    FVector2D MovementInput = Value.Get<FVector2D>();
    if (ASoccerPhysicsPawn* Pawn = GetPhysicsPawn())
    {
        Pawn->SetMovementInput(MovementInput);
    }
}

void ASoccerPlayerController::HandleSprintStarted(const FInputActionValue& Value)
{
    if (ASoccerPhysicsPawn* Pawn = GetPhysicsPawn())
    {
        Pawn->SetSprinting(true);
    }
}

void ASoccerPlayerController::HandleSprintCompleted(const FInputActionValue& Value)
{
    if (ASoccerPhysicsPawn* Pawn = GetPhysicsPawn())
    {
        Pawn->SetSprinting(false);
    }
}
```

---

### Task 4: Update Build.cs dependencies

**Files:**
- Modify: `Source/SoccerSim/SoccerSim.Build.cs`

Remove module dependencies that are no longer needed after deleting soccer modules.

- [ ] **Step 1: Rewrite SoccerSim.Build.cs**

Replace entire file with:

```csharp
using UnrealBuildTool;

public class SoccerSim : ModuleRules
{
    public SoccerSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "PhysicsCore",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "ChaosCore",
            "ChaosSolverEngine"
        });

        bEnableExceptions = true;
    }
}
```

Removed: AIModule, GameplayTasks, NavigationSystem, Niagara, UMG, GameplayTags, GeometryFramework, GeometryCore, DynamicMesh.

---

### Task 5: Update config files

**Files:**
- Modify: `Config/DefaultEngine.ini`
- Modify: `Config/DefaultGame.ini`

- [ ] **Step 1: Update DefaultEngine.ini**

Replace the entire file with:

```ini
[/Script/EngineSettings.GameMapsSettings]
GlobalDefaultGameMode=/Script/SoccerSim.SoccerGameMode
GameDefaultMap=/Game/Maps/Sandbox
EditorStartupMap=/Game/Maps/Sandbox

[/Script/Engine.CollisionProfile]
+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,DefaultResponse=ECR_Block,bTraceType=False,bStaticObject=False,Name="PlayerBody")
+Profiles=(Name="PhysicsActor",CollisionEnabled=QueryAndPhysics,bCanModify=True,ObjectTypeName="PhysicsBody",CustomResponses=,HelpMessage="Physics ragdoll bodies")
+Profiles=(Name="SoccerPlayer",CollisionEnabled=QueryAndPhysics,bCanModify=True,ObjectTypeName="PlayerBody",CustomResponses=,HelpMessage="Game-level capsule collision")

[/Script/Engine.PhysicsSettings]
DefaultGravityZ=-981.0
bEnableCCD=True
MaxPhysicsDeltaTime=0.016667
bSubstepping=True
MaxSubstepDeltaTime=0.008333
MaxSubsteps=4

[/Script/Engine.RendererSettings]
r.GPUSkin.Support16BitBoneIndex=True
r.GPUSkin.UnlimitedBoneInfluences=True
r.SkinCache.CompileShaders=True
SkeletalMesh.UseExperimentalChunking=1
r.GenerateMeshDistanceFields=True

[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_Default
-D3D12TargetedShaderFormats=PCD3D_SM5
+D3D12TargetedShaderFormats=PCD3D_SM5
+D3D12TargetedShaderFormats=PCD3D_SM6
-D3D11TargetedShaderFormats=PCD3D_SM5
+D3D11TargetedShaderFormats=PCD3D_SM5
bGenerateNaniteFallbackMeshes=True
Compiler=Default
AudioSampleRate=48000
AudioCallbackBufferFrameSize=1024
AudioNumBuffersToEnqueue=1
AudioMaxChannels=0
AudioNumSourceWorkers=4
SpatializationPlugin=
SourceDataOverridePlugin=
ReverbPlugin=
OcclusionPlugin=
CompressionOverrides=(bOverrideCompressionTimes=False,DurationThreshold=5.000000,MaxNumRandomBranches=0,SoundCueQualityIndex=0)
CacheSizeKB=65536
MaxChunkSizeOverrideKB=0
bResampleForDevice=False
MaxSampleRate=48000.000000
HighSampleRate=32000.000000
MedSampleRate=24000.000000
LowSampleRate=12000.000000
MinSampleRate=8000.000000
CompressionQualityModifier=1.000000
AutoStreamingThreshold=0.000000
SoundCueQualityIndex=-1
```

Key changes: Removed Ball, FieldBoundary, GoalTrigger collision channels/profiles. Changed game mode from BP to C++ class. Changed map to `/Game/Maps/Sandbox`. Kept physics substepping and renderer settings.

- [ ] **Step 2: Update DefaultGame.ini**

Replace entire file with:

```ini
[/Script/EngineSettings.GameMapsSettings]
GlobalDefaultGameMode=/Script/SoccerSim.SoccerGameMode
```

Removed all SoccerPlayerPawn, SoccerBall, SoccerField, SoccerGameInstance config sections.

---

### Task 6: Build and verify compilation

- [ ] **Step 1: Build the project**

```bash
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Build/BatchFiles/Build.bat" \
  SoccerSimEditor Win64 Development "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject" -waitmutex
```

Expected: Build succeeds with 0 errors. There may be warnings about unused variables in the physics pawn code (harmless).

If build fails:
- Check for any `#include` referencing deleted modules (e.g., `SoccerSimTypes.h`, `SoccerBall.h`, etc.) in the surviving files
- The surviving files (SoccerPhysicsPawn, PhysicsLocomotionComponent, PhysicsMuscleComponent) only include each other + UE framework headers + `SoccerSim.h`, so they should be clean
- The rewritten GameMode and PlayerController only include `SoccerPhysicsPawn.h` + UE headers + `SoccerSim.h`

- [ ] **Step 2: Commit the stripped core**

```bash
git add -A
git commit -m "Strip GameMode and PlayerController to sandbox: spawn one physics pawn, move + sprint input only"
```

---

### Task 7: Create Sandbox map (in-editor)

The Sandbox map needs: a ground plane, a directional light, a sky light, a PlayerStart, and the physics pawn will auto-spawn via the GameMode's DefaultPawnClass.

This must be done in the UE editor (maps are .uasset binary files that can't be created from code). The map needs to be saved as `/Game/Maps/Sandbox`.

- [ ] **Step 1: Launch the editor**

```bash
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe" \
  "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject"
```

- [ ] **Step 2: Create the Sandbox map**

In the editor:
1. File → New Level → Empty Level
2. Add a Floor: drag a cube from Place Actors, scale to (50, 50, 1), position at (0, 0, -50). This creates a 50m x 50m ground plane.
3. Set the floor mesh collision to BlockAll
4. Add a Directional Light from Place Actors, rotate to (-50, 30, 0) for pleasant lighting
5. Add a Sky Light from Place Actors
6. Add an Exponential Height Fog from Place Actors
7. Add a PlayerStart from Place Actors, position at (0, 0, 100)
8. Save as `/Game/Maps/Sandbox`

- [ ] **Step 3: Verify PIE spawns the physics pawn**

Press Play. The GameMode should:
1. Spawn an ASoccerPhysicsPawn at the PlayerStart location
2. PlayerController possesses it
3. The pawn should appear (Dribble mesh) and physics should initialize

If the pawn falls through the floor immediately: check that the ground plane collision blocks PhysicsBody (the ragdoll body collision channel). If the pawn doesn't appear: check Output Log for `LogSoccerSim: SoccerGameMode::StartPlay - Physics sandbox` and any errors from SoccerPhysicsPawn::InitPhysics.

- [ ] **Step 4: Commit the new map**

```bash
git add Content/Maps/Sandbox.uasset
git commit -m "Add Sandbox map: ground plane, lighting, PlayerStart"
```

---

### Task 8: Fix content references and verify PIE

The physics pawn has hardcoded constructor references to the Dribble mesh and physics asset. These should still work if those assets exist. Verify they load.

- [ ] **Step 1: Check that Dribble mesh and physics asset still exist**

```bash
find Content -name "Dribble*" -o -name "BS_Locomotion*" | sort
```

Expected: The Dribble skeletal mesh, Dribble_PhysicsAsset, and possibly other Dribble-related assets should still be present since we only deleted source code, not content assets.

- [ ] **Step 2: Delete soccer-specific content assets**

```bash
# These are no longer needed:
rm -rf Content/Blueprints/BP_Soccer*
rm -rf Content/Blueprints/WBP_Soccer*
# Keep BP_SoccerPhysicsPawn if it exists — it may be needed
# Don't delete animation assets — the physics pawn references them
```

- [ ] **Step 3: Launch editor and test PIE**

```bash
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe" \
  "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject"
```

In PIE:
1. The Sandbox map loads
2. A ragdoll pawn spawns at PlayerStart
3. The pawn should stand (or attempt to stand) — physics drives it
4. Check Output Log for errors

- [ ] **Step 4: Commit cleanup**

```bash
git add -A
git commit -m "Clean up unused content blueprints, verify physics pawn spawns in Sandbox map"
```

---

### Task 9: Verify WASD input works

- [ ] **Step 1: In PIE, press WASD**

The PlayerController routes IA_Move to `SoccerPhysicsPawn::SetMovementInput()`, which forwards to `PhysicsLocomotionComponent`. The root drive should apply forces to the pelvis.

Expected behaviors:
- W: force pushes pelvis forward (positive X)
- S: force pushes pelvis backward
- A/D: force pushes pelvis sideways
- Shift: increases target speed from JogSpeed (400) to SprintSpeed (750)

If nothing moves:
1. Check Output Log for "PhysicsLocomotion: Pelvis bone 'Hips' not found" — the physics asset may use a different bone name
2. Verify IA_Move and IA_Sprint are set in the PlayerController's blueprint defaults (or the BP_SoccerPlayerController if it still exists)
3. If using the C++ PlayerController directly (no BP), the IA_Move/IA_Sprint pointers will be null — you need to either create a BP child or set them via the editor

**Critical note about input actions:** The Enhanced Input assets (IA_Move, IA_Sprint, IM_Default) are uasset files in Content/. They should still exist. The PlayerController's `DefaultMappingContext`, `IA_Move`, and `IA_Sprint` UPROPERTY pointers need to be set. Two options:
- **Option A (recommended):** Create a new `BP_SandboxPlayerController` Blueprint based on `ASoccerPlayerController`, set the input assets in its defaults, update DefaultEngine.ini game mode to reference the BP
- **Option B:** Set them via C++ constructor with `ConstructorHelpers::FObjectFinder`

- [ ] **Step 2: Commit when input works**

```bash
git add -A
git commit -m "Verify WASD input drives physics pawn root locomotion"
```

---

### Task 10: Update CLAUDE.md for new project state

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update CLAUDE.md to reflect the sandbox state**

Replace the entire CLAUDE.md with content that reflects the new physics sandbox project. Key changes:
- Remove all soccer references (roadmap phases A-E, verification protocol for soccer, project structure)
- Document the physics sandbox: one ragdoll, ground plane, WASD control
- Keep build/deploy commands
- Keep MCP bridge docs (UnrealClaude, soft-ue-cli)
- Keep UE5 coding conventions
- Keep known quirks that still apply
- Update project structure to show only surviving modules
- New verification protocol: spawn pawn, check standing stability, test input

---

## Self-Review

**Spec coverage check:**
- Nuclear cleanup of all soccer modules → Task 1
- Strip GameMode → Task 2
- Strip PlayerController → Task 3
- Update Build.cs → Task 4
- Update configs → Task 5
- Build verification → Task 6
- Create Sandbox map → Task 7
- Verify content references → Task 8
- Verify WASD input → Task 9
- Update docs → Task 10

**Placeholder scan:** No TBDs, TODOs, or "implement later" patterns. All code is complete.

**Type consistency:** `ASoccerPhysicsPawn` is consistently referenced. `GetPhysicsPawn()` returns `ASoccerPhysicsPawn*`. `SetMovementInput(FVector2D)` and `SetSprinting(bool)` match the physics pawn's interface.
