param(
    [ValidateSet('all','vst','standalone')]
    [string]$Target = 'all'
)

$ErrorActionPreference = 'Stop'

$VersionTag = '2.2.4'
$Variants = @('a','b','c','d')
$PaymentUnlockRelativePath = 'assets\licensing\payment_unlock_v2_2_4_beta.json'

function Test-Command {
    param([string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-WingetPackage {
    param(
        [string]$PackageId,
        [string]$CommandName
    )

    if (Test-Command $CommandName) {
        return
    }

    if (-not (Test-Command 'winget')) {
        throw "winget not available. Install App Installer from Microsoft Store first."
    }

    Write-Host "Installing $PackageId"
    winget install --id $PackageId --accept-source-agreements --accept-package-agreements -e
}

function Resolve-FirstExisting {
    param([string[]]$Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    return $null
}

function Install-PaymentUnlockProof {
    param([string]$RootPath)

    $source = Join-Path $RootPath $PaymentUnlockRelativePath
    if (-not (Test-Path $source)) {
        Write-Warning "Payment proof bundle missing: $source"
        return
    }

    $licenseDir = Join-Path $env:APPDATA 'DawAI\license'
    New-Item -ItemType Directory -Force -Path $licenseDir | Out-Null
    $dest = Join-Path $licenseDir 'payment_unlock_v2_2_4_beta.json'
    Copy-Item -Force $source $dest
    Write-Host "Installed payment proof bundle: $dest"
}

Ensure-WingetPackage -PackageId 'OpenJS.NodeJS.LTS' -CommandName 'node'
Ensure-WingetPackage -PackageId 'Kitware.CMake' -CommandName 'cmake'

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

Write-Host 'Installing npm dependencies'
npm install

Write-Host 'Configuring CMake build'
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

$buildTargets = @('aifred_vst3_A','aifred_vst3_B','aifred_vst3_C','aifred_vst3_D')
Write-Host "Building Release binaries: $($buildTargets -join ', ')"
cmake --build build --config Release --target $buildTargets

$standaloneInstallDir = Join-Path $env:LOCALAPPDATA 'Programs\DawAI\bin'
$defaultFlVstFolder = Join-Path $env:USERPROFILE 'Documents\Image-Line\FL Studio\Plugins\VST'

New-Item -ItemType Directory -Force -Path $standaloneInstallDir | Out-Null
New-Item -ItemType Directory -Force -Path $defaultFlVstFolder | Out-Null

if ($Target -eq 'all' -or $Target -eq 'vst') {
    foreach ($suffix in $Variants) {
        $upper = $suffix.ToUpperInvariant()
        $vstSource = Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\VST3\AIFRED_${suffix}.vst3"
        if (-not (Test-Path $vstSource)) {
            Write-Warning "Missing VST variant artifact: $vstSource"
            continue
        }

        $vstDest = Join-Path $defaultFlVstFolder "AIFRED_v${VersionTag}${suffix}.vst3"
        if (Test-Path $vstDest) {
            Remove-Item -Recurse -Force $vstDest
        }
        Copy-Item -Recurse -Force $vstSource $vstDest
        Write-Host "Installed VST variant ${suffix}: $vstDest"
    }

    Write-Host "VST variants installed to: $defaultFlVstFolder"
}

if ($Target -eq 'all' -or $Target -eq 'standalone') {
    foreach ($suffix in $Variants) {
        $upper = $suffix.ToUpperInvariant()
        $source = Resolve-FirstExisting @(
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Standalone\AIFRED_${suffix}.exe"),
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Standalone\AIFRED_${suffix}")
        )

        if (-not $source) {
            Write-Warning "Missing standalone variant artifact for ${suffix}"
            continue
        }

        $dest = Join-Path $standaloneInstallDir "aifred_standalone_v${VersionTag}${suffix}.exe"
        Copy-Item -Force $source $dest
        Write-Host "Installed standalone variant ${suffix}: $dest"
    }

    Write-Host "Standalone variants installed to: $standaloneInstallDir"
}

Install-PaymentUnlockProof -RootPath $root

Write-Host "Windows install script complete (target=$Target)."
