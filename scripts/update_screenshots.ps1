# Regenerate docs/screenshots using the latest build.
# Usage: powershell -ExecutionPolicy Bypass -File scripts/update_screenshots.ps1
$ErrorActionPreference = "Stop"

$root    = Split-Path -Parent $PSScriptRoot
$exe     = Join-Path $root "bin\Release\perception.exe"
$shots   = Join-Path $root "docs\screenshots"
$themesDir = Join-Path $shots "themes"

if (-not (Test-Path $exe)) { throw "Executable not found: $exe (run scripts/build.ps1 -Gui first)" }
if (-not (Test-Path $themesDir)) { New-Item -ItemType Directory -Path $themesDir | Out-Null }

function Invoke-Snap {
    param([string[]]$ArgList)
    Write-Host "==> $($ArgList -join ' ')"
    & $exe @ArgList
    if ($LASTEXITCODE -ne 0) { throw "snapshot failed: $($ArgList -join ' ')" }
}

# ---- 1. Main window (default dark-classic theme) ----
Invoke-Snap @("--snapshot", (Join-Path $shots "main.png"))

# ---- 2. Python console REPL (version query) ----
$verScript = "import sys`nsys.version"
Invoke-Snap @("--snapshot", (Join-Path $shots "python-console.png"), "--console-script", $verScript)

# ---- 3. Console echo / execution demos ----
Invoke-Snap @("--snapshot", (Join-Path $shots "console-echo.png"), "--console-script", "print('hello')")
Invoke-Snap @("--snapshot", (Join-Path $shots "console-echo2.png"), "--console-script", "2 + 2")
Invoke-Snap @("--snapshot", (Join-Path $shots "console-echo3.png"), "--console-script", "for i in range(3): print(i * 10)")
Invoke-Snap @("--snapshot", (Join-Path $shots "console-echo4.png"), "--console-script", "print(1 / 0)")

$testScript = "import math`nprint(math.pi)`nx = 42`nprint(f'x = {x}')`ndef greet(name):`n    return f'hi, {name}'`n`nprint(greet('Perception'))"
Invoke-Snap @("--snapshot", (Join-Path $shots "console-test.png"), "--console-script", $testScript)

# ---- 4. Dock floating / restored (one run, two images) ----
Invoke-Snap @("--snapshot-float", (Join-Path $shots "dock-floating.png"),
              "--snapshot-restore", (Join-Path $shots "dock-restored.png"))

# ---- 5. UI polish series (light theme) ----
Invoke-Snap @("--theme", "light-classic", "--snapshot", (Join-Path $shots "ui-opt-main.png"))
Invoke-Snap @("--theme", "light-classic", "--snapshot", (Join-Path $shots "ui-opt-main2.png"), "--console-script", "")
Invoke-Snap @("--theme", "light-classic", "--snapshot-float", (Join-Path $shots "ui-opt-float.png"))
Invoke-Snap @("--theme", "light-classic", "--snapshot", (Join-Path $shots "ui-opt-input.png"), "--console-script", "print('Hello, Perception')")
Invoke-Snap @("--theme", "light-classic", "--snapshot", (Join-Path $shots "ui-opt-tb.png"))

# ---- 6. Theme gallery (25 themes, same order as the Theme menu) ----
$themes = @(
    "dark-classic", "dark-blue", "nord", "one-dark", "dracula", "monokai",
    "gruvbox-dark", "solarized-dark", "tokyo-night", "rose-pine",
    "catppuccin-mocha", "everforest-dark", "kanagawa", "night-owl", "ayu-dark",
    "light-classic", "light-blue", "material-light", "solarized-light",
    "rose-pine-dawn", "catppuccin-latte", "github-light",
    "hc-black", "hc-white", "hc-blue"
)
foreach ($t in $themes) {
    Invoke-Snap @("--theme", $t, "--snapshot", (Join-Path $themesDir "$t.png"))
}

Write-Host ""
Write-Host "All screenshots written to $shots"
