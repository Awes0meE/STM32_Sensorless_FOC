param(
  [ValidateSet("build", "clean", "flash", "erase", "probe", "reset")]
  [string]$Action = "build"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = Join-Path $ProjectRoot "build"
$ObjDir = Join-Path $BuildDir "obj"
$Elf = Join-Path $BuildDir "stm32_drv8301.elf"
$Hex = Join-Path $BuildDir "stm32_drv8301.hex"
$Bin = Join-Path $BuildDir "stm32_drv8301.bin"
$Map = Join-Path $BuildDir "stm32_drv8301.map"

$CubeClt = if ($env:STM32CUBECLT_PATH) { $env:STM32CUBECLT_PATH } else { "D:\ST\STM32CubeCLT_1.21.0" }
$ToolBin = Join-Path $CubeClt "GNU-tools-for-STM32\bin"
$Gcc = Join-Path $ToolBin "arm-none-eabi-gcc.exe"
$Objcopy = Join-Path $ToolBin "arm-none-eabi-objcopy.exe"
$Size = Join-Path $ToolBin "arm-none-eabi-size.exe"
$Programmer = Join-Path $CubeClt "STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"

function Assert-File($Path) {
  if (-not (Test-Path $Path)) {
    throw "Missing required tool or file: $Path"
  }
}

function Invoke-Tool($Exe, [string[]]$Arguments) {
  & $Exe @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "$Exe failed with exit code $LASTEXITCODE"
  }
}

function Get-ObjPath($Source) {
  $full = (Resolve-Path (Join-Path $ProjectRoot $Source)).Path
  $rel = (Resolve-Path -Relative $full) -replace "^\.[\\/]", ""
  $safe = $rel -replace "[:\\/]", "_"
  return Join-Path $ObjDir "$safe.o"
}

function Build-Firmware {
  Assert-File $Gcc
  Assert-File $Objcopy
  Assert-File $Size

  New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

  $defines = @(
    "USE_STDPERIPH_DRIVER",
    "STM32F446xx",
    "ARM_MATH_CM4",
    "SIMULINK_USE_ARM_MATH",
    "__FPU_PRESENT=1"
  )

  $includes = @(
    "user",
    "motor",
    "user/pc_communication/inc",
    "Libraries",
    "Libraries/CMSIS/Include",
    "Libraries/CMSIS/Device/ST/STM32F4xx/Include",
    "Libraries/STM32F4xx_StdPeriph_Driver/inc"
  )

  $cpuFlags = @(
    "-mcpu=cortex-m4",
    "-mthumb",
    "-mfpu=fpv4-sp-d16",
    "-mfloat-abi=hard"
  )

  $commonFlags = @(
    "-g3",
    "-O2",
    "-ffunction-sections",
    "-fdata-sections",
    "-fmessage-length=0"
  ) + $cpuFlags

  $cFlags = @("-std=gnu99") + $commonFlags
  $asmFlags = @("-x", "assembler-with-cpp") + $commonFlags

  $defineFlags = $defines | ForEach-Object { "-D$_" }
  $includeFlags = $includes | ForEach-Object { "-I$_" }

  $sources = @(
    "Libraries/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c",
    "Libraries/CMSIS/Device/ST/STM32F4xx/Source/Templates/SW4STM32/startup_stm32f446xx.s",
    "Libraries/CMSIS/DSP_Lib/Source/CommonTables/arm_common_tables.c",
    "Libraries/CMSIS/DSP_Lib/Source/CommonTables/arm_const_structs.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_cos_f32.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_cos_q15.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_cos_q31.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_sin_f32.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_sin_q15.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_sin_q31.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_sqrt_q15.c",
    "Libraries/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_sqrt_q31.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/misc.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_adc.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_exti.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_syscfg.c",
    "Libraries/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c",
    "motor/adc.c",
    "motor/drv8301.c",
    "motor/foc_algorithm.c",
    "motor/hall_sensor.c",
    "motor/L_identification_wrapper.c",
    "motor/low_task.c",
    "motor/R_flux_identification_wrapper.c",
    "motor/speed_pid.c",
    "motor/stm32_ekf_wrapper.c",
    "user/board_config.c",
    "user/exti.c",
    "user/main.c",
    "user/oled.c",
    "user/oled_display.c",
    "user/oled_font.c",
    "user/pc_communication_init.c",
    "user/stm32f4xx_it.c",
    "tools/gcc_syscalls.c"
  )

  $objects = @()
  Push-Location $ProjectRoot
  try {
    foreach ($source in $sources) {
      $obj = Get-ObjPath $source
      $objects += $obj
      $objParent = Split-Path -Parent $obj
      New-Item -ItemType Directory -Force -Path $objParent | Out-Null

      $ext = [System.IO.Path]::GetExtension($source).ToLowerInvariant()
      if ($ext -eq ".s") {
        $args = $asmFlags + $defineFlags + $includeFlags + @("-c", $source, "-o", $obj)
      } else {
        $args = $cFlags + $defineFlags + $includeFlags + @("-c", $source, "-o", $obj)
      }
      Invoke-Tool $Gcc $args
    }

    $linker = "Keil_Project/STM32F446RETx_FLASH.ld"
    $linkArgs = $cpuFlags + @(
      "-T", $linker,
      "-Wl,-Map=$Map,--gc-sections,--print-memory-usage,--no-warn-rwx-segments",
      "-specs=nano.specs",
      "-specs=nosys.specs"
    ) + $objects + @("-lm", "-lc", "-lnosys", "-o", $Elf)

    Invoke-Tool $Gcc $linkArgs
    Invoke-Tool $Objcopy @("-O", "ihex", $Elf, $Hex)
    Invoke-Tool $Objcopy @("-O", "binary", $Elf, $Bin)
    Invoke-Tool $Size @($Elf)
  } finally {
    Pop-Location
  }
}

switch ($Action) {
  "clean" {
    if (Test-Path $BuildDir) {
      Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }
  }
  "build" {
    Build-Firmware
  }
  "probe" {
    Assert-File $Programmer
    Invoke-Tool $Programmer @("-l")
  }
  "flash" {
    Build-Firmware
    Assert-File $Programmer
    Invoke-Tool $Programmer @("-c", "port=SWD", "mode=UR", "-w", $Hex, "-v", "-rst")
  }
  "erase" {
    Assert-File $Programmer
    Invoke-Tool $Programmer @("-c", "port=SWD", "mode=UR", "-e", "all")
  }
  "reset" {
    Assert-File $Programmer
    Invoke-Tool $Programmer @("-c", "port=SWD", "mode=UR", "-rst")
  }
}
