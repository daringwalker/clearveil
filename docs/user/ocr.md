# 识别和复制图片文字

打开含文字的图片后，点击工具栏的“文字选择工具”光标按钮。Clearveil 会在后台识别
当前图片；完成后，鼠标移到可识别文字上会显示文本光标。

- 拖过文字可连续选择，选区使用主题强调色显示；
- 双击一个字符会选择整个词；
- `Ctrl+A` 选择识别出的全部文字；
- `Esc` 清除当前文字选区；
- `Ctrl+C` 或右键菜单复制所选文字；没有文字选区时仍复制图片。

该模式按需工作。启用后切换图片会识别新图片，关闭模式会取消等待中的任务。旧图片
的过期识别结果不会覆盖当前图片。送入 Tesseract 的位图最大边限制为 4096 像素，
字符坐标再映射回原图逻辑尺寸，以控制大图识别的峰值内存。

## 安装识别引擎和语言

Arch Linux：

```bash
sudo pacman -S tesseract tesseract-data-eng tesseract-data-chi_sim
```

Debian / Ubuntu：

```bash
sudo apt install tesseract-ocr \
  tesseract-ocr-eng tesseract-ocr-chi-sim
```

Fedora：

```bash
sudo dnf install tesseract tesseract-langpack-eng \
  tesseract-langpack-chi_sim
```

可随时打开“帮助 → OCR 支持”查看当前构建是否包含识别引擎、已经发现的语言模型、
当前发行版的软件包名称和可复制的安装命令。若 Clearveil 是从源码构建，构建阶段还需
安装相应的 Tesseract 开发包（Debian / Ubuntu 为 `libtesseract-dev`，Fedora 为
`tesseract-devel`）；普通用户使用已构建的软件包时不需要开发包。

语言数据由系统管理，Clearveil 不会下载或在用户目录中复制模型。其他语言安装相应的
Tesseract traineddata 包即可；重新启动 Clearveil 后会自动发现。识别时会组合所有已安装的
文字模型，与当前系统语言和 Clearveil 界面语言无关；`osd` 方向检测模型不作为文字语言加载。
