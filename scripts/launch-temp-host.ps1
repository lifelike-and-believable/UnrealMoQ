[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$HostRoot,
    [string]$HostProjectName = "UnrealMoQHost",
    [string[]]$EditorArgs = @("-log"),
    [switch]$ReuseExisting,
    [switch]$SkipBuild,
    [switch]$Detach,
    [switch]$PrepareOnly
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
        if (($null -ne $versionData.MajorVersion) -and ($null -ne $versionData.MinorVersion)) {
            $engineAssociation = "{0}.{1}" -f $versionData.MajorVersion, $versionData.MinorVersion
        }
    } catch {
        Write-Warning "Failed to read Build.version. Falling back to default engine association '$engineAssociation'."
    }
}

if (-not $HostRoot -or $HostRoot.Trim().Length -eq 0) {
    $HostRoot = [System.IO.Path]::Combine($repoRoot, "Saved", "HostProject")
}
$HostRoot = [System.IO.Path]::GetFullPath($HostRoot)

if (-not (Test-Path $HostRoot)) {
    New-Item -Path $HostRoot -ItemType Directory -Force | Out-Null
}

$hostRootResolved = [System.IO.Path]::GetFullPath((Join-Path $HostRoot $HostProjectName))
$uprojectPath = Join-Path $hostRootResolved ("{0}.uproject" -f $HostProjectName)
$pluginDest = Join-Path $hostRootResolved "Plugins\UnrealMoQ"
$defaultPluginSource = Join-Path $repoRoot "UnrealMoQ"
$packagedPluginRoot = Join-Path $repoRoot "Saved\Builds\UnrealMoQ"
$packagedHostPlugin = Join-Path $packagedPluginRoot "HostProject\Plugins\UnrealMoQ"
$packagedPluginSource = if (Test-Path $packagedHostPlugin) { $packagedHostPlugin } else { $packagedPluginRoot }

if ($SkipBuild) {
    if (-not (Test-Path $packagedPluginSource)) {
        throw "SkipBuild requested but no packaged plugin was found under '$packagedPluginRoot'. Run the build-plugin task first."
    }
    $pluginSource = $packagedPluginSource
    Write-Host "SkipBuild enabled; copying plugin from packaged build at '$pluginSource'."
} else {
    if (-not (Test-Path $defaultPluginSource)) {
        throw "Unable to locate plugin sources at '$defaultPluginSource'."
    }
    $pluginSource = $defaultPluginSource
}

if (-not $ReuseExisting -and (Test-Path $hostRootResolved)) {
    Write-Host "Removing existing host project at '$hostRootResolved'."
    Remove-Item -Path $hostRootResolved -Recurse -Force
}

if (-not (Test-Path $hostRootResolved)) {
    New-Item -Path $hostRootResolved -ItemType Directory -Force | Out-Null
    $hostRootResolved = (Resolve-Path $hostRootResolved).Path
}

New-Item -Path (Join-Path $hostRootResolved "Config") -ItemType Directory -Force | Out-Null
New-Item -Path (Join-Path $hostRootResolved "Content\Developers") -ItemType Directory -Force | Out-Null
New-Item -Path (Join-Path $hostRootResolved "Plugins") -ItemType Directory -Force | Out-Null
if (-not (Test-Path $pluginDest)) {
    New-Item -Path $pluginDest -ItemType Directory -Force | Out-Null
}

# Remove stale build artifacts when we're rebuilding the module
if (-not $SkipBuild) {
    foreach ($staleDir in "Binaries","Intermediate","Saved") {
        $pluginDirToClean = Join-Path $pluginDest $staleDir
        if (Test-Path $pluginDirToClean) {
            Remove-Item -Path $pluginDirToClean -Recurse -Force -ErrorAction SilentlyContinue
        }

        $hostDirToClean = Join-Path $hostRootResolved $staleDir
        if (Test-Path $hostDirToClean) {
            Remove-Item -Path $hostDirToClean -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
} else {
    Write-Host "SkipBuild specified; preserving existing binaries in '$hostRootResolved'."
}

Write-Host "Syncing plugin into host project..."
$robocopyArgs = @(
    $pluginSource,
    $pluginDest,
    "/MIR",
    "/NFL","/NDL","/NJH","/NJS","/NC","/NS","/NP"
)

$excludeDirs = @(".git",".github",".vs",".idea",".vscode")
if (-not $SkipBuild) {
    $excludeDirs += "Binaries","Intermediate","Saved"
}
if ($excludeDirs.Count -gt 0) {
    $robocopyArgs += "/XD"
    $robocopyArgs += $excludeDirs
}

$robocopyArgs += "/XF"
$robocopyArgs += @(".gitignore",".gitattributes",".gitmodules")

robocopy @robocopyArgs | Out-Null
$roboExit = $LASTEXITCODE
if ($roboExit -ge 8) {
    throw "Robocopy failed while copying plugin (exit code $roboExit)."
}

if (-not $SkipBuild) {
    $buildBat = Join-Path $engineRootResolved "Engine\\Build\\BatchFiles\\Build.bat"
    if (-not (Test-Path $buildBat)) {
        throw "Unable to find Build.bat at '$buildBat'."
    }

    Write-Host "Compiling UnrealMoQ for the host project..."
    & $buildBat "UnrealEditor" "Win64" "Development" $uprojectPath | Write-Host
    if ($LASTEXITCODE -ne 0) {
        throw "UnrealBuildTool failed to build the host project (exit code $LASTEXITCODE)."
    }
} else {
    $editorBinary = Join-Path $pluginDest "Binaries\Win64\UnrealEditor-UnrealMoQ.dll"
    $gameBinary = Join-Path $pluginDest "Binaries\Win64\UnrealGame-UnrealMoQ.dll"
    if ((-not (Test-Path $editorBinary)) -and (-not (Test-Path $gameBinary))) {
        Write-Warning "SkipBuild requested but no plugin binary was found at '$pluginDest\Binaries\Win64'. Unreal may prompt to rebuild."
    }
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

if ($PrepareOnly) {
    Write-Host "Host project prepared at '$hostRootResolved'."
    exit 0
}

Write-Host "Launching Unreal Editor..."

if ($Detach) {
    $launchArgs = @($uprojectPath) + $EditorArgs
    $process = Start-Process -FilePath $editorExe -ArgumentList $launchArgs -PassThru
    Write-Host "Unreal Editor launched (PID $($process.Id)) in detached mode."
    exit 0
} else {
    & $editorExe $uprojectPath @EditorArgs
    exit $LASTEXITCODE
}
