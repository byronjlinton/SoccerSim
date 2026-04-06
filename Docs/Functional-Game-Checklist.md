# Functional Game Checklist

Use this checklist to get a playable match with input, HUD (score/clock), and optional character skin. All editor steps must be done by you in Unreal Editor; the agent cannot drive the editor.

---

## 1. One-time: Enhanced Input (Session 7)

Follow **Docs/Session7-Enhanced-Input-Checklist.md** so that:

- Enhanced Input plugin is enabled and default input classes are set.
- Input Actions and **IMC_SoccerDefault** exist in `Content/Input/`.
- **BP_SoccerPlayerController** has all IA_* and **DefaultMappingContext** assigned.
- **BP_SoccerGameMode** uses BP_SoccerPlayerController and your pawn/ball/field/goal classes.
- Your test map’s **World Settings → GameMode Override** is **BP_SoccerGameMode**.

After this, movement (WASD / stick), pass, shoot, switch player, and view toggle (V) work.

---

## 2. Optional: HUD on screen (score, clock, phase)

1. Create a **Widget Blueprint** (e.g. **WBP_SoccerHUD**) with **Parent Class** = **SoccerHUDWidget**.
2. In the widget, add text blocks and bind them to the C++ functions: **GetHomeScore**, **GetAwayScore**, **GetMatchClockText**, **GetCurrentPhaseText** (or use **GetSoccerGameState** for full access).
3. Open **BP_SoccerPlayerController** → **Class Defaults** → set **HUD Widget Class** to **WBP_SoccerHUD** (or your widget asset).

On Play, the controller will create the widget and add it to the viewport so the HUD is visible.

---

## 3. Optional: Skin material (characters instead of topology)

If players use the MetaHuman mesh but appear as wireframe/topology, set a skin material in config:

1. In Content Browser, locate the **character material** for your MetaHuman (e.g. **M_Character** under the MetaHuman folder).
2. Right-click it → **Copy Reference**.
3. Open **Config/DefaultGame.ini** and find the section `[/Script/SoccerSim.SoccerPlayerPawn]`.
4. Uncomment and set:
   ```ini
   DefaultMetaHumanSkinMaterialPath="/Game/MetaHumans/MetaHuman1/M_Character"
   ```
   (Use the path you copied.)

The game applies this material to all material slots on the player mesh when using the MetaHuman mesh.

---

## 4. Lumen / distance fields

**Config/DefaultEngine.ini** already has **Generate Mesh Distance Fields** enabled under `[/Script/Engine.RendererSettings]` (`r.GenerateMeshDistanceFields=True`). This helps Lumen use software distance fields. If you see “no ray tracing data”–style warnings, ensure that option stays enabled and that static meshes have **Generate Distance Field** enabled in their asset settings if needed.

---

## 5. Full match (two halves, ball out, reposition)

The game now supports:

- **Second half:** After the first half ends (match clock reaches half duration), phase goes to **Half Time**. After a configurable **Halftime Duration** (GameMode: `HalftimeDurationSeconds`, default 15 s), **Second Half Kick Off** runs, then **Second Half** (clock resets and runs again) until **Full Time**.
- **Ball out of play:** Throw-in, goal kick, and corner are detected; the ball is placed at the correct spot and reset (velocity cleared). Play continues.
- **Reset to kick-off:** After a goal, after 3 seconds the ball is reset to center and **all 22 players are repositioned** to their formation slots.
- **Score broadcast:** `OnScoreChanged` is broadcast when a goal is scored so UI can update.

No extra setup required; use **BP_SoccerGameMode** and optionally tune **Half Duration Seconds** and **Halftime Duration Seconds** in the GameMode defaults.

---

## 6. AI re-possession on switch

When you **switch player** (e.g. Tab / LB), the pawn you were controlling is given back to its **AI controller**, so all non-human players stay AI-driven. No setup required.

---

## 7. Menus and full game flow (optional)

For main menu → match → pause (Escape) → match end (Rematch / Main Menu), see **Docs/Menus-And-Flow.md**. You will:

- Set **Game Instance Class** to **SoccerGameInstance** (Project Settings).
- Set **Game Default Map** to your main menu map.
- Create main menu and match maps and wire **Open Match Map** / **Quit To Main Menu** / **Rematch** from widgets to the Player Controller.

---

## 8. Verify: functional game

1. Open the project in Unreal Editor.
2. Open your test map and set **World Settings → GameMode Override** to **BP_SoccerGameMode** (if not already).
3. **Play**.
4. Confirm:
   - You can **move** (WASD or gamepad).
   - **Pass** and **shoot** affect the ball.
   - **Score** and **clock** are visible if you set **HUD Widget Class** on BP_SoccerPlayerController.
   - **V** toggles between broadcast camera and possessed player.
   - **Switch player** (Tab) leaves the previous player under AI control.
   - First half ends → Half Time → after halftime duration, second half runs to Full Time.
   - Ball out (touchline / goal line) places the ball and play continues.
   - After a goal, ball and all players reset to formation.

For asset paths (mesh, ball, pitch, MetaHuman), see **Docs/Best-Product-Setup.md**.
