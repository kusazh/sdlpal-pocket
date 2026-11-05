# SDLPAL for Analogue Pocket 0.2.0

这是 SDLPAL 的非官方 openfpgaOS 移植版。发布包只包含开源程序、对应源码、许可证和资源准备工具，**不含仙剑游戏数据、音乐、游戏字体文件或存档**。仙剑奇侠传及相关名称的权利属于各自权利人；本项目不代表大宇、Analogue 或 SDLPAL 官方。

## 安装

需要 Pocket 固件 2.2+、Python 3.9+，以及你自己合法取得的**繁体 DOS 版**仙剑资源。Windows 95/98 柔情版不适用本包。目前资源准备流程在 macOS 验证；Linux 可从源码编译转换器，需要 C++11 编译器。Windows 原生流程尚未验证。

1. 解压发布包，在电脑上保留整个目录（包含 `source/`）。
2. 将自己游戏的资源文件放在一个目录，例如 `~/Games/PAL`。文件名大小写均可，不需要 PAL.EXE 或 RPG 存档。所需文件：

   ```text
   ABC.MKF BALL.MKF DATA.MKF F.MKF FBP.MKF FIRE.MKF GOP.MKF
   MAP.MKF MGO.MKF MIDI.MKF MUS.MKF PAT.MKF RGM.MKF RNG.MKF
   SSS.MKF VOC.MKF M.MSG WORD.DAT WOR16.ASC WOR16.FON
   ```

3. 在发布包目录运行（路径自行替换）：

   ```sh
   python3 prepare.py --game "$HOME/Games/PAL" --output ./ready --fetch-runtime
   ```

   工具只读取你的游戏目录，在本机生成 `pal.pak` 和 DOS FM 音乐 `pal-fm.pcm`，音乐约 686 MiB。macOS 附带 arm64/x86_64 通用转换器，不依赖 SDL、Homebrew 或 Ruby；Linux 使用 `CXX` 指定的编译器，默认 `c++`。转换可能需要几分钟，预留至少 1 GiB 空间。

   `--fetch-runtime` 从 openfpgaOS SDK 的固定版本下载三个运行时文件并按上游 MANIFEST 校验，不下载游戏数据。运行时由其上游独立授权，参见 `RUNTIME.md`。离线安装可将此参数换为 `--runtime /path/to/openfpgaSDK/runtime`；必须与本包固定版本一致。工具要求输出目录尚不存在，不会写入已有存档或修改 SD 卡。

4. 将 `ready/` 内的 **Assets、Cores、Platforms** 三个目录合并复制到 SD 卡根目录。不要只复制发布包中尚未准备完成的 `sdcard/`。
5. 在 Pocket 的 openFPGA 中启动 SDLPAL / Local.PAL，选择 PAL。

生成后的 `ready/` 含你的游戏数据和音乐，只用于自己的安装，请勿作为本项目发布包上传或分享。

## 操作

| 按键 | 功能 |
|---|---|
| 方向键 | 移动、菜单选择 |
| A | 确认、对话 |
| B / X / Start | 取消、菜单 |
| Y | 状态 |
| L / R | 上一页、下一页 |
| Select | 自动战斗 |

原生 320×200 输出，已移除 FPS 标记和 F3 / L+R 调试快捷键。Pocket 缩放器处理 4:3 显示比例。使用游戏内存档，不支持 Pocket Memories、睡眠或即时存档。

存档位于 `Saves/pal/common/pal-1.sav` 至 `pal-5.sav`。本包不含存档，安装或更新时保留已有 `Saves/`。沿用 `Local.PAL` 标识，以便继续读取原型版存档。

## 源码与重新构建

`source/` 包含本 ELF 对应的移植源码、引擎、SDK 源码/头文件、SDK 自带的 musl 链接库、原始 musl 1.2.5 源码压缩包，以及构建脚本和许可证。应用整体按 GNU GPL v3 分发；各第三方文件保留原有许可和版权。修改版应继续提供对应源码和许可证，参见 `LICENSES/` 和 `NOTICE`。没有运行时签名锁定，可用自行构建的 ELF 替换 `Assets/pal/common/pal.elf`。

构建程序只需要 Docker/OrbStack，无须游戏数据。在 `source/src/pal/` 运行：

```sh
bash ../../tools/sdk-container.sh make -j6 all
```

输出为 `source/.obj/pal/app.elf`。宿主转换器：`make tools/render-rix`，仅需 C++11。重新生成公开包：`python3 release.py`（macOS 构建通用转换器；其他系统可省略该二进制，安装时从源码编译）。资源准备脚本使用 Python 标准库，无需 pip 包。

## 验证与边界

已验证 RISC-V 编译、桌面初始化/场景加载/存档往返、320×200 全画面逐像素输出和 FM 流播放；准备工具输出应与本地原型资源逐字节一致。0.2.0 新安装流程仍需 Pocket 真机验收。当前不保证简体、修改版、Windows 版资源兼容，不支持其原版 EXE 补丁。不提供游戏资源下载地址。
