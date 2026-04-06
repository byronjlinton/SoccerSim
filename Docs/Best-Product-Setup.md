# Best Product Setup (MetaHuman + Ball + Grass)

One-time checklist so that when you open **SoccerSim.uproject** and press **Play**, the game shows: **MetaHuman** players (no placeholder cylinders), a **recognizable football**, and a **grass** pitch.

The project is already configured with conventional asset paths in `Config/DefaultGame.ini`. Once you create (or add) assets at those paths—or paste your own paths into config—Play will use them automatically.

---

## 1. MetaHuman (one-time)

1. Open the project in Unreal Editor.
2. Create one **MetaHuman Character** (Content Browser → Miscellaneous → MetaHuman Character). Name it e.g. `MH_Soccer`.
3. Open it in **MetaHuman Creator** (in-editor), customize if desired, then click **Assemble**. If prompted about overwriting, choose a folder such as `Content/MetaHumans/MH_Soccer/`.
4. After assembly, find the **Skeletal Mesh** (e.g. `SK_MH_Soccer`) in the Content Browser. **Copy Reference from the Skeletal Mesh (SK_...), not the MetaHuman asset.** Either:
   - **Option A:** Save/assemble so the mesh path is `/Game/MetaHumans/MH_Soccer/SK_MH_Soccer`. The default config already points there; no config edit needed.
   - **Option B:** Right-click the **Skeletal Mesh** (SK_...) → **Copy Reference**, then in `Config/DefaultGame.ini`, under `[/Script/SoccerSim.SoccerPlayerPawn]`, set `DefaultMetaHumanMeshPath="<paste that path>"`. The path must match exactly.
5. (Optional) To drive the MetaHuman with run/sprint/kick: create an **Animation Blueprint** that targets the MetaHuman skeleton, retarget animations (see **MetaHuman-Setup.md**), then in `DefaultGame.ini` set `DefaultMetaHumanAnimBlueprintPath="/Game/.../YourAnimBP.YourAnimBP"` (Copy Reference from the Anim BP asset). The pawn will use it when the MetaHuman mesh is loaded.
6. Retarget run/sprint/kick animations to the MetaHuman rig. See **MetaHuman-Setup.md** for the full workflow and links.

---

## 2. Football (one-time)

1. In Content Browser, create a **Material** (or Material Instance) under `Content/Art/Ball/` named `M_Football`.
2. Make it look like a football: white base with black patches (e.g. use a texture sample with a football texture, or a procedural pattern).
3. Save the asset. The default config uses `/Game/Art/Ball/M_Football`. If you put the asset elsewhere, set in `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerBall]`:  
   `DefaultFootballMaterialPath="/Game/.../YourMaterial"`.

---

## 3. Grass pitch (one-time)

1. Add a grass material to the project, e.g. via **Quixel Bridge** (Megascans) or create a material under `Content/Art/Pitch/` named `M_Grass`.
2. The default config uses `/Game/Art/Pitch/M_Grass`. If your grass asset has a different path, set in `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerField]`:  
   `DefaultPitchMaterialPath="/Game/.../YourMaterial"`.

---

## 4. Verification

1. **Default game mode:** Run **Scripts\EnsureSoccerSimDefaults.ps1** once if you have not already (sets default game mode).
2. **Paths:** Confirm `DefaultMetaHumanMeshPath`, `DefaultFootballMaterialPath`, and `DefaultPitchMaterialPath` in `Config/DefaultGame.ini` point to assets that exist (or use the conventional paths above and create assets there). Optionally set `DefaultMetaHumanAnimBlueprintPath` to an Anim BP that targets the MetaHuman skeleton.
3. **Build:** Close the editor. Build from command line or Visual Studio (see **Build and run** below). Reopen the project.
4. **Play:** Press Play on any level (including an empty/Untitled level). Confirm:
   - View is the **broadcast camera** (not the pawn).
   - **MetaHuman** players are visible (no placeholder cylinders).
   - **Football** has the intended look.
   - **Pitch** shows grass.
5. **Debug logs:** Agent debug logs are off unless `SOCCERSIM_AGENT_DEBUG` is enabled; no action needed for normal use.
6. **If players don't show the MetaHuman:** The game needs the **Skeletal Mesh** path, not the MetaHuman Character path (e.g. `/Game/MetaHuman1.MetaHuman1`). After "Create Full Rig" and **Export Combined Skeletal Mesh** in the MetaHuman Editor, the mesh is often saved as `/Game/MetaHuman1_CombinedSkelMesh`—that path is already set in config. Otherwise, in the Content Browser find the Skeletal Mesh (e.g. `MetaHuman1_CombinedSkelMesh` or `SK_...`), right-click → **Copy Reference**, and set `DefaultMetaHumanMeshPath="<paste>"` in `Config/DefaultGame.ini`. If you still see placeholders, check the **Output Log** for `MetaHuman mesh failed to load: /Game/...`.
7. **Clean restart after config changes:** From the project root, run **Scripts\Restart-SoccerSim.ps1** in PowerShell. This closes any running Unreal Editor and opens the project so config (e.g. `DefaultMetaHumanMeshPath`) is reloaded. Save your work in the editor before running.
8. **If the mesh shows but characters don't animate:** Set an Animation Blueprint that targets the MetaHuman skeleton—either use a Blueprint pawn with **Anim Class** set, or set `DefaultMetaHumanAnimBlueprintPath` in `DefaultGame.ini` to your MetaHuman Anim BP (Copy Reference).

---

## 5. Build and run

- **After C++ changes:** Close the editor, build, then reopen the project and Play.
- **Command line (from project root):**  
  `"C:\Program Files\Epic Games\Unreal 2\UE_5.7\Engine\Build\BatchFiles\Build.bat" SoccerSimEditor Win64 Development "c:\Users\byron\Cursor\SoccerSim\SoccerSim.uproject" -waitmutex`  
  (Adjust `UE_5.7` and paths for your install.)
- **Visual Studio:** Set configuration to **BuiltWithUnrealBuildTool**, platform **Win64**, then build the game target. Close the editor before building.
- **Lighting:** Directional light, sky light, and height fog are spawned at runtime, so an empty level is well lit without placing lights in the editor.

---

## Summary

| Asset        | Conventional path                    | Config key                          |
| ------------ | ------------------------------------ | ----------------------------------- |
| MetaHuman SK | `/Game/MetaHumans/MH_Soccer/SK_MH_Soccer` | `DefaultMetaHumanMeshPath`          |
| MetaHuman Anim BP (optional) | `/Game/Anim/ABP_MetaHuman_Soccer` | `DefaultMetaHumanAnimBlueprintPath` |
| Football mat | `/Game/Art/Ball/M_Football`          | `DefaultFootballMaterialPath`       |
| Grass mat    | `/Game/Art/Pitch/M_Grass`            | `DefaultPitchMaterialPath`           |

Create assets at these paths (or set the config keys to your paths), then build and Play for the best product look.
