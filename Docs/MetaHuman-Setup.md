# MetaHuman Setup for SoccerSim

For the full **best-product** checklist (MetaHuman + ball + grass), see **Best-Product-Setup.md**.

## First-run checklist (one-time)

After this one-time setup, the game will show MetaHuman characters instead of the default mannequin.

1. Open the project in Unreal Editor.
2. Create one **MetaHuman Character** (Content Browser → Miscellaneous → MetaHuman Character). Name it e.g. `MH_Soccer_01`.
3. Open it in **MetaHuman Creator** (in-editor), customize if desired, then click **Assemble**. If prompted about overwriting, rename or choose another folder.
4. In the Content Browser, find the assembled **Skeletal Mesh** (e.g. `SK_...`) and copy its asset path (right-click → Copy Reference).
5. Either:
   - **Option A:** In `Config/DefaultGame.ini`, add under `[/Script/SoccerSim.SoccerPlayerPawn]`:  
     `DefaultMetaHumanMeshPath="/Game/.../SK_..."` (paste your path).
   - **Option B:** Create a Blueprint that inherits from `SoccerPlayerPawn`, set **MetaHumanMeshPath** to that mesh, and set this Blueprint as the default Pawn class in Project Settings → Maps & Modes (or on your Game Mode).
6. Retarget run/sprint/kick animations to the MetaHuman rig (see [Epic: Retargeting to MetaHumans](https://dev.epicgames.com/documentation/en-US/metahuman/retargeting-animation-blueprints-to-metahumans-in-unreal-engine/)).

After step 5, the game will use your MetaHuman mesh for all player pawns (config) or for the default pawn class (Blueprint).

---

## Research summary

- **MetaHuman is free** with Unreal Engine for creators under $1M revenue/year ([Epic license](https://metahuman.unrealengine.com)).
- **UE 5.7:** MetaHuman Creator is **built into Unreal Engine 5.6+**; no separate web app. You create and assemble MetaHumans in the editor ([Exporting MetaHumans to UE5](https://dev.epicgames.com/documentation/en-us/metahuman/exporting-metahumans-to-unreal-engine-5)).
- **Workflow:** Create a MetaHuman Character asset in the Content Browser → open in MetaHuman Creator (in-editor) → customize (face, body, hair, clothing) → **Assemble** (UE Cine pipeline). This produces a character Blueprint and assets (including the Skeletal Mesh) in the project.
- **Known 5.7 issue:** When assembling, avoid overwriting existing Actor Blueprints. Rename your MetaHuman asset or choose a different target directory if prompted ([Known Issues 5.7](https://dev.epicgames.com/documentation/en-us/metahuman/known-issues-5-7)).
- **C++ use:** The assembled character has a **Skeletal Mesh** (e.g. `SK_...`). SoccerSim’s `SoccerPlayerPawn` can load that mesh via **MetaHumanMeshPath** (or via config) and call `GetMesh()->SetSkeletalMesh(...)` in `BeginPlay`.

---

## Step-by-step: Get one human in the game

1. **Open the project** in Unreal Editor (SoccerSim.uproject).
2. **Content Browser:** Right-click → **Miscellaneous** → **MetaHuman Character** (or **Create Advanced Asset** → MetaHuman Character). Name it e.g. `MH_Soccer_01`.
3. **Double-click** the MetaHuman Character asset to open **MetaHuman Creator** (in-editor).
4. **Customize** (face, body, hair, clothing) as desired. Use a preset for speed.
5. **Assemble:** Click **Assemble** (UE Cine pipeline). If you get a warning about overwriting, rename the asset or choose another folder, then Assemble again.
6. After assembly, the project will contain a **Blueprint** and related assets. In the Content Browser, locate the **Skeletal Mesh** for this MetaHuman (e.g. under the same folder, name often `SK_...` or similar). **Copy its asset path:** right-click → **Copy Reference** (or note the path, e.g. `/Game/MetaHumans/MH_Soccer_01/SK_MH_Soccer_01`).
7. **Assign the mesh to the game:**
   - **Option A (Blueprint):** Create or open a Blueprint that inherits from `SoccerPlayerPawn`. Set **MetaHumanMeshPath** to the Skeletal Mesh asset you copied. Set this Blueprint as the **Default Pawn Class** in your Game Mode (or in **Project Settings → Maps & Modes** if you use a Blueprint Game Mode).
   - **Option B (Config):** In `Config/DefaultGame.ini`, under a section `[/Script/SoccerSim.SoccerPlayerPawn]`, add:
     ```ini
     DefaultMetaHumanMeshPath="/Game/MetaHumans/MH_Soccer_01/SK_MH_Soccer_01"
     ```
     (Use the actual path from step 6.) The pawn will load this mesh at runtime if the path is valid.
8. **Retarget animations:** Existing run, sprint, and kick animations are authored for the default mannequin. To use them on the MetaHuman rig, follow Epic’s [Retargeting Animation Blueprints to MetaHumans](https://dev.epicgames.com/documentation/en-US/metahuman/retargeting-animation-blueprints-to-metahumans-in-unreal-engine/) (IK Retargeter). Our **SoccerAnimInstance** and animation slots (e.g. run, sprint, kick montages) stay the same; only the skeleton and mesh change.

---

## Assigning the mesh (summary)

- **MetaHumanMeshPath** on `SoccerPlayerPawn`: set in a Blueprint subclass or on the default Pawn class used by the Game Mode.
- **DefaultMetaHumanMeshPath** in `Config/DefaultGame.ini`: optional; if set, the pawn uses this path when no per-class path is set, so one MetaHuman can drive all players.
- **DefaultMetaHumanAnimBlueprintPath** in `Config/DefaultGame.ini`: optional; when the MetaHuman mesh is loaded, the pawn will use this Animation Blueprint (targeting the MetaHuman skeleton, with retargeted run/sprint/kick) if the path is set. Use Copy Reference from the Anim BP asset.

---

## Optional: Default player mesh (Mannequin)

If you want character-shaped players without creating a MetaHuman, you can use the **UE5 Mannequin** (e.g. Quinn) from the Third Person template:

1. Create a **Third Person** template project in the Epic Games Launcher (or use an existing one).
2. In that project, open **Content/Characters/Mannequins/Meshes** and find the skeletal mesh (e.g. `SK_Quinn`).
3. In the Content Browser: right-click the **Characters** folder (or the Mannequins folder) → **Migrate** → choose your SoccerSim project’s Content folder. This copies the Mannequin assets into SoccerSim.
4. In SoccerSim, in `Config/DefaultGame.ini`, under `[/Script/SoccerSim.SoccerPlayerPawn]`, add:
   ```ini
   DefaultPlayerMeshPath="/Game/Characters/Mannequins/Meshes/SKM_Quinn"
   ```
   (Use the actual path after migration; it may be `SK_Quinn` or `SKM_Quinn` depending on the template.)
5. Restart the editor and press Play. All 22 players will use the Mannequin mesh. If the path is missing or invalid, the game falls back to team-colored placeholder boxes.

**DefaultPlayerMeshPath** is tried only when **MetaHumanMeshPath** / **DefaultMetaHumanMeshPath** are not set; it does not require the MetaHuman plugin.

---

## Retargeting (existing AnimBP and slots)

- **SoccerAnimInstance** drives locomotion (speed → blend run/sprint). It expects a compatible skeleton.
- **Kick montages** and **AnimNotify_KickContact** are used for shooting and passing. After retargeting to the MetaHuman rig, ensure the notify and montage slots still fire correctly.
- Epic’s doc above covers creating an IK Retargeter from the source (mannequin) to the MetaHuman and applying it to the Animation Blueprint.

---

## Troubleshooting

1. **Placeholders (cylinders) instead of MetaHuman:** Confirm the path in config matches the **Skeletal Mesh** (SK_...) exactly. In the Content Browser, right-click the **Skeletal Mesh** (SK_...), not the MetaHuman asset → **Copy Reference**, then paste into `DefaultMetaHumanMeshPath` in `Config/DefaultGame.ini`. After changing config, run **Scripts\Restart-SoccerSim.ps1** then Play. If it still fails, check the **Output Log** for a warning like `MetaHuman mesh failed to load: /Game/...` to confirm the path or asset is wrong.
2. **Config not applied:** After editing `DefaultGame.ini`, fully close the editor and run **Scripts\Restart-SoccerSim.ps1** (or reopen the project manually) so config is reloaded from disk.
3. **Mesh shows but no animation (T-pose or statue):** The mesh component needs an Animation Blueprint that targets the MetaHuman skeleton. Either use a Blueprint pawn with the mesh component’s **Anim Class** set to your MetaHuman Anim BP, or set `DefaultMetaHumanAnimBlueprintPath` in `DefaultGame.ini` to your Anim BP asset path (Copy Reference from the Anim BP).
