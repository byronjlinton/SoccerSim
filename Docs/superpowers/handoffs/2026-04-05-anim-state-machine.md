# Handoff: AnimBP State Machine Build

**Date:** 2026-04-05
**Status:** COMPLETE — committed as 8cb9873
**Plan:** `Docs/superpowers/plans/2026-04-05-anim-state-machine.md`
**Spec:** `Docs/superpowers/specs/2026-04-05-anim-state-machine-design.md`

---

## Summary

All animation pipeline tasks completed. Pivoted from state machine (impossible to wire transitions via API) to BlendListByBool blend tree. 22 players animated in PIE, no errors.

## Completed

### Task 1: Kick AnimMontage (DONE)
- Created `/Game/Anim/AM_Kick` as proper AnimMontage via AnimMontageFactory
- Set `KickMontagePath` on `/Game/Blueprints/BP_SoccerPlayerPawn`
- Saved BP_SoccerPlayerPawn

### Task 2: API Feasibility Probe (DONE)
- `add-graph-node` with AnimGraphNode_StateMachine creates shell only
- No Python API for creating state machine internals (states, transitions)
- Must use manual editor + Python hybrid approach

### Task 4: Build Animation Graph (DONE)

**Architecture: BlendListByBool tree (replaced state machine)**

State machine transitions proved impossible to configure via Python API (no access to transition condition graphs). Pivoted to hierarchical boolean blending:

```
Output Pose ← HasBall BlendListByBool
  ├── false (no ball) ← IsMoving BlendListByBool
  │   ├── false: Offensive_Idle_Anim (SequencePlayer)
  │   └── true:  Jog_Forward_Anim (SequencePlayer)
  └── true (has ball): Soccer_Spin_Anim (SequencePlayer)
```

**Node GUIDs in AnimGraph:**
| Node | GUID |
|---|---|
| Output Pose Root | `649B580A-4ACB-208B-B3C5-BAA3C08A0397` |
| HasBall BlendListByBool | `988A07E9-46C3-3A00-C2B0-A69DABE80EC3` |
| IsMoving BlendListByBool | `14058474-482A-CFF4-42BD-DAB8E7B97C91` |
| Idle SequencePlayer | `35699317-44FC-351B-4683-F7B5E877213B` |
| Locomotion SequencePlayer | `EDB6882B-453C-8276-3AAC-E685672ABFAE` |
| Dribble SequencePlayer | `DE70F1ED-4995-FB8D-15B6-D08F0A7ABCF6` |
| Get bHasBall | `6964B9B6-4C7F-4D6E-5215-EE95FA704EF4` |
| Get bIsMoving | `7552785D-465F-DD33-7D58-10839F9115DA` |
| StateMachine (disconnected) | `1EE1D460-4920-699F-38A6-B19D36D550B3` |

### Task 5: Verify in PIE (DONE)
- PIE started, 22 players confirmed with ABP_SoccerPlayer_C anim class
- Viewport captured: players NOT in T-pose (standing poses visible)
- No animation errors in logs
- Kill editor to clean up

### Task 6: Commit (DONE)
- Commit 8cb9873: 48 files changed, animation pipeline complete

---

## Key API Discoveries

- **State machine transitions cannot be configured via Python API** — the `can_enter_transition` pin on AnimGraphNode_TransitionResult is inside transition graphs that have no Python editing interface
- **BlendListByBool works as alternative** — hierarchical boolean blending achieves same visual result
- **K2Node_VariableGet requires full VariableReference struct**: `{"MemberName":"bHasBall","MemberGuid":"...","bSelfContext":true}` — without `bSelfContext`, pins are empty
- **connect-graph-pins works with just GUIDs** (no `--graph-name`) — GUIDs are unique within the asset
- **UE Python: SoccerAnimInstance properties** not accessible via `get_editor_property('b_is_moving')` or `.bIsMoving` at runtime — must use direct C++ access or reflection

---

## Next Steps (Future Work)

1. **Team-colored materials** — home/away team jersey colors on the Dribble mesh
2. **Expand animation set** — add jog backward, strafe left/right, header, strike animations to blend tree (already retargeted, just need wiring)
3. **Kick montage integration** — wire AM_Kick to play on input action, return to blend tree after
4. **BlendSpace for locomotion** — replace single Jog_Forward with directional blendspace (forward/back/strafe)
5. **Remove disconnected state machine node** — LocomotionSM still in AnimGraph but not connected to output

---

## Key Technical Details

- **AnimBP:** `/Game/Anim/ABP_SoccerPlayer`
- **Parent:** `SoccerAnimInstance` (C++) — exposes `bIsMoving`, `bHasBall`, `MovementSpeed`
- **Skeleton:** `Dribble_Skeleton`
- **soft-ue-cli:** `C:\Users\byron\AppData\Roaming\Python\Python314\Scripts\soft-ue-cli.exe`
- **MSYS_NO_PATHCONV=1** for all soft-ue-cli calls
- **MCP bugs:** Fall back to bash if MCP tool parameter parsing fails
- **Verification:** capture-viewport + Read PNG, never trust logs alone
- **External image analysis tools (analyze_image, mcp__4_5v_mcp) fail with 400 errors** — use Read tool on PNG files instead (multimodal)
