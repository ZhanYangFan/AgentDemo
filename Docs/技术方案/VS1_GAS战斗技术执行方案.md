# VS1 战斗（GAS）技术执行方案

> 依据：`Docs/策划案/01_GAS战斗与技能框架.md`（模块规格）、`06_统一项目GDD.md` §5/§6/§8/§11/§12。
> VS1 验收（GDD §12）：**能开枪打死一只郊狼**。本方案覆盖 01 分册完整目标，按里程碑小步交付。
> 分层原则：**基础系统层** = 可复用底座（被 AI/任务/装备依赖）；**上层业务层** = VS1 具体内容（武器/技能/郊狼/HUD/动画内容）。

---

## 0. 目标与验收口径

| 层 | 目标 | 验收 |
|----|------|------|
| 基础系统 | GAS 底座 + 动画表现底座（状态机/分层/混合/事件桥） | 01 §11 清单逐条可演示 |
| 上层业务 | 步枪+砍刀+手雷+召唤诱饵打郊狼 | GDD §12 VS1：开枪打死一只郊狼 |
| 面试口径 | 01 §12 追问 + 动画分层/混合/驱动题能讲 5 分钟 | 每条追问在代码里有对应落点 |

## 1. 范围与边界

- **Must（01 §3）**：ASC 挂 PlayerState；6 属性 AttributeSet；自定义 EffectContext+NetSerialize；三类 GA（射线/实体/召唤）；Buff 系统（BuffComponent+BuffManager）；三种修饰符路径；受击 GA+GC 部位表现；双武器切换 AbilitySet
- **动画 Must（本方案补充，对应 01 "受击表现/核心流程"）**：攻击判定由动画时刻驱动（GameplayEvent），非按键即判定；上半身动作层与移动混合；瞄准偏移；部位受击动画
- **Should**：技能 CD UI、简易锁敌/准星扩散、换弹
- **Won't**：射击手感打磨、复杂动画状态机（指全帧率打磨级的 Locomotion 重构/Motion Matching）、完整 AI（VS2）、Root Motion 位移攻击、IK
- **GDD 硬边界**：死亡抛 `OnActorKilled`（§6.1，VS1 只做事件源）；敌人 Id 只用 Registry 白名单；AbilitySystem/ 不写任务进度

---

## 2. 分层架构总览

```
┌─ 上层业务层（VS1 内容，可被替换/扩展） ─────────────────────┐
│  GA_Fire_Hitscan / GA_Fire_Projectile / GA_Melee_Sweep    │  AbilitySystem/Abilities/
│  GA_Summon_Minion / GA_HitReact                            │
│  WeaponActor + WeaponManagerComponent + DT_Weapon          │  Equipment/
│  AgentEnemyCharacter（郊狼靶子）                            │  AI/（VS2 接管行为）
│  WBP_HUD_InGame                                            │  UI 资产
│  攻击/受击/死亡 Montage、AimOffset、武器动画集               │  Content/Animation/
└─────────────────────────────────────────────────────────────┘
┌─ 基础系统层（底座，01 规格 + 动画底座，VS2+ 复用） ─────────┐
│  【GAS 底座】                                              │
│   AgentAbilitySystemComponent（玩家挂 PlayerState/敌人挂自身）│
│   AgentAttributeSet（Health/Mana/AttackPower/Armor…）       │
│   AgentGameplayEffectContext（骨骼/方向/暴击+NetSerialize）  │
│   AgentAbilitySystemLibrary / DA_AbilitySet 授予管线         │
│   输入→GameplayTag→GA 触发管线                              │
│   BuffComponent + BuffManagerSubsystem + DT_Buff            │
│   AgentDamageExecCalc（伤害公式）                            │
│   GameplayCue 管线 / 死亡事件出口（OnActorKilled）           │
│  【动画底座】                                              │
│   ABP 分层架构（Locomotion 状态机 + UpperBody 层 + FullBody）│
│   Anim Layer Interface（武器动画集热插拔）                   │
│   AnimNotify_GameplayTag → ASC GameplayEvent 桥             │
│   骨骼→受击动画变体映射（数据驱动）                          │
└─────────────────────────────────────────────────────────────┘
```

依赖方向：**上层 → 基础，基础不引用上层**。GDD §5：Combat 不依赖任务；AI 依赖 Combat 伤害/受击。

---

## 3. 基础系统层设计（GAS 底座）

### 3.1 GAS 接入（前置接线）

| 项 | 改动 |
|----|------|
| 插件 | `AgentDemo.uproject` 启用 `GameplayAbilities`（当前未启用！） |
| Build.cs | Public 依赖加 `GameplayAbilities` `GameplayTags` `GameplayTasks` |
| 重启 | 加插件必须重启编辑器（LiveCoding 解决不了插件） |

**ASC 挂载（01 Must 第 1 条，Lyra/Aura 做法）**：
- **玩家**：ASC 挂 `AgentPlayerState`（`IAbilitySystemInterface`）。`AgentCharacter::PossessedBy`（Server）/ `OnRep_PlayerState`（Client）里 `InitAbilityActorInfo(PlayerState, Character)`，Character 缓存 ASC/AttributeSet 引用
- **敌人**：ASC 挂敌人 Character 自身，`BeginPlay` 里 `InitAbilityActorInfo(this, this)`
- 面试备答点：挂 PlayerState → 死亡换 pawn 技能/属性不丢；敌人挂自身 → 生命周期随尸体

### 3.2 属性系统 `AgentAttributeSet`

必做 6 个（ReplicatedUsing + GetLifetimeReplicatedProps）：`Health / MaxHealth / Mana / MaxMana / AttackPower / Armor`
扩展：`Shield / MaxShield`（护盾 Buff 落点）、`Ammo / MaxAmmo`（武器弹药）。

- `PreAttributeChange` Clamp；`PostGameplayEffectExecute` 判 Health≤0 → 死亡流程（`State.Dead` Tag + 死亡事件广播）
- 访问走 `ATTRIBUTE_ACCESSORS` 宏

### 3.3 `AgentGameplayEffectContext`

字段：`HitBoneName`（命中骨骼）、`HitDirection`（命中方向）、`bIsCritical`。实现 `GetScriptStruct`/`NetSerialize` 样板。**动画侧消费者**：受击 GA 按 HitBoneName 选部位动画（见 3.11），血迹方向用 HitDirection。

### 3.4 能力授予管线

- `DA_AbilitySet`（PrimaryDataAsset）：`StartupAbilities` / `StartupEffects` / `Tags`
- `AgentAbilitySystemComponent::GiveAbilitySet(DA)`：GiveAbility 并记录 `FGameplayAbilitySpecHandle` 数组（**可卸载**，武器切换回收旧 set）
- 配置入口：蓝图子类 `EditDefaultsOnly` 注入，不硬编码

### 3.5 输入 → GA 触发管线

```
IA_Fire / IA_Skill1 / IA_Skill2 / IA_SwitchWeapon
  → AgentPlayerController（现有管线扩展，TMap<UInputAction*, FGameplayTag> 数据点由 BP 注入）
  → AgentCharacter → ASC->TryActivateAbilitiesByTag(Tag)
```
AI 侧同口（GDD §6.5）：`BTTask_ActivateAbilityByTag`，VS2 用。

### 3.6 Buff 系统

`DT_BuffDefinition`（BuffTag/StackPolicy/Duration/Period/GEClass）+ `BuffComponent`（挂 Actor，查询/表现入口）+ `BuffManagerSubsystem`（GameInstanceSubsystem，与 UIManager 同模式，统一增删查，Server 执行）。
示例：灼烧（Periodic 跳伤）、减速（属性层演示，移动速度回写桥见 §9 风险）、护盾（Duration GE 加 Shield）。

### 3.7 伤害三条路径（面试必讲，各留一个示例）

| 路径 | 示例落点 |
|------|----------|
| AttributeBased | 灼烧跳伤基于施加者 AttackPower 系数 |
| SetByCaller | 开火 GA：`Data.Damage` ← 武器行 Damage |
| ExecCalc | `AgentDamageExecCalc`：`Base × (1+AttackPower/100) − Armor`，先扣 Shield 再扣 Health，Context.bIsCritical 乘暴击系数 |

### 3.8 GameplayCue 表现管线

`GameplayCue.Combat.Hit.*`（按部位/材质）、`GameplayCue.Weapon.*`（枪口）。纪律：GC 只做表现，逻辑不进 GC。

### 3.9 死亡事件出口

AttributeSet 判死 → `State.Dead` Tag + 禁用输入 + 死亡表现 + Server 广播 `OnActorKilled(Killer, Victim, VictimIdTag)`。VS1 定义接口与广播点（`IQuestWorldSink` 占位），VS4 由 QuestManager 消费。玩家死亡流程（复活点扣 Gold）归 VS3+，留 TODO。

### 3.10 网络同步策略（01 §7）

GA `InstancedPerActor`；属性 Server 改值 RepNotify 刷 UI；Instant 伤害以 Server 为准；Context 字段随 NetSerialize 到客户端；M7 Listen Server 双端验证（用户手动测）。

---

## 3.11 动画表现底座（补全：状态机 / 分层 / 混合 / 动画驱动数据）

> 定位：动画是战斗的**表现层底座**，与 GAS 底座平级。原则与 GAS 一致——**C++/数据驱动，动画蓝图只做配置与状态表达**。

### A. ABP 分层架构（改造现有 `ABP_Unarmed`）

现有：Locomotion 由 `BS_Idle_Walk_Run`（八方向 2D BlendSpace）自驱动。VS1 在其上扩为三层：

```
Output Pose
  └─ Layered Blend Per Bone（骨骼 spine_02，上下半身分离）
       ├─ 下半身/基础：Locomotion 状态机（现有八方向，不动）
       │     ├─ Idle/Walk/Run（BS 混合）
       │     └─ Death（FullBody 状态，State.Dead 时进入，禁退出）
       └─ 上半身：UpperBody Slot（Montage 播放入口）
             └─ 开火/近战/投掷/受击 Montage 在此播放，与移动叠加
```

- **状态机边界**：Locomotion 状态机只管移动与死亡；攻击/受击**不进状态机**，走 Montage Slot（攻击是瞬时动作，进状态机会爆炸式膨胀——面试题落点）
- **分层目的**：边跑边开枪（下半身跑、上半身开火）——射击游戏的基本要求，也是"动画分层"的最直观演示
- **死亡**：`State.Dead` Tag → ABP 读 Tag（或 Character 同步 bool）→ 进 Death 状态播死亡 Montage（FullBody 覆盖分层）

### B. Anim Layer Interface（武器动画集热插拔）

- 定义 `ALI_Combat`（Animation Layer Interface）：声明 `RifleLayer` / `MeleeLayer` 等接口层
- 每把武器一个动画集实现（`ABP_ALI_Rifle` / `ABP_ALI_Melee` 实现接口）
- 切换武器 → `LinkAnimClassLayers` 换实现 → **换武器 = 换动画集**，与 AbilitySet 切换同节奏（Lyra 同款机制，面试强点）
- 武器行 `DT_WeaponDefinition` 加列：`AnimLayerClass`（SoftClassPath）——动画集也是数据驱动的

### C. 瞄准偏移 AimOffset（动画混合的第二个演示点）

- 步枪持枪态：`AO_Rifle_Aim`（Pitch -75°~10° 采样，对齐现有 MinViewPitch/MaxViewPitch）
- 驱动源：Controller Pitch（ABP 自驱动读取，与现有 Direction/GroundSpeed 同模式，C++ 不写动画变量）
- 叠加方式：在 UpperBody 层后与 Montage 叠加（AO 是姿势叠加，Montage 是动作播放——两者并存不冲突）

### D. 动画驱动数据判定（核心机制：AnimNotify → GameplayEvent）

**攻击判定时机由动画决定，不由按键决定**：

```
GA 激活 → PlayMontageAndWait(攻击 Montage)
  Montage 上的 UAnimNotify_GameplayTag（如 Event.Montage.Fire）到达判定帧
  → 自动向 ASC 发送 GameplayEvent
  → GA 内 WaitGameplayEvent 收到 → 此刻才 LineTrace/Sweep/Spawn 投掷物
```

- C++ 零 notify 绑定代码：`UAnimNotify_GameplayTag` 是 GAS 自带 notify，发事件进 ASC；GA 用 `UAbilityTask_WaitGameplayEvent` 等待
- 数值/时刻解耦：策划在 Montage 里拖 notify 位置调判定帧，不改代码
- 近战用 `AnimNotifyState` 版本开判定窗（窗内 Sweep 持续判定）
- **面试备答**：为什么不用 AnimNotify 直接调函数——GameplayEvent 走 ASC 通道，Server/Client 路由与预测由 GAS 处理；裸 notify 调函数要自己处理网络

### E. 骨骼 → 受击动画变体（数据驱动）

- `DT_HitReactVariants` 或 `TMap<FName, TObjectPtr<UAnimMontage>>` 数据点：骨骼名（或部位分组：头/躯干/左臂/右臂/腿）→ 受击 Montage 变体
- `GA_HitReact` 读 `Context.HitBoneName` → 查表选 Montage → UpperBody 层播放
- 死亡不受分层影响：FullBody Death Montage

### F. 动画资产（已定：Lyra 迁移，2026-08-07 完成）

已从本地 Lyra 工程（`D:\UEProject\LyraStarterGame`，UE 5.8 同版本）迁移，**策略：骨骼兼容共享**——Lyra `SK_Mannequin` 与本工程骨架骨骼集完全一致（66 骨骼零差异），只迁动画/骨架/武器/音效，**不迁网格贴图**（角色仍用 `SKM_Manny_Simple`，靠骨架兼容播放 Lyra 动画）。

已迁入（路径与 Lyra 原结构一致）：
- `Characters/Heroes/Mannequin/Meshes/SK_Mannequin`（Lyra 骨架，含其 Slot/Socket 配置）
- `Animations/Actions`（受击方向×强度、方向死亡 6 种、Rifle_Melee、Rifle_GrenadeToss 等 91 项）
- `Animations/AimOffsets`（`AO_MM_Rifle_Idle_ADS` / `AO_MM_Rifle_Idle_Hipfire` 全套切片）
- `Animations/Locomotion/Rifle`（步枪持枪移动全套，Step 6+ 战斗 Locomotion 升级用）
- `Animations/`（Lyra 曲线压缩设置、枚举/结构体）
- `Weapons/Rifle`（AM_MM_Rifle_Fire/Equip/Reload/DryFire + 武器网格 ABP_Weap 等）
- `Audio/`（步枪枪声/机械音效、近战挥砍、SFX 声类、衰减预设）

主动放弃（Lyra 框架耦合或 C++ 依赖，缺失不影响动画播放）：脚步系统（FootFXAnimModifier/AN_FootPlant）、转身修正（TurnYawAnimModifier）、自定义 notify（AN_Melee/AN_Reload/AN_PlayWeaponMontage）、SKM_Quinn 预览网格引用、Lyra 自带的死引用（Turn 动画序列）。

**骨架已统一（用户处理）**：迁移过来的 Lyra 骨架因预览网格引用缺失导致动画编辑器崩溃，已删除并把所有动画**重定向到项目自有骨架**（骨骼集一致，全部可用）。全项目单一骨架，无兼容层。

---

## 4. 上层业务层设计

### 4.1 `GA_Fire_Hitscan`（步枪开火）

触发 → Cost GE（Ammo-1）→ Cooldown GE → 播 Fire Montage（UpperBody）→ **GameplayEvent 到达判定帧** → 相机中心 LineTrace → 组装 Context（骨骼/方向/暴击：头部命中=暴击 简化规则）→ SetByCaller 注入武器 Damage → Apply GE_Damage → 目标受击 → GC 枪口/命中表现。

### 4.2 `GA_Fire_Projectile`（手雷）

投掷 Montage 的 GameplayEvent 帧 Spawn `ProjectileActor`（重力弧线）→ 碰撞 AoE：半径内 Apply GE_Damage + 灼烧 Buff → GC 爆炸。Cost：Mana。

### 4.3 `GA_Melee_Sweep`（砍刀）

挥砍 Montage + AnimNotifyState 判定窗 → 窗内 Box Sweep → Apply GE_Damage + Context（HitDirection 驱动血迹方向）。

### 4.4 `GA_Summon_Minion`（召唤诱饵）

Spawn 限时友方小怪（复用 AgentEnemyCharacter + Faction Tag），到时销毁。VS1 只"召出、站桩、到时消失"，行为归 VS2。

### 4.5 `GA_HitReact`（受击）

受击 Tag 触发 → 读 Context.HitBoneName → 查部位变体表选 Montage → UpperBody 播放 → 部位 GC。玩家/敌人共用。

### 4.6 武器系统

- `WeaponActor` Attach 手部 Socket；`WeaponManagerComponent`：`EquipWeapon(WeaponId)` → 读 DT → Spawn/Attach → **GiveAbilitySet（换技能）+ LinkAnimClassLayers（换动画集）** → 卸载旧 set/层
- `DT_WeaponDefinition`：`WeaponId / Damage / FireGA / EquipMontage / FireMontage / AnimLayerClass / AmmoAttr`
- 行：`Weapon_Rifle_T1`（Hitscan，25）、`Weapon_Machete`（Melee，40）——Machete 先加 Registry 再引用（GDD §8 纪律）
- `IA_SwitchWeapon`（1/2）切换，输入映射同步换

### 4.7 郊狼（`Enemy_Coyote` 可击杀目标）

- `AgentEnemyCharacter` 基类（`AI/` 域）：ASC+AttributeSet 挂自身 + RegistryId 身份 + 受击/死亡通用流程
- `BP_Enemy_Coyote`：Mannequin 占位 + 只含受击 GA 的 AbilitySet + 专属受击/死亡动画变体表
- VS1 站桩（无 BT 无移动）；死亡：FullBody 死亡 Montage → 禁碰撞 → 延迟销毁 → 抛 OnActorKilled
- `M_Town_Dev` 镇外放 3 只（对齐 GDD 环 1 靶量）

### 4.8 HUD 最小 UI

- `AgentHUD` 落地：`BeginPlay` 创建 `WBP_HUD_InGame`（常驻 HUD 走 HUD 类，**不经 UIManager**；UIManager 只管打开/关闭型 UI）
- `UAgentHUDWidget`（C++）：`GetGameplayAttributeValueChangeDelegate` 绑定刷血/蓝/弹药；技能栏 3 格 CD 遮罩；武器名/弹药数（01 §8 的 1/2/4，红屏可选不做）

---

## 5. GameplayTag 清单（经 ProjectSettings GameplayTag 表注册）

```
Ability.Fire.Hitscan / Ability.Fire.Projectile / Ability.Melee.Sweep / Ability.Summon.Minion / Ability.HitReact
Event.Montage.Fire / Event.Montage.Throw / Event.Montage.MeleeWindow   ← 动画事件桥
Data.Damage（SetByCaller）
Buff.Burn / Buff.Slow / Buff.Shield
State.Dead
Faction.Player / Faction.Enemy
GameplayCue.Combat.Hit.* / GameplayCue.Weapon.*
```

## 6. 数据资产清单

| 资产 | 类型 | 内容 |
|------|------|------|
| `DA_AbilitySet_Player` / `_Rifle` / `_Machete` / `_Coyote` | DataAsset | 各角色/武器的 GA 授予集 |
| `DT_WeaponDefinition` | DataTable | Rifle/Machete（含 FireMontage/AnimLayerClass 列） |
| `DT_BuffDefinition` | DataTable | Burn/Slow/Shield |
| `DT_HitReactVariants`（或 Map 数据点） | DataTable/配置 | 骨骼/部位 → 受击 Montage 变体 |
| `ALI_Combat` + `ABP_ALI_Rifle` / `ABP_ALI_Melee` | Anim Layer 资产 | 武器动画集热插拔 |
| `AO_Rifle_Aim` | AimOffset | 瞄准 Pitch 混合 |
| 攻击/受击/死亡 Montage | AnimMontage | 含 GameplayTag Notify 帧位配置 |
| `GE_Damage / GE_Cost_* / GE_Cooldown_* / GE_Buff_*` | GE 蓝图 | 纯数据配置 |
| `IA_Fire / IA_Skill1 / IA_Skill2 / IA_SwitchWeapon` + IMC 更新 | 输入资产 | BP 注入 |
| `WBP_HUD_InGame` | Widget 蓝图 | 继承 UAgentHUDWidget |

## 7. 文件/目录清单（Source/AgentDemo/ 映射）

```
AbilitySystem/            ← GAS 底座（GDD §11 所有者：01）
  Public/  AgentAbilitySystemComponent / AgentAttributeSet / AgentGameplayEffectContext
           AgentAbilitySystemLibrary / DA_AbilitySet / AgentGameplayAbility(基类)
  Private/ 对应 cpp；Abilities/ Executions/ Buff/ 子域同结构
Animation/                ← 动画底座（C++ 侧薄层：事件桥说明、部位映射组件）
  Public/  AgentAnimInstance（如需 C++ 基类暴露 Tag/状态给 ABP）
Equipment/                ← WeaponActor / WeaponManagerComponent（03 前置，VS3 接装备栏）
AI/                       ← AgentEnemyCharacter（VS2 接行为）
Core/                     ← AgentPlayerState 加 ASC；AgentHUD 落地；AgentCharacter 加 ASC 缓存/输入转发
Player/                   ← AgentPlayerController 加技能输入绑定
Content/Animation/        ← Montage/AO/ALI/ABP_ALI_* 动画资产
```

## 8. 实施序列（小步交付，每步可验收可提交）

| Step | 里程碑 | 交付 | 验收（用户手动测） |
|------|--------|------|-------------------|
| 1 | M1 | GAS 插件接入 + ASC 挂 PlayerState + AttributeSet + 测试键掉血 | 按键 Health 变化，日志可见 |
| 2 | M2 | GA_Fire_Hitscan + SetByCaller + 郊狼站桩可打死（按键即判定占位） | **GDD VS1 验收：开枪打死郊狼** |
| 3 | M3+动画 | **ABP 分层改造 + 攻击 Montage + GameplayEvent 判定切换** + EffectContext + GA_HitReact + 部位动画/GC | 边跑边开枪；判定帧跟动画走；部位受击表现 |
| 4 | M4 | BuffComponent + BuffManager + 灼烧/减速 | Buff Tag 可查，灼烧周期掉血 |
| 5 | M5 | AgentDamageExecCalc（攻防+护盾+暴击） | 换属性伤害数值变化正确 |
| 6 | M6+动画 | 武器系统 + 砍刀 Sweep + **ALI 武器动画集热插拔 + AimOffset** + WBP_HUD_InGame | 1/2 切武器：技能/动画集/输入同步换；瞄准随视角 |
| 7 | M7 补 | 手雷 Projectile + 召唤诱饵（投掷动画事件帧） | 三类 GA 各 1 个可用 |
| 8 | M7 | Listen Server 双端验证（含动画/GC 复制表现） | Client 看到伤害/Buff/受击/动画表现 |

Step 2 完成即达 GDD VS1 玩家验收；Step 3 起补 01 完整度与动画深度（面试口径）。

## 9. 风险与坑

- **GameplayAbilities 插件未启用**：Step 1 先改 uproject + Build.cs，**必须重启编辑器**
- **动画资产已迁入**（Lyra，见 3.11.F）；Step 3 需配骨架兼容（CompatibleSkeletons）后 Lyra 动画才能在我们网格上播放；残留空引用（脚步/自定义 notify）会让 Montage 加载时报 warning，可容忍或逐步清理
- **动画蓝图图 MCP 读不了**（CLAUDE.md 已记坑）：ABP 分层/状态机改造可能需要用户在编辑器手动完成关键节点，或半自动（write_graph_dsl 对 ABP 支持未验证，实施时先小规模试）
- **LiveCoding 与新增反射类型**：新增 UCLASS/USTRUCT（EffectContext 等）后建议 IDE Build 不走 LiveCoding（VS0 教训）
- **InitAbilityActorInfo 双端时机**：PossessedBy/OnRep_PlayerState 两处都调，漏 Client 侧 GA 激活失败
- **GiveAbilitySet 可卸载**：切武器漏回收 handle 残留旧技能
- **Montage 与分层冲突**：攻击 Montage 必须指定 UpperBody Slot，错放 FullBody 会打断移动；死亡才用 FullBody
- **GameplayEvent 丢失**：Montage 被打断（受击插进来）时 WaitGameplayEvent 等不到事件——GA 要有 OnCancelled 分支清理（不判定不扣费或退还）
- **减速 Buff 与移动组件**：GE 改属性，MaxWalkSpeed 需回写桥，VS1 只做属性层演示并注明
- **Machete 不在 Registry**：先加 GDD §8 表再建武器行
- **GameplayTag 先注册再引用**（对齐内容 Id 纪律）

## 10. 面试备答映射（01 §12 + 动画题 → 代码落点）

| 追问 | 落点 |
|------|------|
| ASC 为何挂 PlayerState | §3.1 双端初始化 |
| GE Duration/Infinite/Instant 场景 | GE_Damage(Instant)/GE_Buff(Duration)/装备被动(Infinite,VS3) |
| Prediction 失败回滚 | 开火预测与 Server 校正（Step 8 演示） |
| ExecCalc vs Modifier | §3.7 三路径对照 |
| GC 边界 | §3.8 表现全走 GC |
| **攻击为什么走 Montage 不进状态机** | §3.11.A：瞬时动作 vs 持续状态的边界 |
| **分层混合怎么实现边跑边打** | §3.11.A Layered Blend Per Bone @ spine_02 |
| **动画帧怎么驱动伤害判定** | §3.11.D AnimNotify_GameplayTag → WaitGameplayEvent，网络由 GAS 管 |
| **换武器怎么换动画** | §3.11.B Anim Layer Interface 热插拔 |
| **瞄准怎么跟视角** | §3.11.C AimOffset Pitch 驱动 |
