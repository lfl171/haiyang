#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BTPlayerController.generated.h"

/**
 * BTPlayerController —— 绑定调试快捷键。
 * F1：开关 BLACK TIDE 调试面板。
 */
UCLASS()
class BLACKTIDE_API ABTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;

private:
	void OnToggleDebugPanel();
};
