$ErrorActionPreference = 'Stop'

param(
    [ValidateSet('all','vst','standalone')]
    [string]$InstallTarget = 'all',
    [string]$PackageTitle = 'DawAI Canonical 2.2.4 Beta Installer',
    [string]$OutputExe = (Join-Path (Split-Path -Parent $PSScriptRoot) 'dist\DawAI-Canonical-224-Installer.exe')
)

$VersionTag = '2.2.4'
$Variants = @('a','b','c','d')
$root = Split-Path -Parent $PSScriptRoot
$staging = Join-Path $env:TEMP "dawai_one_click_installer_$InstallTarget"
$payloadRoot = Join-Path $staging 'payload_tree'
$payloadZip = Join-Path $staging 'payload.zip'
$iexpress = Join-Path $env:WINDIR 'System32\iexpress.exe'
$paymentUnlockSource = Join-Path $root 'assets\licensing\payment_unlock_v2_2_4_beta.json'

function Resolve-FirstExisting {
    param([string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Copy-DirectoryPayload {
    param(
        [string]$Source,
        [string]$Destination
    )

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    if (Test-Path $Destination) {
        Remove-Item -Recurse -Force $Destination
    }
    Copy-Item -Recurse -Force $Source $Destination
}

function Copy-FilePayload {
    param(
        [string]$Source,
        [string]$Destination
    )

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -Force $Source $Destination
}

Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $staging | Out-Null
New-Item -ItemType Directory -Force -Path $payloadRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputExe) | Out-Null

if ($InstallTarget -eq 'all' -or $InstallTarget -eq 'vst') {
    foreach ($suffix in $Variants) {
        $upper = $suffix.ToUpperInvariant()
        $source = Resolve-FirstExisting @(
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Release\VST3\AIFRED_${suffix}.vst3"),
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\VST3\AIFRED_${suffix}.vst3")
        )

        if (-not $source) {
            throw "Missing VST artifact for variant ${suffix}"
        }

        Copy-DirectoryPayload -Source $source -Destination (Join-Path $payloadRoot "vst\AIFRED_${suffix}.vst3")
    }
}

if ($InstallTarget -eq 'all' -or $InstallTarget -eq 'standalone') {
    foreach ($suffix in $Variants) {
        $upper = $suffix.ToUpperInvariant()
        $source = Resolve-FirstExisting @(
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Release\Standalone\AIFRED_${suffix}.exe"),
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Release\Standalone\AIFRED_${suffix}"),
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Standalone\AIFRED_${suffix}.exe"),
            (Join-Path $root "build\Source\aifred_vst3_${upper}_artefacts\Standalone\AIFRED_${suffix}")
        )

        if (-not $source) {
            throw "Missing standalone artifact for variant ${suffix}"
        }

        Copy-FilePayload -Source $source -Destination (Join-Path $payloadRoot "standalone\aifred_standalone_v${VersionTag}${suffix}.exe")
    }
}

if (Test-Path $paymentUnlockSource) {
    Copy-FilePayload -Source $paymentUnlockSource -Destination (Join-Path $payloadRoot 'license\payment_unlock_v2_2_4_beta.json')
}

if (Test-Path $payloadZip) {
    Remove-Item -Force $payloadZip
}
Compress-Archive -Path (Join-Path $payloadRoot '*') -DestinationPath $payloadZip -CompressionLevel Optimal

$installCmd = Join-Path $staging 'install_payload.cmd'
$cmdLines = @(
    '@echo off',
    'setlocal enabledelayedexpansion',
    'set "PAYLOAD_DIR=%TEMP%\AIFR3D_2_2_4_Beta_Payload"',
    'if exist "%PAYLOAD_DIR%" rmdir /s /q "%PAYLOAD_DIR%"',
    'powershell -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -Force ''%~dp0payload.zip'' ''%PAYLOAD_DIR%''"',
    'if errorlevel 1 exit /b 1'
)

if ($InstallTarget -eq 'all' -or $InstallTarget -eq 'vst') {
    $cmdLines += @(
        'set "VST_DIR=%USERPROFILE%\Documents\Image-Line\FL Studio\Plugins\VST"',
        'if not exist "%VST_DIR%" mkdir "%VST_DIR%"'
    )

    foreach ($suffix in $Variants) {
        $cmdLines += @(
            "if exist ""%VST_DIR%\AIFRED_v${VersionTag}${suffix}.vst3"" rmdir /s /q ""%VST_DIR%\AIFRED_v${VersionTag}${suffix}.vst3""",
            ('xcopy /E /I /Y "{0}" "{1}"' -f "%PAYLOAD_DIR%\vst\AIFRED_${suffix}.vst3",
                                            "%VST_DIR%\AIFRED_v${VersionTag}${suffix}.vst3\")
        )
    }
}

if ($InstallTarget -eq 'all' -or $InstallTarget -eq 'standalone') {
    $cmdLines += @(
        'set "STANDALONE_DIR=%LOCALAPPDATA%\Programs\DawAI\bin"',
        'if not exist "%STANDALONE_DIR%" mkdir "%STANDALONE_DIR%"'
    )

    foreach ($suffix in $Variants) {
        $cmdLines += ('copy /Y "{0}" "{1}"' -f "%PAYLOAD_DIR%\standalone\aifred_standalone_v${VersionTag}${suffix}.exe",
                                              "%STANDALONE_DIR%\aifred_standalone_v${VersionTag}${suffix}.exe")
    }
}

$cmdLines += @(
    'set "LICENSE_DIR=%APPDATA%\DawAI\license"',
    'if not exist "%LICENSE_DIR%" mkdir "%LICENSE_DIR%"'
)

if (Test-Path (Join-Path $payloadRoot 'license\payment_unlock_v2_2_4_beta.json')) {
    $cmdLines += 'copy /Y "%PAYLOAD_DIR%\license\payment_unlock_v2_2_4_beta.json" "%LICENSE_DIR%\payment_unlock_v2_2_4_beta.json"'
}

$cmdLines += @(
    'echo DawAI 2.2.4 Beta install complete.',
    'endlocal'
)

Set-Content -Path $installCmd -Value ($cmdLines -join "`r`n") -Encoding ASCII

$sedPath = Join-Path $staging 'dawai_installer.sed'
$prompt = 'This package installs the prebuilt DawAI 2.2.4 Beta Windows payload.'
$sed = @"
[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=0
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=$prompt
DisplayLicense=
FinishMessage=DawAI install finished.
TargetName=$OutputExe
FriendlyName=$PackageTitle
AppLaunched=cmd /c install_payload.cmd
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
SourceFiles=SourceFiles
[SourceFiles]
SourceFiles0=$staging
[SourceFiles0]
%FILE0%=
%FILE1%=
[Strings]
FILE0=install_payload.cmd
FILE1=payload.zip
"@
Set-Content -Path $sedPath -Value $sed -Encoding ASCII

if (-not (Test-Path $iexpress)) {
    throw "IExpress not found at $iexpress"
}

& $iexpress /N $sedPath
Write-Host "Built $OutputExe"
