// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "GameplayCameraSystemToolset.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameplayCameraSystemToolSet, Log, All);

/**
 * MCP 工具集：创建与配置 GameplayCameras 资产（CameraRig / CameraAsset）。
 * 工具通过 JSON 字符串传递参数/结果，MCP 侧以
 * GameplayCameraSystemToolSet.UGameplayCameraSystemToolset.<fn> 调用。
 *
 * 约定：
 * - 返回 FString 的工具：成功为路径/JSON 结果；失败为 {"error":"原因"} JSON，
 *   同时写 LogGameplayCameraSystemToolSet Warning 日志。
 * - 修改结构/参数后资产处于 Dirty，需要构建才生效：
 *   调 BuildRig / BuildCameraAsset，或 SetRigRoot / SetSingleDirector（也会构建）。
 */
UCLASS(BlueprintType, Hidden)
class UGameplayCameraSystemToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	/** 创建 CameraRig 资产并标脏（由编辑器保存落盘）。AssetPath 如 /Game/Cameras/CR_ThirdPerson。返回资产路径。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString CreateCameraRig(const FString& AssetPath);

	/** 创建 CameraAsset 资产并标脏。返回资产路径。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString CreateCameraAsset(const FString& AssetPath);

	/** 向 CameraRig 添加节点并挂到父节点的引用属性（数组或单值对象属性）。
	 *  NodeClassName 支持四种写法：短名 "BoomArmCameraNode"、U 前缀 "UBoomArmCameraNode"、
	 *  全路径 "/Script/GameplayCameras.BoomArmCameraNode"、编辑器显示名（如 "Sequence" = ArrayCameraNode）。
	 *  ParentNodePath 为空表示仅创建不挂载（出现在 GetRigGraph 的 loose 列表）。
	 *  InputProperty 如 "Children"（Sequence/Array 节点）、"InputSlot"（BoomArm/SplineOrbit 节点）。
	 *  返回节点路径（可作后续 Parent/NodePath 参数）。改完请调 BuildRig 重建。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString AddNodeToRig(const FString& RigPath, const FString& NodeClassName, const FString& ParentNodePath, const FString& InputProperty, const FString& NodeName);

	/** 设置节点参数（反射 JSON 写入）。
	 *  PropertyName 用 C++ PascalCase（如 "BoomOffset"）；JsonValue 为该属性的 JSON：
	 *  相机参数结构 {"Value":...}（如 {"Value":{"x":0,"y":0,"z":-350}}）、普通 struct 平铺、
	 *  资产引用/数组用路径字符串（如 ["/Game/Input/IA_Look.IA_Look"]）。
	 *  返回 true=写入成功。改完请调 BuildRig 重建。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static bool SetNodeParameter(const FString& NodePath, const FString& PropertyName, const FString& JsonValue);

	/** 读取节点参数（JSON）。只读细节面板可编辑属性；无法序列化的属性列入 "unreadable" 数组，不再整体失败。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString GetNodeParameters(const FString& NodePath);

	/** 读取 CameraRig 结构（JSON）：
	 *  {"nodes":[{"path","class"}...], "edges":[{"parent","child","property","index"}...], "loose":[{"path","class"}...]}
	 *  edges 按属性数组序给出（index=-1 为单值属性）；loose 为未挂接到 RootNode 树下的散节点。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString GetRigGraph(const FString& RigPath);

	/** 设置 CameraRig 的 RootNode 并构建（BuildCameraRig）。返回 false=构建有错误，详见日志。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static bool SetRigRoot(const FString& RigPath, const FString& RootNodePath);

	/** 配置 CameraAsset 使用 SingleCameraDirector 引用指定 CameraRig 并构建。
	 *  已是 SingleCameraDirector 则复用（不重复新建）；返回 false=构建有错误，详见日志。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static bool SetSingleDirector(const FString& AssetPath, const FString& RigPath);

	/** 从 CameraRig 删除节点：自动从引用它的父属性（数组/单值/RootNode）摘除并标记垃圾回收。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static bool RemoveNodeFromRig(const FString& RigPath, const FString& NodePath);

	/** 构建 CameraRig 并收集构建消息。返回 {"ok":bool,"status":"Clean|...","messages":[...]} JSON。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString BuildRig(const FString& RigPath);

	/** 构建 CameraAsset 并收集构建消息。返回 {"ok":bool,"status":"Clean|...","messages":[...]} JSON。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString BuildCameraAsset(const FString& AssetPath);

	/** 读取构建状态（不触发构建）。入参可为 CameraRig 或 CameraAsset 路径。返回 {"status":"Clean|CleanWithWarnings|WithErrors|Dirty"}。 */
	UFUNCTION(meta = (AICallable), Category = "GameplayCameras")
	static FString GetBuildStatus(const FString& RigOrAssetPath);
};
