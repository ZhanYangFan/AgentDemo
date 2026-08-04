// Copyright AgentDemo Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AgentCharacter.generated.h"

class UGameplayCameraComponent;
class UCameraRigAsset;
class UBoomArmCameraNode;

/**
 * 项目统一玩家角色：第三人称跟随相机（GameplayCameraSystem）+ WASD 移动（八方向动画）。
 * 相机由 UGameplayCameraComponent 驱动（CameraReference 数据点由蓝图 BP_AgentCharacter
 * 指向 CA_ThirdPerson），C++ 不硬编码资产路径。
 * 角色朝向跟随控制器（bUseControllerRotationYaw），动画由 ABP_Unarmed 自驱动。
 */
UCLASS()
class AGENTDEMO_API AAgentCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAgentCharacter();

protected:
	/** GameplayCameraSystem 相机组件（数据配置层注入 CameraReference 资产） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGameplayCameraComponent> GameplayCamera;

public:
	/** Returns the gameplay camera component **/
	UGameplayCameraComponent* GetGameplayCamera() const { return GameplayCamera.Get(); }

	/** 切换冲刺状态（切换 MaxWalkSpeed 在走/跑速度之间） */
	void SetSprintActive(bool bActive);

	/** 鼠标滚轮缩放相机距离（WheelDelta>0 拉近，<0 拉远） */
	void ZoomCamera(float WheelDelta);

protected:
	/** 行走速度（cm/s） */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 240.f;

	/** 冲刺速度（cm/s） */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 600.f;

	// ---- 相机缩放（数据配置：蓝图 BP_AgentCharacter 注入 rig 引用） ----

	/** 相机 CameraRig（运行时修改 BoomOffset 实现缩放） */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UCameraRigAsset> CameraRig;

	/** 最远相机距离（cm） */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = 0))
	float MaxCameraDistance = 700.f;

	/** 最近相机距离（第一人称，cm） */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = 0))
	float MinCameraDistance = 40.f;

	/** 相机高度（BoomOffset Z） */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraHeight = 100.f;

	/** 当前缩放距离（cm） */
	float CurrentCameraDistance = 700.f;

private:
	/** 缓存的 BoomArm 节点（第一次缩放时查找） */
	UPROPERTY(Transient)
	TObjectPtr<UBoomArmCameraNode> CachedBoomArm;

	/** 查找并应用当前缩放距离到 rig */
	void ApplyCameraZoom();
};
