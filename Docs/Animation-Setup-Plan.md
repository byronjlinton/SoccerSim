# Animation Setup Plan for SoccerSim

## Current State
- MetaHuman mesh exists: `/Game/MetaHuman1_CombinedSkelMesh`
- Post-process ABPs exist (body, face, clothing)
- **NO soccer animations** (run, sprint, kick, tackle, celebrate)
- **NO Animation Blueprint** for gameplay

## Immediate Fix (Today)

### Option A: Use MetaHuman's Built-in Body ABP
The MetaHuman comes with `ABP_Body_PostProcess` which handles:
- Cloth simulation
- Body deformations
- Basic pose

**Path:** `/Game/MetaHumans/Common/Body/ABP_Body_PostProcess`

This will at least make the characters not be T-pose, but they won't have run/kick animations.

### Option B: Quick Mixamo Integration
1. Download free soccer animations from Mixamo:
   - Run with ball
   - Sprint
   - Kick
   - Idle
   - Tackle

2. Retarget to MetaHuman skeleton in UE5
3. Create simple ABP_SoccerPlayer with blendspaces

### Option C: Use UE5 Mannequin Animations
UE5 has built-in animations that can be retargeted:
- ThirdPersonRun
- ThirdPersonWalk
- ThirdPersonIdle

## Recommended Path

**Short-term (today):**
1. Set `DefaultMetaHumanAnimBlueprintPath` to `/Game/MetaHumans/Common/Body/ABP_Body_PostProcess`
2. This will give basic body pose (not T-pose)
3. Characters will slide (no leg movement) but look better

**Medium-term (this week):**
1. Download Mixamo soccer animations
2. Retarget to MetaHuman skeleton
3. Create proper ABP_SoccerPlayer

## Animation List Needed

| Animation | Source | Priority |
|-----------|--------|----------|
| Idle | Mixamo/UE5 | High |
| Walk | Mixamo/UE5 | High |
| Jog | Mixamo | High |
| Sprint | Mixamo | High |
| Kick (various) | Mixamo | Medium |
| Tackle | Mixamo | Medium |
| Celebrate | Mixamo | Low |
| Header | Mixamo | Low |

## Mixamo Setup Steps

1. Go to mixamo.com (free with Adobe account)
2. Search for "soccer" animations
3. Download as "in place" for blendspaces
4. Import to UE5
5. Use IK Retargeter to convert to MetaHuman skeleton
6. Create blendspace for movement
7. Create ABP with state machine

## Files to Create

```
Content/
├── Anim/
│   ├── Mixamo/           # Raw Mixamo imports
│   ├── Retargeted/       # IK Retargeted to MetaHuman
│   ├── Blendspaces/
│   │   └── BS_Movement  # Idle-Walk-Jog-Sprint
│   └── ABP_SoccerPlayer # Main animation blueprint
```

## Config Update (Immediate)

```ini
[/Script/SoccerSim.SoccerPlayerPawn]
DefaultMetaHumanMeshPath="/Game/MetaHuman1_CombinedSkelMesh"
DefaultMetaHumanAnimBlueprintPath="/Game/MetaHumans/Common/Body/ABP_Body_PostProcess"
DefaultMetaHumanSkinMaterialPath="/Game/MetaHumans/MetaHuman1/Body/Materials/MI_Body_Baked_VT"
```

This will at least show properly textured MetaHumans (not wireframe) with basic pose.
