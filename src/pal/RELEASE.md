# SDLPAL for Analogue Pocket

支持繁体 DOS 版和 Windows 版（PAL98）仙剑奇侠传，已通过 Pocket 实机测试。请自行准备合法取得的游戏目录。

## 安装

1. 下载并完整解压发布 ZIP。
2. 在解压目录运行，把路径换成自己的游戏目录：

   ```sh
   python3 prepare.py "/path/to/pal"
   ```

3. 等待完成，将 `ready/` 中的 **Assets、Cores、Platforms** 合并复制到 SD 卡根目录。
4. 在 Pocket 的 openFPGA 中启动 SDLPAL，选择 PAL（DOS）或 PAL98（Windows）。

不用挑选或改名游戏文件。工具会自动识别版本；缺少文件时会给出提示。准备过程需要联网，可能花费几分钟，建议预留 2 GiB 空间。

电脑需要 Python；macOS 可直接使用，Linux 另需 C++ 编译器。Pocket 需要支持 openFPGA 的固件。

PAL98 目录中有 `1.AVI` 至 `6.AVI` 且电脑安装了 FFmpeg 时，工具会自动加入动画；否则使用游戏原有动画。

## 操作

| 按键 | 功能 |
|---|---|
| 方向键 | 移动、菜单选择 |
| A | 确认、对话 |
| B / X / Start | 取消、菜单 |
| Y | 状态 |
| L / R | 上一页、下一页 |
| Select | 自动战斗 |

动画播放时按确认或取消可跳过。画面保留原生像素，居中显示。

## 存档与更新

使用游戏内菜单存档。存档位于 SD 卡的 `Saves/pal/common/`，DOS 使用 `pal-1.sav` 至 `pal-5.sav`，PAL98 使用 `pal98-1.sav` 至 `pal98-5.sav`。

两版可以同时安装，存档互不覆盖。更新时覆盖安装文件，保留已有 `Saves/`。不支持 Pocket 睡眠和 Memories。

## 可选参数

普通安装只需上面的命令。以下参数按需使用：

- `--output 目录`：更换生成位置；默认 `ready/`，不能是已有目录。
- `--without-avi`：跳过 AVI 动画。
- `--runtime /path/to/runtime`：离线使用本地运行时，版本见 `RUNTIME.md`。

## 源码与许可证

本包不含游戏资源和存档。准备后生成的游戏文件仅供个人安装，请勿作为本项目的发布包分享。

应用采用 GPLv3，完整源码位于 `source/`，第三方许可证见 `LICENSES/` 和 `NOTICE`。运行时由工具另行获取，其来源与授权见 `RUNTIME.md`。

从源码构建：在 `source/` 目录运行 `make build`，需要容器运行环境。生成发布包使用 `make release`。
