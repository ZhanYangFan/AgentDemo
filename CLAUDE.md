# AgentDemo — Claude Code 开发指南

> Vibe Coding：人定目标与验收，AI 按规范小步实现。
> 宿主工程：`D:\UEProject\AgentDemo`（UE 5.8，模块 `AgentDemo`）。
> 原版 `.cursor/rules/` 及 `AGENTS.md` 已合并到此文件。

---

## 开工前必读（按顺序）

1. `Docs/策划案/00_面试复习Demo总纲.md` — 单项目主循环与周计划
2. `Docs/策划案/06_统一项目GDD.md` — 边界、接口、Registry、存盘、切片
3. 当前要做的模块分册 `01`～`05`
4. `Docs/开发规范/VibeCoding工作流.md` — 怎么跟 AI 协作

---

## 当前工程事实

- 路径：`D:\UEProject\AgentDemo`
- 引擎：UE 5.8，模块 `AgentDemo`
- 模板：TopDown（原 Variant_TwinStick / Variant_Strategy 已删除）
- **基础玩法框架已完成**（第三人称 WASD + 八方向动画，C++ 逻辑 + 蓝图数据配置层）：
  ```
  Source/AgentDemo/
    Core/       ← AgentGameMode / AgentGameState / AgentPlayerState / AgentHUD（C++ 框架类）
    Character/  ← AgentCharacter（第三人称 SpringArm 相机 + 八方向旋转策略）
    Player/     ← AgentPlayerController（WASD/走跑跳/鼠标视角）
  ```
  蓝图数据配置层：`BP_AgentGameMode`（类装配）/ `BP_AgentCharacter`（Mesh+动画）/ `BP_AgentPlayerController`（输入资产引用）
  输入资产：`/Game/Input/`（IMC_AgentDefault + IA_Move/Look/Jump/Sprint，A/S 用 Negate+Swizzle 修饰符）
  默认地图 `M_Town_Dev`（/Game/Maps/），GameMode 为 `BP_AgentGameMode`
- **Mannequin 资产链**（勿混淆）：`SKM_Manny_Simple`/`SKM_Quinn_Simple` 是**网格**（USkeletalMesh），`SK_Mannequin` 是**骨架**（USkeleton），`ABP_Unarmed` 是动画蓝图
- **八方向动画**：`BS_Idle_Walk_Run`（2D BlendSpace：X=方向 -180~180° 八采样、Y=速度 0/300/600）由 `ABP_Unarmed` 的 BlueprintUpdateAnimation **自驱动**（Direction/GroundSpeed/ShouldMove/IsFalling）；角色 `bUseControllerRotationYaw=true` + `bOrientRotationToMovement=false` 走完整八方向分支，C++ 不驱动动画变量
- **UE 行为坑**：LiveCoding 修改 C++ 默认值后**蓝图 CDO 不自动继承**（固化旧快照），须在蓝图 CDO 显式配置；动画蓝图图不能用 `read_graph_dsl` 读取（返回空≠图空），诊断用 `find_nodes`/`get_connected_subgraph`
- **UE 模块组织规范**（参考引擎 GASToolsets）：模块头（Public/）只声明模块类；模块实现（Private/`<Module>.cpp`）含 IMPLEMENT_MODULE + StartupModule/ShutdownModule 定义；功能类自包含（声明+日志声明在头、实现+日志定义在 cpp）；依赖单向（模块→功能类，功能类不反向引用模块）；**Windows 文件名大小写不敏感**——模块 cpp 用 `F` 前缀命名（如 `FGameplayCameraSystemToolSetModule.cpp`）避免与功能文件（`GameplayCameraSystemToolset.cpp`）撞车
- **新玩法代码**同样按域分目录，每个域内分 `Public/` 和 `Private/`

---

## 垂直切片顺序（禁止跳步横铺）

`VS0 大厅壳 → VS1 战斗 → VS2 AI 小兵 → VS3 背包制造 → VS4 任务 Agent → VS5 Boss → VS6 商业化互通 → VS7 存盘`

**一次会话只推进一个 VS 或一个子验收项。**

---

## 给 Agent 下任务时的标准话术

```
目标：<一句话>
范围：只改 <目录/类>
依据：策划案 0X §Y / GDD §Z
验收：<可勾选条目>
不要做：<明确排除>
```

---

## 硬约束

- **先读再改**：改代码前必须读相关策划案和现有代码
- **Server 权威**：伤害、背包、制造、接交任务、购买只在 Server；Client 发意图 RPC
- **跨系统只走 GDD 约定接口**：`IQuestWorldSink` / `IRewardService` 等，禁止模块直掏私有数据
- **内容 Id 必须进 Registry 白名单**后再引用
- **不扩需求**、不写无关文档、不擅自大重构
- **垂直切片**：一次只做一个 VS 的验收项，不要横向堆半成品

---

## 模块目录约定

| 目录 | 文档 | 禁止 |
|------|------|------|
| `AbilitySystem/` | 01 | 写任务进度 |
| `AI/` | 02 | 绕过 GA 直接改玩法血量 |
| `Inventory/` `Equipment/` `Craft/` | 03 | UI 直接改数量 |
| `Quest/` `Agent/` `Dialogue/` | 04 | 直接 Spawn 装备（走 RewardService） |
| `Commerce/` `Guild/` `SDK/` | 05 | 另起货币系统 |
| `Persist/` | GDD §10 | 掺业务规则 |

跨系统只用约定接口：`IQuestWorldSink`、`IRewardService`、装备→AbilitySet、BT→ActivateAbilityByTag。

---

## UE C++ 实现约定

- **纯 C++ 写逻辑**，不做蓝图逻辑（蓝图只用于：数据资产配置、动画蓝图、材质、音效）
- **禁止在 C++ 硬编码资产路径**（`ConstructorHelpers` / `FSoftObjectPath` 字面量都不行）；资产引用一律通过**蓝图子类**（如 `BP_AgentCharacter` 配置 Mesh 组件）或 **DataAsset** 数据配置层注入，C++ 只留空 `UPROPERTY(EditDefaultsOnly)` 数据点
- **文件夹命名**：大驼峰（PascalCase），如 `QuestSystem/`、`AbilitySystem/`
- **文件结构**：每个模块分 `Public/` 和 `Private/` 目录，符合 UE 模块规范
  ```
  Source/AgentDemo/<ModuleName>/
    Public/     ← 头文件（对外暴露的接口）
    Private/    ← .cpp 实现 + 内部头文件
  ```
- **类命名**：`AAgent*` / `UAgent*` / `FAgent*`（或模块前缀如 `UQuest*`），日志用 `LogAgentDemo`
- **复制与权威**：改背包/伤害/接交任务/购买只在 Server；Client 发意图 RPC
- **包含顺序**：生成头最后包含；`Build.cs` 按需加模块依赖
- 优先最小可编译骨架 → 接数据 → 接 UI；不要一次 PR 塞满整个 VS

---

## 文档规则

- `00` + `06` 管主循环与接口；与分册冲突时以它们为准
- 分册 `01`～`05` 是模块规格，不是独立工程说明书
- 改接口/Registry/存盘字段时，**同步改 `06` 与 `CLAUDE.md` 相关表**
- 开发流程写在 `Docs/开发规范/`，不要把流程塞进策划案正文

---

## 禁止事项

- 不要把策划案当成已实现代码；未落地前先搭最小可编译骨架再填逻辑
- 不要在未做 VS1 时深挖 LLM HttpAgent
- 不要新建第二个游戏工程拆系统
- 不要提交密钥、本地绝对路径配置进仓库
- 不要往已删除的 `Variant_TwinStick` / `Variant_Strategy` 目录写代码
