# AnimBP State Machine Design — ABP_SoccerPlayer

**Date**: 2026-04-05
**Status**: Draft

## Goal

Replace the single SequencePlayer(Offensive_Idle) in ABP_SoccerPlayer with a state machine + montage architecture that serves the physics-based gameplay.

## Architecture

**Principle**: Animation is visual feedback for physics events, not the driver.

| Layer | Mechanism | Purpose |
|-------|-----------|---------|
| State machine | Idle ↔ Locomotion ↔ Dribble | Continuous locomotion states |
| Montage slot | Kick, Header, Shot | One-shot physics actions overlaid on current state |

## State Machine: LocomotionSM

**3 states, driven by `bIsMoving` and `bHasBall` from SoccerAnimInstance:**

```
        bIsMoving=true
 Idle ─────────────────→ Locomotion
  ↑      bIsMoving=false     │
  │ ←────────────────────────┘
  │                           │ bHasBall=true
  │                           ↓
  │                       Dribble
  │      bIsMoving=false      │
  └←──────────────────────────┘
  ↑      bHasBall=false       │
  └←──────────────────────────┘ (→ Locomotion if still moving)
```

**Transitions:**
| From | To | Condition |
|------|----|-----------|
| Idle | Locomotion | `bIsMoving == true` |
| Idle | Dribble | `bIsMoving == true && bHasBall == true` |
| Locomotion | Idle | `bIsMoving == false` |
| Locomotion | Dribble | `bHasBall == true` |
| Dribble | Idle | `bIsMoving == false` |
| Dribble | Locomotion | `bHasBall == false` |

**Animations per state:**
- **Idle**: Offensive_Idle_Anim (loop)
- **Locomotion**: Jog_Forward_Anim (loop) — BlendSpace upgrade later
- **Dribble**: Dribble_Anim (loop)

## Montage Slot (Kicks)

- Kick_Soccerball_Anim compiled as montage on DefaultGroup.DefaultSlot
- Plays *overlaid* on whatever state machine state is active
- `AnimNotify_KickContact` fires at contact frame → `ExecuteKickFromNotify()` applies physics impulse
- Montage ends → character returns to current state machine state seamlessly

## Deferred Items

- BlendSpace for directional locomotion (Jog_Forward/Back/Strafe_Left/Strafe_Right blended by direction)
- Sprint animations (bIsSprinting currently unused in state machine — could be Locomotion sub-state)
- Header montage (Soccer_Header_Anim)
- Shot montage (Strike_Forward_Jog_Anim)

## Implementation Approach

State machines cannot be fully constructed via soft-ue-cli's graph node API (UE state machines are nested sub-graphs). Options:

1. **UE Python script** (`run-python-script`) — most programmatic control, but UE Python API for AnimBP state machines is limited
2. **Manual editor setup** — most reliable, user creates state machine in editor following this spec
3. **Hybrid** — Python sets up what it can, manual for the rest

Recommendation: Start with UE Python script attempt, fall back to manual if API gaps emerge.
