# 运行时来源与授权

安装工具会自动下载并校验所需的 [openfpgaOS SDK](https://github.com/openfpgaOS/openfpgaSDK) 运行时。离线安装可用 `--runtime /path/to/runtime` 指定本地副本，版本必须与本包一致。

版本与校验信息记录在安装工具、发布元数据和 `runtime/MANIFEST` 中。所需文件为 `pocket/os25.rbf_r`、`pocket/loader.bin`、`pocket/os.bin`，不包含在公开发布包中。

运行时独立于本项目授权。源码及授权声明见 [openfpgaOS](https://github.com/openfpgaOS/openfpgaOS) 的 `NOTICE`。其中包含的 Analogue Pocket Framework、Intel IP 和第三方 RTL，应遵守对应版本的上游授权；本项目的 GPLv3 和 SDK 的 Apache 许可证不替代这些授权。
