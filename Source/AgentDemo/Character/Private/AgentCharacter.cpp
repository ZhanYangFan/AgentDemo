// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "Core/CameraRigAsset.h"
#include "Core/CameraNode.h"
#include "Nodes/Common/BoomArmCameraNode.h"

AAgentCharacter::AAgentCharacter()
{
	// 角色胶囊
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// 第三人称八方向旋转策略：角色朝向跟随控制器（相机），不转向移动方向。
	// ABP_Unarmed 据此（bOrientRotationToMovement=false）走完整 CalculateDirection
	// 分支，播放八方向动画。
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// GameplayCameraSystem 相机组件：
	// bSetControlRotationWhenViewTarget 对应旧 bUsePawnControlRotation（鼠标→ControlRotation→相机闭环）。
	// CameraReference（相机资产）由蓝图 BP_AgentCharacter 数据配置层注入（C++ 不硬编码资产路径）。
	GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));
	GameplayCamera->SetupAttachment(RootComponent);
	GameplayCamera->bSetControlRotationWhenViewTarget = true;

	// 骨架网格/动画：数据配置层（蓝图子类 BP_AgentCharacter）负责，
	// C++ 不硬编码资产路径。此处仅对齐 Mesh 组件与胶囊的默认相对变换。
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}

void AAgentCharacter::SetSprintActive(bool bActive)
{
	GetCharacterMovement()->MaxWalkSpeed = bActive ? RunSpeed : WalkSpeed;
}

void AAgentCharacter::ZoomCamera(float WheelDelta)
{
	// 滚轮步进（每格 ~40cm），钳制在 [Min, Max]
	const float Step = 40.f;
	CurrentCameraDistance = FMath::Clamp(CurrentCameraDistance - WheelDelta * Step, MinCameraDistance, MaxCameraDistance);
	ApplyCameraZoom();
}

void AAgentCharacter::ApplyCameraZoom()
{
	UCameraRigAsset* Rig = CameraRig.LoadSynchronous();
	if (!Rig)
	{
		return;
	}

	// 查找 BoomArm 节点（按名字，缓存在私有成员）
	if (!CachedBoomArm)
	{
		// 遍历 RootNode 树找 BoomArmCameraNode
		TArray<UCameraNode*> Stack;
		if (Rig->RootNode)
		{
			Stack.Add(Rig->RootNode);
		}
		while (Stack.Num() > 0)
		{
			UCameraNode* Node = Stack.Pop(EAllowShrinking::No);
			if (!Node)
			{
				continue;
			}
			if (UBoomArmCameraNode* BoomArm = Cast<UBoomArmCameraNode>(Node))
			{
				CachedBoomArm = BoomArm;
				break;
			} 
			FCameraNodeChildrenView CameraNodeChildrenView = Node->GetChildren();
			for (const TObjectPtr<UCameraNode>& Child : CameraNodeChildrenView)
			{
				if (Child)
				{
					Stack.Add(Child);
				}
			}
		}
	}

	if (CachedBoomArm)
	{
		CachedBoomArm->BoomOffset = FVector3dCameraParameter(FVector(-CurrentCameraDistance, 0.f, CameraHeight));
		Rig->BuildCameraRig();
	}
}
