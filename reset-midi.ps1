# List all processes using USB MIDI devices
Get-PnpDevice -FriendlyName "*MIDI*" | ForEach-Object {
    $device = $_
    Get-Process | Where-Object {
        $_.Modules.DeviceName -match $device.DeviceID
    } | Stop-Process -Force -ErrorAction SilentlyContinue
}