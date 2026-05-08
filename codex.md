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
- 启动电流：`MOTOR_STARTUP_CURRENT=2.0f`。
- 速度闭环切入：`SPEED_LOOP_CLOSE_RAD_S=50.0f`。
- 电机参数：`RS_PARAMETER=0.59f`，`LS_PARAMETER=0.001f`，`FLUX_PARAMETER=0.01150f`。
- 速度环：`SPEED_PI_P=0.002F`，`SPEED_PI_I=3.0F`，输出限幅 `-5.0F` 到 `8.0F`。
- DRV8301：6-PWM 输入，gate current `0.7 A`，OCP current limit，`OC_ADJ_SET_18`，电流放大倍数 `GAIN_AMP_10`。

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

## 不依赖上位机后的操作方式

- 上电后固件先初始化硬件、OLED、DRV8301、FOC，并进行电流零偏采样。
- 零偏完成后，按 `KEY1 / PC4` 切换电机启停。
- `KEY2 / PC5` 切换 OLED 显示页。
- `KEY3 / PB2` 每次将 `Speed_Ref` 增加 `5 Hz`。
- `motor_start()` 当前默认设置 `Speed_Ref=20.0F`。

## 已知风险和后续任务

- USB 上位机通信虽然默认关闭，但工程仍保留通信库和 USB 源文件；需要重新使用上位机时，再把 `PC_COMMUNICATION_ENABLE` 置 1 并复核 USB 48 MHz 时钟。
- 当前无感 FOC 参数来自原工程，未确认适合用户手头电机；首轮必须关注启动电流、速度闭环切入点、RS/LS/磁链。
- 当前 `KEY3` 只增加速度参考，没有降低速度参考的按键逻辑；实机调参时可能需要补一个减速或复位入口。
- 保护阈值仍沿用原工程 DRV8301 配置；若要跑大功率，先明确 shunt、母线电压、MOSFET、散热和电源限流能力。
- 仓库中包含 Debug/Release/Keil 输出文件，后续整理时可考虑加 `.gitignore`，但不要在没有确认前大规模删除历史文件。

## 协作约定

- 修改硬件相关代码时，优先查 `Netlist_Sheet_2026-05-09.enet` 与 `user/board_config.h` 是否一致。
- 改动 motor control 参数时，同步更新本文件“当前参数快照”和“适配日志”。
- 新增或修改源码注释时使用 UTF-8；不要再用 GBK 保存 `user/`、`motor/` 下的 C/H 文件。
- 每次准备推 GitHub 前，至少运行：
  - `git diff --check`
  - 针对改动 C 文件的 `arm-none-eabi-gcc -fsyntax-only` 语法检查，或在 Keil/IAR 中完整构建。
- 不要默认恢复或删除用户已有工程文件、备份文件、输出文件；清理前先确认范围。
