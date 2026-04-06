# Restart-SoccerSim.ps1
# Closes any running Unreal Editor, then opens SoccerSim.uproject.
# Use this when you changed Config (e.g. DefaultGame.ini) so the editor loads fresh config.
# WARNING: This force-closes the editor. Save your work before running, or close the editor yourself first.

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$UprojectPath = Join-Path $ProjectRoot "SoccerSim.uproject"
if (-not (Test-Path -LiteralPath $UprojectPath)) {
    $ProjectRoot = "C:\Users\byron\Cursor\SoccerSim"
    $UprojectPath = Join-Path $ProjectRoot "SoccerSim.uproject"
}
$resolved = Resolve-Path -LiteralPath $UprojectPath -ErrorAction SilentlyContinue
if ($resolved) { $UprojectPath = $resolved.Path }

Write-Host "SoccerSim restart: closing Unreal Editor (if running), then opening project."
$processes = Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue
if ($processes) {
    $processes | Stop-Process -Force
    Write-Host "Closed Unreal Editor. Waiting 3 seconds..."
    Start-Sleep -Seconds 3
} else {
    Write-Host "No Unreal Editor process found. Opening project..."
}

Start-Process $UprojectPath
Write-Host "Launched: $UprojectPath"
Write-Host "After the editor opens, press Play. Config (e.g. DefaultMetaHumanMeshPath) is loaded from disk on startup."
