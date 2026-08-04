// Copyright AgentDemo Project. All Rights Reserved.
//
// 工具集实现：GameplayCameras 资产创建/节点树构建/参数读写。
// 本文件仅包含工具集逻辑，不依赖模块（模块入口见 FGameplayCameraSystemToolSetModule.cpp）。

#include "GameplayCameraSystemToolset.h"

#include "AssetToolsModule.h"

#include "ToolsetRegistry/ToolsetLibrary.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Core/CameraAsset.h"
#include "Core/CameraNode.h"
#include "Core/CameraRigAsset.h"
#include "Core/CameraDirector.h"
#include "Directors/SingleCameraDirector.h"
#include "Build/CameraBuildContext.h"
#include "Build/CameraBuildLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayCameraSystemToolset)

using namespace UE::Cameras;

namespace
{
	/** JSON 转义（引号/反斜杠/换行）。 */
	FString JsonEscape(const FString& In)
	{
		FString Out = In;
		Out.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Out.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Out.ReplaceInline(TEXT("\r"), TEXT(""));
		Out.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		return Out;
	}

	/** FString 返回型工具的统一失败出口：记 Warning 日志 + 返回 {"error":"..."} JSON。 */
	FString FailJson(const FString& ToolName, const FString& Reason)
	{
		UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s failed: %s"), *ToolName, *Reason);
		return FString::Printf(TEXT("{\"error\":\"%s\"}"), *JsonEscape(Reason));
	}

	/** bool 返回型工具的统一失败出口：记 Warning 日志 + false。 */
	bool FailBool(const FString& ToolName, const FString& Reason)
	{
		UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s failed: %s"), *ToolName, *Reason);
		return false;
	}

	/**
	 * 解析节点类名 → UClass。支持：短名 "BoomArmCameraNode"、U 前缀、
	 * 全路径 "/Script/GameplayCameras.X"、编辑器 DisplayName（如 "Sequence"）、去 CameraNode 后缀（如 "BoomArm"）。
	 * 首次调用时枚举 UCameraNode 全部非抽象子类建缓存（键为小写）。
	 */
	UClass* ResolveNodeClass(const FString& ClassName)
	{
		static TMap<FString, UClass*> Cache;
		if (Cache.Num() == 0)
		{
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* Class = *It;
				if (!Class->IsChildOf(UCameraNode::StaticClass())
					|| Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
				{
					continue;
				}

				FString ClassNameNoPrefix = Class->GetName();
				ClassNameNoPrefix.RemoveFromStart(TEXT("U"));

				Cache.Add(Class->GetPathName().ToLower(), Class);          // /Script/GameplayCameras.X
				Cache.Add(Class->GetName().ToLower(), Class);                // UBoomArmCameraNode
				Cache.Add(ClassNameNoPrefix.ToLower(), Class);               // BoomArmCameraNode

				FString ShortAlias = ClassNameNoPrefix;
				if (ShortAlias.RemoveFromEnd(TEXT("CameraNode")))
				{
					Cache.Add(ShortAlias.ToLower(), Class);                  // BoomArm
				}
				const FString DisplayName = Class->GetMetaData(TEXT("DisplayName"));
				if (!DisplayName.IsEmpty())
				{
					Cache.Add(DisplayName.ToLower(), Class);                 // Sequence
				}
			}
		}

		if (UClass** Found = Cache.Find(ClassName.ToLower()))
		{
			return *Found;
		}
		// 兜底：蓝图节点类等未入缓存的全路径加载
		return LoadObject<UClass>(nullptr, *ClassName);
	}

	/** 通过 AssetTools 创建资产并标脏，返回资产路径。 */
	FString CreateAssetWithTools(const FString& AssetPath, UClass* AssetClass, FString& OutError)
	{
		const FString PackagePath = FPackageName::GetLongPackagePath(AssetPath);
		const FString AssetName = FPackageName::GetShortName(AssetPath);
		if (PackagePath.IsEmpty() || AssetName.IsEmpty())
		{
			OutError = FString::Printf(TEXT("invalid asset path '%s' (expect /Game/Folder/AssetName)"), *AssetPath);
			return FString();
		}

		IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
		UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackagePath, AssetClass, nullptr);
		if (!NewAsset)
		{
			OutError = FString::Printf(TEXT("AssetTools.CreateAsset failed for '%s'"), *AssetPath);
			return FString();
		}
		return NewAsset->GetPathName();
	}

	/**
	 * 在 OwnerClass 上查找名为 PropName 的引用属性（FObjectProperty 或元素为对象引用的 FArrayProperty），
	 * 并校验 ChildClass 能否放入。成功返回属性指针，失败填 OutError。
	 */
	FProperty* FindReferencePropertyChecked(UClass* OwnerClass, const FName PropName, UClass* ChildClass, FString& OutError)
	{
		if (FArrayProperty* ArrProp = FindFProperty<FArrayProperty>(OwnerClass, PropName))
		{
			FObjectProperty* Inner = CastField<FObjectProperty>(ArrProp->Inner);
			if (!Inner || !Inner->PropertyClass->IsChildOf(UCameraNode::StaticClass()))
			{
				OutError = FString::Printf(TEXT("array property '%s' on %s does not hold camera nodes"), *PropName.ToString(), *OwnerClass->GetName());
				return nullptr;
			}
			if (!ChildClass->IsChildOf(Inner->PropertyClass))
			{
				OutError = FString::Printf(TEXT("type mismatch: '%s' elements are %s, but node is %s"),
					*PropName.ToString(), *Inner->PropertyClass->GetName(), *ChildClass->GetName());
				return nullptr;
			}
			return ArrProp;
		}
		if (FObjectProperty* ObjProp = FindFProperty<FObjectProperty>(OwnerClass, PropName))
		{
			if (!ObjProp->PropertyClass->IsChildOf(UCameraNode::StaticClass()))
			{
				OutError = FString::Printf(TEXT("property '%s' on %s is not a camera node reference"), *PropName.ToString(), *OwnerClass->GetName());
				return nullptr;
			}
			if (!ChildClass->IsChildOf(ObjProp->PropertyClass))
			{
				OutError = FString::Printf(TEXT("type mismatch: '%s' expects %s, but node is %s"),
					*PropName.ToString(), *ObjProp->PropertyClass->GetName(), *ChildClass->GetName());
				return nullptr;
			}
			return ObjProp;
		}
		OutError = FString::Printf(TEXT("property '%s' not found on %s"), *PropName.ToString(), *OwnerClass->GetName());
		return nullptr;
	}

	/** 把节点挂到父节点的引用属性（已校验过类型）。 */
	bool AttachNodeToProperty(UObject* ParentNode, FProperty* Prop, UCameraNode* ChildNode)
	{
		if (FArrayProperty* ArrProp = CastField<FArrayProperty>(Prop))
		{
			FObjectProperty* Inner = CastField<FObjectProperty>(ArrProp->Inner);
			FScriptArrayHelper Helper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(ParentNode));
			const int32 NewIndex = Helper.AddValue();
			Inner->SetObjectPropertyValue(Helper.GetRawPtr(NewIndex), ChildNode);
			return true;
		}
		if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		{
			ObjProp->SetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<UObject*>(ParentNode), ChildNode);
			return true;
		}
		return false;
	}

	/** 一条父子边：child 经 parent 的 Property 属性（数组序 Index，单值为 -1）挂接。 */
	struct FNodeEdge
	{
		FString ParentPath;
		FString ChildPath;
		FString Property;
		int32 Index = -1;
	};

	/** 反射扫描 Node 的全部相机节点引用属性，收集出边。 */
	void CollectNodeEdges(UCameraNode* Node, TArray<FNodeEdge>& OutEdges)
	{
		for (TFieldIterator<FProperty> It(Node->GetClass()); It; ++It)
		{
			if (FObjectProperty* ObjProp = CastField<FObjectProperty>(*It))
			{
				if (!ObjProp->PropertyClass->IsChildOf(UCameraNode::StaticClass()))
				{
					continue;
				}
				const UObject* Child = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<UObject*>(Node));
				if (const UCameraNode* ChildNode = Cast<UCameraNode>(Child))
				{
					OutEdges.Add(FNodeEdge{ Node->GetPathName(), ChildNode->GetPathName(), ObjProp->GetFName().ToString(), -1 });
				}
			}
			else if (FArrayProperty* ArrProp = CastField<FArrayProperty>(*It))
			{
				FObjectProperty* Inner = CastField<FObjectProperty>(ArrProp->Inner);
				if (!Inner || !Inner->PropertyClass->IsChildOf(UCameraNode::StaticClass()))
				{
					continue;
				}
				FScriptArrayHelper Helper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(Node));
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					const UObject* Child = Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index));
					if (const UCameraNode* ChildNode = Cast<UCameraNode>(Child))
					{
						OutEdges.Add(FNodeEdge{ Node->GetPathName(), ChildNode->GetPathName(), ArrProp->GetFName().ToString(), Index });
					}
				}
			}
		}
	}

	/** 把构建日志转发到本模块日志类别。 */
	void ForwardBuildMessages(const FCameraBuildLog& BuildLog)
	{
		for (const FCameraBuildLogMessage& Message : BuildLog.GetMessages())
		{
			const FString Text = Message.ToString();
			switch (Message.Severity)
			{
			case EMessageSeverity::Error:
			case EMessageSeverity::CriticalError:
				UE_LOG(LogGameplayCameraSystemToolSet, Error, TEXT("%s"), *Text);
				break;
			case EMessageSeverity::Warning:
			case EMessageSeverity::PerformanceWarning:
				UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s"), *Text);
				break;
			default:
				UE_LOG(LogGameplayCameraSystemToolSet, Log, TEXT("%s"), *Text);
				break;
			}
		}
	}

	FString BuildStatusToString(ECameraBuildStatus Status)
	{
		return StaticEnum<ECameraBuildStatus>()->GetNameStringByValue(static_cast<int64>(Status));
	}

	/** 组装构建结果 JSON：{"ok":bool,"status":"...","messages":[...]}。 */
	FString BuildResultJson(ECameraBuildStatus Status, const FCameraBuildLog& BuildLog)
	{
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("ok"), !BuildLog.HasErrors());
		Writer->WriteValue(TEXT("status"), BuildStatusToString(Status));
		Writer->WriteArrayStart(TEXT("messages"));
		for (const FCameraBuildLogMessage& Message : BuildLog.GetMessages())
		{
			Writer->WriteValue(Message.ToString());
		}
		Writer->WriteArrayEnd();
		Writer->WriteObjectEnd();
		Writer->Close();
		return Out;
	}
}

FString UGameplayCameraSystemToolset::CreateCameraRig(const FString& AssetPath)
{
	FString Error;
	const FString Result = CreateAssetWithTools(AssetPath, UCameraRigAsset::StaticClass(), Error);
	return Result.IsEmpty() ? FailJson(TEXT("CreateCameraRig"), Error) : Result;
}

FString UGameplayCameraSystemToolset::CreateCameraAsset(const FString& AssetPath)
{
	FString Error;
	const FString Result = CreateAssetWithTools(AssetPath, UCameraAsset::StaticClass(), Error);
	return Result.IsEmpty() ? FailJson(TEXT("CreateCameraAsset"), Error) : Result;
}

FString UGameplayCameraSystemToolset::AddNodeToRig(const FString& RigPath, const FString& NodeClassName, const FString& ParentNodePath, const FString& InputProperty, const FString& NodeName)
{
	static const FString ToolName = TEXT("AddNodeToRig");

	UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigPath);
	if (!Rig)
	{
		return FailJson(ToolName, FString::Printf(TEXT("rig not found: '%s'"), *RigPath));
	}

	UClass* NodeClass = ResolveNodeClass(NodeClassName);
	if (!NodeClass)
	{
		return FailJson(ToolName, FString::Printf(TEXT("unknown node class '%s' (use short name, U-prefixed name, /Script/ path or display name)"), *NodeClassName));
	}

	// 先校验父节点与挂载属性，再创建节点，避免失败时留下孤儿对象
	UObject* ParentNode = nullptr;
	FProperty* AttachProp = nullptr;
	if (!ParentNodePath.IsEmpty())
	{
		ParentNode = FindObject<UObject>(nullptr, *ParentNodePath);
		if (!ParentNode)
		{
			return FailJson(ToolName, FString::Printf(TEXT("parent node not found: '%s'"), *ParentNodePath));
		}
		if (ParentNode->GetOutermost() != Rig->GetOutermost())
		{
			return FailJson(ToolName, FString::Printf(TEXT("parent node '%s' is not inside rig '%s'"), *ParentNodePath, *RigPath));
		}
		if (InputProperty.IsEmpty())
		{
			return FailJson(ToolName, TEXT("inputProperty is empty but parentNodePath is set (e.g. \"Children\" or \"InputSlot\")"));
		}

		FString Error;
		AttachProp = FindReferencePropertyChecked(ParentNode->GetClass(), FName(*InputProperty), NodeClass, Error);
		if (!AttachProp)
		{
			return FailJson(ToolName, Error);
		}
	}

	// 无冲突时保留用户给定的名字；有冲突才交给 MakeUniqueObjectName 加 _N 后缀
	const FName DesiredName = NodeName.IsEmpty() ? NodeClass->GetFName() : FName(*NodeName);
	FName FinalName = DesiredName;
	if (FindObject<UObject>(Rig, *DesiredName.ToString()))
	{
		FinalName = MakeUniqueObjectName(Rig, NodeClass, DesiredName);
	}
	UCameraNode* NewNode = NewObject<UCameraNode>(Rig, NodeClass, FinalName, RF_Transactional);

	if (ParentNode)
	{
		if (!AttachNodeToProperty(ParentNode, AttachProp, NewNode))
		{
			NewNode->MarkAsGarbage();
			return FailJson(ToolName, FString::Printf(TEXT("failed to attach node to '%s'"), *ParentNodePath));
		}
		ParentNode->PostEditChange();
	}

	Rig->MarkPackageDirty();
	return NewNode->GetPathName();
}

bool UGameplayCameraSystemToolset::SetNodeParameter(const FString& NodePath, const FString& PropertyName, const FString& JsonValue)
{
	static const FString ToolName = TEXT("SetNodeParameter");

	UObject* Node = FindObject<UObject>(nullptr, *NodePath);
	if (!Node)
	{
		return FailBool(ToolName, FString::Printf(TEXT("node not found: '%s'"), *NodePath));
	}

	// 包装成 {"PropertyName": <JsonValue>} 交给 UToolsetLibrary 反射写入
	const FString WrappedJson = FString::Printf(TEXT("{\"%s\": %s}"), *PropertyName, *JsonValue);
	const bool bOk = UToolsetLibrary::SetObjectProperties(Node, WrappedJson);
	if (bOk)
	{
		Node->PostEditChange();
		Node->MarkPackageDirty();
	}
	else
	{
		UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s failed to write '%s' on '%s' (check property name case and JSON shape)"),
			*ToolName, *PropertyName, *NodePath);
	}
	return bOk;
}

FString UGameplayCameraSystemToolset::GetNodeParameters(const FString& NodePath)
{
	static const FString ToolName = TEXT("GetNodeParameters");

	UObject* Node = FindObject<UObject>(nullptr, *NodePath);
	if (!Node)
	{
		return FailJson(ToolName, FString::Printf(TEXT("node not found: '%s'"), *NodePath));
	}

	// 只读细节面板可编辑属性：编辑器内部属性（GraphNode* 等）会让序列化整批失败
	TArray<FName> PropertyNames;
	for (TFieldIterator<FProperty> It(Node->GetClass()); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible) && !It->HasAnyPropertyFlags(CPF_Deprecated))
		{
			PropertyNames.Add(It->GetFName());
		}
	}

	FString Result = UToolsetLibrary::GetObjectProperties(Node, PropertyNames);
	if (Result.StartsWith(TEXT("{")))
	{
		return Result;
	}

	// 整批失败则逐属性降级：能读的合并，不能读的列入 unreadable
	TArray<FString> ReadableChunks;
	TArray<FString> Unreadable;
	for (const FName& PropertyName : PropertyNames)
	{
		const FString Single = UToolsetLibrary::GetObjectProperties(Node, { PropertyName });
		if (Single.StartsWith(TEXT("{")) && Single.EndsWith(TEXT("}")))
		{
			ReadableChunks.Add(Single.Mid(1, Single.Len() - 2)); // 去花括号
		}
		else
		{
			Unreadable.Add(FString::Printf(TEXT("\"%s\""), *PropertyName.ToString()));
		}
	}
	UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s: %d unreadable properties on '%s': %s"),
		*ToolName, Unreadable.Num(), *NodePath, *FString::Join(Unreadable, TEXT(", ")));

	return FString::Printf(TEXT("{%s,\"unreadable\":[%s]}"),
		*FString::Join(ReadableChunks, TEXT(",")), *FString::Join(Unreadable, TEXT(",")));
}

FString UGameplayCameraSystemToolset::GetRigGraph(const FString& RigPath)
{
	static const FString ToolName = TEXT("GetRigGraph");

	UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigPath);
	if (!Rig)
	{
		return FailJson(ToolName, FString::Printf(TEXT("rig not found: '%s'"), *RigPath));
	}

	// 从 RootNode 遍历可达节点，同时收集父子边
	TArray<UCameraNode*> Stack;
	TArray<UCameraNode*> Reachable;
	TArray<FNodeEdge> Edges;
	if (Rig->RootNode)
	{
		Stack.Add(Rig->RootNode);
	}
	while (Stack.Num() > 0)
	{
		UCameraNode* Node = Stack.Pop(EAllowShrinking::No);
		if (!Node || Reachable.Contains(Node))
		{
			continue;
		}
		Reachable.Add(Node);

		const int32 EdgeStart = Edges.Num();
		CollectNodeEdges(Node, Edges);
		for (int32 Index = EdgeStart; Index < Edges.Num(); ++Index)
		{
			if (UCameraNode* Child = Cast<UCameraNode>(FindObject<UObject>(nullptr, *Edges[Index].ChildPath)))
			{
				Stack.Add(Child);
			}
		}
	}

	// 收集未挂接的散节点（含仅创建未挂载、或已从树上摘掉的）
	TArray<UCameraNode*> Loose;
	ForEachObjectWithOuter(Rig, [&Reachable, &Loose](UObject* Obj)
	{
		// IsValid 过滤已 MarkAsGarbage、等待回收的节点
		if (UCameraNode* Node = Cast<UCameraNode>(Obj); Node && IsValid(Node) && !Reachable.Contains(Node))
		{
			Loose.Add(Node);
		}
	}, /*bIncludeNestedObjects*/ true);

	auto WriteNodeObject = [](TSharedRef<TJsonWriter<>>& Writer, const UCameraNode* Node)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("path"), Node->GetPathName());
		Writer->WriteValue(TEXT("class"), Node->GetClass()->GetName());
		Writer->WriteObjectEnd();
	};

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	Writer->WriteObjectStart();

	Writer->WriteArrayStart(TEXT("nodes"));
	for (const UCameraNode* Node : Reachable)
	{
		WriteNodeObject(Writer, Node);
	}
	Writer->WriteArrayEnd();

	Writer->WriteArrayStart(TEXT("edges"));
	for (const FNodeEdge& Edge : Edges)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("parent"), Edge.ParentPath);
		Writer->WriteValue(TEXT("child"), Edge.ChildPath);
		Writer->WriteValue(TEXT("property"), Edge.Property);
		Writer->WriteValue(TEXT("index"), Edge.Index);
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();

	Writer->WriteArrayStart(TEXT("loose"));
	for (const UCameraNode* Node : Loose)
	{
		WriteNodeObject(Writer, Node);
	}
	Writer->WriteArrayEnd();

	Writer->WriteObjectEnd();
	Writer->Close();
	return Out;
}

bool UGameplayCameraSystemToolset::SetRigRoot(const FString& RigPath, const FString& RootNodePath)
{
	static const FString ToolName = TEXT("SetRigRoot");

	UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigPath);
	if (!Rig)
	{
		return FailBool(ToolName, FString::Printf(TEXT("rig not found: '%s'"), *RigPath));
	}
	UCameraNode* RootNode = Cast<UCameraNode>(FindObject<UObject>(nullptr, *RootNodePath));
	if (!RootNode)
	{
		return FailBool(ToolName, FString::Printf(TEXT("root node not found or not a camera node: '%s'"), *RootNodePath));
	}

	Rig->RootNode = RootNode;

	FCameraBuildLog BuildLog;
	BuildLog.SetLoggingPrefix(Rig->GetName());
	FCameraBuildContext BuildContext(BuildLog, ECameraBuildReason::UserAction);
	Rig->BuildCameraRig(BuildContext);
	ForwardBuildMessages(BuildLog);

	Rig->MarkPackageDirty();
	if (BuildLog.HasErrors())
	{
		UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s: build of '%s' has errors"), *ToolName, *RigPath);
		return false;
	}
	return true;
}

bool UGameplayCameraSystemToolset::SetSingleDirector(const FString& AssetPath, const FString& RigPath)
{
	static const FString ToolName = TEXT("SetSingleDirector");

	UCameraAsset* Asset = LoadObject<UCameraAsset>(nullptr, *AssetPath);
	if (!Asset)
	{
		return FailBool(ToolName, FString::Printf(TEXT("camera asset not found: '%s'"), *AssetPath));
	}
	UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigPath);
	if (!Rig)
	{
		return FailBool(ToolName, FString::Printf(TEXT("rig not found: '%s'"), *RigPath));
	}

	// 已是 SingleCameraDirector 则复用，避免重复调用堆积孤儿 Director 子对象
	USingleCameraDirector* Director = Cast<USingleCameraDirector>(Asset->GetCameraDirector());
	if (!Director)
	{
		if (UCameraDirector* OldDirector = Asset->GetCameraDirector())
		{
			OldDirector->MarkAsGarbage();
		}
		Director = NewObject<USingleCameraDirector>(Asset, NAME_None, RF_Transactional);
		Asset->SetCameraDirector(Director);
	}
	Director->CameraRig = Rig;

	FCameraBuildLog BuildLog;
	BuildLog.SetLoggingPrefix(Asset->GetName());
	FCameraBuildContext BuildContext(BuildLog, ECameraBuildReason::UserAction);
	Asset->BuildCamera(BuildContext);
	ForwardBuildMessages(BuildLog);

	Asset->MarkPackageDirty();
	if (BuildLog.HasErrors())
	{
		UE_LOG(LogGameplayCameraSystemToolSet, Warning, TEXT("%s: build of '%s' has errors"), *ToolName, *AssetPath);
		return false;
	}
	return true;
}

bool UGameplayCameraSystemToolset::RemoveNodeFromRig(const FString& RigPath, const FString& NodePath)
{
	static const FString ToolName = TEXT("RemoveNodeFromRig");

	UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigPath);
	if (!Rig)
	{
		return FailBool(ToolName, FString::Printf(TEXT("rig not found: '%s'"), *RigPath));
	}
	UCameraNode* Target = Cast<UCameraNode>(FindObject<UObject>(nullptr, *NodePath));
	if (!Target)
	{
		return FailBool(ToolName, FString::Printf(TEXT("node not found: '%s'"), *NodePath));
	}
	if (Target->GetOutermost() != Rig->GetOutermost())
	{
		return FailBool(ToolName, FString::Printf(TEXT("node '%s' is not inside rig '%s'"), *NodePath, *RigPath));
	}

	// 从 RootNode 摘除
	bool bRemoved = false;
	if (Rig->RootNode == Target)
	{
		Rig->RootNode = nullptr;
		bRemoved = true;
	}

	// 从引用它的其他节点属性（数组/单值）摘除
	ForEachObjectWithOuter(Rig, [&Target, &bRemoved](UObject* Obj)
	{
		UCameraNode* Holder = Cast<UCameraNode>(Obj);
		if (!Holder || Holder == Target)
		{
			return;
		}
		for (TFieldIterator<FProperty> It(Holder->GetClass()); It; ++It)
		{
			if (FObjectProperty* ObjProp = CastField<FObjectProperty>(*It))
			{
				if (ObjProp->PropertyClass->IsChildOf(UCameraNode::StaticClass())
					&& ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<UObject*>(Holder)) == Target)
				{
					ObjProp->SetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<UObject*>(Holder), nullptr);
					bRemoved = true;
				}
			}
			else if (FArrayProperty* ArrProp = CastField<FArrayProperty>(*It))
			{
				FObjectProperty* Inner = CastField<FObjectProperty>(ArrProp->Inner);
				if (!Inner || !Inner->PropertyClass->IsChildOf(UCameraNode::StaticClass()))
				{
					continue;
				}
				FScriptArrayHelper Helper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(Holder));
				for (int32 Index = Helper.Num() - 1; Index >= 0; --Index)
				{
					if (Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)) == Target)
					{
						Helper.RemoveValues(Index);
						bRemoved = true;
					}
				}
			}
		}
	}, /*bIncludeNestedObjects*/ true);

	if (!bRemoved)
	{
		UE_LOG(LogGameplayCameraSystemToolSet, Log, TEXT("%s: node '%s' was not attached anywhere, deleting loose node"), *ToolName, *NodePath);
	}

	Target->MarkAsGarbage();
	Rig->MarkPackageDirty();
	return true;
}

FString UGameplayCameraSystemToolset::BuildRig(const FString& RigPath)
{
	static const FString ToolName = TEXT("BuildRig");

	UCameraRigAsset* Rig = LoadObject<UCameraRigAsset>(nullptr, *RigPath);
	if (!Rig)
	{
		return FailJson(ToolName, FString::Printf(TEXT("rig not found: '%s'"), *RigPath));
	}

	FCameraBuildLog BuildLog;
	BuildLog.SetLoggingPrefix(Rig->GetName());
	FCameraBuildContext BuildContext(BuildLog, ECameraBuildReason::UserAction);
	Rig->BuildCameraRig(BuildContext);
	ForwardBuildMessages(BuildLog);
	Rig->MarkPackageDirty();
	return BuildResultJson(Rig->GetBuildStatus(), BuildLog);
}

FString UGameplayCameraSystemToolset::BuildCameraAsset(const FString& AssetPath)
{
	static const FString ToolName = TEXT("BuildCameraAsset");

	UCameraAsset* Asset = LoadObject<UCameraAsset>(nullptr, *AssetPath);
	if (!Asset)
	{
		return FailJson(ToolName, FString::Printf(TEXT("camera asset not found: '%s'"), *AssetPath));
	}

	FCameraBuildLog BuildLog;
	BuildLog.SetLoggingPrefix(Asset->GetName());
	FCameraBuildContext BuildContext(BuildLog, ECameraBuildReason::UserAction);
	Asset->BuildCamera(BuildContext);
	ForwardBuildMessages(BuildLog);
	Asset->MarkPackageDirty();
	return BuildResultJson(Asset->GetBuildStatus(), BuildLog);
}

FString UGameplayCameraSystemToolset::GetBuildStatus(const FString& RigOrAssetPath)
{
	static const FString ToolName = TEXT("GetBuildStatus");

	UObject* Obj = LoadObject<UObject>(nullptr, *RigOrAssetPath);
	if (!Obj)
	{
		return FailJson(ToolName, FString::Printf(TEXT("asset not found: '%s'"), *RigOrAssetPath));
	}
	if (UCameraRigAsset* Rig = Cast<UCameraRigAsset>(Obj))
	{
		return FString::Printf(TEXT("{\"status\":\"%s\"}"), *BuildStatusToString(Rig->GetBuildStatus()));
	}
	if (UCameraAsset* Asset = Cast<UCameraAsset>(Obj))
	{
		return FString::Printf(TEXT("{\"status\":\"%s\"}"), *BuildStatusToString(Asset->GetBuildStatus()));
	}
	return FailJson(ToolName, FString::Printf(TEXT("'%s' is neither a camera rig nor a camera asset"), *RigOrAssetPath));
}

DEFINE_LOG_CATEGORY(LogGameplayCameraSystemToolSet);
