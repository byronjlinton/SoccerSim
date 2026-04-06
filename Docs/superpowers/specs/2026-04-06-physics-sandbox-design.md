# Physics Sandbox Design — Backbreaker-Style Ragdoll Prototype

**Date**: 2026-04-06
**Status**: Approved
**Supersedes**: 2026-04-06-active-ragdoll-physics-design.md

---

## Goal

Build a physics sandbox where the player directly controls a single active ragdoll via WASD/shift. No ball, no AI, no soccer rules. The entire experience is the puppet — how it runs, leans, stumbles, recovers, and feels.

Inspired by NaturalMotion's Euphoria (Backbreaker, GTA IV/V, RDR2). Euphoria is no longer licensable; UE5 has all the building blocks. This design assembles them with custom C++ behavioral logic.

---

## Architecture

```
PhysicsSandboxGameMode (AGameModeBase)
├── Spawns one PhysicsPawn
├── PlayerController possesses it
└── Nothing else

PhysicsPawn : APawn
├── USkeletalMeshComponent (18-body physics asset)
├── UPhysicalAnimationComponent (motor drives toward poses)
├── UPhysicsLocomotionComponent (root drive, ground spring, rotation, balance)
├── UPhysicsMuscleComponent (joint motor strength per state)
├── UCapsuleComponent (game collision, kinematic, follows pelvis)
└── No CharacterMovementComponent, no AnimBP, no animation clips

SandboxPlayerController : APlayerController
├── WASD → SetMovementInput on PhysicsPawn
├── Shift → SetSprinting on PhysicsPawn
└── Nothing else

Map: Empty ground plane, one light, one PhysicsPawn spawn point
```

Physics bodies ARE the animation. No canned clips.

---

## Nuclear Cleanup

### Delete entirely

| Directory/Files | Contents | Reason |
|----------------|----------|--------|
| `AI/` (7 files) | Controllers, team brain, GK AI, perception, utility evaluator | No AI in sandbox |
| `Animation/` (2 files) | SoccerAnimInstance, AnimNotify_KickContact | Physics IS the animation |
| `Ball/` | SoccerBall + FIFA-spec physics | No ball |
| `Camera/` | SoccerBroadcastCamera | No broadcast camera |
| `Data/` | SoccerTacticsDataAsset | No formations |
| `Field/` | SoccerField, SoccerGoal | No field |
| `Match/` | Match state machine | No match flow |
| `UI/` | SoccerHUDWidget | No HUD |
| `Player/SoccerPlayerPawn.h/.cpp` | Old CMC-driven pawn | Replaced by PhysicsPawn |
| `Utils/SoccerSimTypes.h` | Soccer enums/structs | Soccer-specific |

### Keep and modify

| File | Action |
|------|--------|
| `Player/SoccerPhysicsPawn.h/.cpp` | Keep as-is (main pawn) |
| `Player/PhysicsLocomotionComponent.h/.cpp` | Keep as-is |
| `Player/PhysicsMuscleComponent.h/.cpp` | Keep as-is |
| `Core/SoccerPlayerController.h/.cpp` | Strip to move + sprint only |
| `Core/SoccerGameMode.h/.cpp` | Strip to spawn one PhysicsPawn |
| `Core/SoccerGameState.h/.cpp` | Strip to empty or remove |
| `Core/SoccerGameInstance.h/.cpp` | Keep as-is |
| `SoccerSim.Build.cs` | Remove AI, Navigation, Niagara, UMG, GameplayTasks dependencies |
| `SoccerSim.h/.cpp` | Keep (log category) |

### Input

Keep: `IA_Move` (WASD), `IA_Sprint` (Shift)
Remove: IA_Pass, IA_Shoot, IA_ThroughBall, IA_Tackle, IA_SwitchPlayer, IA_Lob, IA_ViewToggle

### Content

Keep: Dribble skeletal mesh, Dribble_PhysicsAsset, BS_Locomotion blendspace (may be useful for reference)
Remove: Field, goals, ball blueprints, team materials, stadium geometry, BP_SoccerPhysicsPawn
New: Sandbox map (empty ground plane, light, PhysicsPawn spawn point)

---

## Physics System Detail

The physics system is inherited from the existing spec and code. This section summarizes what's already built and what's added per phase.

### Physics Skeleton (18 bodies)

```
Pelvis (root body, ~15 kg)
├── Spine_01, Spine_02, Spine_03 (~3 kg each)
│   ├── Neck_01, Head
│   ├── Clavicle_L → UpperArm_L → LowerArm_L → Hand_L
│   └── Clavicle_R → UpperArm_R → LowerArm_R → Hand_R
├── Thigh_L → Calf_L → Foot_L
└── Thigh_R → Calf_R → Foot_R
Total: ~75 kg
```

Constraint joints with anatomical angular limits. Capsule collision per body. Foot bodies slightly wider for ground contact stability.

### Root Locomotion Drive

Spring-damper force on pelvis driving toward desired velocity. Ground spring holds pelvis at standing height. Rotation spring turns pelvis toward movement direction.

Parameters (all UPROPERTY EditDefaultsOnly):
| Parameter | Default | Purpose |
|-----------|---------|---------|
| RootDriveSpring | 4000 | Acceleration aggression |
| RootDriveDamping | 300 | Prevents oscillation |
| GroundSpring | 8000 | Holds pelvis up |
| GroundDamping | 500 | Prevents bounce |
| TurnSpring | 1500 | Rotation speed |
| TurnDamping | 200 | Prevents rotational oscillation |
| JogSpeed | 400 cm/s | Jog target velocity |
| SprintSpeed | 750 cm/s | Sprint target velocity |
| StandingHeight | 95 cm | Pelvis Z when standing |

Emergent behaviors (free from physics): acceleration lean, deceleration lean, turning lean, collision deflection.

### Muscle Layer

Joint motor strengths vary by state (Standing/Running/Stumbling). Force clamping per joint group prevents instability while allowing natural "give" on impact.

| Group | Standing | Running | Stumbling | Max Force (Nm) |
|-------|----------|---------|-----------|----------------|
| Spine | 0.8 | 0.6 | 0.3 | 5000 |
| Arms | 0.4 | 0.3 | 0.15 | 1000 |
| Legs | 0.9 | 0.7 | 0.4 | 4000 |
| Head | 0.5 | 0.4 | 0.2 | 500 |

Strength blending via FMath::VInterpTo with configurable rates.

### Balance System (Phase 4)

- COM calculation from all physics body masses
- Support polygon from foot contact points
- Stability score (0-1) based on COM projection distance
- Corrections when stability < 0.7: hip shift, arm extension, stumble step

### Stumble & Recovery (Phase 5)

Detection: impulse > 500 Ns, stability < 0.3, velocity perturbation > 100 cm/s/frame.

Recovery behaviors scale with severity:
1. Mild (stability 0.3-0.5): widen stance, rigid arms, corrective step
2. Moderate (stability 0.1-0.3): windmill arms, stumble steps, lower COM
3. Severe (stability < 0.1): max arm flailing, transition to Falling if unrecoverable

State machine: Standing <-> Running <-> Stumbling -> Falling -> GettingUp -> Standing

---

## Development Phases

### Phase 1: Standing Ragdoll (Foundation)
**Milestone**: Pawn stands on ground plane, doesn't fall over or jitter.

1. Nuclear cleanup (delete all soccer modules)
2. Strip GameMode to spawn one PhysicsPawn
3. Strip PlayerController to move + sprint only
4. Update Build.cs dependencies
5. Create Sandbox map (ground plane + light + spawn point)
6. Verify: pawn spawns, stands stable

### Phase 2: Root Locomotion
**Milestone**: WASD moves the ragdoll with physical momentum. Lean on accel/decel.

1. Wire IA_Move to PhysicsPawn SetMovementInput
2. Wire IA_Sprint to SetSprinting
3. Camera-relative input direction
4. Test: observe forward lean on acceleration, momentum on release

### Phase 3: Procedural Running
**Milestone**: Legs cycle, arms swing, body bobs naturally.

1. Sinusoidal hip/knee/foot angle generator in Pose Generator
2. Phase-sync with root velocity (stride x frequency = speed)
3. Arm counter-swing
4. Speed-dependent amplitude
5. Spine twist and forward lean
6. Test: running looks natural at jog and sprint speeds

### Phase 4: Balance System
**Milestone**: COM tracking visible, auto-correction works.

1. COM calculation from body masses
2. Foot-ground contact detection
3. Support polygon calculation
4. Stability score
5. Hip sway correction when stability < 0.7
6. Debug visualization (COM marker, support polygon, stability bar)
7. Test: push pawn gently, observe balance correction

### Phase 5: Stumble & Recovery
**Milestone**: Ragdoll reacts to impacts, stumbles, recovers.

1. Perturbation detection
2. Arm windmill counterbalance
3. Procedural stumble stepping
4. State machine transitions
5. Falling state (full ragdoll, reduced motors)
6. Getting-up state (gradual motor increase, pelvis rises)
7. Test: apply impulse, observe stumble -> recover or fall -> get up

### Phase 6: Tuning
**Milestone**: Feels incredible.

1. Tune all spring constants, motor strengths, timing curves
2. Sprint -> jog transition quality
3. Direction change quality (cutting, pivoting)
4. Head tracking
5. Camera follows pawn (simple chase cam)

### Phase 7: Expansion
**Milestone**: More to interact with.

1. Add obstacles (walls, ramps, barriers)
2. Add impulse-apply mode (click to push ragdoll)
3. Add second ragdoll for collision testing
4. Debug overlay (state, stability, forces)

---

## Performance

One physics pawn = ~1.0-2.0ms per frame (18 bodies + balance solve + pose generation). Well within budget for 60 FPS.

---

## Risks

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Physics skeleton unstable/jittery | High | Tune solver iterations, force clamping, substep control |
| Balance feels robotic | Medium | Add noise, smooth transitions |
| Getting-up looks bad | High | Start with teleport recovery, iterate |
| Dribble skeleton incompatible with physics asset | Medium | May need to retarget physics asset specifically |
