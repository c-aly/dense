# Install the dense CLI from GitHub Releases (Windows).
#
#   irm https://raw.githubusercontent.com/c-aly/dense/main/scripts/install.ps1 | iex
#
# Env / variables:
#   $env:DENSE_VERSION  tag without leading v, or "latest" (default)
#   $env:DENSE_REPO     owner/name (default c-aly/dense)
#   $env:DENSE_INSTALL_DIR  install folder (default %LOCALAPPDATA%\Dense\bin)
$ErrorActionPreference = "Stop"

$Repo = if ($env:DENSE_REPO) { $env:DENSE_REPO } else { "c-aly/dense" }
$Version = if ($env:DENSE_VERSION) { $env:DENSE_VERSION } else { "latest" }
$DestDir = if ($env:DENSE_INSTALL_DIR) { $env:DENSE_INSTALL_DIR } else {
    Join-Path $env:LOCALAPPDATA "Dense\bin"
}

$Asset = "dense-windows-amd64.zip"
if ($Version -eq "latest") {
    $Url = "https://github.com/$Repo/releases/latest/download/$Asset"
} else {
    $Ver = $Version.TrimStart("v")
    $Url = "https://github.com/$Repo/releases/download/v$Ver/$Asset"
}

Write-Host "dense: downloading $Asset ($Version)…"
$Tmp = Join-Path $env:TEMP ("dense-install-" + [guid]::NewGuid().ToString())
New-Item -ItemType Directory -Path $Tmp | Out-Null
try {
    $Zip = Join-Path $Tmp $Asset
    Invoke-WebRequest -Uri $Url -OutFile $Zip -UseBasicParsing

    Expand-Archive -Path $Zip -DestinationPath $Tmp -Force
    $Bin = Get-ChildItem -Path $Tmp -Recurse -Filter "dense.exe" | Select-Object -First 1
    if (-not $Bin) { throw "archive had no dense.exe" }

    New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
    Copy-Item -Force $Bin.FullName (Join-Path $DestDir "dense.exe")
    Write-Host "dense: installed → $(Join-Path $DestDir 'dense.exe')"

    # Add DestDir to the user PATH if missing.
    $UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $Parts = @()
    if ($UserPath) { $Parts = $UserPath -split ";" | Where-Object { $_ -ne "" } }
    if ($Parts -notcontains $DestDir) {
        $NewPath = ($Parts + $DestDir) -join ";"
        [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
        $env:Path = "$DestDir;$env:Path"
        Write-Host "dense: added $DestDir to your user PATH (new terminals pick this up)."
    }

    Write-Host "dense: try: dense --help"
} finally {
    Remove-Item -Recurse -Force $Tmp -ErrorAction SilentlyContinue
}
