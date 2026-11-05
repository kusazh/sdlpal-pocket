# openfpgaOS runtime dependency

本公开包不再分发 FPGA bitstream、loader 或 OS 二进制。安装时由用户选择从上游获取或提供已有副本。这些文件独立于 GPL 应用，在上游自己的授权条件下提供。

固定 SDK 版本：a408ddc12aed0dfaa4aa22c06af82f829db77126

来源：https://github.com/openfpgaOS/openfpgaSDK/tree/a408ddc12aed0dfaa4aa22c06af82f829db77126/runtime

上游运行时源码版本：618a3eb（SDK runtime/MANIFEST 标注）。

源码与授权声明：https://github.com/openfpgaOS/openfpgaOS/tree/618a3eb

特别参见：https://github.com/openfpgaOS/openfpgaOS/blob/618a3eb/NOTICE

运行时包含 Analogue Pocket Framework、Intel IP 和第三方 RTL，其授权不能由本移植程序的 GPL 或 SDK 的 Apache 许可证代替。本包的安装工具不会重新授权这些组件；取得运行时后应遵守其上游条件。

安装工具只获取以下三个文件，校验上游 MANIFEST 的 MD5（用于版本一致性，来源固定为 HTTPS 上游地址）：

| 文件 | MD5 |
|---|---|
| pocket/os25.rbf_r | 637c8e0f2656f5ac7344e43f98262617 |
| pocket/loader.bin | 48c625e82daf68d14680893ac8cb27af |
| pocket/os.bin | cdc237dc18192844beff9375ebe4e7b1 |

不获取或分发 SoundFont、Roland 采样、游戏资源或示例音乐。安装后形成的 SD 卡目录不是本项目的可再分发公开包。
