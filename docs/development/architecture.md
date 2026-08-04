# Clearveil 架构

本文记录代码的职责边界和渐进式重构顺序。重构必须保持用户配置兼容，并为每次
拆分增加独立测试；不以一次性重写替代可运行的软件。

## 设计原则

- `MainWindow` 是界面组合根，只组装动作、控件和控制器，不实现可复用状态机。
- 文档、图片序列和设置模型不依赖 `MainWindow` 或具体控件。
- 控制器通过明确的输入、信号和小型接口协调界面；不通过查找任意子控件工作。
- 设置键由拥有该设置的模块集中维护，旧键迁移也在该模块完成。
- 后台任务只返回值对象；Qt 控件和当前文档只能在 GUI 线程更新。
- 新模块先加入 `clearveil_core` 并具备独立测试，再由主窗口接入。

## 源码布局

当前重构采用保持类名和翻译上下文不变的渐进方式：

```text
src/
├── app/                 应用入口、动作、设置、MainWindow、单实例
├── browser/             文件夹导航、扫描、监控与文件操作
├── domain/              图片文档和图片序列
├── filmstrip/           缩略图模型、视图、布局与缓存
├── imaging/             解码、色彩、格式、图片源与导出
├── platform/            桌面集成、系统外观和窗口行为
├── ui/
│   ├── dialogs/         关于、编辑、格式能力等对话框
│   └── panels/          信息、取色器、界面布局和面板框架
├── viewer/              画布、手势、平铺视图和查看状态
├── workflows/           加载、编辑、会话与幻灯片协调
├── icons/               项目自有矢量图标
└── resources.qrc        Qt 资源入口

tests/
├── CMakeLists.txt        测试目标与回归测试组
└── integration/         当前跨模块应用测试

cmake/
├── ClearveilWarnings.cmake
└── ClearveilPackaging.cmake
```

目录移动不伴随命名空间或类名批量修改，以免无意义地改变 Qt translation context。
下一轮将现有集成测试按职责拆成共享测试支持、模型单元测试和应用集成测试，并逐步将
当前综合性的 `clearveil_core` 拆成依赖方向更明确的内部目标。

## 当前边界

```text
main / SingleInstance
        │
        ▼
MainWindow（组合、菜单、顶层工作流）
   ├── PanelLayoutController（停靠、悬浮、锁定、位置持久化）
   ├── PanelTitleBar / PersistentMenu（面板抓取、形态切换、连续配置）
   ├── ClearveilIcon（主题自适应的统一 SVG 动作图标）
   ├── ApplicationSettings（默认值、校验、配置读写）
   ├── ActionRegistry（动作 ID、工具栏与快捷键描述）
   ├── ImageSessionController（已打开列表、目录列表、当前与待加载项）
   ├── FilmstripController（来源切换、选择同步、滚动位置）
   ├── FilmstripView（缩略图绘制、横纵布局、覆盖滚动条与关闭入口）
   ├── DirectoryMonitor（目录变化监听与合并刷新请求）
   ├── ImageLoadController（异步解码、最后请求、取消、内存缓存与预读）
   ├── ImageDecoder / ImageSource（后端选择、受限预览、区域读取与流式导出）
   ├── VipsImageSource（超大图片磁盘后备、随机区域读取与有界内存）
   ├── TiledImageViewModel（可见区域分块调度与有界瓦片缓存）
   ├── LargeImageSampleController（合并高频请求并异步读取精确原始像素）
   ├── DisplayColor（自动目标解析与纯图像色彩转换）
   ├── DisplayColorController（屏幕变化、异步转换与最后请求）
   ├── SlideshowController（计时、顺序、随机与全屏生命周期）
   ├── ColorPickerController（当前样本、格式化、画布与面板协调）
   ├── OcrController（单线程识别、最后请求和过期结果丢弃）
   ├── OcrEngine / OcrResult（Tesseract 适配、有界输入和字符坐标）
   ├── OcrTextSelectionModel（命中测试、阅读顺序和文本选区）
   ├── CanvasAppearanceController（画布外观动作与透明棋盘格状态）
   ├── CanvasGestureController（原生触控板与触屏双指手势状态）
   ├── SystemAppearanceController（桌面门户深浅色偏好与实时变更）
   ├── WindowDragController（顶部空白区域与系统窗口拖动）
   ├── FileOperations（重命名、复制、移动、回收站与外部启动）
   ├── ImageExportService（格式归一、原子编码与结构化结果）
   ├── DesktopIntegration（打印渲染与桌面壁纸门户）
   ├── ImageEditController（编辑命令、可用性、参数与结果）
   ├── WindowModeController（窗口标志、全屏快照与窗口适应图片）
   ├── ImageDocument（图像内容、编辑、撤销、保存）
   ├── ImageSequence（已打开图片的顺序与导航）
   ├── ImageCanvas（绘制、缩放、平移、裁剪、取色与文字选择输入）
   ├── ThumbnailModel / PersistentThumbnailCache
   └── FrameController / MetadataPanel / BrowserWidget / CompareWidget
```

目前已经完成二十二个基础边界：`PanelLayoutController` 拥有面板注册表、浮动动作同步、
默认悬浮位置、布局锁定和 `layout/docks/*` 的读写；`ApplicationSettings` 集中管理
设置默认值、合法化和兼容键；`ActionRegistry` 让工具栏、快捷键编辑器和鼠标动作
共享同一动作 ID；`ImageSessionController` 将目录预览和已打开列表明确分离；
`FilmstripController` 管理常驻缩略图模型之间的切换、选择与滚动状态；
`FilmstripView` 从主窗口中接管缩略图委托、文件名显隐、横纵布局、覆盖滚动条和关闭
按钮命中；`DirectoryMonitor` 合并文件系统通知，目录增删只增量更新模型而不重置。
`ImageLoadController` 管理异步主图请求、逻辑取消、缓存和相邻预读；缓存保存完整
`ImageLoadResult`，因此大图再次激活时会复用受限预览、逻辑尺寸和区域源，而不是只
复用一张丢失原图能力的 `QImage`；同步首次打开也会写入同一缓存。
`SlideshowController` 管理播放状态、计时、随机顺序及由播放触发的全屏生命周期；
`ColorPickerController` 管理当前像素样本、颜色格式化以及画布与取色面板之间的协调。
`FileOperations` 统一文件路径校验、冲突检测、跨文件系统移动回退和结构化错误结果。
`ImageExportService` 统一普通保存与帧导出的原子写入，`DesktopIntegration` 隔离打印
渲染和桌面壁纸门户调用；`ImageEditController` 统一编辑命令的可用性、参数校验、
无变化判断及撤销/重做结果。
`WindowModeController` 拥有无边框/置顶标志组合、全屏前后的窗口与组件状态快照，
并合并连续的“窗口适应图片”请求，避免这些状态重新散落到组合根。
`ImageDecoder` 将文件读取、安全尺寸限制、源色彩空间保留和性能指标从文档模型分离。
普通图片继续返回完整 `QImage`；估算解码内存达到 128 MiB 的图片改为返回
`ImageSource` 与最大边 2048px 的预览。`VipsImageSource` 把不可随机读取的 PNG 等格式
解压到自动删除的临时磁盘后备，由 `TiledImageViewModel` 只读取当前可见瓦片；缓存有界，
不把完整原始像素常驻内存。瓦片按到视口中心的距离排序，线程池中只保留正在执行的
一个请求，平移或缩放会替换所有未开始的旧请求。文档离开活动状态时释放随机访问
流水线和临时后备，轻量预览仍可由主图缓存复用。`LargeImageSampleController` 合并鼠标移动请求并在后台读取
11×11 原始像素区域，取色不会退化为预览近似值。停止令牌贯穿区域读取与导出。
`DisplayColor` 负责无界面的目标解析和色彩转换；`DisplayColorController` 监听窗口所在
屏幕、异步转换并丢弃过期结果。`ImageCanvas` 分别保存预览源图、显示图、逻辑尺寸和
可选区域源；适应显示先绘制预览，放大超过预览分辨率后覆盖可见原始瓦片。普通图片
的取色、编辑与保存仍读取完整原始像素；大图取色和保存分别通过区域读取与流式编码
保持原始精度。
`CanvasAppearanceController` 将可持久化的透明棋盘格动作与画布绘制状态同步；主题仍
决定完整画布底色，棋盘格仅裁剪到透明图片的实际显示矩形，未侵入主窗口绘制逻辑。
`SystemAppearanceController` 通过 `org.freedesktop.portal.Settings` 读取并监听
`org.freedesktop.appearance/color-scheme` 的外观变更通知。“系统”主题不再解析为应用
内置的深色或浅色，而是恢复 Qt 桌面平台插件在启动时提供的当前控件风格、调色板和
强调色；明确的浅色/深色选择由独立的 `BreezeTheme` 模块提供 KDE Breeze 兼容调色板，
并优先加载 Breeze 控件风格、不可用时回退到 Fusion。标准菜单、工具栏和复选框始终
交给选中的 Qt 控件风格绘制，Clearveil 特有组件只使用调色板角色定义结构化外观。
`CanvasGestureController` 保存双指手势的上一组中心点与距离，只产生增量缩放和位移；
`ImageCanvas` 负责把 Qt 原生触控板事件及触屏事件转换为该输入，不把设备状态泄漏到
主窗口。Wayland 原生双指捏合使用 Qt 的增量比例，三指平移使用像素位移。
`WindowDragController` 只在菜单栏和工具栏的空白或弹性间隔区域请求系统窗口移动，
菜单项、按钮、滑块和输入控件仍保持原有点击行为。
主窗口只保留这些模块的组装和产品级组合关系。

## 依赖规则

1. `clearveil_core` 不依赖应用入口、单实例实现或 `MainWindow`。
2. 领域对象不能访问 `QSettings`；界面偏好由设置模型或相应 UI 控制器持久化。
3. 解码、预读和缩略图任务不能持有 QWidget 指针。
4. 控制器不得根据界面显示文字定位控件，稳定连接使用指针、枚举或对象 ID。
5. 跨模块数据使用路径、索引、`QImage` 和专用结果结构，不暴露模块私有成员。

## 后续拆分顺序

### 1. 设置与动作模型（已完成）

`ApplicationSettings` 已集中管理主题、语言、输入映射、缩略图缓存和窗口选项，负责
默认值及范围校验；`ActionRegistry` 已让工具栏、快捷键编辑器和鼠标动作共享同一
动作描述、工具栏图标来源和紧凑尺寸下的稳定缩放，并删除散落的 ID 到 `QAction`
映射；设置页直接使用同一份动作名称和图标。所有可配置工具栏动作都使用同一套
24 × 24、统一线宽的内置 SVG，并由 `ClearveilIcon` 在绘制时读取当前调色板；普通、
激活和禁用状态不再切换图形，只调整颜色与按钮强调。后续新增配置迁移时只扩展设置模型。

验收结果：设置往返与损坏值校验可脱离完整窗口测试；动作布局去重、未知项过滤、
缺失项补全和快捷键编码均有控制器级测试。

### 2. 图片会话与缩略图协调（已完成）

`ImageSessionController` 已管理“已打开的图片”、当前目录浏览序列、当前项和
“单击预览/双击加入已打开列表”的规则；`FilmstripController` 已负责模型切换、
选择同步和每个来源的滚动位置。两者都不重建工具栏，也不会因为页签切换重复扫描目录。

验收结果：目录预览、显式加入、关闭当前/非当前条目、来源切换、选择同步和未知路径
清选均已有独立测试；原有幻灯片激活及大目录模型复用回归继续通过。

### 3. 主图加载管线（已完成）

`ImageLoadController` 已封装请求代次、逻辑取消、缓存和相邻预读；`MainWindow`
只提交路径并消费加载结果。主图缓存预算已从固定值改成首选项，范围为
16–4096 MiB，默认 256 MiB，退出程序后清空，不写入磁盘。

验收结果：快速切换只交付最后请求；取消、缓存命中、预读复用和容量合法化均有
确定性测试。解码器内部的协作式中断和主图加载耗时指标留在后续性能工作中。

### 4. 工具与编辑命令（已完成）

`SlideshowController` 已形成独立状态机，顺序循环、随机不重复、序列失效自动停止、
设置同步和全屏进入/恢复均不再由主窗口维护。`ColorPickerController` 已集中当前样本、
位置、HEX/RGB/RGBA/HSL 格式化、复制请求和画布/面板信号协调，并保留固定取色、
Esc 恢复、键盘微调及历史选择行为。

裁剪区域的创建、移动、八个控制点调整、数值输入和提交已经完整封装在 `CropDialog`，
主窗口只负责将接受后的矩形交给 `ImageDocument`，当前没有再增加空转发控制器的必要。
`ImageEditController` 统一旋转、翻转、裁剪、缩放、颜色调整、红眼校正、撤销和重做，
明确区分无图片、多帧模式、非法参数、无实际变化和无历史项。`ImageDocument` 的修改
方法直接返回是否产生新历史状态，不需要通过信号次数反推结果。

验收结果：幻灯片状态转换与取色格式化有独立控制器测试；目录幻灯片缩略图激活、
取色固定/恢复/微调/历史/复制以及裁剪八方向调整的原有窗口测试继续通过。编辑命令
还覆盖参数边界、无变化、尺寸前后状态、撤销/重做、红眼命中和多帧禁用。

### 5. 文件操作（已完成）

`FileOperations` 已集中重命名、复制、移动、移入回收站、在文件管理器中显示和使用
指定应用程序打开。所有操作返回统一的路径、错误类型和底层错误详情；跨文件系统移动
使用“复制、删除源文件、失败时清理目标”的回退流程。确认框、文件选择器和成功后的
图片会话更新仍由 `MainWindow` 负责。

验收结果：非法名称、源文件/目标目录缺失、目标冲突、同位置无操作、重命名、复制、
移动、外部程序校验以及移动后的会话路径替换均有确定性测试。

### 6. 输出与桌面集成（已完成）

`ImageExportService` 使用 `QSaveFile` 统一普通图片保存和动画/多页当前帧导出，负责
扩展名到编码格式的归一、质量范围及打开/编码/提交错误分类。`ImageDocument` 在成功
写入后才更新文件路径和已保存历史点；编码失败不会截断已有目标文件。

`DesktopIntegration` 接收已经由界面配置好的打印机并完成等比例居中渲染，同时封装
XDG Desktop Portal 壁纸请求。打印对话框和用户提示仍属于 `MainWindow`，底层绘制、
文件描述符和 DBus 生命周期不再泄漏到组合根。

验收结果：格式归一、空图、无效目录、成功 PNG、未知编码器保持原文件、PDF 打印和
壁纸源文件缺失均有确定性测试；原有文档保存和当前帧导出流程继续复用同一路径。

### 7. 窗口呈现状态（已完成）

`WindowModeController` 统一管理无边框、总在最前、全屏进入/退出、全屏组件可见性快照
以及窗口适应图片。连续图片或缩放变化产生的尺寸请求在事件循环内合并，窗口标志变化
后会保留普通、最大化、最小化或全屏状态。菜单栏和左上角紧凑菜单仍由 `MainWindow`
按产品组合关系显示。

验收结果：控制器级测试覆盖全屏隐藏与恢复、尺寸请求合并和窗口标志组合；完整窗口
回归继续覆盖全屏选项、窗口适应图片与紧凑菜单联动。

### 8. 面板交互与信息入口（已完成）

`PanelLayoutController` 为每个停靠面板安装统一的 `PanelTitleBar`：悬浮时始终保留
可拖动抓取区和停靠按钮，停靠且锁定时压缩为 4 px，避免占用内容空间。面板形态动作
与标题栏按钮双向同步。窗内悬浮面板可吸附主查看区四边或其他悬浮面板四边，并保存
位置、尺寸、目标面板、吸附方向和相对偏移；布局变化经 300 ms 防抖后主动同步，
正常关闭仍执行最终保存。视图菜单中的面板项使用
`PersistentMenu`，可在一次展开中连续显示、隐藏或悬浮多个面板。原先内容简陋且与
信息面板重复的“图片信息”对话框已移除，完整元数据和直方图只保留一个信息面板入口。

## 显示与格式后端阶段

核心架构稳定阶段已经完成，“显示与格式后端”阶段的第一个里程碑也已完成：主图解码
拥有独立接口、结构化错误、停止令牌和可重复性能基准。下一步建立“源图色彩空间 →
屏幕色彩空间”的输出变换，避免继续在解码时无条件丢弃到 sRGB；之后再评估 CMYK、
HDR 和可选格式后端。插件 ABI 仍要等接口和跨发行版数据稳定后再定义。

### 8. 解码后端基础（已完成）

`ImageDecoder` 返回源尺寸、解码尺寸、格式、内存量、耗时和色彩转换信息，并区分取消、
尺寸超限、读取失败及色彩转换失败。`ImageLoadController` 在新请求、取消、预读替换和
析构时请求停止；Qt `QImageReader::read()` 本身仍是不可中断调用，但读取前后与色彩
转换前后已有检查点，未来后端可以在内部轮询相同令牌。

验收结果：确定性测试覆盖 Display P3 到 sRGB、尺寸限制、预取消及运行中停止令牌；
`clearveil_image_decode_benchmark` 生成固定语料并输出 JSON、中位耗时和回归门槛。

### 9. 自动显示色彩管线（基础完成）

解码阶段默认保留图片嵌入色彩空间。Wayland 会把应用表面按 sRGB 提交给合成器，避免
应用先转换到显示器 ICC 后又被合成器二次转换；X11 构建在检测到 colord 时会根据
`XRANDR_name` 自动读取当前输出的默认 ICC，找不到服务、设备、配置文件或配置无效时
安静降级到 sRGB。整个过程没有普通用户设置项或弹窗。

屏幕目标解析和源图到显示图的转换都在工作线程执行，并使用独立代次与停止令牌；窗口
切换屏幕或快速切换图片时只应用最后结果。`MainWindow` 只创建控制器，并在文档/帧变化
时提交图片，不包含平台探测、ICC 读取或色彩转换代码。

验收结果：测试覆盖 Display P3 到 sRGB、无配置图片的 sRGB 假定、取消和快速切图；
KDE Wayland 实机验证启动、单实例传入第二张图片及缩略图切换后画布均持续正常显示。
Wayland 原生 `color-management-v1` 表面标记仍等待 Qt 提供稳定公开接口。

## 测试分层

- 领域测试：图片文档、序列、格式能力与缓存，不创建顶层窗口。
- 控制器测试：使用最小 Qt 控件或伪实现验证状态转换和配置往返。
- 主窗口回归：验证模块组装以及关键用户流程，不重复测试全部内部组合。
- KDE/GNOME Wayland 验收：覆盖悬浮窗口、主题、HiDPI、输入设备和真实窗口管理器行为。
