<#
.SYNOPSIS
    RVV Backend: Benchmark Runner (QEMU)
.DESCRIPTION
    Executes the performance benchmarks under QEMU emulation.

    NOTE: Timing results under QEMU are NOT representative of real hardware.
    This script is primarily for correctness under load (stress testing) and
    verifying instruction retire counts if QEMU plugin logging is enabled.
.PARAMETER Compiler
    The compiler identifier used during build (clang/gcc). Default: "clang".
.PARAMETER VLEN
    The target Vector Length. Default: 128.
.PARAMETER Filter
    Regex to filter specific benchmarks. Default: ".*".
#>
Param(
    [string]$Compiler = "clang",
    [int]$VLEN = 128,
    [string]$Filter = ".*"
)

# 1. Source Common Config
# --------------------------------------------------------------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. "$ScriptDir\common.ps1"

# 2. Check Prerequisites
# --------------------------------------------------------------------------
if (-not (Check-Dependency "qemu-riscv64")) { exit 1 }

# 3. Setup Configuration
# --------------------------------------------------------------------------
$BuildDir = Get-BuildDir -CompilerId $Compiler -Vlen $VLEN

if (-not (Test-Path $BuildDir)) {
    Log-Error "Build directory not found: $BuildDir"
    Log-Error "Please run build_rvv_$Compiler.ps1 first."
    exit 1
}

# 4. Configure Environment
# --------------------------------------------------------------------------
$CpuStr = Get-QemuCpu -TargetVlen $VLEN
$QemuCmd = "qemu-riscv64 -cpu $CpuStr -L /usr/riscv64-linux-gnu"

# Force the library to use RVV implementation exclusively
$env:SIMDJSON_FORCE_IMPLEMENTATION = "rvv"

Log-Info "Benchmark Configuration:"
Log-Info "  Backend:   RVV (Forced)"
Log-Info "  Emulator:  $QemuCmd"
Log-Info "  Filter:    $Filter"
Log-Warn "  Note:      Running benchmarks on QEMU is extremely slow."

# 5. Define Targets
# --------------------------------------------------------------------------
# Standard benchmarks produced by the simdjson build system
$BenchTargets = @(
    "bench_minify",
    "bench_dom",
    "bench_ondemand"
)

# 6. Execution Loop
# --------------------------------------------------------------------------
$GlobalFail = $false

foreach ($Target in $BenchTargets) {
    # Try to find the binary in likely locations
    $BinaryPath = "$BuildDir\benchmark\$Target"
    if (-not (Test-Path $BinaryPath)) {
        $BinaryPath = "$BuildDir\bin\$Target"
    }

    if (Test-Path $BinaryPath) {
        Log-Info "--------------------------------------------------"
        Log-Info "Running: $Target"
        Log-Info "--------------------------------------------------"

        # We invoke QEMU directly on the binary
        # We limit iterations to keep runtime reasonable under emulation
        # --benchmark_filter regex provided by user
        # --benchmark_min_time=0.01 reduces the wait time significantly
        $ArgsList = @($BinaryPath, "--benchmark_filter=$Filter", "--benchmark_min_time=0.01")

        # We construct the full command line string for display
        Log-Cmd "$QemuCmd $BinaryPath --benchmark_filter=$Filter --benchmark_min_time=0.01"

        # Execution
        # Note: PowerShell's Start-Process separating arguments for the emulator vs the target is tricky.
        # It's often easier to rely on the wrapper variable if the binary respects it,
        # but here we manually invoke: qemu [flags] binary [binary_flags]

        # Argument list for QEMU: [cpu flags] [binary path] [binary flags]
        # We need to split the QEMU command string into args
        $QemuArgs = $QemuCmd.Split(' ') | Where-Object { $_ }
        $FinalArgs = $QemuArgs + $ArgsList

        $Process = Start-Process -FilePath "qemu-riscv64" -ArgumentList $FinalArgs -PassThru -NoNewWindow -Wait

        if ($Process.ExitCode -ne 0) {
            Log-Error "Benchmark $Target crashed or failed."
            $GlobalFail = $true
        }
    } else {
        Log-Warn "Benchmark binary not found: $Target (Skipping)"
    }
}

# 7. Cleanup
# --------------------------------------------------------------------------
Remove-Item Env:\SIMDJSON_FORCE_IMPLEMENTATION

if ($GlobalFail) {
    Log-Error "One or more benchmarks failed to complete."
    exit 1
} else {
    Log-Info "All benchmarks completed."
    exit 0
}
