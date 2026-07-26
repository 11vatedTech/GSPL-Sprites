param(
    [string]$QtPrefix = $env:QT_PREFIX,
    [string]$ExpectedArch = "msvc2022_64"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($QtPrefix)) {
    Write-Error "Set QT_PREFIX or pass -QtPrefix to the Qt installation prefix, for example C:\Qt\6.8.3\msvc2022_64."
}

$qmake = Join-Path $QtPrefix "bin/qmake.exe"
if (-not (Test-Path $qmake)) {
    Write-Error "qmake.exe was not found under '$QtPrefix'."
}

$version = & $qmake -query QT_VERSION
$installPrefix = & $qmake -query QT_INSTALL_PREFIX
$xspec = & $qmake -query QMAKE_XSPEC

if ($installPrefix -notlike "*$ExpectedArch*") {
    Write-Error "Qt prefix '$installPrefix' does not match expected architecture '$ExpectedArch'."
}

if ($xspec -notmatch "msvc") {
    Write-Error "Qt xspec '$xspec' is not an MSVC build. Use an MSVC Qt package for the MSVC Studio build."
}

$requiredTools = @("qmllint.exe", "qmlformat.exe", "lrelease.exe", "windeployqt.exe")
foreach ($tool in $requiredTools) {
    $path = Join-Path $QtPrefix "bin/$tool"
    if (-not (Test-Path $path)) {
        Write-Error "Required Qt tool missing: $path"
    }
}

Write-Host "GSPL Studio Qt environment verified."
Write-Host "Qt version: $version"
Write-Host "Qt prefix: $installPrefix"
Write-Host "Qt xspec: $xspec"
