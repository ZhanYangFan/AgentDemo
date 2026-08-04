// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentLobbyGameMode.h"

#include "AgentLobbyWidget.h"
#include "Blueprint/UserWidget.h"

AAgentLobbyGameMode::AAgentLobbyGameMode()
{
	// 纯 UI 壳：不生成玩家角色
	DefaultPawnClass = nullptr;
}

void AAgentLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (LobbyWidgetClass)
	{
		LobbyWidget = CreateWidget<UAgentLobbyWidget>(GetWorld(), LobbyWidgetClass);
		if (LobbyWidget)
		{
			LobbyWidget->AddToViewport(0);
		}
	}
}
