# SoccerSim Visual Roadmap - FIFA-Like Game

**Goal:** Create a visually stunning, realistic soccer game using only free assets (MetaHuman, Quixel, community resources).

---

## Phase 1: Foundation (Week 1)
**Goal:** Fix core visuals, make the game look polished at a basic level

### 1.1 MetaHuman Loading Fix
- [ ] Verify `MetaHuman1_CombinedSkelMesh` loads correctly
- [ ] Set up Animation Blueprint for MetaHuman skeleton
- [ ] Create retargeted run/sprint/kick animations
- [ ] Test 22 players on field with MetaHumans

### 1.2 Pitch Quality
- [ ] Create high-quality grass material using Quixel Megascans
- [ ] Add field markings (penalty box, center circle, etc.)
- [ ] Add subtle grass displacement/deformation
- [ ] Add pitch edge/turf transition

### 1.3 Ball Quality
- [ ] Create proper football material (white + black pentagons)
- [ ] Add ball rotation visualization
- [ ] Add ball trail effect when moving fast

### 1.4 Goals
- [ ] Create realistic net material (translucent grid)
- [ ] Add goal post shine/reflection
- [ ] Add net physics when ball hits

---

## Phase 2: Stadium (Week 2)
**Goal:** Create an immersive stadium environment

### 2.1 Stadium Geometry
- [ ] Create stadium stands (basic shape)
- [ ] Add seat rows with team colors
- [ ] Create stadium roof/cover
- [ ] Add sideline structures (benches, cameras)

### 2.2 Crowd System
- [ ] Research Niagara-based crowd (OverCrowd/PopcornFX patterns)
- [ ] Create LOD crowd meshes (3D close, 2D far)
- [ ] Add crowd reactions (cheer, boo, wave)
- [ ] Link crowd audio to events

### 2.3 Stadium Lighting
- [ ] Add stadium floodlights (visible geometry)
- [ ] Create light beams in fog/mist
- [ ] Add dynamic time-of-day (sunset match)
- [ ] Add lens flare from lights

---

## Phase 3: Effects (Week 3)
**Goal:** Add life and energy to the game

### 3.1 Particle Effects
- [ ] Grass spray when running/sliding
- [ ] Dirt/debris on impact
- [ ] Goal celebration confetti/paper
- [ ] Ball impact dust
- [ ] Sweat/water droplets (rain)

### 3.2 Weather System
- [ ] Rain effect (particles + material wetness)
- [ ] Puddles on pitch
- [ ] Dynamic sky (clouds, sun position)
- [ ] Wind effect on ball trajectory

### 3.3 Cloth Physics
- [ ] Enable GPU cloth on player kits
- [ ] Add wind response to jerseys
- [ ] Add sweat/dirt accumulation on kits

---

## Phase 4: Polish (Week 4)
**Goal:** Professional broadcast quality

### 4.1 Camera Work
- [ ] Multiple broadcast angles (sideline, goal, overhead)
- [ ] Smooth camera transitions
- [ ] Replay system (slow-mo)
- [ ] Camera shake on big events (goals, tackles)

### 4.2 Post-Processing
- [ ] Film grain for cinematic feel
- [ ] Chromatic aberration on fast motion
- [ ] Depth of field on close-ups
- [ ] Motion blur refinement
- [ ] Color grading LUT for broadcast look

### 4.3 UI/HUD
- [ ] Score overlay (broadcast style)
- [ ] Match timer
- [ ] Player name tags
- [ ] Team badges
- [ ] Commentary subtitles

---

## Free Asset Sources

| Asset Type | Source |
|------------|--------|
| Characters | MetaHuman Creator (built-in UE5.7) |
| Materials | Quixel Megascans (free with Epic account) |
| Particles | Niagara (built-in) |
| Audio | Freesound.org, Epic free audio packs |
| Textures | Quixel, ambientCG (CC0) |
| Animations | Mixamo (free with Adobe account) |

---

## Technical Notes

### Performance Targets
- 60 FPS on mid-range hardware
- 22 players + crowd + effects
- LOD system for distance optimization

### Key Files to Modify
- `Source/SoccerSim/Player/SoccerPlayerPawn.cpp` - MetaHuman loading
- `Source/SoccerSim/Field/SoccerField.cpp` - Pitch material
- `Source/SoccerSim/Ball/SoccerBall.cpp` - Ball material
- `Source/SoccerSim/Core/SoccerGameMode.cpp` - Stadium spawning
- `Source/SoccerSim/Camera/SoccerBroadcastCamera.cpp` - Camera work

---

## Progress Tracking

| Phase | Start | End | Status |
|-------|-------|-----|--------|
| 1. Foundation | 2026-03-09 | - | 🔄 80% Complete |
| 2. Stadium | - | - | ⏳ Pending |
| 3. Effects | - | - | ⏳ Pending |
| 4. Polish | - | - | ⏳ Pending |

---

## Daily Log

### 2026-03-09
- Assessed current project state
- Created visual roadmap
- **FIXED:** MetaHuman loading configuration
  - Set `DefaultMetaHumanMeshPath="/Game/MetaHuman1_CombinedSkelMesh"`
  - Set `DefaultMetaHumanAnimBlueprintPath="/Game/MetaHumans/Common/Body/ABP_Body_PostProcess"`
  - Set `DefaultMetaHumanSkinMaterialPath="/Game/MetaHumans/MetaHuman1/Body/Materials/MI_Body_Baked_VT"`
- Created Animation-Setup-Plan.md for Mixamo integration
- **CREATED:** `Animation-Editor-StepByStep.md` (detailed IK retargeting guide)
- **MODIFIED:** `SoccerPlayerPawn.h/.cpp` (added animation support)
- **CREATED:** Python import script `Content/Python/import_animations.py`
- **BLOCKED:** FBX import via mouse automation (unreliable)
- **LEARNING:** Mouse automation on Unreal Editor is unreliable, should use Python API or manual steps

### Current Blockers
1. FBX import dialog not responding to automated clicks
2. Need manual import OR Python API execution

### Next Steps (Prioritized)
1. Import FBX animations (manual or Python)
2. Create IK Rig and Retargeter
3. Retarget all 11 animations
4. Create Blendspace for 8-directional movement
5. Build ABP_SoccerPlayer
6. Create ball material
7. Create pitch material with field markings
8. Add goal geometry
