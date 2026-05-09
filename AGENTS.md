# Agent Handoff

This STM32F4 + DRV8301 sensorless FOC project is adapted for the user's custom PCB and Panasonic 6MD030Z compressor. Use `codex.md` as the long-term project memory and `log.md` as the chronological lab notebook.

## Current State

- Firmware style: STM32F4 StdPeriph, not HAL.
- Main project: `Keil_Project/stm32_drv8301_keil.uvprojx`.
- VSCode/ST-Link path is wired through `tools/vscode-build.ps1`.
- USB/PC host communication is disabled by default with `PC_COMMUNICATION_ENABLE=0`.
- Current drive mode is the low-speed EC11 open-loop product demo: 30-80Hz electrical, 450-1200rpm mechanical, no power-on auto-start. Do not re-enable 120Hz+ auto tests without an explicit safety reason.
- `COMPRESSOR_EC11_DIRECTION=-1` because the bench encoder direction was confirmed reversed.

## Working Rules

- Keep hardware and motor-control edits small and verifiable.
- Preserve user changes and generated project files unless explicitly asked to clean them.
- Source and docs are UTF-8.
- Before pushing, run:
  - `git diff --check`
  - `powershell -NoProfile -ExecutionPolicy Bypass -File tools/vscode-build.ps1 build`

## Latest EKF Baseline

As of 2026-05-09, beta normal signs plus confirmed 6MD030Z parameters are flashed:

- `RS_PARAMETER=0.188f`
- `LS_PARAMETER=0.00036f`
- `FLUX_PARAMETER=0.00580f`
- `EKF_V_BETA_SIGN=+1.0f`
- `EKF_I_BETA_SIGN=+1.0f`

Open-loop trace results:

- 30Hz: EKF `+29.21Hz`, ratio `0.974`
- 45Hz: EKF `+43.06Hz`, ratio `0.957`
- 60Hz: EKF `+56.93Hz`, ratio `0.949`

Current development branch: `feature/open-loop-pot-compressor`.

This branch is currently frozen around a safer product-demo point: open-loop FOC, EC11 adjustable 30-80Hz, `Iq≈2.5A`, `Id≈3A`, `COMPRESSOR_AUTO_TEST_ENABLE=0`, and `COMPRESSOR_OPEN_LOOP_VF_ENABLE=0`. EKF handoff experiments are paused; EKF remains useful for OLED/trace diagnostics only.
