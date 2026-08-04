# 模块 01｜GAS 战斗与技能框架

> 对应简历：RPGDemo（Aura）+ ProjectF 开火/受击 GA  
> **统一项目中的角色**：全项目的伤害/技能/受击底座；被 AI、任务击杀统计、装备 AbilitySet 共同依赖。  
> 总览见 [00总纲](./00_面试复习Demo总纲.md) / [06统一GDD](./06_统一项目GDD.md)

---

## 0. 在统一项目里怎么接

| 对接方 | 约定 |
|--------|------|
| AI (02) | 敌人攻击只 `TryActivateAbilityByTag`，不直接改血 |
| 装备 (03) | 换武器 = 换 AbilitySet；防具 = Infinite GE |
| 任务 (04) | 死亡时抛 `OnActorKilled(VictimRegistryId)` |
| 商业化 (05) | 不直接依赖；通关奖励可发消耗品 GE |
| 地图 | 主要在 `M_Town` / `M_MineBoss` 使用，不做独立胜负结算 |

本模块不再追求「单独清波次胜利」；胜负与节奏交给任务环与 Boss 通关。

---

## 1. 一句话定位

第三人称射击/近战混合战斗层：玩家用 2～3 个技能打怪，怪物有受击反馈；底层全走 GAS，并在 Listen Server 下验证同步。

---

## 2. 玩法目标（玩家侧）

| 目标 | 说明 |
|------|------|
| 在项目中的用途 | 完成击杀类任务、矿洞 Boss 战、野外生存 |
| 失败 | 玩家 HP ≤ 0 → 小镇复活点（扣少量 Gold，任务进度保留） |
| 不再单独结算 | 无「清波次胜利」；击杀只推任务/掉落 |

---

## 3. 功能范围

### 必做（Must）

1. **ASC 挂载**：PlayerState 上挂 ASC（Lyra/Aura 常见做法），Character 持有 AttributeSet 引用。
2. **AttributeSet**：Health / MaxHealth / Mana / MaxMana / AttackPower / Armor（至少这 6 个）。
3. **自定义 GameplayEffectContext**：携带命中骨骼名、命中方向、是否暴击；实现 NetSerialize。
4. **三类 GameplayAbility**：
   - 射线类（开火 / 点射）
   - 实体类（投掷物 / 近战判定盒）
   - 召唤类（召唤 1 只友方或敌方小怪，生命周期有限）
5. **Buff 系统**：
   - 用 GE + GameplayTag 表达 Buff（灼烧、减速、护盾）
   - 自定义 `BuffComponent` 挂在 Actor 上
   - `BuffManager`（Subsystem 或 Component）统一增删查
6. **修饰符路径演示**（面试必讲）：
   - 默认 AttributeBased 修改
   - 自定义 `GameplayEffectExecutionCalculation`
   - SetByCaller 注入伤害数值
7. **受击表现**：受击 GA + GameplayCue，按部位播不同特效/音效；近战砍击有血迹方向。
8. **武器切换**：至少 2 把武器，切换时换 AbilitySet / Input。

### 可做（Should）

- 技能 CD UI（可用动态材质圆环，呼应「落叶归根」材质 CD）
- 简单锁敌 / 准星扩散（呼应 Lyra WeaponInstance spread 概念即可，不必完整）

### 不做（Won’t）

- 完整射击手感打磨、复杂动画状态机、完整 AI（AI 放到 Demo02）

---

## 4. 核心玩法流程

```
Spawn Player
  → Give AbilitySet（开火 / 技能A / 技能B / 受击）
  → 输入触发 GA
      → Cost GE（扣蓝/弹药）
      → Cooldown GE
      → 命中判定（LineTrace / Projectile / Sweep）
      → 组装 EffectContext（骨骼、方向、暴击）
      → Apply GE（伤害 / Buff）
      → 目标 ASC 触发受击 Tag → 激活受击 GA
      → GC 播特效
  → HP≤0 → Death GE / Tag → 禁用输入、播死亡
```

---

## 5. 数据设计

### 5.1 DataAsset：`DA_AbilitySet_Player`

| 字段 | 类型 | 说明 |
|------|------|------|
| StartupAbilities | TArray\<TSubclassOf\<UGameplayAbility\>\> | 开局授予 |
| StartupEffects | TArray\<TSubclassOf\<UGameplayEffect\>\> | 开局被动 |
| Tags | FGameplayTagContainer | 角色身份标签 |

### 5.2 DataTable：`DT_WeaponDefinition`

| 列 | 类型 | 示例 |
|----|------|------|
| WeaponId | Name | Rifle / Machete |
| Damage | float | 25 / 40 |
| FireGA | SoftClassPath | GA_Fire_Rifle |
| EquipMontage | SoftObjectPath | … |
| AmmoAttr | GameplayTag | Attr.Ammo |

### 5.3 Buff 配置：`DT_BuffDefinition`

| 列 | 说明 |
|----|------|
| BuffTag | 唯一 Tag，如 `Buff.Burn` |
| StackPolicy | 可叠加 / 刷新 / 互斥 |
| Duration | 秒 |
| Period | 跳伤间隔 |
| GEClass | 对应 GameplayEffect |

---

## 6. 类与模块建议

```
/Source/DemoGAS/
  AbilitySystem/
    DemoAbilitySystemComponent.h
    DemoAttributeSet.h
    DemoGameplayEffectContext.h      // 自定义 Context + NetSerialize
    DemoAbilitySystemLibrary.h       // 组装 Context、查 Tag 工具
  Abilities/
    GA_Fire_Projectile.h             // 实体
    GA_Fire_Hitscan.h                // 射线
    GA_Summon_Minion.h               // 召唤
    GA_HitReact.h                    // 受击
  Executions/
    DemoDamageExecCalc.h             // 自定义伤害公式
  Buff/
    BuffComponent.h
    BuffManagerSubsystem.h
  Equipment/
    WeaponActor.h
    WeaponManagerComponent.h
  UI/
    HUD_AttributeBinder.h            // 绑定 Health/Mana/CD
```

---

## 7. 网络同步要点（必须能讲清）

| 对象 | 同步方式 | 你要能说的一句话 |
|------|----------|------------------|
| GA | `InstancedPerActor` / 部分 `InstancePerActor` + Replication Policy | 技能实例按策略复制，预测用 Prediction Key |
| AttributeSet | `ReplicatedUsing` + `GetLifetimeReplicatedProps` | 属性走属性复制，改值尽量在 Server |
| GE | 依赖 ASC 的 ActiveGameplayEffects 复制 | 持续 GE 复制到模拟端做表现；瞬时伤害以 Server Apply 为准 |
| EffectContext | 自定义字段进 NetSerialize | 命中部位等信息随 GE 一起到客户端 |
| GameplayCue | 本地/网络触发策略 | 表现用 GC，逻辑不要放 GC |

面试加分句：**「逻辑权威在 Server，Client 只做预测与表现；Buff 的加删以 Server Tag/GE 为准。」**

---

## 8. UI 需求（最小）

1. 血条 / 蓝条（属性绑定）
2. 技能栏 3 格 + CD 遮罩（材质或 ProgressBar）
3. 受击时屏幕微红（可选）
4. 武器名 / 弹药数

---

## 9. 关卡与内容量（归入统一地图）

- 敌人 Id 与 02/06 Registry 对齐：`Enemy_Coyote`（近战）、`Enemy_Bandit`（远程）、Boss 另见 02
- 武器：步枪（Hitscan）+ 砍刀（近战盒），可由 03 制造/装配获得
- 技能：手雷（Projectile）+ 召唤诱饵（Summon）
- 开发期可在 `M_Town` 旁放射击靶场；正式体验不进独立竞技场图

---

## 10. 里程碑

| 阶段 | 交付 | 天数（参考） |
|------|------|-------------|
| M1 | ASC + AttributeSet + 改血生效 | 1 |
| M2 | Hitscan 开火 + 伤害 GE + SetByCaller | 1～2 |
| M3 | 自定义 EffectContext + 受击 GA/GC | 1～2 |
| M4 | BuffComponent + 灼烧/减速 | 1 |
| M5 | ExecutionCalculation 伤害公式 | 1 |
| M6 | 武器切换 AbilitySet + 简易 UI | 1～2 |
| M7 | Listen Server 双端验证同步 | 1 |

---

## 11. 验收清单

- [ ] 单机可击杀怪物，HP 归零有死亡表现
- [ ] 三种 GA（射线 / 实体 / 召唤）各至少 1 个可用
- [ ] 自定义 EffectContext 能传到客户端并驱动部位特效
- [ ] Buff 可叠加/刷新，UI 或日志能看到 Tag
- [ ] ExecCalc / AttributeBased / SetByCaller 三种改属性路径都有示例
- [ ] Listen Server 下，客户端能看到伤害与 Buff 表现
- [ ] 能按「面试口述模板」完整讲 5 分钟不卡壳

---

## 12. 面试高频追问（提前备答）

1. ASC 为什么挂 PlayerState 而不是 Character？死亡/换 pawn 时技能怎么办？  
2. GE 的 Duration / Infinite / Instant 分别适合什么？  
3. Prediction 失败怎么回滚？你 Demo 里哪些做了预测？  
4. Execution 和 Modifier 的区别？为什么伤害常用 ExecCalc？  
5. GameplayCue 和直接播特效的边界？
