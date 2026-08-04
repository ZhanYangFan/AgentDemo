// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgentLobbyWidget.generated.h"

/**
 * 大厅主界面基类：登录 + 进入游戏（逻辑在 C++，布局由 WBP 蓝图数据配置）。
 * WBP_LobbyMain 继承此类，按钮事件调用 TryLogin/EnterFrontier。
 */
UCLASS()
class AGENTDEMO_API UAgentLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 点击「登录」：发起 Mock 登录并刷新登录状态显示 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void TryLogin();

	/** 点击「进入游戏」：切换到小镇 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void EnterFrontier();

	/** 登录状态变化时通知蓝图（刷新账号名文本等） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnLoginStateChanged(const FString& DisplayName);

protected:
	/** 刷新登录状态（调用 OnLoginStateChanged） */
	void RefreshLoginState();
};
