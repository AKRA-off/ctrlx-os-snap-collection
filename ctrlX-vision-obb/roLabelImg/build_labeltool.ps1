# =====================================================================
# build_labeltool.ps1 - build the patched roLabelImg annotation tool
# into a single PORTABLE Windows .exe (no install, no Python needed on
# the target machine).
#
# Usage (from the roLabelImg folder, in a shell WITH internet):
#     powershell -ExecutionPolicy Bypass -File .\build_labeltool.ps1
#
# Optional: point it at a specific Python that has PyQt5 + lxml:
#     .\build_labeltool.ps1 -Python "C:\path\to\python.exe"
#
# Output: .\dist\roLabelImg.exe   (copy that single file anywhere)
# =====================================================================
param(
    [string]$Python = "C:\Users\kra7spb\Documents\flask_app\venv-win3\Scripts\python.exe"
)

# Note: keep on "Continue" - native tools (python/pip) write progress and
# expected import failures to stderr; we gate on $LASTEXITCODE ourselves.
$ErrorActionPreference = "Continue"
Set-Location -Path $PSScriptRoot

Write-Host "== Using Python: $Python" -ForegroundColor Cyan
& $Python --version

# 1) Make sure PyInstaller is available (needs internet the first time)
& $Python -c "import PyInstaller" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "== PyInstaller not found - installing (needs internet)..." -ForegroundColor Cyan
    & $Python -m pip install --upgrade pyinstaller
    if ($LASTEXITCODE -ne 0) {
        Write-Host "== pip install pyinstaller FAILED (no internet / proxy?)." -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "== PyInstaller already installed." -ForegroundColor Green
}

# 2) Sanity-check the app's own dependencies (only PyQt5 is required now -
#    XML uses the stdlib ElementTree, so lxml is intentionally NOT bundled)
& $Python -c "import PyQt5; print('deps OK: PyQt5')"

# 3) Generate a Windows icon (app.ico) from the modern app PNG, via Qt
#    (no Pillow required - Qt ships an ICO writer)
& $Python -c @"
from PyQt5.QtGui import QImage
img = QImage('icons_modern/app.png')
if img.isNull():
    print('WARN: icons_modern/app.png not found - building without a custom icon')
else:
    img.scaled(256, 256).save('app.ico')
    print('wrote app.ico')
"@

$iconArg = @()
if (Test-Path ".\app.ico") { $iconArg = @("--icon", "app.ico") }

# 4) Clean previous build
Remove-Item -Recurse -Force .\build, .\dist, .\roLabelImg.spec -ErrorAction SilentlyContinue

# 5) Build. --onefile = one portable .exe, --windowed = no console window.
#    --paths libs lets PyInstaller find the runtime-added internal modules.
#    --add-data "SRC;DEST" (Windows uses ';') bundles the on-disk assets.
& $Python -m PyInstaller --noconfirm --clean --onefile --windowed `
    --name roLabelImg `
    --paths libs `
    --add-data "icons_modern;icons_modern" `
    --add-data "data;data" `
    --add-data "icons;icons" `
    --exclude-module lxml `
    --exclude-module torch --exclude-module torchvision `
    --exclude-module cv2 --exclude-module numpy --exclude-module scipy `
    --exclude-module pandas --exclude-module matplotlib `
    --exclude-module ultralytics --exclude-module onnxruntime `
    --exclude-module sklearn --exclude-module tkinter --exclude-module PIL `
    @iconArg `
    roLabelImg.py

if ($LASTEXITCODE -eq 0 -and (Test-Path ".\dist\roLabelImg.exe")) {
    $mb = "{0:N1}" -f ((Get-Item ".\dist\roLabelImg.exe").Length / 1MB)
    Write-Host ""
    Write-Host "== DONE: dist\roLabelImg.exe  ($mb MB)" -ForegroundColor Green
    Write-Host "   Copy that single file to the other machine and double-click it."
} else {
    Write-Host "== BUILD FAILED - see the PyInstaller output above." -ForegroundColor Red
    exit 1
}
