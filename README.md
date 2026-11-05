# SDLPAL for Analogue Pocket

在 Analogue Pocket 上游玩繁体 DOS 版或 Windows 版（PAL98）仙剑奇侠传。已通过实机测试。

请自行准备合法取得的游戏目录，本项目不提供游戏资源。

## 安装

1. 从 [Releases](https://github.com/kusazh/sdlpal-pocket/releases/latest) 下载 ZIP 并完整解压。
2. 在解压目录运行，把路径换成自己的游戏目录：

   ```sh
   python3 prepare.py "/path/to/pal"
   ```

3. 将生成的 `ready/` 中的 `Assets/`、`Cores/`、`Platforms/` 合并复制到 SD 卡根目录。
4. 在 Pocket 的 openFPGA 中启动 SDLPAL，选择 PAL 或 PAL98。

电脑需要 Python。macOS 可直接使用；Linux 另需 C++ 编译器。若要播放 PAL98 的 AVI 动画，电脑还需安装 FFmpeg。工具会自动识别游戏版本并准备所需文件。

详细操作、存档和可选参数见[安装说明](src/pal/RELEASE.md)。

## 操作与存档

方向键移动，A 确认，B / X / Start 打开菜单，Y 查看状态，L / R 翻页，Select 自动战斗。播放动画时按确认或取消可跳过。

使用游戏内菜单存档。DOS 与 PAL98 可以同时安装，存档互不覆盖；更新时保留 SD 卡上的 `Saves/`。不支持 Pocket 睡眠和 Memories。

## 开发

```sh
make build    # 编译程序
make release  # 生成发布包
```

构建需要容器运行环境，不需要游戏资源。发布包位于 `build/releases/`。测试方法见[开发说明](src/pal/README.md)。

## 许可证

本项目是基于 [SDLPAL](https://github.com/sdlpal/sdlpal) 和 [openfpgaOS SDK](https://github.com/openfpgaOS/openfpgaSDK) 的非官方移植，与大宇、Analogue 无隶属关系。

应用采用 [GPLv3](LICENSE)，发布包包含对应源码。第三方依赖保留各自许可证，见 [NOTICE](NOTICE)、[LICENSES](LICENSES) 和 [UPSTREAM.md](UPSTREAM.md)。运行时授权见 [RUNTIME.md](src/pal/RUNTIME.md)。
