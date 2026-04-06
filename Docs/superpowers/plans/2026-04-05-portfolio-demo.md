# SoccerSim Portfolio Demo — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bootstrap SoccerSim from "code exists but PIE shows nothing" to a photorealistic 3-minute 5v5 playable demo for hiring managers.

**Architecture:** Two phases — Phase 0 diagnoses and fixes the code-to-game gap (Blueprint wiring, collision, physics activation). Phase A builds visual polish on top (stadium, materials, post-processing, audio). System-by-system approach: each task verifies one subsystem in PIE before moving to the next.

**Tech Stack:** UE 5.7 C++, UnrealClaude MCP (primary automation), soft-ue-cli MCP (fallback), Git Bash, Quixel Megascans (Phase A)

---

## File Structure

### Files Modified (Phase 0)
- `SoccerSim.uproject` — Enable UnrealClaude plugin
- `Config/DefaultGame.ini` — Fix asset paths if broken
- `Config/DefaultEngine.ini` — Verify collision profiles
- Blueprint properties set via MCP: `BP_SoccerGameMode`, `BP_SoccerPlayerController`, `BP_SoccerPlayerPawn`, `BP_SoccerBall`, `BP_SoccerField`, `BP_SoccerGoal`

### Files Created (Phase A)
- Stadium geometry via MCP (stands, floodlights)
- Material instances (MI_Home, MI_Away, MI_Football, MI_Grass_PBR)
- Post-processing volume + LUT
- Audio assets (crowd, whistle, kicks)
- UI widgets (start screen, end screen)

---

## Phase 0 — Make It Work

### Task 0.1: Enable UnrealClaude Plugin

**Files:**
- Modify: `SoccerSim.uproject`

**Context:** UnrealClaude plugin exists in `Plugins/UnrealClaude/` but is NOT enabled in the .uproject file. Only SoftUEBridge is enabled. Without UnrealClaude, we lose the primary MCP automation path.

- [ ] **Step 1: Add UnrealClaude to .uproject plugins array**

Open `SoccerSim.uproject` and add after the SoftUEBridge entry:

```json
{
  "Name": "UnrealClaude",
  "Enabled": true
}
```

- [ ] **Step 2: Rebuild project**

```bash
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Build/BatchFiles/Build.bat" \
  SoccerSimEditor Win64 Development "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject" -waitmutex
```

Expected: Build succeeds with 0 errors.

- [ ] **Step 3: Verify plugin loads**

Launch editor. Check output log for `UnrealClaude` initialization message. Verify MCP endpoint responds at `http://localhost:3000/mcp`.

- [ ] **Step 4: Commit**

```bash
git add SoccerSim.uproject
git commit -m "Enable UnrealClaude plugin in .uproject"
```

---

### Task 0.2: PIE Baseline — Capture What's Broken

**Context:** Before fixing anything, we need ground truth. Launch PIE, capture viewport, read logs.

- [ ] **Step 1: Launch editor**

```bash
"/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe" \
  "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject"
```

- [ ] **Step 2: Start PIE**

Wait for editor to fully load. Press Play (or PIE auto-starts via GameDefaultMap=/Game/Maps/Match).

- [ ] **Step 3: Capture viewport**

Use UnrealClaude `capture-viewport` tool. Save screenshot. Read the PNG — document exactly what's visible.

- [ ] **Step 4: Read output log**

Use UnrealClaude `get-logs --filter LogSoccerSim` tool. Record ALL log entries — warnings and errors indicate what's failing.

- [ ] **Step 5: Run diagnostic Python script**

Use UnrealClaude `run-python-script` to query PIE world state:

```python
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
if world:
    balls = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall)
    players = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerPlayerPawn)
    fields = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerField)
    goals = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerGoal)
    print(f"Balls: {len(balls)}, Players: {len(players)}, Fields: {len(fields)}, Goals: {len(goals)}")
    for b in balls:
        print(f"  Ball at {b.get_actor_location()}, velocity {b.get_velocity()}")
    for p in players[:3]:
        print(f"  Player {p.get_name()} at {p.get_actor_location()}, mesh={p.get_mesh_component()}")
else:
    print("NO GAME WORLD — PIE not running or world not accessible")
```

Expected: We'll see what actually spawned vs what's missing.

- [ ] **Step 6: Document baseline**

Write findings to a diagnostic note (don't save to memory — this is ephemeral). Record:
- Screenshot description
- Actor counts (ball, players, field, goals)
- All LogSoccerSim warnings/errors
- What's visible vs missing

This baseline determines exactly which subsequent tasks are needed.

---

### Task 0.3: Fix GameMode Blueprint Defaults

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerGameMode.uasset`

**Context:** `ASoccerGameMode::StartPlay()` spawns everything — field, goals, ball, teams, camera. If `FieldClass`, `BallClass`, `PlayerPawnClass`, `GoalClass`, or `BroadcastCameraClass` are null in BP defaults, nothing spawns. The C++ constructor hardcodes `PlayerControllerClass = ASoccerPlayerController::StaticClass()` but everything else needs BP assignment.

- [ ] **Step 1: Query BP_GameMode properties**

Use UnrealClaude `query-asset` or `get-asset-property` to check BP_SoccerGameMode defaults:

```
FieldClass — should be BP_SoccerField_C
BallClass — should be BP_SoccerBall_C
GoalClass — should be BP_SoccerGoal_C
PlayerPawnClass — should be BP_SoccerPlayerPawn_C
BroadcastCameraClass — should be BP_SoccerBroadcastCamera_C
PlayerControllerClass — should be BP_SoccerPlayerController_C (may be auto-set)
GameStateClass — should be BP_SoccerGameState_C (may be auto-set)
```

- [ ] **Step 2: Set any null class references**

Use UnrealClaude `set-asset-property` for each null class. Expected values:
- `FieldClass` → `/Game/Blueprints/BP_SoccerField.BP_SoccerField_C`
- `BallClass` → `/Game/Blueprints/BP_SoccerBall.BP_SoccerBall_C`
- `GoalClass` → `/Game/Blueprints/BP_SoccerGoal.BP_SoccerGoal_C`
- `PlayerPawnClass` → `/Game/Blueprints/BP_SoccerPlayerPawn.BP_SoccerPlayerPawn_C`
- `BroadcastCameraClass` → `/Game/Blueprints/BP_SoccerBroadcastCamera.BP_SoccerBroadcastCamera_C`
- `PlayerControllerClass` → `/Game/Blueprints/BP_SoccerPlayerController.BP_SoccerPlayerController_C`
- `GameStateClass` → `/Game/Blueprints/BP_SoccerGameState.BP_SoccerGameState_C`

- [ ] **Step 3: Save asset**

Use UnrealClaude `save-asset` on BP_SoccerGameMode.

- [ ] **Step 4: Verify in PIE**

Restart PIE. Run diagnostic Python script from Task 0.2 Step 5. Expected: actor counts should be > 0 for field, ball, goals, players.

- [ ] **Step 5: Commit**

Only if changes were needed.

---

### Task 0.4: Fix PlayerController Blueprint Defaults

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerPlayerController.uasset`

**Context:** `ASoccerPlayerController` needs Enhanced Input assets wired. Without `DefaultMappingContext` and all IA_* references, no input works. Without `HUDWidgetClass`, no HUD shows.

- [ ] **Step 1: Query BP_PlayerController properties**

Check these EditDefaultsOnly properties:
```
DefaultMappingContext — should be /Game/Input/IMC_Soccer
IA_Move — should be /Game/Input/IA_Move
IA_Sprint — should be /Game/Input/IA_Sprint
IA_Pass — should be /Game/Input/IA_Pass
IA_Shoot — should be /Game/Input/IA_Shoot
IA_ThroughBall — should be /Game/Input/IA_ThroughBall
IA_Tackle — should be /Game/Input/IA_Tackle
IA_SwitchPlayer — should be /Game/Input/IA_SwitchPlayer
IA_Lob — should be /Game/Input/IA_Lob
IA_ViewToggle — should be /Game/Input/IA_ViewToggle
HUDWidgetClass — should be /Game/Blueprints/WBP_SoccerHUD
```

- [ ] **Step 2: Set any null references**

Use `set-asset-property` for each null property with the paths above.

- [ ] **Step 3: Save asset**

- [ ] **Step 4: Verify in PIE**

Restart PIE. Press WASD — player should move. Press V — camera should toggle. HUD should show score/clock.

- [ ] **Step 5: Commit**

---

### Task 0.5: Fix PlayerPawn Blueprint Defaults

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerPlayerPawn.uasset`

**Context:** `ASoccerPlayerPawn` needs the Dribble skeletal mesh path and ABP_SoccerPlayer AnimBP path. DefaultGame.ini configures these as `/Game/Anim/Mixamo/Dribble` and `/Game/Anim/ABP_SoccerPlayer`. If these paths don't match actual assets, players will T-pose or show placeholder geometry.

- [ ] **Step 1: Verify asset paths exist**

Check that these assets actually exist:
- `/Game/Anim/Mixamo/Dribble` — Dribble skeletal mesh
- `/Game/Anim/ABP_SoccerPlayer` — Animation Blueprint
- `/Game/Art/M_DynamicColor` — Dynamic color material

Use UnrealClaude `search-assets` or query the content browser.

- [ ] **Step 2: Verify DefaultGame.ini paths match**

Read `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerPlayerPawn]`:
```
DefaultMetaHumanMeshPath="/Game/Anim/Mixamo/Dribble"
DefaultMetaHumanAnimBlueprintPath="/Game/Anim/ABP_SoccerPlayer"
```

If paths don't match actual assets, update DefaultGame.ini.

- [ ] **Step 3: Check BP_SoccerPlayerPawn collision profile**

The pawn's capsule component must use `SoccerPlayer` collision profile. Verify via `get-asset-property`:
```
CapsuleComponent.CollisionProfileName — should be "SoccerPlayer"
```

- [ ] **Step 4: Verify in PIE**

Restart PIE. Run diagnostic script — check that players have valid mesh components. Capture viewport — players should show Dribble mesh, not placeholders.

- [ ] **Step 5: Commit**

---

### Task 0.6: Fix Ball Physics

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerBall.uasset`
- Possibly modify: `Source/SoccerSim/Ball/SoccerBall.cpp`

**Context:** The ball needs a physics-enabled collision sphere using the `Ball` collision profile. C++ constructor creates the sphere and sets simulating physics, but if the collision profile isn't registered, physics won't activate.

- [ ] **Step 1: Verify Ball collision profile**

Check DefaultEngine.ini collision profiles:
```
+Profiles=(Name="Ball",CollisionEnabled=QueryAndPhysics,ObjectTypeName="Ball",...)
```

If missing, add it. The profile must include responses for ECC_WorldStatic (Block), ECC_WorldDynamic (Block), ECC_Pawn (Block).

- [ ] **Step 2: Verify BP_Ball collision component**

Check that BP_SoccerBall's sphere component uses `Ball` collision profile and `Simulate Physics = true`.

- [ ] **Step 3: Verify in PIE**

Restart PIE. Run diagnostic — ball should spawn at center, fall to ground, rest on field. Ball should NOT fall through floor.

- [ ] **Step 4: If ball falls through floor**

Add to SoccerBall.cpp constructor (if not already there):
```cpp
CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
```

Rebuild, re-verify.

- [ ] **Step 5: Commit**

---

### Task 0.7: Fix Field Rendering

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerField.uasset`
- Possibly modify: `Source/SoccerSim/Field/SoccerField.cpp`

**Context:** SoccerField uses `UDynamicMesh3` to generate field markings procedurally. If the dynamic mesh subsystem isn't initialized or the material isn't assigned, markings won't render.

- [ ] **Step 1: Verify BP_Field has valid material**

Check `PitchMaterialPath` — should be `/Game/Art/Pitch/M_Grass` or null (green fallback in C++).

- [ ] **Step 2: Verify field spawns at correct location**

PIE diagnostic script should show field at origin (0,0,0). Check actor location.

- [ ] **Step 3: Verify field has collision plane**

The field must have a physics collision plane for the ball to rest on. Check that `PitchMesh` component has `Simulate Physics = false` and blocks the Ball collision channel.

- [ ] **Step 4: If markings don't render**

Check SoccerField::BeginPlay() — verify DynamicMesh3 generation code runs. Add `UE_LOG` temporarily if needed to trace execution. Common issue: `DynamicMesh` component not registered or material not applied.

- [ ] **Step 5: Verify in PIE**

Capture viewport — should see green pitch with white line markings (touchlines, center circle, penalty areas, etc.).

- [ ] **Step 6: Commit**

---

### Task 0.8: Fix Goal Spawning + Scoring

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerGoal.uasset`

**Context:** Two goals should spawn — home at (-HalfLength, 0, 0) and away at (HalfLength, 0, 0). Each has posts, crossbar, net, and a GoalTrigger overlap for scoring. `TeamId` must be set to Home or Away respectively.

- [ ] **Step 1: Verify BP_Goal has TeamId set**

This won't be set on the BP itself — it's set by GameMode when spawning. Verify `ASoccerGameMode::SpawnField()` passes TeamId:
```cpp
HomeGoal->TeamId = ETeamId::Home;
AwayGoal->TeamId = ETeamId::Away;
```

- [ ] **Step 2: Verify goal trigger has correct collision profile**

GoalTrigger must use `GoalTrigger` collision profile (QueryOnly, overlaps with Ball).

- [ ] **Step 3: Verify scoring works**

PIE test: move ball (or use console command) into goal trigger. Check logs for goal detection. Verify `OnGoalScored` fires.

- [ ] **Step 4: Commit**

---

### Task 0.9: Fix AI Activation

**Files:**
- Verify/fix via MCP: `Content/Blueprints/BP_SoccerAIController.uasset`

**Context:** GameMode spawns `ASoccerAIController` for each non-possessed player. If the AI controller class isn't set in GameMode or the BP doesn't inherit correctly, players stand still.

- [ ] **Step 1: Verify BP_SoccerAIController inherits from ASoccerAIController**

Check BP parent class — should be `SoccerAIController`.

- [ ] **Step 2: Verify GameMode spawns AI controllers**

Check that `ASoccerGameMode::SpawnTeams()` creates AI controllers and possesses each pawn:
```cpp
ASoccerAIController* AI = GetWorld()->SpawnActor<ASoccerAIController>(...);
AI->Possess(PlayerPawn);
```

- [ ] **Step 3: Verify AI moves in PIE**

Watch non-possessed players for 10 seconds. They should track ball, move toward it, attempt passes. If static, check AIController::BeginPlay() and OnPossess() fire.

- [ ] **Step 4: Commit**

---

### Task 0.10: Verify Match Flow End-to-End

**Context:** Now that all subsystems work, verify the full match flow.

- [ ] **Step 1: Verify kickoff sequence**

PIE starts → match phase should be KickOff → ball at center → 2-second delay → FirstHalf starts → clock begins.

- [ ] **Step 2: Verify goal detection**

Kick ball into goal. Verify:
- Score increments
- Score HUD updates
- Ball resets to center
- Players reposition to formation
- Match continues

- [ ] **Step 3: Verify halftime**

Wait for first half to end (270 seconds by default). Verify:
- Phase transitions to HalfTime
- After 15 seconds, SecondHalfKickOff
- Then SecondHalf begins
- Clock resets

- [ ] **Step 4: Verify full time**

Wait for second half to end. Verify phase transitions to FullTime.

- [ ] **Step 5: Commit any fixes**

- [ ] **Step 6: Tag Phase 0 complete**

```bash
git tag phase-0-complete
```

**Phase 0 exit criteria**: Playable soccer match — players move, ball reacts to kicks, AI controls non-possessed players, goals score, clock runs, HUD shows score/time/phase, full match flow works.

---

## Phase A — Make It Beautiful

### Task A.1: Stadium Geometry

**Files:**
- Created via MCP: Stadium static meshes, actor blueprint

**Context:** Empty pitch needs surrounding stadium to look photorealistic. Simple geometry is sufficient — stands, floodlights, sideline structures. These can be basic BSP/geometry shapes with materials.

- [ ] **Step 1: Create stadium stands**

Use UnrealClaude to spawn and position stand geometry around the field. Basic shapes:
- 4 stand sections (behind each goal + 2 sidelines)
- Height: ~15m, depth: ~10m
- Material: concrete base + colored seats

- [ ] **Step 2: Create floodlights**

Spawn 4 floodlight towers at field corners. Height ~30m. Add spotlights pointing down.

- [ ] **Step 3: Configure Lumen outdoor lighting**

Set up directional light (sun), sky light, exponential height fog. Tune for late-afternoon match lighting.

- [ ] **Step 4: Verify in PIE**

Capture viewport — should see stadium surrounding the pitch with dramatic lighting.

- [ ] **Step 5: Commit**

---

### Task A.2: Materials

**Files:**
- Created/modified via MCP: Material instances

**Context:** Replace placeholder materials with photorealistic ones.

- [ ] **Step 1: Create PBR grass material**

Use Quixel Megascans grass texture or create procedural grass material with:
- Base color: Green with subtle variation
- Normal map: Grass blade detail
- Roughness: 0.8-0.9 (matte grass)

- [ ] **Step 2: Create football material**

White base with black pentagon pattern. Use texture or procedural approach.

- [ ] **Step 3: Create team-colored jersey materials**

Create `MI_Home` (red) and `MI_Away` (blue) from `M_DynamicColor`:
```python
# Via UnrealClaude script
import unreal
# Create MI_Home with BaseColor = (0.8, 0.1, 0.1, 1.0)
# Create MI_Away with BaseColor = (0.1, 0.2, 0.8, 1.0)
```

- [ ] **Step 4: Apply materials in SoccerPlayerPawn**

Update C++ to assign MI_Home to Home team players and MI_Away to Away team players in `UpdateTeamAppearance()`.

- [ ] **Step 5: Verify in PIE**

Players should show red (home) and blue (away) jerseys. Ball should look like a football. Pitch should look like real grass.

- [ ] **Step 6: Commit**

---

### Task A.3: Broadcast Camera System

**Files:**
- Modify: `Source/SoccerSim/Camera/SoccerBroadcastCamera.h/.cpp`

**Context:** Multi-angle broadcast camera with smooth transitions.

- [ ] **Step 1: Add camera modes enum**

Add `ECameraMode` enum: `Sideline, GoalBehind, Tactical, Replay`.

- [ ] **Step 2: Implement camera switching**

Press V cycles through modes. Each mode has different position/rotation targets. Interpolate between them smoothly.

- [ ] **Step 3: Add camera shake**

On goal events and hard tackles, apply camera shake via `CameraShakeBase`.

- [ ] **Step 4: Verify in PIE**

V key should cycle camera angles smoothly. Goals should trigger subtle shake.

- [ ] **Step 5: Commit**

---

### Task A.4: Animation Polish

**Files:**
- Modify via MCP: `ABP_SoccerPlayer`, `BP_SoccerPlayerPawn`
- Create via MCP: BlendSpace for locomotion

**Context:** Replace single Jog_Forward with 8-directional BlendSpace. Wire kick montage.

- [ ] **Step 1: Create BlendSpace**

Create BS_Locomotion with:
- Horizontal axis: Direction (-180 to 180)
- Vertical axis: Speed (0 to 600)
- Samples: Idle, Jog_Forward/Back/Left/Right at appropriate speed/direction

Note: BlendSpace axis configuration may require manual editor work (MCP limitation). If so, document the exact setup for manual creation.

- [ ] **Step 2: Update ABP to use BlendSpace**

Replace current Jog_Forward sequence player with BS_Locomotion BlendSpace in the Locomotion branch.

- [ ] **Step 3: Wire kick montage**

Connect AM_Kick montage to shoot/pass input. Use `AnimNotify_KickContact` to time the actual kick impulse with the animation frame.

- [ ] **Step 4: Verify in PIE**

Players should show directional running (not moonwalking). Kick animation should play on shoot/pass.

- [ ] **Step 5: Commit**

---

### Task A.5: Audio

**Files:**
- Created via MCP/editor: Sound assets, audio components
- Modify: `Source/SoccerSim/Core/SoccerGameMode.h/.cpp` (whistle triggers)
- Modify: `Source/SoccerSim/Ball/SoccerBall.cpp` (kick/bounce sounds)
- Modify: `Source/SoccerSim/Player/SoccerPlayerPawn.cpp` (footstep sounds)

**Context:** Silent match looks dead. Need ambient crowd, whistle, kick sounds.

- [ ] **Step 1: Source audio files**

Download free sounds from freesound.org or use UE Starter Content:
- Crowd ambient (looping, 2-3 layers)
- Whistle (short blast)
- Kick impact (2-3 variations)
- Ball bounce (ground and post)
- Goal crowd roar

- [ ] **Step 2: Import and create SoundCues**

Import WAV files to `Content/Audio/`. Create SoundCues with attenuation.

- [ ] **Step 3: Add crowd ambient**

Spawn an `UAudioComponent` on the field actor. Play crowd ambient loop on BeginPlay. Volume modulated by match events.

- [ ] **Step 4: Add whistle triggers**

In SoccerGameMode, play whistle sound on phase transitions (KickOff, HalfTime, FullTime).

- [ ] **Step 5: Add kick/bounce sounds**

In SoccerBall::OnBallHit, play kick sound. In ground bounce handling, play bounce sound.

- [ ] **Step 6: Verify in PIE**

Should hear crowd ambient, whistle on kickoff, kick sounds when striking ball.

- [ ] **Step 7: Commit**

---

### Task A.6: Post-Processing

**Files:**
- Created via MCP: Post-processing volume, LUT texture

**Context:** Broadcast TV look requires color grading, DOF, motion blur.

- [ ] **Step 1: Create post-processing volume**

Add a global post-processing volume covering the field. Configure:
- Color grading: Warm tones, slight desaturation of shadows, boosted highlights
- Bloom: Subtle, 0.1 intensity
- Motion blur: 0.3 amount, per-object
- Vignette: 0.3 intensity
- Lens flare: On floodlight sources

- [ ] **Step 2: Create or import color grading LUT**

Create a 32x1 LUT texture with warm broadcast grading. Apply to post-process volume.

- [ ] **Step 3: Add depth of field**

Cinematic DOF with focal distance tied to ball position. Bokeh shape: circular. Max bokeh size: 0.5.

- [ ] **Step 4: Verify in PIE**

Capture viewport — should have cinematic broadcast look with warm grading, subtle bloom, and background blur.

- [ ] **Step 5: Commit**

---

### Task A.7: Broadcast HUD

**Files:**
- Modify via MCP: `Content/Blueprints/WBP_SoccerHUD.uasset`
- Possibly modify: `Source/SoccerSim/UI/SoccerHUDWidget.h/.cpp`

**Context:** Replace basic HUD with broadcast-style overlay.

- [ ] **Step 1: Design HUD layout**

Broadcast-style score bar at top of screen:
```
[HOME BADGE] HOME 0 - 0 AWAY [AWAY BADGE]  |  03:00  |  1ST HALF
```

- [ ] **Step 2: Update WBP_SoccerHUD widget**

Add team name text, larger score, styled clock, phase indicator. Use broadcast-style fonts and colors (white text, dark semi-transparent background).

- [ ] **Step 3: Add controlled player name tag**

Show current player name/number below the score bar.

- [ ] **Step 4: Verify in PIE**

HUD should look like a TV broadcast overlay.

- [ ] **Step 5: Commit**

---

### Task A.8: Scale to 5v5 + Match Config

**Files:**
- Modify: `Source/SoccerSim/Core/SoccerGameMode.h/.cpp` (team size, match duration)
- Modify: `Source/SoccerSim/AI/SoccerAIController.cpp` (adjusted formation)
- Created: Start screen widget, end screen widget

**Context:** Scale from 22 players (11v11) to 10 players (5v5). Set match to 3 minutes.

- [ ] **Step 1: Change team size**

In SoccerGameMode, change `PlayersPerTeam` from 11 to 5 (or make it configurable via UPROPERTY). Adjust formation for 5v5 (e.g., 1-2-1 GK).

- [ ] **Step 2: Change match duration**

Set `HalfDurationSeconds = 90.0f` (1.5 min per half = 3 min total).

- [ ] **Step 3: Create start screen widget**

Simple widget: "SOCCER SIM" title, "Press Start to Play" prompt. Shown on game start.

- [ ] **Step 4: Create end screen widget**

Shows final score, "Play Again" and "Quit" buttons.

- [ ] **Step 5: Wire menu flow**

Start screen → press Start → PIE begins match → FullTime → end screen → Play Again restarts.

- [ ] **Step 6: Verify full demo flow**

Play a complete 3-minute 5v5 match from start screen to end screen. Verify:
- Start screen appears
- Match plays (3 minutes)
- Score updates work
- End screen shows final score
- Play Again works

- [ ] **Step 7: Commit + tag**

```bash
git tag phase-a-complete
```

**Phase A exit criteria**: 3-minute 5v5 match with photorealistic visuals, broadcast camera, audio, post-processing, styled HUD, start/end screens. Portfolio-ready.

---

## Self-Review

### 1. Spec Coverage

| Spec Requirement | Task |
|-----------------|------|
| Fix PIE (players don't move) | 0.3-0.5, 0.9 |
| Fix PIE (ball doesn't react) | 0.6 |
| Fix PIE (field looks bare) | 0.7, 0.8 |
| Stadium + lighting | A.1 |
| Materials (grass, ball, jerseys) | A.2 |
| Broadcast camera | A.3 |
| Animation (BlendSpace, kicks) | A.4 |
| Audio (crowd, whistle, kicks) | A.5 |
| Post-processing | A.6 |
| Broadcast HUD | A.7 |
| Scale to 5v5 + 3 min | A.8 |
| Start/end screens | A.8 |

All spec requirements covered. No gaps.

### 2. Placeholder Scan

No TBDs, TODOs, or "fill in later". Every step has specific actions, file paths, and verification steps. Phase 0 Steps 0.3-0.9 intentionally include conditional "if X is null, set it" logic because we don't know exactly which BP properties are missing until Task 0.2 baseline.

### 3. Type Consistency

All class names match across tasks: `BP_SoccerGameMode_C`, `ASoccerGameMode`, `ASoccerPlayerController`, etc. Collision profile names ("Ball", "SoccerPlayer", "GoalTrigger") are consistent between DefaultEngine.ini and the tasks that reference them.

### 4. Risk Notes

- Tasks 0.3-0.9 depend on Task 0.2 baseline. If the baseline reveals unexpected issues (e.g., missing C++ module, compile errors), additional tasks may be needed.
- Task A.1 (Stadium) and A.4 (BlendSpace) may need manual editor work that MCP can't fully automate.
- Task A.2 (Quixel materials) requires Quixel Bridge plugin and an Epic account with Megascans access.
