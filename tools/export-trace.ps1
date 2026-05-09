param(
  [string]$OutCsv = "",
  [int]$Port = 61234
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = Join-Path $ProjectRoot "build"
$Elf = Join-Path $BuildDir "stm32_drv8301.elf"
$TraceWorkDir = Join-Path $env:TEMP "stm32_trace"
$TempElf = Join-Path $TraceWorkDir "stm32_drv8301.elf"
$TraceBin = Join-Path $TraceWorkDir "trace_buffer.bin"
$TraceMetaBin = Join-Path $TraceWorkDir "trace_meta.bin"
$GdbScript = Join-Path $TraceWorkDir "trace_dump.gdb"
$ServerLog = Join-Path $TraceWorkDir "trace_gdbserver.log"

if ([string]::IsNullOrWhiteSpace($OutCsv)) {
  $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
  $OutCsv = Join-Path $BuildDir "trace_$stamp.csv"
}

$CubeClt = if ($env:STM32CUBECLT_PATH) { $env:STM32CUBECLT_PATH } else { "D:\ST\STM32CubeCLT_1.21.0" }
$ToolBin = Join-Path $CubeClt "GNU-tools-for-STM32\bin"
$Gdb = Join-Path $ToolBin "arm-none-eabi-gdb.exe"
$GdbServer = Join-Path $CubeClt "STLink-gdb-server\bin\ST-LINK_gdbserver.exe"
$ProgrammerDir = Join-Path $CubeClt "STM32CubeProgrammer\bin"

function Assert-File($Path) {
  if (-not (Test-Path $Path)) {
    throw "Missing required file: $Path"
  }
}

function Convert-GdbPath($Path) {
  if (Test-Path $Path) {
    return ((Resolve-Path $Path).Path -replace "\\", "/")
  }
  return ([System.IO.Path]::GetFullPath($Path) -replace "\\", "/")
}

function Read-U32($Bytes, $Offset) {
  return [BitConverter]::ToUInt32($Bytes, $Offset)
}

function Read-I16($Bytes, $Offset) {
  return [BitConverter]::ToInt16($Bytes, $Offset)
}

function Read-U8($Bytes, $Offset) {
  return [uint32]$Bytes[$Offset]
}

Assert-File $Elf
Assert-File $Gdb
Assert-File $GdbServer

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $TraceWorkDir | Out-Null
$OutParent = Split-Path -Parent $OutCsv
if (-not [string]::IsNullOrWhiteSpace($OutParent)) {
  New-Item -ItemType Directory -Force -Path $OutParent | Out-Null
}

Remove-Item -LiteralPath $TraceBin,$TraceMetaBin,$GdbScript -ErrorAction SilentlyContinue
Copy-Item -LiteralPath $Elf -Destination $TempElf -Force

$elfGdb = Convert-GdbPath $TempElf
$traceBinGdb = Convert-GdbPath $TraceBin
$traceMetaGdb = Convert-GdbPath $TraceMetaBin

@"
set confirm off
set pagination off
file $elfGdb
target remote localhost:$Port
monitor halt
dump binary memory $traceBinGdb &trace_buffer ((char*)&trace_buffer)+sizeof(trace_buffer)
dump binary memory $traceMetaGdb &trace_meta ((char*)&trace_meta)+sizeof(trace_meta)
detach
quit
"@ | Set-Content -Path $GdbScript -Encoding ascii

$serverArgs = @(
  "-p", "$Port",
  "-cp", $ProgrammerDir,
  "-d",
  "-g",
  "-f", $ServerLog,
  "-l", "1"
)

$server = $null
try {
  $server = Start-Process -FilePath $GdbServer -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
  Start-Sleep -Seconds 2

  & $Gdb -q -batch -x $GdbScript
  if ($LASTEXITCODE -ne 0) {
    throw "GDB trace dump failed with exit code $LASTEXITCODE"
  }
}
finally {
  if ($server -ne $null -and -not $server.HasExited) {
    Stop-Process -Id $server.Id -Force
  }
}

Assert-File $TraceBin
Assert-File $TraceMetaBin

$meta = [System.IO.File]::ReadAllBytes($TraceMetaBin)
$magic = Read-U32 $meta 0
$writeIndex = Read-U32 $meta 4
$wrapped = Read-U32 $meta 8
$sampleCount = Read-U32 $meta 12
$active = Read-U32 $meta 16
$bufferSize = Read-U32 $meta 20
$recordSize = Read-U32 $meta 24
$samplePeriodMs = Read-U32 $meta 28

if ($magic -ne 0x54434f46) {
  throw ("Trace magic mismatch: 0x{0:x8}" -f $magic)
}
if (($recordSize -ne 32) -and ($recordSize -ne 48) -and ($recordSize -ne 52)) {
  throw "Unexpected trace record size: $recordSize"
}

$bytes = [System.IO.File]::ReadAllBytes($TraceBin)
$recordsInDump = [Math]::Floor($bytes.Length / $recordSize)
if ($bufferSize -gt $recordsInDump) {
  $bufferSize = $recordsInDump
}

if ($wrapped -ne 0) {
  $recordsToWrite = $bufferSize
  $startIndex = $writeIndex % $bufferSize
} else {
  $recordsToWrite = [Math]::Min($writeIndex, $bufferSize)
  $startIndex = 0
}

$lines = New-Object System.Collections.Generic.List[string]
if ($recordSize -ge 52) {
  $lines.Add("time_ms,ol_hz,ekf_hz,iq_ref_a,iq_fb_a,id_fb_a,ia_a,ib_a,ic_a,vbus_v,vd_v,vq_v,angle_rad,target_hz,foc_theta_rad,ekf_angle_rad,valpha_v,vbeta_v,ialpha_a,ibeta_a,ekf_angle_err_rad,ekf_speed_ratio,diag_flags,state,fault,motor,speed_flag")
} elseif ($recordSize -ge 48) {
  $lines.Add("time_ms,ol_hz,ekf_hz,iq_ref_a,iq_fb_a,id_fb_a,ia_a,ib_a,ic_a,vbus_v,vd_v,vq_v,angle_rad,target_hz,foc_theta_rad,ekf_angle_rad,valpha_v,vbeta_v,ialpha_a,ibeta_a,diag_flags,state,fault,motor,speed_flag")
} else {
  $lines.Add("time_ms,ol_hz,ekf_hz,iq_ref_a,iq_fb_a,id_fb_a,ia_a,ib_a,ic_a,vbus_v,vd_v,vq_v,angle_rad,state,fault,motor,speed_flag")
}

for ($n = 0; $n -lt $recordsToWrite; $n++) {
  $recordIndex = ($startIndex + $n) % $bufferSize
  $base = $recordIndex * $recordSize
  $timeMs = Read-U32 $bytes ($base + 0)
  $ol = (Read-I16 $bytes ($base + 4)) / 10.0
  $ekf = (Read-I16 $bytes ($base + 6)) / 10.0
  $iqRef = (Read-I16 $bytes ($base + 8)) / 100.0
  $iqFb = (Read-I16 $bytes ($base + 10)) / 100.0
  $idFb = (Read-I16 $bytes ($base + 12)) / 100.0
  $ia = (Read-I16 $bytes ($base + 14)) / 100.0
  $ib = (Read-I16 $bytes ($base + 16)) / 100.0
  $ic = (Read-I16 $bytes ($base + 18)) / 100.0
  $vbus = (Read-I16 $bytes ($base + 20)) / 100.0
  $vd = (Read-I16 $bytes ($base + 22)) / 100.0
  $vq = (Read-I16 $bytes ($base + 24)) / 100.0
  $angle = (Read-I16 $bytes ($base + 26)) / 1000.0
  if ($recordSize -ge 52) {
    $targetHz = (Read-I16 $bytes ($base + 28)) / 10.0
    $focTheta = (Read-I16 $bytes ($base + 30)) / 1000.0
    $ekfAngle = (Read-I16 $bytes ($base + 32)) / 1000.0
    $valpha = (Read-I16 $bytes ($base + 34)) / 100.0
    $vbeta = (Read-I16 $bytes ($base + 36)) / 100.0
    $ialpha = (Read-I16 $bytes ($base + 38)) / 100.0
    $ibeta = (Read-I16 $bytes ($base + 40)) / 100.0
    $ekfAngleErr = (Read-I16 $bytes ($base + 42)) / 1000.0
    $ekfSpeedRatio = (Read-I16 $bytes ($base + 44)) / 1000.0
    $diagFlags = Read-I16 $bytes ($base + 46)
    $state = Read-U8 $bytes ($base + 48)
    $fault = Read-U8 $bytes ($base + 49)
    $motor = Read-U8 $bytes ($base + 50)
    $speedFlag = Read-U8 $bytes ($base + 51)

    $lines.Add(("{0},{1:f1},{2:f1},{3:f2},{4:f2},{5:f2},{6:f2},{7:f2},{8:f2},{9:f2},{10:f2},{11:f2},{12:f3},{13:f1},{14:f3},{15:f3},{16:f2},{17:f2},{18:f2},{19:f2},{20:f3},{21:f3},{22},{23},{24},{25},{26}" -f `
      $timeMs,$ol,$ekf,$iqRef,$iqFb,$idFb,$ia,$ib,$ic,$vbus,$vd,$vq,$angle,$targetHz,$focTheta,$ekfAngle,$valpha,$vbeta,$ialpha,$ibeta,$ekfAngleErr,$ekfSpeedRatio,$diagFlags,$state,$fault,$motor,$speedFlag))
  } elseif ($recordSize -ge 48) {
    $targetHz = (Read-I16 $bytes ($base + 28)) / 10.0
    $focTheta = (Read-I16 $bytes ($base + 30)) / 1000.0
    $ekfAngle = (Read-I16 $bytes ($base + 32)) / 1000.0
    $valpha = (Read-I16 $bytes ($base + 34)) / 100.0
    $vbeta = (Read-I16 $bytes ($base + 36)) / 100.0
    $ialpha = (Read-I16 $bytes ($base + 38)) / 100.0
    $ibeta = (Read-I16 $bytes ($base + 40)) / 100.0
    $diagFlags = Read-I16 $bytes ($base + 42)
    $state = Read-U8 $bytes ($base + 44)
    $fault = Read-U8 $bytes ($base + 45)
    $motor = Read-U8 $bytes ($base + 46)
    $speedFlag = Read-U8 $bytes ($base + 47)

    $lines.Add(("{0},{1:f1},{2:f1},{3:f2},{4:f2},{5:f2},{6:f2},{7:f2},{8:f2},{9:f2},{10:f2},{11:f2},{12:f3},{13:f1},{14:f3},{15:f3},{16:f2},{17:f2},{18:f2},{19:f2},{20},{21},{22},{23},{24}" -f `
      $timeMs,$ol,$ekf,$iqRef,$iqFb,$idFb,$ia,$ib,$ic,$vbus,$vd,$vq,$angle,$targetHz,$focTheta,$ekfAngle,$valpha,$vbeta,$ialpha,$ibeta,$diagFlags,$state,$fault,$motor,$speedFlag))
  } else {
    $state = Read-U8 $bytes ($base + 28)
    $fault = Read-U8 $bytes ($base + 29)
    $motor = Read-U8 $bytes ($base + 30)
    $speedFlag = Read-U8 $bytes ($base + 31)

    $lines.Add(("{0},{1:f1},{2:f1},{3:f2},{4:f2},{5:f2},{6:f2},{7:f2},{8:f2},{9:f2},{10:f2},{11:f2},{12:f3},{13},{14},{15},{16}" -f `
      $timeMs,$ol,$ekf,$iqRef,$iqFb,$idFb,$ia,$ib,$ic,$vbus,$vd,$vq,$angle,$state,$fault,$motor,$speedFlag))
  }
}

$lines | Set-Content -Path $OutCsv -Encoding utf8

Write-Host "Trace CSV: $OutCsv"
Write-Host "samples=$recordsToWrite period_ms=$samplePeriodMs write_index=$writeIndex wrapped=$wrapped total_sample_count=$sampleCount active=$active"
