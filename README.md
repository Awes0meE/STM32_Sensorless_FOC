<h1 align="center">STM32 Sensorless FOC</h1>

<p align="center">
  <strong>Sensorless FOC firmware for a custom STM32F446RET6 + DRV8301 compressor driver board.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F446RET6-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white" alt="MCU">
  <img src="https://img.shields.io/badge/Gate%20Driver-DRV8301-CC0000?style=for-the-badge&logo=texasinstruments&logoColor=white" alt="DRV8301">
  <img src="https://img.shields.io/badge/Control-Sensorless%20FOC-0A66C2?style=for-the-badge" alt="Sensorless FOC">
  <img src="https://img.shields.io/badge/Library-STM32F4%20StdPeriph-2E8B57?style=for-the-badge" alt="StdPeriph">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/IDE-Keil%20%7C%20IAR%20%7C%20VSCode-6A5ACD?style=flat-square" alt="IDE">
  <img src="https://img.shields.io/badge/Mode-Open--Loop%20Compressor%20Demo-orange?style=flat-square" alt="Mode">
  <img src="https://img.shields.io/badge/PC%20Communication-Disabled%20by%20Default-lightgrey?style=flat-square" alt="PC Communication">
  <img src="https://img.shields.io/badge/Target-12V%20Bench%20Validation-blue?style=flat-square" alt="Target">
</p>

<p align="center">
  <a href="#zh">
    <img src="https://img.shields.io/badge/Language-中文说明-red?style=for-the-badge" alt="中文说明">
  </a>
  <a href="#en">
    <img src="https://img.shields.io/badge/Language-English%20README-blue?style=for-the-badge" alt="English README">
  </a>
</p>

---

<a id="zh"></a>

<details open>
<summary><strong>🇨🇳 中文说明</strong></summary>

## 项目简介

本仓库是基于 `STM32F446RET6 + DRV8301` 的无感 FOC 固件工程。当前适配目标是用户手头的自制 PCB，而不是依赖原工程的 USB 上位机调试环境。

长期项目记忆、硬件映射、参数快照和调试日志见 [`codex.md`](codex.md)。

---

## 工程状态

| 项目 | 当前状态 |
| --- | --- |
| MCU | `STM32F446RET6` |
| Gate Driver | `DRV8301DCAR` |
| 固件库 | STM32F4 StdPeriph 标准外设库，不是 HAL |
| 主工程 | `Keil_Project/stm32_drv8301_keil.uvprojx` |
| 兼容工程 | 根目录保留 IAR 工程文件 |
| 当前默认模式 | `SENSORLESS_FOC_SELECT` |
| USB / PC 上位机通信 | 默认关闭 |

当前 `PC_COMMUNICATION_ENABLE` 默认为 `0`，因此不需要 USB 上位机软件即可运行低速产品演示版本。

---

## 当前硬件适配

已按 2026-05-09 的 PCB 网表核对并适配：

| 功能模块 | 引脚 / 资源 |
| --- | --- |
| TIM1 六路互补 PWM | PA8 / PA9 / PA10 与 PB13 / PB14 / PB15 |
| 两相电流采样 | PC1 / PC2 |
| 母线电流 | PA6 |
| 母线电压 | PC0，分压系数 `100k / 4.99k` |
| DRV8301 bit-bang SPI | PA15 / PC10 / PC11 / PC12 |
| OLED SH1106 | PB3 / PB5 / PC13 / PC8 / PC7 |
| 按键 | PC4 / PC5 / PB2 |

---

## 不依赖上位机的操作方式

| 操作器件 | 功能 |
| --- | --- |
| `KEY1 / PC4` | 电机启停 |
| `KEY2 / PC5` | 切换 OLED 显示 |
| `EC11 / PB6/PB7` | 调节开环目标速度 |

当前用于低速产品演示，上电默认设定为 `1200rpm / 80Hz`，旋钮最低可调到 `450rpm / 30Hz`，每格 `10rpm / 0.667Hz`。

按 `KEY1` 后会直接爬到当前旋钮设定值，不再先冲到默认 `1200rpm`。

OLED 显示含义：

| 字段 | 含义 |
| --- | --- |
| `rpm:` | 当前开环机械转速 |
| `set:` | 旋钮设定转速 |
| `ol:` | 当前开环拖动电频率 |
| `r:` | 命令 q 轴电流 |
| `q:` | 实际 q 轴反馈电流 |

---

## 压缩机安全启动版本

当前代码按 12V 台架低速试机做了第一版安全边界。

| 项目 | 当前设定 |
| --- | --- |
| 目标设备 | Panasonic `6MD030Z` 小型 24V 无感正弦波压缩机 |
| 当前验证方式 | 12V 先低速验证 |
| 速度参考范围 | `30-80Hz` 电频率 |
| 机械转速等效 | 对 4 对极压缩机约等价于 `450-1200rpm` |
| 自动上探 | 当前安全版不会自动上探 `120Hz+` |
| 母线保护 | `9.0-15.5V` |
| 软件过流保护 | 相电流连续超过 `6A` 会停机 |
| 重启保护 | 停机 / 故障后保留 `10s` 重启等待 |
| DRV8301 硬件过流阈值 | 已从 `OC_ADJ_SET_18` 降到 `OC_ADJ_SET_8` |

### 启动逻辑

- 前 `1s` 使用 `Id=3A / Iq=0A` 定位。
- 之后使用 `Id=3A / Iq=0-2.5A` 慢拖。
- `Iq` 约 `6.7s` 爬到 `2.5A`。
- 当前使用 `1s` 固定电角度定位。
- 定位后按 EC11 设定值做 `1-30..80Hz` 正向开环慢拖并保持。
- 升速 `2Hz/s`，降速 `8Hz/s`。
- 当前诊断版先不让 EKF / 速度闭环接管。

### 当前验证结果

12V 台架下，`30Hz / 45Hz / 60Hz` 开环保持已可稳定运行。用户反馈 `OL≈80Hz` 吸力已经满足当前产品需求。

此前 `120Hz+` 上探会带来明显温升，因此当前固件保持低速开环驱动，不自动闭环接管。

### OLED 保护码

OLED 第二页的 `F:` 是压缩机保护码：

| 代码 | 含义 |
| --- | --- |
| `1` | 欠压 |
| `2` | 过压 |
| `3` | 过流 |
| `4` | 堵转 |
| `5` | 重启等待 |

---

## 重要参数位置

关键参数集中在以下文件：

| 文件 | 内容 |
| --- | --- |
| `user/board_config.h` | PWM、ADC、引脚、通信开关 |
| `motor/adc.h` | ADC 换算、电流采样、电压采样系数 |
| `motor/foc_define_parameter.h` | FOC 模式、电机参数、启动电流 |
| `motor/speed_pid.c` | 速度环 PI 参数和输出限幅 |
| `motor/drv8301.c` | DRV8301 寄存器初始化 |

当前参数是压缩机 12V 安全启动初值，不应视为最终整定值。

换电机、换母线电压或带载前，需要重新低压限流调试。

---

## 构建与检查

首选用 Keil 打开：

```text
Keil_Project/stm32_drv8301_keil.uvprojx
```

命令行环境可用 `arm-none-eabi-gcc -fsyntax-only` 做轻量语法检查；最终烧录前仍建议用 Keil / IAR 做完整构建。

---

## VSCode + ST-Link

仓库已经补齐 VSCode 下的 GCC 编译、ST-Link 下载和调试配置。

默认使用本机工具链路径：

```text
D:/ST/STM32CubeCLT_1.21.0
```

如果工具链装在别的位置，构建脚本可通过环境变量 `STM32CUBECLT_PATH` 覆盖；调试配置则需要同步修改 `.vscode/launch.json` 里的 ST 工具路径。

### VSCode 推荐插件

| 插件 | ID |
| --- | --- |
| Microsoft C/C++ | `ms-vscode.cpptools` |
| Cortex-Debug | `marus25.cortex-debug` |

### 常用任务

| 任务 | 功能 |
| --- | --- |
| `Build firmware` | 生成 `build/stm32_drv8301.elf/.hex/.bin` |
| `Probe ST-Link` | 枚举 ST-Link 探针 |
| `Flash firmware (ST-Link)` | 编译后用 SWD 下载并复位 |
| `Erase chip (ST-Link)` | 整片擦除 |
| `Reset target (ST-Link)` | 复位目标板 |
| `Export trace CSV` | 从 MCU RAM 导出已冻结的启动黑匣子 CSV |

调试入口在 VSCode Run and Debug 里选择 `Debug firmware (ST-Link)`。

下载前请确认压缩机电源限流，且不要让压缩机在无人看管状态下启动。

---

## RAM Trace 黑匣子

当前固件带 RAM trace 黑匣子：

- 按 `KEY1` 启动时清空并开始记录。
- 故障或停机后冻结。
- trace 每 `40ms` 采样一次。
- 最多约 `82s`。
- 导出脚本为 `tools/export-trace.ps1`。

基础字段包括：

```text
ol / ekf / r / q / Id / Ia / Ib / Ic / Vbus / Vd / Vq / state / fault
```

当前诊断版新增字段包括：

```text
target_hz / foc_theta / ekf_angle / Valpha / Vbeta / Ialpha / Ibeta /
ekf_angle_err / ekf_speed_ratio / diag_flags
```

这些字段用于排查 EKF 符号、初始化分支和开环到 EKF 接管条件。

---

## 当前分支状态

当前分支：

```text
feature/open-loop-pot-compressor
```

该分支已固化为低速 EC11 产品演示版：

- 上电不自启动。
- 按 `KEY1` 后以开环 FOC 启动。
- 启动后保持在旋钮设定的 `30-80Hz` 范围内。
- 历史 EKF handoff 诊断代码仍保留在工程中。
- 当前 `EKF_HANDOFF_BLEND_ENABLE=0`。
- EKF 只用于显示和 trace 观察。

---

## EKF 诊断状态

当前进一步禁止 EKF 在 `1s` 定位阶段更新：

- 定位阶段只跑电流环和开环定位。
- 定位结束瞬间重置 EKF。
- 之后再让 EKF 从开环旋转开始观察。

这样可避免静止定位阶段的电压 / 电流瞬态把 EKF 预先带到错误速度分支。

历史诊断里曾试过 EKF beta 轴输入取反和磁链半值：

- beta 取反在旧参数下能改变方向，但幅值仍约半速。
- 磁链半值让 `45Hz` EKF 从约 `+21.9Hz` 提高到约 `+27.3Hz`。
- 这证明磁链方向判断有意义，但不能单独解释半速问题。

---

## 当前确认版参数

当前烧录的是“确认版参数 + EKF 增益修复 + beta 正常符号”版本：

| 参数 | 数值 |
| --- | --- |
| `RS_PARAMETER` | `0.188f` |
| `LS_PARAMETER` | `0.00036f` |
| `FLUX_PARAMETER` | `0.00580f` |
| `EKF_V_BETA_SIGN` | `+1` |
| `EKF_I_BETA_SIGN` | `+1` |

依据：

- `6MD030Z` 为 8 极 / 4 对极。
- 端子相间电阻 `0.376Ω`，模型单相电阻取一半。
- `3A` 通电 `Ld/Lq` 约 `0.34/0.38mH`。
- 单电感 EKF 暂取平均。

此版还修复了 `stm32_ekf_wrapper.c` 中 Kalman 增益矩阵计算时覆盖原值的问题。

### beta 正常符号复测结果

| 开环电频率 | EKF 估算电频率 | EKF / OL |
| --- | --- | --- |
| `30Hz` | `+29.21Hz` | `0.974` |
| `45Hz` | `+43.06Hz` | `0.957` |
| `60Hz` | `+56.93Hz` | `0.949` |

EKF 方向和速度尺度已基本修复，下一阶段可以设计低风险开环到 EKF 角度 / 速度的闭环接管。

---

## 编码约定

源码和文档统一使用 UTF-8。

仓库已用 `.editorconfig` 和 `.gitattributes` 固定编码与换行规则，避免中文注释在 GitHub 或现代编辑器中显示为乱码。

---

## 上电提醒

首次烧录后请低压、限流、空载运行，先确认：

- DRV8301 `FAULT` 未触发
- 电流零偏采样完成
- OLED 显示正常
- KEY1 能正确启停
- 相电流、母线电压换算值合理

确认以上项目后，再逐步提高母线电压和负载。

</details>

---

<a id="en"></a>

<details>
<summary><strong>🇬🇧 English README</strong></summary>

## Project Overview

This repository contains a sensorless FOC firmware project based on `STM32F446RET6 + DRV8301`. The current target is the user's custom PCB, rather than the original project's USB PC debugging environment.

Long-term project memory, hardware mapping, parameter snapshots, and debug logs are documented in [`codex.md`](codex.md).

---

## Project Status

| Item | Current Status |
| --- | --- |
| MCU | `STM32F446RET6` |
| Gate Driver | `DRV8301DCAR` |
| Firmware Library | STM32F4 StdPeriph library, not HAL |
| Main Project | `Keil_Project/stm32_drv8301_keil.uvprojx` |
| Compatibility Project | IAR project files are kept in the repository root |
| Default Mode | `SENSORLESS_FOC_SELECT` |
| USB / PC Communication | Disabled by default |

`PC_COMMUNICATION_ENABLE` is currently set to `0`, so the firmware does not require the USB PC software for the low-speed product demonstration mode.

---

## Current Hardware Adaptation

The firmware has been checked and adapted according to the PCB netlist on 2026-05-09:

| Function | Pins / Resources |
| --- | --- |
| TIM1 six complementary PWM outputs | PA8 / PA9 / PA10 and PB13 / PB14 / PB15 |
| Two-phase current sensing | PC1 / PC2 |
| DC bus current | PA6 |
| DC bus voltage | PC0, divider ratio `100k / 4.99k` |
| DRV8301 bit-bang SPI | PA15 / PC10 / PC11 / PC12 |
| OLED SH1106 | PB3 / PB5 / PC13 / PC8 / PC7 |
| Buttons | PC4 / PC5 / PB2 |

---

## Standalone Operation Without PC Software

| Control | Function |
| --- | --- |
| `KEY1 / PC4` | Motor start / stop |
| `KEY2 / PC5` | Switch OLED page |
| `EC11 / PB6/PB7` | Adjust open-loop target speed |

The current firmware is used for a low-speed product demonstration. The default setting after power-up is `1200rpm / 80Hz`. The minimum encoder setting is `450rpm / 30Hz`, with each step equal to `10rpm / 0.667Hz`.

After pressing `KEY1`, the motor ramps directly to the current encoder setting. It no longer ramps to the default `1200rpm` first.

OLED field meanings:

| Field | Meaning |
| --- | --- |
| `rpm:` | Current open-loop mechanical speed |
| `set:` | Encoder target speed |
| `ol:` | Current open-loop electrical drag frequency |
| `r:` | Commanded q-axis current |
| `q:` | Actual q-axis feedback current |

---

## Compressor Safe-Start Version

The current code implements the first safety boundary version for 12V bench low-speed testing.

| Item | Current Setting |
| --- | --- |
| Target Device | Panasonic `6MD030Z` small 24V sensorless sinusoidal compressor |
| Current Validation Method | Low-speed verification at 12V first |
| Speed Reference Range | `30-80Hz` electrical frequency |
| Mechanical Speed Equivalent | Approximately `450-1200rpm` for a 4-pole-pair compressor |
| Automatic High-Speed Exploration | The safe version does not automatically explore `120Hz+` |
| DC Bus Protection | `9.0-15.5V` |
| Software Overcurrent Protection | Stops if phase current continuously exceeds `6A` |
| Restart Protection | Keeps a `10s` restart waiting time after stop / fault |
| DRV8301 Hardware OC Threshold | Reduced from `OC_ADJ_SET_18` to `OC_ADJ_SET_8` |

### Startup Logic

- The first `1s` uses `Id=3A / Iq=0A` for alignment.
- Then it uses `Id=3A / Iq=0-2.5A` for slow dragging.
- `Iq` ramps to `2.5A` in about `6.7s`.
- The current startup uses a fixed electrical angle for `1s`.
- After alignment, the compressor performs forward open-loop dragging from `1Hz` to the EC11 target range of `30..80Hz` and then holds.
- Acceleration is `2Hz/s`.
- Deceleration is `8Hz/s`.
- The current diagnostic version does not allow EKF / speed closed-loop takeover yet.

### Current Validation Result

On the 12V bench, open-loop holding at `30Hz / 45Hz / 60Hz` has been stable. User feedback indicates that `OL≈80Hz` already provides enough suction for the current product requirement.

Previous `120Hz+` exploration caused noticeable temperature rise, so the current firmware keeps low-speed open-loop drive and does not automatically switch to closed-loop control.

### OLED Protection Code

The `F:` field on the second OLED page is the compressor protection code:

| Code | Meaning |
| --- | --- |
| `1` | Undervoltage |
| `2` | Overvoltage |
| `3` | Overcurrent |
| `4` | Stall |
| `5` | Restart waiting |

---

## Important Parameter Locations

Key parameters are concentrated in the following files:

| File | Content |
| --- | --- |
| `user/board_config.h` | PWM, ADC, pins, communication switches |
| `motor/adc.h` | ADC conversion, current sensing, voltage sensing coefficients |
| `motor/foc_define_parameter.h` | FOC mode, motor parameters, startup current |
| `motor/speed_pid.c` | Speed-loop PI parameters and output limit |
| `motor/drv8301.c` | DRV8301 register initialization |

The current parameters are only initial values for 12V compressor safe-start testing. They should not be treated as final tuned values.

Before changing the motor, DC bus voltage, or load condition, low-voltage current-limited debugging must be repeated.

---

## Build and Check

The preferred build entry is the Keil project:

```text
Keil_Project/stm32_drv8301_keil.uvprojx
```

For command-line checking, `arm-none-eabi-gcc -fsyntax-only` can be used for lightweight syntax validation. Before final flashing, a full build with Keil / IAR is still recommended.

---

## VSCode + ST-Link

The repository now includes GCC build, ST-Link flashing, and debugging configuration for VSCode.

The default local toolchain path is:

```text
D:/ST/STM32CubeCLT_1.21.0
```

If the toolchain is installed elsewhere, the build script can be overridden with the environment variable `STM32CUBECLT_PATH`. The ST tool path in `.vscode/launch.json` also needs to be updated accordingly.

### Recommended VSCode Extensions

| Extension | ID |
| --- | --- |
| Microsoft C/C++ | `ms-vscode.cpptools` |
| Cortex-Debug | `marus25.cortex-debug` |

### Common Tasks

| Task | Function |
| --- | --- |
| `Build firmware` | Generate `build/stm32_drv8301.elf/.hex/.bin` |
| `Probe ST-Link` | Enumerate the ST-Link probe |
| `Flash firmware (ST-Link)` | Build, flash through SWD, and reset |
| `Erase chip (ST-Link)` | Full chip erase |
| `Reset target (ST-Link)` | Reset the target board |
| `Export trace CSV` | Export the frozen startup black-box CSV from MCU RAM |

The debug entry is `Debug firmware (ST-Link)` in VSCode Run and Debug.

Before flashing, make sure the compressor power supply is current-limited. Do not allow the compressor to start unattended.

---

## RAM Trace Black Box

The current firmware includes a RAM trace black box:

- The trace is cleared and started when `KEY1` is pressed.
- The trace is frozen after a fault or stop.
- The sampling interval is `40ms`.
- The maximum duration is about `82s`.
- The export script is `tools/export-trace.ps1`.

Basic fields include:

```text
ol / ekf / r / q / Id / Ia / Ib / Ic / Vbus / Vd / Vq / state / fault
```

The current diagnostic version also records:

```text
target_hz / foc_theta / ekf_angle / Valpha / Vbeta / Ialpha / Ibeta /
ekf_angle_err / ekf_speed_ratio / diag_flags
```

These fields are used to diagnose EKF sign, initialization branches, and open-loop to EKF takeover conditions.

---

## Current Branch Status

Current branch:

```text
feature/open-loop-pot-compressor
```

This branch has been frozen as a low-speed EC11 product demonstration version:

- No auto-start after power-up.
- After pressing `KEY1`, the compressor starts with open-loop FOC.
- The compressor then holds within the EC11 target range of `30-80Hz`.
- Historical EKF handoff diagnostic code is still retained in the project.
- `EKF_HANDOFF_BLEND_ENABLE=0`.
- EKF is only used for display and trace observation.

---

## EKF Diagnostic Status

The current firmware further prevents EKF updates during the `1s` alignment stage:

- During alignment, only the current loop and open-loop alignment are executed.
- EKF is reset at the instant alignment ends.
- EKF then starts observing from open-loop rotation.

This prevents voltage / current transients during static alignment from pulling EKF into the wrong speed branch in advance.

Historical diagnostics tested EKF beta-axis input inversion and half-flux setting:

- Beta inversion changed the direction under old parameters, but the magnitude remained approximately half-speed.
- Half flux increased the `45Hz` EKF value from about `+21.9Hz` to about `+27.3Hz`.
- This proves that flux direction judgement is meaningful, but it does not alone explain the half-speed issue.

---

## Current Confirmed Parameters

The currently flashed version is the “confirmed parameters + EKF gain fix + normal beta sign” version:

| Parameter | Value |
| --- | --- |
| `RS_PARAMETER` | `0.188f` |
| `LS_PARAMETER` | `0.00036f` |
| `FLUX_PARAMETER` | `0.00580f` |
| `EKF_V_BETA_SIGN` | `+1` |
| `EKF_I_BETA_SIGN` | `+1` |

Basis:

- `6MD030Z` is an 8-pole / 4-pole-pair compressor.
- Terminal line-to-line resistance is `0.376Ω`; the model uses half of this value as the single-phase resistance.
- At `3A`, measured `Ld/Lq` is about `0.34/0.38mH`.
- The single-inductance EKF temporarily uses the average value.

This version also fixes the issue in `stm32_ekf_wrapper.c` where original values were overwritten during Kalman gain matrix calculation.

### Retest Result with Normal Beta Sign

| Open-Loop Electrical Frequency | EKF Estimated Electrical Frequency | EKF / OL |
| --- | --- | --- |
| `30Hz` | `+29.21Hz` | `0.974` |
| `45Hz` | `+43.06Hz` | `0.957` |
| `60Hz` | `+56.93Hz` | `0.949` |

EKF direction and speed scale have been basically corrected. The next stage can design a low-risk closed-loop takeover from open-loop angle / speed to EKF angle / speed.

---

## Encoding Convention

All source code and documentation use UTF-8.

The repository uses `.editorconfig` and `.gitattributes` to fix encoding and line-ending rules, preventing Chinese comments from becoming garbled on GitHub or in modern editors.

---

## Power-On Checklist

After the first firmware flash, run at low voltage, with current limiting, and without load. Confirm the following items first:

- DRV8301 `FAULT` is not triggered
- Current zero-offset sampling is completed
- OLED display works normally
- KEY1 can correctly start and stop the motor
- Phase current and DC bus voltage conversion values are reasonable

After confirming the above items, gradually increase the DC bus voltage and load.

</details>
