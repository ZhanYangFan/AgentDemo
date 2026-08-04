// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AgentHUD.generated.h"

/**
 * 项目统一 HUD：游戏内 UI 宿主。
 * 预留：VS1 属性条、技能 CD、任务追踪等 UMG 挂载点。
 */
UCLASS()
class AGENTDEMO_API AAgentHUD : public AHUD
{
	GENERATED_BODY()

public:
	AAgentHUD();
};
