// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AgentGameState.generated.h"

/**
 * 项目统一 GameState：跨客户端共享的会话状态。
 * 预留：Boss 引用、进度旗标（Flags）等 VS2+ 内容。
 */
UCLASS()
class AGENTDEMO_API AAgentGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AAgentGameState();
};
