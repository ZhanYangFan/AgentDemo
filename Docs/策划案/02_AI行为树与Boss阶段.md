# 模块 02｜AI 行为树与 Boss 阶段

> 对应简历：落叶归根（阶段 Boss + 小兵 AI、UMG）  
> **统一项目中的角色**：小镇野外敌群 AI + 主线高潮 `M_MineBoss`；技能执行依赖 01，解锁依赖 04 第 3 环。  
> 总览见 [00总纲](./00_面试复习Demo总纲.md) / [06统一GDD](./06_统一项目GDD.md)

---

## 0. 在统一项目里怎么接

| 对接方 | 约定 |
|--------|------|
| Combat (01) | BTTask 只激活 GA；受击硬直听 HitReact Tag |
| 任务 (04) | Coyote/Bandit 死亡推任务；Boss 首通走 RewardService |
| 进度 | `Flags.MineUnlocked` 后治安官才允许进矿洞 |
| UI | Boss 血条仅在 `M_MineBoss`；小镇 HUD 用通用敌方指示即可 |
| CD 材质 | 复用玩家技能栏，呼应简历 UMG/材质 CD |

---

## 1. 一句话定位

野外两套个性小兵 + 矿洞 **两阶段 Boss**；用行为树做决策，用 GAS 做执行。

---

## 2. 玩法目标

| 项 | 内容 |
|----|------|
| 野外 | `Enemy_Coyote` / `Enemy_Bandit` 服务任务与掉落 |
| Boss | `M_MineBoss`：入场 → 两阶段交战 → 击杀 → 回小镇/大厅 |
| 时长 | Boss 战 4～8 分钟 |
| 失败 | 玩家死亡回小镇复活点；Boss 进度可重置本场 |

---

## 3. AI 角色规格

### 3.1 小兵 A：近战冲锋型

| 行为 | 细节 |
|------|------|
| 感知 | 视觉锥 + 听力（受击噪音） |
| 巡逻 | 路径点巡回 |
| 追击 | MoveTo 玩家，丢失目标回巡逻 |
| 攻击 | 进入近战范围播攻击 Montage，造成伤害 |
| 技能 | 每隔 N 秒一次「冲锋」（短暂加速直线冲向玩家） |
| 个性 | 血量低时更激进（缩短攻击间隔） |

### 3.2 小兵 B：远程风筝型

| 行为 | 细节 |
|------|------|
| 理想距离 | 保持 800～1200uu |
| 过近 | 后撤 / 侧移 |
| 过远 | 靠近到射程 |
| 攻击 | 定点射击，需停稳 |
| 技能 | 丢一颗减速雷（走 Demo01 的 Projectile GA 更佳） |
| 个性 | 被近身后优先逃跑，不硬刚 |

### 3.3 Boss：两阶段（简历「阶段式 Boss」）

**阶段 1（100%～50% HP）**

- 技能池：劈砍、扇形冲击波、召唤 2 只小兵 A
- 行为权重：近战为主，偶尔冲击波清近身
- 护盾：无

**阶段切换（50% HP）**

- 进入 `State.Boss.PhaseTransition`（无敌 + 演出 3 秒）
- 清场：可选击退玩家、击杀场上小兵
- 切换行为树子树或切换 Blackboard「Phase=2」

**阶段 2（50%～0%）**

- 技能池：阶段 1 技能 + 全屏点名砸地 + 召唤小兵 B
- 攻击频率提升 30%
- 可释放「狂暴」：移速/攻速 Buff（走 GAS Tag 更佳）

---

## 4. 行为树结构设计

### 4.1 通用小兵树（示意）

```
Root Selector
├─ [IsDead] → Death Task
├─ [HasHitReact] → PlayHitReact
├─ Combat Service（更新距离、是否可见）
│   └─ Selector
│       ├─ [CanUseSkill] → UseSkill → Cooldown BB
│       ├─ [InAttackRange] → Attack
│       └─ MoveTo / KeepDistance（近战与远程用不同 Task）
└─ Patrol（MoveTo 路径点 → Wait）
```

### 4.2 Boss 树（示意）

```
Root Selector
├─ [PhaseTransition] → PlayMontage + Wait + SetPhase
├─ [Phase==2] → Phase2 Subtree
│     Selector: Smash / SummonB / Wave / Melee
└─ [Phase==1] → Phase1 Subtree
      Selector: SummonA / Wave / Melee
```

**关键设计点（面试要讲）：**

- 阶段用 Blackboard `Enum Phase` 或 GameplayTag，**不要复制两整棵无关的树硬切换**（可切换 Subtree Asset）。
- 技能选择用 **Utility / 加权随机 + CD**，避免固定循环被玩家摸清后无挑战，也避免纯随机太难讲。
- 自定义 `BTTask`：`BTTask_ActivateAbilityByTag`、`BTTask_FaceTarget`、`BTTask_SpawnMinions`。
- 自定义 `BTService`：`BTS_UpdateCombatFocus`（刷新目标、距离、LOS）。

---

## 5. 与 GAS 的衔接（推荐）

| AI 行为 | 实现 |
|---------|------|
| 攻击/技能 | AI Controller 发 Tag → 角色 ASC `TryActivateAbilityByTag` |
| 受击硬直 | 收到 `Event.HitReact` → 短时禁用移动 EQS/行为 |
| 阶段无敌 | GE_Grant `State.Invulnerable` + 取消伤害 |
| Boss 狂暴 | GE_Buff 改 AttackSpeed / MoveSpeed |

这样面试可以说：**「AI 决策在行为树，执行在 GAS，表现在 Montage/GC。」**

---

## 6. 感知与寻路

| 模块 | 要求 |
|------|------|
| AIPerception | Sight + Hearing；团队态度 Enemy |
| NavMesh | Boss 房完整烘焙；冲锋技能可用 `LaunchCharacter` 或临时关闭避障 |
| EQS（可选） | 远程小兵找「与玩家保持理想距离的点」 |

---

## 7. UI（呼应简历 UMG）

### 必做

1. **Boss 血条**：顶部大血条，阶段切换时颜色/样式变化  
2. **阶段提示**：Phase 2 出现「狂暴」字幕  
3. **小地图或敌方指示器**（可选，一个箭头即可）

### 简历彩蛋（加分）

- 技能 CD：动态材质圆环（Progress 写入 Scalar Parameter）
- 人物面板：简单属性条，用材质做「流动感」边框（不需写实到原作水平）

### UI 绑定约定

- 控件逻辑：`Widget` 订阅 ASC Attribute / Tag 变化，禁止 Tick 轮询血量。
- Boss 血量：监听 Boss 的 AttributeSet，或 GameState 上的 Boss 引用。

---

## 8. 关卡脚本 / 导演

用 `ABossEncounterVolume` 或 Level Blueprint（最终应收进 C++/BP 类）：

1. 玩家进入触发体 → 关门 / 播 banter  
2. Spawn Boss  
3. HP 过半 → 广播 PhaseChange（给 UI）  
4. Boss 死亡 → 开门 + 结算 UI  

---

## 9. 数据配置

`DA_AICharacter_Melee` / `Ranged` / `Boss`：

| 字段 | 说明 |
|------|------|
| BehaviorTree | 树资产 |
| Blackboard | BB 资产 |
| AbilitySet | 可释放的 GA 列表 |
| SightRadius / LoseRadius | 感知 |
| IdealCombatRange | 远程用 |
| PhaseHPThreshold | Boss 用，默认 0.5 |

---

## 10. 里程碑

| 阶段 | 交付 |
|------|------|
| M1 | 小兵 A 巡逻-追击-攻击闭环 |
| M2 | 小兵 B 保持距离 + 射击 |
| M3 | Boss 阶段 1 技能循环 |
| M4 | 50% 切阶段演出 + 阶段 2 |
| M5 | Boss UI + CD 材质 |
| M6 | 与 Demo01 伤害/受击打通 |

---

## 11. 验收清单

- [ ] 两种小兵行为明显不同（录屏对比 30 秒能看出来）
- [ ] Boss 两阶段技能池不同，切换有表现与无敌窗
- [ ] 技能通过行为树 Task 触发，而不是 Character Tick 里 if-else 堆技能
- [ ] 有自定义 BTTask 或 BTService 至少 2 个
- [ ] Boss 血条与阶段 UI 正确
- [ ] （加分）CD 用动态材质显示
- [ ] 能讲清：Blackboard 关键字段、阶段切换时序、感知丢失后的回退逻辑

---

## 12. 面试高频追问

1. 行为树 vs 状态机，你什么时候用哪个？  
2. 服务（Service）和装饰器（Decorator）的职责边界？  
3. 多 AI 同屏如何做性能（感知频率、EQS 节流）？  
4. Boss 阶段切换如何避免「同一帧被打死跳过演出」？  
5. AI 放技能和玩家放技能如何共用同一套 GA？
