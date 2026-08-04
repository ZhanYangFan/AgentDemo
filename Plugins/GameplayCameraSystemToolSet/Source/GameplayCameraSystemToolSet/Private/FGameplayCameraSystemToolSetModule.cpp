// Copyright AgentDemo Project. All Rights Reserved.
//
// 模块入口（引擎标准：模块 cpp 独立，参考 GASToolsets）。
// 模块实现（StartupModule/ShutdownModule）内联在 Public/FGameplayCameraSystemToolSetModule.h。

#include "FGameplayCameraSystemToolSetModule.h"

IMPLEMENT_MODULE(FGameplayCameraSystemToolSetModule, GameplayCameraSystemToolSet)


void FGameplayCameraSystemToolSetModule::StartupModule()
{
	UE_LOG(LogGameplayCameraSystemToolSet, Log, TEXT("StartupModule begin"));
	UToolsetRegistry::RegisterToolsetClass(UGameplayCameraSystemToolset::StaticClass());
	UE_LOG(LogGameplayCameraSystemToolSet, Log, TEXT("StartupModule end"));
}

void FGameplayCameraSystemToolSetModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UToolsetRegistry::UnregisterToolsetClass(UGameplayCameraSystemToolset::StaticClass());
	}
}
