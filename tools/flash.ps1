<#
.SYNOPSIS
    Automated Flashing Script for ZZLIGHT ESP32 Wireless Art-Net to DMX Node.
.DESCRIPTION
    Flashes pre-compiled v5.2.0 Hardened Production Master binaries or builds from source
    using esptool.exe and arduino-cli.
.PARAMETER Port
    Serial COM port (default: auto-detected, e.g. COM10)
.PARAMETER Baud
    Flashing baud rate (default: 921600)
#>
param(
    [string]$Port = "",
    [int]$Baud = 921600
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RepoRoot "build"
$BinFile = Join-Path $BuildDir "artnet_dmx_final.ino.bin"
$BootFile = Join-Path $BuildDir "artnet_dmx_final.ino.bootloader.bin"
$PartFile = Join-Path $BuildDir "artnet_dmx_final.ino.partitions.bin"
$BootAppFile = Join-Path $BuildDir "boot_app0.bin"

# 1. Locate esptool.exe
$esptool = "C:\Users\andre\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
if (-not (Test-Path $esptool)) {
    $found = Get-ChildItem "C:\Users\andre\AppData\Local\Arduino15\packages\esp32\tools\esptool_py" -Recurse -Filter "esptool.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $esptool = $found.FullName }
}
if (-not (Test-Path $esptool)) {
    Write-Error "esptool.exe tidak ditemukan di Arduino15. Pastikan ESP32 Board Core terpasang."
}

# 2. Auto-detect COM port if not specified
if (-not $Port) {
    $devices = Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match "CH9102|CH340|CP210|USB-SERIAL|UART" -and $_.Name -match "\(COM\d+\)" }
    if ($devices) {
        if ($devices[0].Name -match '\((COM\d+)\)') {
            $Port = $Matches[1]
            Write-Host "✓ Terdeteksi port serial otomatis: $Port ($($devices[0].Name))" -ForegroundColor Green
        }
    }
}
if (-not $Port) {
    $ports = [System.IO.Ports.SerialPort]::GetPortNames()
    if ($ports.Count -eq 1) {
        $Port = $ports[0]
        Write-Host "✓ Menggunakan port tunggal terdeteksi: $Port" -ForegroundColor Green
    } elseif ($ports.Count -gt 1) {
        $Port = $ports[0]
        Write-Host "! Ditemukan banyak port ($($ports -join ', ')). Memilih $Port." -ForegroundColor Yellow
    } else {
        Write-Error "Tidak ada ESP32 yang terdeteksi di port serial. Sambungkan kabel USB lalu coba lagi."
    }
}

# 3. Verify binaries exist
if (-not (Test-Path $BinFile) -or -not (Test-Path $BootFile) -or -not (Test-Path $PartFile)) {
    Write-Host "Biner belum tersedia di folder build/, mengompilasi dari sumber..." -ForegroundColor Cyan
    $cli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    $tempDir = Join-Path $env:TEMP "arduino_build_test\artnet_dmx_final"
    New-Item -ItemType Directory -Force $tempDir | Out-Null
    Copy-Item (Join-Path $RepoRoot "firmware\artnet_dmx_final.ino") "$tempDir\artnet_dmx_final.ino" -Force
    & $cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 "$tempDir\artnet_dmx_final.ino"
    $sketchBuild = Get-ChildItem "C:\Users\andre\AppData\Local\arduino\sketches" -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    New-Item -ItemType Directory -Force $BuildDir | Out-Null
    Copy-Item "$($sketchBuild.FullName)\*.bin" $BuildDir -Force
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "⚡ ZZLIGHT ESP32 Flasher — v5.2.0 Hardened Production Master" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Target Port : $Port" -ForegroundColor White
Write-Host "Baud Rate   : $Baud" -ForegroundColor White
Write-Host "Firmware    : $BinFile" -ForegroundColor White
Write-Host "------------------------------------------------------------" -ForegroundColor Gray

# 4. Execute esptool flash
& $esptool `
    --chip esp32 `
    --port $Port `
    --baud $Baud `
    --before default_reset `
    --after hard_reset `
    write_flash -z `
    --flash_mode dio `
    --flash_freq 80m `
    --flash_size 4MB `
    0x1000 $BootFile `
    0x8000 $PartFile `
    0xe000 $BootAppFile `
    0x10000 $BinFile

if ($LASTEXITCODE -eq 0) {
    Write-Host "------------------------------------------------------------" -ForegroundColor Gray
    Write-Host "🎉 FLASHING SELESAI & BERHASIL 100%!" -ForegroundColor Green
    Write-Host "ESP32 telah di-hard-reset dan kini menjalankan firmware v5.2.0." -ForegroundColor Green
    Write-Host "Layar LCD 16x2 akan menampilkan 'ZZLIGHT PROJECT' lalu 'ARTNET-DMX'." -ForegroundColor Green
} else {
    Write-Error "Flashing gagal dengan exit code $LASTEXITCODE."
}
