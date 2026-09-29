#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BTGameMode.generated.h"

/**
 * BTGameMode —— BLACK TIDE 默认 GameMode。
 * 装配调试 HUD 与调试 PlayerController。
 */
UCLASS()
class BLACKTIDE_API ABTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABTGameMode();
};
