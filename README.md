# STM32 Sensorless FOC

基于 `STM32F446RET6 + DRV8301` 的无感 FOC 固件工程。当前适配目标是用户手头的自制 PCB，而不是依赖原工程的 USB 上位机调试环境。

## 工程状态

- MCU：`STM32F446RET6`
- Gate driver：`DRV8301DCAR`
- 固件库：STM32F4 StdPeriph 标准外设库，不是 HAL
- 主工程：`Keil_Project/stm32_drv8301_keil.uvprojx`
- 兼容工程：根目录保留 IAR 工程文件
- 当前默认：`SENSORLESS_FOC_SELECT`，USB/PC 上位机通信关闭

长期项目记忆、硬件映射、参数快照和调试日志见 [codex.md](codex.md)。

## 当前硬件适配

已按 2026-05-09 的 PCB 网表核对并适配：

- TIM1 六路互补 PWM：PA8/PA9/PA10 与 PB13/PB14/PB15
- 两相电流采样：PC1/PC2
- 母线电流：PA6
- 母线电压：PC0，分压系数 `100k / 4.99k`
- DRV8301 bit-bang SPI：PA15/PC10/PC11/PC12
- OLED SH1106：PB3/PB5/PC13/PC8/PC7
- 按键：PC4/PC5/PB2

## 不依赖上位机的操作方式

当前 `PC_COMMUNICATION_ENABLE` 默认为 `0`，因此不需要 USB 上位机软件。

- `KEY1 / PC4`：电机启停
- `KEY2 / PC5`：切换 OLED 显示
- `KEY3 / PB2`：速度参考增加 `5 Hz`

默认启动速度参考在 `motor/low_task.c` 的 `motor_start()` 中设置为 `20.0F`。

## 重要参数

关键参数集中在：

- `user/board_config.h`：PWM、ADC、引脚、通信开关
- `motor/adc.h`：ADC 换算、电流采样、电压采样系数
- `motor/foc_define_parameter.h`：FOC 模式、电机参数、启动电流
- `motor/speed_pid.c`：速度环 PI 参数和输出限幅
- `motor/drv8301.c`：DRV8301 寄存器初始化

当前参数来自原工程和本次硬件核对，不应视为最终电机整定值。换电机、换母线电压或带载前，需要重新低压限流调试。

## 构建与检查

首选用 Keil 打开：

```text
Keil_Project/stm32_drv8301_keil.uvprojx
```

命令行环境可用 `arm-none-eabi-gcc -fsyntax-only` 做轻量语法检查；最终烧录前仍建议用 Keil/IAR 做完整构建。

## 编码约定

源码和文档统一使用 UTF-8。仓库已用 `.editorconfig` 和 `.gitattributes` 固定编码与换行规则，避免中文注释在 GitHub 或现代编辑器中显示为乱码。

## 上电提醒

首次烧录后请低压、限流、空载运行，先确认：

- DRV8301 `FAULT` 未触发
- 电流零偏采样完成
- OLED 显示正常
- KEY1 能正确启停
- 相电流、母线电压换算值合理

确认以上项目后，再逐步提高母线电压和负载。
