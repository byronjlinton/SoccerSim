#Requires -Version 5.1
<#
.SYNOPSIS
  Ensures SoccerSim default game mode is set in config so Play uses SoccerGameMode on any level.
.DESCRIPTION
  Adds or updates [/Script/EngineSettings.GameMapsSettings] and GlobalDefaultGameMode
  in DefaultEngine.ini and DefaultGame.ini. Safe to run multiple times.
#>

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ConfigPath = Join-Path $ProjectRoot 'Config'
$EngineIni = Join-Path $ConfigPath 'DefaultEngine.ini'
$GameIni = Join-Path $ConfigPath 'DefaultGame.ini'

$Section = '[/Script/EngineSettings.GameMapsSettings]'
$Key = 'GlobalDefaultGameMode=/Script/SoccerSim.SoccerGameMode'

function Ensure-IniSectionAndKey {
    param([string]$Path, [string]$SectionName, [string]$KeyLine)
    if (-not (Test-Path -LiteralPath $Path)) { return }
    $content = Get-Content -LiteralPath $Path -Raw
    $lines = [System.Collections.ArrayList]::new([string[]]$content.Split("`n"))
    $insertIndex = -1
    $inSection = $false
    $keyFound = $false
    $modified = $false
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $trimmed = $line.Trim()
        if ($trimmed -eq $SectionName) {
            $inSection = $true
            $insertIndex = $i + 1
            continue
        }
        if ($inSection) {
            if ($trimmed -match '^\[') { break }
            if ($trimmed -match '^GlobalDefaultGameMode=') {
                $keyFound = $true
                if ($trimmed -ne $KeyLine) { $lines[$i] = $KeyLine; $modified = $true }
                break
            }
        }
    }
    if (-not $keyFound) {
        if ($inSection -and $insertIndex -ge 0) {
            $lines.Insert($insertIndex, $KeyLine) | Out-Null
        } else {
            $idx = 0
            for ($i = 0; $i -lt $lines.Count; $i++) {
                if ($lines[$i].Trim() -eq '[/Script/Engine.Engine]') { $idx = $i + 1; break }
            }
            $lines.Insert($idx, '')
            $lines.Insert($idx + 1, $SectionName)
            $lines.Insert($idx + 2, $KeyLine)
        }
        $modified = $true
    }
    if ($modified) {
        $lines -join "`n" | Set-Content -LiteralPath $Path -NoNewline
    }
}

Ensure-IniSectionAndKey -Path $EngineIni -SectionName $Section -KeyLine $Key
Ensure-IniSectionAndKey -Path $GameIni -SectionName $Section -KeyLine $Key

Write-Host 'Default game mode set to SoccerGameMode. Restart Unreal Editor if it is open, then press Play on any level (including an empty one).'
