// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "AgentCharacter.h"
#include "AgentDemo.h"

AAgentPlayerController::AAgentPlayerController()
{
	bShowMouseCursor = false;
}

void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AAgentPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAgentPlayerController::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAgentPlayerController::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AAgentPlayerController::JumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAgentPlayerController::JumpEnded);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AAgentPlayerController::SprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAgentPlayerController::SprintEnded);
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AAgentPlayerController::Zoom);
	}
	else
	{
		UE_LOG(LogAgentDemo, Error, TEXT("AAgentPlayerController: 未找到 EnhancedInputComponent（需全局 Enhanced Input）"));
	}
}

void AAgentPlayerController::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (APawn* ControlledPawn = GetPawn())
	{
		// 移动方向相对相机朝向（控制器的 Yaw）。
		// Enhanced Input 约定：W=Y+（前）、S=Y-（后）、A=X-（左）、D=X+（右）
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Forward = YawRotation.RotateVector(FVector::ForwardVector);
		const FVector Right = YawRotation.RotateVector(FVector::RightVector);

		ControlledPawn->AddMovementInput(Forward, Axis.Y);
		ControlledPawn->AddMovementInput(Right, Axis.X);
	}
}

void AAgentPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);
	ClampViewPitch();
}

void AAgentPlayerController::JumpStarted()
{
	if (AAgentCharacter* ControlledCharacter = Cast<AAgentCharacter>(GetPawn()))
	{
		ControlledCharacter->Jump();
	}
}

void AAgentPlayerController::JumpEnded()
{
	if (AAgentCharacter* ControlledCharacter = Cast<AAgentCharacter>(GetPawn()))
	{
		ControlledCharacter->StopJumping();
	}
}

void AAgentPlayerController::SprintStarted()
{
	if (AAgentCharacter* ControlledCharacter = Cast<AAgentCharacter>(GetPawn()))
	{
		ControlledCharacter->SetSprintActive(true);
	}
}

void AAgentPlayerController::SprintEnded()
{
	if (AAgentCharacter* ControlledCharacter = Cast<AAgentCharacter>(GetPawn()))
	{
		ControlledCharacter->SetSprintActive(false);
	}
}

void AAgentPlayerController::Zoom(const FInputActionValue& Value)
{
	const float WheelDelta = Value.Get<float>();
	if (AAgentCharacter* ControlledCharacter = Cast<AAgentCharacter>(GetPawn()))
	{
		ControlledCharacter->ZoomCamera(WheelDelta);
	}
}

void AAgentPlayerController::ClampViewPitch()
{
	FRotator ControlRot = GetControlRotation();
	const float ClampedPitch = FMath::ClampAngle(ControlRot.Pitch, MinViewPitch, MaxViewPitch);
	if (!FMath::IsNearlyEqual(ControlRot.Pitch, ClampedPitch))
	{
		ControlRot.Pitch = ClampedPitch;
		SetControlRotation(ControlRot);
	}
}
