param([string]$RuntimeDirectory = (Join-Path $PSScriptRoot '..\cmake-build'))
$ErrorActionPreference = 'Stop'
$RuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $RuntimeDirectory 'diagnostics\latest-input-recording.json') -Raw | ConvertFrom-Json
if (Get-Process SSSVRecompiled -ErrorAction SilentlyContinue) { throw 'Close the running game before replaying.' }
$runDirectory = Join-Path $RuntimeDirectory ('diagnostics\replay-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path (Join-Path $runDirectory 'saves-before-replay') | Out-Null
Get-ChildItem -LiteralPath (Join-Path $RuntimeDirectory 'saves') -File | Copy-Item -Destination (Join-Path $runDirectory 'saves-before-replay')
Get-ChildItem -LiteralPath $manifest.starting_saves -File | Copy-Item -Destination (Join-Path $RuntimeDirectory 'saves')
$oldRecord = $env:SSSV_INPUT_RECORD
$oldReplay = $env:SSSV_INPUT_REPLAY
try {
    $env:SSSV_INPUT_RECORD = $null
    $env:SSSV_INPUT_REPLAY = $manifest.recording
    $game = Start-Process -FilePath (Join-Path $RuntimeDirectory 'SSSVRecompiled.exe') -WorkingDirectory $RuntimeDirectory -WindowStyle Normal -PassThru
    Write-Output "Replaying recorded inputs; process $($game.Id); results $runDirectory"
    $game.WaitForExit()
} finally {
    $env:SSSV_INPUT_RECORD = $oldRecord
    $env:SSSV_INPUT_REPLAY = $oldReplay
}
foreach ($name in @('frame-stalls.log', 'render-timings.log', 'full-sync-timings.log', 'missing-sync-dl.txt')) {
    $source = Join-Path $RuntimeDirectory $name
    if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $runDirectory }
}
Write-Output "Replay finished; results $runDirectory"
