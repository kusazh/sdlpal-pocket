# PAL98 动画

安装工具会自动处理游戏目录中的 `1.AVI` 至 `6.AVI`，电脑需要安装 FFmpeg。使用 `--without-avi` 可跳过。

转换后的动画随游戏一起复制到 SD 卡。播放时按确认或取消可跳过。动画保留原始尺寸，居中显示。

## 开发检查

在新的 `build/` 子目录中，将准备好的 PAL98 文件链接为桌面程序使用的文件名：

| 安装文件 | 桌面文件名 |
|---|---|
| `pal98.pak` | `pal.pak` |
| `pal98-fm.pcm` | `pal-fm.pcm` |
| `pal98-sfx.mkf` | `pal-sfx.mkf` |
| `pal98-avi.pav` | `pal-avi.pav` |

在仓库根目录编译桌面程序：

```sh
make -C src/pal -o pal.pak -o pal-fm.pcm desktop
```

进入上述测试目录，运行桌面程序的绝对路径：

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /path/to/pal-desktop --avi-smoke-test
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /path/to/pal-desktop --avi-realtime-test
```

第一项检查全部动画、居中画面、跳过和播放后的恢复；第二项按实际播放速度检查第一段动画。
