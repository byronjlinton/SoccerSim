#!/bin/bash
# SoccerSim Autonomous Session Chain
# Spawns headless claude -p sessions that self-chain via handoff docs.
# Uses GLM-5.1 via Z.AI routing (inherited from ~/.claude/settings.json).
#
# Usage:
#   bash Scripts/chain.sh                 # start the chain
#   bash Scripts/chain.sh --status        # check chain status
#   bash Scripts/chain.sh --stop          # stop the chain
#   bash Scripts/chain.sh --test          # single short session test

set -euo pipefail
cd "$(git rev-parse --show-toplevel 2>/dev/null || echo '.')"

CHAIN_DIR=".claude/chain"
mkdir -p "$CHAIN_DIR"

STATUS_FILE="$CHAIN_DIR/status"
COUNTER_FILE="$CHAIN_DIR/counter"
LOG_FILE="$CHAIN_DIR/chain.log"
STOP_FILE="$CHAIN_DIR/stop"

# --- Commands ---
if [[ "${1:-}" == "--stop" ]]; then
    touch "$STOP_FILE"
    echo "Stop signal sent. Chain will halt after current session."
    exit 0
fi

if [[ "${1:-}" == "--status" ]]; then
    if [[ -f "$STATUS_FILE" ]]; then
        cat "$STATUS_FILE"
        echo ""
        echo "Sessions completed: $(cat "$COUNTER_FILE" 2>/dev/null || echo 0)"
        echo "Log: $LOG_FILE"
    else
        echo "Chain not running."
    fi
    exit 0
fi

# --- Resume prompt template ---
RESUME_PROMPT='You are an autonomous agent continuing the SoccerSim project. Bypass mode: terse, no filler, execute the plan.

STEP 1 — ORIENTATION:
- Read C:\Users\byron\.claude\projects\c--Users-byron-Cursor-SoccerSim\memory\MEMORY.md
- Find the "Active:" entry and read that handoff doc
- Read CLAUDE.md in the project root for build/verification commands

STEP 2 — EXECUTE:
- Continue work from the handoff doc'\''s "Remaining steps" or "In Progress" section
- DO NOT re-probe or re-verify facts already documented in the handoff — trust it
- After each meaningful step, update the handoff doc with what you did and evidence

STEP 3 — CHAIN OR STOP:
When all tasks are done OR context is getting heavy:
  a. Write/update the handoff doc with exact current state (GUIDs, errors, last step)
  b. Update MEMORY.md if the Active pointer needs changing
  c. Run exactly: bash Scripts/chain.sh
  d. Print: CHAIN_SPAWN_NEXT
If the ENTIRE task is complete, write "Status: COMPLETE" in the handoff and do NOT chain.

KEY FACTS:
- Project root: c:\Users\byron\Cursor\SoccerSim
- Memory: C:\Users\byron\.claude\projects\c--Users-byron-Cursor-SoccerSim\memory\
- Handoff dir: Docs/superpowers/handoffs/
- CRITICAL: export MSYS_NO_PATHCONV=1 before any soft-ue-cli calls
- soft-ue-cli MCP tools may have parameter bugs — fall back to bash if MCP fails
- Use capture-viewport + Read tool on PNG to verify visuals (never trust logs alone)

BEGIN WORK NOW.'

# --- Test mode: single short session ---
MAX_TURNS=100
if [[ "${1:-}" == "--test" ]]; then
    MAX_TURNS=5
    RESUME_PROMPT='This is a chain test. Read MEMORY.md at C:\Users\byron\.claude\projects\c--Users-byron-Cursor-SoccerSim\memory\MEMORY.md, then read the Active handoff doc. Report: 1) what task is active, 2) what step is next, 3) are MCP tools working (run soft-ue-cli status). Be terse. Do NOT chain after this. Print CHAIN_TEST_DONE when finished.'
fi

# --- Session counter ---
SESSION_NUM=$(($(cat "$COUNTER_FILE" 2>/dev/null || echo 0) + 1))
echo "$SESSION_NUM" > "$COUNTER_FILE"

# --- Clean stop file on fresh start ---
if [[ "$SESSION_NUM" -eq 1 ]]; then
    rm -f "$STOP_FILE"
fi

# --- Check stop signal ---
if [[ -f "$STOP_FILE" ]]; then
    echo "[$(date)] Chain stopped by user after $((SESSION_NUM - 1)) sessions." | tee -a "$LOG_FILE"
    rm -f "$STOP_FILE"
    echo "STOPPED" > "$STATUS_FILE"
    exit 0
fi

# --- Run session ---
echo "RUNNING session #$SESSION_NUM — $(date)" > "$STATUS_FILE"
echo "[$(date)] === Session #$SESSION_NUM started ===" >> "$LOG_FILE"

echo "Chain session #$SESSION_NUM starting..."
echo "  Monitor: tail -f $LOG_FILE"
echo "  Stop:    bash Scripts/chain.sh --stop"
echo ""

SESSION_OUT="$CHAIN_DIR/session_${SESSION_NUM}.txt"
MSYS_NO_PATHCONV=1 claude -p "$RESUME_PROMPT" \
    --dangerously-skip-permissions \
    --max-turns "$MAX_TURNS" \
    --output-format text \
    > "$SESSION_OUT" 2>&1 || true

EXIT_CODE=$?
# Append session output to main log
cat "$SESSION_OUT" >> "$LOG_FILE"

echo "[$(date)] === Session #$SESSION_NUM ended (exit: $EXIT_CODE) ===" >> "$LOG_FILE"

# --- Test mode: just report and exit ---
if [[ "${1:-}" == "----test" ]]; then
    echo "TEST COMPLETE" > "$STATUS_FILE"
    exit 0
fi

# --- Check chain signal in session output ---
SESSION_OUT="$CHAIN_DIR/session_${SESSION_NUM}.txt"
if grep -q "CHAIN_SPAWN_NEXT" "$SESSION_OUT" 2>/dev/null; then
    echo "[$(date)] Chain signal detected, spawning session #$((SESSION_NUM + 1))..." >> "$LOG_FILE"
    echo "SPAWNING_NEXT" > "$STATUS_FILE"
    exec bash Scripts/chain.sh
elif echo "$SESSION_OUTPUT" | grep -q "COMPLETE"; then
    echo "[$(date)] Task marked COMPLETE. Chain halted." | tee -a "$LOG_FILE"
    echo "COMPLETE" > "$STATUS_FILE"
else
    echo "[$(date)] Session ended without chain signal. Chain halted." | tee -a "$LOG_FILE"
    echo "HALTED (session $SESSION_NUM ended, no chain signal)" > "$STATUS_FILE"
fi
