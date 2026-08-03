param(
    [string]$PackageDirectory = "out/packages/package-msvc-release",
    [string]$MesaDirectory = "Mesa/x64"
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$PackageDirectory = [IO.Path]::GetFullPath((Join-Path $RepoRoot $PackageDirectory))
$MesaDirectory = [IO.Path]::GetFullPath((Join-Path $RepoRoot $MesaDirectory))
$WorkDirectory = Join-Path $RepoRoot 'out/package-verification/windows'
$ConsumerSource = Join-Path $RepoRoot 'test/TestInstall/geoqik_install_test_template'

Remove-Item $WorkDirectory -Recurse -Force -ErrorAction SilentlyContinue
New-Item $WorkDirectory -ItemType Directory | Out-Null

$Archives = @(Get-ChildItem $PackageDirectory -Filter '*.zip')
$Installers = @(Get-ChildItem $PackageDirectory -Filter '*.exe')
if ($Archives.Count -ne 1 -or $Installers.Count -ne 1) {
    throw "Expected one ZIP and one NSIS installer in $PackageDirectory"
}

function Test-PackageChecksum {
    param([IO.FileInfo]$Package)

    $ChecksumPaths = @("$($Package.FullName).sha256", "$($Package.FullName).SHA256")
    $ChecksumPath = $ChecksumPaths | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $ChecksumPath) {
        throw "Missing SHA-256 checksum for $($Package.Name)"
    }

    $ExpectedHash = (Get-FileHash $Package.FullName -Algorithm SHA256).Hash
    $ChecksumContent = Get-Content $ChecksumPath -Raw
    if ($ChecksumContent -notmatch $ExpectedHash) {
        throw "Invalid SHA-256 checksum for $($Package.Name)"
    }
}

Test-PackageChecksum $Archives[0]
Test-PackageChecksum $Installers[0]

function Test-PackagePrefix {
    param(
        [string]$Prefix,
        [string]$Name
    )

    $ExpectedPaths = @(
        'include/GeoQik/GeoQik.hpp',
        'include/GeoQikClient/GeoQikClient.hpp',
        'lib/cmake/geoqik/geoqikConfig.cmake',
        'lib/geoqik.lib',
        'bin/geoqik.dll',
        'bin/geoqik_server.exe',
        'share/doc/geoqik/License'
    )
    foreach ($RelativePath in $ExpectedPaths) {
        if (-not (Test-Path (Join-Path $Prefix $RelativePath))) {
            throw "$Name package is missing $RelativePath"
        }
    }

    $BuildDirectory = Join-Path $WorkDirectory "consumer-$Name"
    cmake -S $ConsumerSource -B $BuildDirectory `
        '-DCMAKE_BUILD_TYPE=Release' `
        "-DCMAKE_PREFIX_PATH=$Prefix"
    if ($LASTEXITCODE -ne 0) { throw "Could not configure the $Name package consumer" }

    cmake --build $BuildDirectory --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Could not build the $Name package consumer" }

    $DirectExecutable = Get-ChildItem $BuildDirectory -Filter 'use_installdir.exe' -Recurse | Select-Object -First 1
    $ClientExecutable = Get-ChildItem $BuildDirectory -Filter 'use_installdir_client.exe' -Recurse | Select-Object -First 1
    if (-not $DirectExecutable -or -not $ClientExecutable) {
        throw "Could not find the $Name package consumer executables"
    }

    $RuntimeDirectories = @(
        $DirectExecutable.DirectoryName,
        $ClientExecutable.DirectoryName,
        (Join-Path $Prefix 'bin')
    ) | Select-Object -Unique
    foreach ($RuntimeDirectory in $RuntimeDirectories) {
        Copy-Item (Join-Path $MesaDirectory '*.dll') $RuntimeDirectory -Force
    }

    $OriginalPath = $env:PATH
    try {
        $env:PATH = "$(Join-Path $Prefix 'bin');$MesaDirectory;$OriginalPath"
        & $DirectExecutable.FullName
        if ($LASTEXITCODE -ne 0) { throw "$Name direct-library smoke test failed" }
        & $ClientExecutable.FullName
        if ($LASTEXITCODE -ne 0) { throw "$Name client/server smoke test failed" }
    }
    finally {
        $env:PATH = $OriginalPath
    }
}

$ArchiveDirectory = Join-Path $WorkDirectory 'archive'
Expand-Archive $Archives[0].FullName -DestinationPath $ArchiveDirectory
$ArchiveRoots = @(Get-ChildItem $ArchiveDirectory -Directory)
if ($ArchiveRoots.Count -ne 1) {
    throw 'Expected the ZIP to contain one package root'
}
Test-PackagePrefix -Prefix $ArchiveRoots[0].FullName -Name 'archive'

$InstallDirectory = Join-Path $WorkDirectory 'nsis-install'
$InstallResult = Start-Process $Installers[0].FullName -ArgumentList '/S', "/D=$InstallDirectory" -Wait -PassThru
if ($InstallResult.ExitCode -ne 0) {
    throw "NSIS installation failed with exit code $($InstallResult.ExitCode)"
}
Test-PackagePrefix -Prefix $InstallDirectory -Name 'nsis'

$Uninstaller = Join-Path $InstallDirectory 'Uninstall.exe'
if (-not (Test-Path $Uninstaller)) {
    throw 'NSIS package did not install an uninstaller'
}
$UninstallResult = Start-Process $Uninstaller -ArgumentList '/S' -Wait -PassThru
if ($UninstallResult.ExitCode -ne 0) {
    throw "NSIS uninstall failed with exit code $($UninstallResult.ExitCode)"
}

Write-Host "Verified ZIP and NSIS packages in $PackageDirectory"
