# AgentDemo 统一项目总纲

> 目标：把简历里的 GAS / AI / 背包制造 / AI 任务 / 商业化客户端 **串进同一个可玩项目**，用一条主循环把系统焊死，方便复习与面试演示。  
> 工程：`D:\UEProject\AgentDemo`。开发协作：`AGENTS.md`。

---

## 一、产品一句话

**西部小镇生存委托**：你在 Frontier 小镇接 AI 治安官派发的委托，靠战斗、搜刮与制造武装自己，清剿威胁，最终挑战矿洞 Boss；大厅侧有公会与商城战令，服务「成长与社交展示」。

---

## 二、简历能力 → 项目模块（不再拆成多个工程）

| 简历点 | 项目内模块名 | 文档 |
|--------|-------------|------|
| Aura / ProjectF 战斗 | **Combat（GAS）** | [01](./01_GAS战斗与技能框架.md) |
| 落叶归根 AI | **AI / BossEncounter** | [02](./02_AI行为树与Boss阶段.md) |
| ProjectF 背包制造 | **Inventory / Craft / Persist** | [03](./03_背包装备与制造.md) |
| ProjectF AI 任务 | **QuestAgent（主循环发动机）** | [04](./04_AI生成任务循环.md) |
| 创造吧 公会商业化 | **FrontEnd / Commerce** | [05](./05_轻量公会与商业化客户端.md) |

详细串联见 → **[统一项目GDD](./06_统一项目GDD.md)**

---

## 三、一条主循环（所有系统挂在这上面）

```
大厅（公会/商城/战令） 
    → 进入小镇 
        → 与治安官对话（QuestAgent 生成任务）
            → 野外战斗（GAS + 小兵 AI）
            → 搜刮材料 → 背包 → 铁匠制造 → 装备变强
            → 交任务拿奖（物品/战令经验/货币）
            → AI 根据上下文发下一环
        → 第 3 环解锁矿洞 
            → 阶段 Boss 战（AI 阶段 + GAS 受击）
            → 通关结算 → 回大厅展示/购物
```

**面试一句话**：  
「主循环是 AI 任务驱动；战斗和制造是完成手段；商业化是成长与变现层；Boss 是内容高潮。」

---

## 四、工程结构（单 UE 工程）

宿主：**`D:\UEProject\AgentDemo`**（模块名 `AgentDemo`）。  
协作方式见仓库根目录 `AGENTS.md` 与 `Docs/开发规范/VibeCoding工作流.md`。

```
AgentDemo/
  AGENTS.md                       // Vibe Coding 入口
  .cursor/rules/                  // Cursor Agent 规则
  Source/AgentDemo/
    AbilitySystem/                // ← 01
    AI/                           // ← 02
    Inventory/ Equipment/ Craft/  // ← 03
    Quest/ Agent/ Dialogue/       // ← 04
    Commerce/ Guild/ SDK/         // ← 05
    Persist/
    Variant_*                     // 模板参考，主线勿堆这里
  Content/Maps/ ...
  Docs/策划案/ ...
  Docs/开发规范/ ...
```

网络：Listen Server 足够；Gameplay 权威在 Server；大厅 Mock 协议也跑在「假 Server」对象上，保持话术一致。

---

## 五、实现顺序（按依赖，不是按多个 Demo）

| 周 | 交付切片 | 完成标志 |
|----|----------|----------|
| 1 | Combat 竖切 | 能开枪、扣血、受击 GC |
| 2 | AI 小兵 + 简易 Boss 房 | 两种行为可见；Boss 两阶段 |
| 3 | 背包 / 装备 / 制造 / 存盘 | 重进读盘成功 |
| 4 | QuestAgent 三环主线 | Mock 模式打通到解锁矿洞 |
| 5 | 大厅公会 + 商城 + 战令 | 交任务加 BP 经验；货币互通 |
| 6 | 打磨联调 + 录屏口条 | 一条 8～12 分钟演示带 |

任何一周结束，项目都应 **能启动、能玩当前已接通的那一段**（垂直切片，而不是横向堆半成品）。

---

## 六、统一技术约束

| 项 | 约定 |
|----|------|
| 引擎 | UE 5.8（本工程） |
| 语言 | C++ 逻辑 + BP 表现/配置 |
| 数据 | DataAsset / DataTable；任务输出为 JSON→UStruct |
| 权威 | 伤害、背包、制造、接交任务、购买：仅 Server |
| 降级 | LLM 失败 → 模板任务；支付/登录 → SDK Mock |
| 美术 | 引擎资产即可，重点在系统闭环 |

---

## 七、面试演示脚本（单项目一条片）

1. 大厅点开公会/商城（讲协议与缓存）→ 进游戏  
2. 找治安官接第 1 环（讲 Agent 结构化输出 + Validator）  
3. 野外击杀（讲 GAS Context / 同步）  
4. 铁匠制造并换装（讲 Instance + DS 存盘）  
5. 交任务 → 自动第 2、3 环 → 进矿洞打阶段 Boss（讲行为树阶段）  
6. 回大厅战令领奖（讲动态数据与发货）

---

## 八、文档索引

- 开发协作：[AGENTS.md](../../AGENTS.md) / [VibeCoding工作流](../开发规范/VibeCoding工作流.md)
- 0. [06_统一项目GDD](./06_统一项目GDD.md) ← **玩法先读这个**
- 1. [01_GAS战斗与技能框架](./01_GAS战斗与技能框架.md)
- 2. [02_AI行为树与Boss阶段](./02_AI行为树与Boss阶段.md)
- 3. [03_背包装备与制造](./03_背包装备与制造.md)
- 4. [04_AI生成任务循环](./04_AI生成任务循环.md)
- 5. [05_轻量公会与商业化客户端](./05_轻量公会与商业化客户端.md)
