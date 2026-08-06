// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

UUserWidget* UAgentUIManagerSubsystem::OpenGlobalUI(TSubclassOf<UUserWidget> WidgetClass)
{
	// 幂等：已有全局 UI 时直接返回，不重复创建
	if (GlobalUI)
	{
		return GlobalUI;
	}
	if (!WidgetClass)
	{
		return nullptr;
	}

	GlobalUI = CreateWidget<UUserWidget>(GetGameInstance(), WidgetClass);
	if (GlobalUI)
	{
		GlobalUI->AddToViewport(0);
		ApplyInputMode(GlobalUIInputMode);
	}
	return GlobalUI;
}

void UAgentUIManagerSubsystem::CloseGlobalUI()
{
	if (GlobalUI)
	{
		GlobalUI->RemoveFromParent();
		GlobalUI = nullptr;
	}
	ApplyInputMode(DefaultInputMode);
}

void UAgentUIManagerSubsystem::ApplyInputMode(EAgentUIInputMode Mode)
{
	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	switch (Mode)
	{
	case EAgentUIInputMode::GameOnly:
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
		break;

	case EAgentUIInputMode::GameAndUI:
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		break;
	}

	case EAgentUIInputMode::UIOnly:
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		break;
	}
	}
}
