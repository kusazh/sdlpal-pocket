# openfpgaOS runtime dependency

本公开包不再分发 FPGA bitstream、loader 或 OS 二进制。安装时由用户选择从上游获取或提供已有副本。这些文件独立于 GPL 应用，在上游自己的授权条件下提供。

运行时来自 [openfpgaOS SDK](https://github.com/openfpgaOS/openfpgaSDK) 的 `runtime/` 目录。安装工具中记录所需的 SDK 修订，发布元数据和 `runtime/MANIFEST` 记录依赖来源与校验信息。

运行时源码与授权声明见 [openfpgaOS](https://github.com/openfpgaOS/openfpgaOS)，特别参见其 `NOTICE`。应以所用运行时对应修订中的声明为准。

运行时包含 Analogue Pocket Framework、Intel IP 和第三方 RTL，其授权不能由本移植程序的 GPL 或 SDK 的 Apache 许可证代替。本包的安装工具不会重新授权这些组件；取得运行时后应遵守其上游条件。

安装工具只获取以下三个文件，并按上游 MANIFEST 校验。下载使用安装工具指定修订的 HTTPS 上游地址：

- `pocket/os25.rbf_r`
- `pocket/loader.bin`
- `pocket/os.bin`

不获取或分发 SoundFont、Roland 采样、游戏资源或示例音乐。安装后形成的 SD 卡目录不是本项目的可再分发公开包。
