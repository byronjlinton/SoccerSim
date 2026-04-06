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

## MCP Bridges

### UnrealClaude (Primary — port 3000)
- **Plugin**: `Plugins/UnrealClaude/` — 20+ MCP tools
- **Tools**: Actor manipulation, Blueprint editing, AnimBP state machines, materials, assets, viewport capture, console commands, script execution
- **Dynamic UE 5.7 context**: Accurate API docs on demand via `unreal_get_ue_context`
- **MCP endpoint**: `http://localhost:3000/mcp` — starts automatically when editor loads
- **Key advantage**: AnimBP state machine editing (transitions, conditions) that soft-ue-cli cannot do

### soft-ue-cli (Fallback — port 8080)
- **CLI**: `C:\Users\byron\AppData\Roaming\Python\Python314\Scripts\soft-ue-cli.exe`
- **Critical**: Set `MSYS_NO_PATHCONV=1` when calling from Git Bash
- **Limitations**: Parameter parsing bugs in some tools — fall back to bash if MCP call fails
- **Use when**: UnrealClaude is unavailable or a specific soft-ue-cli tool works better

### General
- **PIE auto-starts**: `GameDefaultMap=/Game/Maps/Match` in DefaultEngine.ini
- **Asset creation**: Must stop PIE first
- **Viewport capture**: Does NOT capture UMG/HUD — only 3D scene

## UE5 Coding Conventions

- **Prefixes**: A (Actors), U (UObjects/Components), F (structs), E (enums), I (interfaces)
- Every exposed property: `UPROPERTY()` with appropriate specifiers
- Every exposed function: `UFUNCTION()` with appropriate specifiers
- Headers use forward declarations; .cpp uses `#include`
- `#pragma once` in all headers, `GENERATED_BODY()` in all classes
- Components created in constructor with `CreateDefaultSubobject<T>()`
- `FName` for identifiers, `FText` for UI, `FString` for general strings
- Enhanced Input System (not legacy input)
- All magic numbers extracted to `UPROPERTY(EditDefaultsOnly)` or constants

## Known UE5.7 Quirks

- **DefaultLitMaterial has NO parameters**: `SetVectorParameterValue("BaseColor", ...)` silently does nothing. Use `/Game/Art/M_DynamicColor` (custom material with VectorParameter "BaseColor" connected to Base Color output).
- **Collision profiles need explicit channel responses**: "Ball" profile doesn't implicitly block ECC_WorldStatic. Added `SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block)` in C++ constructor.
- **Blueprint EditDefaultsOnly properties**: C++ base declares UPROPERTY but sets no defaults. BP child must have values set via `set-asset-property`.
- **SoftUEBridge HTTP handler can deadlock**: After heavy Python scripting, restart the editor. Save assets frequently before heavy operations.

## Project Structure

```
Source/SoccerSim/
  AI/          — Utility AI controllers, team brain, GK AI, perception
  Animation/   — SoccerAnimInstance (blendspace params), AnimNotify_KickContact
  Ball/        — SoccerBall (FIFA-spec: Magnus, drag, rolling, CCD)
  Camera/      — SoccerBroadcastCamera (ball tracking, 42-deg FOV)
  Core/        — GameMode, GameState, PlayerController, GameInstance
  Data/        — SoccerTacticsDataAsset (formations, stats)
  Field/       — SoccerField (105x68m dynamic markings), SoccerGoal
  Input/       — Enhanced Input (move, sprint, pass, shoot, tackle, switch, lob)
  Match/       — Match state machine (kickoff→halves→fulltime), ball-out-of-play
  Player/      — SoccerPlayerPawn (stamina, 11 kick types, dribble, shot charging)
  UI/          — SoccerHUDWidget (score, clock, phase)
  Utils/       — SoccerSimTypes.h (constants, enums, structs)
```

## Current State

- **C++**: All 12 modules complete — no stubs, no TODOs
- **Players**: 22 players, Dribble skeletal mesh, ABP_SoccerPlayer_C with BlendListByBool tree
- **Animations**: 10 Mixamo anims retargeted onto Dribble_Skeleton via IK Rig pipeline
- **AnimBP**: BlendListByBool blend tree (Idle/Locomotion/Dribble) — pivoted from state machine because Python API couldn't wire transition conditions
- **Ball**: White sphere, FIFA-spec physics (CCD, Magnus 0.00045, drag Cd=0.25, rolling mu=0.015)
- **Field**: 105x68m pitch with white dynamic mesh markings, goals with scoring
- **Match Flow**: Kickoff → First Half → Half Time → Second Half → Full Time, ball-out-of-play, goal resets
- **AI**: Dual Utility AI + FSM, 10+ formations, man-marking, pressing, GK specialization
- **HUD**: Score, clock, phase — working in PIE
- **Camera**: Broadcast camera with ball tracking

## Roadmap (A→E)

### Phase A: Animation Completion
1. Team-colored materials (MI_Home red + MI_Away blue from M_DynamicColor)
2. BlendSpace for 8-directional locomotion (forward/back/strafe)
3. Wire kick montage (AM_Kick triggered on shoot/pass)
4. Clean up ABP (remove disconnected state machine node)

### Phase B: Gameplay Polish
5. Set pieces (free kicks, penalties, corner delivery)
6. Audio system (whistle, kick sounds, crowd ambient)
7. Visual referee on field + foul detection
8. Improved player switching logic

### Phase C: Visual Quality
9. Ball material (football texture — white + black pentagons)
10. Pitch material (Quixel grass, normal map, displacement)
11. Goal nets (physics-enabled mesh)
12. Stadium geometry (stands, floodlights, sideline)

### Phase D: Broadcast Experience
13. Multi-camera system (sideline, goal, tactical, replay)
14. HUD redesign (broadcast overlay, team badges, player names)
15. Post-processing (color grading LUT, DOF, motion blur)
16. Crowd system (Niagara LOD with event reactions)

### Phase E: Polish & Ship
17. Menu system (main menu, team select, pause)
18. Performance pass (LOD, culling, 60 FPS target)
19. Weather system (rain, wind, dynamic sky)
20. Save/load (match settings, controller config)

## Verification Protocol

Never claim something works from logs alone. Always:
1. `capture-viewport` → visually analyze screenshot (Read the PNG)
2. `run-python-script` → query ball/player positions in PIE world
3. `get-logs --filter error` → check for LogSoccerSim errors
4. FPS: `set-console-var t.MaxFPS 60` works; `stat fps` does NOT work via soft-ue-cli

### PIE Python Query Template
```python
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
# Then: unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall)
```
