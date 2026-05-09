# STM32 Sensorless FOC 项目长期记忆

更新时间：2026-05-09

## 当前安全固化版：EC11 30-80Hz 开环可调

- 最新安全决策：用户观察到压缩机在高频开环测试后外壳约 `45-50°C`，且 `OL≈80Hz` 的吸力已经满足当前产品需求；停止继续向 `120Hz+` 开环硬推。
- 当前烧录版本已关闭上电自启动：`COMPRESSOR_AUTO_TEST_ENABLE=0`，复位后不会自动启动压缩机，必须按 `KEY1` 才运行。
- EC11 旋钮范围已锁定为 `450-1200rpm`，对应 8 极 6MD030Z 的 `30-80Hz` 电角频率；默认值为 `1200rpm / 80Hz`，每格 `60rpm / 4Hz`。用户确认旋钮方向反了，当前已设 `COMPRESSOR_EC11_DIRECTION=-1`。
- `COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_RPM` 改为 `COMPRESSOR_EC11_DEFAULT_RPM`，即启动 boost 最高只到 `80Hz`，不再使用 `2000rpm / 133Hz`。
- 高频 V/f 实验分支已关闭：`COMPRESSOR_OPEN_LOOP_VF_ENABLE=0`。当前版本回到电流环开环 FOC 保持，不使用 110Hz 以上的电压模式。
- 软件连续相电流保护恢复到 `COMPRESSOR_PHASE_CURRENT_LIMIT_A=6.0A`，快速相电流保护仍为 `10.0A`；`MOTOR_STARTUP_CURRENT` 和运行 `Iq` 保持 `2.5A`，`Id` 定位/运行保持 `3.0A`。
- 本版本定位为“低速产品演示/旋钮可调安全版”，不是 EKF 闭环接管版；EKF 仍只用于 OLED/trace 观察。

## 最新分支记忆：开环 EC11 压缩机控制

- 当前新增分支：`feature/open-loop-pot-compressor`。
- 分支目标：先不继续推进 EKF 接管，把已经验证稳定的开环 FOC 做成接近商品压缩机驱动板的控制方式。
- EC11 旋钮硬件：A 相 `PB6`，B 相 `PB7`；OLED CS 已由用户确认是 `PC7`，与 PB7 不冲突。
- 速度换算：6MD030Z 是 8 极电机，即 4 对极；机械 `1800rpm` 等于电角频率 `120Hz`，机械 `3000rpm` 等于电角频率 `200Hz`。
- 当前启动逻辑：按 `KEY1` 后 1s 定位，然后开环拉到 EC11 设定目标；当前默认/最高为 `1200rpm / 80Hz`，不再做 `120Hz+` 自动测试或高频 boost。
- EC11 参数：默认 `1200rpm`，范围 `450-1200rpm`，每格 `60rpm`；旋钮方向已按实测改为 `COMPRESSOR_EC11_DIRECTION=-1`。
- 开环斜率：升速 `2Hz/s`，降速 `8Hz/s`。
- 本分支关闭 `EKF_HANDOFF_BLEND_ENABLE`，EKF 只做 OLED/trace 诊断，不参与驱动角度。
- OLED 默认直接显示参数页，不再先显示 logo；`KEY2` 只在参数页/扩展页之间切换。参数页：`rpm:` 当前开环机械转速，`set:` EC11 设定 rpm，`ol:` 当前电角 Hz，`ekf:` EKF Hz，`r/q` 为 q 轴命令和反馈电流。
- OLED 页面切换残影已修：`display_flag` 变化时会复位 `clear_display_flag`，从波形页切回参数页会强制清屏；波形页也会清掉最后一列旧 buffer。
- 已通过 `tools/vscode-build.ps1 build`，并已通过 `tools/vscode-build.ps1 flash` 烧录到 ST-LINK 连接的 STM32F446。
- 测试工况声明：截至当前所有“稳定运行”结论都来自压缩机进气口、出气口均不接管路的开放空载工况；用户手堵吸气口导致的 `F3` 属于人为负载扰动，不等同于带真实制冷系统压差运行。
- EC11 开环首次 fault 诊断：导出的 CSV 为空，因为旧 trace 只在 EKF 接管窗口附近启动；GDB 读到 `fault_code=3`，`Vbus=12.106V`，`Ia=-3.60A`，`Ib=-5.88A`，`Ic=9.48A`，`current_fault_cnt=50`，`drv8301_fault_flag=0`。结论是软件相电流过流，不是 EKF 接管或 DRV8301 硬件 fault。已把产品开环分支 trace 改成从启动开始记录，采样周期 `10ms`。
- EC11 开环第二次 trace：`build/trace_20260509_105216_ec11_start_fault.csv`，903 samples/10ms。故障前最后 800ms：`ol_hz=109.4->121.2Hz`、`avg_vbus=11.123V`、`min_vbus=11.04V`、`avg_iq=2.854A`、`avg_id=3.009A`、非故障帧 `max_abs_phase=4.24A`。故障帧：`fault=3`、`ia=-4.07A`、`ib=-5.47A`、重构 `ic=9.54A`。判断为在开环约 `121Hz` 电角频率、机械约 `1818rpm` 附近出现软件相电流过流；不建议直接抬高过流阈值，优先降低 boost 或做分段限流/降 Id。
- 历史自动测试结论：曾短暂启用 `COMPRESSOR_AUTO_TEST_ENABLE=1` 做 1800-2000rpm 上探；1800rpm/120Hz 需要保留足够 d 轴吸附电流，2000rpm/133Hz 一带会明显发热且控制裕量不足。该自动测试现已关闭，不应在产品低速版恢复。
- 最新烧录版：低速 EC11 产品演示安全版，`30-80Hz` 可调，关闭上电 logo与自动测试。上一轮 120Hz 堵吸口触发 `fault=3` 说明软件相电流保护有效，但不作为当前低速版目标工况。

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
- 压缩机安全启动：`COMPRESSOR_SAFE_START_ENABLE=1`，12V 台架开环保持诊断版。
- 速度参考范围：`30-80 Hz` 电频率；对 4 对极 6MD030Z 约等价于 `450-1200 rpm` 机械转速。当前安全版不再自动上探 `120Hz+`。
- 启动电流：先用 `COMPRESSOR_STARTUP_ID_CURRENT=3.0f` 做 1s d 轴定位，定位结束后 `Id=3A`、`Iq=0-2.5A` 正向开环慢拖；`Iq` 约 `6.7s` 爬到 2.5A。
- 开环强拖：`COMPRESSOR_FORCE_START_ENABLE=1`，`COMPRESSOR_OPEN_LOOP_HOLD_ENABLE=1`，启动阶段先 `1s` 固定电角度 d 轴定位，再用 `1-30..80 Hz` 开环角度斜坡，升速 `2Hz/s`、降速 `8Hz/s`，到 EC11 设定值后保持开环；`COMPRESSOR_OPEN_LOOP_DIRECTION=1.0f`。反向 `-1.0f` 已实测更差。
- OLED 诊断：参数页 `rpm:` 为当前开环机械转速，`set:` 为 EC11 设定转速，`ol:` 为当前开环电频率；底行 `r:` 为命令 q 轴电流 `FOC_Input.Iq_ref`，`q:` 为实际 q 轴反馈电流 `Current_Idq.Iq`；不要再把命令值当作真实相电流。
- RAM trace：`motor/trace.c/h` 当前为接管窗口密集记录模式；按 `KEY1` 只清空并 armed，等 `target_ok` 或 handoff/fallback 出现后才真正开始写入。采样周期 `20ms`，1792 条约 `35.84s`，故障/停机后自动冻结。导出用 `tools/export-trace.ps1` 或 VSCode 任务 `Export trace CSV`。当前 trace record 为 64 字节，包含 `ekf_angle_err_rad`、`ekf_speed_ratio`、`handoff_blend`、`trip_blend`、`trip_angle_err_rad`、`trip_speed_ratio`、`trip_reason`、`handoff_state`、`handoff_offset_rad`。`handoff_state=4` 是接管前降电流阶段，此时 `handoff_blend` 表示电流衰减进度；`handoff_state=1` 才表示 EKF 角度混合进度。
- 速度闭环切入：`SPEED_LOOP_CLOSE_RAD_S=120.0f`。
- 保护阈值：母线 `9.0-15.5 V`，相电流连续超过 `6.0 A` 停机，故障/停机后 `10s` 重启等待。
- 故障码：`1` 欠压，`2` 过压，`3` 过流，`4` 堵转，`5` 重启等待。
- 电机参数：`RS_PARAMETER=0.188f`，`LS_PARAMETER=0.00036f`，`FLUX_PARAMETER=0.00580f`。依据用户重新确认的 6MD030Z 参数：8 极/4 对极；端子相间电阻 U-V/V-W/W-U 为 `0.376Ω`，EKF 相模型取单相约 `0.188Ω`；3A 通电电感 `Lq≈0.38mH`、`Ld≈0.34mH`，当前单电感模型取平均约 `0.36mH`；`flux` 按 `Ke=2.96 V/kRPM` 与 `Kt=0.049 N·m/A` 尺度取约 `0.0058Wb`。
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
- 已新增“压缩机安全启动版本”：当前切到 EC11 低速开环保持档，默认/最高运行目标为 `80Hz` 电频率；启动/运行先 `Id=3A/Iq=0A` 定位，再 `Id=3A/Iq=0-2.5A` 拖动，12V 母线窗口、相电流软件保护、150s 启动检查、3s 堵转确认、10s 重启等待。
- 2026-05-09 中间验证：加入 `2-18 Hz` 开环强拖启动后，12V 下压缩机曾可低速 `C:run`，但重复启动成功率低；后续用 RAM trace 证明电流环能跟随，真正问题在 EKF/速度闭环接管。
- 2026-05-09 关键基线：45Hz 开环保持、`Id=3A/Iq=3A`、12V 台架电源约 1A 时压缩机稳定运行且吸力强；trace 显示 `Vbus≈11.53-11.62V`、`Vq≈2.38-2.40V`、`Iq` 误差约 0.01A。第一次稳定导出里 EKF 约 `-21Hz`，后续三次 repeat 导出里 EKF 稳定为正且约 `36.7-36.8Hz`；完整断电重启后也复现 `+36.8Hz`。多频点标定版中 30/45/60Hz 又全部表现为负向 EKF：`-16.08/-21.93/-28.33Hz`。当时判断为 EKF 输入符号/初始化/输出符号不可信，未完成校准前禁止自动闭环接管。
- 已将 6MD030Z 确认版 EKF 参数写入 `motor/foc_define_parameter.h`：`Rs=0.188Ω`、`Ls=0.36mH`、`flux=0.00580Wb`。注意当前 `stm32_ekf_wrapper.c` 只使用 `Rs/Ls/flux`，没有使用极对数或转动惯量；极对数主要用于电角速度和机械 rpm 的换算。
- 已把 DRV8301 硬件过流阈值从 `OC_ADJ_SET_18` 降为 `OC_ADJ_SET_8`，更适合 12V 小压缩机安全试机。
- 已打通 VSCode + STM32CubeCLT + ST-Link 链路：`.vscode/tasks.json`、`.vscode/launch.json`、`.vscode/c_cpp_properties.json`、`tools/vscode-build.ps1`、`tools/export-trace.ps1`。
- 2026-05-09 多频点 EKF 标定版曾用于 `KEY3` 循环 `cap=30/45/60Hz`。当前产品演示安全版改为 EC11 旋钮设定 `30-80Hz`，`KEY3` 频点循环不作为主要控制入口。
- 2026-05-09 EKF 初始化时序诊断补丁已烧录：新增 `motor_control_ready` 门闩，防止 KEY1 启动后 ADC JEOC 在 `foc_algorithm_initialize()` 完成前抢跑 `motor_run()/EKF`；OLED EKF 改为 signed Hz；trace record 扩展到 48 字节并记录 `target_hz/foc_theta/ekf_angle/Valpha/Vbeta/Ialpha/Ibeta/diag_flags`。
- ready 门闩后 30Hz 复测仍为负 EKF：定位后 OLED 显示 `-8Hz`，稳定后 `-16Hz`，trace 稳定段 `EKF=-16.16Hz`、`EKF/ol=-0.539`、`diag_flags=5`。说明 OLED signed 修正有效，但负分支不只是初始化抢跑造成；trace 采样已改为 40ms 以覆盖约 82s。
- 45Hz 完整 trace 抓到定位阶段 EKF 假速度：在 0-1000ms 定位阶段 `theta=0`、`Iq_ref=0`、`Id≈3A`，EKF 从 0 跑到约 `-8.7Hz`。已烧录补丁：定位阶段 `foc_ekf_update_enable=0` 跳过 EKF 更新，定位结束瞬间 `foc_ekf_reset()`，然后再开启 EKF 更新。下一轮重点看定位阶段 EKF 是否保持 0，以及定位后是否仍走负分支。
- 暂停定位阶段 EKF 后，45Hz 复测显示 0-1000ms EKF 保持 0，但定位结束后 40-80ms 内仍跳到 `-8Hz`，稳定段约 `-22Hz`。这说明负分支发生在开环刚开始，而不是定位阶段污染。已烧录 beta 轴输入取反诊断版：只对送入 EKF 的 `Vbeta/Ibeta` 取反，FOC/SVPWM/电流环实际驱动不变。
- beta 轴输入取反诊断结果：45Hz 稳定段 EKF 从取反前 `-22Hz` 翻成 `+21.9Hz`，且 `Vq/Id/Iq` 与取反前一致，确认方向问题来自 EKF αβ 坐标手性/相序定义。幅值仍只有 `ol` 的约 `0.487`，下一步复测 30/60Hz 后再查 EKF 速度尺度、单位、模型参数或输出解释。EKF 角度 trace 当前会饱和到 `-32.768`，后续需归一化角度记录。
- beta 轴输入取反 30Hz 复测：稳定段 `EKF=+16.21Hz`、`EKF/ol=+0.540`、`Vq≈1.79V`、`Id/Iq≈3A`。方向修正进一步确认；还需测 60Hz。如果 30/45/60Hz 都约为 0.5，则优先查 EKF 速度尺度/单位或模型参数。
- beta 轴输入取反 60Hz 复测：稳定段 `EKF=+28.22Hz`、`EKF/ol=+0.470`、`Vq≈2.97V`、`Id/Iq≈3A`。三频点为 30Hz `0.540`、45Hz `0.487`、60Hz `0.470`，方向已修正，幅值约半。相间电阻测量要除以 2 才接近模型单相 `Rs`；但半速更像 `FLUX_PARAMETER=0.00650Wb` 偏大或 EKF 速度尺度问题。下一步临时试 `FLUX_PARAMETER=0.00325Wb` 只影响 EKF。
- EKF 磁链半值诊断版结果：`FLUX_PARAMETER=0.00325Wb`、beta 轴取反保持启用时，45Hz 稳定段 `EKF=+27.29Hz`、`EKF/ol=0.606`，相比原 `flux=0.00650Wb` 的 `+21.90Hz` 明显上升，但仍未接近 45Hz。结论是磁链方向判断正确，但半速问题不是单独由 `flux` 大 2 倍造成。
- 用户重新确认 `0.376Ω` 为端子相间电阻后，`Rs` 半值方向已执行；没有继续做单变量 `Rs=0.188/flux=0.00650`，而是结合确认版 `Ld/Lq/Ke/Kt` 一起改成当前物理参数，并同步修 EKF 增益 bug。
- 2026-05-09 已根据用户重新确认的扫描版手册参数改成“确认版参数 + EKF 增益修复”版本：`Rs=0.188Ω`、`Ls=0.36mH`、`flux=0.00580Wb`；并修复 `stm32_ekf_wrapper.c` 中 Kalman 增益矩阵计算覆盖原值的问题。原代码在计算 `K = P H^T S^-1` 时先覆盖 `K_x_0`，随后计算 `K_x_1` 又使用了覆盖后的 `K_x_0`，会使 EKF 更新矩阵偏斜。
- 确认版参数 + EKF 增益修复 + beta 取反复测 45Hz：稳定段 `EKF=-43.11Hz`、`EKF/ol=-0.958`、`Iq/Id≈3.00A`、`Vbus≈11.59V`、`Vq≈2.38V`。幅值已经基本对上，方向为负，说明半速问题基本解决，且 beta 轴取反不再适用。当前已烧录 beta 正常符号版：`EKF_V_BETA_SIGN=+1`、`EKF_I_BETA_SIGN=+1`，下一轮优先复测 45Hz，预期 EKF 接近 `+43Hz` 到 `+45Hz`。
- beta 正常符号 + 确认版参数 + EKF 增益修复三频点结果：30Hz 稳定段 `EKF=+29.21Hz`、`EKF/ol=0.974`；45Hz 稳定段 `EKF=+43.06Hz`、`EKF/ol=0.957`；60Hz 稳定段 `EKF=+56.93Hz`、`EKF/ol=0.949`。`Iq/Id≈3A` 均稳定跟随，无 fault。结论：EKF 方向与速度尺度已基本修复，可进入“谨慎闭环接管”阶段，但应先加接管条件、角度误差监控和可回退逻辑。
- 历史上曾从 `main` 切出 `feature/ekf-handoff-diagnostics` 分支并烧录接管前诊断版。此版实际 FOC 驱动角度仍为开环 `hall_angle`，不进行闭环接管；新增 `ekf_angle_error_rad = EKF angle - FOC_Input.theta`、`ekf_speed_ratio = EKF_Hz / open_loop_hz`、`ekf_handoff_speed_ok`、`ekf_handoff_angle_ok`、`ekf_handoff_ready`。接管候选条件为开环不低于 25Hz、速度比例 `0.90-1.10`、角度误差绝对值不超过 `0.70rad`，连续 10000 个 10kHz FOC tick 约 1s 后置 ready。
- 接管诊断版 OLED 第二页把原 `F:` 位置改成 `ph:`，显示 EKF 与开环角度差的电角度度数；故障状态仍由顶部 `C:fault` 和保护状态机判断。trace `diag_flags` 新增 bit4 速度 OK、bit5 角度 OK、bit6 handoff ready。
- 接管诊断版三频点结果：30Hz `speed_ratio=0.973`、相位误差均值约 `10.7°`；45Hz `speed_ratio=0.957`、相位误差均值约 `7.9°`；60Hz `speed_ratio=0.950`、相位误差均值约 `5.0°`。三点稳定段 `diag_flags=125`，即速度 OK、角度 OK、handoff ready 均成立。下一步可以做“角度渐变混合接管版”，不要硬切。
- 历史 `feature/ekf-handoff-diagnostics` 曾烧录角度渐变接管版：满足 handoff ready 后，`ekf_handoff_blend` 用约 `2s` 从 `0` 增至 `1`，FOC 角度按最短电角度差从开环 `hall_angle` 混到 EKF 角度；`FOC_Input.speed_fdk` 同步按 blend 在开环速度和 EKF 速度间插值。若 blending/full EKF 阶段速度比例或相位误差失效，则 `ekf_handoff_state=FALLBACK` 并回到开环角度。trace `diag_flags` 新增 bit7 blending、bit8 full EKF angle、bit9 fallback。
- 2026-05-09 已确认 `stm32_ekf_wrapper.c` 不是缺失支持文件：`foc_algorithm_step()` 每个 FOC tick 会调用 `stm32_ekf_Outputs_wrapper()` 和 `stm32_ekf_Update_wrapper()`，输入 `Valpha/Vbeta/Ialpha/Ibeta/Rs/Ls/flux`，输出 `FOC_Output.EKF[2]` 速度和 `FOC_Output.EKF[3]` 角度。30/45/60Hz 标定里 EKF 速度已能跟 OL 接近，说明 wrapper 正在参与运算；真正风险在开环强拖电流矢量切到 EKF 坐标系时的接管策略。
- 2026-05-09 当前接管策略：先用严格 ready 门槛确认 EKF 与开环同步，再进入 `EKF_HANDOFF_STATE_DECAY=4`。DECAY 阶段仍使用开环角度，但把 `Id_ref` 用约 `8s` 从 `3A` 降到 `0A`，把 `Iq_ref` 从 `3A` 轻降到 `2.5A`；DECAY 完成后改用专用接管门槛，确认低 d 轴电流下 EKF 仍稳定，再锁存 `handoff_offset_rad = open_loop_angle - EKF_angle` 并进入 BLEND。BLEND 阶段用约 `8s` 将角度从 `EKF_angle + offset` 衰减到纯 EKF 角度，`speed_fdk` 仍保持开环速度；进入 EKF 状态后才使用 EKF speed。
- 最新软化参数：接管前 ready 用严格 `speed_ratio=0.90-1.10`、角度误差 `<=0.70rad`、连续约 `1s`；DECAY `EKF_HANDOFF_DECAY_TICKS=80000`；DECAY 完成后的接管门槛为 `speed_ratio=0.94-1.06`、角度误差 `<=0.90rad`、连续约 `1s`；BLEND `EKF_HANDOFF_BLEND_TICKS=80000`；接管运行中速度健康窗口 `0.75-1.25` 且连续约 `100ms` 不健康才 fallback。fallback 后保持开环角度和低 d 轴电流，避免回退瞬间又把 3A d 轴定位电流打回去。
- DECAY 后专用门槛版 45Hz 结果：能从 `DECAY(4)` 进入 `BLEND(1)`，但 BLEND 约 `1.24s` 后在 `trip_blend=0.155` 回退到 `FALLBACK(3)`；锁存 `trip_speed_ratio=1.267`、`trip_angle_err=-2.508rad`、`trip_reason=24`。BLEND 前电流和 Vq 正常，问题是 EKF 角度参与后观测速度被带高。下一步不要继续放宽保护；应改成更慢/更小步的相位校正，或先做“速度观测锁定/PLL式 theta correction”，避免直接按 EKF 角度混合。
- VSCode 构建默认使用 `D:\ST\STM32CubeCLT_1.21.0`，也可通过环境变量 `STM32CUBECLT_PATH` 覆盖构建脚本路径；调试配置路径在 `.vscode/launch.json` 中维护。
- GCC 构建产物位于 `build/stm32_drv8301.elf/.hex/.bin`，`build/` 已由 `.gitignore` 忽略。
- 为了让 PC 通信关闭时能 GCC 链接，`user/stm32f4xx_it.*` 中的 USB OTG handler 已按 `PC_COMMUNICATION_ENABLE` 包裹；GCC 专用 newlib stub 位于 `tools/gcc_syscalls.c`。

## 不依赖上位机后的操作方式

- 上电后固件先初始化硬件、OLED、DRV8301、FOC，并进行电流零偏采样。
- 零偏完成后，按 `KEY1 / PC4` 切换电机启停。
- `KEY2 / PC5` 切换 OLED 显示页。
- `KEY3 / PB2` 在停机时切换开环标定频点：`30 -> 45 -> 60 Hz` 循环；运行中按 KEY3 不改变频点，避免扰动压缩机。
- `motor_start()` 当前会先 `1s` 定位，然后按 EC11 设定做 `1-30..80Hz` 开环慢拖并保持。OLED 参数页 `ol:` 是实际开环电频率，`rpm:`/`set:` 是当前/设定机械转速，`r:`/`q:` 是命令/反馈 q 轴电流。

## 已知风险和后续任务

- USB 上位机通信虽然默认关闭，但工程仍保留通信库和 USB 源文件；需要重新使用上位机时，再把 `PC_COMMUNICATION_ENABLE` 置 1 并复核 USB 48 MHz 时钟。
- 当前 6MD030Z 参数是根据用户确认的手册参数和实测 trace 整定得到的第一轮 EKF 启动参数；30/45/60Hz 开环保持已能稳定运行，EKF 速度尺度约为开环的 `0.974/0.957/0.949`，但还不是最终产品参数。
- EKF 半速和方向问题已基本修复：已确认 beta 正常符号正确，修复了 EKF 增益矩阵覆盖 bug，并把 `Rs/Ls/flux` 调整到 6MD030Z 确认参数。当前仍禁止自动闭环接管；下一步应先记录/显示 EKF 角度与开环角度差，再做可回退的角度渐变混合。
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
