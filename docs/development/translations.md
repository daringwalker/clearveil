# 翻译

源字符串使用英文，简体中文维护在
`translations/clearveil_zh_CN.ts`。构建时由 `lrelease6` 生成 QM 文件并嵌入应用。

更新翻译的一般流程：

```bash
lupdate6 src -ts translations/clearveil_zh_CN.ts
lrelease6 translations/clearveil_zh_CN.ts \
  -qm /tmp/clearveil_zh_CN.qm
```

提交前检查：

- 没有意外变更 Qt translation context；
- 没有未完成的用户可见字符串；
- 删除已经确认不再需要的 vanished/obsolete 项；
- 中英文菜单助记键和快捷键没有冲突；
- 浅色与深色主题下文本均可辨认。

结构重构时不要仅为了目录整洁批量改变类名或翻译上下文。
