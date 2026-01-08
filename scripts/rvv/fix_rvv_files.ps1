# scripts/rvv/fix_rvv_files.ps1
# Purpose: Normalize RVV-related text files only (CRLF->LF, trim trailing whitespace, ensure final newline)
# Changes requested:
#  - Do NOT close the console at the end (no exit); instead return.
#  - List files validated OK (unchanged) and files changed.

[CmdletBinding()]
param(
  [switch] $DryRun,
  [switch] $IncludeUntracked
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Info([string]$msg)  { Write-Host "[INFO]  $msg" }
function Write-Warn([string]$msg)  { Write-Host "[WARN]  $msg" }
function Write-Err ([string]$msg)  { Write-Host "[ERROR] $msg" }

function Ensure-Git {
  if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git not found in PATH."
  }
}

function Is-RepoRoot {
  if (Test-Path -LiteralPath ".git") { return $true }
  try {
    $inside = (git rev-parse --is-inside-work-tree 2>$null)
    return ($inside -eq "true")
  } catch {
    return $false
  }
}

function Normalize-Text([string]$text) {
  $t = $text -replace "`r`n", "`n"
  $t = ($t -split "`n", -1) | ForEach-Object { $_ -replace "[`t ]+$", "" } | Join-String -Separator "`n"
  if ($t.Length -gt 0 -and -not $t.EndsWith("`n")) { $t += "`n" }
  return $t
}

function Get-RvvCandidateFiles {
  $pathSpecs = @(
    ".github/workflows/rvv-*.yml",
    ".github/workflows/rvv-*.yaml",
    "cmake/toolchains/rvv-*.cmake",
    "include/simdjson/rvv.h",
    "include/simdjson/rvv/**",
    "src/rvv.cpp",
    "scripts/rvv/**",
    "tools/rvv/**",
    "tests/rvv/**",
    "extra/rvv/**",
    "benchmark/rvv/**"
  )

  Write-Info "Collecting RVV candidate files (tracked$(if($IncludeUntracked){' + untracked'}else{''}))..."
  Write-Info ("Pathspecs:`n  - " + ($pathSpecs -join "`n  - "))

  $tracked = @(git ls-files -- @pathSpecs 2>$null)
  Write-Info ("Tracked matches: {0}" -f $tracked.Count)

  $set = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
  foreach ($f in $tracked) { if ($f) { [void]$set.Add($f) } }

  if ($IncludeUntracked) {
    $untracked = @(git ls-files --others --exclude-standard -- @pathSpecs 2>$null)
    Write-Info ("Untracked matches: {0}" -f $untracked.Count)
    foreach ($f in $untracked) { if ($f) { [void]$set.Add($f) } }
  }

  $textExt = @(".h",".hpp",".c",".cc",".cpp",".inl",".md",".txt",".cmake",".yml",".yaml",".sh",".ps1",".patch")

  $out = New-Object System.Collections.Generic.List[string]
  foreach ($f in ($set | Sort-Object)) {
    if (-not $f) { continue }
    if (-not (Test-Path -LiteralPath $f)) {
      Write-Warn "Listed by git but not found on disk: $f"
      continue
    }

    $ext = [System.IO.Path]::GetExtension($f)
    if ($textExt -notcontains $ext) {
      Write-Info "Skipping (non-text ext '$ext'): $f"
      continue
    }

    $out.Add($f) | Out-Null
  }

  return ,($out.ToArray())
}

# MAIN (no 'exit' so it won't close the console when run directly)
try {
  Ensure-Git

  if (-not (Is-RepoRoot)) {
    throw "Run this from the repo root (where .git exists), or cd into the repo first."
  }

  Write-Info ("Repo root: {0}" -f (Get-Location))
  if ($DryRun) { Write-Info "DryRun enabled: no files will be written." }

  $targets = Get-RvvCandidateFiles
  Write-Info ("RVV fix scope: {0} file(s)" -f $targets.Count)
  if ($targets.Count -eq 0) {
    Write-Info "Nothing to do."
    return
  }

  $changed   = New-Object System.Collections.Generic.List[string]
  $ok        = New-Object System.Collections.Generic.List[string]
  $failed    = New-Object System.Collections.Generic.List[string]

  foreach ($f in $targets) {
    Write-Info "Checking: $f"
    try {
      $orig = Get-Content -LiteralPath $f -Raw
      $norm = Normalize-Text $orig

      if ($orig -ne $norm) {
        $changed.Add($f) | Out-Null
        Write-Info "  -> Needs normalization."

        if (-not $DryRun) {
          [System.IO.File]::WriteAllText((Resolve-Path $f), $norm, (New-Object System.Text.UTF8Encoding($false)))
          Write-Info "  -> Written."
        } else {
          Write-Info "  -> DryRun: not written."
        }
      } else {
        $ok.Add($f) | Out-Null
        Write-Info "  -> OK (already normalized)."
      }
    } catch {
      $failed.Add($f) | Out-Null
      Write-Warn ("  -> FAILED: {0}" -f $_.Exception.Message)
    }
  }

  Write-Host ""
  Write-Info "Summary:"
  Write-Host ("  Scope:   {0}" -f $targets.Count)
  Write-Host ("  OK:      {0}" -f $ok.Count)
  Write-Host ("  Changed: {0}" -f $changed.Count)
  Write-Host ("  Failed:  {0}" -f $failed.Count)

  Write-Host ""
  Write-Info "Files validated OK (already normalized):"
  if ($ok.Count -eq 0) { Write-Host "  (none)" } else { $ok | ForEach-Object { Write-Host ("  " + $_) } }

  Write-Host ""
  Write-Info "Files changed (normalized):"
  if ($changed.Count -eq 0) { Write-Host "  (none)" } else { $changed | ForEach-Object { Write-Host ("  " + $_) } }

  if ($failed.Count -gt 0) {
    Write-Host ""
    Write-Warn "Files failed:"
    $failed | ForEach-Object { Write-Host ("  " + $_) }
  }

  # Return a result object (useful if dot-sourced or invoked from another script)
  [pscustomobject]@{
    Scope   = $targets.Count
    Ok      = @($ok)
    Changed = @($changed)
    Failed  = @($failed)
    DryRun  = [bool]$DryRun
  }
  return
}
catch {
  Write-Err $_.Exception.Message
  # No exit; just return
  return
}
