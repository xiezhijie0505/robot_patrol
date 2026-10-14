# 向 STM32 发一行底盘指令（自动补 \n）
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [Parameter(Mandatory = $true)][string]$Cmd,
    [int]$Baud = 115200
)

$serial = New-Object System.IO.Ports.SerialPort $Port, $Baud, None, 8, One
$serial.NewLine = "`n"
$serial.ReadTimeout = 500
$serial.WriteTimeout = 500
try {
    $serial.Open()
    $line = $Cmd.TrimEnd("`r", "`n")
    $serial.WriteLine($line)
    Start-Sleep -Milliseconds 50
    Write-Host "sent: $line"
    try {
        $resp = $serial.ReadExisting()
        if ($resp) { Write-Host "recv: $resp" }
    } catch {}
}
finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
