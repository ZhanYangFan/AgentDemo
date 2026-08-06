// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "AgentUIManagerSubsystem.generated.h"

class UUserWidget;

/** UI 输入模式策略 */
UENUM()
enum class EAgentUIInputMode : uint8
{
	/** 游戏态：输入全归游戏，隐藏鼠标（默认模式） */
	GameOnly,
	/** 全局 UI 打开：UI 可点、游戏输入不被吞（即使恢复失败也不破坏游戏输入） */
	GameAndUI,
	/** UI 独占：仅留给真正需要屏蔽游戏输入的模态场景 */
	UIOnly,
};

/**
 * UI 管理器：统一管理全局 UI 的打开/关闭与输入模式切换。
 * 生命周期跟随 GameInstance（跨关卡存活，大厅→小镇无缝）。
 *
 * 输入模式约定：DefaultInputMode 为游戏态默认（GameOnly）；
 * 打开全局 UI 切到 GlobalUIInputMode（GameAndUI——UMG 点击不需要独占输入，
 * 且即使恢复失败也不会吞掉游戏输入）；关闭全局 UI 恢复默认。
 */
UCLASS()
class AGENTDEMO_API UAgentUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 打开全局 UI（幂等：已打开则直接返回现有实例）。
	 * 创建 → AddToViewport → 切到 GlobalUIInputMode。
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* OpenGlobalUI(TSubclassOf<UUserWidget> WidgetClass);

	/** 关闭全局 UI：RemoveFromParent → 恢复 DefaultInputMode */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseGlobalUI();

	/** 当前打开的全局 UI（无则 nullptr） */
	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetGlobalUI() const { return GlobalUI; }

protected:
	/** 应用输入模式到本地 PlayerController（连带鼠标显隐） */
	void ApplyInputMode(EAgentUIInputMode Mode);

	/** 游戏态默认输入模式（游戏开局即此约定，全局 UI 关闭时恢复） */
	EAgentUIInputMode DefaultInputMode = EAgentUIInputMode::GameOnly;

	/** 全局 UI 打开时的输入模式 */
	EAgentUIInputMode GlobalUIInputMode = EAgentUIInputMode::GameAndUI;

	/** 当前打开的全局 UI */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> GlobalUI;
};
