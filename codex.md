# STM32 Sensorless FOC 项目长期记忆

更新时间：2026-05-09

## 项目身份

- 仓库：`https://github.com/Awes0meE/STM32_Sensorless_FOC`
- 本地路径：`d:\XJTLU\科研_DIY\FOC_Project\STM32F4_DRV8301_code`
- 目标：基于 `STM32F446RET6 + DRV8301` 跑无感 FOC。
- 固件风格：STM32F4 StdPeriph 标准外设库，不是 HAL。
- 主要工程：`Keil_Project/stm32_drv8301_keil.uvprojx`；根目录也有 IAR 工程文件。

## 长期声明

- 默认优先适配用户手头这套 PCB，而不是原作者上位机/开发板环境。
- 用户当前没有 USB 上位机通信代码和软件；固件应能在不依赖 PC 上位机的情况下启动、调试和保护。
- 后续写代码时遵循已安装的 `karpathy-guidelines` skill：少改、局部改、能验证，不做无关重构。
- 上电调试必须先低压限流、空载验证，不能直接大电流带载试车。

## 当前硬件事实

硬件资料位于：

- 原理图：`d:\XJTLU\科研_DIY\FOC_Project\EDA导出文件\电路原理图\SCH_Sheet_2026-05-09.pdf`
- BOM：`d:\XJTLU\科研_DIY\FOC_Project\EDA导出文件\BOM_DRV8301_1_Sheet_2026-05-09.xlsx`
- 网表：`d:\XJTLU\科研_DIY\FOC_Project\EDA导出文件\Netlist_Sheet_2026-05-09.enet`

关键匹配结论：

- MCU 是 `STM32F446RET6`，与工程宏 `STM32F446xx` 匹配。
- 驱动芯片是 `DRV8301DCAR`，与代码 `motor/drv8301.*` 匹配。
- 板载晶振是 8 MHz。STM32F446 标准库默认 HSE 8 MHz，可得到 180 MHz HCLK。
- 相电流采样使用 DRV8301 内置放大器，A/B 两相 shunt 为 `20 mΩ`，代码 `SAMPLE_RES=0.020`、`AMP_GAIN=10` 与硬件匹配。
- 母线电压分压为 `100k / 4.99k`，代码已同步为 `VBUS_UP_RES=100.0f`、`VBUS_DOWN_RES=4.99f`。

## 关键引脚映射

| 功能 | MCU 引脚 | 网名/用途 |
| --- | --- | --- |
| PWM AH/BH/CH | PA8 / PA9 / PA10 | `PWM_AH` / `PWM_BH` / `PWM_CH` |
| PWM AL/BL/CL | PB13 / PB14 / PB15 | `PWM_AL` / `PWM_BL` / `PWM_CL` |
| A/B 相电流 | PC1 / PC2 | `ADC_IAI` / `ADC_IBI` |
| 母线电流 | PA6 | `I_BUS` |
| 母线电压 | PC0 | `VBUS_AD` |
| U/V/W 相电压 | PB1 / PB0 / PA7 | `VU_AD` / `VV_AD` / `VW_AD` |
| 温度 | PC3 | `TEMP` |
| Hall A/B/C | PA0 / PA1 / PB10 | `A_H1` / `B_H2` / `C_H3` |
| DRV8301 EN_GATE | PB9 | `EN_GATE` |
| DRV8301 FAULT/OCTW | PD2 / PB4 | `FAULT` / `OCTW` |
| DRV8301 bit-bang SPI | PA15 / PC10 / PC11 / PC12 | NSS / SCK / MISO / MOSI |
| OLED SH1106 | PC13 / PC8 / PC7 / PB3 / PB5 | DC / RES / CS / SCK / MOSI |
| USB | PA11 / PA12 / PB12 | DM / DP / `USB_C` |
| 按键 | PC4 / PC5 / PB2 | KEY1 / KEY2 / KEY3 |

## 当前参数快照

- PWM：`TIM1`，`PWM_TIM_FREQ=10000 Hz`，`FOC_PERIOD=0.0001F`。
- 死区：`DEAD_TIME=200 ns`。
- FOC 模式：`SENSORLESS_FOC_SELECT` 已启用，`HALL_FOC_SELECT` 已注释。
- 压缩机安全启动：`COMPRESSOR_SAFE_START_ENABLE=1`，12V 台架低速版本。
- 速度范围：`30-50 Hz` 电频率；对 4 对极 6MD030Z 约等价于 `450-750 rpm` 机械转速，`KEY3` 以 `5 Hz` 步进循环。
- 启动电流：`MOTOR_STARTUP_CURRENT=3.0f`，当前回退到已验证能真实起转的慢速爬升曲线。
- 开环强拖：`COMPRESSOR_FORCE_START_ENABLE=1`，EKF 未锁定前使用 `2-18 Hz` 开环角度斜坡，斜率 `3 Hz/s`。
- 速度闭环切入：`SPEED_LOOP_CLOSE_RAD_S=120.0f`。
- 保护阈值：母线 `9.0-15.5 V`，相电流连续超过 `6.0 A` 停机，故障/停机后 `10s` 重启等待。
- 故障码：`1` 欠压，`2` 过压，`3` 过流，`4` 堵转，`5` 重启等待。
- 电机参数：`RS_PARAMETER=0.376f`，`LS_PARAMETER=0.00020f`，`FLUX_PARAMETER=0.00650f`，来自 6MD030Z 手册和 Ke/Kt 估算，仍需实测整定。
- 速度环：`SPEED_PI_P=0.002F`，`SPEED_PI_I=1.0F`，输出限幅约 `-2.0F` 到 `4.0F`。
- DRV8301：6-PWM 输入，gate current `0.7 A`，OCP current limit，`OC_ADJ_SET_8`，电流放大倍数 `GAIN_AMP_10`。

这些参数不应视为最终电机参数；换电机或母线电压后必须重新低风险整定。

## 2026-05-09 适配日志

- Git 远端已同步为 `origin=https://github.com/Awes0meE/STM32_Sensorless_FOC.git`。
- 当前代码确认是 StdPeriph 标准外设库，不是 HAL。
- 已新增 `PC_COMMUNICATION_ENABLE=0`，默认关闭 USB/PC 上位机通信路径。
- 已在 `main.c`、`adc.c`、`low_task.c`、`board_config.c` 中用 `PC_COMMUNICATION_ENABLE` 包住 PC 通信初始化、周期通信任务和 USB OTG 中断配置。
- 已补齐 OLED 片选 `PC7 / SH1106_CS` 的宏定义和 GPIO 初始化。
- 已按硬件把 `VBUS_UP_RES` 从 `95.3f` 修正为 `100.0f`。
- 已用 `arm-none-eabi-gcc -fsyntax-only` 对改过的 C 文件做语法检查，通过。
- 已执行 `git diff --check`，通过；仅有 `adc.h` 和 `board_config.h` 的 CRLF 提示。
- 未做完整 Keil/IAR 构建，因为命令行环境未发现 `UV4/armcc/iarbuild`。
- 已新增根目录 `README.md` 作为 GitHub/新人入口；长期细节仍以本文件为准。
- 已将 `user/` 和 `motor/` 中原 GBK/CP1252 编码的源码注释维护为 UTF-8，并新增 `.editorconfig` 声明后续源码按 UTF-8 编辑。
- 已新增“压缩机安全启动版本”：当前为 12V 低速诊断档，默认 30 Hz 电频率、最高 50 Hz 电频率，启动 Iq 慢速爬升到 3.0A，12V 母线窗口、相电流软件保护、20s 启动检查、2s 堵转确认、10s 重启等待。
- 2026-05-09 实机验证：加入 `2-18 Hz` 开环强拖启动后，12V 下压缩机可稳定 `C:run`，EKF 从 0 爬升到 `12-13`，Iq 稳定约 `1.49A`，台架电源约 `220mA`，OCTW/FAULT 不亮，吸气口有可感知吸力。
- 已将 6MD030Z 初始 FOC 参数写入 `motor/foc_define_parameter.h`：`Rs=0.376Ω`、`Ls=0.20mH`、`flux=0.00650Wb`，这些值是启动初值，不是最终标定。
- 已把 DRV8301 硬件过流阈值从 `OC_ADJ_SET_18` 降为 `OC_ADJ_SET_8`，更适合 12V 小压缩机安全试机。
- 已打通 VSCode + STM32CubeCLT + ST-Link 链路：`.vscode/tasks.json`、`.vscode/launch.json`、`.vscode/c_cpp_properties.json`、`tools/vscode-build.ps1`。
- VSCode 构建默认使用 `D:\ST\STM32CubeCLT_1.21.0`，也可通过环境变量 `STM32CUBECLT_PATH` 覆盖构建脚本路径；调试配置路径在 `.vscode/launch.json` 中维护。
- GCC 构建产物位于 `build/stm32_drv8301.elf/.hex/.bin`，`build/` 已由 `.gitignore` 忽略。
- 为了让 PC 通信关闭时能 GCC 链接，`user/stm32f4xx_it.*` 中的 USB OTG handler 已按 `PC_COMMUNICATION_ENABLE` 包裹；GCC 专用 newlib stub 位于 `tools/gcc_syscalls.c`。

## 不依赖上位机后的操作方式

- 上电后固件先初始化硬件、OLED、DRV8301、FOC，并进行电流零偏采样。
- 零偏完成后，按 `KEY1 / PC4` 切换电机启停。
- `KEY2 / PC5` 切换 OLED 显示页。
- `KEY3 / PB2` 将目标速度在 `30/35/40/45/50 Hz` 电频率间循环。
- `motor_start()` 当前默认从 `30 Hz` 电频率目标启动，不再使用原工程的 `20.0F`。OLED 的 `tgt:1800` 是旧显示口径，不代表真实机械 1800 rpm。

## 已知风险和后续任务

- USB 上位机通信虽然默认关闭，但工程仍保留通信库和 USB 源文件；需要重新使用上位机时，再把 `PC_COMMUNICATION_ENABLE` 置 1 并复核 USB 48 MHz 时钟。
- 当前 6MD030Z 参数是根据手册估算的启动初值；实机首轮必须记录空载/轻载电流、启动时间、EKF 机械 rpm 是否稳定、母线跌落。
- 12V 下无感低速启动仍可能因为反电动势太小而失败；如果 450-750 rpm 诊断档运行不稳，下一步优先做开环对齐/强拖再切 EKF。
- 保护阈值现在适合 12V 小压缩机试机；若要跑 24V 或更大功率，先明确 shunt、母线电压、MOSFET、散热和电源限流能力，并重新整定阈值。
- 仓库中包含 Debug/Release/Keil 输出文件，后续整理时可考虑加 `.gitignore`，但不要在没有确认前大规模删除历史文件。

## 协作约定

- 修改硬件相关代码时，优先查 `Netlist_Sheet_2026-05-09.enet` 与 `user/board_config.h` 是否一致。
- 改动 motor control 参数时，同步更新本文件“当前参数快照”和“适配日志”。
- 新增或修改源码注释时使用 UTF-8；不要再用 GBK 保存 `user/`、`motor/` 下的 C/H 文件。
- 每次准备推 GitHub 前，至少运行：
  - `git diff --check`
  - 针对改动 C 文件的 `arm-none-eabi-gcc -fsyntax-only` 语法检查，或在 Keil/IAR 中完整构建。
- VSCode/GCC 链路改动后运行 `powershell -NoProfile -ExecutionPolicy Bypass -File tools/vscode-build.ps1 build`。
- 不要默认恢复或删除用户已有工程文件、备份文件、输出文件；清理前先确认范围。
