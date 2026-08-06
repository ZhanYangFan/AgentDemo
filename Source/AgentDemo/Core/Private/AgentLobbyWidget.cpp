// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentLobbyWidget.h"

#include "AgentGameInstance.h"
#include "AgentUIManagerSubsystem.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

void UAgentLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &UAgentLobbyWidget::HandleLoginClicked);
	}
	if (EnterGameButton)
	{
		EnterGameButton->OnClicked.AddDynamic(this, &UAgentLobbyWidget::HandleEnterGameClicked);
	}

	// 构造时按当前登录态初始化（例如从小镇返回大厅时已是已登录态）
	RefreshLoginState();
}

void UAgentLobbyWidget::TryLogin()
{
	UAgentGameInstance* GI = GetGameInstance<UAgentGameInstance>();
	if (!GI)
	{
		return;
	}

	const FString Account = AccountInput ? AccountInput->GetText().ToString().TrimStartAndEnd() : FString();
	if (Account.IsEmpty())
	{
		// 轻量反馈：账号为空不发起登录
		if (LoginStatusText)
		{
			LoginStatusText->SetText(FText::FromString(TEXT("请输入账号")));
		}
		return;
	}

	// 密码 VS0 不校验（Mock 服务忽略），仅保留输入交互
	GI->Login(Account);
	RefreshLoginState();
}

void UAgentLobbyWidget::EnterFrontier()
{
	if (UAgentGameInstance* GI = GetGameInstance<UAgentGameInstance>())
	{
		// 经 UI 管理器关闭大厅（移除 UI + 恢复默认输入模式），再切到小镇
		if (UAgentUIManagerSubsystem* UIManager = GI->GetSubsystem<UAgentUIManagerSubsystem>())
		{
			UIManager->CloseGlobalUI();
		}
		GI->EnterFrontier();
	}
}

void UAgentLobbyWidget::RefreshLoginState()
{
	const UAgentGameInstance* GI = GetGameInstance<UAgentGameInstance>();
	const bool bLoggedIn = GI && GI->IsLoggedIn();

	if (StateSwitcher)
	{
		StateSwitcher->SetActiveWidgetIndex(bLoggedIn ? 1 : 0);
	}

	if (LoginStatusText)
	{
		LoginStatusText->SetText(FText::FromString(bLoggedIn ? TEXT("✓已登录") : TEXT("未登录")));
	}

	if (bLoggedIn && WelcomeText && GI)
	{
		WelcomeText->SetText(FText::Format(
			NSLOCTEXT("Lobby", "WelcomeFormat", "欢迎回来，{0}"),
			FText::FromString(GI->GetDisplayName())));
	}
}

void UAgentLobbyWidget::HandleLoginClicked()
{
	TryLogin();
}

void UAgentLobbyWidget::HandleEnterGameClicked()
{
	EnterFrontier();
}
