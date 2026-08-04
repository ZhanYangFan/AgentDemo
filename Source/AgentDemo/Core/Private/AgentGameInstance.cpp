// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "MockLoginService.h"

void UAgentGameInstance::Login()
{
	if (!LoginService)
	{
		LoginService = NewObject<UMockLoginService>(this);
	}

	LoginService->Login(TEXT("Mock"), [this](bool bSuccess, const FString& DisplayName)
	{
		// 登录态已写入 LoginService；UI 侧通过 GetDisplayName/IsLoggedIn 读取
	});
}

bool UAgentGameInstance::IsLoggedIn() const
{
	return LoginService && LoginService->IsLoggedIn();
}

FString UAgentGameInstance::GetDisplayName() const
{
	return LoginService ? LoginService->GetDisplayName() : FString();
}

void UAgentGameInstance::EnterFrontier()
{
	UGameplayStatics::OpenLevel(this, TEXT("M_Town_Dev"));
}
