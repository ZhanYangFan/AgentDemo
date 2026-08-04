---
name: gameplay-camera-mcp
description: Builds and configures Unreal Engine 5.8 Gameplay Camera System assets (CameraRig node trees, CameraAsset, camera directors) through the user-unreal MCP GameplayCameraSystemToolset. Use when creating or editing camera rigs or camera assets via MCP (CR_*/CA_*), wiring camera nodes (BoomArm, Offset, FieldOfView, DampenPosition, OcclusionMaterial, InputAxisBinding2D), building third-person/first-person/top-down cameras, or when the user mentions GameplayCamera, GameplayCameras, CameraRig, CameraAsset, camera MCP tools, or 相机系统/摄像机.
---

# Gameplay Camera System via MCP

Build GameplayCameras assets in a running UE 5.8 editor through MCP. All facts below were verified against the live toolset and engine plugin source (`Engine/Plugins/Cameras/GameplayCameras`).

## Prerequisites

- UE editor is running with the **ModelContextProtocol** and **GameplayCameras** plugins enabled.
- MCP server `user-unreal` is connected. Every call goes through its `call_tool`:

```
server: user-unreal
tool:   call_tool
args:   { "toolset_name": "GameplayCameraSystemToolSet.GameplayCameraSystemToolset",
          "tool_name": "<ToolName>", "arguments": { ... } }
```

Helper toolsets you will also need (same `call_tool` envelope, different `toolset_name`):
- `editor_toolset.toolsets.object.ObjectTools` — discover node classes/properties, read back values
- `editor_toolset.toolsets.asset.AssetTools` — exists/find/delete/save assets
- `EditorToolset.LogsToolset` — read editor log when a call misbehaves

## Mental model (30 seconds)

- **CameraAsset** — container asset: references CameraRigs + transitions + one **CameraDirector**. Assigned to a **GameplayCameraComponent** on the pawn/character (replaces legacy CameraBoom+CameraComponent).
- **CameraRig** — standalone asset holding a tree of **CameraNodes**. Camera data (transform, FOV, post-process, variable table) flows through the node pipeline; each node does one operation.
- **ArrayCameraNode** = the editor's **"Sequence"** node. It runs its `Children` array in order and is the standard rig root.
- **CameraDirector** — picks which rig is active. `Single` (one rig, no logic — fully supported by the MCP toolset), `Blueprint` (BP logic), `StateTree` (StateTree logic).

## The 12 camera tools

| Tool | Args → Returns | Notes |
|---|---|---|
| `CreateCameraRig` | `assetPath` → rig path | Saves via editor. E.g. `/Game/Cameras/CR_ThirdPerson` |
| `CreateCameraAsset` | `assetPath` → asset path | E.g. `/Game/Cameras/CA_PlayerCameras` |
| `AddNodeToRig` | `rigPath`, `nodeClassName`, `parentNodePath`, `inputProperty`, `nodeName` → node path | Class aliases + type-checked attach |
| `SetNodeParameter` | `nodePath`, `propertyName`, `jsonValue` → bool | Reflection JSON write, see format table |
| `GetNodeParameters` | `nodePath` → JSON | Editable props only; unreadable ones listed under `"unreadable"` |
| `GetRigGraph` | `rigPath` → JSON | `{nodes[], edges[], loose[]}` with parent→child edges and array order |
| `SetRigRoot` | `rigPath`, `rootNodePath` → bool | Sets RootNode **and builds**; false = build errors |
| `SetSingleDirector` | `assetPath`, `rigPath` → bool | Reuses existing Single director; false = build errors |
| `RemoveNodeFromRig` | `rigPath`, `nodePath` → bool | Detaches from any parent property and deletes |
| `BuildRig` | `rigPath` → JSON | `{ok, status, messages[]}` — build + collect errors |
| `BuildCameraAsset` | `assetPath` → JSON | Same for CameraAsset |
| `GetBuildStatus` | `rigOrAssetPath` → JSON | `{status: Clean/CleanWithWarnings/WithErrors/Dirty}`, no build |

Object path formats:
- Rig/asset: `/Game/Cameras/CR_X.CR_X` (package.object)
- Node: `/Game/Cameras/CR_X.CR_X:NodeName` (colon + subobject name)

**Failure contract (always check returns):**
- FString tools return `{"error":"reason"}` JSON on failure (success values are paths/JSON starting with `/` or `{` without an `error` key).
- bool tools return `false`; the reason is logged as `LogGameplayCameraSystemToolSet` Warning — read it via `LogsToolset.GetLogEntries` with category filter `LogGameplayCameraSystemToolSet`.
- Never continue a workflow after a failed step.

## Hard rules (verified by live testing)

1. **`nodeClassName` accepts aliases**: short name `BoomArmCameraNode`, `U`-prefixed, full path `/Script/GameplayCameras.BoomArmCameraNode`, editor display name (`Sequence` = ArrayCameraNode), or no-suffix `BoomArm`. Unknown names return a clear `{"error":...}`.
2. **Standard rig skeleton**: create an `ArrayCameraNode` (alias `"Sequence"`) named `Root` with empty parent → `SetRigRoot` → attach pipeline nodes to it with `inputProperty: "Children"` → attach input node to BoomArm with `inputProperty: "InputSlot"`.
3. **Rebuild after edits.** Structure/parameter edits leave the asset Dirty. Finish a batch of edits with `BuildRig` (check `ok` + `messages`), or `SetRigRoot`/`SetSingleDirector` (they build too). `GetBuildStatus` tells you whether a build is needed.
4. **Children order = evaluation order.** Pose sources (`AttachToPlayerPawnCameraNode`, `FirstPersonCameraNode`) first → transforms (`OffsetCameraNode`, `BoomArmCameraNode`) → smoothing (`DampenPositionCameraNode`) → lens/collision/post (`FieldOfViewCameraNode`, `OcclusionMaterialCameraNode`, `PostProcessCameraNode`) last. `GetRigGraph` `edges[].index` shows the actual order.
5. **Naming**: keep requested node names simple (`Root`, `BoomArm`, `FOV`) — the tool preserves them unless a same-name node already exists in the rig (then it suffixes `_N`); always use the returned path for follow-up calls.
6. **Case conventions differ by tool**: `SetNodeParameter`/`GetNodeParameters` use C++ PascalCase (`BoomOffset`); `ObjectTools` uses camelCase (`boomOffset`). Convert by uppercasing the first letter.

## SetNodeParameter JSON formats (verified)

`propertyName` is the C++ PascalCase name. `jsonValue` is a JSON **string** (escape quotes).

| Property type | Example `jsonValue` |
|---|---|
| `FVector3dCameraParameter` | `{"Value":{"x":-500,"y":0,"z":50}}` |
| `FRotator3dCameraParameter` | `{"Value":{"pitch":0,"yaw":0,"roll":0}}` |
| `FFloatCameraParameter` / `FDoubleCameraParameter` | `{"Value":5.0}` |
| `FBooleanCameraParameter` | `{"Value":true}` |
| `FVector2dCameraParameter` | `{"Value":{"x":1.0,"y":1.0}}` |
| Plain struct (e.g. `ClampY: FCameraParameterClamping`) | `{"MinValue":-80,"MaxValue":10,"bClampMin":true,"bClampMax":true}` |
| Asset/object array (e.g. `AxisActions`) | `["/Game/Input/IA_Look.IA_Look"]` |
| Single object ref (e.g. `OcclusionTransparencyMaterial`) | `"/Game/Materials/M_Translucent.M_Translucent"` |
| Enum (e.g. `OffsetSpace`) | `"World"` (one of `CameraPose/ActiveContext/OwningContext/Pivot/Pawn/World`) |

Discovery loop for any unfamiliar node — never guess property names:
1. `ObjectTools.search_subclasses` with `base_class: {"refPath": "/Script/GameplayCameras.CameraNode"}` → exact class paths.
2. Add the node, then `GetNodeParameters` (or `ObjectTools.list_properties`) → settable properties with their JSON shape.

## Standard workflow (checklist)

```
- [ ] 1. CreateCameraRig(/Game/Cameras/CR_<Name>)
- [ ] 2. AddNodeToRig "Sequence" named Root (no parent)
- [ ] 3. SetRigRoot(rig, ...:Root)
- [ ] 4. AddNodeToRig pipeline nodes → parent Root, inputProperty "Children"
- [ ] 5. AddNodeToRig InputAxisBinding2DCameraNode → parent BoomArm, inputProperty "InputSlot" (if look input needed)
- [ ] 6. SetNodeParameter on every node (discover schema first)
- [ ] 7. GetRigGraph → verify nodes/edges/loose match intent; BuildRig → ok:true
- [ ] 8. CreateCameraAsset(/Game/Cameras/CA_<Name>) ; SetSingleDirector(CA, CR)
- [ ] 9. Wire CA into the character's GameplayCamera component (see below)
- [ ] 10. PIE and verify (see Acceptance)
```

Naming convention: `CR_*` for rigs, `CA_*` for camera assets, under `/Game/Cameras/`.

## Recipes (official Quick Start values)

**Third-person** (boom + mouse look):
- `Root` = ArrayCameraNode
- Children: `BoomArmCameraNode` (BoomOffset `{-500,0,50}`) → `DampenPositionCameraNode` (ForwardDampingFactor 5) → `FieldOfViewCameraNode` (90) → `OcclusionMaterialCameraNode` (translucent material)
- BoomArm `InputSlot` = `InputAxisBinding2DCameraNode`: `AxisActions ["/Game/Input/IA_Look.IA_Look"]`, `ClampY {MinValue:-80, MaxValue:10, bClampMin:true, bClampMax:true}`, `RevertAxisY {"Value":true}`

**First-person**: BoomOffset `{50,0,50}`, FieldOfView 100. (Or use `FirstPersonCameraNode` as pose source.)

**Top-down**: BoomOffset `{-1000,0,0}`, FieldOfView, DampenPosition (Forward 5), OcclusionMaterial. Set the character controller pitch to about −50 (director or controller side, not a node).

Reference of a working rig in this project: `/Game/Cameras/CR_ThirdPerson` (Root=Array → Offset/BoomArm/AttachToPlayerPawn).

## Wiring to the character (not covered by the camera toolset)

The toolset only builds assets. Assembly is done in the character Blueprint (or via `Blue[reference.md](reference.md)printTools`/`ObjectTools`):
1. Delete legacy `CameraBoom`/`FollowCamera` components; add a **GameplayCamera** component.
2. Set its **Camera** = the `CA_*` asset; enable **Auto Activate**; set **Auto Activate for Player** = `Player 0` (or call `Activate Camera for Player Controller` at runtime).
3. Multi-rig switching needs a **Blueprint Camera Director** (create the CameraAsset with a Blueprint director in the editor): in its evaluation logic call `Activate Camera Rig` per state (see official Quick Start §"Configuring the Camera Director"). The MCP toolset only configures Single directors.
4. Rig enter/exit **transitions** (e.g. Smooth Blend) and shared transitions are not exposed by the toolset — author them in the editor.
5. Runtime parameter changes (zoom, target switching): Blueprint `Get Initial Variable Table` → `Set Camera Rig Parameters`, or Shared/Conditional Camera Data setters — see [reference.md](reference.md).

## Acceptance

- `GetRigGraph` shows every expected node with correct edges (parents, properties, indices) and empty `loose` (unless intentional).
- `BuildRig` / `BuildCameraAsset` return `{"ok":true,"status":"Clean"}`.
- Key params read back correct via `GetNodeParameters` or `ObjectTools.get_properties`.
- PIE log (`LogsToolset.GetLogEntries`, pattern `.*Camera.*`) shows `LogCameraSystem: Activating gameplay camera '<Character>.GameplayCamera'` and no build errors.
- In-game: `Tools → Debug → Camera Debugger` to inspect running rigs, blend weights, and per-layer state.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `{"error":"unknown node class ..."}` | Typo in `nodeClassName`; enumerate valid names with `ObjectTools.search_subclasses` on `/Script/GameplayCameras.CameraNode` |
| `{"error":"type mismatch: 'X' expects Y, but node is Z"}` | Wrong target property — e.g. only Input2D nodes go into `InputSlot`; pipeline nodes go into `Children` |
| `{"error":"property 'X' not found on Y"}` | Wrong `inputProperty` name — inspect the parent with `GetNodeParameters`/editor |
| `SetNodeParameter` false | Wrong property name case (PascalCase) or wrong JSON shape; read the reason in `LogGameplayCameraSystemToolSet` |
| Camera not active in PIE | Component AutoActivate/Player 0 not set; CA not assigned; rig/asset Dirty (run `BuildRig`/`BuildCameraAsset`) |
| Deleted node still listed | It was pending GC in an old build — current builds filter garbage from `loose`; the node is gone from edges, which is what matters |
| Tool missing from describe_toolset / "no longer available" | Plugin was just LiveCoding-compiled — restart the editor so the toolset registry re-enumerates UFUNCTIONs |

## Additional resources

- Full node class list, parameter/JSON mapping, layers, runtime parameterization, director & transition details: [reference.md](reference.md)
