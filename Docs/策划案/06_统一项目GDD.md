# 06｜AgentDemo 统一项目 GDD

> 宿主工程：`D:\UEProject\AgentDemo`。  
> 本文是「把 01～05 串进同一个项目」的总设计。各系统细则仍以 01～05 为准；冲突时以本文的 **主循环与接口约定** 为准。  
> 怎么用 AI 开发：见 `AGENTS.md`、`Docs/开发规范/VibeCoding工作流.md`。

---

## 1. 高概念

| 项 | 内容 |
|----|------|
| 名称 | AIAgent / Frontier Trust（可暂名） |
| 类型 | 第三人称动作 + 轻 RPG + AI 驱动委托 |
| 视角 | 第三人称 |
| 单次完整体验 | 30～45 分钟（含大厅）；核心三环任务约 15～25 分钟 |
| 网络 | Listen Server（1 人自测为主，预留 2 人进同一小镇） |
| 成功标准 | 一条录屏讲完简历上所有主技术点，系统间有真实数据往来 |

---

## 2. 世界观（够用就好，服务内容挂点）

Frontier 是西部边境小镇。郊狼与强盗侵扰外围，矿洞深处出现「异化首领」。  
治安官不直接给死任务表，而是让 **委托 Agent** 根据近期灾情与你的表现，生成委托。  
铁匠可制造装备；镇外有公会驻节与军需商城（大厅表现）。

用途：给 NPC、敌人 Id、Boss、商品命名一个统一皮，避免五个系统各说各话。

---

## 3. 地图与进度门闸

| 地图 | 内容 | 解锁条件 |
|------|------|----------|
| `M_Frontend` | 登录 Mock、公会、商城、战令、进入游戏 | 默认 |
| `M_Town` | 小镇 NPC、铁匠、野外刷新、任务目标点 | 从大厅进入 |
| `M_MineBoss` | 阶段 Boss 房 | 完成主线第 3 环任务后，治安官开启矿洞 |

进度旗标（存盘）：

```
Flags.TutorialDone
Flags.Ring1Cleared / Ring2Cleared / Ring3Cleared
Flags.MineUnlocked
Flags.BossDefeated
BattlePass.XP / PremiumUnlocked
Guild.Id（可空）
```

---

## 4. 主循环状态机

```
[Frontend]
   EnterWorld
      ↓
[Town_Explore] ←→ [Dialogue_Agent] → AcceptQuest
      ↓                                      ↓
[Quest_Active] ←———— progress events ———— [Combat/AI]
      ↓
 ReadyToTurnIn → TurnIn → GrantRewards
      ↓
  ringIndex++ →（若 ring==3 完成）Set MineUnlocked
      ↓
[Mine_Boss] → BossDefeated → Victory → 回 Frontend 领战令
```

**QuestAgent 是节奏发动机**：没有任务时引导对话；有任务时 HUD 追踪压制其他杂念。

---

## 5. 系统依赖图（编译与实现顺序）

```
                    ┌──────── Persist(SaveGame) ────────┐
                    ↓                                   │
 Combat(GAS) ←——→ Inventory/Equipment/Craft  ←———————─┤
     ↑                      ↑                           │
     │                      │                           │
    AI/Boss          QuestAgent / Rewards ──────────────┤
     ↑                      ↑                           │
     └──── WorldHooks ──────┘                           │
                                                        │
 Frontend Commerce/Guild  ←── Currency / BP_XP ─────────┘
```

依赖规则：

1. **Combat** 不依赖任务；任务依赖 Combat 的死亡事件。  
2. **Inventory** 不依赖任务；任务发奖、Collect 目标依赖 Inventory。  
3. **AI** 依赖 Combat 的伤害/受击；Boss 关卡可独立进，但正式流程由任务解锁。  
4. **Commerce** 可读 Inventory 货币；购买产出走同一发奖管道。  
5. **Persist** 序列化：属性摘要可选、背包、任务、旗标、战令、公会 Id。

---

## 6. 跨系统接口约定（焊死用）

> 实现时先定这些接口，避免五个文档各写各的。

### 6.1 战斗 → 任务

```
IQuestWorldSink::OnActorKilled(Killer, Victim, VictimIdTag)
```

- 怪物 DataAsset 带 `IdentityTag` / `RegistryId`（如 `Enemy_Coyote`）  
- 死亡时 Server 广播给 `QuestManager`

### 6.2 背包 → 任务

```
IQuestWorldSink::OnInventoryChanged(Player, ItemId, NewCount)
```

- Collect 目标每次变更时复核  

### 6.3 任务 → 发奖

```
IRewardService::Grant(Player, TArray<FRewardSpec>)
```

- 统一入口：任务提交、Boss 首通、商城购买发货、战令领取  
- 内部再分发：Item / Currency / BattlePassXP / UnlockFlag  

### 6.4 装备 → 战斗

```
EquipmentManager::OnSlotChanged → WeaponManager / ASC GiveAbilitySet
                                → Apply Infinite GE（护甲等）
```

### 6.5 AI → 战斗

```
BTTask_ActivateAbilityByTag → ASC.TryActivateAbilitiesByTag
```

### 6.6 大厅 ↔ 玩法

| 数据 | 方向 | 说明 |
|------|------|------|
| Gold / Diamond | 双向 | 玩法掉落与任务加 Gold；商城消耗 |
| BattlePass XP | 玩法→大厅 | 交任务、击杀 Boss 加 |
| 外观商品 | 大厅→玩法 | Demo 可只改角色色或挂件 Tag |

---

## 7. 内容编排（把 01～05 的内容塞进同一体验）

### 7.1 默认三环主线（MockAgent 剧本，也可被 LLM 替换）

| 环 | 任务 | 逼出的系统 |
|----|------|------------|
| 1 | 击杀 3×`Enemy_Coyote` | GAS 开火、小兵 AI、任务追踪 |
| 2 | 收集 2×`Item_Herb` 并 TalkTo `NPC_Vera`；奖励制造配方+材料 | 背包 Collect、对话、制造解锁 |
| 3 | Reach `Zone_MineEntrance` 并击杀 2×`Enemy_Bandit`；奖励解锁矿洞 | Reach、进 Boss 图门闸 |

### 7.2 铁匠支线（始终可用）

- 有配方后制造步枪/皮甲 → 装备 → 属性变化可在环 3 / Boss 感知到  

### 7.3 矿洞 Boss（02 全文内容）

- 两阶段 Boss + 近战小兵 / 远程小兵各一波  
- 首通奖励：钻石（假）、战令大额 XP、纪念外观  

### 7.4 大厅（05）

- 进游戏前可浏览；Boss 后再回来领战令更有「商业化闭环」感  

---

## 8. 角色与生成物清单（统一 Registry）

所有 AI 输出、掉落、制造、商城商品 **只能引用下表 Id**（扩展内容先加表再引用）。

| RegistryId | 类型 | 出现位置 |
|------------|------|----------|
| NPC_Carter | NPC | 治安官 / Agent 入口 |
| NPC_Vera | NPC | 酒馆 / TalkTo |
| NPC_Smith | NPC | 铁匠 / 制造台 |
| Enemy_Coyote | AI | 镇外 |
| Enemy_Bandit | AI | 矿洞入口 |
| Enemy_Boss_Hollow | AI | 矿洞 Boss |
| Item_Herb | 道具 | 采集点 |
| Ore_Iron / Hide_Wolf | 材料 | 掉落 |
| Weapon_Rifle_T1 / Armor_Leather_T1 | 装备 | 制造/商城 |
| Recipe_Rifle_T1 | 配方 | 环 2 奖励 |
| Zone_WaterTower / Zone_MineEntrance | 区域 | Reach |
| Curr_Gold / Curr_Diamond | 货币 | 全局 |

---

## 9. UI 信息架构（单套 HUD，分模式）

```
Frontend UI（CommonUI 图层可选）
  Lobby / Guild / Shop / BattlePass

InGame HUD
  属性条、技能 CD（材质）、武器弹药
  任务追踪（左上）
  交互提示

Modal
  背包 / 装备 / 制造
  对话
  结算 / Boss 血条（进矿洞时）
```

输入：`I` 背包，`C` 制造（近铁匠自动开也可），`Tab` 地图/任务日志，`Esc` 回大厅确认。

---

## 10. 存盘范围（一份 Save 打通）

```
SaveSlot_Player
├─ Inventory + EquipmentInstances + Slots
├─ Currencies
├─ Quest: ringIndex, activeSpec, progress, journal[]
├─ Flags（矿洞解锁等）
├─ BattlePass state
├─ GuildId / local guild mock state
└─ UnlockedRecipes
```

读盘门闩：`bProgressionReady` 前，拒绝：拾取、制造、接任务、购买。

---

## 11. 模块目录与所有权（避免文件打架）

| 目录 | 所有者文档 | 禁止事项 |
|------|------------|----------|
| AbilitySystem/ | 01 | 不要在这里写任务进度 |
| AI/ | 02 | 技能执行必须走 GA，不直接扣血（临时 stub 除外） |
| Inventory/ | 03 | UI 不直接改数量 |
| Quest/ Agent/ | 04 | 不直接 Spawn 装备，走 RewardService |
| Commerce/ | 05 | 不另起货币系统，用 CurrencyComponent |
| Persist/ | 总控 | 只序列化，不掺业务规则 |

---

## 12. 垂直切片里程碑（取代「五个独立 Demo」）

| Slice | 玩家可感知结果 | 接通模块 |
|-------|----------------|----------|
| VS0 | 进大厅点「进入游戏」进小镇 | 05 壳 + 地图 |
| VS1 | 能开枪打死一只郊狼 | 01 |
| VS2 | 郊狼会追击/攻击，远程强盗会风筝 | 02 小兵 |
| VS3 | 拾取→背包→制造→装备变强 | 03 |
| VS4 | 治安官给任务→完成→领奖→下一环 | 04 |
| VS5 | 三环后进矿洞打两阶段 Boss | 02 Boss + 门闸 |
| VS6 | 货币/战令/公会与玩法互通 | 05 全量 |
| VS7 | 杀进程重进进度仍在 | Persist |

---

## 13. 整包验收清单

- [ ] 仅启动本工程，可从大厅玩到 Boss 通关再回大厅领战令  
- [ ] 任务发奖、商城购买、战令领取都走 `IRewardService`  
- [ ] 怪物死亡与背包变化能推动任务进度  
- [ ] 装备切换改变 GAS 战斗表现  
- [ ] Boss 阶段切换正常，且只有任务解锁后可进  
- [ ] Save/Load 后环数、背包、解锁旗标一致  
- [ ] MockAgent 可全离线通关；HttpAgent 可开关  
- [ ] 8～12 分钟演示脚本可完整走完  

---

## 14. 各分册在本项目中的读法

- **01～05**：仍然是各系统的详细规格（规则、类、验收、面试题）  
- **本文**：管边界、顺序、Id、主循环、接口  
- 分册中的「单独 Demo / 独立关卡」一律理解为 **本项目中的子关卡或子 UI**，不再新建第二工程  

若某分册示例与本文冲突（例如单独的竞技场胜负条件），以 **任务主线 + Boss 通关** 为准。
