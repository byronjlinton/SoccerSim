# SoccerSim Broadcast Visuals — Master Plan (Autonomous)

Single combined plan for making the game look like a realistic TV broadcast. **No paid assets.** Execute in order. After this, gameplay can be refined.

---

## Current state (what the agent did)

- **MetaHuman:** Plugin enabled in `.uproject`. `SoccerPlayerPawn` has `MetaHumanMeshPath` and optional config key `DefaultMetaHumanMeshPath` in `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerPlayerPawn]`. Full workflow and first-run checklist: **Docs/MetaHuman-Setup.md**. Asset guidelines: **Docs/Visuals-AssetGuidelines.md**.
- **Ball:** `SoccerBall` has **FootballMaterialPath** (FSoftObjectPath). If set (e.g. `/Game/Art/Ball/M_Football`) and valid, that material is applied; else a white dynamic material is used. Create `M_Football` in Content/Art/Ball/ with a white+black pattern for a recognizable football.
- **Pitch:** `SoccerField` has **PitchMaterialPath**. If set (e.g. Megascans grass), it is applied; else green dynamic material.
- **Goals:** `SoccerGoal` has **NetMaterialPath**. If set (e.g. translucent grid material), it is applied to net panels; else light grey.
- **Lighting:** Stadium lighting spawned in `SoccerGameMode::SpawnStadiumLighting()` (DirectionalLight -40° pitch, 30° yaw, intensity 11; SkyLight; ExponentialHeightFog). Tuned for a broadcast-style key light.
- **Camera:** `SoccerBroadcastCamera` has BroadcastFOV (default 42°), bFixedAngle. Recommended FOV 40–45°.
- **Post-process:** Unbounded volume with AutoExposureBias and ColorSaturation. Optional LUT can be set in-editor on the volume.
- **Remaining one-time steps (user or later run):** Create one MetaHuman in Creator → Assemble → set mesh path (config or Blueprint). Optionally create M_Football, pitch grass material, and net material in Content and set the corresponding paths on Blueprints or in code defaults.

---

## Prerequisites (do first)

1. **Plugins (.uproject)**  
   - **MetaHuman** is enabled in `SoccerSim.uproject` (`"Name": "MetaHuman"`). Quixel/Megascans: enable **Quixel Bridge** or **Megascans** in the editor (Edit > Plugins) if you want to add grass/materials from the library; the game runs and looks good without it (agent-created materials are used for pitch, ball, net).

2. **Build**  
   - After plugin changes, build from command line (Build.bat) or IDE; do not rely on compiling from inside the editor while it is running.

---

## How to run

1. **Normal run:** Double-click `SoccerSim.uproject` (or open it from the Epic Games Launcher), then press **Play**. You should see the broadcast camera, green pitch, goals, ball, and 22 visible players (team-colored placeholders if no MetaHuman/mannequin mesh is set).
2. **After C++ changes:** Close the Unreal Editor, run a full build from the command line (e.g. `Build.bat SoccerSimEditor Win64 Development "<path-to>\SoccerSim.uproject" -waitmutex`) or from Visual Studio, then reopen the project and press Play.

No editor setup is required for basic visibility; the game mode and spawn logic are already configured.

- **View toggle:** Press **V** (or the key bound to IA_ViewToggle) to switch between the broadcast camera and the possessed player view. In player view you can move and control the camera from behind your player.
- **Level:** Use an empty level (or one without a large floor at Z=0) so the pitch fills the view; SoccerSim spawns the pitch at Z=100. If you see a checkerboard, that is often the default level floor—the pitch is above it.

---

## Phase 1: Pitch and lighting

- **Pitch look**  
  - In `SoccerField.cpp`: keep current pitch mesh (scaled plane/cube). In `BeginPlay`, create a `UMaterialInstanceDynamic` from an engine base material that supports a color parameter (e.g. `/Engine/BasicShapes/BasicShapeMaterial` or similar), set a grass-green base color, and assign to `PitchMesh->SetMaterial(0, ...)`. No dependency on Bridge; works in empty project.

- **Stadium lighting**  
  - In `SoccerGameMode::StartPlay`, after spawning the field, spawn a `ADirectionalLight` and optionally `ASkyLight` and `AExponentialHeightFog` so an empty level has stadium-style daylight. Set rotation and intensity for a main key light (e.g. sun from above and to one side). No user placement required.

- **Files:** `Source/SoccerSim/Field/SoccerField.cpp`, `Source/SoccerSim/Core/SoccerGameMode.cpp`.

**Done when:** Pitch is visibly green from the broadcast camera; level is well lit without placing lights in the editor.

---

## Phase 2: Broadcast camera (single angle)

- **SoccerBroadcastCamera** has `BroadcastFOV` (default 42°) and `bFixedAngle`. Recommended FOV for a single classic broadcast angle: **40–45°**. Set `bFixedAngle = true` for a fixed sideline view that does not follow the ball; `false` for smooth follow.
- FOV is applied in `BeginPlay`; fixed-angle behavior is in `Tick` (no or minimal position interpolation when true).

**Done when:** Camera has configurable TV-like FOV and an optional fixed sideline angle.

---

## Phase 3: Goals and ball (agent-created look)

- **Goals**  
  - In `SoccerGoal.cpp`, the net is currently simple geometry. If net meshes use a material slot, create a `UMaterialInstanceDynamic` from a simple translucent/grey material (engine or project) and assign to net components so the goal reads as having a net. Keep trigger and logic unchanged.

- **Ball**  
  - In `SoccerBall.cpp` in `BeginPlay`, create a `UMaterialInstanceDynamic` from a base material that supports color (e.g. white), set base color to white; optionally add a second material slot or overlay for dark patches (football pattern) if the engine material supports it. Otherwise white sphere is acceptable. Keep physics and scale unchanged.

- **Files:** `Source/SoccerSim/Field/SoccerGoal.cpp`, `Source/SoccerSim/Ball/SoccerBall.cpp`.

**Done when:** Goals show a net-like material; ball is white (or simple pattern) instead of grey.

---

## Phase 4: Players (MetaHuman-ready) and docs

- **Mesh assignment**  
  - Ensure `SoccerPlayerPawn` can use an external skeletal mesh: either (a) in C++, try to load a mesh from a configurable path (e.g. `/Game/MetaHumans/...`) and call `GetMesh()->SetSkeletalMesh(...)` in `BeginPlay` if found, or (b) document that a Blueprint subclass of `SoccerPlayerPawn` should set the mesh to the MetaHuman skeleton. Prefer (a) with a `UPROPERTY(EditDefaultsOnly) FSoftObjectPath MetaHumanMeshPath` so the default is empty and the user (or agent) can set it after importing a MetaHuman.

- **Docs**  
  - Add `Docs/Visuals-AssetGuidelines.md`: state that all assets are free-only (MetaHuman, Quixel). List steps: enable MetaHuman plugin (done in .uproject), create 2–3 MetaHumans in MetaHuman Creator, import into project, assign mesh path in game mode or pawn defaults, retarget animations to MetaHuman rig. Note skeleton/AnimBP requirements and “avoid” low-quality human packs.

- **Files:** `Source/SoccerSim/Player/SoccerPlayerPawn.cpp` (and .h if needed), `Docs/Visuals-AssetGuidelines.md`.

**Done when:** Pawn supports optional MetaHuman mesh path; doc describes the full free-asset pipeline.

---

## Phase 5: Polish (post-process)

- An unbounded **APostProcessVolume** is spawned in `SoccerGameMode::StartPlay` with AutoExposureBias and ColorSaturation for a subtle broadcast look. For a stronger film look, add a **LUT** (Look-Up Texture): create or import a LUT asset, then in the Post Process Volume settings set **Color Grading** → **Color Grading LUT** to that texture (or document the asset path for a one-time assignment in the editor).
- **Files:** `Source/SoccerSim/Core/SoccerGameMode.cpp`.

**Done when:** Running the game has a subtle broadcast-style post-process; LUT is optional and can be set in-editor.

---

## Verification

- Build: `"C:\Program Files\Epic Games\Unreal 2\UE_5.7\Engine\Build\BatchFiles\Build.bat" SoccerSimEditor Win64 Development "C:\Users\byron\Cursor\SoccerSim\SoccerSim.uproject" -waitmutex`  
- Open `SoccerSim.uproject` in the editor; press Play on an empty level. Expect: green pitch, good lighting, broadcast camera, white (or patterned) ball, goal nets visible, optional post-process. MetaHuman characters appear only after the user imports MetaHumans and sets the mesh path (or uses a Blueprint).

---

## Installation reference (for agent)

- **MetaHuman:** Enabled in `SoccerSim.uproject` via `Plugins` array. No separate installer; plugin ships with the engine. User creates characters in https://metahuman.unrealengine.com and imports via the MetaHuman Plugin in-editor.
- **Quixel Bridge:** Usually built into UE5; enable in .uproject if listed. User opens Bridge in-editor to download Megascans; no code change required for Bridge itself. Agent-created materials (dynamic instances) do not require Bridge.
- **Build:** Always use the Build.bat command line (or IDE build) when the editor is closed or when modules change; do not compile from inside the running editor for full rebuilds.
