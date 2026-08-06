// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgentLobbyWidget.generated.h"

class UTextBlock;
class UButton;
class UWidgetSwitcher;
class UEditableTextBox;

/**
 * 大厅主界面基类：登录 + 进入游戏（逻辑在 C++，布局由 WBP 蓝图数据配置）。
 * WBP_LobbyMain 继承此类。未登录/已登录两态由 StateSwitcher 切换：
 * Child 0 = 登录面板，Child 1 = 已登录面板。
 */
UCLASS()
class AGENTDEMO_API UAgentLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** 点击「登入」：读取账号输入，发起 Mock 登录并刷新界面 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void TryLogin();

	/** 点击「进入游戏」：切换到小镇 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void EnterFrontier();

protected:
	/** 按 GameInstance 登录态直刷界面（切面板、状态标签、欢迎文本） */
	void RefreshLoginState();

	// ---- 公共区（两态共享） ----

	/** 顶部标题 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** 右上角状态标签（未登录 / ✓已登录 / 请输入账号） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LoginStatusText;

	/** 两态切换器：Child 0 = 登录面板，Child 1 = 已登录面板 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> StateSwitcher;

	// ---- 未登录面板 ----

	/** 账号输入框 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> AccountInput;

	/** 密码输入框（Mock 不校验，仅保留交互） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> PasswordInput;

	/** 「登入」按钮 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LoginButton;

	// ---- 已登录面板 ----

	/** 欢迎文本（欢迎回来，{账号}） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WelcomeText;

	/** 「进入游戏」按钮 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EnterGameButton;

private:
	UFUNCTION()
	void HandleLoginClicked();

	UFUNCTION()
	void HandleEnterGameClicked();
};
