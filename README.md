# SDLPAL for Analogue Pocket

基于 [SDLPAL](https://github.com/sdlpal/sdlpal) 和 [openfpgaOS SDK](https://github.com/openfpgaOS/openfpgaSDK) 的非官方仙剑奇侠传 DOS 版移植。程序运行在 Pocket 的 openFPGA / openfpgaOS 上，不执行 PAL.EXE。

**本仓库及公开发布包不包含游戏资源、游戏音乐、原版字体文件或存档。用户必须自行提供合法取得的繁体 DOS 版资源。** 不支持 Windows 柔情版。

## 安装

从 [GitHub Releases](https://github.com/kusazh/sdlpal-pocket/releases) 下载发布 ZIP，完整解压。在解压目录运行：

```sh
python3 prepare.py --game /path/to/your/PAL --output ./ready --fetch-runtime
```

若尚无 Release，可先按下面的源码构建方法执行 `make release`，再进入生成的发布目录运行同一条命令。

工具在本机打包游戏数据并转换 DOS FM 音乐，随后从固定的 openfpgaOS 上游版本获取三个运行时文件并校验。将 `ready/` 中的 `Assets/`、`Cores/`、`Platforms/` 合并复制到 SD 卡根目录。

准备工具需要 Python。macOS 包含 arm64/x86_64 通用转换器；Linux 需要 C++ 编译器。Pocket 需要支持 openFPGA 的兼容固件。完整资源清单、离线安装和操作方法见 [安装说明](src/pal/RELEASE.md)，运行时独立授权与来源见 [RUNTIME.md](src/pal/RUNTIME.md)。

原生 320×200 输出，已移除 FPS 标记和调试快捷键。方向键移动、A 确认、B/X/Start 菜单、Y 状态、L/R 翻页、Select 自动战斗。使用游戏内存档，不支持睡眠或 Pocket Memories。

## 从源码构建

安装并启动构建脚本支持的容器运行环境，然后运行：

```sh
make build
```

无需游戏资源，输出 `.obj/pal/app.elf`。完整公开发布包：

```sh
make release
```

发布 ZIP 输出到 `build/releases/`，包含程序、用户准备工具、对应源码和许可证；不包含运行时二进制及游戏数据。发布到 GitHub 时上传此 ZIP 和相邻的 `.zip.sha256` 文件，勿上传自己生成的 `ready/` 或任何旧的完整测试包。

宿主 FM 转换器：`make renderer`，需要 C++ 编译器。桌面检查需要 SDL2 及自己的游戏目录：

```sh
make test GAME_DIR=/absolute/path/to/PAL
```

可通过 `SDL2_CONFIG` 指定本机的 `sdl2-config` 命令。细节见 [开发说明](src/pal/README.md)。游戏数据和测试产物已加入 `.gitignore`。

## 仓库结构

```text
src/pal/          移植代码、SDLPAL 引擎、转换与发布脚本
src/sdk/          固定版本的 openfpgaOS SDK（含 musl 头文件和链接库）
tools/, scripts/ SDK 构建依赖
runtime/MANIFEST 运行时版本清单，不含运行时二进制
LICENSES/        第三方许可证及字体声明
SDK-FILES.txt    公开源码包所需的 SDK 文件清单
```

## 许可证与来源

应用整体按 [GNU GPL v3](LICENSE) 分发，各依赖保留自己的许可证。SDK 使用 Apache-2.0，musl 使用 MIT，FM 转换器包含 AdPlug / DOSBox 组件。详见 [NOTICE](NOTICE)、[LICENSES](LICENSES) 和 [UPSTREAM.md](UPSTREAM.md)。对应源码随发布包一起提供。

仙剑奇侠传、Analogue Pocket 等名称的权利属于各自权利人。本项目不代表大宇、Analogue、SDLPAL 或 openfpgaOS 官方。程序按许可证提供，不作担保。

已通过独立源码构建、资源转换一致性和桌面检查；公开发布包的安装流程仍待 Pocket 真机验收。
