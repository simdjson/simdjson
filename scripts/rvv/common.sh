#!/bin/bash
# ==========================================================================
# RVV Backend: Common Shell Functions
# ==========================================================================
# This script provides shared utilities for the RVV CI/Dev scripts.
# It sources 'common.env' to maintain the Single Source of Truth.
#
# Usage: source scripts/rvv/common.sh
# ==========================================================================

# 1. Resolve Script Directory & Source Config
# --------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${SCRIPT_DIR}/common.env"

if [ -f "$ENV_FILE" ]; then
    source "$ENV_FILE"
else
    echo "Error: Could not find configuration file at $ENV_FILE"
    exit 1
fi

# 2. Colors & Logging
# --------------------------------------------------------------------------
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_cmd() {
    echo -e "${CYAN}> $1${NC}"
}

# 3. Environment Verification
# --------------------------------------------------------------------------
check_dependency() {
    local cmd=$1
    if ! command -v "$cmd" &> /dev/null; then
        log_error "Missing required dependency: $cmd"
        return 1
    fi
    return 0
}

# 4. QEMU Configuration Helper
# --------------------------------------------------------------------------
# Usage: get_qemu_cpu <vlen>
# Returns the fully formed -cpu string based on the template in common.env
get_qemu_cpu() {
    local target_vlen=$1
    if [ -z "$target_vlen" ]; then
        log_error "get_qemu_cpu called without VLEN argument"
        return 1
    fi

    # We use envsubst-like behavior or simple eval to replace ${VLEN}
    # Since we are in bash, simple substitution is safest.
    # The template is: rv64,v=true,...,vlen=${VLEN}
    # We strip the literal '${VLEN}' and append the actual value,
    # or perform a string replace.

    # Simple, robust string replacement:
    local cpu_str="${QEMU_CPU_TEMPLATE//\$\{VLEN\}/$target_vlen}"
    echo "$cpu_str"
}

# 5. Build Directory Helper
# --------------------------------------------------------------------------
# Usage: ensure_build_dir <compiler_id> <vlen>
# Creates and returns the path to the specific build folder
get_build_dir() {
    local compiler=$1
    local vlen=$2
    echo "build/rvv-${compiler}-${MARCH_BASELINE}-vlen${vlen}"
}

prepare_build_dir() {
    local dir=$1
    if [ ! -d "$dir" ]; then
        log_info "Creating build directory: $dir"
        mkdir -p "$dir"
    fi
}
