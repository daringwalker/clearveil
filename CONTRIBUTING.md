# 参与贡献

感谢参与 Clearveil。项目优先保证图片浏览体验、稳定性、性能和 Linux 桌面集成，
不计划发展为评分、标签、注释或重型图库管理软件。

## 开始之前

- 缺陷和功能请求先搜索现有 Issues；
- 大型功能或架构变化先创建讨论 Issue；
- 安全问题不要提交公开 Issue，请遵循 [SECURITY.md](SECURITY.md)；
- UI 改动应说明与现有交互原则的关系，并提供前后截图。

## 本地验证

按照 [构建文档](docs/development/building.md) 安装依赖，然后运行：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

涉及界面、主题、拖放、文件门户、单实例或窗口管理的改动，还应在真实 Wayland 桌面
进行人工验证。测试图片应当允许再分发且不包含隐私信息。

## 代码约定

- 使用 C++20 和 Qt 6.6 可用的 API；
- 后台线程不直接访问 QWidget 或当前文档；
- 新的可复用状态机放入明确控制器或模型，不继续扩充 `MainWindow`；
- 避免无关的大规模格式化、类名和翻译上下文变化；
- 修复缺陷时增加能复现原问题的测试；
- 保持用户设置键和已有配置兼容。

提交应小而完整，标题使用祈使语气并说明意图。Pull Request 中说明验证范围、
已知限制及界面截图。
