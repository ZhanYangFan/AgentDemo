# Lyra 动画资产迁移清单（VS1 用）

> 数据来源：Lyra 工程（`D:\UEProject\LyraStarterGame`）Asset Registry 权威依赖闭包，2026-08-07 计算。
> 闭包共 526 项 = **导出 461 项（156MB）** + 排除 65 项。
> 操作方式：Lyra 编辑器内容浏览器选中下方「迁移起点」→ 右键 **Migrate** → 目标选 `D:\UEProject\AgentDemo\Content` → 弹窗按「取消勾选」清单处理。

---

## 1. 迁移起点（在 Lyra 内容浏览器选中这些）

| 选中项 | 内容 | 量级 |
|---|---|---|
| `Content/Characters/Heroes/Mannequin/Animations/Actions` | 受击 15（方向×强度）/ 死亡 6 / Rifle_Melee / GrenadeToss 等 | 91 个 |
| `Content/Characters/Heroes/Mannequin/Animations/AimOffsets` | 步枪 AO（ADS + Hipfire 全套切片） | 98 个 |
| `Content/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle` | 步枪持枪移动全套（含 `ABP_RifleAnimLayers` 不勾，见 §2） | 118 个 |
| `Content/Characters/Heroes/Mannequin/Animations` 根目录散装文件 | 曲线压缩设置 + 枚举/结构体（被 70+ 动画引用） | 5 个 |
| `Content/Characters/Heroes/Mannequin/Meshes/SK_Mannequin` | Lyra 骨架（含 Slot/Socket 配置） | 1 个 |
| `Content/Weapons/Rifle`（整个文件夹） | 开火/换弹 Montage + 武器网格 + 贴图 + 音效 | 20 个 |

## 2. Migrate 弹窗中取消勾选（Lyra C++ 耦合，勾了必然是坏资产）

| 类别 | 资产 |
|---|---|
| 动画蓝图（7） | `ABP_Mannequin_Base`、`ABP_ItemAnimLayersBase`、`ABP_RifleAnimLayers`、`ABP_RifleAnimLayers_Feminine`、`ABP_Manny_PostProcess`、`ABP_Quinn_PostProcess`、`ABP_Weap_Rifle` |
| 自定义 Notify（2+） | `AnimNotifies/AN_PlayWeaponMontage`、`Heroes/Abilities/AN_Melee`、`AN_Reload` |
| 动画修改器（4） | `AnimModifiers/FootFXAnimModifier`、`FootstepEffectTagModifier`、`TurnYawAnimModifier` 等 |
| 脚步特效 | `Effects/` 下 10 个（AN_FootPlant_L/R 等） |
| 角色网格链（可选） | `Meshes/SKM_Manny`、`SKM_Quinn` + `Mannequin/Materials` + `Mannequin/Textures`（**437MB**，我们角色继续用 `SKM_Manny_Simple`，不勾） |

> 取消勾选后，Montage 加载会报"找不到 notify/modifier"的 warning——无害，动画正常播放，VS1 不需要脚步/转身系统。

## 3. 会被依赖自动带入（正常，保留）

| 内容 | 大小 | 说明 |
|---|---|---|
| 武器网格 + 贴图 | ~49MB | `Weapons/Rifle/Mesh` + `Textures`，WeaponActor 外观用 |
| 音频链 | ~6MB | 枪声（Punch/Noise/Bolt/Clip）、Foley、MetaSound 预设、声类、Submix、衰减 |
| 物理材质 | 极小 | `PhysMat_Player` / `PhysMat_Player_WeakSpot` / `PM_Character`（受击部位判定可用） |

## 4. 迁移后收尾（在 AgentDemo 编辑器）

1. **骨架二选一**：
   - **A. 统一到项目骨架**（推荐，你上次已做过）：删 `/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin` → 弹窗「替换引用」→ 选 `/Game/Characters/Mannequins/Meshes/SK_Mannequin`
   - B. 保留 Lyra 骨架：需手动把它的 Preview Mesh 设为 `SKM_Manny_Simple`（否则动画编辑器打开崩溃）
2. **补 Slot 组**：Montage 引用 Lyra 的 Slot（`UpperBody`/`FullBody` 等）；统一骨架后在项目骨架上补建同名 Slot 组（VS1 Step 3 ABP 分层时用）
3. **验证**：打开 `AM_MM_Rifle_Fire` 能正常预览播放即成功

## 5. 资产与 VS1 技能的对应关系

| VS1 内容 | 用哪个 Lyra 资产 |
|---|---|
| 步枪开火 GA | `Weapons/Rifle/Animations/AM_MM_Rifle_Fire`（+ Equip/Reload/DryFire） |
| 瞄准偏移 | `AimOffsets/AO_MM_Rifle_Idle_Hipfire`（腰射）/ `AO_MM_Rifle_Idle_ADS`（机瞄） |
| 受击 GA（部位变体） | `Actions/AM_MM_HitReact_*`（Front/Back/Left/Right × Lgt/Med/Hvy） |
| 死亡表现 | `Actions/AM_MM_Death_*`（6 方向变体，可按 HitDirection 选播） |
| 近战（砍刀占位） | `Actions/AM_MM_Rifle_Melee`（枪托挥击） |
| 手雷投掷 | `Actions/AM_MM_Rifle_GrenadeToss` |
| 战斗 Locomotion（Step 6 升级） | `Locomotion/Rifle/` 全套（Jog 八方向 + 启停 + Idle） |
