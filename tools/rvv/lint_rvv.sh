#!/bin/bash
set -euo pipefail

# -----------------------------------------------------------------------------
# tools/rvv/lint_rvv.sh
# -----------------------------------------------------------------------------
# RVV-only linter for simdjson RVV backend.
#
# Checks (RVV scope only):
#  1) Header include guards: present, consistent, unique (no collisions).
#  2) Guard naming: matches file-based expected macro (best-effort).
#  3) LMUL policy: flags any obvious LMUL>1 usage in RVV sources/headers.
#  4) QEMU_CPU drift: scripts/docs/workflows mentioning RVV should use the
#     canonical template: rv64,v=true,vext_spec=v1.0,vlen=${VLEN}
#  5) Force-implementation env var drift: standardize on SIMDJSON_FORCE_IMPLEMENTATION.
#
# Exit code:
#  - 0 if clean
#  - 1 if errors
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Allow override: tools/rvv/lint_rvv.sh --root /path/to/repo
if [[ "${1:-}" == "--root" ]]; then
  [[ -n "${2:-}" ]] || { echo "Error: --root requires a path" >&2; exit 2; }
  REPO_ROOT="$(cd "${2}" && pwd)"
  shift 2
fi

VERBOSE=0
if [[ "${1:-}" == "--verbose" ]]; then
  VERBOSE=1
  shift
fi

log() { echo "[$(basename "$0")] $*"; }
vlog() { [[ "$VERBOSE" -eq 1 ]] && log "$@"; }

die() { echo "Error: $*" >&2; exit 2; }

need_cmd() { command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"; }

need_cmd find
need_cmd grep
need_cmd awk
need_cmd sed
need_cmd tr

# -----------------------------------------------------------------------------
# Scope (RVV only)
# -----------------------------------------------------------------------------
RVV_DIRS=(
  "include/simdjson/rvv"
  "src/rvv"
  "tests/rvv"
  "tools/rvv"
  "scripts/rvv"
  "doc/rvv"
  ".github/workflows"
)

# Build file lists (only existing dirs)
SCOPE_DIRS=()
for d in "${RVV_DIRS[@]}"; do
  [[ -d "${REPO_ROOT}/${d}" ]] && SCOPE_DIRS+=("${REPO_ROOT}/${d}")
done

if [[ "${#SCOPE_DIRS[@]}" -eq 0 ]]; then
  die "No RVV-related directories found under ${REPO_ROOT}"
fi

# Headers to validate guards
mapfile -t RVV_HEADERS < <(
  find "${REPO_ROOT}/include/simdjson/rvv" -type f \( -name "*.h" -o -name "*.hpp" \) 2>/dev/null | sort
)

# RVV code files to scan for LMUL usage
mapfile -t RVV_CODE < <(
  {
    [[ -d "${REPO_ROOT}/include/simdjson/rvv" ]] && find "${REPO_ROOT}/include/simdjson/rvv" -type f \( -name "*.h" -o -name "*.hpp" \)
    [[ -d "${REPO_ROOT}/src/rvv" ]] && find "${REPO_ROOT}/src/rvv" -type f \( -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \)
  } 2>/dev/null | sort
)

# Text files that may contain QEMU_CPU / FORCE var references (still RVV-related)
mapfile -t RVV_TEXT < <(
  {
    [[ -d "${REPO_ROOT}/scripts/rvv" ]] && find "${REPO_ROOT}/scripts/rvv" -type f
    [[ -d "${REPO_ROOT}/tools/rvv" ]] && find "${REPO_ROOT}/tools/rvv" -type f
    [[ -d "${REPO_ROOT}/doc/rvv" ]] && find "${REPO_ROOT}/doc/rvv" -type f
    [[ -d "${REPO_ROOT}/.github/workflows" ]] && find "${REPO_ROOT}/.github/workflows" -type f -name "*.yml" -o -name "*.yaml"
  } 2>/dev/null | sort
)

ERRORS=0
WARNINGS=0

fail() { echo "[ERROR] $*" >&2; ERRORS=$((ERRORS+1)); }
warn() { echo "[WARN ] $*" >&2; WARNINGS=$((WARNINGS+1)); }

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------
upper_macro() {
  # file base -> macro-ish: stage1.h -> STAGE1_H ; rvv_runtime.h -> RVV_RUNTIME_H
  local s="$1"
  s="$(echo "$s" | tr '[:lower:]' '[:upper:]')"
  # non-alnum -> underscore
  s="$(echo "$s" | sed -E 's/[^A-Z0-9]+/_/g; s/^_+//; s/_+$//')"
  echo "$s"
}

expected_guard_for_header() {
  local path="$1"
  local base
  base="$(basename "$path")"
  echo "SIMDJSON_RVV_$(upper_macro "$base")"
}

extract_guard_pair() {
  # Prints: "<ifndef_macro> <define_macro>" or empty if not found.
  local f="$1"
  # only consider the first #ifndef and first #define after it
  awk '
    BEGIN { ifndef=""; define="" }
    /^[[:space:]]*#[[:space:]]*ifndef[[:space:]]+[A-Za-z_][A-Za-z0-9_]*/ && ifndef=="" {
      ifndef=$2
      next
    }
    /^[[:space:]]*#[[:space:]]*define[[:space:]]+[A-Za-z_][A-Za-z0-9_]*/ && ifndef!="" && define=="" {
      define=$2
      print ifndef, define
      exit
    }
  ' "$f" 2>/dev/null || true
}

# -----------------------------------------------------------------------------
# 1) Include guards: present, match, unique
# -----------------------------------------------------------------------------
lint_include_guards() {
  vlog "Checking include guards in include/simdjson/rvv ..."
  if [[ "${#RVV_HEADERS[@]}" -eq 0 ]]; then
    warn "No RVV headers found to check include guards."
    return
  fi

  declare -A seen_guard=()
  declare -A seen_file=()

  for h in "${RVV_HEADERS[@]}"; do
    local pair
    pair="$(extract_guard_pair "$h")"
    if [[ -z "${pair}" ]]; then
      fail "Missing or malformed include guard: ${h#${REPO_ROOT}/}"
      continue
    fi

    local ifndef_macro define_macro
    ifndef_macro="$(echo "$pair" | awk '{print $1}')"
    define_macro="$(echo "$pair" | awk '{print $2}')"

    if [[ "${ifndef_macro}" != "${define_macro}" ]]; then
      fail "Guard mismatch (#ifndef vs #define) in ${h#${REPO_ROOT}/}: ${ifndef_macro} vs ${define_macro}"
      continue
    fi

    local guard="${ifndef_macro}"
    local expected
    expected="$(expected_guard_for_header "$h")"

    # Enforce prefix (soft)
    if [[ "${guard}" != SIMDJSON_RVV_* ]]; then
      warn "Guard does not start with SIMDJSON_RVV_ in ${h#${REPO_ROOT}/}: ${guard}"
    fi

    # Enforce expected-name (soft but useful to catch copy/paste collisions)
    if [[ "${guard}" != "${expected}" ]]; then
      warn "Guard name differs from expected in ${h#${REPO_ROOT}/}: got ${guard}, expected ${expected}"
    fi

    # Uniqueness check (hard)
    if [[ -n "${seen_guard[$guard]:-}" ]]; then
      fail "Include guard collision: ${guard} used by both ${seen_guard[$guard]} and ${h#${REPO_ROOT}/}"
    else
      seen_guard[$guard]="${h#${REPO_ROOT}/}"
    fi
  done
}

# -----------------------------------------------------------------------------
# 2) LMUL policy: flag obvious LMUL > 1 usage in RVV code
# -----------------------------------------------------------------------------
lint_lmul_policy() {
  vlog "Checking LMUL policy (m1 only) ..."
  if [[ "${#RVV_CODE[@]}" -eq 0 ]]; then
    warn "No RVV code files found to scan for LMUL usage."
    return
  fi

  # Patterns that strongly indicate LMUL>1 usage.
  # - vsetvl_*m2/m4/m8
  # - vector types ...m2_t, ...m4_t, ...m8_t
  # - reinterpret helpers for m2/m4/m8
  local patterns=(
    "__riscv_vsetvl_[A-Za-z0-9_]*m(2|4|8)\b"
    "\bv[a-z0-9]+m(2|4|8)_t\b"
    "__riscv_vreinterpret_v_[A-Za-z0-9_]*m(2|4|8)_[A-Za-z0-9_]*\b"
  )

  local hit=0
  for f in "${RVV_CODE[@]}"; do
    # Skip vendored or generated if any (best-effort)
    [[ "$f" == *"/.git/"* ]] && continue

    for re in "${patterns[@]}"; do
      if grep -nE "${re}" "$f" >/dev/null 2>&1; then
        hit=1
        break
      fi
    done

    if [[ "$hit" -eq 1 ]]; then
      # show a few lines per file
      warn "LMUL>1 indicators found in ${f#${REPO_ROOT}/} (policy is LMUL=m1)."
      for re in "${patterns[@]}"; do
        grep -nE "${re}" "$f" 2>/dev/null | head -n 5 | sed "s|^|  |"
      done
      hit=0
      # treat as ERROR (policy-locked)
      ERRORS=$((ERRORS+1))
    fi
  done
}

# -----------------------------------------------------------------------------
# 3) QEMU_CPU template drift (scripts/docs/workflows mentioning RVV)
# -----------------------------------------------------------------------------
lint_qemu_cpu() {
  vlog "Checking QEMU_CPU template drift ..."
  local canonical_re='rv64,v=true,vext_spec=v1\.0,vlen=\$\{?VLEN\}?'
  local found_any=0

  for f in "${RVV_TEXT[@]}"; do
    # Only consider files that mention QEMU_CPU / qemu-riscv64 / rvv
    if ! grep -qiE 'QEMU_CPU|qemu-riscv64|rvv' "$f" 2>/dev/null; then
      continue
    fi
    found_any=1

    # Extract any cpu strings that look like rv64,...
    local lines
    lines="$(grep -nE 'rv64,.*vlen=' "$f" 2>/dev/null || true)"
    [[ -z "$lines" ]] && continue

    # If any line contains rv64,... but not the canonical fields, warn.
    while IFS= read -r line; do
      # Ignore comments that contain examples? Still check; drift in comments matters too.
      if echo "$line" | grep -qE "${canonical_re}"; then
        : # ok
      else
        warn "Non-canonical QEMU CPU template in ${f#${REPO_ROOT}/}: ${line}"
      fi
    done <<< "$lines"
  done

  [[ "$found_any" -eq 0 ]] && warn "No RVV-related files mentioning QEMU_CPU/qemu-riscv64 found to validate."
}

# -----------------------------------------------------------------------------
# 4) Force-implementation variable drift
# -----------------------------------------------------------------------------
lint_force_impl_var() {
  vlog "Checking force-implementation env var drift ..."
  local bad=0

  for f in "${RVV_TEXT[@]}"; do
    # Look for either name
    if grep -qE 'SIMDJSON_FORCE_IMPL\b|SIMDJSON_FORCE_IMPLEMENTATION\b|SIMDJSON_FORCE_IMPLEMENTATION=' "$f" 2>/dev/null; then
      :
    else
      continue
    fi

    if grep -qE 'SIMDJSON_FORCE_IMPL\b' "$f" 2>/dev/null; then
      warn "Found SIMDJSON_FORCE_IMPL in ${f#${REPO_ROOT}/}; standard is SIMDJSON_FORCE_IMPLEMENTATION."
      bad=1
    fi
  done

  # treat as warning only (repo might support aliases)
  [[ "$bad" -eq 1 ]] && WARNINGS=$((WARNINGS+0))
}

# -----------------------------------------------------------------------------
# Run all checks
# -----------------------------------------------------------------------------
log "Repo root: ${REPO_ROOT}"
log "Scope dirs:"
for d in "${SCOPE_DIRS[@]}"; do
  log "  - ${d#${REPO_ROOT}/}"
done

lint_include_guards
lint_lmul_policy
lint_qemu_cpu
lint_force_impl_var

echo ""
log "Summary: ${ERRORS} error(s), ${WARNINGS} warning(s)"

if [[ "$ERRORS" -ne 0 ]]; then
  exit 1
fi
exit 0
