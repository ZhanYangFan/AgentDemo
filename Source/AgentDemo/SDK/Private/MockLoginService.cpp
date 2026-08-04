// Copyright AgentDemo Project. All Rights Reserved.

#include "MockLoginService.h"

void UMockLoginService::Login(const FString& Platform, TFunction<void(bool bSuccess, const FString& DisplayName)> OnDone)
{
	// Mock：始终成功，账号名固定
	bLoggedIn = true;
	DisplayName = TEXT("Mock_Player");

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
