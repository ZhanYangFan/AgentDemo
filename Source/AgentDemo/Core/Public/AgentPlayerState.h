// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AgentPlayerState.generated.h"

/**
 * 项目统一 PlayerState：跨 Pawn 存活的玩家数据。
 * 预留：VS1 的 AbilitySystemComponent 挂载点（随 PlayerState 存活）。
 */
UCLASS()
class AGENTDEMO_API AAgentPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AAgentPlayerState();
};
