[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$HostRoot,
    [string]$HostProjectName = "UnrealMoQHost",
    [string[]]$EditorArgs = @("-log"),
    [switch]$ReuseExisting
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
$editorExe = Join-Path $engineRootResolved "Engine\\Binaries\\Win64\\UnrealEditor.exe"
if (-not (Test-Path $editorExe)) {
    throw "UnrealEditor.exe not found at '$editorExe'."
}

$engineVersionFile = Join-Path $engineRootResolved "Engine\\Build\\Build.version"
$engineAssociation = "5.7"
if (Test-Path $engineVersionFile) {
    try {
        $versionData = Get-Content -Path $engineVersionFile -Raw | ConvertFrom-Json
        if ($versionData.MajorVersion -and $versionData.MinorVersion -ne $null) {
            $engineAssociation = "{0}.{1}" -f $versionData.MajorVersion, $versionData.MinorVersion
        }
    } catch {
        Write-Warning "Failed to read Build.version. Falling back to default engine association '$engineAssociation'."
    }
}

if (-not $HostRoot -or $HostRoot.Trim().Length -eq 0) {
    $HostRoot = Join-Path $repoRoot "Saved\\HostProject"
}

if (-not (Test-Path $HostRoot)) {
    New-Item -Path $HostRoot -ItemType Directory -Force | Out-Null
}

$hostRootResolved = Join-Path $HostRoot $HostProjectName
$uprojectPath = Join-Path $hostRootResolved ("{0}.uproject" -f $HostProjectName)
$pluginSource = Join-Path $repoRoot "UnrealMoQ"
$pluginDest = Join-Path $hostRootResolved "Plugins\\UnrealMoQ"

if (-not (Test-Path $pluginSource)) {
    throw "Unable to locate plugin sources at '$pluginSource'."
}

if (-not $ReuseExisting -and (Test-Path $hostRootResolved)) {
    Write-Host "Removing existing host project at '$hostRootResolved'."
    Remove-Item -Path $hostRootResolved -Recurse -Force
}

if (-not (Test-Path $hostRootResolved)) {
    New-Item -Path $hostRootResolved -ItemType Directory -Force | Out-Null
}

New-Item -Path (Join-Path $hostRootResolved "Config") -ItemType Directory -Force | Out-Null
New-Item -Path (Join-Path $hostRootResolved "Content") -ItemType Directory -Force | Out-Null
New-Item -Path (Join-Path $hostRootResolved "Plugins") -ItemType Directory -Force | Out-Null
if (-not (Test-Path $pluginDest)) {
    New-Item -Path $pluginDest -ItemType Directory -Force | Out-Null
}

Write-Host "Syncing plugin into host project..."
$robocopyArgs = @(
    $pluginSource,
    $pluginDest,
    "/MIR",
    "/NFL","/NDL","/NJH","/NJS","/NC","/NS","/NP",
    "/XD", ".git",".github","Binaries","Intermediate","Saved",".vs",".idea",".vscode"
)

$robocopyArgs += "/XF"
$robocopyArgs += @(".gitignore",".gitattributes",".gitmodules")

robocopy @robocopyArgs | Out-Null
$roboExit = $LASTEXITCODE
if ($roboExit -ge 8) {
    throw "Robocopy failed while copying plugin (exit code $roboExit)."
}

$uproject = [ordered]@{
    FileVersion       = 3
    EngineAssociation = $engineAssociation
    Category          = "Samples"
    Description       = "Temporary host project for the UnrealMoQ plugin."
    Modules           = @()
    Plugins           = @(
        @{ Name = "UnrealMoQ"; Enabled = $true }
    )
}

$uprojectJson = $uproject | ConvertTo-Json -Depth 5
Set-Content -Path $uprojectPath -Value $uprojectJson -Encoding UTF8

Write-Host "Launching Unreal Editor..."
& $editorExe $uprojectPath @EditorArgs
exit $LASTEXITCODE
