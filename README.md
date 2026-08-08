# AgentDemo

基于 **Unreal Engine 5.8** 的面试复习 Demo：把 GAS 战斗、AI、背包制造、AI 任务与轻量商业化客户端串进**同一个可玩项目**。

**产品一句话**：西部小镇生存委托——在 Frontier 小镇接 AI 治安官派发的委托，靠战斗、搜刮与制造武装自己，清剿威胁并挑战矿洞 Boss；大厅侧提供公会与商城战令。

## 环境要求

| 项 | 说明 |
|----|------|
| 引擎 | Unreal Engine **5.8** |
| 模块 | `AgentDemo` |
| 模板基底 | TopDown（含 TwinStick / Strategy 变体；主线玩法请写在 `Source/AgentDemo/`） |

用 UE 5.8 打开 `AgentDemo.uproject` 即可编译运行。

## 垂直切片进度

按顺序推进，禁止跳步横铺：

| 切片 | 内容 | 状态 |
|------|------|------|
| VS0 | 大厅壳（登录 UI + UIManager + 大厅地图） | 进行中 / 已完成壳层 |
| VS1 | GAS 战斗 | 技术方案已落地，实现中 |
| VS2 | AI 小兵 | 待做 |
| VS3 | 背包制造 | 待做 |
| VS4 | 任务 Agent | 待做 |
| VS5 | Boss | 待做 |
| VS6 | 商业化互通 | 待做 |
| VS7 | 存盘 | 待做 |

## 仓库结构

```
AgentDemo/
├── AGENTS.md                 # Agent / Vibe Coding 开发入口
├── Source/AgentDemo/         # 玩法代码（按域分目录）
├── Plugins/                  # 含 GameplayCameraSystemToolSet 等
├── Content/                  # 关卡、UI、动画等资产
├── Config/
└── Docs/
    ├── 策划案/               # 总纲、分册、统一 GDD
    ├── 技术方案/             # VS 执行方案、资产迁移清单
    └── 开发规范/             # Vibe Coding 工作流
```

## 文档入口

开工前建议按顺序阅读：

1. [面试复习 Demo 总纲](Docs/策划案/00_面试复习Demo总纲.md)
2. [统一项目 GDD](Docs/策划案/06_统一项目GDD.md)
3. 当前模块分册 `Docs/策划案/01`～`05`
4. [Vibe Coding 工作流](Docs/开发规范/VibeCoding工作流.md)
5. 协作约定：[AGENTS.md](AGENTS.md)

## 主循环（系统挂点）

```
大厅（公会/商城/战令）
  → 进入小镇
    → 治安官对话（QuestAgent 生成任务）
      → 野外战斗（GAS + 小兵 AI）
      → 搜刮 → 背包 → 制造 → 装备
      → 交任务拿奖 → AI 发下一环
    → 解锁矿洞 → 阶段 Boss → 结算回大厅
```

## 开发约定（摘要）

- **Server 权威**：伤害、背包、制造、接交任务、购买
- 跨系统只走 GDD 约定接口，禁止模块直掏私有数据
- 内容 Id 须进 Registry 白名单后再引用
- 一次会话只推进一个 VS 或一个子验收项
- 不要提交密钥或本地绝对路径配置

详细规则见 `AGENTS.md` 与 `.cursor/rules/`。

## 克隆

```bash
git clone https://github.com/ZhanYangFan/AgentDemo.git
```

仓库体积较大（含 Content 资产），克隆可能需要一些时间。
