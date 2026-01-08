<#
.SYNOPSIS
    RVV Backend: Build Script (GCC)
.DESCRIPTION
    Configures and builds the project for RISC-V 64-bit using GCC.
    Requires a valid RISC-V GCC cross-toolchain in the PATH.
.PARAMETER VLEN
    The target Vector Length (used for directory naming/separation). Default: 128.
.PARAMETER Clean
    If set, deletes the build directory before starting.
.PARAMETER Generator
    The CMake generator to use. Default: "Ninja".
#>
Param(
    [int]$VLEN = 128,
    [switch]$Clean,
    [string]$Generator = "Ninja"
)

# 1. Source Common Config
# --------------------------------------------------------------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. "$ScriptDir\common.ps1"

# 2. Check Prerequisites
# --------------------------------------------------------------------------
if (-not (Check-Dependency "cmake")) { exit 1 }
# Check for the specific RISC-V GCC binary usually found in cross-toolchains
if (-not (Check-Dependency "riscv64-linux-gnu-g++")) {
    Log-Error "GCC cross-compiler (riscv64-linux-gnu-g++) not found in PATH."
    exit 1
}
if ($Generator -eq "Ninja") {
    if (-not (Check-Dependency "ninja")) { exit 1 }
}

# 3. Setup Configuration
# --------------------------------------------------------------------------
$CompilerId = "gcc"
$BuildDir = Get-BuildDir -CompilerId $CompilerId -Vlen $VLEN
$SourceDir = Resolve-Path "$ScriptDir\..\.."

Log-Info "Configuration:"
Log-Info "  Compiler:  $CompilerId"
Log-Info "  VLEN:      $VLEN (Directory Label)"
Log-Info "  Build Dir: $BuildDir"

# Clean if requested
if ($Clean -and (Test-Path $BuildDir)) {
    Log-Info "Cleaning build directory..."
    Remove-Item -Path $BuildDir -Recurse -Force
}
Prepare-BuildDir $BuildDir

# 4. Toolchain Strategy
# --------------------------------------------------------------------------
# We prioritize a dedicated toolchain file if it exists (M9).
# Otherwise, we use manual cross-compile flags.
$ToolchainFile = Join-Path $SourceDir "cmake\toolchains\rvv-qemu-gcc.cmake"
$CmakeArgs = @("-G", $Generator, "-S", $SourceDir, "-B", $BuildDir)

if (Test-Path $ToolchainFile) {
    Log-Info "Using Toolchain File: $ToolchainFile"
    $CmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile"
} else {
    Log-Warn "No specific toolchain file found. Using manual cross-compile flags."

    # Manual Cross-Compile Flags for GCC
    $CmakeArgs += "-DCMAKE_SYSTEM_NAME=Linux"
    $CmakeArgs += "-DCMAKE_SYSTEM_PROCESSOR=riscv64"
    $CmakeArgs += "-DCMAKE_C_COMPILER=riscv64-linux-gnu-gcc"
    $CmakeArgs += "-DCMAKE_CXX_COMPILER=riscv64-linux-gnu-g++"

    # Target Flags (Architecture + ABI)
    $TargetFlags = "-march=$script:MARCH_BASELINE -mabi=$script:MABI"
    $CmakeArgs += "-DCMAKE_CXX_FLAGS=""$TargetFlags"""
    $CmakeArgs += "-DCMAKE_C_FLAGS=""$TargetFlags"""
}

# Append Common Flags (simdjson options)
if ($script:COMMON_CMAKE_FLAGS) {
    $script:COMMON_CMAKE_FLAGS.Split(' ') | ForEach-Object {
        if (-not [string]::IsNullOrWhiteSpace($_)) {
            $CmakeArgs += $_
        }
    }
}

# 5. Execution
# --------------------------------------------------------------------------
try {
    Log-Info "Step 1: Configuring..."
    Log-Cmd "cmake $CmakeArgs"

    # Run CMake Configure
    $Process = Start-Process -FilePath "cmake" -ArgumentList $CmakeArgs -PassThru -NoNewWindow -Wait
    if ($Process.ExitCode -ne 0) { throw "CMake configuration failed." }

    Log-Info "Step 2: Building..."
    $BuildArgs = @("--build", $BuildDir, "--config", "Release")
    Log-Cmd "cmake $BuildArgs"

    # Run CMake Build
    $Process = Start-Process -FilePath "cmake" -ArgumentList $BuildArgs -PassThru -NoNewWindow -Wait
    if ($Process.ExitCode -ne 0) { throw "Build failed." }

    Log-Info "Build Complete!"
    Log-Info "Artifacts are in: $BuildDir"
}
catch {
    Log-Error $_.Exception.Message
    exit 1
}
