# STM32 Sensorless FOC 压缩机驱动调试日志

本文档记录我从零开始把 `STM32F446RET6 + DRV8301` 无感 FOC 固件移植到自制压缩机驱动板上的过程。写法尽量保留实验报告和项目博客的口吻，重点记录我做了什么、测到了什么、基于这些结果做了什么判断，以及下一步为什么这样走。

## 项目背景

时间：2026-05-09

项目目标：用一套自制 `STM32F446RET6 + DRV8301` 三相逆变硬件，实现对 Panasonic `6MD030Z` 小型 24V 无感 BLDC 压缩机的安全启动和后续 FOC 驱动。

我手头的情况：

- 代码来自一个已有的 STM32 DRV8301 无感 FOC 工程。
- 原工程依赖 USB 上位机调试，但我没有上位机通信代码，也没有上位机软件。
- 硬件是我自己的 PCB，有原理图、BOM 和网表。
- 我希望先做一个“压缩机安全启动版本”，从 12V 台架电源开始低风险验证。
- 后续目标是逐步闭环调试，最终适配成更接近产品级的压缩机驱动板。

## 0. 仓库和工程接管

我把原来位于另一个路径的工程移动到了当前路径：

```text
d:\XJTLU\科研_DIY\FOC_Project\STM32F4_DRV8301_code
```

然后同步 Git 远程仓库：

```text
https://github.com/Awes0meE/STM32_Sensorless_FOC
```

初步阅读代码后确认：

- 这是一个 STM32F4 标准外设库工程，不是 HAL 工程。
- 主控目标是 `STM32F446` 系列。
- 工程里包含 Keil/IAR 相关文件，后续又补了 VSCode + GCC + ST-Link 的链路。
- FOC 主体逻辑位于 `motor/`。
- 板级初始化、GPIO、ADC、TIM、按键、OLED 等位于 `user/`。
- 无感估算核心使用了 `stm32_ekf_wrapper.c`，这是扩展卡尔曼滤波观测器相关代码。

这个阶段的决策：

- 后续适配优先服务我手头这块 PCB，而不是原工程作者的开发环境。
- 因为没有 USB 上位机，所以固件必须能脱离上位机独立启动、显示状态、接受按键控制并做保护。
- 长期记忆、调试记录和参数说明放在仓库文档里，避免后面调通后忘记前面的过程。

## 1. 硬件资料核对

我提供了以下硬件资料：

```text
d:\XJTLU\科研_DIY\FOC_Project\EDA导出文件\电路原理图\SCH_Sheet_2026-05-09.pdf
d:\XJTLU\科研_DIY\FOC_Project\EDA导出文件\BOM_DRV8301_1_Sheet_2026-05-09.xlsx
d:\XJTLU\科研_DIY\FOC_Project\EDA导出文件\Netlist_Sheet_2026-05-09.enet
```

根据网表和原理图，确认硬件关键点：

- MCU：`STM32F446RET6`
- Driver：`DRV8301DCAR`
- 三相逆变：外置 MOSFET
- 相电流采样：DRV8301 内部电流采样放大器，A/B 两相 shunt
- 相电流 shunt：约 `20 mΩ`
- DRV8301 电流采样放大倍数：代码中使用 `10x`
- 母线电压采样：`100k / 4.99k` 分压
- OLED：SH1106
- 按键：`KEY1/KEY2/KEY3`
- DRV 故障信号：`OCTW` 和 `FAULT`

这个阶段的决策：

- 保留 DRV8301 架构，直接适配当前 PCB 的引脚和采样比例。
- 先关闭 USB/PC 上位机通信路径。
- 通过 OLED 和按键完成最基本的人机交互。
- 首次上电必须低压限流，不能直接带压缩机大电流启动。

## 2. 代码适配和工程整理

已经完成的主要代码和工程改动：

- 新增 `codex.md` 作为项目长期记忆。
- 修正板级引脚映射，使代码匹配当前 PCB。
- 关闭默认 USB 上位机通信。
- 补齐 OLED 片选和显示路径。
- 修正母线电压采样比例。
- 将源码和文档统一维护为 UTF-8。
- 增加 `.editorconfig` 和 `.gitattributes`，避免中文注释继续因为编码不一致出现乱码。
- 增加“压缩机安全启动版本”相关状态机和保护逻辑。
- 增加 VSCode + STM32CubeCLT + ST-Link 的编译、下载、调试链路。

当前固件操作方式：

- `KEY1`：启动/停止；如果处于故障状态，先清故障。
- `KEY2`：切换 OLED 页面。
- `KEY3`：切换目标速度。

当前安全启动版本的保护思路：

- 12V 台架测试优先。
- 低电流限制启动。
- 母线欠压/过压保护。
- 相电流软件过流保护。
- DRV8301 硬件故障监测。
- 堵转检测和重启等待。
- 发生故障后关闭 PWM，并把三相输出回到安全状态。

这个阶段的决策：

- 不追求一步到位产品级，而是先保证“烧得进去、空载安全、采样可信、再逐步带压缩机”。
- 暂时保留原工程 EKF 观测器，先用低风险实测判断它在当前硬件和压缩机上的可用性。
- 参数先使用 6MD030Z 手册和估算值，后续根据实机反馈逐步修正。

## 3. 压缩机和参考驱动板资料分析

我提供了压缩机和商品驱动板资料：

```text
d:\XJTLU\工作相关\卷云科技有限责任公司\通用数据手册\6MD030Z_松下24V压缩机数据手册.pdf
d:\XJTLU\科研_DIY\FOC_Project\datasheet\GE2117-V2_压缩机驱动板说明书.pdf
```

从压缩机资料中提取到的关键参数：

- 压缩机：Panasonic `6MD030Z`
- 额定电压：24V
- 控制方式：无感正弦波驱动
- 极数：8 极，即 4 对极
- 额定转速区间：约 1800-4500 rpm
- 额定输出功率：约 100W
- 绕组电阻：资料给出 `0.376Ω`，但还需要实测确认是线电阻还是相电阻
- 电感：资料给出 Ld/Lq 相关数据，实际应后续整定
- 反电动势/磁链：可由 Ke/Kt 估算，但最终也要靠实机修正

从商品驱动板资料中看到的产品级特征：

- 有明确的上电等待和启动流程。
- 有启动电流、速度源、最小/最大转速等可配置参数。
- 有欠压、过压、过流、过温、堵转、缺相等保护。
- 有通信接口和运行状态反馈。
- 启动失败后有重启间隔，不会短时间反复冲击压缩机。

这个阶段的决策：

- 先做一个简化版“安全启动”，学习商品驱动板的保护和启动思路。
- 产品级功能后续再逐步补齐，包括参数存储、通信、故障日志、温度保护、缺相判断等。
- 由于我计划先用 12V，而且手头商品板在 12V 下能跑，因此 12V 低压验证是可行的第一阶段。

## 4. 编译、下载、调试链路打通

为了在 VSCode 里用 ST-Link 烧录和调试，工程补了 VSCode 配置和构建脚本。

当前使用的工具链路径：

```text
D:\ST\STM32CubeCLT_1.21.0
```

VSCode 任务：

- `Probe ST-Link`
- `Build firmware`
- `Flash firmware (ST-Link)`
- `Erase chip (ST-Link)`
- `Reset target (ST-Link)`
- `Clean firmware`

这个阶段的结果：

- GCC 编译链路已经打通。
- 固件可以生成 `.elf/.hex/.bin`。
- ST-Link 下载任务已经配置好。
- VSCode 调试入口也已经配置好。

这个阶段的决策：

- 后续所有硬件调试都以“先能复现构建和烧录”为前提。
- 每次关键代码变更后，都要重新 build，必要时再 flash 到板子上验证。

## 5. 第一次真实硬件上电前计划

进入实物调试时，我决定采用逐步闭环方式：

```text
我测一小步 -> 反馈数据 -> 判断是否安全 -> 再进入下一步
```

第一步不是直接接压缩机，而是空载验证：

- 不接压缩机三相线。
- 用 12V 台架电源供电。
- 先限流。
- 连接 ST-Link。
- 先 `Probe ST-Link`。
- 再 `Build firmware`。
- 再 `Flash firmware`。
- 上电后不按 `KEY1`，只观察静态状态。

这一步的目标：

- 确认 ST-Link 能识别。
- 确认固件能编译。
- 确认固件能烧录。
- 确认板子静态电流正常。
- 确认 OLED 正常。
- 确认 DRV8301 没有故障。
- 确认不启动时三相输出没有异常动作。

## 6. Step 1：空载烧录和静态检查

实际操作记录：

- 压缩机三相线未接。
- 台架电源设为 12V 稳压输出。
- 限流设为 4A。
- 使用 ST-Link 连接并烧录。

我的反馈数据：

```text
Probe ST-Link：成功
Build firmware：成功
Flash firmware：成功
12V 上电后实际电压：12V
电源显示电流：33mA
OLED 第一页：慧驱动 logo
OLED 第二页：C:stop / tgt:1800 / ekf:0 / Iq:0.0 / F:0
不按 KEY1 时三相输出：没有明显电压或动作
DRV8301 OCTW：不亮
DRV8301 FAULT：不亮
DRV 相关 LED：只有 5V 电源指示灯亮
```

判断：

- ST-Link、编译、下载链路全部正常。
- 12V 空载 33mA 很健康，说明静态供电没有明显短路或异常负载。
- OLED 能显示 logo 和状态页，说明基本 UI 正常。
- `OCTW/FAULT` 不亮，说明 DRV8301 静态状态正常。
- 不按 `KEY1` 时三相无动作，说明默认状态没有误驱动电机。

这个阶段的决策：

- Step 1 通过。
- 暂时不接压缩机。
- 在进入 Step 2 前，先验证 ADC 采样链路，尤其是 Ia/Ib 和 Vbus 测试点。
- OLED 当时显示 `tgt:1800`，与当时固件显示逻辑一致；后续 Step 13 发现这其实是把电频率口径误显示成了机械 rpm。

## 7. Step 1.5：用万用表确认采样测试点

我提出想用万用表测预留的 `Ia/Ib` 测试点，但担心应该用电流档还是电压档。

根据原理图和网表确认：

- `IA1` 测试点连接到 `ADC_IA`。
- `IB1` 测试点连接到 `ADC_IB`。
- `ADC_IA/ADC_IB` 来自 DRV8301 的 `SO1/SO2` 电流采样放大输出。
- 测试点后面经过 `51Ω + 4.7nF` 滤波进入 STM32 ADC。

结论：

```text
IA1/IB1 是电压信号测试点，必须用万用表直流电压档测量。
不能用电流档。
```

原因：

- 电流档等效低阻，直接碰 `SO1/SO2` 或 ADC 输入可能造成短路。
- 轻则读数异常或触发故障，重则损坏 DRV8301、MCU ADC 输入或万用表保险。

实际测量方式：

```text
万用表 DC V 档
黑表笔接 GND
红表笔分别测 IA1、IB1、VBUS/VBUS_AD
```

我的反馈数据：

```text
IA1 = 1.646V
IB1 = 1.646V
VBUS = 0.567V
电源仍然是 12V / 33mA
测量过程中 OCTW/FAULT 没有亮
```

判断：

- `IA1` 和 `IB1` 都是 `1.646V`，非常接近理论中点偏置 `1.65V`。
- A/B 两路采样偏置高度一致，说明 DRV8301 电流采样输出、滤波、ADC 前端至少在静态下非常健康。
- `VBUS = 0.567V` 与 12V 母线经过 `100k / 4.99k` 分压后的结果一致。
- 测量过程中 DRV8301 没有报错，说明测试方式安全，采样点没有被误短路。

这个阶段的决策：

- 空载供电链路通过。
- DRV8301 静态状态通过。
- 相电流采样偏置通过。
- 母线电压采样比例通过。
- 可以进入 Step 2，但 Step 2 仍然要保持低风险，不直接进行大电流或长时间启动。

## 8. 当前结论

截至 2026-05-09，项目已经完成从“代码能不能用”到“硬件静态状态可信”的第一轮验证。

已确认：

- 当前固件能在我的板子上编译、下载、运行。
- VSCode + ST-Link 链路可用。
- 12V 空载上电电流正常。
- OLED 正常显示。
- DRV8301 无静态故障。
- 不启动时三相无异常输出。
- IA/IB 采样偏置正常。
- Vbus 分压采样正常。

暂未验证：

- 接入压缩机后三相相序是否正确。
- 电流方向是否与 FOC 算法定义一致。
- EKF 在低速启动时是否能稳定收敛。
- 开环拖动到闭环切换是否可靠。
- 12V 下能否稳定启动这个压缩机。
- 当前速度单位和 OLED 显示是否完全一致。
- 当前 Rs/Ls/flux 参数是否足够接近真实压缩机。

## 9. 下一步计划

下一步进入 Step 2，建议目标是“接压缩机但继续保持受控风险”：

- 先断电接入压缩机 U/V/W。
- 不按 `KEY1`，仅观察静态电流和故障灯。
- 确认接入压缩机后 12V 静态电流没有异常变化。
- 再考虑短按启动，观察是否只是轻微尝试启动，还是出现大电流、抖动、FAULT/OCTW、堵转保护。

Step 2 的核心不是追求马上转起来，而是确认：

- 接上压缩机以后硬件没有静态异常。
- 启动瞬间保护是否有效。
- 如果失败，失败形态是什么：相序、电流方向、参数、EKF、启动策略，还是供电能力。

## 10. Step 2：接入压缩机但不启动

在 Step 1 和 Step 1.5 均通过后，我进入接入压缩机的静态检查阶段。这一步仍然不按 `KEY1` 启动，目标是确认压缩机接上以后不会造成静态短路、漏电、DRV8301 故障或异常发热。

断电后，先用万用表电阻档测量压缩机三相线之间的电阻。

我的反馈数据：

```text
U-V = 0.5Ω
V-W = 0.5Ω
W-U = 0.5Ω
表笔短接电阻 = 0Ω
万用表最小电阻档 = 400Ω 档，低阻精度有限
```

判断：

- 三组三相线间电阻一致，说明压缩机绕组没有明显断相。
- 由于普通万用表在 1Ω 以下精度有限，`0.5Ω` 只能作为粗略读数。
- 三相电阻一致性比绝对值更重要；后续如果需要精确电阻，应该用四线法或恒流源法测量。

随后把压缩机三相线接到板子 `U/V/W`。相序已按数据手册核对，当前认为接线顺序正确。

台架电源设置：

```text
电源电压：12V
限流：1A
```

接入压缩机但不启动时的反馈数据：

```text
电源电压 = 12V
电源电流 = 36mA
OLED 第二页 = C:stop / tgt:1800 / ekf:0 / Iq:0.0 / F:0
OCTW = 不亮
FAULT = 不亮
压缩机声音 = 无
压缩机抖动 = 无
压缩机发热 = 无
```

判断：

- 接上压缩机后，静态电流从空载约 `33mA` 只上升到 `36mA`，变化很小。
- `OCTW/FAULT` 均不亮，说明 DRV8301 没有检测到静态异常。
- 压缩机没有声音、抖动、发热，说明未启动状态下三相桥没有误输出。
- Step 2 通过，可以进入第一次极短启动测试。

这个阶段的决策：

- 第一次启动仍然使用 12V 和 1A 限流。
- 第一次启动只做短时观察，不追求持续运行。
- 重点记录启动瞬间电源是否打限流、压缩机动作形态、DRV 故障灯、OLED 故障码和 EKF 显示。

## 11. Step 3：第一次极短启动，触发假欠压

在 Step 2 静态检查通过后，我保持：

```text
电源电压：12V
限流：1A
OLED：第二页
压缩机：已接 U/V/W
```

按 `KEY1` 前状态仍然正常：

```text
电压、电流、OLED、DRV8301 状态均与 Step 2 基本一致
```

按 `KEY1` 后的反馈：

```text
OLED 瞬间显示 C:fault
F 故障码 = 1
电源没有打限流
电压没有掉
压缩机完全没动
声音 = 无
EKF = 0
Iq = 0
```

判断：

- `F=1` 对应欠压保护。
- 但台架电源没有掉压，也没有进入限流，压缩机完全没有动作。
- 因此这不像是真正的功率侧欠压，更像是固件状态机或 ADC 变量刷新时机的问题。

代码排查结论：

- 欠压判断使用全局变量 `Vbus`。
- 原代码只在 `motor_run()` 中把 `ADC1->JDR1` 换算成 `Vbus`。
- 上电 offset 采样阶段虽然 ADC 已经采了母线电压，但 `get_offset()` 只累计了 A/B 相电流 offset，没有同步刷新 `Vbus`。
- 第一次按 `KEY1` 时，`motor_start()` 会先检查 `Vbus`，此时 `Vbus` 可能仍然是默认 `0`。
- 于是固件把 `Vbus=0` 当成欠压，直接触发 `F=1`，PWM 尚未真正输出，所以压缩机、Iq、EKF 都没有动作。

修复决策：

- 不修改欠压阈值。
- 不绕过欠压保护。
- 只把 ADC 注入通道 `JDR1` 到 `Vbus` 的换算提取成公共小函数。
- 在 offset 采样阶段也刷新 `Vbus`，让启动前保护逻辑看到真实母线电压。

这次修复对应文件：

```text
motor/adc.c
```

## 12. Step 3 复测：假欠压修复后进入真实启动调试

烧录修复后的固件并重新上电后，再次按 `KEY1` 启动。

我的反馈数据：

```text
启动成功进入运行尝试
Iq 最高爬到 1.49A
EKF 最高爬到 9Hz
电源电压没有掉
电源没有打限流
电源最高电流约 330mA
压缩机有轻微抖动
能听见压缩机内部油液动作声音，判断可能已经有轻微起转
随后 OLED 显示 C:fault
F 故障码 = 4
OCTW = 不亮
FAULT = 不亮
```

判断：

- 这次不再瞬间 `F=1`，说明 `Vbus` 启动前刷新问题已经修复。
- `Iq` 从 0 爬升到 `1.49A`，说明电流参考和电流环开始工作。
- `EKF` 从 0 爬升到 `9Hz`，说明无感观测器已经看到了一定的速度/反电动势信息。
- 压缩机有轻微机械响应，说明三相驱动不再是完全无输出状态。
- 电源只到约 `330mA`，且没有掉压，说明这次不是电源能力不足，也不是硬件过流。
- `OCTW/FAULT` 全程不亮，说明 DRV8301 没有报硬件过温/过流故障。
- `F=4` 是软件堵转判断触发。当前固件要求启动检查结束时 `EKF_Hz >= 12Hz`，而本次最高只有 `9Hz`，因此状态机按设定停机。

修复/调参决策：

- 这不是新的硬件故障，而是启动判据对当前 12V、1A、低速诊断场景偏严格。
- 下一版临时诊断固件把启动检查时间从 `12s` 放宽到 `20s`。
- 堵转判据从 `12Hz` 临时下调到 `6Hz`，低于本次已经实测达到的 `9Hz`。
- 目的不是最终产品参数，而是观察压缩机在更长时间内能否继续爬升，还是稳定卡在低速抖动。

这次调参对应文件：

```text
motor/foc_define_parameter.h
```

## 13. Step 3 再复测：12V 低压稳定连续运行

烧录放宽后的 12V 低压诊断固件后，再次按 `KEY1` 启动。

我的反馈数据：

```text
压缩机可以稳定一直运行
不会再跳 fault
EKF 最高到 17Hz，稳定后在 16Hz 和 17Hz 之间来回跳
Iq 最高到 1.49A
电源最高电流约 290mA
电压没有掉
压缩机现象：明显连续转动，同时带有少许轻微抖动
OCTW = 不亮
FAULT = 不亮
```

判断：

- 当前 12V、1A 限流条件下，压缩机已经能进入连续运行状态。
- `F4` 堵转保护不再触发，说明上一次 `EKF=9Hz` 时被 `12Hz` 堵转门槛卡住是主要停机原因之一。
- `OCTW/FAULT` 都不亮，说明这个测试点没有触发 DRV8301 硬件过流或过温警告。
- 电源电流约 `290mA` 且电压不掉，说明当前只是很轻载、低速运行，不是接近额定工况。
- `EKF=16-17Hz` 是无感观测器估算到的速度信号。结合当前代码看，它更接近电角频率换算后的 Hz，而不是机械 rpm。
- 因为 6MD030Z 是 8 极，即 4 对极，如果 `EKF_Hz` 是电频率，那么对应机械转速约为：

```text
mechanical rpm = electrical Hz * 60 / pole_pairs
16Hz -> 240 rpm
17Hz -> 255 rpm
```

阶段结论：

- 本阶段已经验证：硬件、DRV8301、电流采样、基础 FOC 输出、EKF 观测和安全保护链路可以让压缩机在 12V 下低速连续运行。
- 但这不是 1800rpm 级别运行，更像是低速安全旋转验证。
- 下一步必须统一速度单位：固件内部仍可使用电频率 Hz 给 FOC 和 EKF，但 OLED、用户目标值、日志和调参界面应该显示机械 rpm，避免把 `30Hz` 电频率误认为 `1800rpm` 机械速度。

下一步决策：

- 优先修改速度单位显示和目标管理。
- 把用户侧目标改成机械 rpm。
- 在写入 `Speed_Ref` 前转换为电频率 Hz：

```text
electrical Hz = mechanical rpm * pole_pairs / 60
```

- OLED 上的 `tgt` 和 `ekf` 都显示机械 rpm。
- 这样后续调试时才能准确区分“低速安全旋转”“接近商品板启动速度”和“产品级运行速度”。

同步代码修改：

```text
motor/foc_define_parameter.h
- 将用户侧速度常量从 Hz 改为 rpm：
  450/525/600/675/750 rpm 低速诊断档
- 当前诊断档保持等价物理输出：
  450-750 rpm 对 4 对极压缩机等价于 30-50 Hz 电频率
- 堵转阈值改为机械口径：
  COMPRESSOR_STALL_SPEED_RPM = 90 rpm

motor/low_task.c / motor/low_task.h
- 新增 rpm 与电频率 Hz 的换算
- 用户目标变量改为 compressor_target_speed_rpm
- 写入 Speed_Ref 前统一换算成电频率 Hz
- 堵转判断改为机械 rpm 口径
- 暴露 compressor_get_ekf_speed_rpm() 给 OLED 显示使用

user/oled_display.c
- OLED 第二页 tgt 显示目标机械 rpm
- OLED 第二页 ekf 显示 EKF 换算后的机械 rpm

README.md / codex.md
- 同步把当前安全启动档从误写的 1800-3000 rpm 改为真实的 450-750 rpm 低速诊断档
```

预期下一次烧录后，若物理运行状态与本次相近：

```text
tgt 应显示 450
ekf 应显示约 240-255 rpm
```

这个显示变化不是电机突然变慢，而是把原先错误的速度显示口径修正为机械 rpm。

## 14. Step 4：速度口径修正后出现高 Iq 抖动和 F4

烧录速度单位修正版后，再次按 `KEY1` 启动。

我的反馈数据：

```text
Iq 最高到 2.5A 多
EKF 最高到 149 rpm
随后 EKF 突然掉到约 67 rpm
Iq 掉回约 1.49A
EKF 在 60 多 rpm 附近跳了几秒后 C:fault
F 故障码 = 4
一开始加速时压缩机轻微抖动
到 EKF 最高附近时抖动加重了两下
掉速后、报 fault 前的几秒，能感觉到转子一直在原地抖
```

判断：

- 这次 `F4` 仍然是软件堵转保护，不是 DRV8301 硬件故障。
- `EKF` 掉到 60 多 rpm 后低于当前 `90 rpm` 堵转阈值，持续数秒后触发 `F4`，保护动作符合预期。
- 关键异常是启动电流继续爬到 `2.5A+` 后抖动明显加重，随后 EKF 掉速并进入原地抖动。
- 这说明当前 12V 诊断档不适合继续把启动 Iq 往 2A 以上拉；更高电流没有换来稳定加速，反而破坏了低速同步。
- 前一轮已经验证 `Iq≈1.49A` 时能连续运行，因此下一版先把启动 Iq 固定在这个已验证稳定点。

同步代码修改：

```text
motor/foc_define_parameter.h
- MOTOR_STARTUP_CURRENT 从 3.0A 降到 1.5A
- 新增 COMPRESSOR_STARTUP_IQ_STEP_A，保留原来的慢速爬升步进

motor/adc.c
- Iq_ref 爬升到 MOTOR_STARTUP_CURRENT 后直接保持
- 移除原先爬到高电流后再降到一半的启动电流曲线

README.md / codex.md
- 同步当前 12V 诊断档启动电流为 1.5A
```

下一次测试目的：

- 验证限流到 `1.5A` 后是否恢复前一轮的稳定连续运行。
- 如果仍然抖动或 F4，下一步再降低目标 rpm 或开始做真正的开环对齐/强拖启动，而不是继续堆电流。

## 15. Step 4 复测：1.5A 固定版过保守，回退到稳定版本

烧录 `1.5A` 固定诊断版后再次测试。

我的反馈数据：

```text
Iq 最高到 1.5A
EKF 最高爬到 73-74 rpm
约 10s 后 C:fault
F 故障码 = 4
可调电源最高只有约 150mA
手摸压缩机完全感觉不到在转
压缩机本体没有任何声音或动作
```

判断：

- `1.5A` 固定版太保守，虽然 OLED 命令电流到了 1.5A，但 DC 输入只有约 150mA，压缩机没有可感知动作。
- 此时 EKF 显示的 73-74 rpm 不能直接当作真实机械转速，可能包含低速估算漂移或抖动。
- 继续在这个版本上调小保护阈值没有意义，因为机械上没有真实起转。

回退决策：

- 恢复到两版前已经实测可以连续运行的固件行为。
- 保留 `motor/adc.c` 中的 `Vbus` 启动前刷新修复，因为它解决的是确定存在的假欠压 bug。
- 回退速度目标管理到原来的 Hz 口径：
  - `COMPRESSOR_MIN_SPEED_HZ = 30`
  - `COMPRESSOR_MAX_SPEED_HZ = 50`
  - `COMPRESSOR_SPEED_STEP_HZ = 5`
- 回退 OLED 显示到当时稳定版本：
  - `tgt = compressor_target_speed_hz * 60`
  - `ekf = EKF_Hz`
- 回退启动电流曲线：
  - `MOTOR_STARTUP_CURRENT = 3.0A`
  - `Iq_ref` 继续使用原来的慢速爬升与后续回落逻辑
- 堵转阈值保持前一轮稳定运行时的放宽诊断值：
  - `COMPRESSOR_STALL_SPEED_HZ = 6`
  - `COMPRESSOR_STARTUP_CHECK_MS = 20000`

这次回退对应文件：

```text
motor/foc_define_parameter.h
motor/low_task.c
motor/low_task.h
motor/adc.c
user/oled_display.c
```

注意：

- 回退后的 OLED `tgt:1800` 仍是旧显示口径，不代表真实机械 1800 rpm。
- 当时已经判断 `30 Hz` 更像电频率，对 4 对极压缩机约等于 450 rpm 机械转速。
- 这次优先恢复“能真实转起来”的行为，速度显示口径后面再用不改变控制行为的方式单独修。

## 16. Step 5：回退版仍随机失步，加入开环强拖启动段

烧录回退版后再次测试。

我的反馈数据：

```text
启动时 Iq 往上爬
Iq 爬到 2A 多时压缩机剧烈抖动
随后 Iq 直接跳回 1.49A
这期间 EKF 一直是 0，没有动过
跳回 1.49A 后 C:fault
F 故障码 = 4
台架电源最高约 300mA，后面掉到 100mA 多
OCTW/FAULT 都不亮
```

判断：

- 这次故障不是硬件过流或 DRV8301 故障。
- `EKF` 一直为 0，说明无感观测器没有拿到有效转子运动/角度。
- 但原启动逻辑在 EKF 为 0 时仍然继续爬升 `Iq_ref`，等于“没有可靠角度还继续加力”，所以表现为原地抖动。
- 前面那次能连续运行更像是初始转子位置/负载状态刚好给了 EKF 起步机会，不能视为确定性启动策略。

同步代码修改：

```text
motor/foc_define_parameter.h
- 新增 COMPRESSOR_FORCE_START_ENABLE
- 新增开环启动频率：
  COMPRESSOR_OPEN_LOOP_START_HZ = 2Hz
  COMPRESSOR_OPEN_LOOP_MAX_HZ = 18Hz
  COMPRESSOR_OPEN_LOOP_RAMP_HZ_S = 3Hz/s

motor/adc.c
- 新增 compressor_update_open_loop_start()
- 启动时先用 hall_angle 作为强拖开环角度
- hall_angle_add 按开环电频率缓慢增加
- EKF 速度没有超过 SPEED_LOOP_CLOSE_RAD_S 前，不再用 EKF 角度直接驱动
- EKF 速度超过阈值后，再切回 EKF 角度和原速度闭环路径
```

这版的目标：

- 不再赌 EKF 零速起步。
- 先用低速旋转电流矢量把压缩机拖起来。
- 观察 EKF 是否能从 0 开始跟上真实运动。
- 如果仍然无法起转，再调开环频率斜坡和启动电流，而不是继续让 EKF 在零速闭眼加力。

## 17. Step 5 复测：开环强拖版本稳定运行

烧录开环强拖启动版本后再次测试。

我的反馈数据：

```text
压缩机可以稳定运行
EKF 从 0 开始往上爬
EKF 爬到 12-13 之间后稳定来回跳
Iq 先往上爬一段，最后稳定在 1.49A
OCTW = 不亮
FAULT = 不亮
OLED 状态 = C:run
台架电源电流稳定在约 220mA
压缩机能明显感觉到在连续转
运行时有轻微抖动
手摸吸气口能感受到吸力
```

判断：

- 开环强拖启动策略有效解决了“EKF 零速不锁定时原地抖动”的问题。
- `EKF` 能从 0 爬升并稳定在 `12-13`，说明观测器已经能跟随拖动后的真实运动。
- `Iq` 最终稳定在约 `1.49A`，与前面成功运行时的稳定值一致。
- 台架电源约 `220mA`，电压未报告掉落，说明当前仍是 12V 低功率安全运行状态。
- `OCTW/FAULT` 都不亮，说明没有触发 DRV8301 硬件过流/过温告警。
- 吸气口有吸力，说明压缩机不是纯空转抖动，已经产生了可感知的压缩机功能输出。

阶段结论：

- 本阶段首个可固化版本是“12V 开环强拖 + EKF 接管”的安全启动固件。
- 这一版适合推送到 GitHub 作为后续调参基线。
- 后续优化应围绕降低抖动、确认真实转速、调开环斜坡、显示实际/命令电流分离、再逐步提高电压和速度展开。

## 18. Step 6：重复启动验证失败，降低起步随机性

按计划做 5 次重复启动验证后，结果不稳定。

我的反馈数据：

```text
5 次启动中只有 1 次成功
失败时 EKF 一开始往上爬到约 10
随后 EKF 立刻掉到约 3
Iq 爬到 2A 多
随后 Iq 掉到约 1.49A
过一会儿触发 C:fault / F4
```

判断：

- 开环强拖方向是对的，但上一版仍然有较大启动随机性。
- EKF 能爬到 10 后掉到 3，说明转子没有稳定跟上开环斜坡。
- 继续使用 `2-18 Hz`、`3 Hz/s` 的开环斜坡偏激进，容易把转子拖丢。
- 启动峰值电流继续冲到 2A 多，但没有换来稳定跟随，说明应优先放慢拖动节奏，而不是继续加电流。

同步代码修改：

```text
motor/foc_define_parameter.h
- MOTOR_STARTUP_CURRENT 从 3.0A 降到 2.1A
- 新增 COMPRESSOR_STARTUP_HOLD_CURRENT = 1.5A
- 新增 COMPRESSOR_OPEN_LOOP_ALIGN_MS = 1200ms
- 开环强拖从 2-18Hz / 3Hz/s 改为 1-12Hz / 1Hz/s
- 启动检查窗口从 20s 延长到 30s

motor/adc.c
- 新增 compressor_open_loop_ticks
- 启动前 1.2s 固定电角度，对齐转子
- 对齐后再慢速开环旋转
- Iq 爬升到 2.1A 后回落并保持 1.5A，而不是继续沿用 3A 峰值
```

下一次验证目标：

- 仍然做 5 次启动循环。
- 成功标准不是跑更快，而是至少显著提高启动成功率，并降低“EKF 到 10 又掉到 3”的掉同步概率。

## 19. Step 6 复测：固定角度对齐版本过保守，撤销对齐

烧录“1.2s 固定角度对齐 + 1-12Hz 慢拖 + 2.1A 峰值”版本后测试，结果比上一版更差。

我的反馈数据：

```text
按 KEY1 后压缩机没有任何动静
Iq 爬到 2.1A 后掉到 1.5A
EKF 全程为 0，没有变化
OCTW/FAULT 都不亮
最后 C:fault / F4
台架电源最高约 200mA
Iq 掉下来后电源约 110mA
```

判断：

- 这版代码策略有问题，不应继续沿用。
- 固定角度对齐阶段没有让压缩机进入有利启动位置，反而使启动过程没有任何可感知运动。
- `EKF=0` 全程不动，说明转子没有被实际拖起来；这不是保护阈值问题。
- `2.1A` 峰值在当前开环策略下太保守，命令电流爬升但没有换来实际运动。

同步代码修改：

```text
motor/foc_define_parameter.h
- MOTOR_STARTUP_CURRENT 恢复到 3.0A
- COMPRESSOR_STARTUP_HOLD_CURRENT 保持 1.5A
- 撤销固定角度对齐：COMPRESSOR_OPEN_LOOP_ALIGN_MS = 0
- 开环强拖恢复到 2-18Hz
- 开环斜率保持较慢的 1Hz/s，避免上一版 3Hz/s 太容易拖丢
```

下一次验证目标：

- 验证无固定对齐、3A 峰值、慢斜坡能否重新产生真实机械运动。
- 重点看 EKF 是否从 0 动起来，以及是否还出现“到 10 又掉 3”的失步。

## 20. Step 6 再复测：低速强拖思路陷入循环，切到强启动诊断

继续测试“无固定对齐、3A 峰值、2-18Hz 慢斜坡”版本后，仍然无法可靠起转。

我的反馈数据：

```text
Iq 最高爬到 2.5A
EKF 最高爬到 10
随后 Iq 立刻掉到 1.5A
EKF 掉到 2-3
过一会儿 C:fault / F4
手摸压缩机几乎没感觉到转起来
吸气口没有吸力
```

判断：

- 低速几百毫安、低开环频率的策略已经陷入循环，不能继续只微调斜率。
- 压缩机需要进入更接近正常工作区间，才可能稳定建立流体和反电动势。
- 商品驱动板的启动思路通常不是让压缩机长期停在几百毫安附近，而是先把电流和速度拉到可运行区间。
- 因此下一版切换为“强启动诊断版”，目标是验证更接近正常工作区的启动是否更可靠。

同步代码修改：

```text
motor/foc_define_parameter.h
- 默认速度目标从 30Hz 提高到 120Hz 电频率
- 速度范围改为 120-200Hz 电频率，对 4 对极约 1800-3000rpm 机械转速
- 启动 Iq 峰值从 3.0A 提高到 3.5A
- 起转后保持 Iq 从 1.5A 提高到 3.0A
- Iq 爬升斜率提高到 1A/s
- 开环强拖改为 5-120Hz，斜率 15Hz/s
- 堵转阈值保持 10Hz 电频率，启动检查窗口 30s
```

测试要求：

- 台架电源先设 `12V / 2A` 限流，不要一开始放到 4A。
- 按 `KEY1` 后只观察一次，不做 5 次循环。
- 如果电源进入限流、压缩机剧烈撞击、板子异响、OCTW/FAULT 亮，立即停止。

这次验证的关键不是省电，而是确认强启动能否让压缩机真实起转并产生吸力。

## 21. Step 6 强启动复测：命令电流上去了，但没有形成真实旋转

烧录“120Hz 目标、3.5A 峰值、3.0A 保持、5-120Hz 开环强拖”诊断版后，压缩机有更明显动静，但仍然没有真实进入压缩运行。

我的反馈数据：

```text
Iq 到 3A 时，压缩机明显有动静
感觉没有在转，而是在原地抖
有一个人耳能听到的固定频段振动声
吸气口没有吸力
电源电压没有掉
电源没有限流
Iq 最后稳定在 3.0
EKF 没有超过 10，最后掉到 2
台架电源最高约 500mA
最后仍然 C:fault / F4
```

判断：

- 这次不是“电流再加一点就好”的问题，而是控制策略和诊断信息不够了。
- OLED 原先显示的 `Iq` 是 `FOC_Input.Iq_ref`，也就是命令电流，不是实际 q 轴反馈电流。
- 屏幕显示 3A，但台架电源只有约 500mA、没有吸力、EKF 锁不住，说明“命令 Iq”没有可靠转化成有效电磁转矩。
- 固定频段抖动通常意味着转子被定子磁场拉扯，但没有跟随旋转磁场建立连续转动。
- 下一步不能再盲目提高电流或速度，应先确认电流采样、Park 变换、相序、开环角度方向、电流环极性是否一致。

同步代码修改：

```text
motor/foc_algorithm.h
- 导出 Current_Idq，允许 OLED 页面读取实际 dq 电流反馈。

user/oled_display.c
- OLED 第 2 页底行从只显示命令 Iq，改为同时显示：
  r: 命令 q 轴电流 FOC_Input.Iq_ref
  q: 实际 q 轴反馈电流 Current_Idq.Iq
- 保留 tgt、ekf 和 F 故障码显示。
```

编译与烧录：

```text
Build firmware 成功
FLASH: 25736 B / 512 KB
RAM: 5936 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 只做一次短启动，不再连续反复试。
- 重点记录 `r:` 和 `q:` 的关系。
- 如果 `r:` 到 3A 而 `q:` 接近 0 或乱跳，优先排查电流采样/相序/电流环方向。
- 如果 `q:` 能跟上 3A 但仍只抖不转，优先排查开环旋转方向、相序、机械负载和启动算法。

## 22. Step 7：用 GE2117 说明书反推启动策略

烧录诊断显示版后，继续测试强启动。

我的反馈数据：

```text
未按 KEY1 时 r=0，q=0
按 KEY1 后 r 最高到 3，q 最高到 3
r 和 q 两个数字几乎同步
EKF 最高到 3，之后掉到 0
台架电源最高约 330mA，没有限流
机械现象：无吸力、无明显撞击声、基本没动静
最后 F4
DRV OCTW/FAULT 都不亮
```

判断：

- q 轴电流反馈能跟随命令值，说明这次不是简单的“电流环完全不跟随”。
- 但 EKF 几乎没有速度、母线功率很低、机械没有吸力，说明电流矢量没有形成有效连续转矩。
- 现在最可疑的是启动算法：未知转子位置下直接给 q 轴电流并旋转角度，可能只是在数学坐标里“q=3A”，但物理转子并没有进入可跟随的初始位置。

从 `GE2117-V2_压缩机驱动板说明书.pdf` 读到的可模仿点：

```text
上电后等待 10s
接到启动信号后，无论设定速度多少，先以 3000rpm 初始化启动
稳定在 3000rpm 速度 10s 后，再逐步闭环到设定速度
启动前定位时间默认 1000ms
启动 Lq 电流默认 3.00A
启动 Ld 电流默认 3.00A
有低速力矩补偿、最大功率限制、堵转/缺相保护
```

同步代码修改：

```text
motor/foc_define_parameter.h
- MOTOR_STARTUP_CURRENT 从 3.5A 改为 3.0A，对齐 GE2117 默认 Lq 启动电流
- 新增 COMPRESSOR_STARTUP_ID_CURRENT = 3.0A，对齐 GE2117 默认 Ld 启动电流
- Iq 爬升斜率改为 3A/s，让 1s 定位后更快进入 3A 启动段
- 启动前定位改为 1000ms
- 开环强拖改为 20-200Hz，斜率 35Hz/s
- 200Hz 电频率对应 4 对极约 3000rpm，模仿 GE2117 的 3000rpm 初始化启动
- 启动检查窗口改为 10s
- 堵转判定速度改为 40Hz 电频率
- 新增 COMPRESSOR_OPEN_LOOP_DIRECTION，后续可快速反向测试

motor/adc.c
- 新增 compressor_is_aligning()
- 启动前 1s 使用固定电角度，并给 Id=3A、Iq=0A 进行 d 轴定位
- 定位结束后，启动阶段保持 Id=3A，同时 Iq 从 0A 爬升到 3A
- 启动阶段一直使用开环角度，不再让 EKF 在低速/未锁定时提前接管
- 撤销末尾无条件清零 Id_ref，避免定位电流被覆盖
```

编译与烧录：

```text
Build firmware 成功
FLASH: 25768 B / 512 KB
RAM: 5936 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 这版不是为了省电，而是验证“先 d 轴定位，再 3000rpm 初始化启动”的商品板式策略是否能让转子真正跟上。
- 按 KEY1 后前 1s 看到 `r:` 约为 0 属于正常，因为那一秒在做 d 轴定位。
- 1s 后 `r:` 应爬到 3A，`q:` 应跟随。
- 如果仍然完全不动，下一步优先把 `COMPRESSOR_OPEN_LOOP_DIRECTION` 改成 `-1.0f` 做反向启动测试。

## 23. Step 7 复测：正向 GE2117 仿启动仍未起转，切反向测试

烧录“1s d 轴定位 + 20-200Hz 正向开环 + Id/Iq 3A”版本后测试。

我的反馈数据：

```text
前 1 秒有轻微定位动静/吸附感
1 秒后 r 最高 3，q 最高 3
EKF 最高仍然只到 3，十几秒后掉到 0
电源最高约 617mA，没有限流
压缩机能感受到轻微震动
没有感受到连续转动
气口无吸力
最后 F4
OCTW/FAULT 不亮
```

判断：

- 定位阶段有吸附感，说明 d 轴定位确实作用到了转子。
- 旋转阶段 q 轴电流仍能跟随，但转子没有跟上旋转磁场。
- 这比上一轮更支持“旋转方向/相序组合不匹配”的假设。
- 下一步只改一个变量：反转开环角度方向，保持电流、定位时间、目标速度都不变，便于判断方向是否是主因。

同步代码修改：

```text
motor/foc_define_parameter.h
- COMPRESSOR_OPEN_LOOP_DIRECTION 从 1.0f 改为 -1.0f

codex.md
- 同步记录当前正在测试反向开环启动。
```

编译与烧录：

```text
Build firmware 成功
FLASH: 25772 B / 512 KB
RAM: 5936 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 同样只做一次短启动。
- 如果反向后 EKF 明显上升、压缩机有吸力或能连续转，说明方向/相序是主问题。
- 如果反向后仍无连续转动，下一步应转向检查相线/电流采样对应关系，或做低风险六步/定子磁场相序诊断。

## 24. Step 7 反向复测：方向反了，改为定位后再柔性爬升

烧录 `COMPRESSOR_OPEN_LOOP_DIRECTION=-1.0f` 反向开环测试版后，结果更差。

我的反馈数据：

```text
感觉方向反了
前 1 秒定位吸附感还在
1 秒后压缩机发出比较大的嗡嗡声
压缩机原地震，不转
吸气口没有吸力
EKF 一直是 0，没动过
最后 F4
```

判断：

- 反向比正向更差，`COMPRESSOR_OPEN_LOOP_DIRECTION=-1.0f` 基本可以判定不是正确启动方向。
- 正向至少曾经让 EKF 有微弱响应，因此恢复 `+1.0f`。
- 上一版还暴露了一个启动节奏问题：定位期间虽然 OLED 的 `r:` 显示为 0，但内部 `Iq_ref` 仍在爬升，1s 定位结束时 q 轴命令会瞬间跳到 3A。
- 这个 q 轴阶跃可能直接把刚定位好的转子踢丢，造成嗡嗡震动而不是连续跟随。

同步代码修改：

```text
motor/foc_define_parameter.h
- 新增 COMPRESSOR_STARTUP_RUN_ID_CURRENT = 0.0A
- COMPRESSOR_OPEN_LOOP_DIRECTION 从 -1.0f 恢复为 1.0f
- 开环起始频率从 20Hz 降到 2Hz
- 开环斜率从 35Hz/s 降到 20Hz/s
- 启动检查窗口从 10s 延长到 15s

motor/adc.c
- 定位期间强制 Iq_ref = 0，并保持 speed_close_loop_flag = 0
- 定位期间 FOC_Input.speed_fdk = 0
- 定位阶段使用 Id=3A、Iq=0A
- 定位结束后改为 Id=0A、Iq 从 0A 爬升到 3A
```

编译与烧录：

```text
Build firmware 成功
FLASH: 25872 B / 512 KB
RAM: 5936 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 按 KEY1 后前 1s 仍应有定位吸附感，`r:` 应保持 0。
- 1s 后 `r:` 不应瞬间跳到 3，而应在约 1s 内从 0 爬到 3。
- 机械上如果策略有效，应先出现更低频、更连续的拖动感，而不是立刻大声嗡嗡。
- 如果仍然只震不转，下一步不再继续调启动参数，应进入相线/电流采样映射诊断。

## 25. Step 7 慢爬复测：Iq 仍然太快，改成慢拖观察版

烧录“正向、2Hz 起拖、Iq 定位后再爬升”版本后测试，结果比反向更有信息量，但仍未起转。

我的反馈数据：

```text
前 1 秒有定位吸附感
r 在定位阶段保持 0
1 秒后 r 从 0 线性爬到 3
但 r 约 1 秒内就爬满，仍然太快
压缩机基本不动
EKF 最高到 10
r 从 0 爬到 3 后，EKF 立刻又掉回 2-3
电源最高 300mA 多，没有限流
无吸力
最后 F4
```

判断：

- 定位后不再是瞬间阶跃，方向也恢复正确，因此 EKF 能短暂到 10。
- 但 `Iq` 在约 1 秒内爬到 3A，仍然把转子拉丢。
- 当前掉同步发生在“q 轴电流爬满 + 开环频率继续上升”的组合附近。
- 下一步要明显放慢 `Iq` 和开环频率，记录掉同步发生时的开环频率，而不是继续猜。

同步代码修改：

```text
motor/foc_define_parameter.h
- COMPRESSOR_STARTUP_IQ_RAMP_A_S 从 3.0A/s 降到 0.375A/s，约 8s 爬到 3A
- COMPRESSOR_OPEN_LOOP_MAX_HZ 从 200Hz 降到 120Hz
- COMPRESSOR_OPEN_LOOP_RAMP_HZ_S 从 20Hz/s 降到 5Hz/s
- COMPRESSOR_STARTUP_CHECK_MS 从 15s 延长到 30s
- COMPRESSOR_STALL_CONFIRM_MS 从 2s 延长到 3s
- COMPRESSOR_STALL_SPEED_HZ 从 40Hz 降到 20Hz

motor/adc.c / motor/adc.h
- 将 compressor_open_loop_speed_hz 从 adc.c 私有静态变量改为可外部读取的全局变量。

user/oled_display.c
- OLED 第 2 页第一行从 tgt 改为 ol，显示当前开环拖动电频率。
```

编译与烧录：

```text
Build firmware 成功
FLASH: 25852 B / 512 KB
RAM: 5936 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 观察 `ol:`、`ekf:`、`r:`、`q:` 四个量。
- 重点记录 EKF 是在 `ol` 多少 Hz、`r` 多少 A 时开始掉下去。
- 如果 `ol` 很低、`r/q` 也很低时仍掉同步，下一步应进入相线/电流采样映射诊断。
- 如果慢拖能让 EKF 稳定上升，再继续优化斜率和启动接管条件。

## 26. Step 8：加入 RAM trace 黑匣子

我提出一个问题：既然 ST-Link 一直插着，能不能让 Codex 直接开调试、自己读取变量，而不是我盯 OLED 手动反馈。

评估结论：

- 直接用 GDB 变量窗口边跑边读不适合 FOC，因为普通调试读变量经常会暂停 MCU，可能破坏 PWM/电流环实时性。
- 更稳的方案是在固件里加 RAM trace 环形缓冲区，让 MCU 自己按固定周期记录关键变量。
- 测试结束后，再用 ST-Link/GDB 一次性把 RAM dump 出来转 CSV，这样不会干扰启动过程。

同步代码修改：

```text
motor/trace.h
- 新增 TRACE_RECORD_DEF，一条记录 32 字节。
- 新增 TRACE_META_DEF，保存 magic、write_index、wrapped、sample_count、active、buffer_size、record_size、sample_period_ms。
- trace_buffer 大小 2048 条。
- 采样周期 20ms，最多记录约 40.96s。

motor/trace.c
- 新增 trace_reset()：按 KEY1 启动时清空 trace 并开始记录。
- 新增 trace_sample_10ms()：在 10ms 低速任务中调用，每 20ms 实际写入一条。
- 记录字段包括：
  time_ms
  ol_hz
  ekf_hz
  Iq_ref / Iq_fb / Id_fb
  Ia / Ib / Ic
  Vbus
  Vd / Vq
  hall_angle
  compressor_state / fault / motor_start_stop / speed_close_loop_flag
- 故障或停机后自动冻结 trace，避免后续空闲数据覆盖启动过程。

motor/low_task.c
- motor_start() 中调用 trace_reset()。
- SysTick 10ms 任务中调用 trace_sample_10ms()。

motor/foc_algorithm.h
- 导出 Voltage_DQ，供 trace 记录 Vd/Vq。

tools/vscode-build.ps1
- 将 motor/trace.c 加入 GCC 构建源文件。

tools/export-trace.ps1
- 新增 ST-Link/GDB RAM dump 脚本。
- 脚本会启动 ST-LINK_gdbserver，连接 GDB，halt MCU，dump trace_buffer 和 trace_meta，再解析为 CSV。
- 由于 arm-none-eabi-gdb 对中文路径处理不好，脚本会先把 ELF 和临时 dump 放到 `%TEMP%\stm32_trace` 纯 ASCII 路径。

.vscode/tasks.json
- 新增 `Export trace CSV` 任务。
```

验证：

```text
Build firmware 成功
FLASH: 27320 B / 512 KB
RAM: 71512 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
export-trace.ps1 冒烟测试成功
空记录测试输出 samples=0，说明 GDB dump 和 CSV 解析链路可用
随后执行 Reset target 恢复目标板运行
```

下一次测试流程：

1. 上电，按 `KEY2` 切到参数页。
2. 按 `KEY1` 启动，让固件自己记录。
3. 等它 F4，或出现需要停机的现象后不要立刻断板子电。
4. 告诉 Codex “跑完了，导出 trace”。
5. Codex 运行 `tools/export-trace.ps1` 导出 CSV 并分析掉同步点。

## 27. Step 8 trace 首次实测：电流跟得上，但转子没跟住

慢拖观察版跑到 `F4` 后，使用 `tools/export-trace.ps1` 导出 RAM trace。

导出结果：

```text
Trace CSV: build/trace_latest.csv
samples = 1500
period_ms = 20
覆盖时间约 30s
wrapped = 0，说明没有覆盖旧数据
fault = 4，最后进入堵转故障
```

关键统计：

```text
EKF 最大值：4.7Hz
EKF 最大值发生时：time=9140ms, ol=42.7Hz, r=3.00A, q=3.00A
r/q 平均误差：约 0.006A
母线电压范围：11.67V - 12.04V
Vd 最大绝对值：0.73V
Vq 最大绝对值：0.80V
首次 F4：time=29980ms, ol=120Hz, ekf=1.1Hz, r=3.00A, q=3.02A
```

阈值点：

```text
r>=0.5A: time=2320ms, ol=8.7Hz,  ekf=1.0Hz
r>=1.0A: time=3660ms, ol=15.3Hz, ekf=2.0Hz
r>=1.5A: time=4980ms, ol=21.9Hz, ekf=2.7Hz
r>=2.0A: time=6320ms, ol=28.6Hz, ekf=3.6Hz
r>=2.5A: time=7660ms, ol=35.3Hz, ekf=2.9Hz
r>=3.0A: time=8980ms, ol=41.9Hz, ekf=4.1Hz
```

判断：

- 电流环不是主要问题，`q` 几乎完全跟随 `r`。
- 母线没有掉压，台架电源不是主要限制。
- `Vq` 远没有顶到 12V 母线附近，电压输出没有饱和。
- 真正的问题是转子没有跟住开环磁场；当 `r` 还没足够大时，`ol` 已经爬到较高电频率。
- 之前把启动阶段 `Id` 关掉可能也不符合 GE2117 参数表中的 `Ld=3A, Lq=3A` 启动方式。

同步代码修改：

```text
motor/foc_define_parameter.h
- COMPRESSOR_STARTUP_RUN_ID_CURRENT 从 0.0A 改为 3.0A
- 开环起始频率从 2Hz 降到 1Hz
- 开环最高频率从 120Hz 降到 60Hz
- 开环斜率从 5Hz/s 降到 1.5Hz/s
- 启动检查窗口从 30s 延长到 40s
- 堵转速度阈值从 20Hz 降到 8Hz

README.md / codex.md
- 同步记录当前为低频 Ld+Lq 慢启动诊断档。
```

编译与烧录：

```text
Build firmware 成功
FLASH: 27328 B / 512 KB
RAM: 71512 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 这版的目标不是直接冲到 3000rpm，而是确认转子能否在 `1-20Hz` 电频率附近跟上开环磁场。
- 如果这版仍然只有电流跟随、EKF 不动，下一步应优先检查相线与电流采样映射，而不是继续调启动斜坡。

## 28. Step 8 二次 trace：中段能跟，闭环接管瞬间打崩

我观察到：中间 `ol` 20 多 Hz 的时候转得挺好，后面才丢步。重新检查 trace 后，确认这个观察是对的。

关键时间线：

```text
12580ms: ol=18.4Hz, ekf=18.7Hz, Id=3.00A, Iq=3.01A, state=STARTING
20580ms: ol=30.4Hz, ekf=25.6Hz, Id=3.00A, Iq=3.00A, state=STARTING
30580ms: ol=45.3Hz, ekf=36.6Hz, Id=3.00A, Iq=3.01A, state=STARTING
39900ms: ol=59.2Hz, ekf=47.7Hz, Id=2.98A, Iq=2.98A, state=STARTING
40000ms: ol=59.4Hz, ekf=286.1Hz, r=-1.98A, q=-0.17A, Id=-0.23A, Vq=24.00V, state=RUNNING
```

分段统计：

```text
ol=18-40Hz 区间：
- EKF 平均约 25.2Hz
- EKF 最大约 33.0Hz
- ol 与 EKF 平均差约 4.0Hz

ol=40-59Hz 区间：
- EKF 平均约 39.8Hz
- EKF 最大约 47.7Hz
- ol 与 EKF 平均差约 9.6Hz
```

判断修正：

- 不能再说“转子根本没跟住开环磁场”，更准确的说法是：低频/中频开环阶段已经能跟住一部分，而且体感能转；真正失控发生在 `STARTING -> RUNNING` 的闭环接管瞬间。
- 40s 启动检查通过后，状态机切到 `RUNNING`，控制分支从开环角度切到 EKF/速度闭环。
- 切换瞬间 `Id` 从 3A 掉到 0A，`EKF` 出现 286Hz 尖峰，速度环输出打到负电流，`Vq` 顶到 24V，上一次稳定开环被直接打崩。
- 这说明下一步重点不是再加启动电流，而是延后/取消粗暴闭环接管，先证明开环保持能否稳定运行。

同步代码修改：

```text
motor/foc_define_parameter.h
- 新增 COMPRESSOR_OPEN_LOOP_HOLD_ENABLE = 1
- COMPRESSOR_OPEN_LOOP_MAX_HZ 从 60Hz 降到 45Hz，停在 trace 显示较稳定的区间附近。

motor/adc.c
- 在 COMPRESSOR_OPEN_LOOP_HOLD_ENABLE 启用时，RUNNING 状态也继续走开环强拖分支。
- 这样 40s 状态切到 RUNNING 后，不再立刻切 EKF/速度闭环，也不会把 Id 从 3A 清掉。

README.md / codex.md
- 同步记录当前为开环保持诊断版。
```

编译与烧录：

```text
Build firmware 成功
FLASH: 27336 B / 512 KB
RAM: 71512 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一次验证目标：

- 让压缩机跑到 `ol=45Hz` 附近后保持。
- 如果它能一直稳定转、有吸力、不再 40s 后打崩，就证明主问题是闭环接管策略。
- 若开环保持也丢步，再回头看 trace 判断是 45Hz 本身过高，还是机械负载/相序/采样映射问题。

## 29. Step 9：45Hz 开环保持稳定运行，EKF 估计不可信

烧录“45Hz 开环保持诊断版”后测试，压缩机表现明显变好。

我的反馈数据：

```text
感觉转的一点问题都没有
吸力非常稳定，而且很有劲
期间用手指堵了几次吸气口，能感受到明显吸力
台架电源电流稳定在约 1A
没有异常
测试后手动停机，未复位，导出 trace
```

导出结果：

```text
Trace CSV: build/trace_openloop_stable.csv
samples = 2048
period_ms = 20
wrapped = 1
覆盖最后约 41s
active = 0，说明停机后 trace 已冻结
```

稳定区间统计：

```text
ol 平均/保持：45.0Hz
EKF 平均：-21.57Hz
EKF 范围：-24.80Hz 到 -18.10Hz
Iq_ref 平均：3.00A
Iq_fb 平均：3.00A，范围 2.95A 到 3.07A
Id_fb 平均：3.00A，范围 2.92A 到 3.09A
Vbus 平均：11.62V，范围 11.43V 到 11.83V
Vd 平均：0.08V，范围 -0.76V 到 0.91V
Vq 平均：2.40V，范围 1.75V 到 2.76V
Iq 平均误差：约 0.010A
```

判断：

- 当前硬件、相线、功率级和电流环已经可以让压缩机在 12V 下稳定开环运行。
- 45Hz 电频率、`Id=3A/Iq=3A` 是目前实测稳定、有吸力的安全工作点。
- 电源电流约 1A，结合 trace 中 `Vbus≈11.62V`、`Vq≈2.4V`，说明这不是虚假的 OLED 显示，而是真实在做功。
- 但 EKF 在稳定运行时长期显示约 `-21Hz`，与开环命令 `ol=45Hz` 方向和幅值都不一致。
- 因此上一轮闭环接管炸掉，根因不只是切换瞬间太粗暴，更重要的是 EKF 当前估计值不能直接用于速度闭环/角度闭环。

阶段结论：

- 当前应把“45Hz 开环保持”作为新的安全基线。
- 在 EKF 的方向、尺度、参数和输入符号校正前，不应再让固件自动切入 EKF/速度闭环。
- 下一步应围绕 EKF 校准展开：确认 `Voltage_Alpha_Beta`、`Current_Ialpha_beta`、`Rs/Ls/flux`、相序和速度符号，使 EKF 在开环稳定运行时至少满足方向一致、量级接近。

## 30. 给后续博客的素材线索

这套项目适合写成一个分阶段博客系列：

1. 从开源 STM32 FOC 工程接手开始，先判断 HAL/标准库和工程结构。
2. 根据网表把别人的代码适配到自己的 DRV8301 硬件。
3. 在没有 USB 上位机的情况下，如何用 OLED、按键和 ST-Link 做最小可调试闭环。
4. 为什么压缩机驱动不能一上来就按启动：低压、限流、空载、采样偏置的意义。
5. IA/IB 为什么要用电压档测，而不是电流档。
6. 从 1.646V 的电流采样偏置和 0.567V 的 Vbus 分压，判断板子静态健康。
7. 后续再展开 EKF、无感启动、相序判断、参数整定和产品级保护。

## 31. 45Hz 开环稳定运行第二次导出

在确认压缩机已经手动停机、未复位的情况下，第二次从 RAM trace 导出 CSV：

```text
Trace CSV: build/trace_repeat2_45hz.csv
samples = 2048
period_ms = 20
wrapped = 1
total_sample_count = 2182
active = 0
```

稳定区间筛选条件为 `ol >= 44.5Hz` 且 `motor = 1`，共 681 条记录，约覆盖 13.6s：

```text
ol 平均：44.99Hz，范围 44.50Hz 到 45.00Hz
EKF 平均：36.82Hz，范围 35.10Hz 到 37.60Hz
Iq_ref 平均：3.00A
Iq_fb 平均：3.00A，范围 2.96A 到 3.03A
Id_fb 平均：3.00A，范围 2.95A 到 3.04A
Vbus 平均：11.60V，范围 11.39V 到 11.70V
Vd 平均：0.03V，范围 -0.17V 到 0.31V
Vq 平均：2.38V，范围 2.24V 到 2.56V
Iq 平均误差：约 0.010A，最大约 0.040A
```

时间点摘录：

```text
10.00s: ol=14.5Hz, ekf=16.7Hz, Iq=3.01A, Id=2.99A, Vbus=11.73V, STARTING
20.00s: ol=29.6Hz, ekf=26.0Hz, Iq=3.00A, Id=3.01A, Vbus=11.70V, STARTING
30.00s: ol=44.5Hz, ekf=35.7Hz, Iq=2.99A, Id=2.99A, Vbus=11.55V, STARTING
40.00s: ol=45.0Hz, ekf=37.1Hz, Iq=3.00A, Id=2.96A, Vbus=11.55V, RUNNING
```

判断修正：

- 第二次和上一条 repeat 结果一致：45Hz 开环运行时 EKF 为正，平均约 `36.8Hz`，不再是第一次 `trace_openloop_stable.csv` 里的 `-21Hz`。
- 电流环仍然非常稳，`Id/Iq` 都贴着 3A，母线没有塌陷，说明当前 45Hz 开环基线本身可靠。
- EKF 的方向现在看起来可能是对的，但幅值仍比开环 45Hz 低约 18%，并且不同导出之间出现过 `-21Hz` 与 `+36.8Hz` 的明显差异。
- 下一步不能直接闭环接管；应先继续做 EKF 复现实验和尺度校准，确认这是初始化/符号问题，还是 `Rs/Ls/flux/采样周期/极对数` 参数导致的比例偏差。

## 32. 45Hz 开环稳定运行第三次导出，断电重启后复现

这次按计划做了完整断电重启复现实验：

```text
按 KEY1 停机
关闭 12V 台架电源，等待后重新上电
OLED 正常显示后按 KEY1 启动
运行到 ol=45Hz 并稳定吸气
按 KEY1 停机
未复位、未断电，导出 trace
```

我的现场反馈：

```text
稳定吸气
稳定电源电流约 1A
EKF 大概稳定在 36Hz
没有 OCTW/FAULT
没有异常抖动/声音
```

导出结果：

```text
Trace CSV: build/trace_repeat3_powercycle_45hz.csv
samples = 2048
period_ms = 20
wrapped = 1
total_sample_count = 2831
active = 0
```

稳定区间筛选条件为 `ol >= 44.5Hz` 且 `motor = 1`，共 1330 条记录：

```text
ol 平均：45.00Hz，范围 44.50Hz 到 45.00Hz
EKF 平均：36.83Hz，范围 35.20Hz 到 37.60Hz
Iq_ref 平均：3.00A
Iq_fb 平均：3.00A，范围 2.96A 到 3.04A
Id_fb 平均：3.00A，范围 2.96A 到 3.04A
Vbus 平均：11.53V，范围 10.85V 到 11.70V
Vq 平均：2.38V，范围 2.23V 到 2.53V
Iq 平均误差：约 0.010A，最大约 0.040A
```

与前两次 repeat 对比：

```text
repeat1: EKF 平均 36.70Hz，Vbus 平均 11.60V，Vq 平均 2.39V
repeat2: EKF 平均 36.82Hz，Vbus 平均 11.60V，Vq 平均 2.38V
repeat3: EKF 平均 36.83Hz，Vbus 平均 11.53V，Vq 平均 2.38V
```

判断修正：

- 完整断电重启后仍然复现 `EKF≈36.8Hz`，所以当前稳定工况下 EKF 的方向基本可以先视为正向。
- 三次 repeat 对齐后，当前主要问题从“EKF 方向乱跳”收敛为“EKF 速度幅值约为开环频率的 0.818 倍”。
- 45Hz 开环基线已经可以作为后续 EKF 标定的稳定试验台。
- 下一步应做多频点开环标定，例如 30Hz、45Hz、60Hz，对比 `EKF/ol` 比例是否固定。如果比例固定，优先查极对数/采样周期/速度单位换算；如果比例随频率变化，优先查 `Rs/Ls/flux` 和 EKF 模型参数。

## 33. 多频点 EKF 标定版

为了继续验证 EKF 速度估计偏差的性质，固件改成了多频点开环标定版。

代码修改：

```text
motor/foc_define_parameter.h
- 新增 COMPRESSOR_OPEN_LOOP_CAL_HZ_LOW = 30Hz
- 新增 COMPRESSOR_OPEN_LOOP_CAL_HZ_MID = 45Hz
- 新增 COMPRESSOR_OPEN_LOOP_CAL_HZ_HIGH = 60Hz
- 新增 COMPRESSOR_OPEN_LOOP_DEFAULT_HZ = 30Hz

motor/adc.c / motor/adc.h
- 新增 compressor_open_loop_target_hz
- 开环速度不再固定爬到 COMPRESSOR_OPEN_LOOP_MAX_HZ，而是爬到当前 target/cap。

motor/low_task.c
- KEY3 仅在停机时循环切换 cap：30 -> 45 -> 60 -> 30。
- 运行中按 KEY3 不改变 cap，避免对正在运行的压缩机做突变扰动。

user/oled_display.c
- OLED 第二页增加 cap 显示：
  ol:xxx cap:xxx
```

构建与烧录：

```text
Build firmware 成功
FLASH: 27564 B / 512 KB
RAM: 71512 B / 128 KB
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一轮实验顺序：

```text
1. 上电后默认 cap=030，直接按 KEY1 启动，跑 30Hz。
2. 跑稳后按 KEY1 停机，不复位、不掉电，导出 CSV。
3. 停机状态按 KEY3，让 cap=045，再按 KEY1 启动。
4. 跑稳后按 KEY1 停机，不复位、不掉电，导出 CSV。
5. 停机状态按 KEY3，让 cap=060，再按 KEY1 启动。
6. 跑稳后按 KEY1 停机，不复位、不掉电，导出 CSV。
```

每个频点需要记录：

```text
是否稳定吸气
稳定电源电流
OLED 上 EKF 大概稳定值
是否有 OCTW/FAULT
是否有异常抖动/声音
```

判断逻辑：

- 如果 `EKF/ol` 在 30/45/60Hz 近似固定，优先怀疑速度单位换算、极对数、采样周期或 EKF 输出尺度。
- 如果 `EKF/ol` 随频率明显变化，优先怀疑电机模型参数 `Rs/Ls/flux` 或 EKF 输入量尺度。
- 任何频点出现明显异常抖动、失吸、过流或故障，都先停机，不继续更高频点。

## 34. 多频点 EKF 标定：30Hz 导出

30Hz 频点测试完成后，在停机、未复位、未掉电状态下导出 trace：

```text
Trace CSV: build/trace_cal_30hz.csv
samples = 2048
period_ms = 20
wrapped = 1
total_sample_count = 2175
active = 0
```

稳定区间筛选条件为 `29.5Hz <= ol <= 30.5Hz` 且 `motor = 1`，共 1177 条记录：

```text
ol 平均：30.00Hz，范围 29.50Hz 到 30.00Hz
EKF 平均：-16.08Hz，范围 -16.70Hz 到 -14.30Hz
EKF/ol 平均：-0.536
Iq_ref 平均：3.00A
Iq_fb 平均：3.00A，范围 2.96A 到 3.04A
Id_fb 平均：3.00A，范围 2.94A 到 3.03A
Vbus 平均：11.68V，范围 11.55V 到 11.78V
Vd 平均：0.18V，范围 0.01V 到 0.41V
Vq 平均：1.80V，范围 1.66V 到 1.98V
Iq 平均误差：约 0.008A，最大约 0.040A
```

时间点摘录：

```text
10.00s: ol=14.5Hz, ekf=-11.7Hz, Iq=3.00A, Id=3.00A, Vbus=11.73V, STARTING
20.00s: ol=29.6Hz, ekf=-15.8Hz, Iq=3.00A, Id=3.01A, Vbus=11.68V, STARTING
30.00s: ol=30.0Hz, ekf=-16.0Hz, Iq=3.00A, Id=3.01A, Vbus=11.63V, STARTING
40.00s: ol=30.0Hz, ekf=-16.0Hz, Iq=2.99A, Id=3.00A, Vbus=11.70V, RUNNING
```

阶段判断：

- 30Hz 下电流环仍然稳定，`Id/Iq` 都能贴住 3A，母线电压没有塌陷。
- `Vq≈1.80V` 明显低于 45Hz 时的 `Vq≈2.38V`，符合频率更低、反电动势更低的预期。
- 但 EKF 在 30Hz 稳定段为负，平均 `-16.08Hz`，与 45Hz repeat 中稳定的 `+36.7-36.8Hz` 不一致。
- 这说明 EKF 问题不只是简单的固定比例偏差；很可能存在低频可观测性、初始化方向、模型参数或输入符号耦合问题。
- 下一步继续测 45Hz 和 60Hz；如果 45Hz 再次为正、60Hz 也为正，则 30Hz 可能处在 EKF 低速不可靠区。如果 45Hz/60Hz 也出现符号翻转，则优先排查 EKF 初始化和输入符号。

## 35. 多频点 EKF 标定：45Hz 导出，复现负向 EKF

45Hz 频点测试完成后，在停机、未复位、未掉电状态下导出 trace。

我的现场反馈：

```text
cap = 45Hz
吸力正常
台架电源电流约 1A
OLED 上 EKF 一直显示 0
没有异常声音
```

导出结果：

```text
Trace CSV: build/trace_cal_45hz.csv
samples = 2048
period_ms = 20
wrapped = 1
total_sample_count = 2692
active = 0
```

稳定区间筛选条件为 `44.5Hz <= ol <= 45.5Hz` 且 `motor = 1`，共 1191 条记录：

```text
ol 平均：45.00Hz，范围 44.50Hz 到 45.00Hz
EKF 平均：-21.93Hz，范围 -22.60Hz 到 -20.30Hz
EKF/ol 平均：-0.487
Iq_ref 平均：3.00A
Iq_fb 平均：3.00A，范围 2.96A 到 3.03A
Id_fb 平均：3.00A，范围 2.96A 到 3.05A
Vbus 平均：11.55V，范围 11.31V 到 11.67V
Vq 平均：2.38V，范围 2.21V 到 2.53V
Iq 平均误差：约 0.010A，最大约 0.040A
```

时间点摘录：

```text
20.00s: ol=29.6Hz, ekf=-15.9Hz, Iq=3.01A, Id=3.00A, Vbus=11.63V, STARTING
30.00s: ol=44.5Hz, ekf=-21.2Hz, Iq=2.98A, Id=2.99A, Vbus=11.51V, STARTING
40.00s: ol=45.0Hz, ekf=-22.2Hz, Iq=2.99A, Id=2.98A, Vbus=11.48V, RUNNING
50.00s: ol=45.0Hz, ekf=-21.7Hz, Iq=3.00A, Id=2.98A, Vbus=11.56V, RUNNING
```

阶段判断：

- 45Hz 下机械表现、母线和电流环仍然正常，说明开环 FOC 做功稳定。
- Trace 中稳定段没有接近 0 的 EKF 样本；OLED 显示 0 不是观测器真的为 0，更可能是当前 OLED 用无符号数字显示 `EKF_Hz`，遇到负值时显示失真。
- 45Hz 曾经三次 repeat 得到 `+36.7-36.8Hz`，这次在多频点测试中又复现 `-21.9Hz`，说明 EKF 存在明显的符号/初始化分支问题。
- 下一步仍然先测 60Hz，判断更高反电动势下 EKF 是否稳定转向正值；之后应优先修正 EKF 显示为 signed，并排查 EKF 初始化、角度初值、输入电压/电流符号。

## 36. 多频点 EKF 标定：60Hz 导出与阶段结论

60Hz 频点测试完成后，在停机、未复位、未掉电状态下导出 trace。

我的现场反馈：

```text
cap = 60Hz
吸力比 45Hz 更大，非常好
台架电源电流约 1.27A
声音正常
没有故障
```

导出结果：

```text
Trace CSV: build/trace_cal_60hz.csv
samples = 2048
period_ms = 20
wrapped = 1
total_sample_count = 3316
active = 0
```

稳定区间筛选条件为 `59.5Hz <= ol <= 60.5Hz` 且 `motor = 1`，共 1311 条记录：

```text
ol 平均：60.00Hz
EKF 平均：-28.33Hz，范围 -29.10Hz 到 -26.00Hz
EKF/ol 平均：-0.472
Iq_fb 平均：3.00A
Id_fb 平均：3.00A
Vbus 平均：11.51V，范围 11.29V 到 11.63V
Vd 平均：-0.06V，范围 -0.36V 到 0.50V
Vq 平均：2.97V，范围 2.83V 到 3.19V
Iq 平均误差：约 0.013A，最大约 0.040A
```

时间点摘录：

```text
30.00s: ol=44.5Hz, ekf=-22.0Hz, Iq=3.00A, Id=3.00A, Vbus=11.56V, STARTING
40.00s: ol=59.4Hz, ekf=-27.3Hz, Iq=3.02A, Id=3.03A, Vbus=11.43V, RUNNING
50.00s: ol=60.0Hz, ekf=-27.8Hz, Iq=2.99A, Id=2.99A, Vbus=11.50V, RUNNING
60.00s: ol=60.0Hz, ekf=-28.1Hz, Iq=3.00A, Id=3.00A, Vbus=11.56V, RUNNING
```

三频点对比：

```text
30Hz: EKF 平均 -16.08Hz, EKF/ol = -0.536, Vq 平均 1.80V
45Hz: EKF 平均 -21.93Hz, EKF/ol = -0.487, Vq 平均 2.38V
60Hz: EKF 平均 -28.33Hz, EKF/ol = -0.472, Vq 平均 2.97V
```

阶段判断：

- 30/45/60Hz 三个频点机械表现均正常，60Hz 甚至吸力更强，说明开环 FOC、电流采样、电流环、SVPWM 和功率级整体是能稳定做功的。
- `Vq` 随开环频率从约 `1.80V -> 2.38V -> 2.97V` 增加，符合反电动势随转速上升的趋势。
- EKF 在本组三频点全部为负，且绝对值随频率上升。这说明 EKF 不是完全无响应，而是在以错误方向/错误输入符号跟随某个速度相关量。
- 因为 45Hz 之前曾三次出现 `+36.7-36.8Hz`，当前仍不能直接断定只需把 EKF 取反；更稳妥的下一步是先修 OLED signed 显示和 trace 里额外记录 EKF 角度/输入电压电流符号，然后做“只改一个符号”的诊断版本。
- 当前禁止闭环接管的判断不变；下一步优先排查 EKF 的输入符号、角度初始化和输出符号，而不是继续提高开环频率或电流。

## 37. EKF 初始化时序检查与诊断补丁

新的现场观察：

```text
之前 EKF=+36Hz 的几次：
- 按 KEY1 后，1s 转子定位结束，EKF 立刻跳到约 8Hz
- 随后 EKF 慢慢跟着 ol 往上走

多频点 30/45/60Hz 这三次：
- OLED 上 EKF 一直显示 0
- trace 实际显示 EKF 为负值：30Hz=-16Hz，45Hz=-22Hz，60Hz=-28Hz
```

重新检查中断与调用时序：

```text
TIM1 center-aligned PWM 运行
TIM1 CC4 触发 ADC injected conversion
ADC JEOC 中断触发 ADC_IRQHandler()
ADC_IRQHandler() 里调用 motor_run()
motor_run() 里调用 foc_algorithm_step()
foc_algorithm_step() 里先计算电流环/SVPWM，再执行 stm32_ekf_Outputs_wrapper 和 stm32_ekf_Update_wrapper

SysTick 1ms
每 10ms 执行 low_control_task()
low_control_task() 处理 KEY1/KEY2/KEY3、状态机和 trace_sample_10ms()
```

关键发现：

- FOC/EKF 主计算不在 TIM1 update 中断里；`TIM1_UP_TIM10_IRQHandler()` 只清标志，而且 TIM1 update interrupt 当前被注释掉。
- ADC 中断优先级为 preemption priority 1；SysTick 在 GCC 路径下没有被显式改 NVIC 优先级，实际不应高于 ADC。
- KEY1 在 `low_control_task()` 中先把 `motor_start_stop` 从 0 改成 1，然后后面才调用 `motor_start()` 做 `foc_algorithm_initialize()`。
- 因为 ADC 中断优先级高于 SysTick，理论上存在一个窗口：`motor_start_stop=1` 已经成立，但 EKF/FOC 还没初始化完成，ADC JEOC 抢占进来跑一次 `motor_run()`。
- 这个窗口虽然很短，但正好能解释 EKF 有时落入不同初始分支，尤其是 EKF 这种对初始条件敏感的观测器。

同步代码修改：

```text
motor/low_task.c / motor/low_task.h
- 新增 volatile uint8_t motor_control_ready。
- motor_start() 一开始先置 ready=0。
- foc_algorithm_initialize()、compressor_reset_control()、compressor_open_loop_reset()、trace_reset()、状态机 STARTING 均完成后，才 enable PWM 并置 ready=1。
- motor_stop()、compressor_fault_trip()、compressor_clear_fault() 先置 ready=0，再停 PWM/复位控制量。

motor/adc.c / motor/adc.h
- ADC_IRQHandler() 只有在 get_offset_flag==2 且 motor_control_ready!=0 时才推进 hall_angle 并调用 motor_run()。
- 新增 compressor_open_loop_reset()，显式复位开环启动计数、ol 频率、hall_angle 和 hall_angle_add。
- 新增 compressor_aligning_flag，供 trace 记录定位阶段。

user/oled_display.c
- EKF 显示改为 signed Hz，负值会显示为 -016/-022/-028，不再误显示成 0。

motor/trace.c / motor/trace.h
- trace record 从 32 字节扩展到 48 字节。
- 新增记录 target_hz、FOC_Input.theta、EKF angle、Valpha/Vbeta、Ialpha/Ibeta、diag_flags。
- diag_flags bit0=control_ready，bit1=aligning，bit2=open_loop_force_branch。

tools/export-trace.ps1
- 支持旧 32 字节和新 48 字节 trace record。
- 新 CSV 表头增加 target_hz/foc_theta_rad/ekf_angle_rad/valpha_v/vbeta_v/ialpha_a/ibeta_a/diag_flags。
```

构建、烧录与导出格式自检：

```text
Build firmware 成功
FLASH: 28624 B / 512 KB
RAM: 104280 B / 128 KB，约 79.56%
Flash firmware 成功
Download verified successfully
MCU reset 成功

空 trace 导出自检成功：
Trace CSV: build/trace_diag_format_check.csv
samples = 0
record_size = 48
```

下一轮验证目标：

- 从默认 `cap=030` 开始跑一次，重点看 OLED 是否显示负 EKF，而不是 0。
- 导出 trace 后确认 `diag_flags` 在启动后为 `7` 左右：ready=1、aligning=1、open_loop=1；定位结束后应变为 `5`：ready=1、aligning=0、open_loop=1。
- 观察 1s 定位结束时 EKF 是否还会随机跳到正分支；如果 ready 门闩后正负分支更一致，说明初始化抢跑窗口确实参与了问题。

## 38. ready 门闩后 30Hz 复测

烧录 EKF 初始化时序诊断补丁后，重新测试默认 `cap=030Hz`。

我的现场反馈：

```text
30Hz 新版已停机，导出
EKF 定位后立刻显示：-8
EKF 稳定后显示：-16
吸力：有
电流：830mA
声音/故障：无
```

导出结果：

```text
Trace CSV: build/trace_diag_30hz_ready.csv
samples = 2048
period_ms = 20
wrapped = 1
total_sample_count = 2430
active = 0
```

稳定区间筛选条件为 `29.5Hz <= ol <= 30.5Hz` 且 `motor = 1`，共 1432 条记录：

```text
ol 平均：29.997Hz
target 平均：30.000Hz
EKF 平均：-16.158Hz，范围 -16.7Hz 到 -14.5Hz
EKF/ol 平均：-0.539
Iq_ref 平均：3.000A
Iq_fb 平均：3.001A
Id_fb 平均：3.001A
Vbus 平均：11.641V
Vd 平均：0.167V
Vq 平均：1.790V
Iq 平均误差：约 0.008A
稳定段 diag_flags 全部为 5
```

`diag_flags=5` 的含义：

```text
bit0 = 1: motor_control_ready 已置位
bit1 = 0: 已过 1s 定位阶段
bit2 = 1: 正在走 open-loop force branch
```

阶段判断：

- OLED signed 显示修正有效：这次不再显示 0，而是现场能看到 `-8 -> -16`。
- ready 门闩后，30Hz 仍然稳定落在负 EKF 分支，说明负分支不只是“初始化未完成前 ADC 抢跑”造成的。
- 因为本次运行超过 41s，环形 trace 覆盖掉了启动最早 7.6s，没有抓到 1s 定位结束瞬间；因此把 trace 采样从 20ms 改为 40ms，覆盖时间从约 41s 增加到约 82s，便于下一轮同时看到定位、爬坡和稳定段。

## 39. 45Hz 完整 trace 抓到定位阶段 EKF 假速度

40ms trace 版本烧录后，重新测试 `cap=045Hz` 并导出：

```text
Trace CSV: build/trace_diag_45hz_ready.csv
samples = 1206
period_ms = 40
wrapped = 0
total_sample_count = 1206
active = 0
```

稳定区间筛选条件为 `44.5Hz <= ol <= 45.5Hz` 且 `motor = 1`，共 455 条记录：

```text
ol 平均：44.995Hz
target 平均：45.000Hz
EKF 平均：-21.941Hz，范围 -22.6Hz 到 -20.2Hz
EKF/ol 平均：-0.488
Iq_ref 平均：3.000A
Iq_fb 平均：2.999A
Id_fb 平均：2.999A
Vbus 平均：11.520V
Vq 平均：2.381V
Iq 平均误差：约 0.009A
稳定段 diag_flags 全部为 5
```

这次最关键的是启动前 1s 定位阶段没有被覆盖掉。摘录：

```text
t=0ms:    ol=1Hz, ekf=0.0Hz,  Iq_ref=0A, Id≈3A, flags=7, theta=0
t=40ms:   ol=1Hz, ekf=-4.7Hz, Iq_ref=0A, Id≈3A, flags=7, theta=0
t=80ms:   ol=1Hz, ekf=-7.8Hz, Iq_ref=0A, Id≈3A, flags=7, theta=0
t=120ms:  ol=1Hz, ekf=-8.5Hz, Iq_ref=0A, Id≈3A, flags=7, theta=0
t=160ms 到 960ms: EKF 基本稳定在 -8.7Hz，仍处于定位阶段
t=1000ms: 定位结束，flags 从 7 变成 5，开环角度开始转动，EKF 仍约 -8.6Hz
```

判断：

- 定位阶段 `theta=0`、`Iq_ref=0`，转子理论上被 d 轴吸附，没有开环旋转命令。
- EKF 却在定位阶段自己跑到 `-8.7Hz`，这不是实际转速，而是观测器把静止定位阶段的电压/电流瞬态解释成了负速度。
- 这与之前“定位后 EKF 立刻显示 -8”完全对应，也解释了为什么 EKF 会从一开始就落入负分支。

同步代码修改：

```text
motor/foc_algorithm.c / h
- 新增 foc_ekf_update_enable。
- 新增 foc_ekf_reset()，只重置 EKF wrapper、EKF state 和 FOC_Output.EKF，不重置电流环 PI。
- foc_algorithm_step() 在 foc_ekf_update_enable=0 时跳过 stm32_ekf_Outputs_wrapper / Update_wrapper，并把 FOC_Output.EKF 清零。

motor/adc.c
- 定位阶段 aligning=1 时设置 foc_ekf_update_enable=0。
- 检测到 aligning 从 1 变为 0 的瞬间调用 foc_ekf_reset()。
- 定位结束后再设置 foc_ekf_update_enable=1，让 EKF 从开环旋转刚开始的状态重新观察。

motor/trace.c
- diag_flags 新增 bit3：foc_ekf_update_enable。
```

构建与烧录：

```text
Build firmware 成功
FLASH: 28648 B / 512 KB
RAM: 104296 B / 128 KB，约 79.57%
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一轮验证目标：

- 重新跑 45Hz。
- 预期 0-1000ms 定位阶段 OLED 的 `ekf:` 应保持 0 或接近 0，不应再跑到 `-8`。
- 定位结束后 EKF 从 0 重新开始收敛；如果仍然走负分支，再继续查输入电压/电流符号。

## 40. EKF 暂停定位版 45Hz 复测：负分支发生在开环刚开始

烧录“定位阶段暂停 EKF、定位结束重置 EKF”版本后，重新测试 `cap=045Hz`。

我的现场反馈：

```text
45Hz EKF 暂停定位版已停机，导出
1 秒定位阶段 EKF 保持 0
定位结束后 EKF 跳到 -8
吸力、电流、声音、故障均正常
```

导出结果：

```text
Trace CSV: build/trace_diag_45hz_ekf_pause_align.csv
samples = 1159
period_ms = 40
wrapped = 0
total_sample_count = 1159
active = 0
```

稳定区间筛选条件为 `44.5Hz <= ol <= 45.5Hz` 且 `motor = 1`，共 408 条记录：

```text
ol 平均：44.994Hz
target 平均：45.000Hz
EKF 平均：-22.020Hz，范围 -22.6Hz 到 -21.3Hz
EKF/ol 平均：-0.489
Iq_ref 平均：3.000A
Iq_fb 平均：2.998A
Id_fb 平均：2.999A
Vbus 平均：11.507V
Vq 平均：2.380V
稳定段 diag_flags 全部为 13
```

`diag_flags=13` 的含义：

```text
bit0 = 1: motor_control_ready
bit1 = 0: 非定位阶段
bit2 = 1: open-loop force branch
bit3 = 1: EKF update enable
```

启动阶段摘录：

```text
0-960ms: flags=7，定位阶段，EKF 保持 0
1000ms: flags=13，EKF 打开后第一个样本为 -0.9Hz
1040ms: EKF=-6.4Hz
1080ms: EKF=-8.0Hz
之后低速开环阶段维持在约 -8Hz，并随 ol 增大继续负向增长
```

判断：

- “定位阶段污染”已经被修掉；EKF 在 0-960ms 不再自行跑到 -8Hz。
- 负分支是在定位结束后、EKF update 打开后的 40-80ms 内形成的。
- 这更像 EKF 的 αβ 坐标符号/相序方向与当前 FOC 开环方向不一致，而不是状态机时序问题。
- 下一步做单变量符号诊断：只改变送入 EKF 的 `β` 轴电压/电流符号，驱动输出和电流环保持不变。若 EKF 变正且量级更接近 ol，说明 EKF 的坐标系 handedness 与当前 Clarke/SVPWM/相序相反。

## 41. EKF beta 轴输入取反诊断版

基于第 40 步结论，烧录单变量符号诊断版。

代码修改：

```text
motor/foc_define_parameter.h
- 新增 EKF_V_ALPHA_SIGN = +1
- 新增 EKF_V_BETA_SIGN  = -1
- 新增 EKF_I_ALPHA_SIGN = +1
- 新增 EKF_I_BETA_SIGN  = -1

motor/foc_algorithm.c
- 只改变送入 EKF wrapper 的 alpha/beta 输入符号：
  EKF Valpha = +FOC Valpha
  EKF Vbeta  = -FOC Vbeta
  EKF Ialpha = +FOC Ialpha
  EKF Ibeta  = -FOC Ibeta
- SVPWM、电流环、开环角度、压缩机实际驱动完全不变。

motor/trace.c
- trace 中 valpha/vbeta/ialpha/ibeta 改为记录实际送入 EKF 的接口值，而不是原始 FOC alpha/beta 值。
```

构建与烧录：

```text
Build firmware 成功
FLASH: 28648 B / 512 KB
RAM: 104296 B / 128 KB，约 79.57%
Flash firmware 成功
Download verified successfully
MCU reset 成功
```

下一轮验证目标：

- 重新跑 45Hz。
- 机械表现理论上应与上一版一致，因为控制输出没有改。
- 重点看定位结束后 EKF 是否还跳 `-8`，还是变成正值。
- 如果 EKF 变正，说明 EKF 的 beta 轴坐标方向与当前 FOC/相序相反；后续再决定是保留输入变换，还是统一坐标定义。

## 42. EKF beta 轴输入取反 45Hz 复测

烧录 beta 轴输入取反诊断版后，重新测试 `cap=045Hz` 并导出：

```text
Trace CSV: build/trace_diag_45hz_beta_flip.csv
samples = 1183
period_ms = 40
wrapped = 0
total_sample_count = 1183
active = 0
```

稳定区间筛选条件为 `44.5Hz <= ol <= 45.5Hz` 且 `motor = 1`，共 432 条记录：

```text
ol 平均：44.995Hz
target 平均：45.000Hz
EKF 平均：+21.900Hz，范围 +20.4Hz 到 +22.6Hz
EKF/ol 平均：+0.487
Iq_ref 平均：3.000A
Iq_fb 平均：2.999A
Id_fb 平均：3.000A
Vbus 平均：11.549V
Vq 平均：2.380V
Iq 平均误差：约 0.010A
稳定段 diag_flags 全部为 13
```

启动阶段摘录：

```text
0-960ms: flags=7，定位阶段，EKF 保持 0
1000ms: flags=13，EKF=+1.0Hz
1040ms: EKF=+6.6Hz
1080ms: EKF=+8.1Hz
随后 EKF 保持正向并随 ol 增大
30.0s: ol=44.5Hz, EKF=+21.9Hz
```

关键判断：

- 只把送入 EKF 的 `Vbeta/Ibeta` 取反，机械驱动、电流环、SVPWM 均不变，EKF 速度就从 `-22Hz` 翻成 `+21.9Hz`。
- 这基本确认：当前 EKF wrapper 的 αβ 坐标手性与本项目实际 FOC/Clarke/相序定义相反，至少 beta 轴需要取反才能得到正向速度。
- `Vq≈2.38V`、`Id/Iq≈3A` 与取反前几乎一致，说明这个改动确实没有改变实际驱动，只改变了观测器输入。
- 但 `EKF/ol≈0.487`，幅值仍只有开环频率的一半左右；因此 beta 轴取反解决的是“方向”，还没有解决“尺度”。
- Trace 中 `ekf_angle_rad` 饱和到 `-32.768`，说明 EKF 输出角度没有归一化到 `0-2π` 或 `±π`，当前 int16 trace 对角度不够友好。后续应在 trace 侧记录归一化角度，或先只用速度判断。

下一步：

- 先复测 30Hz 和 60Hz beta 取反，确认 `EKF/ol` 比例是否仍在 `0.47-0.54` 附近。
- 如果三个频点都约为 `0.5`，优先查 EKF 速度单位/模型输出尺度，而不是继续改启动策略。
- EKF 未达到接近开环频率前，仍禁止闭环接管。

## 43. EKF beta 轴输入取反 30Hz 复测

烧录 beta 轴输入取反诊断版后，测试 `cap=030Hz` 并导出：

```text
Trace CSV: build/trace_diag_30hz_beta_flip.csv
samples = 928
period_ms = 40
wrapped = 0
total_sample_count = 928
active = 0
```

稳定区间筛选条件为 `29.5Hz <= ol <= 30.5Hz` 且 `motor = 1`，共 429 条记录：

```text
ol 平均：29.994Hz
target 平均：30.000Hz
EKF 平均：+16.209Hz，范围 +15.0Hz 到 +16.8Hz
EKF/ol 平均：+0.540
Iq_ref 平均：3.000A
Iq_fb 平均：3.000A
Id_fb 平均：3.000A
Vbus 平均：11.642V，范围 11.480V 到 11.750V
Vq 平均：1.790V
Iq 平均误差：约 0.008A，最大约 0.030A
稳定段 diag_flags 全部为 13
```

启动阶段摘录：

```text
0-960ms: flags=7，定位阶段，EKF 保持 0
1000ms: EKF=+1.0Hz
1040ms: EKF=+6.6Hz
1080ms: EKF=+8.1Hz
2000ms: ol=2.5Hz, EKF=+7.9Hz
20000ms: ol=29.6Hz, EKF=+16.1Hz
```

与 45Hz beta 反向结果对比：

```text
30Hz: EKF 平均 +16.21Hz, EKF/ol = +0.540, Vq 平均 1.79V
45Hz: EKF 平均 +21.90Hz, EKF/ol = +0.487, Vq 平均 2.38V
```

阶段判断：

- beta 轴取反后，30Hz 和 45Hz 都得到正向 EKF，方向修正结论进一步确认。
- `Id/Iq/Vbus/Vq` 仍然稳定，说明诊断版没有破坏实际驱动。
- `EKF/ol` 在 30Hz 和 45Hz 下都接近 0.5，但不是完全固定；仍需测试 60Hz 判断比例是否继续保持在 `0.47-0.54`。
- 如果 60Hz 也约为 0.5，则下一步优先查 EKF 速度尺度/单位或模型参数，而不是继续动启动策略。

## 44. EKF beta 轴输入取反 60Hz 复测

烧录 beta 轴输入取反诊断版后，测试 `cap=060Hz` 并导出：

```text
Trace CSV: build/trace_diag_60hz_beta_flip.csv
samples = 1524
period_ms = 40
wrapped = 0
total_sample_count = 1524
active = 0
```

稳定区间筛选条件为 `59.5Hz <= ol <= 60.5Hz` 且 `motor = 1`，共 521 条记录：

```text
ol 平均：59.996Hz
target 平均：60.000Hz
EKF 平均：+28.221Hz，范围 +26.2Hz 到 +29.0Hz
EKF/ol 平均：+0.470
Iq_ref 平均：3.000A
Iq_fb 平均：3.002A
Id_fb 平均：3.001A
Vbus 平均：11.471V，范围 11.260V 到 11.630V
Vq 平均：2.971V
Iq 平均误差：约 0.014A，最大约 0.040A
稳定段 diag_flags 全部为 13
```

三频点 beta 反向对比：

```text
30Hz: EKF 平均 +16.21Hz, EKF/ol = +0.540, Vq 平均 1.79V
45Hz: EKF 平均 +21.90Hz, EKF/ol = +0.487, Vq 平均 2.38V
60Hz: EKF 平均 +28.22Hz, EKF/ol = +0.470, Vq 平均 2.97V
```

阶段判断：

- beta 轴取反后，30/45/60Hz 全部得到正向 EKF，方向问题已基本定位。
- EKF 速度幅值约为开环频率的一半，且随频率升高比例从 `0.54` 降到 `0.47`，不是单纯随机噪声。
- 低阻测量要注意：三线压缩机若是星形绕组，万用表测到的 U-V 相间电阻是两相串联；电机模型里的 `Rs` 通常应填单相电阻，所以应取相间电阻的一半再扣除表笔/线阻。之前手测 U-V≈0.5Ω，则粗略单相 `Rs≈0.25Ω`，但 400Ω 档低阻精度有限。
- 不过“EKF 速度约半”更像磁链 `flux` 偏大或 EKF 输出尺度问题。EKF 方程中反电动势项约为 `flux * omega`；如果 `flux` 填大约 2 倍，观测器会用约 1/2 的 `omega` 去解释同样的反电动势。
- 当前 `FLUX_PARAMETER=0.00650Wb` 可能偏大；下一步做只影响 EKF 的磁链半值诊断，把 `flux` 临时改为 `0.00325Wb`，看 45Hz 下 EKF 是否接近 `ol=45Hz`。

## 45. EKF 磁链半值诊断版

为了验证 “EKF 约为 ol 一半” 是否由磁链参数偏大导致，烧录磁链半值诊断版。

代码修改：

```text
motor/foc_define_parameter.h
- FLUX_PARAMETER: 0.00650f -> 0.00325f
```

保留不变：

```text
EKF beta 轴输入取反仍启用
Id/Iq 开环启动策略不变
SVPWM、电流环、开环角度不变
```

判断逻辑：

- EKF 方程中的反电动势项近似为 `flux * omega`。
- 如果之前 `flux` 填得约大 2 倍，那么 EKF 会用约 1/2 的 `omega` 去解释同样的反电动势。
- 把 `flux` 减半后，如果 45Hz 下 EKF 接近 45Hz，说明主要问题是磁链参数尺度；如果仍约 22Hz，则继续查速度单位/采样周期/模型输出解释。

## 46. EKF 磁链半值 45Hz 复测

烧录 `FLUX_PARAMETER=0.00325Wb`、beta 轴取反保持启用后，测试 `cap=045Hz` 并导出：

```text
Trace CSV: build/trace_diag_45hz_flux_half.csv
samples = 1270
period_ms = 40
wrapped = 0
total_sample_count = 1270
active = 0
```

稳定区间筛选条件为 `44.5Hz <= ol <= 45.5Hz` 且 `motor = 1`，共 519 条记录：

```text
ol 平均：44.996Hz
target 平均：45.000Hz
EKF 平均：+27.286Hz，范围 +25.7Hz 到 +28.0Hz
EKF/ol 平均：+0.606
Iq_ref 平均：3.000A
Iq_fb 平均：3.002A
Id_fb 平均：3.001A
Vbus 平均：11.565V，范围 11.430V 到 11.650V
Vq 平均：2.365V
Iq 平均误差：约 0.010A，最大约 0.040A
稳定段 diag_flags 全部为 13
```

与 flux 半值前的 45Hz beta 反向版对比：

```text
flux = 0.00650Wb: EKF 平均 +21.90Hz, EKF/ol = +0.487
flux = 0.00325Wb: EKF 平均 +27.29Hz, EKF/ol = +0.606
```

阶段判断：

- 磁链减半后 EKF 确实上升，说明 `flux` 参数影响方向正确。
- 但 EKF 只从 `21.9Hz` 提到 `27.3Hz`，没有接近 `45Hz`，所以半速问题不是单独由 `flux` 大 2 倍造成的。
- 用户提醒“相间电阻可能应该填一半”是合理方向。三相星形绕组从 U-V 端子测得的是两相串联电阻；EKF/PMSM 模型里的 `Rs` 通常是单相电阻。若 `0.376Ω` 来自端子间/相间电阻，则模型应先试 `Rs≈0.188Ω`。用户用万用表测 `U-V≈0.5Ω`，也提示单相可能在 `0.25Ω` 附近，但低阻档误差较大。
- 下一步应做 `Rs` 半值诊断：把 `RS_PARAMETER=0.376f` 改为 `0.188f`，并先把 `FLUX_PARAMETER` 恢复到 `0.00650f`，这样可以单独验证“Rs 是否填成了相间值”。如果 EKF 明显上升，再做 `Rs/flux` 组合标定。

## 47. 用户重新确认 6MD030Z 手册参数，并修复 EKF

用户说明扫描版规格书参数如下：

```text
极数：8 极，也就是 4 对极
相间电阻：U-V = V-W = W-U = 0.376Ω
Ke：2.96 V/kRPM
Kt：0.049 N·m/A
转动惯量：2.2E-05，备注为转子单体
无通电单向 d 轴电感：0.13mH
无通电单向 q 轴电感：0.18mH
50Hz 通电电感：
  1A: Lq=0.46mH, Ld=0.45mH
  2A: Lq=0.40mH, Ld=0.37mH
  3A: Lq=0.38mH, Ld=0.34mH
```

代码审查结论：

- 当前 `stm32_ekf_wrapper.c` 并不是完整机械模型 EKF。它的输入只有 `Valpha/Vbeta/Ialpha/Ibeta/Rs/Ls/flux`，状态是 `Ialpha/Ibeta/omega/theta`。
- 这个 wrapper 里没有使用极对数和转动惯量；`xD[2]` 是电角速度，`xD[3]` 是电角度。8 极/4 对极主要用于把电频率换算为机械 rpm，例如 45Hz 电频率约等于 `45/4*60 = 675rpm`。
- 发现一个明确的 EKF 代码问题：Kalman 增益计算 `K = P H^T S^-1` 时，原代码先覆盖 `K_0_0/K_1_0/...`，随后计算第二列 `K_0_1/K_1_1/...` 又使用了已经覆盖后的第一列，导致 EKF 更新矩阵被算歪。

代码修改：

```text
motor/foc_define_parameter.h
- RS_PARAMETER:   0.376f -> 0.188f
- LS_PARAMETER:   0.00020f -> 0.00036f
- FLUX_PARAMETER: 0.00325f -> 0.00580f

motor/stm32_ekf_wrapper.c
- Kalman 增益矩阵乘法改为先保存 K 原始值，再计算两列，避免覆盖污染。
- EKF 角度归一化补上 `<0` 时加 `2π`，后续闭环接管时角度更稳。
```

参数判断：

- `Rs=0.188Ω`：因为用户确认 `0.376Ω` 是端子相间电阻，三相星形等效到模型单相电阻应约为一半。
- `Ls=0.36mH`：当前 EKF 只有一个 `Ls`，无法分别使用 `Ld/Lq`，所以按 3A 工作区间 `Ld=0.34mH`、`Lq=0.38mH` 取平均。
- `flux=0.00580Wb`：按 `Ke=2.96 V/kRPM` 和 `Kt=0.049 N·m/A` 的尺度恢复到合理磁链量级，不继续使用仅用于诊断的半值 `0.00325Wb`。

验证：

```text
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\vscode-build.ps1 build
结果：成功

powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\vscode-build.ps1 flash
结果：成功，ST-LINK 下载并复位
```

下一步现场测试：保持 12V 限流，先测 `cap=45Hz`，观察 OLED 的 `ekf:` 是否相比上一版的 `+27Hz` 更接近 `ol=45Hz`，停机后导出 trace。

## 48. 确认版参数 + EKF 增益修复后 45Hz 导出

用户测试停机后导出：

```text
Trace CSV: build/trace_diag_45hz_confirmed_params_ekf_fix_beta_flip.csv
samples = 1364
period_ms = 40
wrapped = 0
active = 0
```

稳定区间筛选条件为 `44.5Hz <= ol <= 45.5Hz` 且 `diag_flags = 13`、`fault = 0`，共 613 条：

```text
ol 平均：44.996Hz
EKF 平均：-43.107Hz，范围 -43.5Hz 到 -42.6Hz
EKF/ol 平均：-0.958
Iq_fb 平均：3.001A
Id_fb 平均：3.001A
Vbus 平均：11.586V
Vq 平均：2.377V
```

阶段判断：

- 和上一轮 `flux=0.00325Wb` 的 `EKF≈+27.3Hz` 相比，这次 EKF 幅值已经接近开环 45Hz，说明“半速问题”主要不是 beta 符号，而是电机参数和 EKF 增益计算共同造成。
- 当前仍保持 beta 取反，所以结果为负向 `-43Hz`；用户判断“beta 的符号也不用倒过来”与 trace 一致。
- 已把 `EKF_V_BETA_SIGN` 和 `EKF_I_BETA_SIGN` 从 `-1.0f` 恢复为 `+1.0f`，并重新 build/flash。

下一步：复测 `cap=45Hz`，预期 OLED `ekf:` 应接近 `+43Hz` 到 `+45Hz`；如果成立，再继续 30Hz/60Hz 三点验证。

## 49. beta 正常符号后三频点 EKF 复测

恢复 `EKF_V_BETA_SIGN=+1.0f`、`EKF_I_BETA_SIGN=+1.0f` 后，用户依次测试 30Hz、45Hz、60Hz 并停机导出。

导出文件：

```text
build/trace_diag_30hz_confirmed_params_ekf_fix_beta_normal.csv
build/trace_diag_45hz_confirmed_params_ekf_fix_beta_normal.csv
build/trace_diag_60hz_confirmed_params_ekf_fix_beta_normal.csv
```

稳定段统计：

```text
30Hz:
  ol 平均：29.996Hz
  EKF 平均：+29.207Hz，范围 +28.5Hz 到 +29.7Hz
  EKF/ol：0.974
  Iq/Id：2.999A / 3.000A
  Vbus/Vq：11.627V / 1.791V

45Hz:
  ol 平均：44.996Hz
  EKF 平均：+43.056Hz，范围 +42.3Hz 到 +43.7Hz
  EKF/ol：0.957
  Iq/Id：3.001A / 3.001A
  Vbus/Vq：11.614V / 2.372V

60Hz:
  ol 平均：59.995Hz
  EKF 平均：+56.932Hz，范围 +56.4Hz 到 +57.3Hz
  EKF/ol：0.949
  Iq/Id：2.998A / 2.998A
  Vbus/Vq：11.437V / 2.975V
```

阶段结论：

- beta 正常符号是正确方向；之前 beta 取反只是在旧参数和旧 EKF bug 下造成的误导性诊断。
- EKF 半速问题已基本解决，根因组合为：`Rs/Ls/flux` 参数不匹配、电阻相间/单相换算错误、电感取值偏低，以及 EKF Kalman 增益计算覆盖 bug。
- EKF/ol 随频率从 `0.974` 到 `0.949` 略微下降，仍有约 `3-5%` 的模型误差。可能来自单一 `Ls` 无法表示 `Ld/Lq`、电压模型/死区压降、母线电压采样误差或磁链微调。
- 电流环状态健康：三频点 `Iq/Id≈3A` 均稳定，Vq 随频率上升，符合预期。
- 下一阶段可以开始做“谨慎闭环接管”：先在 45Hz 稳定开环时观察 EKF 角度与开环角度差，再加入角度渐变混合，不要直接硬切到 EKF。

## 50. 创建 EKF 接管前诊断分支

从 `main` 创建新分支：

```text
feature/ekf-handoff-diagnostics
```

目标：进入开环到 EKF 闭环接管前的角度诊断阶段，但不让固件真正闭环接管，避免硬切角度导致抖动、失步或过流。

代码修改：

```text
motor/foc_define_parameter.h
- 新增 EKF_HANDOFF_* 诊断阈值：
  - 最低开环频率 25Hz
  - 速度比例允许 0.90 到 1.10
  - 角度误差阈值 0.70rad
  - 连续 10000 个 10kHz FOC tick 后判定 ready

motor/adc.c/h
- 新增 ekf_angle_error_rad
- 新增 ekf_speed_ratio
- 新增 ekf_handoff_speed_ok / ekf_handoff_angle_ok / ekf_handoff_ready
- 每次 FOC step 后计算 EKF angle 与当前开环 FOC_Input.theta 的差值
- 实际 FOC 驱动仍然使用开环 hall_angle，不切 EKF

motor/trace.c/h
- trace record 从 48 字节扩展到 52 字节
- 新增 ekf_angle_err_rad 与 ekf_speed_ratio 两列
- diag_flags 新增：
  - bit4：速度比例达标
  - bit5：角度误差达标
  - bit6：连续满足接管候选条件

tools/export-trace.ps1
- 支持 52 字节 trace record
- CSV 新增 ekf_angle_err_rad、ekf_speed_ratio

user/oled_display.c
- 第二页 `F:` 改为 `ph:`
- `ph:` 显示 `EKF angle - open-loop angle` 的电角度误差，单位为度
```

验证：

```text
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\vscode-build.ps1 build
结果：成功
FLASH: 29648B / 512KB
RAM: 112496B / 128KB, 85.83%

powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\vscode-build.ps1 flash
结果：成功，已通过 ST-LINK 烧录并复位
```

下一步现场测试：按 30Hz、45Hz、60Hz 各跑一次，稳定停机后导出。重点看 `ekf_angle_err_rad` 是否稳定、`ekf_speed_ratio` 是否保持约 `0.95-0.98`，以及 `diag_flags` 是否出现 bit4/bit5/bit6。

## 51. EKF 接管前诊断三频点结果

在 `feature/ekf-handoff-diagnostics` 分支上，用户依次测试 45Hz、30Hz、60Hz。45Hz 第一次导出时没有先手动 stop，trace 最后一条出现 `fault=3` 的无效边界样本；稳定段仍有效。后续测试按“先 KEY1 停机、确认 `C:stop`、再导出”的流程执行。

导出文件：

```text
build/trace_diag_45hz_handoff_diag.csv
build/trace_diag_30hz_handoff_diag.csv
build/trace_diag_60hz_handoff_diag.csv
```

稳定段统计：

```text
30Hz:
  ol 平均：29.995Hz
  EKF 平均：29.196Hz
  EKF/ol：0.973
  ekf_speed_ratio：平均 0.973，范围 0.963 到 0.989
  相位误差：平均 0.186rad = 10.7°，范围 2.3° 到 17.0°
  Iq/Id：3.001A / 3.002A
  Vbus/Vq：11.651V / 1.786V
  diag_flags：125

45Hz:
  ol 平均：44.960Hz
  EKF 平均：43.036Hz
  EKF/ol：0.957
  ekf_speed_ratio：平均 0.957，范围 0.952 到 0.965
  相位误差：平均 0.138rad = 7.9°，范围 2.1° 到 14.2°
  Iq/Id：3.002A / 3.001A
  Vbus/Vq：11.603V / 2.371V
  diag_flags：125

60Hz:
  ol 平均：59.996Hz
  EKF 平均：57.002Hz
  EKF/ol：0.950
  ekf_speed_ratio：平均 0.950，范围 0.944 到 0.980
  相位误差：平均 0.087rad = 5.0°，范围 -0.6° 到 11.8°
  Iq/Id：2.997A / 2.997A
  Vbus/Vq：11.526V / 2.984V
  diag_flags：125
```

`diag_flags=125` 表示：

```text
bit0 motor_control_ready
bit2 open-loop force branch
bit3 EKF update enabled
bit4 speed ratio OK
bit5 angle error OK
bit6 handoff ready
```

阶段结论：

- 三频点速度比例和角度误差都满足当前接管候选条件。
- 相位误差随频率升高反而变小，60Hz 平均仅约 `5°` 电角度。
- 电流环保持健康，`Iq/Id≈3A`，无持续异常电流。
- 下一步可以实现“角度渐变混合接管版”：从开环角度缓慢 blend 到 EKF 角度，加入 fallback，不要硬切。

## 52. 实现角度渐变混合接管版

在 `feature/ekf-handoff-diagnostics` 分支继续实现第一版真正接管逻辑，但仍保持电流策略保守，不启用产品级速度闭环。

设计原则：

- 不硬切 EKF 角度。
- 只有三频点已验证的接管候选条件满足后才开始。
- FOC 角度从开环 `hall_angle` 按最短电角度差混合到 EKF angle。
- 混合时间约 `2s`。
- 若速度比例或 EKF/开环相位误差失效，回退到开环。

代码修改：

```text
motor/foc_define_parameter.h
- 新增 EKF_HANDOFF_BLEND_ENABLE=1
- 新增 EKF_HANDOFF_BLEND_TICKS=20000

motor/adc.c/h
- 新增 ekf_handoff_blend
- 新增 ekf_handoff_state:
  - OPEN_LOOP
  - BLEND
  - EKF
  - FALLBACK
- ekf_angle_error_rad 改为始终表示 EKF angle - open-loop hall_angle
- 满足 ready 后 blend 从 0 增到 1
- FOC_Input.theta = hall_angle + shortest_angle_error(EKF - hall) * blend
- FOC_Input.speed_fdk 同步在开环速度和 EKF 速度之间插值

motor/trace.c
- diag_flags 新增：
  - bit7：正在 blend
  - bit8：已经 full EKF angle
  - bit9：fallback
```

验证：

```text
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\vscode-build.ps1 build
结果：成功
FLASH: 30136B / 512KB
RAM: 112504B / 128KB, 85.83%

powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\vscode-build.ps1 flash
结果：成功，已通过 ST-LINK 烧录并复位
```

下一步现场测试建议：

```text
1. 先测 45Hz
2. 跑稳后观察声音、电源电流、吸力是否和接管前一致
3. OLED ph 不应突然大跳
4. KEY1 停机后导出
5. trace 中应看到 diag_flags 从 125 进入 bit7，再进入 bit8；不能出现 bit9 fallback
```
