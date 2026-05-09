# Agent Handoff

This STM32F4 + DRV8301 sensorless FOC project is adapted for the user's custom PCB and Panasonic 6MD030Z compressor. Use `codex.md` as the long-term project memory and `log.md` as the chronological lab notebook.

## Current State

- Firmware style: STM32F4 StdPeriph, not HAL.
- Main project: `Keil_Project/stm32_drv8301_keil.uvprojx`.
- VSCode/ST-Link path is wired through `tools/vscode-build.ps1`.
- USB/PC host communication is disabled by default with `PC_COMMUNICATION_ENABLE=0`.
- Current drive mode is open-loop FOC hold for 30/45/60Hz EKF calibration. Do not enable automatic EKF closed-loop handoff without adding guarded transition logic.

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

Current development branch: `feature/ekf-handoff-diagnostics`.

This branch now contains the first angle-blended handoff firmware. It waits for handoff-ready diagnostics, blends the FOC angle from open-loop `hall_angle` to EKF angle over about 2 seconds, and falls back to open-loop if speed ratio or EKF/open-loop phase error leaves the valid window. It still keeps the current command strategy conservative at `Id/Iq≈3A`; speed-loop product control is a later stage.
