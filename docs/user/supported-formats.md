# 图片格式支持

Clearveil 不把编解码能力写死为一张静态清单。程序启动后通过
`QImageReader` 和 `QImageWriter` 查询当前进程实际加载到的 Qt 图片插件，因此
发行版安装的 Qt Image Formats、KImageFormats、Qt SVG 以及它们的可选编解码库
会直接决定最终能力。

在主菜单的“帮助 → 支持的图片格式…”中可以查看：

- 当前 Qt 版本；
- 每个格式族的扩展名；
- 可读、可写、只读或不可用状态；
- 通常提供该能力的后端；
- 选中不可用格式后，当前 Linux 发行版对应的软件包名和安装命令；
- 可复制的纯文本诊断报告。

安装或移除图片插件后需要重启 Clearveil，运行时能力页和文件夹过滤结果才会更新。

## 打开失败

打开失败会区分以下情况：

- 文件已移动或不存在；
- 权限或存储设备导致无法读取；
- 当前环境没有对应解码插件；
- 格式插件存在，但文件损坏、不完整或使用了尚不支持的格式变体；
- 无扩展名文件的内容无法识别。

插件缺失时，提示会显示文件扩展名、建议后端、当前发行版的软件包名和可复制的
安装命令。Clearveil 只提供指引，不会在未经用户确认的情况下修改系统软件包。

当前内置的软件包映射如下：

| 能力 | Arch Linux | Debian / Ubuntu | Fedora |
| --- | --- | --- | --- |
| Qt 扩展图片格式 | `qt6-imageformats` | `qt6-image-formats-plugins` | `qt6-qtimageformats` |
| KDE 扩展图片格式 | `kimageformats` | `kimageformat6-plugins` | `kf6-kimageformats` |
| SVG | `qt6-svg` | `qt6-svg-plugins` | `qt6-qtsvg` |

例如，Arch Linux 缺少 HEIF/HEIC 支持时，Clearveil 会显示：

```sh
sudo pacman -S kimageformats libheif
```

Arch Linux 上 AVIF、JPEG XL、OpenEXR 和相机 RAW 还会分别明确列出
`libavif`、`libjxl`、`openexr` 和 `libraw`。Debian/Ubuntu 与 Fedora 的
KImageFormats 软件包会通过依赖安装相应编解码库。发行版版本过旧而没有表中
软件包时，应使用该发行版的软件包搜索确认可用的 Qt 6/KF6 版本。

Flatpak 环境会给出单独提示：宿主机安装的插件不一定能穿过沙箱，需要使用已包含
相应图片插件的 Clearveil/Flatpak 构建。

Qt 官方说明 `QImageReader::supportedImageFormats()` 会同时返回内置格式与已加载图片
插件提供的格式；Qt Image Formats 和 KDE KImageFormats 都通过这套 Qt 图片 I/O
插件机制接入。
