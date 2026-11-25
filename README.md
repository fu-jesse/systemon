# systemon — nRF54 board System ON idle-current sample

English | [中文](README.zh.md)

A sample for evaluating System ON idle current on the nRF54 family with two typical low-frequency clock configurations, LFXO and LFRC. The application puts the peripherals and RAM into the lowest-power state. You can also switch the configuration along three axes: **logging, turning off unused RAM, and the low-frequency clock**.

- SDK: nRF Connect SDK v3.4.1 (sysbuild)
- Target boards: `nrf54l15dk/nrf54l15/cpuapp`, `nrf54lm20dk/nrf54lm20a/cpuapp`, `nrf54lm20dk/nrf54lm20b/cpuapp`, `nrf54ls05dk/nrf54ls05a/cpuapp`, `nrf54ls05dk/nrf54ls05b/cpuapp`
- Power measurement results: [Doc/power_results.md](Doc/power_results.md) ([中文](Doc/power_results.zh.md))

## What the application does

`main()` prints a banner and then calls `ram_report_and_trim()`:

1. Prints the total, used and free RAM.
2. If `CONFIG_APP_RAM_POWER_DOWN_UNUSED=y` (the default), turns off the 32 KB RAM sections above the used RAM through MEMCONF. This only takes effect in System ON IDLE, and the content is not retained.

Only sections that lie completely inside the application SRAM and completely above the used RAM are turned off. The nRF54LM20 application SRAM is 0x7fc00 (not the full 512 KB), so section 15 is left alone: after the unused RAM is turned off, the LM20 keeps sections 0 and 15 powered, and the nRF54L15 keeps only section 0. The nRF54LS05 has only three 32 KB RAM sections (the A variant has 64 KB = the first two, the B variant has 96 KB = all three); after the unused RAM is turned off it keeps only section 0.

## Configuration

Three axes, 8 combinations in total:

| Axis | Options | How to select |
|---|---|---|
| Logging | on (default) / off (release) | For release add `-DFILE_SUFFIX=release`; it uses `prj_release.conf` and `app_release.overlay` (`uart20` disabled, logging and serial off) |
| RAM | unused RAM turned off (default) / all RAM powered | For all RAM powered add `ram_all.conf` (`CONFIG_APP_RAM_POWER_DOWN_UNUSED=n`) |
| Low-frequency clock | LFXO (default) / LFRC | For LFRC add `lfrc.conf` (`CALIBRATION_PERIOD=4000`, `MAX_SKIP` left at its default of 1) |

Both `prj*.conf` files enable `CONFIG_PM_DEVICE` and `CONFIG_PM_DEVICE_RUNTIME`, and turn off the deprecated `CONFIG_NRF_PLATFORM_LUMOS`. In `prj.conf` the log goes to `uart20`; the log backend resumes it only while printing and suspends it again afterwards.

### Hardware (devicetree overlays)

The overlays have three layers. Each `boards/<board>.overlay` only includes the layer above it and sets its own UART TX pin:

- `boards/dk_common.overlay`: common to all DKs. Deletes the LEDs, buttons, PWM LEDs and the aliases that point to them, disables `pwm20`, and adds `zephyr,pm-device-runtime-auto` to `uart20` (suspended at boot).
- `boards/dk_l15_lm20_common.overlay`: nRF54L15 and nRF54LM20 only. Includes the layer above, then deletes the on-board SPI flash (`mx25r64`), disables `spi00`, and turns the NFC pins into normal GPIOs (`nfct` disabled with `nfct-pins-as-gpios`; `/ns` targets are not verified). The nRF54LS05 has none of these nodes, so it cannot use this layer.
- `boards/nrf54lm20dk_common.overlay`, `boards/nrf54ls05dk_common.overlay`: shared by the two LM20 boards (a/b) and the two LS05 boards (a/b) respectively.

`uart20` keeps only the TX pin, in both the default and the sleep state (nRF54L15: P1.04, nRF54LM20: P1.16, nRF54LS05: P1.10; taken from the generated devicetree, not verified on the boards).

`boards/<board>_release.overlay` additionally includes `app_release.overlay`, which disables `uart20`.

## Build

### With west

```
west build -b nrf54l15dk/nrf54l15/cpuapp
west build -b nrf54l15dk/nrf54l15/cpuapp -- -DFILE_SUFFIX=release
west build -b nrf54lm20dk/nrf54lm20b/cpuapp -- -DFILE_SUFFIX=release -DEXTRA_CONF_FILE="ram_all.conf;lfrc.conf"
```

### With the developer shell (Windows)

Double-click [dev.cmd](dev.cmd) to open a PowerShell with the commands loaded (or run `. .\dev.ps1` in PowerShell).

| Command | What it does |
|---|---|
| `setup` | Checks NCS (tries nrfutil first, then the toolchain and NCS directories set at the top of `dev.ps1`; stops with an error if neither works), then asks for the board, low-frequency clock, RAM mode and logging |
| `build` | Incremental build of the current configuration (`west build`) |
| `rebuild` | Pristine build (`west build -p`) |
| `flash` | Flashes the build of the current configuration, without rebuilding or a mass erase; builds first if there is no build yet |
| `clean` | Deletes the whole `build\` directory (asks first) |

The `setup` choices can also be given as parameters, for example `setup -Board ls05b -Lf lfrc -Ram all -Log off`. `flash` accepts `-Build` to name a build directory and `-DevId` to give a J-Link serial number. If you run `build` or `flash` without having run `setup`, `setup` runs first.

The NCS version, NCS directory and toolchain directory are set at the top of `dev.ps1`.

Build output goes to `build\<board>_<lfclk>[_ramtrim][_log]`. The board is one of `l15`, `lm20a`, `lm20b`, `ls05a`, `ls05b`; `ramtrim` appears only when unused RAM is turned off, and `log` only when logging is on. For example, `build\l15_lfxo` is L15, LFXO, all RAM powered, release.

## Tests (twister)

[sample.yaml](sample.yaml) defines 8 scenarios that cover every combination of the three axes. The scenario names are `sample.systemon.<log|nolog>.<ramtrim|ramall>.<lfxo|lfrc>`, and all supported boards are built:

```
west twister -T . -p nrf54l15dk/nrf54l15/cpuapp
```

## Power measurement

- Connect the external supply to the VDD current-measurement header and remove the jumper on that header (P6 on the nRF54L15 DK; on the nRF54LM20 DK and nRF54LS05 DK follow the silkscreen).
- Power the DK over USB first, then supply the header. Keep USB connected; do not unplug it to save power.
- Supply 3.0 V, with the PPK2 in source-meter mode; close all serial terminals.
- Flash with J-Link, and keep the power on while flashing.

## Files

| Path | Content |
|---|---|
| `src/main.c` | Application code |
| `Kconfig` | `APP_RAM_POWER_DOWN_UNUSED` |
| `prj.conf`, `prj_release.conf` | Default and release configuration |
| `lfrc.conf`, `ram_all.conf` | Extra configuration for the low-frequency clock and RAM |
| `boards/` | Board overlays, shared overlays (`dk_common`, `dk_l15_lm20_common`, `nrf54lm20dk_common`, `nrf54ls05dk_common`) and `_release` overlays |
| `app_release.overlay` | Disables `uart20` in release |
| `sample.yaml` | twister scenarios |
| `dev.ps1`, `dev.cmd` | Developer shell |
| `Doc/power_results.md`, `Doc/power_results.zh.md`, `Doc/power_results.csv` | Power test results (English and Chinese); the matching PPK2 waveforms (`Doc/*_ppk2.png`) are in the same folder |
| `Doc/make_report_html.py` | Converts `power_results.md` (or `power_results.zh.md`) into one self-contained HTML file with the images embedded as base64. Usage: `python Doc/make_report_html.py [input.md]`; the default input is `power_results.md` next to the script |
| `README.md`, `README.zh.md` | This document in English and Chinese |
