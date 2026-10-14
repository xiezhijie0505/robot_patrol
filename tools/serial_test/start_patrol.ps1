# 开始 / 停止矩形自动巡逻
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [ValidateSet("START", "STOP")][string]$Action = "START",
    [int]$Baud = 115200
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& "$scriptDir\send_cmd.ps1" -Port $Port -Baud $Baud -Cmd "PATROL $Action"
