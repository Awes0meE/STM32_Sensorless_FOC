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
- `KEY3 / PB2`：停机时切换开环标定频点 `30 -> 45 -> 60 Hz`

当前用于 EKF 多频点标定，默认开环保持频点为 `30 Hz`，停机时按 `KEY3` 循环选择 `45 Hz`、`60 Hz`。OLED 的 `ol:` 是当前开环拖动电频率，`cap:` 是本次目标保持频点；底行 `r:` 是命令 q 轴电流，`q:` 是实际 q 轴反馈电流。

## 压缩机安全启动版本

当前代码按 12V 台架低速试机做了第一版安全边界：

- 目标设备：Panasonic `6MD030Z` 小型 24V 无感正弦波压缩机，12V 先低速验证。
- 速度参考范围：`120-200 Hz` 电频率，对 4 对极压缩机约等价于 `1800-3000 rpm` 机械转速；当前诊断版不会自动切入该闭环速度参考。
- 启动电流：参考 GE2117 思路，前 `1s` 使用 `Id=3A/Iq=0A` 定位，之后使用 `Id=3A/Iq=0-3A` 慢拖；`Iq` 约 `8s` 爬到 3A。
- 启动角度：当前使用 `1s` 固定电角度定位，然后按 `cap:` 选择值做 `1-30/45/60 Hz` 正向开环慢拖并保持，斜率 `1.5 Hz/s`；当前诊断版先不让 EKF/速度闭环接管。
- 当前验证：12V 台架下 `30/45/60 Hz` 开环保持、`Id=3A/Iq=3A` 均可稳定运行，`45 Hz` 台架电源约 `1A`，`60 Hz` 约 `1.27A` 且吸力更强；trace 显示电流环误差约 `0.01A`、`Vq` 随频率上升。EKF 参数和方向已完成第一轮校准，但当前固件仍保持开环驱动，不自动闭环接管。
- 母线保护：`9.0-15.5 V`；相电流连续超过 `6A` 会停机。
- 停机/故障后保留 `10s` 重启等待，避免短时间反复启动压缩机。
- DRV8301 硬件过流阈值已从 `OC_ADJ_SET_18` 降到 `OC_ADJ_SET_8`。

OLED 第二页的 `F:` 是压缩机保护码：`1` 欠压，`2` 过压，`3` 过流，`4` 堵转，`5` 重启等待。

## 重要参数

关键参数集中在：

- `user/board_config.h`：PWM、ADC、引脚、通信开关
- `motor/adc.h`：ADC 换算、电流采样、电压采样系数
- `motor/foc_define_parameter.h`：FOC 模式、电机参数、启动电流
- `motor/speed_pid.c`：速度环 PI 参数和输出限幅
- `motor/drv8301.c`：DRV8301 寄存器初始化

当前参数是压缩机 12V 安全启动初值，不应视为最终整定值。换电机、换母线电压或带载前，需要重新低压限流调试。

## 构建与检查

首选用 Keil 打开：

```text
Keil_Project/stm32_drv8301_keil.uvprojx
```

命令行环境可用 `arm-none-eabi-gcc -fsyntax-only` 做轻量语法检查；最终烧录前仍建议用 Keil/IAR 做完整构建。

## VSCode + ST-Link

仓库已经补齐 VSCode 下的 GCC 编译、ST-Link 下载和调试配置，默认使用本机：

```text
D:/ST/STM32CubeCLT_1.21.0
```

如果工具链装在别的位置，构建脚本可通过环境变量 `STM32CUBECLT_PATH` 覆盖；调试配置则需要同步修改 `.vscode/launch.json` 里的 ST 工具路径。

VSCode 建议安装：

- Microsoft C/C++：`ms-vscode.cpptools`
- Cortex-Debug：`marus25.cortex-debug`

常用任务：

- `Build firmware`：生成 `build/stm32_drv8301.elf/.hex/.bin`
- `Probe ST-Link`：枚举 ST-Link 探针
- `Flash firmware (ST-Link)`：编译后用 SWD 下载并复位
- `Erase chip (ST-Link)`：整片擦除
- `Reset target (ST-Link)`：复位目标板
- `Export trace CSV`：从 MCU RAM 导出已冻结的启动黑匣子 CSV

调试入口在 VSCode Run and Debug 里选择 `Debug firmware (ST-Link)`。下载前请确认压缩机电源限流，且不要让压缩机在无人看管状态下启动。

当前固件带 RAM trace 黑匣子：按 `KEY1` 启动时清空并开始记录，故障或停机后冻结。trace 每 `40ms` 采样一次，最多约 `82s`，字段包括 `ol/ekf/r/q/Id/Ia/Ib/Ic/Vbus/Vd/Vq/state/fault`。导出脚本为 `tools/export-trace.ps1`。

当前诊断版已加 `motor_control_ready` 启动门闩，避免 ADC 中断在 EKF 初始化完成前抢跑；OLED 的 `ekf:` 使用有符号 Hz 显示。新 trace 还会记录 `target_hz/foc_theta/ekf_angle/Valpha/Vbeta/Ialpha/Ibeta/ekf_angle_err/ekf_speed_ratio/diag_flags`，用于排查 EKF 符号、初始化分支和开环到 EKF 接管条件。

当前分支 `feature/ekf-handoff-diagnostics` 已进入角度渐变接管验证版：接管候选条件连续满足后，用约 `2s` 把 FOC 角度从开环 `hall_angle` 混合到 EKF 角度；异常时回退开环。OLED 第二页 `ph:` 显示 `EKF angle - open-loop angle` 的电角度误差，单位为度。trace 的 `diag_flags` 新增 bit4/bit5/bit6 表示速度比例达标、角度误差达标、连续 1s 达到接管候选条件；bit7 表示正在角度混合，bit8 表示已使用 EKF 角度，bit9 表示进入回退状态。

当前进一步禁止 EKF 在 1s 定位阶段更新：定位阶段只跑电流环和开环定位，定位结束瞬间重置 EKF，再让 EKF 从开环旋转开始观察。这样可避免静止定位阶段的电压/电流瞬态把 EKF 预先带到错误速度分支。

历史诊断里曾试过 EKF beta 轴输入取反和磁链半值。beta 取反在旧参数下能改变方向但幅值仍约半速；磁链半值让 45Hz EKF 从约 `+21.9Hz` 提高到约 `+27.3Hz`，证明磁链方向判断有意义，但不能单独解释半速问题。

当前烧录的是“确认版参数 + EKF 增益修复 + beta 正常符号”版本：`RS_PARAMETER=0.188f`、`LS_PARAMETER=0.00036f`、`FLUX_PARAMETER=0.00580f`，`EKF_V_BETA_SIGN=+1`、`EKF_I_BETA_SIGN=+1`。依据是 6MD030Z 为 8 极/4 对极，端子相间电阻 `0.376Ω`，模型单相电阻取一半；3A 通电 `Ld/Lq` 约 `0.34/0.38mH`，单电感 EKF 暂取平均。此版还修复了 `stm32_ekf_wrapper.c` 中 Kalman 增益矩阵计算时覆盖原值的问题。

beta 正常符号复测三频点结果：30Hz `EKF≈+29.21Hz`、45Hz `EKF≈+43.06Hz`、60Hz `EKF≈+56.93Hz`，对应 `EKF/ol≈0.974/0.957/0.949`。EKF 方向和速度尺度已基本修复，下一阶段可以设计低风险开环到 EKF 角度/速度的闭环接管。

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
