param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [string]$Version = "0.1.0",
    [string]$OutputDir = "release"
)

$ErrorActionPreference = "Stop"

# Build paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RepoRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RepoRoot $BuildDir
$OutputDir = Join-Path $RepoRoot $OutputDir

# Binaries to package
$Binaries = @(
    "gsplc.exe",
    "gspl-sprites.exe",
    "gspl_sprites_plugin_sandbox.exe"
)

# Optional: Qt6 DLLs for Studio
$QtDlls = @()

Write-Host "=== GSPL Sprites Release Packaging ==="
Write-Host "Version:  $Version"
Write-Host "Config:   $Config"
Write-Host "Build:    $BuildDir"
Write-Host "Output:   $OutputDir"
Write-Host ""

# Create output structure
$PackageDir = Join-Path $OutputDir "gspl-sprites-$Version-win64"
if (Test-Path $PackageDir) { Remove-Item -Recurse -Force $PackageDir }
New-Item -ItemType Directory -Path "$PackageDir\bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$PackageDir\include" -Force | Out-Null
New-Item -ItemType Directory -Path "$PackageDir\examples" -Force | Out-Null
New-Item -ItemType Directory -Path "$PackageDir\docs" -Force | Out-Null

# Copy binaries
$BinaryDir = Join-Path $BuildDir $Config
Write-Host "Copying binaries from $BinaryDir..."
foreach ($Bin in $Binaries) {
    $Src = Join-Path $BinaryDir $Bin
    if (Test-Path $Src) {
        Copy-Item $Src "$PackageDir\bin\" -Verbose
    } else {
        Write-Warning "Binary not found: $Bin"
    }
}

# Copy DLL dependencies (MSVC runtime, etc.)
$VcRedistDir = "$env:VCToolsRedistDir\vc14\redist\x64\Microsoft.VC*.CRT"
Get-ChildItem "$BinaryDir\*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item $_.FullName "$PackageDir\bin\" -Verbose
}

# Copy public headers
Write-Host "Copying public headers..."
Get-ChildItem (Join-Path $RepoRoot "include\gspl") -Recurse -Filter "*.hpp" | ForEach-Object {
    $RelPath = $_.FullName.Substring((Join-Path $RepoRoot "include").Length + 1)
    $Dest = Join-Path $PackageDir "include" $RelPath
    $Parent = Split-Path $Dest -Parent
    if (-not (Test-Path $Parent)) { New-Item -ItemType Directory -Path $Parent -Force | Out-Null }
    Copy-Item $_.FullName $Dest -Verbose
}

# Copy example workspaces
Write-Host "Copying examples..."
$ExamplesDir = Join-Path $RepoRoot "examples"
Get-ChildItem $ExamplesDir -Directory | ForEach-Object {
    $Dest = Join-Path $PackageDir "examples" $_.Name
    Copy-Item $_.FullName $Dest -Recurse -Verbose
}

# Copy documentation
Write-Host "Copying docs..."
if (Test-Path (Join-Path $RepoRoot "docs")) {
    Copy-Item (Join-Path $RepoRoot "docs\*") "$PackageDir\docs\" -Recurse -Verbose
}

# Copy license
$LicenseFiles = @("LICENSE", "LICENSE.txt", "LICENSE.md")
foreach ($Lf in $LicenseFiles) {
    $LfPath = Join-Path $RepoRoot $Lf
    if (Test-Path $LfPath) { Copy-Item $LfPath "$PackageDir\" -Verbose; break }
}

# Create README
@"
GSPL Sprites $Version
=====================
GSPL Sprites Compiler, Runtime, and Authoring Tools

See docs/ for documentation.
"@ | Out-File -FilePath "$PackageDir\README.txt" -Encoding ascii

# Create NSIS installer script
$NsisScript = @"
!define PRODUCT_NAME "GSPL Sprites"
!define PRODUCT_VERSION "$Version"
!define PRODUCT_DIR "gspl-sprites-$Version-win64"

SetCompressor lzma
Name "\${PRODUCT_NAME} \${PRODUCT_VERSION}"
OutFile "gspl-sprites-\${PRODUCT_VERSION}-setup.exe"
InstallDir "\$PROGRAMFILES64\\\${PRODUCT_NAME}"

Section "Install"
    SetOutPath \$INSTDIR
    File /r "\${PRODUCT_DIR}\\*.*"
    CreateShortCut "\$DESKTOP\\\${PRODUCT_NAME}.lnk" "\$INSTDIR\\bin\\gsplc.exe"
    WriteUninstaller "\$INSTDIR\\uninstall.exe"
SectionEnd

Section "Uninstall"
    RMDir /r \$INSTDIR
SectionEnd
"@

$NsisPath = Join-Path $OutputDir "gspl-sprites-$Version.nsi"
$NsisScript | Out-File -FilePath $NsisPath -Encoding ascii
Write-Host "NSIS script written to $NsisPath"

# Create zip archive
Write-Host "Creating zip archive..."
$ZipPath = Join-Path $OutputDir "gspl-sprites-$Version-win64.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($PackageDir, $ZipPath)

Write-Host ""
Write-Host "=== Packaging Complete ==="
Write-Host "  Package dir: $PackageDir"
Write-Host "  ZIP archive: $ZipPath"
Write-Host "  NSIS script: $NsisPath"
