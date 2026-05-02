#!/usr/bin/env bash
#	Stop hook: if PLAN.md still has unchecked items or the build is
#	broken, emit a JSON `block` decision so Claude resumes work.

set -uo pipefail

PLAN=/home/ben/src/altirraqt/PLAN.md

# Count unchecked GitHub-style checkboxes.
unchecked=$(grep -c '^- \[ \]' "$PLAN" 2>/dev/null || true)
unchecked=${unchecked:-0}

# Light-weight build check: only run if a previous build directory exists,
# and use --dry-run-style "no work to do" detection so we don't burn CPU on
# every Stop. If ninja/cmake aren't on PATH, skip the check entirely.
build_status="ok"
if [[ -d /home/ben/src/altirraqt/build ]] && command -v cmake >/dev/null 2>&1; then
  if ! cmake --build /home/ben/src/altirraqt/build -j8 >/tmp/altirraqt-build.log 2>&1; then
    build_status="failed"
  fi
fi

if [[ "$unchecked" -gt 0 || "$build_status" != "ok" ]]; then
  reason="If PLAN.md has unchecked boxes or validation failed, continue."
  if [[ "$unchecked" -gt 0 ]]; then
    reason+=" PLAN.md has $unchecked unchecked boxes."
  fi
  if [[ "$build_status" != "ok" ]]; then
    reason+=" Build failed (see /tmp/altirraqt-build.log)."
  fi
  reason+=" Per CLAUDE.md: implement end-to-end (no stubs)."
  printf '{"decision": "block", "reason": %s}\n' \
    "$(printf '%s' "$reason" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')"
fi
