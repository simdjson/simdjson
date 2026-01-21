<#
.SYNOPSIS
    RVV Backend: Common PowerShell Functions
.DESCRIPTION
    This script provides shared utilities for the RVV CI/Dev scripts on Windows.
    It parses 'common.env' to maintain the Single Source of Truth.
.EXAMPLE
    . "$PSScriptRoot\common.ps1"
#>

# 1. Resolve Script Directory & Source Config
# --------------------------------------------------------------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$EnvFile = Join-Path $ScriptDir "common.env"

if (-not (Test-Path $EnvFile)) {
    Write-Error "Error: Could not find configuration file at $EnvFile"
    exit 1
}

# Function to parse Bash-style .env files into PowerShell variables
function Import-EnvFile {
    param([string]$Path)
    Get-Content $Path | ForEach-Object {
        $line = $_.Trim()
        if ($line -and -not $line.StartsWith("#")) {
            if ($line -match '^([A-Za-z_][A-Za-z0-9_]*)=(.*)$') {
                $name = $matches[1]
                $value = $matches[2]

                # Strip quotes if present
                if ($value.StartsWith('"') -and $value.EndsWith('"')) {
                    $value = $value.Substring(1, $value.Length - 2)
                }

                # Set variable in Script scope (global to the session importing this)
                Set-Variable -Name $name -Value $value -Scope Script
            }
        }
    }
}

Import-EnvFile -Path $EnvFile

# 2. Colors & Logging
# --------------------------------------------------------------------------
function Log-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Green
}

function Log-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Log-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Log-Cmd {
    param([string]$Message)
    Write-Host "> $Message" -ForegroundColor Cyan
}

# 3. Environment Verification
# --------------------------------------------------------------------------
function Check-Dependency {
    param([string]$CommandName)
    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) {
        Log-Error "Missing required dependency: $CommandName"
        return $false
    }
    return $true
}

# 4. QEMU Configuration Helper
# --------------------------------------------------------------------------
function Get-QemuCpu {
    param([string]$TargetVlen)

    if (-not $TargetVlen) {
        Log-Error "Get-QemuCpu called without VLEN argument"
        return ""
    }

    # The QEMU_CPU_TEMPLATE from common.env looks like:
    # rv64,v=true,...,vlen=${VLEN}
    # We need to replace '${VLEN}' with the actual number.

    # Note: $QEMU_CPU_TEMPLATE is available because we imported common.env
    if (-not $script:QEMU_CPU_TEMPLATE) {
        Log-Error "QEMU_CPU_TEMPLATE not found. Did common.env load correctly?"
        return ""
    }

    $CpuStr = $script:QEMU_CPU_TEMPLATE.Replace('${VLEN}', $TargetVlen)
    return $CpuStr
}

# 5. Build Directory Helper
# --------------------------------------------------------------------------
function Get-BuildDir {
    param(
        [string]$CompilerId,
        [string]$Vlen
    )
    # Uses the MARCH_BASELINE variable from common.env
    return "build\rvv-${CompilerId}-${script:MARCH_BASELINE}-vlen${Vlen}"
}

function Prepare-BuildDir {
    param([string]$Dir)
    if (-not (Test-Path $Dir)) {
        Log-Info "Creating build directory: $Dir"
        New-Item -ItemType Directory -Force -Path $Dir | Out-Null
    }
}

Log-Info "RVV Environment Loaded. Implementation: $script:IMPL_KEY"
