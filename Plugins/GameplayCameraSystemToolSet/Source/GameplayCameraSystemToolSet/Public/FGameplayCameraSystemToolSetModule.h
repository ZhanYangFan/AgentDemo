// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "GameplayCameraSystemToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"


// ===== 模块入口 =====




/**
 * 模块实现：注册/反注册工具集到 ToolsetRegistry。
 * 实现内联在头文件（小模块惯例）；IMPLEMENT_MODULE 在 GameplayCameraTools.cpp。
 */
class FGameplayCameraSystemToolSetModule : public IModuleInterface
{
public:
	virtual void StartupModule() override ;

	virtual void ShutdownModule() override;
	
};
