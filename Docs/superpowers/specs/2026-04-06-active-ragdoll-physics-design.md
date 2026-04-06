# Active Ragdoll Physics Design — Single Player Prototype

**Date**: 2026-04-06
**Status**: Approved
**Scope**: ONE player. All other 21 players remain existing SoccerPlayerPawn (CMC-driven, unchanged).

---

## Goal

Build a single physics-driven player pawn that moves with emergent momentum, physically plausible balance, and stumble/recovery reactions — inspired by NaturalMotion's Euphoria (Backbreaker, GTA IV/V, RDR2).

Euphoria is no longer licensable (NaturalMotion delicensed it in 2017). UE5 has all the building blocks but no pre-built "active ragdoll" system. This design assembles those blocks with custom C++ behavioral logic.

---

## Architecture Overview

```
ASoccerPhysicsPawn (APawn, NOT ACharacter)
├── USkeletalMeshComponent (with Physics Asset — 18 rigid bodies)
├── UPhysicalAnimationComponent (motor drives toward target poses)
├── UPhysicsLocomotionComponent (custom — the nervous system)
│   ├── Root Drive (semi-kinematic pelvis velocity controller)
│   ├── Balance Controller (COM tracking, support polygon)
│   ├── Pose Generator (procedural joint angle targets)
│   ├── Stumble Controller (perturbation detection, recovery)
│   └── State Machine (Standing/Running/Stumbling/Falling/GettingUp)
├── UPhysicsMuscleComponent (joint motor strength management)
├── UCapsuleComponent (game-level collision, follows pelvis)
└── ISoccerPlayerInterface (so AI/HUD/GameMode work unchanged)
```

No UCharacterMovementComponent. No animation clips. The physics bodies ARE the animation.

---

## Section 1: Physics Skeleton

### Pawn Class
`ASoccerPhysicsPawn : public APawn` in `Source/SoccerSim/Player/`

### Physics Asset Bodies (~18 rigid bodies)

```
Pelvis (root body, ~15 kg)
├── Spine_01 (~3 kg), Spine_02 (~3 kg), Spine_03 (~3 kg)
│   ├── Neck_01 (~1 kg), Head (~4 kg)
│   ├── Clavicle_L → UpperArm_L (~2 kg) → LowerArm_L (~1.5 kg) → Hand_L (~0.4 kg)
│   └── Clavicle_R → UpperArm_R (~2 kg) → LowerArm_R (~1.5 kg) → Hand_R (~0.4 kg)
├── Thigh_L (~7 kg) → Calf_L (~4 kg) → Foot_L (~1 kg)
└── Thigh_R (~7 kg) → Calf_R (~4 kg) → Foot_R (~1 kg)
Total: ~75 kg (adult male)
```

### Collision Shapes
- Bodies: capsule primitives (radius proportional to body part width)
- Ground: static plane at Z=0
- Foot bodies: slightly wider capsule for stable ground contact

### Constraint Joints
- Angular limits matching human anatomy (e.g., knee: 0-140 degrees flexion, limited lateral rotation)
- No linear freedom (joints don't dislocate)
- Configurable drive motors (spring + damping + max force)

### Dual Collision Systems
1. **Physics bodies**: simulated by Chaos, produce visual motion, respond to impulses
2. **Capsule component**: kinematic, follows pelvis physics body, handles game collision (walls, ball proximity detection)

---

## Section 2: Root Locomotion Drive

The pelvis is the root body. It's physically simulated but receives a virtual spring-damper force driving it toward the desired velocity. This is NOT kinematic movement — it's a force applied to a physics body.

### Root Drive Controller (runs each physics tick)

```
Input: DesiredVelocity (direction + magnitude from input/AI)

1. CurrentVelocity = PelvisBody.GetLinearVelocity()
2. VelocityError = DesiredVelocity - CurrentVelocity
3. DriveForce = RootDriveSpring * VelocityError - RootDriveDamping * CurrentVelocity
4. PelvisBody.AddForce(DriveForce)

Ground constraint:
5. PelvisHeight = PelvisBody.GetPosition().Z
6. StandingHeight = 95.0 cm (pelvis height when standing)
7. If PelvisHeight < StandingHeight:
     UpForce = (StandingHeight - PelvisHeight) * GroundSpring - PelvisZVelocity * GroundDamping
     PelvisBody.AddForce(FVector(0, 0, UpForce))

Rotation:
8. CurrentFacing = PelvisBody.GetRotation()
9. TargetFacing = DesiredVelocity.Rotation()
10. Torque = TurnSpring * AngleDiff - TurnDamping * AngularVelocity
    PelvisBody.AddTorque(Torque)
```

### Parameters (all UPROPERTY EditDefaultsOnly)

| Parameter | Default | Purpose |
|-----------|---------|---------|
| RootDriveSpring | 4000 | Acceleration aggression |
| RootDriveDamping | 300 | Prevents oscillation |
| GroundSpring | 8000 | Prevents sinking through floor |
| GroundDamping | 500 | Prevents bounce |
| TurnSpring | 1500 | Rotation speed toward movement direction |
| TurnDamping | 200 | Prevents rotational oscillation |
| JogSpeed | 400 cm/s | Target jog velocity |
| SprintSpeed | 750 cm/s | Target sprint velocity |

### Emergent Behaviors (free from physics)

- **Acceleration lean**: pelvis accelerates, feet are planted → body naturally pitches forward
- **Deceleration lean**: pelvis decelerates, upper body momentum carries forward → forward lean on stopping
- **Turning lean**: centripetal force tilts body into turns
- **Collision response**: external impulse fights root drive → partial deflection, not rigid bounce

---

## Section 3: Muscle Layer (Joint Motor Controllers)

Each joint has a spring-dotor drive that pulls it toward a target orientation. The muscle layer manages target angles and strengths per-frame.

### Joint Groups

| Group | Bodies | Motor Strength (Standing) | Motor Strength (Running) | Motor Strength (Stumbling) |
|-------|--------|--------------------------|-------------------------|---------------------------|
| Spine | Spine_01-03, Neck | 0.8 | 0.6 | 0.3 |
| Left Arm | Clavicle_L through Hand_L | 0.4 | 0.3 | 0.15 |
| Right Arm | Clavicle_R through Hand_R | 0.4 | 0.3 | 0.15 |
| Left Leg | Thigh_L, Calf_L, Foot_L | 0.9 | 0.7 | 0.4 |
| Right Leg | Thigh_R, Calf_R, Foot_R | 0.9 | 0.7 | 0.4 |
| Head | Neck_01, Head | 0.5 | 0.4 | 0.2 |

### Strength Blending

Motor strengths interpolate smoothly between states using `FMath::VInterpTo()` with configurable rates:
- Transition to running: 5.0 blend speed (responsive)
- Transition to stumbling: 12.0 blend speed (immediate)
- Recovery from stumble: 2.0 blend speed (gradual)

### Force Clamping

Each joint has `MaxAngularForce` to prevent instability:
- Spine: 5000 Nm
- Arms: 1000 Nm
- Legs: 4000 Nm
- Head: 500 Nm

Lower max force = more "give" on impact, more natural-looking reactions.

---

## Section 4: Nervous System / Behavior Layer

Custom `UPhysicsLocomotionComponent` that orchestrates the whole body.

### 4a. Balance Controller

**COM Calculation**:
```cpp
FVector COM = FVector::ZeroVector;
float TotalMass = 0.0f;
for (auto& [BodyName, BodyInstance] : PhysicsBodies)
{
    float Mass = BodyInstance.GetBodyMass();
    COM += BodyInstance.GetCOMPosition() * Mass;
    TotalMass += Mass;
}
COM /= TotalMass;
```

**Support Polygon**: Convex hull of foot contact points projected onto ground plane.

**Stability Score**: 0-1 based on COM projection distance from support polygon center, normalized by polygon radius.

**Correction Behaviors** (when stability < 0.7):
1. Hip shift: offset pelvis target toward support polygon center
2. Arm extension: extend arm on side opposite to COM drift
3. Stumble step: trigger procedural step in drift direction

### 4b. Pose Generator

Produces target joint angles for each state. Pure procedural math, no animation clips.

**Idle Pose**: Upright stance with slight sinusoidal sway.
```
SpinePitch = 0 + sin(Time * 0.5) * 2.0  // slight sway
HipShift = sin(Time * 0.3) * 1.0         // weight shifting
```

**Running Cycle**:
```
Phase = fmod(Time * StepFrequency, 2*PI)
StepFrequency = Speed / StrideLength  // faster at higher speed

// Legs cycle in opposition
LeftHipPitch = -HipAmplitude * sin(Phase)
RightHipPitch = -HipAmplitude * sin(Phase + PI)
LeftKneePitch = KneeProfile(Phase)       // sharper bend during swing
RightKneePitch = KneeProfile(Phase + PI)
LeftFootPitch = FootProfile(Phase)       // dorsiflexion during swing
RightFootPitch = FootProfile(Phase + PI)

// Arms counter-swing
LeftShoulderPitch = ArmAmplitude * sin(Phase)
RightShoulderPitch = ArmAmplitude * sin(Phase + PI)
LeftElbowPitch = ElbowProfile(Phase)     // bent during backswing
RightElbowPitch = ElbowProfile(Phase + PI)

// Spine rotation (slight twist opposing shoulder)
SpineYaw = SpineTwistAmplitude * sin(Phase)
SpinePitch = SpeedRatio * 8.0           // forward lean proportional to speed
```

**Speed-Dependent Parameters**:

| Parameter | Jog (400 cm/s) | Sprint (750 cm/s) |
|-----------|----------------|-------------------|
| HipAmplitude | 35 deg | 50 deg |
| ArmAmplitude | 25 deg | 45 deg |
| StrideLength | 120 cm | 160 cm |
| SpineTwistAmplitude | 5 deg | 10 deg |

**Knee/Foot Profiles**: Precomputed lookup tables that produce realistic knee bend patterns (sharp flexion during swing phase, extended during stance).

### 4c. Stumble Controller

**Detection**:
- Impulse threshold: any body receives > 500 Ns impulse
- Balance threshold: stability score drops below 0.3
- Velocity perturbation: horizontal velocity changes by > 100 cm/s in one frame

**Recovery Behaviors** (ordered by severity):

1. **Mild perturbation** (stability 0.3-0.5):
   - Widen stance (increase leg spread target)
   - Increase arm motor strength to 0.6 (rigid counterbalance)
   - Quick corrective step in drift direction

2. **Moderate stumble** (stability 0.1-0.3):
   - Windmill arms in direction opposing fall (angular momentum)
   - Take 1-2 rapid stumbling steps
   - Lower COM (bend knees) to widen support base
   - Increase torso rigidity to prevent upper body collapse

3. **Severe stumble** (stability < 0.1):
   - Maximum arm flailing
   - Multiple rapid steps
   - If COM velocity is too high to recover → transition to Falling state

**Recovery Detection**:
- COM projection returns inside support polygon AND
- COM velocity is decreasing AND
- No new perturbation for 0.3s

### 4d. State Machine

```
States: Standing ↔ Running ↔ Stumbling → Falling → GettingUp → Standing
                  ↕
              Dribbling
```

**Transitions**:

| From | To | Condition |
|------|----|-----------|
| Standing | Running | Input magnitude > 0.1 |
| Running | Standing | Input magnitude < 0.05 for 0.3s |
| Running | Dribbling | Has ball flag set |
| Running | Stumbling | Impulse > 500 Ns or stability < 0.3 |
| Standing | Stumbling | Impulse > 300 Ns |
| Stumbling | Running | Recovery detected (COM stable, no perturbation) |
| Stumbling | Falling | Stability < 0.05 for 0.5s |
| Falling | GettingUp | All bodies within 50cm of ground for 1.0s |
| GettingUp | Standing | Pelvis reaches standing height, stability > 0.7 |

Each state sets:
- Pose targets (from Pose Generator)
- Motor strengths (from muscle layer table)
- Root drive parameters (spring/damping adjustments)
- Transition monitoring logic

---

## Section 5: SoccerSim Integration

### Coexistence
- `ASoccerPhysicsPawn` coexists with existing `ASoccerPlayerPawn`
- Config flag in DefaultGame.ini: `bUsePhysicsPawn=true/false`
- Only ONE physics pawn on the pitch (the prototype). Other 21 players unchanged.

### Interface Extraction
```cpp
class ISoccerPlayerInterface {
    virtual void SetMovementInput(FVector2D Input) = 0;
    virtual void SetSprinting(bool bSprint) = 0;
    virtual void AttemptKick(EKickType KickType) = 0;
    virtual ETeamId GetTeamId() const = 0;
    virtual bool HasBall() const = 0;
    // ... etc
};
```
Both `ASoccerPlayerPawn` and `ASoccerPhysicsPawn` implement this interface.

### AI Controller
Same `ASoccerAIController` drives both pawn types via the interface. No AI changes needed.

### Ball Interaction
- Physics pawn's foot bodies have Chaos collision enabled with the ball
- Kick impulse = foot body velocity * ball mass at contact frame
- No code-driven kick needed — physics produces the kick naturally
- Dribble: foot contact with ball while running pushes ball forward naturally
- Can add a slight bias force to keep ball ahead of player during dribble

### What Stays Unchanged
- Match state machine (kickoff, halves, goals)
- HUD (score, clock, phase)
- Formation system
- Camera
- Ball physics (Magnus, drag, CCD)
- AI decision-making

### What Gets Replaced (for the physics pawn only)
- `UpdateMovement()` → root drive controller
- `CharacterMovementComponent` → removed
- Animation montages → procedural pose targets
- BlendListByBool anim tree → physics bodies are the animation

### What's Adapted
- Stamina system → replicated on physics pawn, affects root drive spring
- Kick direction/power → still calculated, applied as target velocity to the foot body before contact
- Player data / stats → same struct, muscle strength and balance recovery scaled by stats

---

## Section 6: Development Phases

### Phase 1: Standing Ragdoll (Foundation)
**Milestone**: Pawn stands on pitch, doesn't fall over or jitter.

Tasks:
1. Create `ASoccerPhysicsPawn` class (APawn subclass)
2. Create Physics Asset with 18 bodies on the Dribble skeleton
3. Set up joint constraints with human-anatomical limits
4. Implement root drive ground spring (holds pelvis at standing height)
5. Set all joint motors to standing-pose targets
6. Test: spawn pawn, verify stability

### Phase 2: Root Locomotion
**Milestone**: Pawn moves with input, has physical momentum.

Tasks:
1. Implement root velocity drive (spring-damper toward desired velocity)
2. Camera-relative input direction
3. Acceleration/deceleration curves (stamina-aware)
4. Rotation drive (pelvis turns toward movement direction)
5. Sprint/jog speed toggle
6. Test: WASD moves pawn, observe lean on accel/decel

### Phase 3: Procedural Running
**Milestone**: Legs cycle, arms swing, body bobs naturally.

Tasks:
1. Implement sinusoidal hip/knee/foot angle generator
2. Phase-sync cycling with root velocity (stride length × frequency = speed)
3. Arm counter-swing (opposite to legs)
4. Speed-dependent amplitude parameters
5. Spine twist and forward lean proportional to speed
6. Test: running looks natural at all speeds

### Phase 4: Balance System
**Milestone**: COM tracking, support polygon, stability score visible.

Tasks:
1. Calculate COM from all physics body masses
2. Detect foot-ground contacts (collision events)
3. Compute support polygon from contact points
4. Calculate stability score (0-1)
5. Implement hip sway correction when stability < 0.7
6. Add debug visualization (COM marker, support polygon, stability bar)
7. Test: standing sway, push pawn gently → corrects balance

### Phase 5: Stumble & Recovery
**Milestone**: Pawn reacts to impacts, stumbles, recovers balance.

Tasks:
1. Perturbation detection (impulse threshold)
2. Arm windmill counterbalance behavior
3. Procedural stumble stepping
4. State machine transitions (Running → Stumbling → Running/Falling)
5. Recovery detection logic
6. Falling state (full ragdoll, reduced motor strength)
7. Getting-up state (gradual motor strength increase, pelvis rises)
8. Test: apply impulse → stumble → recover OR fall → get up

### Phase 6: Soccer Integration
**Milestone**: Physics pawn plays soccer with the ball.

Tasks:
1. Extract `ISoccerPlayerInterface` from existing pawn
2. Adapt `ASoccerPhysicsPawn` to implement interface
3. Config-driven pawn class selection in GameMode
4. Ball collision with physics foot bodies
5. Kick from physics momentum (foot velocity at contact)
6. Dribble via physics (foot pushes ball on contact)
7. AI controller drives physics pawn via interface
8. Test: AI controls physics pawn, kicks ball, dribbles down field

### Phase 7: Tuning & Polish
**Milestone**: Feels amazing, looks human.

Tasks:
1. Tune all spring constants, motor strengths, timing curves
2. Stamina effects on physics (lower drive force, slower recovery)
3. Per-player stat variation (mass, muscle strength, balance)
4. Sprint → jog transition quality
5. Direction change quality (cutting, pivoting)
6. Visual polish (mesh orientation, head tracking)

---

## UE5 API Reference

### Key Classes
- `UPhysicalAnimationComponent` — motor drives toward animation targets
- `FPhysicalAnimationData` — per-body drive settings (orientation/position/velocity strength)
- `FAnimNode_RigidBody` — ragdoll in anim graph (not used — we drive physics directly)
- `USkeletalMeshComponent::SetAllBodiesSimulatePhysics()` — enable physics on mesh
- `USkeletalMeshComponent::SetConstraintProfile()` — switch constraint profiles
- `Chaos::FRigidBody` — low-level Chaos body access

### Performance Considerations
- One physics pawn = ~1.0-2.0ms per frame (18 bodies + balance solve + pose generation)
- Only enable full physics on the prototype pawn
- Other 21 players remain CMC-driven (no physics cost)
- Future scaling: enable physics only on the 2-4 players near ball or in collision

### Known UE5.7 Considerations
- UPhysicalAnimationComponent is tagged Experimental — API works, no stability guarantee
- Chaos ragdoll solver runs on game thread for anim-graph-integrated ragdolls — our custom approach (direct physics control) avoids this bottleneck
- Physics Asset must be created in editor on the Dribble skeleton — cannot be created purely in C++

---

## Risks and Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Physics skeleton is unstable/jittery | High | Tune solver iterations, force clamping, substep control |
| Balance system feels robotic | Medium | Add noise to corrections, smooth transitions |
| Performance too heavy | Low | One pawn is fine; profile and optimize later |
| Getting-up looks bad | High | Start with teleport recovery, improve iteratively |
| Ball interaction inconsistent | Medium | Fallback to code-driven kicks with physics-driven follow-through |
| Skeleton not compatible with Dribble mesh | Medium | May need to retarget Physics Asset to Dribble skeleton specifically |
