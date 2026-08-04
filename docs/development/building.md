# 构建 Clearveil

## 必需组件

- 支持 C++20 的 GCC 或 Clang
- CMake 3.24+
- Ninja
- Qt 6.6+：Core、Gui、Widgets、Concurrent、DBus、Network、PrintSupport、Svg
- Qt Linguist 的 `lrelease6`
- pkg-config
- libvips

可选组件：

- Exiv2：完整 EXIF/IPTC/XMP 元数据
- colord：X11 显示器 ICC 配置自动发现
- Tesseract 5：图片文字识别；语言数据独立安装
- Qt Image Formats、KImageFormats：更多图片格式

## 发行版软件包

```bash
# Arch Linux
sudo pacman -S --needed base-devel cmake ninja pkgconf \
  qt6-base qt6-svg qt6-tools exiv2 colord libvips
sudo pacman -S --needed tesseract tesseract-data-eng tesseract-data-chi_sim

# Debian / Ubuntu
sudo apt install build-essential cmake ninja-build pkg-config \
  qt6-base-dev libqt6svg6-dev qt6-l10n-tools \
  libexiv2-dev libcolord-dev libvips-dev
sudo apt install libtesseract-dev tesseract-ocr \
  tesseract-ocr-eng tesseract-ocr-chi-sim

# Fedora
sudo dnf install gcc-c++ cmake ninja-build pkgconf-pkg-config \
  qt6-qtbase-devel qt6-qtsvg-devel qt6-qttools-linguist \
  exiv2-devel colord-devel vips-devel
sudo dnf install tesseract-devel tesseract-langpack-eng \
  tesseract-langpack-chi_sim
```

不同发行版版本可能拆分软件包；Clearveil 同时支持 Arch 常见的 `lrelease6`，以及
Debian / Ubuntu 位于 `/usr/lib/qt6/bin/lrelease` 的命名和路径。如果配置阶段仍提示
找不到翻译编译器，请安装该发行版的 Qt 6 Linguist/Qt Tools 软件包，而不是只安装
Qt 运行库。

CMake 找不到 Tesseract 时仍可构建查看器，但“文字选择工具”和“帮助 → OCR 支持”会明确提示当前构建
不含 OCR，并给出当前发行版的软件包名和安装命令。识别语言由系统安装的 traineddata 决定；
Clearveil 会自动组合所有已安装的 Tesseract 文字识别模型（不包含仅用于方向检测的 `osd`），与系统或界面语言无关，也不会下载或缓存识别模型。
打包者也可使用 `-DCLEARVEIL_ENABLE_OCR=OFF` 明确构建不含 OCR 的版本。

## 配置、构建和测试

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

普通构建不编译性能基准。使用
`-DCLEARVEIL_BUILD_BENCHMARKS=ON` 显式启用。
维护者可以使用 `-DCLEARVEIL_WARNINGS_AS_ERRORS=ON` 复现 CI 的严格警告检查。

## 安装到暂存目录

```bash
cmake --install build --prefix "$PWD/stage/usr"
```

不要以构建目录中的动态链接二进制压缩包作为跨发行版通用包。公开发布应提供源码包，
并分别通过发行版包或 Flatpak 交付二进制。
