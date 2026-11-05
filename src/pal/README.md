# PAL / Analogue Pocket

本工程把 SDLPAL 编译为 openfpgaOS 的 RISC-V 应用，读取用户的 DOS 原版或 PAL98 资源；不执行 PAL.EXE。公开发布包不含游戏资源及运行时二进制，程序不显示 FPS 标记或提供调试快捷键。安装和重新构建详见 [RELEASE.md](RELEASE.md)，运行时独立依赖见 [RUNTIME.md](RUNTIME.md)。

## 上机

将 `../../build/pocket/pal/` 中的 `Assets/`、`Cores/`、`Platforms/` 合并复制到 Pocket SD 卡根目录。启动 openFPGA 中的 SDLPAL / Local.PAL，选择 PAL（只有一个实例时通常自动选择）。需要支持 openFPGA 的兼容固件，不支持睡眠。

使用菜单中的游戏存档；本原型没有实现 Pocket Memories/save states。五个游戏存档映射到 APF 10–14 槽，DOS 文件名为 `pal-1.sav` 至 `pal-5.sav`，PAL98 为 `pal98-1.sav` 至 `pal98-5.sav`。两种存档不兼容，使用独立文件避免互相覆盖。安装包不包含空白存档，更新时不要删除设备上的已有存档。

| 按键 | 功能 |
|---|---|
| 方向键 | 移动、菜单选择 |
| A | 确认、对话 |
| B / X / Start | 取消、菜单 |
| Y | 状态快捷键 |
| L / R | 上一页、下一页 |
| Select | 自动战斗快捷键 |

游戏画面保持原生 320×200，不做软件缩放，在 320×240 输出区域内居中，上下各留 20 行黑边。320×240 画布按 4:3 交给 Pocket 显示，游戏像素不做纵向比例校正；Pocket 最终屏幕缩放仍由系统设置决定。AVI 保持 288×180，居中于同一画布，上下各留 30 行黑边。已完成 Pocket 实机测试。

发布版没有 FPS 覆盖层，F3 和 L+R 不再用于调试切换。

已完成 Pocket 实机测试；桌面集成检查用于持续验证资源、显示、音频和存档行为。

## 构建

在本目录运行：

```sh
bash build.sh
```

此命令仅构建 ELF，不读取游戏目录。公开打包请在仓库根目录运行 `make release`。构建脚本使用容器中的交叉编译环境，资源打包与 SD 目录组装使用宿主 Ruby。

资源由用户显式提供：`make resources GAME_DIR=/absolute/path/to/PAL`。也可先执行 `ruby pack.rb /path/to/PAL pal.pak`，音乐另需 `./tools/render-rix /path/to/PAL/MUS.MKF pal-fm.pcm`（转换器由 `make resources` 构建），再执行 `ruby package.rb`。这组 Ruby / Makefile 资源命令仅用于 DOS。PAL98 使用发布包的 `prepare.py /path/to/PAL98`，安装方法见 RELEASE.md；不要把 PAL98 交给旧的 `make resources` 流程。

桌面集成检查：

```sh
bash smoke.sh
```

可通过 `SDL2_CONFIG` 指定本机的 `sdl2-config` 命令。`make desktop` 生成 `pal-desktop`，从本目录运行即可使用键盘体验（方向键、Enter/Space 确认、Escape 菜单）。桌面版可播放 FM 音乐；PAL98 WAV 音效可在桌面播放；DOS VOC 音效仍使用 Pocket 后端，桌面不播放。

## 已实现与边界

- 原版引擎逻辑、经典战斗分支、YJ_1/YJ_2/MKF/RNG、DOS Big5 与 PAL98 UTF-8 文字；不启用 GLSL、外部音频格式和启动设置 UI。
- 游戏内部保持 8 位索引色和原生 320×200，在 320×240 / 4:3 画布内居中显示，上下各留 20 行黑边；兼容旧资源包中的 WindowHeight=200 配置。发布版不绘制调试覆盖层。
- 21 个资源/配置文件打进 `pal.pak`，占用一个数据槽。包大小 26,982,464 字节；原型整包驻留内存，多个 `fmemopen` 流共享只读数据并各自维护位置。校验索引边界，避免直接扫描 SD 目录。
- 音乐使用原版 `MUS.MKF` 的 RIX 数据，在构建时由 SDLPAL 自带 DOSBox 整数 OPL2 合成器还原为 44.1 kHz / 16-bit 单声道 PCM；关闭环绕，不再使用 MIDI / 通用 SoundFont。`pal-fm.pcm` 约 686 MiB，单独映射到 APF 槽 5，运行时只保留 1 MiB 环形缓冲，由硬件 mixer 播放；每半缓冲约 5.94 秒，事件循环负责补充。长时间完全不处理事件（超过半缓冲时间）可能造成重复音频。
- 转换包含首次播放及保留 OPL 状态的第二遍，循环时重复第二遍；短曲按游戏指定只播放一次。支持切曲、停止、音乐开关、音量与淡入淡出。86 首有效曲目均检查有非零输出，空槽保留；音乐资源来自用户原始文件，没有下载替代配乐。DOS 音效使用 SDK VOC 解码；PAL98 WAV 音效使用 SDL/SDK WAV 解码和 PCM mixer，支持完整 505 槽中的非空音效，缓存最多 4 MiB，已结束的声音可以淘汰。设置更改不持久化。
- 移除 SoundFont 后，常驻资源包约 25.7 MiB，音乐缓冲 1 MiB，另需 ELF、引擎运行时、音效缓存和系统内存；686 MiB 音乐文件不会整包加载。
- 集成检查实际初始化 DOS 游戏、加载首个场景资源、解压标题背景、绘制繁体菜单文字，并将金钱/场景状态保存后重新加载比较；还对音乐实际播放回调进行跨多个缓冲／曲目循环的逐样本比较，检查单次短曲、静音、切曲淡入淡出和停止；逐像素检查原生 320×200 居中后的完整输出及对称黑边，另用扫描行图案验证四边和每行均未裁切。截图是测试构造的标题背景，不代表已经自动通关开场剧情。
- PAL98 准备工具将用户的 UTF-16 `WORD_cht.txt`、`MSG_cht.txt` 按 SSS 脚本的对话跨度和清屏指令转换成引擎原生 UTF-8 语言文件；使用内置繁体字体，不需借用 DOS 字体。图像与文本资源包约 29 MiB，18 MiB 的 `Sounds.mkf` 作为 `pal98-sfx.mkf` 映射到槽 6 按需读取。PAL98 的首场景、标题、363 个 WAV 音效与独立存档往返已通过桌面检查；已完成 Pocket 实机测试。
- PAL98 AVI：准备工具检测到六段原视频和 FFmpeg 时，自动预转为 `pal98-avi.pav`，通过 APF 槽 7 流式播放。播放器以音频实际播放样本数作为时钟，32 帧预读队列和 131,072 样本环形缓冲额外占用约 2 MiB，画面落后时跳帧；确认/取消键跳过。结束、跳过和读取失败时停止专用音频 voice 并恢复游戏调色板与画面，缺失/错误资源回退引擎动画。转换依赖宿主 FFmpeg，运行时不依赖解码库。格式和验证见 [MOVIES.md](MOVIES.md)。
- 不提供 DOS 二进制补丁/修改器兼容性，不自动导入目录内的 `0.RPG`。

## 来源与修改

- openfpgaSDK：SDK-owned 目录保持原样；运行时组装前校验 `runtime/MANIFEST`。
- SDLPAL：源码保存在 `engine/`，保留上游许可证。
- 引擎改动仅在 `main.c`（桌面集成测试入口）、`util.c`（虚拟文件路径和错误提示）、`video.c`（索引色输出）、`input.c`（掌机按键），由 `PAL_POCKET` 控制主要适配分支。
- 本目录提供独立 Makefile、平台配置、文件/声音后端、打包及集成检查工具。未修改上层原始 `PAL/`。

生成的 SD 卡目录包含用户本地游戏资源，仅用于本地测试。对外分发时应排除 `pal.pak`，按 SDLPAL GPLv3 提供对应源码，并遵守 SDK、运行时及 DOSBox/AdPlug 各自的许可证；`pal-fm.pcm` 是游戏音乐的派生文件，也应排除。

## 公开发布

在已有 ELF 后运行 `python3 release.py`，或用 `make release` 先构建，发布 ZIP 输出到 `build/releases/`。打包使用显式文件列表，排除游戏资源、渲染音乐、存档、测试样本、图像和运行时二进制，包含对应源码和许可证。安装时 `prepare.py` 读取用户合法取得的游戏目录；运行时可以由用户提供，或从固定上游版本下载。不要公开旧的 `build/pocket/pal/` 本地完整包。
