# Unreal Editor Step-by-Step Guide: Animation Setup

This is a **follow-along guide** for setting up the animation system in Unreal Editor. Open UE5 and follow each step.

---

## STEP 1: Enable Required Plugins

1. **Edit → Plugins**
2. Search for and enable:
   - **IK Rig** (Built-in)
   - **Animation Modifiers** (Built-in)
3. **Restart the editor** if prompted

---

## STEP 2: Import Mixamo Animations

Your FBX files are in `Content/Anim/Mixamo/`. If they're not yet imported:

1. **Content Browser** → Navigate to `Content/Anim/Mixamo/`
2. **Right-click** → **Import to /Game/Anim/Mixamo/**
3. Select all `.fbx` files (11 files)
4. In FBX Import Options:
   ```
   Mesh                    □ Uncheck (we only want animations)
   Skeleton                [Auto-detect or select existing]
   Animation               
     Import Animations     ✓ Check
     Animation Length      "Exported Time"
     Use Default Sample Rate ✓ Check
     Convert Scene         ✓ Check
   ```
5. Click **Import All**
6. After import, you should see:
   - `SK_Mixamo_Skeleton` (or similar skeleton asset)
   - 11 animation sequences

---

## STEP 3: Create IK Rig for Mixamo (Source)

1. **Content Browser** → Right-click → **Add(+) → Animation → IK Rig**
2. Name it: `IKR_Mixamo`
3. **Double-click** to open

### Add Bone Retarget Chains:

In the **IK Rig** panel:

| Chain Name | Bones (select from skeleton tree) |
|------------|-----------------------------------|
| Spine | Root → Hips → Spine → Spine1 → Spine2 |
| Head | Spine2 → Neck → Head |
| LeftArm | Spine2 → LeftShoulder → LeftArm → LeftForeArm → LeftHand |
| RightArm | Spine2 → RightShoulder → RightArm → RightForeArm → RightHand |
| LeftLeg | Hips → LeftUpLeg → LeftLeg → LeftFoot → LeftToeBase |
| RightLeg | Hips → RightUpLeg → RightLeg → RightFoot → RightToeBase |

### Set Retarget Modes:

For each chain, set **IK Retargeting Mode**:
- **Spine**: Relative
- **Head**: Relative
- **LeftArm**: Absolute
- **RightArm**: Absolute
- **LeftLeg**: Absolute
- **RightLeg**: Absolute

**Save** the IK Rig.

---

## STEP 4: Create IK Rig for MetaHuman (Target)

1. Find your MetaHuman skeleton: `/Game/MetaHuman1_CombinedSkelMesh`
2. **Right-click** the skeleton → **Create → IK Rig**
3. Name it: `IKR_MetaHuman`
4. **Double-click** to open

### Add Bone Retarget Chains (MetaHuman bone names):

| Chain Name | Bones |
|------------|-------|
| Spine | root → pelvis → spine_01 → spine_02 → spine_03 |
| Head | spine_03 → neck_01 → head |
| LeftArm | spine_03 → clavicle_l → upperarm_l → lowerarm_l → hand_l |
| RightArm | spine_03 → clavicle_r → upperarm_r → lowerarm_r → hand_r |
| LeftLeg | pelvis → thigh_l → calf_l → foot_l → ball_l |
| RightLeg | pelvis → thigh_r → calf_r → foot_r → ball_r |

### Set Retarget Modes (same as source):

- **Spine**: Relative
- **Head**: Relative
- **LeftArm**: Absolute
- **RightArm**: Absolute
- **LeftLeg**: Absolute
- **RightLeg**: Absolute

**Save** the IK Rig.

---

## STEP 5: Create IK Retargeter

1. **Content Browser** → Create folder `Content/Anim/IK/`
2. **Right-click** → **Add(+) → Animation → IK Retargeter**
3. Name it: `IKR_MixamoToMetaHuman`
4. **Double-click** to open

### Configure Retargeter:

In the **Details** panel:
- **Source IK Rig**: `IKR_Mixamo`
- **Target IK Rig**: `IKR_MetaHuman`

### Verify Chain Mapping:

The chains should auto-map. Check in the **Chain Mapping** section:
```
Spine    → Spine
Head     → Head
LeftArm  → LeftArm
RightArm → RightArm
LeftLeg  → LeftLeg
RightLeg → RightLeg
```

### Adjust Settings:

In **Settings** tab:
- **Root Bone Mode**: Remap Root
- **Strip Scale from Target**: ✓ Check

### Test Preview:

1. Click **Preview Animation** dropdown
2. Select one of your Mixamo animations
3. Verify the MetaHuman preview shows correct animation
4. If feet clip through ground, adjust **Foot Offset** in settings

**Save** the retargeter.

---

## STEP 6: Export Retargeted Animations

1. With `IKR_MixamoToMetaHuman` open
2. Create folder: `Content/Anim/Retargeted/`
3. For each animation:
   - Select source animation in the **Preview Animation** dropdown
   - Click **Export Selected Animation** button
   - Save to `Content/Anim/Retargeted/` with name: `AM_[Name]_MetaHuman`

**Animations to export:**
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

---

## STEP 7: Create 8-Directional Blendspace

1. Create folder: `Content/Anim/Blendspaces/`
2. **Right-click** → **Animation → Blend Space**
3. Select the **MetaHuman skeleton**
4. Name it: `BS_8DirectionalMovement`
5. **Double-click** to open

### Configure Axes:

In the **Asset Details** panel:

**Horizontal Axis:**
- Name: `Direction`
- Min: `-180`
- Max: `180`
- Grid Snap: `45`

**Vertical Axis:**
- Name: `Speed`
- Min: `0`
- Max: `700`
- Grid Snap: `100`

### Place Animations:

Drag animations from **Asset Browser** to the grid:

```
Speed
 700 │                                              [JogForward]
     │
 500 │         [StrafeLeft]              [StrafeRight]
     │
 300 │    [BackDiagonal]    [Backward]    
     │
 100 │
     │
   0 └───────────────────────────────────────────────── Direction
    -180    -90      0       90      180
                    [Idle]
```

**Exact positions:**

| Animation | Direction | Speed |
|-----------|-----------|-------|
| AM_OffensiveIdle_MetaHuman | 0 | 0 |
| AM_JogForward_MetaHuman | 0 | 600 |
| AM_JogBackward_MetaHuman | 180 | 400 |
| AM_JogStrafeLeft_MetaHuman | -90 | 400 |
| AM_JogStrafeRight_MetaHuman | 90 | 400 |
| AM_JogBackwardDiagonal_MetaHuman | 135 | 350 |

### Blending Settings:

- **Interpolation Time**: 0.2
- **Interpolation Type**: Linear

**Save** the blendspace.

---

## STEP 8: Create Animation Blueprint

1. **Content Browser** → `Content/Anim/`
2. **Right-click** → **Animation → Animation Blueprint**
3. Select **MetaHuman skeleton**
4. **Parent Class**: `SoccerAnimInstance` (your C++ class)
5. Name it: `ABP_SoccerPlayer`
6. **Double-click** to open

### Event Graph Setup:

The parent class already provides these variables:
- `MovementSpeed` (float)
- `MovementDirectionDegrees` (float, -180 to 180)
- `bIsSprinting` (bool)
- `bHasBall` (bool)
- `bIsMoving` (bool)
- `BlendSpaceInput` (Vector2D)

No additional Event Graph logic needed!

### Anim Graph Setup:

1. **Right-click** in Anim Graph → **Add New State Machine**
2. Name it: `LocomotionSM`
3. **Double-click** the state machine to open it

#### Create States:

1. **Idle** (entry state)
2. **Moving**
3. **Dribbling** (optional)
4. **Sprinting**

#### State Machine Layout:

```
[Entry] → [Idle] ←→ [Moving] ←→ [Sprinting]
                    ↓
              [Dribbling]
```

#### Configure States:

**Idle State:**
- Drag in `AM_OffensiveIdle_MetaHuman`
- Connect to **Output Pose**

**Moving State:**
- Drag in `BS_8DirectionalMovement` (your blendspace)
- Connect **MovementDirectionDegrees** → **Direction** input
- Connect **MovementSpeed** → **Speed** input
- Connect to **Output Pose**

**Dribbling State:**
- Drag in `AM_Dribble_MetaHuman`
- Connect to **Output Pose**

**Sprinting State:**
- Can reuse blendspace with higher speed multiplier
- Or use `AM_StrikeForwardJog_MetaHuman` for variety

#### Transitions:

| From | To | Condition | Duration |
|------|----|-----------|----------|
| Idle | Moving | `bIsMoving` | 0.15s |
| Moving | Idle | `!bIsMoving` | 0.2s |
| Moving | Dribbling | `bHasBall && bIsMoving` | 0.1s |
| Dribbling | Moving | `!bHasBall` | 0.1s |
| Moving | Sprinting | `bIsSprinting` | 0.1s |
| Sprinting | Moving | `!bIsSprinting` | 0.15s |

**Save** the animation blueprint.

---

## STEP 9: Add Action Slots (Kick, Header)

### Create Anim Slot:

1. **Content Browser** → **Add(+) → Animation → Anim Slot**
2. Name it: `ActionSlot`

### Add to Anim Graph:

In `ABP_SoccerPlayer`:

1. **Right-click** → search for **"Play Slot"**
2. Select **Slot: ActionSlot**
3. Place it **after** the Locomotion state machine:

```
[LocomotionSM] → [Slot 'ActionSlot'] → [Final Pose]
```

### Create Kick Montage:

1. **Content Browser** → `Content/Anim/Montages/`
2. **Right-click** on `AM_KickSoccerball_MetaHuman` → **Create → Anim Montage**
3. Name it: `AM_KickSoccerball_Montage`
4. Open and set **Slot Name** to: `ActionSlot`

The kick animation will now overlay on top of locomotion when played.

---

## STEP 10: Configure Project Settings

Edit `Config/DefaultGame.ini`:

```ini
[/Script/SoccerSim.SoccerPlayerPawn]
; MetaHuman skeletal mesh
DefaultMetaHumanMeshPath="/Game/MetaHuman1_CombinedSkelMesh"

; Animation blueprint
DefaultMetaHumanAnimBlueprintPath="/Game/Anim/ABP_SoccerPlayer.ABP_SoccerPlayer"

; Optional: skin material
DefaultMetaHumanSkinMaterialPath="/Game/MetaHumans/MetaHuman1/Body/Materials/MI_Body_Baked_VT"
```

**Restart the editor** after changing config.

---

## STEP 11: Test In-Game

1. **Play** in editor
2. Players should now show animated locomotion
3. Test:
   - Jog in all directions (8-way movement)
   - Sprint (hold sprint button)
   - Dribble (when player has ball)
   - Kick (when in range of ball)

---

## Troubleshooting Checklist

- [ ] IK Rig plugins enabled
- [ ] Both IK Rigs saved with matching chain names
- [ ] Retargeter has both rigs assigned
- [ ] All 11 animations retargeted to MetaHuman skeleton
- [ ] Blendspace uses MetaHuman skeleton
- [ ] Animation Blueprint uses `SoccerAnimInstance` as parent
- [ ] Config paths are correct (Copy Reference from assets)
- [ ] Editor restarted after config changes

---

## Quick Asset Reference

| Asset | Path | Description |
|-------|------|-------------|
| MetaHuman Skeleton | `/Game/MetaHuman1_CombinedSkelMesh` | Target skeleton |
| Mixamo IK Rig | `/Game/Anim/IK/IKR_Mixamo` | Source IK setup |
| MetaHuman IK Rig | `/Game/Anim/IK/IKR_MetaHuman` | Target IK setup |
| Retargeter | `/Game/Anim/IK/IKR_MixamoToMetaHuman` | Converts animations |
| Blendspace | `/Game/Anim/Blendspaces/BS_8DirectionalMovement` | 8-way movement |
| Anim Blueprint | `/Game/Anim/ABP_SoccerPlayer` | Main animation controller |
