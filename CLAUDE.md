# SoccerSim — UE5.7 C++ Soccer Game

## Build & Deploy

```bash
# Build (from project root, Git Bash)
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Build/BatchFiles/Build.bat" \
  SoccerSimEditor Win64 Development "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject" -waitmutex

# Launch editor
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe" \
  "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject"

# Kill editor (when PIE won't stop)
taskkill //F //IM UnrealEditor.exe
```

## soft-ue-cli (MCP Bridge)

- **CLI path**: `C:\Users\byron\AppData\Roaming\Python\Python314\Scripts\soft-ue-cli.exe`
- **Port**: 8080 (SoftUEBridge plugin in UE editor)
- **Critical**: Set `MSYS_NO_PATHCONV=1` when calling soft-ue-cli from Git Bash
- **MCP tools**: Some have parameter parsing bugs — fall back to bash if MCP tool fails
- **PIE auto-starts**: `GameDefaultMap=/Game/Maps/Match` in DefaultEngine.ini
- **Asset creation**: Must stop PIE first (can't create assets while PIE is running)
- **Viewport capture**: Does NOT capture UMG/HUD overlays — only the 3D scene

## UE5 Coding Conventions

- **Prefixes**: A (Actors), U (UObjects/Components), F (structs), E (enums), I (interfaces)
- Every exposed property: `UPROPERTY()` with appropriate specifiers
- Every exposed function: `UFUNCTION()` with appropriate specifiers
- Headers use forward declarations; .cpp uses `#include`
- `#pragma once` in all headers, `GENERATED_BODY()` in all classes
- Components created in constructor with `CreateDefaultSubobject<T>()`
- Use `FName` for identifiers, `FText` for UI, `FString` for general strings
- Enhanced Input System (not legacy input)
- All magic numbers extracted to `UPROPERTY(EditDefaultsOnly)` or constants

## Known UE5.7 Quirks

- **DefaultLitMaterial has NO parameters**: `SetVectorParameterValue("BaseColor", ...)` silently does nothing. Use `/Game/Art/M_DynamicColor` (custom material with VectorParameter "BaseColor" connected to Base Color output).
- **Collision profiles need explicit channel responses**: The "Ball" profile in DefaultEngine.ini doesn't implicitly block ECC_WorldStatic. Added `SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block)` in C++ constructor.
- **Blueprint EditDefaultsOnly properties**: C++ base classes declare UPROPERTY but set no defaults. BP child classes must have values set via `set-asset-property`.

## Project Structure

```
Source/SoccerSim/
  AI/          — Utility AI controllers, team brain, perception
  Animation/   — SoccerAnimInstance, kick contact notify
  Ball/        — SoccerBall (physics: Magnus, drag, rolling resistance, CCD)
  Camera/      — BroadcastCamera (follows ball)
  Core/        — GameMode, GameState, PlayerController, GameInstance
  Data/        — Formation data, player data structs
  Field/       — SoccerField (dynamic mesh markings), SoccerGoal
  Input/       — Input processing
  Match/       — Match state machine, referee
  Player/      — SoccerPlayerPawn (movement, stamina, kicks, dribble)
  UI/          — HUD widget (score, clock, phase)
  Utils/       — SoccerSimTypes.h (constants, enums, structs)
```

## Current State

- **Phase**: Visual fix verified (materials + collision working), ready for Mixamo animations
- **Players**: Placeholder cylinders (red=Home, blue=Away) — MetaHuman disabled
- **Ball**: White sphere with proper physics (CCD, Magnus, drag), verified on field (Z=111)
- **Field**: Green pitch with white dynamic mesh markings, verified
- **Animations**: 9 Mixamo anims imported but each has own skeleton — need retargeting
- **Next**: Mixamo mesh + animation retargeting

## Verification Protocol

Never claim something works from logs alone. Always:
1. `capture-viewport` → visually analyze screenshot (Read the PNG file — it supports images)
2. `run-python-script` → query ball/player positions in PIE world
3. `get-logs --filter error` → check for LogSoccerSim errors
4. FPS: `set-console-var t.MaxFPS 60` works; `stat fps` does NOT work via soft-ue-cli (not a CVar)

### PIE Python Query Template
```python
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
# Then: unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall)
```
