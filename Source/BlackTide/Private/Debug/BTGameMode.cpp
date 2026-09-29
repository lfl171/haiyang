#include "Debug/BTGameMode.h"

#include "Debug/BTDebugHUD.h"
#include "Debug/BTPlayerController.h"

ABTGameMode::ABTGameMode()
{
	HUDClass = ABTDebugHUD::StaticClass();
	PlayerControllerClass = ABTPlayerController::StaticClass();
}
