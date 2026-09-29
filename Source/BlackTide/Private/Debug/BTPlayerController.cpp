#include "Debug/BTPlayerController.h"

#include "Debug/BTDebugHUD.h"

void ABTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ABTPlayerController::OnToggleDebugPanel);
	}
}

void ABTPlayerController::OnToggleDebugPanel()
{
	if (ABTDebugHUD* HUD = Cast<ABTDebugHUD>(GetHUD()))
	{
		HUD->ToggleDebugPanel();
	}
}
