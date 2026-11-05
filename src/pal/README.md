# 开发说明

用户安装与操作见 [RELEASE.md](RELEASE.md)。游戏目录可以是繁体 DOS 版或 Windows 版（PAL98）。

## 构建与发布

在仓库根目录运行，需要容器运行环境：

```sh
make build
make release
```

程序输出到 `.obj/pal/app.elf`，发布包输出到 `build/releases/`。构建不需要游戏资源。

公开发布只上传 ZIP 和对应的 `.zip.sha256`，不要上传生成的安装目录或游戏文件。

## 桌面检查

需要 SDL2、C++ 编译器和自己的 DOS 游戏目录：

```sh
make test GAME_DIR=/path/to/pal
```

可通过 `SDL2_CONFIG` 指定 `sdl2-config`。测试检查资源加载、画面、音乐和存档，结果保存在 `build/`。

PAL98 可使用准备工具生成的资源运行桌面检查。AVI 转换与检查见 [MOVIES.md](MOVIES.md)。

## 代码位置

- `src/pal/engine/`：SDLPAL 引擎。
- `src/pal/platform_*.c`：Pocket 的显示、声音、文件和动画适配。
- `src/pal/prepare.py`：用户安装工具。
- `src/pal/release.py`：公开发布包生成工具。
- `src/sdk/`、`tools/`、`scripts/`：保留的 SDK 依赖，来源见仓库根目录的 `UPSTREAM.md`。

游戏画面保持 320×200，在 320×240 输出区域内居中，不做软件缩放。DOS 与 PAL98 使用独立资源和存档，共享运行时。

## 许可证

保留 SDLPAL 和各依赖的许可证。发布修改版时，随程序提供对应源码。游戏资源、转换后的音乐和动画、存档及本地测试产物不属于公开发布内容。
