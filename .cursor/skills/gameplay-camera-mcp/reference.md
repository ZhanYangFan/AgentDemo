# Gameplay Camera System — Reference

Detailed reference for [SKILL.md](SKILL.md). Sources: UE 5.8 plugin source (`Engine/Plugins/Cameras/GameplayCameras`), official Overview/Quick Start docs, GPC developer diary (ludovic.chabant.com).

## Full node class list (UE 5.8, via search_subclasses of `/Script/GameplayCameras.CameraNode`)

Use these exact paths as `nodeClassName` (prefix with `/Script/GameplayCameras.`):

**Root / structure**
- `DefaultRootCameraNode`, `RootCameraNode`, `BlendStackCameraNode`, `BlendStackRootCameraNode` — system-level, rarely placed by hand
- `ArrayCameraNode` — editor name **"Sequence"**; runs `Children` array in order; standard rig root
- `CameraRigCameraNode` — references another CameraRig ("Camera Rig Prefab" node; parameter overrides possible)
- `CombinedCameraRigsCameraNode`, `CameraComponentCameraNode`, `BlueprintCameraNode`, `UpdateTrackerCameraNode`

**Pose sources / attachment**
- `AttachToPlayerPawnCameraNode` — `AttachToLocation` (bool param, default true), `AttachToRotation` (false), `SocketName`, `BoneName`
- `AttachToActorCameraNode`, `AttachToActorGroupCameraNode`, `CalcCameraActorCameraNode`, `FirstPersonCameraNode`

**Transforms**
- `OffsetCameraNode` — `TranslationOffset` (vec3 param), `RotationOffset` (rotator param), `OffsetSpace` enum
- `BoomArmCameraNode` — `BoomOffset` (vec3 param), `BoomLengthInterpolator` (CameraValueInterpolator obj), `MaxForwardInterpolationFactor`/`MaxBackwardInterpolationFactor` (double params), `InputSlot` (Input2D node pin), `bAdditiveRotation`
- `SetLocationCameraNode`, `SetRotationCameraNode`, `DampenPositionCameraNode` (`Forward/Lateral/VerticalDampingFactor`, `DampenSpace`), `DampenRotationCameraNode`
- `SplineOffsetCameraNode`, `SplineOrbitCameraNode` (has `InputSlot`), `SplineFieldOfViewCameraNode`

**Lens / body / post**
- `FieldOfViewCameraNode` (`FieldOfView` float param), `OrthographicCameraNode`, `FilmbackCameraNode`, `LensParametersCameraNode`, `BodyParametersCameraNode`, `ClippingPlanesCameraNode`, `AutoFocusCameraNode`, `PostProcessCameraNode`

**Collision**
- `CollisionPushCameraNode` — push-in on collision
- `OcclusionMaterialCameraNode` — `OcclusionTransparencyMaterial` (material obj), `OcclusionSphereRadius` (float param), `OcclusionChannel` (default `ECC_Camera`), `OcclusionTargetPosition` (default `Pawn`), `OcclusionTargetOffsetSpace`, `OcclusionTargetOffset`

**Framing**
- `BaseFramingCameraNode`, `DollyFramingCameraNode`, `PanningFramingCameraNode`, `TargetRayCastCameraNode`

**Input**
- `Input2DCameraNode` / `Input1DCameraNode` — input slot bases
- `InputAxisBinding2DCameraNode` — `AxisActions` (UInputAction array), `Multiplier` (vec2 param), `bIsAccumulated` (default true). Inherits from `CameraRigInput2DSlot`: `ClampX`/`ClampY` (`FCameraParameterClamping`: `MinValue/MaxValue/bClampMin/bClampMax`), `NormalizeX`/`NormalizeY`, `RevertAxisX`/`RevertAxisY` (bool params), `Speed` (vec2 param), `BuiltInVariable` (default `YawPitch`), `bIsPreBlended`
- `RawInputAxisBinding2DCameraNode`, `InputAccumulator2DCameraNode`, `AutoRotateInput2DCameraNode`, `DrivenControlRotationCameraNode`, `CameraRigInput2DSlot`, `CameraRigInput1DSlot`

**Shakes**
- `ShakeCameraNode` (base), `CompositeShakeCameraNode`, `EnvelopeShakeCameraNode`, `PerlinNoiseLocationShakeCameraNode`, `PerlinNoiseRotationShakeCameraNode`, `CameraShakeCameraNode`, `CameraShakeServiceCameraNode`

**Blend nodes** (for transitions, not rig pipelines)
- `BlendCameraNode` (base), `SimpleBlendCameraNode`, `SimpleFixedTimeBlendCameraNode`, `EasingBlendCameraNode`, `LinearBlendCameraNode`, `SmoothBlendCameraNode`, `LocationRotationBlendCameraNode`, `OrbitBlendCameraNode`, `PopBlendCameraNode`, `ViewTargetTransitionParamsBlendCameraNode`

## Parameter type → JSON mapping (for SetNodeParameter)

`T*CameraParameter` structs always wrap as `{"Value": <payload>}`; the optional `.variable` ref is for CameraVariableCollection binding (leave unset).

| C++ type | Payload |
|---|---|
| `FVector3dCameraParameter` | `{"x":0,"y":0,"z":0}` |
| `FVector2dCameraParameter` | `{"x":1.0,"y":1.0}` |
| `FRotator3dCameraParameter` | `{"pitch":0,"yaw":0,"roll":0}` |
| `FFloatCameraParameter`, `FDoubleCameraParameter` | number |
| `FBooleanCameraParameter` | `true`/`false` |
| `FCameraParameterClamping` (plain struct, no `Value` wrap) | `{"MinValue":-80,"MaxValue":10,"bClampMin":true,"bClampMax":true}` |
| Enum properties | enum name string, e.g. `"Pawn"`, `"World"` |
| `TObjectPtr<>` single ref | asset path string `"/Game/P/N.N"` |
| `TArray<TObjectPtr<>>` | array of asset path strings |

Note the naming convention split: `SetNodeParameter`/`GetNodeParameters` use C++ PascalCase (`BoomOffset`, `Value`); `ObjectTools` uses camelCase (`boomOffset`, `value`). Convert by uppercasing the first letter.

## CameraRigAsset / CameraAsset properties (for ObjectTools access)

`UCameraRigAsset`:
- `RootNode` (instanced UCameraNode) — set via `SetRigRoot`
- `GameplayTags`, `InitialOrientation` (`ECameraRigInitialOrientation`, default `None`)
- `EnterTransitions` / `ExitTransitions` (instanced `UCameraRigTransition` arrays) — no MCP wrapper; author in editor
- `BuildStatus` (transient; `Dirty` until built)

`UCameraAsset`:
- `EnterTransitions` / `ExitTransitions` = **Shared Transitions** applied when a rig doesn't specify its own
- Director reference + interface parameter mappings (`UCameraAssetInterfaceParameter`: `SourceCameraRig` + `SourceParameterName`)

`UCameraRigTransition`: `Conditions` array (input pin), plus a blend node (e.g. `SmoothBlendCameraNode`).

## Evaluation layers

Rigs activate onto one of four layers on the root node's Blend Stack:

| Order | Layer | Type | Use for |
|---|---|---|---|
| 1 | Base | Additive Persistent | Always-on behavior (breathing, idle sway) |
| 2 | Main | Isolated Transient | The primary camera behavior (your CR_* rigs) |
| 3 | Global | Additive Persistent | Effects stacked on main (camera shake) |
| 4 | Visual | Additive Persistent | Final look tweaks (post process) |

- **Isolated Transient**: rigs evaluate independently and blend by weight; when a new rig reaches 100% blend, older ones leave the stack.
- **Additive Persistent**: each rig's output feeds the next (relay); stays on the stack until explicitly deactivated.

## Runtime parameterization (Blueprint/C++, beyond the toolset)

- **Blendable parameters** (numeric/vector/rotator): blend smoothly during transitions. **Data parameters** (string/enum/object/struct): switch instantly at transition time.
- From a `GameplayCameraComponent`: `Get Shared Camera Data` (applies to all running rigs) vs `Get Conditional Camera Data` (active rig only), then parameter Get/Set nodes.
- `CameraVariableCollection` asset: variables bindable to node properties across rigs; settable at runtime via Camera Data.
- `Get Initial Variable Table` → `Set Camera Rig Parameters` sets per-rig initial values (e.g. drive `BoomOffset` from a zoom variable).
- **Pre-Blending** (`bIsPreBlended` on input slots, blendable parameter Pre-Blend flag): blend parameter values before evaluating rigs; only valid when blending rigs share the same logic. Default off in 5.7+ for bool properties (back-compat).
- `CameraRigMerge` optimization: two stack entries from the same rig asset can merge into one evaluation.

## Director types

- **Single Camera Director** — activates its one assigned rig; no logic. `SetSingleDirector` configures this.
- **Blueprint Camera Director** — BP evaluator (`Run Camera Director` tick or one-shot `Activate Camera Director` event): `Find Evaluation Context Owner Actor` → read state → `Activate Camera Rig`. Prefer computing selection state on the character, keep the director thin.
- **StateTree Camera Director** — StateTree selects rigs (`StateTreeCameraDirectorTasks`).

## Official references

- Overview: https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-camera-system-overview
- Quick Start (3 rigs + Blueprint director switching): https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-camera-system-quick-start
- GPC developer diary (architecture, evaluators, parameterization, pre-blending): https://ludovic.chabant.com/blog/2025/05/05/ue-gameplay-cameras-the-basic-design/
- Best sample project: **GameAnimationSample** (`/Content/Blueprints/Cameras`) — CameraRigPrefab_BasicThirdPersonBehavior + variations, Chooser-based director (UE 5.7+)
