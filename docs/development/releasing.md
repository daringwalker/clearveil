# GitHub 发布流程

Clearveil 的首个公开版本为 `v0.2.0-beta.1`。根目录 `VERSION` 是应用显示版本和发布
标签的唯一版本来源；CHANGELOG、AppStream、Flatpak 和 PKGBUILD 必须与其同步。

## 发布前

1. 确认工作区干净，并检查 `VERSION`、`CHANGELOG.md` 和 AppStream 发布日期。
2. 运行仓库卫生、干净构建和完整测试：

   ```bash
   bash tools/check_repository_hygiene.sh
   bash tools/check_release_metadata.sh
   cmake -S . -B build-release -G Ninja \
     -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_TESTING=ON \
     -DCLEARVEIL_BUILD_BENCHMARKS=OFF
   cmake --build build-release
   ctest --test-dir build-release --output-on-failure
   ```

3. 生成并检查源码包：

   ```bash
   cpack --config build-release/CPackSourceConfig.cmake \
     -G TGZ -B release-artifacts
   bash tools/check_source_archive.sh \
     release-artifacts/clearveil-*-Source.tar.gz
   ```

4. 在真实 KDE Wayland、GNOME Wayland 和 Ubuntu 环境完成启动、打开图片、单实例、
   OCR、主题切换和文件操作验收。

## 创建发布

使用签名标签启动 GitHub Release 工作流：

```bash
bash tools/check_release_version.sh v0.2.0-beta.1
git tag -s v0.2.0-beta.1 -m "Clearveil 0.2.0 beta 1"
git push origin main v0.2.0-beta.1
```

工作流会重新构建和测试，上传源码包、`SHA256SUMS`，并生成一个写入真实源码包
校验和的 `PKGBUILD` 发布附件。预发布标签会自动创建为 GitHub prerelease。

## 发布后

- 下载源码包并验证 SHA-256；
- 使用发布附件中的 `PKGBUILD` 在干净 Arch 环境构建、安装和卸载；
- 将最终校验和同步回仓库中的 `packaging/PKGBUILD`；
- 检查 GitHub Release、CI、CodeQL 和 Dependabot 状态；
- 稳定版发布前补充 README 与 AppStream 的当前界面截图；
- 提交 Flathub 前收窄 Flatpak 文件系统权限并完成独立验收。
