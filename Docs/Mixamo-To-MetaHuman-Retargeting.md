# Mixamo to MetaHuman IK Retargeting Guide

This guide covers the complete process of retargeting Mixamo soccer animations to MetaHuman skeleton in UE5.

## Prerequisites

- UE 5.6+ (IK Retargeter is built-in)
- MetaHuman character already assembled in project
- Mixamo animations downloaded (FBX format)
- Enable **IK Rig** and **Animation Modifiers** plugins if not already enabled

## Phase 1: Import Mixamo Animations

### Step 1: Download Mixamo Animations
Your animations are already in `Content/Anim/Mixamo/`:
- Offensive Idle.fbx
- Jog Forward.fbx / Jog Backward.fbx
- Jog Strafe Left.fbx / Jog Strafe Right.fbx
- Jog Backward Diagonal.fbx
- Dribble.fbx
- Kick Soccerball.fbx
- Soccer Header.fbx
- Soccer Spin.fbx
- Strike Forward Jog.fbx

### Step 2: Import FBX Files (if not already imported)
1. In Content Browser, navigate to `Content/Anim/Mixamo/`
2. Right-click → **Import** → Select all FBX files
3. In the FBX Import dialog, set:
   - **Skeletal Mesh**: Uncheck (we only need the animations)
   - **Animation**: Check ✓
   - **Import Mesh**: Uncheck
   - **Import Animations**: Check ✓
   - **Animation Length**: Exported Time
   - **Use Default Sample Rate**: Check ✓
   - **Convert Scene**: Check ✓

4. Click **Import All**

### Step 3: Set Mixamo Skeleton for Retargeting
Mixamo uses the standard XBot/YBot skeleton. Before retargeting, ensure the skeleton is configured:

1. Find your imported skeleton (e.g., `SK_Mixamo_Skeleton` or similar)
2. Open the **Skeleton Editor** (double-click the skeleton asset)
3. In the Asset Browser, verify animations imported correctly

## Phase 2: Create IK Rigs

### Step 1: Create IK Rig for Mixamo (Source)
1. Content Browser → **Add(+) → Animation → IK Rig**
2. Name it: `IKR_Mixamo`
3. Open the IK Rig asset
4. Click **Add Bone Retarget Chain** for each major chain:
   - **Spine**: Root → Hips → Spine → Spine1 → Spine2
   - **Head**: Spine2 → Neck → Head
   - **LeftArm**: Spine2 → Clavicle → Shoulder → Elbow → Hand
   - **RightArm**: Spine2 → Clavicle_r → Shoulder_r → Elbow_r → Hand_r
   - **LeftLeg**: Hips → UpLeg → Leg → Foot → Toe
   - **RightLeg**: Hips → UpLeg_r → Leg_r → Foot_r → Toe_r

5. Set **Retarget Mode** for each chain:
   - Spine/Head: **Mode: Relative** (maintains pose)
   - Arms/Legs: **Mode: Absolute** (IK-driven)

6. Add **IK Goals** (optional, for foot/hand placement):
   - Add IK Goal for each foot and hand
   - Connect to corresponding bone chains

### Step 2: Create IK Rig for MetaHuman (Target)
1. Find your MetaHuman skeleton: `/Game/MetaHuman1_CombinedSkelMesh`
2. Content Browser → **Add(+) → Animation → IK Rig**
3. Name it: `IKR_MetaHuman`
4. Open and add the same bone chains with MetaHuman bone names:
   - **Spine**: Root → pelvis → spine_01 → spine_02 → spine_03
   - **Head**: spine_03 → neck_01 → head
   - **LeftArm**: spine_03 → clavicle_l → upperarm_l → lowerarm_l → hand_l
   - **RightArm**: spine_03 → clavicle_r → upperarm_r → lowerarm_r → hand_r
   - **LeftLeg**: pelvis → thigh_l → calf_l → foot_l → ball_l
   - **RightLeg**: pelvis → thigh_r → calf_r → foot_r → ball_r

5. Match chain names exactly with the Mixamo IK Rig (important for auto-mapping)

## Phase 3: Create IK Retargeter

### Step 1: Create the Retargeter
1. Content Browser → **Add(+) → Animation → IK Retargeter**
2. Name it: `IKR_MixamoToMetaHuman`
3. Open the IK Retargeter asset
4. In Details panel:
   - **Source IKRig**: Select `IKR_Mixamo`
   - **Target IKRig**: Select `IKR_MetaHuman`

### Step 2: Configure Chain Mapping
1. The retargeter should auto-map chains with matching names
2. Verify in the **Chain Mapping** section that all chains are paired
3. If any are missing, manually drag from source to target chain

### Step 3: Adjust Retargeting Settings
1. In the **Settings** tab:
   - **Root Bone Mode**: **Remap Root** (keeps character grounded)
   - **Strip Scale from Target**: Check ✓

2. Test with a preview animation:
   - Select a source animation from the dropdown
   - Verify the MetaHuman preview animates correctly
   - Adjust foot offset if feet clip through ground

### Step 4: Export Retargeted Animations
1. In IK Retargeter, click **Export Selected Animation**
2. Choose output folder: `Content/Anim/Retargeted/`
3. Repeat for all 11 animations:
   - `AM_OffensiveIdle_MetaHuman`
   - `AM_JogForward_MetaHuman`
   - `AM_JogBackward_MetaHuman`
   - `AM_JogStrafeLeft_MetaHuman`
   - `AM_JogStrafeRight_MetaHuman`
   - `AM_JogBackwardDiagonal_MetaHuman`
   - `AM_Dribble_MetaHuman`
   - `AM_KickSoccerball_MetaHuman`
   - `AM_SoccerHeader_MetaHuman`
   - `AM_SoccerSpin_MetaHuman`
   - `AM_StrikeForwardJog_MetaHuman`

## Phase 4: Create Blendspace

### Step 1: Create Movement Blendspace
1. Content Browser → `Content/Anim/Blendspaces/`
2. Right-click → **Animation → Blend Space**
3. Select the MetaHuman skeleton
4. Name it: `BS_8DirectionalMovement`

### Step 2: Configure Axis Parameters
1. Open the Blendspace
2. In **Axis Settings**:
   - **Horizontal Axis**: `Direction` (Range: -180 to 180)
   - **Vertical Axis**: `Speed` (Range: 0 to 600)

3. Add animation samples in a grid pattern:

```
                    Speed (0-600)
                        ↑
    [BackLeft]    [Backward]    [BackRight]
         \            |            /
          \           |           /
    [Left]----[Idle/Center]----[Right]
          /           |           \
         /            |            \
    [ForwardLeft] [Forward]  [ForwardRight]
                        ↓
                   Direction (-180 to 180)
```

### Step 3: Place Animation Samples
Map animations to positions in the blendspace:

| Animation | Direction | Speed | Position |
|-----------|-----------|-------|----------|
| Offensive Idle | 0 | 0 | Center |
| Jog Forward | 0 | 600 | (0, 600) |
| Jog Backward | 180 | 400 | (180, 400) |
| Jog Strafe Left | -90 | 400 | (-90, 400) |
| Jog Strafe Right | 90 | 400 | (90, 400) |
| Jog Backward Diagonal | 135 | 350 | (135, 350) |

For missing diagonals, you can:
- Create additional animations in Mixamo
- Use existing animations and let the blendspace interpolate
- Duplicate and mirror animations for opposite diagonals

### Step 4: Configure Blending
1. Enable **Smooth Blending**
2. Set **Interpolation Time**: 0.2 seconds
3. Set **Sample Interpolation**: Linear or Cubic

## Phase 5: Create Animation Blueprint

### Step 1: Create ABP_SoccerPlayer
1. Content Browser → `Content/Anim/`
2. Right-click → **Animation → Animation Blueprint**
3. Select MetaHuman skeleton
4. Name it: `ABP_SoccerPlayer`
5. Parent class: `USoccerAnimInstance` (your existing C++ class)

### Step 2: Build the Event Graph
In the Animation Blueprint Event Graph:

1. **Get Variables from Parent** (already provided by SoccerAnimInstance):
   - `MovementSpeed` (float, 0-600+)
   - `MovementDirection` (FVector2D, -1 to 1)
   - `bIsSprinting` (bool)
   - `bHasBall` (bool)

2. **Convert Direction to Degrees**:
   - Use `Atan2(MovementDirection.Y, MovementDirection.X)` → Convert to degrees
   - Or use `Direction = FMath::RadiansToDegrees(FMath::Atan2(Y, X))`

### Step 3: Build the Anim Graph
Create a State Machine:

```
[Locomotion State Machine]
         │
    ┌────┴────┐
    │         │
  [Idle]   [Moving]
    │         │
    │    ┌────┼────┐
    │    │    │    │
    │ [Jog] [Dribble] [Sprint]
    │    │    │    │
    └────┴────┴────┘
         │
    [Kick Action] (Slot: "Kick")
         │
    [Header Action] (Slot: "Header")
```

#### State Transitions:
- **Idle → Moving**: `MovementSpeed > 10`
- **Moving → Idle**: `MovementSpeed < 5`
- **Jog → Dribble**: `bHasBall && MovementSpeed > 50`
- **Dribble → Jog**: `!bHasBall`
- **Jog → Sprint**: `bIsSprinting && MovementSpeed > 500`
- **Sprint → Jog**: `!bIsSprinting`

#### Blendspace Usage:
In the Moving state:
1. Add the `BS_8DirectionalMovement` blendspace
2. Connect:
   - **Direction** → Horizontal input (converted to degrees -180 to 180)
   - **Speed** → Vertical input (MovementSpeed clamped 0-600)

## Phase 6: Configure Project

### Update DefaultGame.ini
Add these settings to `Config/DefaultGame.ini`:

```ini
[/Script/SoccerSim.SoccerPlayerPawn]
; Path to MetaHuman skeletal mesh
DefaultMetaHumanMeshPath="/Game/MetaHuman1_CombinedSkelMesh"

; Path to the new animation blueprint
DefaultMetaHumanAnimBlueprintPath="/Game/Anim/ABP_SoccerPlayer.ABP_SoccerPlayer"

; Optional: skin material
DefaultMetaHumanSkinMaterialPath="/Game/MetaHumans/MetaHuman1/Body/Materials/MI_Body_Baked_VT"
```

## Quick Reference: File Structure

```
Content/
├── Anim/
│   ├── Mixamo/                    # Source animations (FBX)
│   │   ├── Offensive Idle.fbx
│   │   ├── Jog Forward.fbx
│   │   └── ... (11 animations)
│   │
│   ├── Retargeted/                # IK Retargeted animations
│   │   ├── AM_OffensiveIdle_MetaHuman.uasset
│   │   ├── AM_JogForward_MetaHuman.uasset
│   │   └── ... (11 retargeted)
│   │
│   ├── Blendspaces/
│   │   └── BS_8DirectionalMovement.uasset
│   │
│   ├── IK/
│   │   ├── IKR_Mixamo.uasset      # Source IK Rig
│   │   ├── IKR_MetaHuman.uasset   # Target IK Rig
│   │   └── IKR_MixamoToMetaHuman.uasset  # Retargeter
│   │
│   └── ABP_SoccerPlayer.uasset    # Main animation blueprint
```

## Troubleshooting

### Feet Sliding
- Adjust the **Speed** parameter range in blendspace
- Add more animation samples for smoother transitions
- Check retargeter foot offset settings

### Animation Popping
- Increase blend time between states (0.15-0.25s)
- Use sync markers for cyclic animations
- Check animation compression settings

### Wrong Bone Mapping
- Verify bone names in both IK Rigs
- Check chain mapping in retargeter
- Ensure both skeletons use same coordinate system

### Characters Still T-Posing
1. Verify `DefaultMetaHumanAnimBlueprintPath` in config
2. Check Animation Blueprint is using correct skeleton
3. Ensure blendspace has valid animations assigned
4. Check Output Log for animation errors

## Additional Resources

- [Epic Games: IK Retargeting in UE5](https://docs.unrealengine.com/5.0/en-US/ik-rig-animation-retargeting-in-unreal-engine/)
- [Epic Games: Retargeting to MetaHumans](https://dev.epicgames.com/documentation/en-US/metahuman/retargeting-animation-blueprints-to-metahumans-in-unreal-engine/)
- [Mixamo Animation Best Practices](https://help.mixamo.com/hc/en-us/articles/231613367-Character-and-animation-best-practices)
