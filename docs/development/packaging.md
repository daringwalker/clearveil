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
