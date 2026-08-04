// Copyright AgentDemo Project. All Rights Reserved.

#include "AgentGameMode.h"

#include "AgentCharacter.h"
#include "AgentGameState.h"
#include "AgentHUD.h"
#include "AgentPlayerController.h"
#include "AgentPlayerState.h"

AAgentGameMode::AAgentGameMode()
{
	DefaultPawnClass = AAgentCharacter::StaticClass();
	PlayerControllerClass = AAgentPlayerController::StaticClass();
	GameStateClass = AAgentGameState::StaticClass();
	PlayerStateClass = AAgentPlayerState::StaticClass();
	HUDClass = AAgentHUD::StaticClass();
}
