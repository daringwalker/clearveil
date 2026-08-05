# 更新记录

格式参考 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循
[Semantic Versioning](https://semver.org/lang/zh-CN/)。

## [Unreleased]

## [0.2.1-beta.1] - 2026-08-05

### Changed

- 提升预发布版本号，使 GitHub、Arch、DEB、RPM、AppImage 和应用内版本采用统一的
  `0.2.1-beta.1` 版本策略。

## [0.2.0-beta.2] - 2026-08-05

### Added

- 增加 Ubuntu 24.04 与 Debian 13 的独立 DEB 构建、测试和发布产物。
- 在 CI 和 DEB 发布任务中验证英文、简体中文 OCR 模型及真实识别结果。
- 图片信息、状态栏和临时状态消息支持选择及复制文字。

### Changed

- 最低 Qt 版本调整为 6.4，并为较新调色板 API 提供版本兼容处理。
- DEB 包推荐安装英文和简体中文 Tesseract 模型，并在文件名中标明目标发行版。

### Fixed

- 修复图片信息面板为支持文字选择而重复绘制文字、行高重叠的问题。
- 取消信息面板整行选中背景，使实际文字选区更容易辨认。

## [0.2.0-beta.1] - 2026-08-05

### Added

- 异步目录浏览、缩略图双来源和可配置缓存。
- 可调整的界面布局和窗内悬浮面板。
- 改进的裁剪、取色器、幻灯片与缩放锁定工作流。
- 元数据、直方图、色彩管理、触控手势和运行时格式诊断。
- 基于 libvips 的低内存超大图片读取与导出管线。
- 本地 OCR、字符级文字选择和复制。

### Changed

- 整理公开仓库文档、构建配置、贡献规范和发布验证流程。
- 内置浅色与深色主题改用 KDE Breeze 配色，并在可用时使用系统 Breeze Qt 样式。
- 中文显示名调整为“云开见月明”。

## [0.1.0] - 2026-07-28

### Added

- 首个开发预览版，包含基本查看、目录浏览、轻量编辑和中英文界面。

[Unreleased]: https://github.com/daringwalker/clearveil/compare/v0.2.1-beta.1...HEAD
[0.2.1-beta.1]: https://github.com/daringwalker/clearveil/compare/v0.2.0-beta.2...v0.2.1-beta.1
[0.2.0-beta.2]: https://github.com/daringwalker/clearveil/compare/v0.2.0-beta.1...v0.2.0-beta.2
[0.2.0-beta.1]: https://github.com/daringwalker/clearveil/releases/tag/v0.2.0-beta.1
[0.1.0]: https://github.com/daringwalker/clearveil/releases/tag/v0.1.0
