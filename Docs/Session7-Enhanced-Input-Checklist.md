# Session 7: Enhanced Input — Editor Checklist

This session is **editor-only**. Follow these steps in the Unreal Editor after the C++ project compiles. Until you complete them, movement and actions may not work (Enhanced Input assets will be missing).

---

## Step 7.1: Enable Enhanced Input Plugin

1. **Edit → Plugins** → search for **"Enhanced Input"** → ensure it is **Enabled**.
2. **Edit → Project Settings → Input**:
   - **Default Input Component Class** = `EnhancedInputComponent`
   - **Default Player Input Class** = `EnhancedPlayerInput`

---

## Step 7.2: Create Input Actions (Content/Input/)

In the Content Browser, right-click → **Input → Input Action**. Create one asset for each row below:

| Asset Name   | Value Type   | Notes            |
|-------------|--------------|------------------|
| IA_Move     | Axis2D (Vector2D) | Left stick / WASD |
| IA_Sprint   | Digital (Bool)   | RT / Left Shift   |
| IA_Pass     | Digital (Bool)   | A / Spacebar      |
| IA_Shoot    | Digital (Bool)   | B / Left Mouse    |
| IA_ThroughBall | Digital (Bool) | Y / E             |
| IA_Tackle   | Digital (Bool)   | X / Q             |
| IA_SwitchPlayer | Digital (Bool) | LB / Tab        |
| IA_Lob      | Digital (Bool)   | RB / R            |
| IA_ViewToggle | Digital (Bool) | V — toggle broadcast vs player camera |

Save all in a folder such as `Content/Input/`.

---

## Step 7.3: Create Input Mapping Context

Right-click in Content Browser → **Input → Input Mapping Context** → name it **IMC_SoccerDefault**.

Add mappings:

- **IA_Move** → W (Swizzle YXZ, Negate Y), S (Swizzle YXZ), A (Negate X), D  
- **IA_Move** → Gamepad Left Stick (add Dead Zone modifier, radius 0.2)
- **IA_Sprint** → Left Shift, Gamepad Right Trigger
- **IA_Pass** → Spacebar, Gamepad Face Button Bottom
- **IA_Shoot** → Left Mouse Button, Gamepad Face Button Right
- **IA_ThroughBall** → E, Gamepad Face Button Top
- **IA_Tackle** → Q, Gamepad Face Button Left
- **IA_SwitchPlayer** → Tab, Gamepad Left Shoulder
- **IA_Lob** → R, Gamepad Right Shoulder
- **IA_ViewToggle** → V (toggle between broadcast camera and possessed player view)

---

## Step 7.4: Blueprint Subclass of PlayerController

1. Create Blueprint: **BP_SoccerPlayerController** (parent: **SoccerPlayerController**).
2. In **Class Defaults**, assign:
   - **DefaultMappingContext** → IMC_SoccerDefault
   - **IA_Move** → IA_Move
   - **IA_Sprint** → IA_Sprint
   - **IA_Pass** → IA_Pass
   - **IA_Shoot** → IA_Shoot
   - **IA_ThroughBall** → IA_ThroughBall
   - **IA_Tackle** → IA_Tackle
   - **IA_SwitchPlayer** → IA_SwitchPlayer
   - **IA_Lob** → IA_Lob
   - **IA_ViewToggle** → IA_ViewToggle

---

## Step 7.5: Blueprint Subclass of GameMode

1. Create Blueprint: **BP_SoccerGameMode** (parent: **SoccerGameMode**).
2. In **Class Defaults**, assign:
   - **Player Controller Class** → BP_SoccerPlayerController
   - **Ball Class** → SoccerBall (or a BP_SoccerBall if you create one)
   - **Player Pawn Class** → SoccerPlayerPawn (or BP_SoccerPlayerPawn)
   - **Field Class** → SoccerField (or BP_SoccerField)
   - **Goal Class** → SoccerGoal (or BP_SoccerGoal)

---

## Step 7.6: World Settings

1. Open your test map.
2. **Window → World Settings**.
3. Set **GameMode Override** → **BP_SoccerGameMode**.

---

After completing this checklist, input should work and the game will use your Blueprint GameMode and PlayerController with the Enhanced Input assets.
