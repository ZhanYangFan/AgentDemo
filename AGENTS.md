# AgentDemo — Agent 开发指南

本仓库用 **Vibe Coding** 推进：人定目标与验收，AI 按规范小步实现。  
玩法规格在 `Docs/策划案/`，开发规矩在本文件与 `.cursor/rules/`。

## 开工前必读（按顺序）

1. `Docs/策划案/00_面试复习Demo总纲.md` — 单项目主循环与周计划  
2. `Docs/策划案/06_统一项目GDD.md` — 边界、接口、Registry、存盘、切片  
3. 当前要做的模块分册 `01`～`05`  
4. `Docs/开发规范/VibeCoding工作流.md` — 怎么跟 AI 协作

## 当前工程事实

- 路径：`D:\UEProject\AgentDemo`
- 引擎：UE 5.8，模块 `AgentDemo`
- 模板：TopDown（含 Variant_TwinStick / Variant_Strategy）
- **新玩法代码**放在 `Source/AgentDemo/` 下按域分目录，不要继续往 TwinStick/Strategy 变体里堆主线逻辑

## 垂直切片顺序（禁止跳步横铺）

`VS0 大厅壳 → VS1 战斗 → VS2 AI 小兵 → VS3 背包制造 → VS4 任务 Agent → VS5 Boss → VS6 商业化互通 → VS7 存盘`

一次会话只推进 **一个 VS 或一个子验收项**。

## 给 Agent 下任务时的标准话术

```
目标：<一句话>
范围：只改 <目录/类>
依据：策划案 0X §Y / GDD §Z
验收：<可勾选条目>
不要做：<明确排除>
```

## 硬约束

- Server 权威：伤害、背包、制造、接交任务、购买
- 跨系统只走 GDD 约定接口（`IQuestWorldSink` / `IRewardService` 等），禁止模块直掏私有数据
- 内容 Id 必须进 Registry 白名单后再引用
- 不扩需求、不写无关文档、不擅自大重构
- 先读再改；改完对照该模块验收清单

## 模块目录约定

| 目录 | 文档 |
|------|------|
| `AbilitySystem/` | 01 |
| `AI/` | 02 |
| `Inventory/` `Equipment/` `Craft/` | 03 |
| `Quest/` `Agent/` `Dialogue/` | 04 |
| `Commerce/` `Guild/` `SDK/` | 05 |
| `Persist/` | GDD §10 |

## 禁止事项

- 不要把策划案当成已实现代码；未落地前先搭最小可编译骨架再填逻辑  
- 不要在未做 VS1 时深挖 LLM HttpAgent  
- 不要新建第二个游戏工程拆系统  
- 不要提交密钥、本地绝对路径配置进仓库  
