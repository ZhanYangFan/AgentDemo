// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentLobbyWidget.h"

#include "AgentGameInstance.h"

void UAgentLobbyWidget::TryLogin()
{
	if (UAgentGameInstance* GI = GetGameInstance<UAgentGameInstance>())
	{
		GI->Login();
		RefreshLoginState();
	}
}

void UAgentLobbyWidget::EnterFrontier()
{
	if (UAgentGameInstance* GI = GetGameInstance<UAgentGameInstance>())
	{
		GI->EnterFrontier();
	}
}

void UAgentLobbyWidget::RefreshLoginState()
{
	if (UAgentGameInstance* GI = GetGameInstance<UAgentGameInstance>())
	{
		OnLoginStateChanged(GI->IsLoggedIn() ? GI->GetDisplayName() : FString());
	}
}
