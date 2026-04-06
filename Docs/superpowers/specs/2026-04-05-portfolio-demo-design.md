# SoccerSim Portfolio Demo — Full Audit & Direction Spec

**Date**: 2026-04-05
**Status**: Approved for planning

## Context

SoccerSim has a structurally complete C++ codebase (12 modules, no stubs) but nothing works in PIE. Players don't move, ball doesn't react, field looks bare. The gap between "code exists" and "game runs" is the entire Phase 0. After Phase 0, we polish to photorealistic portfolio quality for a 3-minute 5v5 playable demo targeting hiring managers.

## Product Definition

- **What**: Photorealistic 3-minute 5v5 playable soccer demo
- **Who**: Hiring managers and recruiters
- **Format**: Recruiters pick up a gamepad and play
- **Visual bar**: Photorealistic — Quixel materials, Lumen, post-processing
- **Workflow**: Maximum automation via UnrealClaude MCP
- **Wow factor**: First 30 seconds must look like a broadcast TV match

## Ground Truth Audit

### Code State (C++ — structurally complete, NOT functional in PIE)

| Module | Files | Code Quality | PIE Status |
|--------|-------|-------------|------------|
| Ball | SoccerBall.h/.cpp | Excellent (Magnus, drag, CCD) | Not working |
| Field | SoccerField.h/.cpp, SoccerGoal.h/.cpp | Excellent (FIFA-spec markings) | Bare, no markings |
| Player | SoccerPlayerPawn.h/.cpp | Good (11 kicks, dribble, stamina) | Don't move |
| AI | SoccerAIController, TeamBrain, GK AI, UtilityEvaluator | Good (dual AI, formations) | Not functioning |
| Core | GameMode, GameState, PlayerController, GameInstance | Good (match flow, spawning) | Not triggering |
| Animation | SoccerAnimInstance, AnimNotify_KickContact | Good (blendspace params) | T-posing |
| Camera | SoccerBroadcastCamera | Good (tracking, 42-deg FOV) | Unknown |
| UI | SoccerHUDWidget | Clean (score, clock, phase) | Not visible |
| Utils | SoccerSimTypes.h | Complete (all types, constants) | N/A |
| Input | (in PlayerController) | Complete (9 input actions) | Not responding |
| Match | (in GameMode/GameState) | Good (state machine) | Not running |
| Data | SoccerTacticsDataAsset | Minimal | N/A |

### Asset State

| Category | Count | Notes |
|----------|-------|-------|
| Blueprints | 10 | BP_GameMode, BP_PlayerPawn, BP_Ball, BP_Field, BP_Goal, BP_Camera, BP_AIController, BP_PlayerController, BP_GameState, BP_MetaHuman1 |
| Animations | 10 retargeted + 54 Mixamo source | On Dribble_Skeleton via IK Rig pipeline |
| IK Rigs | 10 | 19 retarget chains each |
| Retargeters | 9 | Source-to-Dribble mappings |
| AnimBP | 1 | ABP_SoccerPlayer (BlendListByBool tree) |
| AnimMontage | 1 | AM_Kick (not wired to input) |
| Materials | 4 game + 13 MetaHuman | M_DynamicColor, M_Grass, M_Football, M_GoalNet |
| Input Actions | 9 | Move, Sprint, Pass, Shoot, ThroughBall, Tackle, SwitchPlayer, Lob, ViewToggle |
| Input Mapping | 1 | IMC_Soccer (WASD + gamepad) |
| Skeletal Meshes | 2 | MetaHuman1_CombinedSkelMesh, Dribble mesh |
| Maps | 0 | No .umap found in Content/Maps/ |

### Missing Systems

- Stadium geometry (stands, floodlights, sideline structures)
- Audio (crowd ambient, whistle, kick sounds)
- Team-colored jerseys
- Post-processing (LUT, DOF, motion blur, lens effects)
- Broadcast HUD overlay
- Menu/start screen
- Crowd system
- Set pieces (free kicks, penalties)
- Referee (visual + foul detection)
- Offside rule

## Phase 0 — Make It Work

**Goal**: Get PIE to the point where you can play a full match — players move, ball reacts, AI runs, goals score, timer counts.

**Approach**: System-by-system bootstrapping. Each step: build → PIE → capture viewport → diagnose → fix → verify.

### Step 0.1: Build + PIE Baseline

- Compile the project
- Launch editor, start PIE
- Capture viewport
- Read the output log for errors
- Document exactly what shows up vs what's expected

### Step 0.2: Field

- Verify SoccerField spawns and renders dynamic mesh markings
- Verify SoccerGoal spawns with collision
- Verify boundary triggers detect ball going out
- Debug: If markings missing, check DynamicMesh3 generation in SoccerField::BeginPlay
- Debug: If goals missing, check SoccerGoal spawn in SoccerGameMode

### Step 0.3: Ball

- Verify SoccerBall spawns at center
- Verify physics body falls to ground and rests
- Verify collision with field and goals
- Debug: If ball falls through floor, check collision profile and ECC settings
- Debug: If ball doesn't appear, check DefaultGame.ini DefaultFootballMaterialPath

### Step 0.4: Players

- Verify all 22 players spawn in formation positions
- Verify Dribble skeletal mesh loads on each player
- Verify ABP_SoccerPlayer_C anim instance runs
- Debug: If T-posing, check DefaultMetaHumanMeshPath and DefaultMetaHumanAnimBlueprintPath in DefaultGame.ini
- Debug: If no mesh, verify /Game/Anim/Mixamo/Dribble path exists

### Step 0.5: Input

- Verify WASD/gamepad moves the possessed player
- Verify pass (A/Xbox A) sends ball toward teammate
- Verify shoot (B/Xbox B) kicks ball toward goal
- Debug: If no input response, check IA_*/IMC_Soccer in BP_PlayerController
- Debug: Verify EnhancedInputComponent setup in PlayerController::SetupInputComponent

### Step 0.6: AI

- Verify non-possessed players move (not static)
- Verify AI players track ball position
- Verify AI players pass and make decisions
- Debug: If AI doesn't activate, check BP_GameMode → Player Controller Class → BP_SoccerAIController
- Debug: Verify SoccerAIController::OnPossess fires

### Step 0.7: Match Flow

- Verify match starts at KickOff
- Verify match clock counts
- Verify goals are detected and scored
- Verify ball resets after goal
- Verify Half Time transition
- Debug: If match doesn't start, check SoccerGameMode::BeginPlay state machine init

### Step 0.8: HUD

- Verify score display shows "0 - 0"
- Verify match clock displays and counts
- Verify phase indicator shows current phase
- Debug: If no HUD, check BP_PlayerController → HUD Widget Class property

**Phase 0 exit criteria**: Recruiters can pick up a gamepad, play a full match, see the score change, and the clock runs from kickoff to full time.

## Phase A — Make It Beautiful

**Prerequisite**: Phase 0 complete (everything works)

### Step A1: Stadium + Lighting

- Create stadium geometry (stands, floodlights, sideline)
- Configure Lumen GI with outdoor settings
- Add atmospheric fog and volumetric lighting
- Add visible floodlight beams in haze

### Step A2: Materials

- Quixel Megascans grass material on pitch
- Football texture (white + black pentagons) on ball
- Team-colored jerseys (MI_Home red, MI_Away blue from M_DynamicColor)
- Goal post metallic material, net translucent material

### Step A3: Broadcast Camera

- Multi-angle system (sideline, goal-behind, tactical overhead)
- Smooth camera transitions between angles
- Camera shake on goals and big tackles
- Dynamic zoom based on ball position (tight near goal, wide at midfield)

### Step A4: Animation Polish

- BlendSpace for 8-directional locomotion (replace single Jog_Forward)
- Kick montage triggered on shoot/pass input
- Celebration animation on goal
- Idle variation (weight shift, looking around)

### Step A5: Audio

- Crowd ambient (layered: low murmur base + reactive cheering)
- Whistle for kickoff, half time, full time
- Kick impact sounds (varied by kick type)
- Ball bounce sound on ground/post
- Goal scoring crowd roar

### Step A6: Post-Processing

- Color grading LUT for broadcast TV look
- Depth of field on close-ups
- Motion blur refinement on fast ball movement
- Lens flare from floodlights
- Subtle film grain for cinematic feel
- Chromatic aberration on slow-motion replay moments

### Step A7: Broadcast HUD

- Score overlay (team names, score, clock) — broadcast style
- Team badges/logos
- Match phase indicator
- Player name label on controlled player
- Mini radar/tactical view (optional)

### Step A8: Scale + Match Config

- Scale from 22 players to 10 (5v5)
- Set match duration to 3 minutes
- Add start screen (press Start to begin)
- Add end screen (final score, "Play Again")
- Ensure clean match restart

**Phase A exit criteria**: PIE looks like a broadcast TV soccer match. Screenshots are portfolio-ready. A 3-minute 5v5 match plays from start screen to end screen with photorealistic visuals.

## Technical Constraints

- **Engine**: UE 5.7
- **Platform**: Windows (PC)
- **Build tool**: UBT via Build.bat
- **Automation**: UnrealClaude MCP (primary), soft-ue-cli (fallback)
- **Manual work**: Minimize — only when MCP cannot achieve the result (e.g., BlendSpace axis configuration)
- **Source control**: Git on master branch, no remote

## Verification Protocol

After each step in Phase 0 and Phase A:
1. Build succeeds with zero errors
2. PIE launches without crashes
3. Viewport capture analyzed for visual correctness
4. Output log checked for LogSoccerSim errors/warnings
5. Previous steps still work (no regressions)

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Blueprint wiring issues block Phase 0 | High | Systematic property-by-property verification |
| MetaHuman assets too heavy for 10 players | Medium | Already using Dribble mesh instead |
| Lumen performance poor with stadium | Medium | Profile early, adjust Lumen quality |
| BlendSpace requires manual editor work | High | Accept manual step, automate everything else |
| Quixel assets need manual download | Medium | Use Bridge plugin or pre-download |

## Out of Scope

- Full 11v11 (demo is 5v5)
- Set pieces (free kicks, penalties, corners)
- Referee + foul detection
- Offside rule
- Save/load system
- Weather system
- Multiplayer
- Mobile/console targets
