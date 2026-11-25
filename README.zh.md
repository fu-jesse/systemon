# systemon — nRF54 系列 System ON IDLE 电流评估 sample

[English](README.md) | 中文

用于评估 nRF54 系列在 LFXO 和 LFRC 两种典型低频时钟配置下的 System ON IDLE 电流。应用会将外设和 RAM 配置到低功耗状态,并可在**日志、关闭不用的 RAM、低频时钟**三个维度上切换配置。

- SDK:nRF Connect SDK v3.4.1(sysbuild)
- 目标板:`nrf54l15dk/nrf54l15/cpuapp`、`nrf54lm20dk/nrf54lm20a/cpuapp`、`nrf54lm20dk/nrf54lm20b/cpuapp`、`nrf54ls05dk/nrf54ls05a/cpuapp`、`nrf54ls05dk/nrf54ls05b/cpuapp`
- 功耗测试结果见 [Doc/power_results.zh.md](Doc/power_results.zh.md)([English](Doc/power_results.md))

## 应用做了什么

`main()` 打印一行日志,然后调用 `ram_report_and_trim()`:

1. 打印 RAM 总量、已用量和空闲量。
2. 如果 `CONFIG_APP_RAM_POWER_DOWN_UNUSED=y`(默认),通过 MEMCONF 把已用 RAM 之上的 32 KB 分区关掉。关掉只在 System ON IDLE 生效,内容不保留。

只关"完全落在应用 SRAM 里、并且在已用 RAM 之上"的分区。nRF54LM20 的应用 SRAM 是 0x7fc00(不是整 512 KB),所以第 15 个分区不关;LM20 关掉不用的 RAM 后实际保留第 0 个和第 15 个两个分区,nRF54L15 保留第 0 个一个分区。nRF54LS05 的 RAM 只有 3 个 32 KB 分区(A 版 64 KB 是前 2 个,B 版 96 KB 是 3 个),关掉不用的 RAM 后只保留第 0 个分区。

## 配置

三个维度,共 8 种组合:

| 维度 | 选项 | 怎么选 |
|---|---|---|
| 日志 | 开(默认)/ 关(release) | release 加 `-DFILE_SUFFIX=release`,使用 `prj_release.conf` 和 `app_release.overlay`(`uart20` 禁用,日志和串口关闭) |
| RAM | 关闭不用的 RAM(默认)/ 全部上电 | 全部上电加 `ram_all.conf`(`CONFIG_APP_RAM_POWER_DOWN_UNUSED=n`) |
| 低频时钟 | LFXO(默认)/ LFRC | LFRC 加 `lfrc.conf`(`CALIBRATION_PERIOD=4000`,`MAX_SKIP` 用默认值 1) |

两个 `prj*.conf` 都开启了 `CONFIG_PM_DEVICE` 和 `CONFIG_PM_DEVICE_RUNTIME`,并把已弃用的 `CONFIG_NRF_PLATFORM_LUMOS` 关掉。`prj.conf` 里的日志走 `uart20`,由日志后端按需唤醒、输出完自动挂起。

### 硬件(设备树 overlay)

overlay 分三层,`boards/<board>.overlay` 只 include 上一层并设置各自的 UART TX 引脚:

- `boards/dk_common.overlay`:所有 DK 通用。删除 LED、按键、PWM LED 和指向它们的 alias,`pwm20` 禁用,`uart20` 加 `zephyr,pm-device-runtime-auto`(开机即挂起)。
- `boards/dk_l15_lm20_common.overlay`:nRF54L15 和 nRF54LM20 专用,include 上一层,再删除板载 SPI flash(`mx25r64`)、禁用 `spi00`,并把 NFC 引脚转为普通 GPIO(`nfct` 禁用并设置 `nfct-pins-as-gpios`,`/ns` 目标没有验证)。nRF54LS05 没有这些节点,所以不能用这一层。
- `boards/nrf54lm20dk_common.overlay`、`boards/nrf54ls05dk_common.overlay`:LM20 的 a/b 两个板、LS05 的 a/b 两个板各共用一个。

`uart20` 只保留 TX 引脚,default 和 sleep 两种状态都是(nRF54L15 是 P1.04,nRF54LM20 是 P1.16,nRF54LS05 是 P1.10,取自生成的设备树,没有单独在板上验证)。

`boards/<board>_release.overlay` 在上面基础上再 include `app_release.overlay`,禁用 `uart20`。

## 编译

### 用 west

```
west build -b nrf54l15dk/nrf54l15/cpuapp
west build -b nrf54l15dk/nrf54l15/cpuapp -- -DFILE_SUFFIX=release
west build -b nrf54lm20dk/nrf54lm20b/cpuapp -- -DFILE_SUFFIX=release -DEXTRA_CONF_FILE="ram_all.conf;lfrc.conf"
```

### 用开发 shell(Windows)

双击 [dev.cmd](dev.cmd),会打开一个带命令的 PowerShell(也可以在 PowerShell 里运行 `. .\dev.ps1`)。

| 命令 | 作用 |
|---|---|
| `setup` | 先检查 NCS(先试 nrfutil,再试 `dev.ps1` 顶部的 toolchain 和 NCS 目录,都不行就报错停止),然后选板子、低频时钟、RAM、日志 |
| `build` | 增量编译当前配置(`west build`) |
| `rebuild` | 全新编译(`west build -p`) |
| `flash` | 烧录当前配置的构建,不重新编译、不整片擦除;没有构建时先自动编译 |
| `clean` | 删除整个 `build\` 目录(会先确认) |

`setup` 的选项也可以直接带参数,例如 `setup -Board ls05b -Lf lfrc -Ram all -Log off`。`flash` 可以用 `-Build` 指定构建目录名,`-DevId` 指定 J-Link 序列号。没做过 `setup` 直接运行 `build` 或 `flash` 时,会先自动 `setup`。

NCS 版本、NCS 目录和 toolchain 目录在 `dev.ps1` 顶部的设置里改。

构建输出在 `build\<板子>_<低频时钟>[_ramtrim][_log]`,其中板子是 `l15`、`lm20a`、`lm20b`、`ls05a`、`ls05b`,`ramtrim` 只在关闭不用的 RAM 时才有,`log` 只在日志开时才有。例如 `build\l15_lfxo` 是 L15、LFXO、RAM 全开、release。

## 测试(twister)

[sample.yaml](sample.yaml) 里有 8 个场景,覆盖上面三个维度的全部组合,场景名是 `sample.systemon.<log|nolog>.<ramtrim|ramall>.<lfxo|lfrc>`,所有支持的目标板都会构建:

```
west twister -T . -p nrf54l15dk/nrf54l15/cpuapp
```

## 功耗测量

- 外部电源接 VDD 电流测量排针,拔掉该排针的跳线(nRF54L15 DK 是 P6,nRF54LM20 DK 和 nRF54LS05 DK 以板上丝印为准)。
- 先让 DK 通过 USB 上电,再给排针供电。USB 保持连接,不要为了省电拔掉。
- 供电 3.0 V,PPK2 源表模式;关闭所有串口终端。
- 烧录用 J-Link,烧录时电源要保持打开。

## 目录

| 路径 | 内容 |
|---|---|
| `src/main.c` | 应用代码 |
| `Kconfig` | `APP_RAM_POWER_DOWN_UNUSED` |
| `prj.conf`、`prj_release.conf` | 默认配置和 release 配置 |
| `lfrc.conf`、`ram_all.conf` | 低频时钟和 RAM 的 extra 配置 |
| `boards/` | 板级 overlay、公共 overlay(`dk_common`、`dk_l15_lm20_common`、`nrf54lm20dk_common`、`nrf54ls05dk_common`)和 `_release` overlay |
| `app_release.overlay` | release 下禁用 `uart20` |
| `sample.yaml` | twister 场景 |
| `dev.ps1`、`dev.cmd` | 开发 shell |
| `Doc/power_results.md`、`Doc/power_results.zh.md`、`Doc/power_results.csv` | 功耗测试结果(英文、中文),对应的 PPK2 波形(`Doc/*_ppk2.png`)在同一目录 |
| `Doc/make_report_html.py` | 把 `power_results.md`(或 `power_results.zh.md`)转成一个自包含的 HTML(图片用 base64 嵌入),用法:`python Doc/make_report_html.py [输入.md]`,默认输入是同目录的 `power_results.md` |
| `README.md`、`README.zh.md` | 本文档的英文和中文版 |
