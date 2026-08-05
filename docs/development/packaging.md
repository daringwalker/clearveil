# 打包与发布

## 发布前检查

1. 更新 `CMakeLists.txt` 中的项目版本。
2. 更新 `CHANGELOG.md` 和 AppStream release 条目。
3. 以干净构建目录运行全部测试。
4. 验证 Desktop Entry、AppStream 元数据和 Flatpak manifest。
5. 生成源码包并确认不包含构建目录、缓存、二进制或本地工具元数据。
6. 创建签名的 `vX.Y.Z` 标签。
7. 生成源码包 SHA-256，再更新 `packaging/PKGBUILD`，不能以 `SKIP` 发布。

## 源码包

```bash
cmake -S . -B build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cpack --config build-release/CPackSourceConfig.cmake -G TGZ
```

GitHub 自动生成的 tag archive 也可以作为 Arch 包源，但必须记录固定校验和。

## GitHub Release 二进制包

推送与 `VERSION` 一致的 `v*` 标签后，Release 工作流默认并行生成以下 x86_64 产物：

- Arch Linux：由 `makepkg` 在官方 Arch 容器中生成 `.pkg.tar.zst`；
- Ubuntu 24.04：由 CPack 在 Ubuntu 24.04 原生环境生成带 `_ubuntu24.04_` 标识的
  `.deb`；
- Debian 13：由 CPack 在 Debian 13 原生容器生成带 `_debian13_` 标识的 `.deb`；
- 两种 DEB 的依赖均由 `dpkg-shlibdeps` 从实际链接库计算，并推荐安装英文和简体中文
  Tesseract 模型；
- Fedora/RHEL 系：由 CPack/rpmbuild 在 Fedora 44 中生成 `.rpm`；
- 通用包：在 Ubuntu 22.04 基线上使用 Qt 6.8 LTS 和 linuxdeploy 生成
  `.AppImage`，同时打包 Qt 的 X11 与 Wayland 平台插件。

工作流只有在五种二进制产物、源码包和发布用 PKGBUILD 全部存在时才会发布，并为全部
产物重新生成统一的 `SHA256SUMS`。DEB/RPM/Arch 包使用发行版依赖；AppImage 打包核心
运行库，但额外图片编解码插件和 OCR 语言模型仍可由系统提供。

本地验证已汇总的产物：

```bash
bash tools/check_release_artifacts.sh release-artifacts "$(<VERSION)"
```

## Flatpak

Flatpak 依赖必须固定版本和提交。提交 Flathub 前应重新评估文件系统权限，并在真实
KDE Wayland 和 GNOME Wayland 会话中验证文件选择器、拖放、回收站、外部打开与
单实例转发。

当前 manifest 使用 `--filesystem=host`，因为 Clearveil 不只通过文件选择器读取图片，
还需要接收文件管理器和命令行传入的路径，并执行另存为、重命名、移动和回收站操作。
这是较宽的权限：提交 Flathub 前应验证文件门户能否覆盖这些工作流；如果可以，应
缩小为门户授权和必要的可移动存储路径。

## 发布渠道

首个公开版本使用 `v0.2.0-beta.1` 预发布标记；稳定性达到公开承诺后再发布正式
`v0.2.0`。完整操作步骤见 [GitHub 发布流程](releasing.md)。
