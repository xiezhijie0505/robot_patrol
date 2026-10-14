# 简单动作序列：前进 → 左转 → 停车（每步带心跳）
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [int]$Baud = 115200,
    [int]$Speed = 40
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
function Send([string]$cmd) {
    & "$scriptDir\send_cmd.ps1" -Port $Port -Baud $Baud -Cmd $cmd
}

Write-Host "=== demo_move on $Port ==="
Send "STOP"
Start-Sleep -Milliseconds 300
Send "FWD $Speed"
1..8 | ForEach-Object {
    Start-Sleep -Milliseconds 400
    Send "HB"
}
Send "TURN 40"
1..4 | ForEach-Object {
    Start-Sleep -Milliseconds 400
    Send "HB"
}
Send "STOP"
Write-Host "=== done ==="
