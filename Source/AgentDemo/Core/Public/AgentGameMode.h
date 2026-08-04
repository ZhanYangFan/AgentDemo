// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AgentGameMode.generated.h"

/**
 * 项目统一 GameMode：以 C++ 类直接指定框架链（无蓝图依赖）。
 * 后续游戏规则（关卡流转、重生、权威校验）挂在此类。
 */
UCLASS()
class AGENTDEMO_API AAgentGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAgentGameMode();
};
