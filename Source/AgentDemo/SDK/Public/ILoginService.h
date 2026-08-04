// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ILoginService.generated.h"

/**
 * 登录服务接口（05 分册 §9 SDK 抽象层）。
 * VS0 用 Mock 实现；VS6 换真 SDK 时替换实现类，接口不变。
 */
UINTERFACE(BlueprintType, MinimalAPI)
class ULoginService : public UInterface
{
	GENERATED_BODY()
};

class AGENTDEMO_API ILoginService
{
	GENERATED_BODY()

public:
	/** 发起登录（Platform 如 "QQ"/"WeChat"/"Mock"）。成功回调 bSuccess + 账号显示名。 */
	virtual void Login(const FString& Platform, TFunction<void(bool bSuccess, const FString& DisplayName)> OnDone) = 0;

	/** 是否已登录 */
	virtual bool IsLoggedIn() const = 0;

	/** 当前登录账号显示名 */
	virtual FString GetDisplayName() const = 0;
};
