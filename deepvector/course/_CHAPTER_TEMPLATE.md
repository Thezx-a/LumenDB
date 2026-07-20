# Chapter Template · 章节强制模板

> 每章（中/英）应遵循此结构，保证「点→线→面」与 Hello-Agents 风格一致。

---

```markdown
# 第 X 章：标题 / Chapter X: Title

> 一句话价值主张 / One-line value.

## 前置知识 / Prerequisites
> 📎 链接到 prerequisites 与上一章

## 学习目标 / Objectives
- [ ] …

## 本章在「面」上的位置 / Surface Context
（mermaid 片段，标出本章模块）

---

## 1. 点 Point — 核心概念与语法
### 1.1 概念
### 1.2 语法精讲（逐行）
### 1.3 最小可运行示例

## 2. 线 Line — 如何接到相邻模块
（接口、数据流、错误处理）

## 3. 面 Surface — 整系统行为
（用户请求如何流经本章代码）

## 4. 动手实践 / Hands-on Lab
### Lab A（必做）
### Lab B（挑战）

## 5. 反思思考 / Reflection
1. …
2. …

## 6. 真实面试题 / Interview Drills
> 来源标注（LeetCode 题型 / 系统设计博客 / 牛客讨论方向 / 官方文档）
### Q1
**参考答案要点：** …

## 7. 参考文档 / References
1. [真实链接](https://…) — 说明
2. 本仓库路径：`include/dv/...`
```

---

## Checklist for authors

- [ ] 无臆造论文 DOI / 假链接  
- [ ] mermaid 可渲染  
- [ ] 代码路径与 `include/dv` 一致  
- [ ] 中英两份信息对等（可详略不同，不可矛盾）  
- [ ] 至少 2 道反思 + 2 道动手 + 2 道面试  
```
