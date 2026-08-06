// Copyright AgentDemo Project. All Rights Reserved.

#include "MockLoginService.h"

void UMockLoginService::Login(const FString& Platform, const FString& Account, TFunction<void(bool bSuccess, const FString& DisplayName)> OnDone)
{
	// Mock：始终成功，回显输入的账号名（空账号仅兜底防御，UI 层已拦截）
	bLoggedIn = true;
	DisplayName = Account.IsEmpty() ? TEXT("Mock_Player") : Account;

	if (OnDone)
	{
		OnDone(true, DisplayName);
	}
}

bool UMockLoginService::IsLoggedIn() const
{
	return bLoggedIn;
}

FString UMockLoginService::GetDisplayName() const
{
	return DisplayName;
}
