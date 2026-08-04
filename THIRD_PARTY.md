# 第三方组件

Clearveil 本身以 GPL-3.0-or-later 发布。构建和运行时使用以下主要组件：

| 组件 | 用途 | 许可证 |
|---|---|---|
| Qt 6 | 应用框架、界面、图片插件和平台集成 | LGPL-3.0-only / GPL-3.0-only / 商业许可 |
| libvips | 超大图片按需读取、分块显示和流式导出 | LGPL-2.1-or-later |
| Exiv2 | EXIF、IPTC 和 XMP 元数据 | GPL-2.0-or-later |
| colord | X11 显示器色彩配置发现 | GPL-2.0-or-later |
| Tesseract OCR | 按需图片文字识别与字符坐标 | Apache-2.0 |
| Leptonica | Tesseract 图像处理依赖 | BSD-2-Clause |
| KDE Breeze | 内置浅色/深色调色板与控件风格参考 | LGPL-2.0-or-later |

实际二进制能力和传递依赖由构建使用的 Linux 发行版与 Flatpak runtime 决定。打包者
应保留对应软件包的许可证文件，并遵守其动态链接、源代码提供和署名要求。

KDE Breeze 的颜色值参照其公开的 `BreezeLight.colors` 与 `BreezeDark.colors`；Clearveil
不打包 Breeze 控件风格，运行时仅在系统已提供相应 Qt 插件时加载。
