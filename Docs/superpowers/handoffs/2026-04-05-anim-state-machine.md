# Handoff: AnimBP State Machine Build

**Date:** 2026-04-05
**Status:** Task 4 nearly done — transitions exist but need conditions wired
**Plan:** `Docs/superpowers/plans/2026-04-05-anim-state-machine.md`
**Spec:** `Docs/superpowers/specs/2026-04-05-anim-state-machine-design.md`

---

## Completed

### Task 1: Kick AnimMontage (DONE)
- Created `/Game/Anim/AM_Kick` as proper AnimMontage via AnimMontageFactory
- Set `KickMontagePath` on `/Game/Blueprints/BP_SoccerPlayerPawn`
- Saved BP_SoccerPlayerPawn

### Task 2: API Feasibility Probe (DONE)
- `add-graph-node` with AnimGraphNode_StateMachine creates shell only
- No Python API for creating state machine internals (states, transitions)
- Must use manual editor + Python hybrid approach

### Task 4: Build State Machine (MOSTLY DONE)

**DONE this session:**
- 3 states (Idle, Locomotion, Dribble) already existed from prior session
- Animations already assigned to SequencePlayers (Offensive_Idle_Anim, Jog_Forward_Anim, Soccer_Spin_Anim)
- **Connected all 3 SequencePlayer → StateResult pins** via `connect-graph-pins` (no `--graph-name` needed — GUIDs are asset-unique)
- **Connected Entry → Idle** in state machine overview
- **Created 6 transitions** (removed 6 duplicates that `connect-graph-pins` created)
- Saved and compiled successfully (warnings only)

**Current state machine topology (confirmed via query):**
| Transition | GUID |
|---|---|
| Idle → Locomotion | `2443D587-43A0-4E23-380E-5A963D03E12E` |
| Locomotion → Idle | `5E7E4AD3-4E6B-71E3-7635-DA9310D6A3AF` |
| Locomotion → Dribble | `432CA195-4612-E89A-B99E-9996094CCEA6` |
| Dribble → Idle | `13223ED1-4594-55A1-CDAB-B5ADE9592DA0` |
| Dribble → Locomotion | `7B3C8E90-40D1-B9C9-F40D-3A8BB5FD6AD8` |
| Idle → Dribble | `17A67E44-44FE-F5B1-AE91-0CA83B7C6C0C` |

---

## Remaining: Transition Conditions (THE BLOCKER)

**Problem:** All 6 transitions have empty "Can Enter Transition" — compiler warns they will never fire. The `can_enter_transition` is a **pin** on `AnimGraphNode_TransitionResult` inside each transition graph, not a settable property. Setting it via `set_editor_property('can_enter_transition', True)` on the struct does NOT work — compiler still warns.

**What's needed:** Each transition graph needs a boolean node wired to the TransitionResult's `can_enter_transition` pin.

**Desired conditions:**
| From → To | Rule |
|---|---|
| Idle → Locomotion | `bIsMoving == true && bHasBall == false` |
| Idle → Dribble | `bIsMoving == true && bHasBall == true` |
| Locomotion → Idle | `bIsMoving == false` |
| Locomotion → Dribble | `bHasBall == true` |
| Dribble → Idle | `bIsMoving == false` |
| Dribble → Locomotion | `bHasBall == false` |

**Approaches to try (in order):**

1. **Manual editor approach (RECOMMENDED):** Open ABP in editor, double-click each transition, add boolean condition nodes. This is the most reliable path since the UE Python API for transition graphs is extremely limited.

2. **`add-graph-node` in transition graphs:** Try adding K2 nodes to transition graphs. All 12 transition graphs are named "Transition" — the active ones are `AnimStateTransitionNode_0` through `_5`. Problem: `add-graph-node --graph-name Transition` is ambiguous.

3. **`set-node-property` with pin defaults:** The `set-node-property` CLI claims to support pin defaults. Could try setting `can_enter_transition` default to `true` on the TransitionResult nodes. Need their GUIDs from the transition graphs.

**Key API discoveries this session:**
- `connect-graph-pins` works with just GUIDs (no `--graph-name`) — GUIDs are unique within the asset
- `connect-graph-pins` creates NEW transition nodes when connecting state Out→In pins (one per call — don't call twice for same pair)
- `remove-graph-node` successfully removes transition nodes from state machine graph
- UE Python: `get_animation_graphs()` returns state graphs, transition graphs, but NOT the state machine overview graph
- UE Python: AnimGraphNode_SequencePlayer has NO `find_pin` method
- UE Python: AnimationStateGraph has NO `get_graph_nodes` — use `get_graph_nodes_of_class` instead
- 12 transition graphs persist in `get_animation_graphs()` even after removing 6 transition nodes (stale graphs not GC'd)

---

## Not Yet Started

### Task 5: Verify in PIE
Start PIE → capture viewport → check not T-pose → query anim state → check errors

### Task 6: Commit

---

## Key Technical Details

- **AnimBP:** `/Game/Anim/ABP_SoccerPlayer`
- **Parent:** `SoccerAnimInstance` (C++) — exposes `bIsMoving`, `bHasBall`, `MovementSpeed`
- **Skeleton:** `Dribble_Skeleton`
- **soft-ue-cli:** `C:\Users\byron\AppData\Roaming\Python\Python314\Scripts\soft-ue-cli.exe`
- **MSYS_NO_PATHCONV=1** for all soft-ue-cli calls
- **MCP bugs:** Fall back to bash if MCP tool parameter parsing fails
- **Verification:** capture-viewport + Read PNG, never trust logs alone
- **Editor likely running** — check `soft-ue-cli status` first on resume

## Autonomous Chain System
- **Script:** `Scripts/chain.sh` — self-chaining headless sessions via `claude -p`
- **Bypass permissions:** `defaultMode: bypassPermissions` in `~/.claude/settings.json`
- **Monitor:** `tail -f .claude/chain/chain.log`
- **Stop:** `bash Scripts/chain.sh --stop`
