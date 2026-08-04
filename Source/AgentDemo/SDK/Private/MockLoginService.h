// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ILoginService.h"
#include "MockLoginService.generated.h"

/**
 * Mock 登录服务：模拟登录成功（账号名 "Mock_Player"），无真实 SDK。
 * VS6 替换为真 SDK 实现。
 */
UCLASS()
class UMockLoginService : public UObject, public ILoginService
{
	GENERATED_BODY()

public:
	virtual void Login(const FString& Platform, TFunction<void(bool bSuccess, const FString& DisplayName)> OnDone) override;
	virtual bool IsLoggedIn() const override;
	virtual FString GetDisplayName() const override;

private:
	bool bLoggedIn = false;
	FString DisplayName;
};
