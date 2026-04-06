# AnimBP State Machine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace ABP_SoccerPlayer's single SequencePlayer with a 3-state state machine (Idle/Locomotion/Dribble) and a kick montage overlay.

**Architecture:** State machine handles continuous locomotion (driven by `bIsMoving`, `bHasBall`). Kicks play as montage overlays via DefaultGroup.DefaultSlot. Animation serves physics, not the other way around.

**Tech Stack:** UE5.7 AnimBP, soft-ue-cli, UE Python scripting, AnimMontage, SoccerAnimInstance (C++ already done)

---

## Files

| File | Action | Purpose |
|------|--------|---------|
| `/Game/Anim/ABP_SoccerPlayer` | Modify | Add state machine, remove old SequencePlayer |
| `/Game/Anim/AM_Kick` | Create | AnimMontage from Kick_Soccerball_Anim |
| `Config/DefaultGame.ini` | Modify | Add KickMontagePath if not already present |

**No C++ changes needed** — SoccerAnimInstance already exposes all required properties.

---

## Task 1: Create Kick AnimMontage

**Why first:** The montage is independent of the state machine and can be verified standalone. The C++ code already has `PlayKickMontage()` infrastructure.

**Files:**
- Create: `/Game/Anim/AM_Kick` (AnimMontage derived from Kick_Soccerball_Anim)
- Verify: `Config/DefaultGame.ini` has KickMontagePath set

- [ ] **Step 1: Stop PIE if running**

Cannot create assets while PIE is running.

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli pie-session stop
```

- [ ] **Step 2: Create montage via Python script**

`create-asset` doesn't support AnimMontage natively. Use UE Python to duplicate the anim sequence as a montage:

```python
import unreal

# Create montage from anim sequence using asset tools
source_seq = "/Game/Anim/Retargeted/Kick_Soccerball_Anim"
montage_path = "/Game/Anim/AM_Kick"

# Method 1: Use AssetTools to create montage
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
seq = unreal.load_asset(source_seq)
if seq:
    # Duplicate the sequence as a montage
    montage = asset_tools.duplicate_asset("AM_Kick", "/Game/Anim/", seq)
    if montage:
        unreal.EditorAssetLibrary.save_asset(montage_path, only_if_is_dirty=False)
        print(f"SUCCESS: Created montage at {montage_path}")
    else:
        print("FAILED: Could not duplicate asset")
else:
    print(f"FAILED: Could not load {source_seq}")
```

Run via: `soft-ue-cli run-python-script --script "..."`

**Fallback if Python fails:** Right-click Kick_Soccerball_Anim in Content Browser → Create → Anim Montage. Save as AM_Kick in /Game/Anim/.

- [ ] **Step 3: Verify montage exists**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli query-asset --asset-path /Game/Anim/AM_Kick
```

Expected: AnimMontage class, Skeleton = Dribble_Skeleton.

- [ ] **Step 4: Set KickMontagePath in config**

Check if KickMontagePath is already configured in DefaultGame.ini under the SoccerSim section. If not, add it:

```ini
+KickMontagePath=/Game/Anim/AM_Kick.AM_Kick
```

Verify the C++ loads it in `SoccerPlayerPawn::LoadAnimationMontages()` — this already exists and reads from config.

- [ ] **Step 5: Commit montage creation**

---

## Task 2: Probe State Machine API Feasibility

**Why:** Before committing to an implementation approach, verify what's possible with the available tools. State machines are nested sub-graphs in UE — the CLI may not support them.

- [ ] **Step 1: Try adding a state machine node via CLI**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli add-graph-node "/Game/Anim/ABP_SoccerPlayer" AnimGraphNode_StateMachine --graph-name AnimGraph --position "-200,0"
```

Check the result. If it succeeds, note the node GUID and continue to Step 2. If it fails, skip to Task 3 (manual approach).

- [ ] **Step 2: If node was created, query the AnimBP graph to find it**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli query-blueprint-graph "/Game/Anim/ABP_SoccerPlayer" --graph-name AnimGraph
```

Look for the new AnimGraphNode_StateMachine node. Note its GUID and any sub-graph names.

- [ ] **Step 3: Try adding a state to the state machine**

Attempt to add an Idle state to the state machine's internal graph:

```bash
# The graph-name might be the state machine's generated name (e.g., "LocomotionSM")
MSYS_NO_PATHCONV=1 soft-ue-cli add-graph-node "/Game/Anim/ABP_SoccerPlayer" AnimStateNode --graph-name "LocomotionSM"
```

If this works, the programmatic approach is viable. If not, clean up the test node and proceed to Task 3.

- [ ] **Step 4: Clean up test node if needed**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli remove-graph-node "/Game/Anim/ABP_SoccerPlayer" "<test-node-guid>"
MSYS_NO_PATHCONV=1 soft-ue-cli save-asset "/Game/Anim/ABP_SoccerPlayer"
```

---

## Task 3: Build State Machine (Programmatic Approach)

**Condition:** Only execute if Task 2 probing succeeds.

**Files:**
- Modify: `/Game/Anim/ABP_SoccerPlayer`

**State machine name:** `LocomotionSM`

**States and their animations:**
| State | Animation | Loop |
|-------|-----------|------|
| Idle | Offensive_Idle_Anim | Yes |
| Locomotion | Jog_Forward_Anim | Yes |
| Dribble | Dribble_Anim | Yes |

**Transitions (all based on SoccerAnimInstance bools):**
| From | To | Rule |
|------|----|------|
| Idle | Locomotion | `bIsMoving == true && bHasBall == false` |
| Idle | Dribble | `bIsMoving == true && bHasBall == true` |
| Locomotion | Idle | `bIsMoving == false` |
| Locomotion | Dribble | `bHasBall == true` |
| Dribble | Idle | `bIsMoving == false` |
| Dribble | Locomotion | `bHasBall == false` |

- [ ] **Step 1: Remove existing SequencePlayer node**

The current graph has SequencePlayer GUID `A4DF28D9-4FCA-16CE-EDFF-B887972B2164`.

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli remove-graph-node "/Game/Anim/ABP_SoccerPlayer" "A4DF28D9-4FCA-16CE-EDFF-B887972B2164"
```

- [ ] **Step 2: Add state machine node to AnimGraph**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli add-graph-node "/Game/Anim/ABP_SoccerPlayer" AnimGraphNode_StateMachine --graph-name AnimGraph
```

Note the GUID of the new state machine node.

- [ ] **Step 3: Connect state machine output to Output Pose**

The Output Pose node has GUID `649B580A-4ACB-208B-B3C5-BAA3C08A0397`, pin name `Result`.

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli connect-graph-pins "/Game/Anim/ABP_SoccerPlayer" "<sm-guid>" "Pose" "649B580A-4ACB-208B-B3C5-BAA3C08A0397" "Result"
```

- [ ] **Step 4: Create Idle state**

Add Idle state to the state machine sub-graph. Set its animation to Offensive_Idle_Anim.

```bash
# Determine the sub-graph name from Task 2 probing
MSYS_NO_PATHCONV=1 soft-ue-cli add-graph-node "/Game/Anim/ABP_SoccerPlayer" AnimStateNode --graph-name "LocomotionSM" --properties '{"StateName":"Idle"}'
```

- [ ] **Step 5: Create Locomotion state**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli add-graph-node "/Game/Anim/ABP_SoccerPlayer" AnimStateNode --graph-name "LocomotionSM" --properties '{"StateName":"Locomotion"}'
```

Set its internal SequencePlayer to use Jog_Forward_Anim.

- [ ] **Step 6: Create Dribble state**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli add-graph-node "/Game/Anim/ABP_SoccerPlayer" AnimStateNode --graph-name "LocomotionSM" --properties '{"StateName":"Dribble"}'
```

Set its internal SequencePlayer to use Dribble_Anim.

- [ ] **Step 7: Add transitions**

For each transition in the table above, add a transition node connecting the two states with the boolean condition on the SoccerAnimInstance property.

- [ ] **Step 8: Save and compile**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli save-asset "/Game/Anim/ABP_SoccerPlayer"
MSYS_NO_PATHCONV=1 soft-ue-cli compile-blueprint "/Game/Anim/ABP_SoccerPlayer"
```

---

## Task 4: Build State Machine (Manual Editor Approach)

**Condition:** Execute if Task 2 probing fails (API doesn't support state machine creation).

**Files:**
- Modify: `/Game/Anim/ABP_SoccerPlayer`

- [ ] **Step 1: Open ABP_SoccerPlayer in editor**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli open-asset --asset-path /Game/Anim/ABP_SoccerPlayer
```

- [ ] **Step 2: Remove the existing SequencePlayer node**

In the AnimGraph, select the "Offensive_Idle_Anim Sequence Player" node and press Delete.

- [ ] **Step 3: Add a State Machine**

Right-click in AnimGraph → Add State Machine → name it `LocomotionSM`.

- [ ] **Step 4: Connect LocomotionSM to Output Pose**

Drag from LocomotionSM's output pin to the Output Pose node's Result pin.

- [ ] **Step 5: Open the state machine and create 3 states**

Double-click LocomotionSM to open it. You'll see an entry node.

Create 3 states by right-clicking → Add State:
- `Idle` (make this the default/entry state by dragging Entry → Idle)
- `Locomotion`
- `Dribble`

- [ ] **Step 6: Set Idle state animation**

Double-click Idle state. Right-click → add Sequence Player. Set its sequence to `Offensive_Idle_Anim`. Connect its Pose output to the state's Output Pose. Check "Loop" on the SequencePlayer.

- [ ] **Step 7: Set Locomotion state animation**

Double-click Locomotion state. Add Sequence Player with `Jog_Forward_Anim`. Connect to Output Pose. Check "Loop".

- [ ] **Step 8: Set Dribble state animation**

Double-click Dribble state. Add Sequence Player with `Dribble_Anim`. Connect to Output Pose. Check "Loop".

- [ ] **Step 9: Create transitions**

In the state machine overview, create these transitions by right-clicking the source state → Add Transition:

**Entry → Idle:** (automatic — wire Entry node to Idle state)

**Idle → Locomotion:** Double-click the transition. Add a condition: Property → `bIsMoving` == `true`. Add another condition: Property → `bHasBall` == `false`. Set logic to "All must be true" (AND).

**Idle → Dribble:** Double-click the transition. Conditions: `bIsMoving == true` AND `bHasBall == true`.

**Locomotion → Idle:** Condition: `bIsMoving == false`.

**Locomotion → Dribble:** Condition: `bHasBall == true`.

**Dribble → Idle:** Condition: `bIsMoving == false`.

**Dribble → Locomotion:** Condition: `bHasBall == false`.

- [ ] **Step 10: Save and compile**

Save the AnimBP. Compile it. Check for errors.

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli compile-blueprint "/Game/Anim/ABP_SoccerPlayer"
```

---

## Task 5: Verify in PIE

**Files:**
- Verify: `/Game/Anim/ABP_SoccerPlayer` working in-game

- [ ] **Step 1: Start PIE**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli pie-session start
```

- [ ] **Step 2: Capture viewport**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli capture-viewport --output base64
```

Read the screenshot. Check that players have animations (not frozen T-pose). Idle players should be in Offensive_Idle pose.

- [ ] **Step 3: Query animation state via Python**

```python
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerPlayerPawn)
for a in actors[:3]:
    mesh = a.get_mesh()
    if mesh:
        anim_inst = mesh.get_anim_instance()
        if anim_inst:
            print(f"Actor: {a.get_name()}")
            print(f"  bIsMoving: {anim_inst.get_editor_property('b_is_moving')}")
            print(f"  bHasBall: {anim_inst.get_editor_property('b_has_ball')}")
            print(f"  MovementSpeed: {anim_inst.get_editor_property('movement_speed')}")
```

- [ ] **Step 4: Check for errors**

```bash
MSYS_NO_PATHCONV=1 soft-ue-cli get-logs --filter error
```

- [ ] **Step 5: Commit working state**

---

## Notes

- **Transition conditions:** The SoccerAnimInstance properties (`bIsMoving`, `bHasBall`) are `BlueprintReadOnly`. In the AnimBP transition rules, reference them as `Get bIsMoving` / `Get bHasBall` from the "Variables" category in the transition graph.
- **BlendSpace deferred:** Directional blending (forward/back/strafe) requires a BlendSpace asset that can't be created via CLI. All locomotion uses Jog_Forward_Anim for now.
- **Sprint deferred:** `bIsSprinting` is available but unused in this phase. Can be added as a Locomotion sub-state later.
- **Context length:** This plan is self-contained. If the conversation gets too long, start a new session and reference this plan file.
