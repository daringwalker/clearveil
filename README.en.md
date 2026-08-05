# Clearveil

[![CI](https://github.com/daringwalker/clearveil/actions/workflows/ci.yml/badge.svg)](https://github.com/daringwalker/clearveil/actions/workflows/ci.yml)
[![CodeQL](https://github.com/daringwalker/clearveil/actions/workflows/codeql.yml/badge.svg)](https://github.com/daringwalker/clearveil/actions/workflows/codeql.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](LICENSE)

[简体中文](README.md) · [Documentation](docs/README.md) ·
[Roadmap](docs/roadmap.md) · [Contributing](CONTRIBUTING.md)

Clearveil (Chinese display name: 云开见月明) is an open-source image viewer for the
Linux desktop. It focuses on a canvas-first, single-window browsing experience.
Opening more files reuses the existing process and preserves the list of files
the user explicitly opened, while folder browsing remains one click away.

The project aims for a fast, direct, and configurable everyday image-viewing
experience while maintaining independent code, branding, and visual assets.

## Project background

Clearveil is a personal project shaped by the maintainer's own viewing habits
and interests. It was started to address the lack of a simple Linux image viewer
that fits those habits. The maintainer defines the requirements, product
direction, interaction decisions, testing, and acceptance criteria; all program
code is generated or modified using AI tools.

## Privacy, security, and disclaimer

The maintainer will not intentionally add user-data collection, telemetry,
tracking, or backdoor code to Clearveil. The software may nevertheless contain
unknown defects, security issues, or incompatibilities, and is not guaranteed to
be error-free, uninterrupted, or suitable for any particular purpose.

Anyone may use, study, modify, and redistribute the source code and compiled
builds free of charge under GPL-3.0-or-later. To the maximum extent permitted by
applicable law, users are responsible for evaluating and accepting the risks of
running, installing, modifying, or distributing the software. The maintainer is
not liable for direct, indirect, or other consequences arising from its use. See
[LICENSE](LICENSE) for the complete license and warranty terms.

> Clearveil is currently early-stage software. Core viewing workflows are
> usable, while distribution packaging and cross-desktop compatibility are
> still being prepared for a public beta.

## Screenshots

![Clearveil's default image-viewing interface](docs/assets/screenshots/clearveil-main.png)

![Clearveil's folder browser](docs/assets/screenshots/clearveil-folder-browser.png)

These screenshots were captured on KDE Plasma/Wayland with a fresh default
configuration. The example images come from the NASA Image and Video Library;
see the [documentation asset credits](docs/assets/README.md) for image IDs,
source links, and usage information. NASA does not endorse Clearveil.

## Highlights

- Single-instance multi-file sessions and a switchable current-folder filmstrip
- Asynchronous decoding, bounded caches, prefetching, and a libvips large-image path
- Fit, 100%, continuous zoom, rotate, flip, and fullscreen viewing modes
- Animated GIF/WebP/APNG and multi-frame TIFF/ICO support
- EXIF/IPTC/XMP metadata, histograms, and a precision color picker
- On-demand OCR with character-level mouse selection and text copying
- Crop, resize, tonal adjustment, export, and common desktop file operations
- Configurable toolbar, filmstrip, panels, themes, and Chinese/English UI
- KDE/GNOME and Wayland/X11 integration

## Build

Clearveil requires C++20, CMake 3.24+, Qt 6.4+, and libvips. Exiv2 enables full
metadata support.

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/clearveil [image paths...]
```

See the [build guide](docs/development/building.md), the
[documentation index](docs/README.md), and the
[contribution guide](CONTRIBUTING.md) for more information.

The first public version uses the `v0.2.0-beta.1` prerelease tag. GitHub Release
provides CI-built Arch Linux, Ubuntu 24.04 DEB, Debian 13 DEB, RPM, and AppImage packages alongside the
source archive, SHA-256 checksums, and a PKGBUILD carrying the archive's actual
checksum. See the
[release procedure](docs/development/releasing.md) for the complete process.

Clearveil is licensed under [GPL-3.0-or-later](LICENSE). See
[COPYRIGHT.md](COPYRIGHT.md) for the initial copyright and contribution terms.
