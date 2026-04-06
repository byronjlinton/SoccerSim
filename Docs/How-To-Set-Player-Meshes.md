# How to Set Default Player Meshes

## You don't have to set anything

**By default, all 22 players are visible as team-colored placeholder boxes** (red = Home, blue = Away). No config, no editor steps. Just open the project and press Play.

---

## If you want real character meshes instead of placeholders

You have two options. **Only one** needs to be set.

### Option A: MetaHuman (recommended for best look)

1. In the editor: Content Browser → create **MetaHuman Character** → open in MetaHuman Creator → **Assemble**.
2. In Content Browser, find the assembled **Skeletal Mesh** (e.g. `SK_...`). Right-click it → **Copy Reference**.
3. Open `Config/DefaultGame.ini` in a text editor.
4. Add or uncomment this section and paste your path:
   ```ini
   [/Script/SoccerSim.SoccerPlayerPawn]
   DefaultMetaHumanMeshPath="/Game/YourFolder/SK_YourMetaHuman"
   ```
5. Save, restart the editor if it was open, then Play. All players will use that mesh.

See **Docs/MetaHuman-Setup.md** for full MetaHuman steps and retargeting.

### Option B: UE Mannequin (e.g. Quinn from Third Person template)

1. Migrate the Mannequin from a Third Person template project into `Content/Characters/Mannequins/` (Content Browser → right-click folder → Migrate → choose SoccerSim).
2. Open `Config/DefaultGame.ini`.
3. Add or uncomment:
   ```ini
   [/Script/SoccerSim.SoccerPlayerPawn]
   DefaultPlayerMeshPath="/Game/Characters/Mannequins/Meshes/SKM_Quinn"
   ```
   (Use the actual path after migration; it may be `SK_Quinn` or similar.)
4. Save, restart editor if needed, then Play.

---

## Summary

| What you want              | What to do |
|----------------------------|------------|
| Players visible, no setup  | Nothing. Placeholders are used automatically. |
| Real MetaHuman characters  | Set `DefaultMetaHumanMeshPath` in `Config/DefaultGame.ini` (see Option A). |
| Real Mannequin characters  | Set `DefaultPlayerMeshPath` in `Config/DefaultGame.ini` (see Option B). |

Only one of `DefaultMetaHumanMeshPath` or `DefaultPlayerMeshPath` is used; MetaHuman is tried first. If neither is set or the path is invalid, placeholders are shown.
