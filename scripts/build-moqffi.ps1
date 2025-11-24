[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string[]]$Features = @("with_moq_draft07"),
    [switch]$Clean,
    [string]$SourceRoot,
    [string]$DestinationRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Get-Command cargo -ErrorAction SilentlyContinue)) {
    throw "cargo is not available in PATH. Please install Rust via https://rustup.rs/."
}

if (-not (Get-Command rustc -ErrorAction SilentlyContinue)) {
    throw "rustc is not available in PATH. Please install Rust via https://rustup.rs/."
}

function Get-RustHostTriple {
    try {
        $output = & rustc -vV
        foreach ($line in $output) {
            if ($line -match '^host:\s*(.+)$') {
                return $matches[1].Trim()
            }
        }
    }
    catch {
        Write-Warning "Failed to query rustc host triple: $_"
    }
    return $null
}

function Get-HostPlatformName {
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "Win64"
    }
    elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "Linux"
    }
    elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) {
        return "Mac"
    }

    throw "Unsupported operating system for moq-ffi build sync."
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptRoot "..")

if (-not $SourceRoot -or $SourceRoot.Trim().Length -eq 0) {
    $SourceRoot = Join-Path $repoRoot "External\\moq-ffi\\moq_ffi"
}
$sourceResolved = (Resolve-Path $SourceRoot -ErrorAction Stop).Path

if (-not $DestinationRoot -or $DestinationRoot.Trim().Length -eq 0) {
    $DestinationRoot = Join-Path $repoRoot "UnrealMoQ\\ThirdParty\\moq-ffi"
}
$destinationResolved = (Resolve-Path -Path ((New-Item -ItemType Directory -Path $DestinationRoot -Force).FullName)).Path

# Clean up legacy layouts if they still exist (pre-include/lib structure)
$legacyPath = Join-Path $destinationResolved "moq_ffi"
if (Test-Path $legacyPath) {
    Write-Host "Removing legacy ThirdParty layout at '$legacyPath'."
    Remove-Item -Path $legacyPath -Recurse -Force
}

$targetSubdir = if ($Configuration -eq "Release") { "release" } else { "debug" }
$targetRoot = Join-Path $sourceResolved "target"
$requestedTriple = if ($env:CARGO_BUILD_TARGET) { $env:CARGO_BUILD_TARGET } else { Get-RustHostTriple }

$targetSourceDir = $null
$candidateDirs = @()
if ($requestedTriple) {
    $candidateDirs += Join-Path $targetRoot (Join-Path $requestedTriple $targetSubdir)
}
$candidateDirs += Join-Path $targetRoot $targetSubdir

foreach ($candidate in $candidateDirs) {
    if (Test-Path $candidate) {
        $targetSourceDir = (Resolve-Path $candidate).Path
        break
    }
}

Push-Location $sourceResolved
try {
    if ($Clean) {
        Write-Host "Running cargo clean..."
        & cargo clean
    }

    $cargoArgs = @("build")
    if ($Configuration -eq "Release") {
        $cargoArgs += "--release"
    }

    if ($Features -and $Features.Count -gt 0) {
        $cargoArgs += "--features"
        $cargoArgs += ($Features -join ",")
    }

    Write-Host "Running cargo $($cargoArgs -join ' ')"
    & cargo @cargoArgs
    if ($LASTEXITCODE -ne 0) {
        throw "cargo build failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

if (-not $targetSourceDir) {
    $searched = $candidateDirs -join "', '"
    throw "Expected build artifacts under target directory but none were found. Checked: '$searched'."
}

Write-Host "Using artifact directory: $targetSourceDir"

$includeSource = Join-Path $sourceResolved "include"
$includeDest = Join-Path $destinationResolved "include"
if (Test-Path $includeDest) {
    Remove-Item -Path $includeDest -Recurse -Force
}
New-Item -Path $includeDest -ItemType Directory -Force | Out-Null
Copy-Item -Path (Join-Path $includeSource '*') -Destination $includeDest -Recurse -Force

$platformName = Get-HostPlatformName
$libRoot = Join-Path $destinationResolved "lib"
$platformLibDest = Join-Path $libRoot $platformName
if (Test-Path $platformLibDest) {
    Remove-Item -Path $platformLibDest -Recurse -Force
}
New-Item -Path $platformLibDest -ItemType Directory -Force | Out-Null

$staticLibName = if ($platformName -eq "Win64") { "moq_ffi.lib" } else { "libmoq_ffi.a" }
$staticLibSource = Join-Path $targetSourceDir $staticLibName
if (-not (Test-Path $staticLibSource)) {
    throw "Static library '$staticLibName' was not found in '$targetSourceDir'."
}
Copy-Item -Path $staticLibSource -Destination (Join-Path $platformLibDest (Split-Path -Leaf $staticLibSource)) -Force

Write-Host "Headers copied to '$includeDest'."
Write-Host "Static library '$staticLibName' copied to '$platformLibDest'."

Write-Host "moq-ffi headers and static libraries synced to '$destinationResolved'."
