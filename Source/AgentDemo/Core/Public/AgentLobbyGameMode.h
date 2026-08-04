// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgentLobbyGameMode.generated.h"

class UAgentLobbyWidget;

/**
 * 大厅 GameMode：纯 UI 壳（不 spawn 玩家角色）。
 * BeginPlay 创建 WBP_LobbyMain 并切到 UI 输入模式。
 */
UCLASS()
class AGENTDEMO_API AAgentLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgentLobbyGameMode();

	virtual void BeginPlay() override;

protected:
	/** 大厅主界面 WBP（蓝图数据配置层指定 WBP_LobbyMain） */
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TSubclassOf<UAgentLobbyWidget> LobbyWidgetClass;

	/** 当前大厅 Widget */
	UPROPERTY(Transient)
	TObjectPtr<UAgentLobbyWidget> LobbyWidget;
};
