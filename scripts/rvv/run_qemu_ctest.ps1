<#
.SYNOPSIS
    RVV Backend: CTest Runner (QEMU)
.DESCRIPTION
    Executes the full CTest suite for the cross-compiled RVV build using QEMU.
    It automatically wraps test binaries with the emulator via SIMDJSON_TEST_WRAPPER.
.PARAMETER Compiler
    The compiler identifier used during build (clang/gcc). Default: "clang".
.PARAMETER VLEN
    The target Vector Length. Default: 128.
.PARAMETER Filter
    Optional regex filter for CTest (e.g. "rvv"). Default: "rvv".
#>
Param(
    [string]$Compiler = "clang",
    [int]$VLEN = 128,
    [string]$Filter = "rvv"
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

# 4. Configure QEMU Wrapper
# --------------------------------------------------------------------------
# We construct the QEMU command that CTest will prefix to every binary.
$CpuStr = Get-QemuCpu -TargetVlen $VLEN
$QemuCmd = "qemu-riscv64 -cpu $CpuStr -L /usr/riscv64-linux-gnu"

# Important: Set the wrapper environment variable for simdjson's test system
$env:SIMDJSON_TEST_WRAPPER = $QemuCmd

Log-Info "Test Configuration:"
Log-Info "  Build Dir: $BuildDir"
Log-Info "  Wrapper:   $env:SIMDJSON_TEST_WRAPPER"
Log-Info "  Filter:    $Filter"

# 5. Execute CTest
# --------------------------------------------------------------------------
try {
    Log-Info "Starting CTest..."
    Set-Location -Path $BuildDir

    # --output-on-failure: See what went wrong immediately
    # -R: Run only tests matching regex (usually "rvv")
    # -j: Parallel execution (QEMU handles this well)
    $CTestArgs = @("--output-on-failure", "-R", $Filter, "-j", "$env:NUMBER_OF_PROCESSORS")

    Log-Cmd "ctest $CTestArgs"
    $Process = Start-Process -FilePath "ctest" -ArgumentList $CTestArgs -PassThru -NoNewWindow -Wait

    if ($Process.ExitCode -eq 0) {
        Log-Info "All Tests Passed."
        exit 0
    } else {
        Log-Error "Some tests failed. See output above."
        exit 1
    }
}
catch {
    Log-Error $_.Exception.Message
    exit 1
}
finally {
    # Cleanup environment variable just in case
    Remove-Item Env:\SIMDJSON_TEST_WRAPPER
    Set-Location -Path $ScriptDir
}
