[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [ValidateSet("Debug","Development","Shipping")]
    [string]$Configuration = "Development",
    [string]$TargetPlatforms = "Win64",
    [string]$PackageDir,
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptRoot "..")

if (-not $EngineRoot -or $EngineRoot.Trim().Length -eq 0) {
    $EngineRoot = "C:\\Program Files\\Epic Games\\UE_5.7"
}

if (-not (Test-Path $EngineRoot)) {
    throw "Unable to find Unreal Engine root at '$EngineRoot'. Set UE_ROOT or pass -EngineRoot."
}

$engineRootResolved = (Resolve-Path $EngineRoot).Path
$runUatPath = Join-Path $engineRootResolved "Engine\\Build\\BatchFiles\\RunUAT.bat"
if (-not (Test-Path $runUatPath)) {
    throw "RunUAT.bat not found at '$runUatPath'."
}

$pluginDescriptor = Join-Path $repoRoot "UnrealMoQ\\UnrealMoQ.uplugin"
if (-not (Test-Path $pluginDescriptor)) {
    throw "Unable to locate UnrealMoQ.uplugin at '$pluginDescriptor'."
}
$pluginDescriptorPath = (Resolve-Path $pluginDescriptor).Path

if (-not $PackageDir -or $PackageDir.Trim().Length -eq 0) {
    $PackageDir = Join-Path $repoRoot "Saved\\Builds\\UnrealMoQ"
}

if ($Clean -and (Test-Path $PackageDir)) {
    Remove-Item -Path $PackageDir -Recurse -Force
}

if (-not (Test-Path $PackageDir)) {
    New-Item -Path $PackageDir -ItemType Directory -Force | Out-Null
}

$packagePath = (Resolve-Path $PackageDir).Path

$arguments = @(
    "BuildPlugin",
    "-Plugin=`"$pluginDescriptorPath`"",
    "-Package=`"$packagePath`"",
    "-TargetPlatforms=$TargetPlatforms",
    "-Configuration=$Configuration"
)

Write-Host "Running RunUAT with arguments:`n$($arguments -join ' ')"
& $runUatPath @arguments
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "BuildPlugin failed with exit code $exitCode."
}

Write-Host "Plugin build completed. Artifacts available in '$packagePath'."
