# SoccerSim Visuals — Asset Guidelines (Free Only)

For the **best product** setup (MetaHuman + football + grass in one checklist), see **Best-Product-Setup.md**. The project supports conventional config paths for ball and pitch materials as well as the MetaHuman mesh; once assets exist at those paths (or you set the paths in config), Play uses them automatically.

All visuals use **free** sources only. No paid Marketplace packs.

---

## What is free

- **MetaHuman (Epic):** Free with Unreal Engine for creators under $1M revenue/year. Create characters at [MetaHuman Creator](https://metahuman.unrealengine.com) or in-editor (UE 5.7+). Import via the MetaHuman plugin.
- **Quixel Megascans:** Free with a free Epic account when used in Unreal Engine. Use **Quixel Bridge** (enable in Edit > Plugins if needed) to browse and add grass, materials, etc. to the project.
- **Agent-created:** Pitch (green dynamic material), ball (white), goal net (grey), lighting, and post-process are set up in code so the game looks good without any assets.

---

## Player characters (MetaHuman-ready)

- See **Docs/MetaHuman-Setup.md** for the full research-backed workflow (in-editor Creator, Assemble, retargeting).
- The **MetaHuman** plugin is enabled in `SoccerSim.uproject`. You do not need to install anything else.
- **SoccerPlayerPawn** supports an optional skeletal mesh:
  - In the Blueprint subclass of `SoccerPlayerPawn` (or in the default Pawn class used by the Game Mode), set **MetaHumanMeshPath** to a MetaHuman (or other) skeletal mesh asset (e.g. `/Game/MetaHumans/.../SK_...`).
  - **Config fallback:** In `Config/DefaultGame.ini`, under `[/Script/SoccerSim.SoccerPlayerPawn]`, set `DefaultMetaHumanMeshPath="/Game/.../SK_..."` to use one mesh for all pawns without editing the Blueprint.
  - If both are empty, the default character mesh (engine mannequin) is used.
- **To use MetaHumans:** Follow **MetaHuman-Setup.md** (create MetaHuman Character → open in Creator → customize → Assemble → copy Skeletal Mesh path → set **MetaHumanMeshPath** or **DefaultMetaHumanMeshPath**; then retarget run/sprint/kick to the MetaHuman rig).
- **Avoid:** Generic FPS/soldier human packs, low-poly "game" figures, or any paid character pack that looks cheap or silly.

---

## Pitch, ball, goals, lighting

- **Pitch:** By default a grass-green dynamic material is applied in code. For a photoreal grass look, set **PitchMaterialPath** on the Field Blueprint, or set **DefaultPitchMaterialPath** in `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerField]` (e.g. `/Game/Art/Pitch/M_Grass`). Add a Megascans grass/turf via Quixel Bridge if desired. The code applies it in `SoccerField::BeginPlay` if the path is valid.
- **Ball:** By default a white dynamic material is applied in code. For a **recognizable football** (white + black patches), set **FootballMaterialPath** on the Ball Blueprint, or set **DefaultFootballMaterialPath** in `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerBall]` (e.g. `/Game/Art/Ball/M_Football`). **To create the asset (one-time):** In Content Browser create **Material** under `Content/Art/Ball/` named `M_Football`. Set Base Color to white and add a black pentagon/hexagon-style pattern. If the path is empty or invalid, the ball falls back to a plain white material.
- **Goals:** Net panels use a light-grey dynamic material by default. To make the net read clearly as a net, set **NetMaterialPath** on the Goal Blueprint to a translucent or masked material with a grid pattern (create in Content under e.g. `Content/Art/Goals/M_Net`). If the path is empty or invalid, the existing grey material is used.
- **Lighting:** Directional light, sky light, and height fog are **spawned at runtime** by the Game Mode so an empty level is well lit. No need to place lights in the editor.

---

## Build and run

- Build from **command line** or **Visual Studio** (not from inside the running editor):  
  `Build.bat SoccerSimEditor Win64 Development "path\to\SoccerSim.uproject" -waitmutex`
- Run **Scripts\EnsureSoccerSimDefaults.ps1** once so the default game mode is set; then open the project and press **Play** on any level (including Untitled).
