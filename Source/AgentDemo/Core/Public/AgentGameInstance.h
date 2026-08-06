// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AgentGameInstance.generated.h"

class UMockLoginService;

/**
 * 项目统一 GameInstance：跨关卡状态载体（登录态、预留货币/战令等存盘字段）。
 * VS0 承载登录态与大厅→小镇切换；VS6 填充货币/战令/公会数据。
 */
UCLASS()
class AGENTDEMO_API UAgentGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 发起登录（Mock），Account 为输入的账号名 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void Login(const FString& Account);

	/** 是否已登录 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsLoggedIn() const;

	/** 当前登录账号显示名 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	FString GetDisplayName() const;

	/** 进入游戏（切换到小镇 M_Town_Dev） */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void EnterFrontier();

protected:
	/** 登录服务（VS0 Mock，VS6 换真 SDK 实现） */
	UPROPERTY()
	TObjectPtr<UMockLoginService> LoginService;
};
