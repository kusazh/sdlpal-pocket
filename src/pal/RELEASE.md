# SDLPAL for Analogue Pocket

这是 SDLPAL 的非官方 openfpgaOS 移植版。发布包只包含开源程序、对应源码、许可证和资源准备工具，**不含仙剑游戏数据、音乐、游戏字体文件或存档**。仙剑奇侠传及相关名称的权利属于各自权利人；本项目不代表大宇、Analogue 或 SDLPAL 官方。

## 安装

准备好自己合法取得的繁体 **DOS 版或 Windows 版（PAL98）游戏目录**，不用挑选、改名或混合其中的文件。

1. 下载并完整解压发布 ZIP。
2. 在解压目录运行一条命令，把路径换成自己的游戏目录：

   ```sh
   python3 prepare.py "/path/to/仙剑游戏目录"
   ```

3. 等待完成，将生成的 `ready/` 中的 **Assets、Cores、Platforms** 合并复制到 SD 卡根目录。
4. 在 Pocket 的 openFPGA 中启动 SDLPAL，选择 PAL（DOS）或 PAL98（Windows）。

工具自动识别版本、准备资源与音乐，并下载和校验所需运行时。Windows 版目录内有完整的六段 AVI 且电脑安装了 FFmpeg 时，会自动加入动画；否则使用游戏原有动画。缺少必要资源时，工具会提示缺少的文件。当前支持繁体 DOS 版和带繁体 TXT 文本、RIX 音乐的 PAL98 目录。

电脑需要 Python；macOS 已附带音乐转换器，Linux 另需 C++ 编译器。准备过程可能需要几分钟，建议预留 2 GiB 空间。Pocket 需要支持 openFPGA 的兼容固件。

DOS 与 Windows 版可同时安装，工具会分别生成 `PAL.json` 和 `PAL98.json`。更新时保留 SD 卡上的 `Saves/`。生成目录含自己的游戏资源，仅供个人安装，请勿作为公开发布包分享。

<details>
<summary>可选参数与离线安装</summary>

- `--output 目录`：指定生成位置，默认是 `ready/`；该目录须尚不存在。
- `--without-avi`：跳过 AVI 转换。
- `--runtime /path/to/runtime`：使用固定版本的本地 SDK 运行时，供离线安装使用。版本与授权见 `RUNTIME.md`。

旧版 `--game`、`--fetch-runtime`、`--with-avi` 参数仍兼容。

</details>

## 操作

| 按键 | 功能 |
|---|---|
| 方向键 | 移动、菜单选择 |
| A | 确认、对话 |
| B / X / Start | 取消、菜单 |
| Y | 状态 |
| L / R | 上一页、下一页 |
| Select | 自动战斗 |

原生 320×200 无软件缩放，居中于 320×240 画布、上下各留 20 行黑边，已移除 FPS 标记和 F3 / L+R 调试快捷键。使用游戏内存档，不支持 Pocket Memories、睡眠或即时存档。

存档位于 `Saves/pal/common/pal-1.sav` 至 `pal-5.sav`；PAL98 使用同目录下的 `pal98-1.sav` 至 `pal98-5.sav`，两者不兼容且互不覆盖。本包不含存档，安装或更新时保留已有 `Saves/`。沿用 `Local.PAL` 标识，以便继续读取原型版存档。

## 源码与重新构建

`source/` 包含本 ELF 对应的移植源码、引擎、SDK 源码/头文件、SDK 自带的 musl 链接库、原始 musl 源码压缩包，以及构建脚本和许可证。应用整体按 GNU GPL v3 分发；各第三方文件保留原有许可和版权。修改版应继续提供对应源码和许可证，参见 `LICENSES/` 和 `NOTICE`。没有运行时签名锁定，可用自行构建的 ELF 替换 `Assets/pal/common/pal.elf`。

构建程序需要构建脚本支持的容器运行环境，无须游戏数据。在 `source/src/pal/` 运行：

```sh
bash ../../tools/sdk-container.sh make -j6 all
```

输出为 `source/.obj/pal/app.elf`。宿主转换器：`make tools/render-rix`，需要 C++ 编译器。重新生成公开包：`python3 release.py`（macOS 构建通用转换器；其他系统可省略该二进制，安装时从源码编译）。资源准备脚本使用 Python 标准库，无需 pip 包。

## 验证与边界

已验证 RISC-V 编译、桌面初始化/场景加载/存档往返、原生 320×200 居中输出及上下对称黑边和 FM 流播放；DOS 准备工具输出与原 Ruby 打包器逐字节一致。PAL98 已验证 YJ_2 场景和标题解压、繁体文本、363 个非空 WAV 音效的解码及 PCM 校验、音效播放/静音/缓存上限和独立存档往返。AVI 已验证六片全部 6,385 帧与 PCM 的一致性、居中黑边、音频计时与跳帧、跳过、播放后状态恢复及异常资源回退；桌面实时播放也已通过。已完成 Pocket 实机测试。当前不保证简体、修改版或其他 Windows 资源布局兼容，不支持其原版 EXE 补丁。不提供游戏资源下载地址。
