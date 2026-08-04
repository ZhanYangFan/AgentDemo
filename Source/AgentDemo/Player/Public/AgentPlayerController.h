// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgentPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * 项目统一 PlayerController：第三人称 WASD 移动（走/跑）+ 鼠标视角 + 跳跃。
 * 输入资产（IMC/IA）由蓝图数据配置层（BP_AgentPlayerController）注入，
 * C++ 只留空数据点，不硬编码键位。
 */
UCLASS()
class AGENTDEMO_API AAgentPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgentPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** WASD 移动（相对相机朝向；X=左右、Y=前后） */
	void Move(const struct FInputActionValue& Value);

	/** 鼠标控制视角 */
	void Look(const struct FInputActionValue& Value);

	/** 跳跃按下/松开 */
	void JumpStarted();
	void JumpEnded();

	/** 冲刺按下/松开 */
	void SprintStarted();
	void SprintEnded();

	/** 鼠标滚轮缩放相机距离 */
	void Zoom(const struct FInputActionValue& Value);

	/** 视角 Pitch 限位 */
	void ClampViewPitch();

protected:
	// ---- 输入数据配置（蓝图 BP_AgentPlayerController 注入资产，C++ 不硬编码） ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ZoomAction;

	/** 视角 Pitch 范围（第三人称肩后视角） */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MinViewPitch = -75.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MaxViewPitch = -10.f;
};
