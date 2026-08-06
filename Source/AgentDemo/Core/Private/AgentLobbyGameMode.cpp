// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentLobbyGameMode.h"

#include "AgentLobbyWidget.h"
#include "AgentUIManagerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"

AAgentLobbyGameMode::AAgentLobbyGameMode()
{
	// 纯 UI 壳：不生成玩家角色
	DefaultPawnClass = nullptr;
}

void AAgentLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 经 UI 管理器打开大厅主界面（输入模式由 Manager 统一接管）
	if (UAgentUIManagerSubsystem* UIManager = GetWorld()->GetGameInstance()->GetSubsystem<UAgentUIManagerSubsystem>())
	{
		LobbyWidget = Cast<UAgentLobbyWidget>(UIManager->OpenGlobalUI(LobbyWidgetClass));
	}
}
