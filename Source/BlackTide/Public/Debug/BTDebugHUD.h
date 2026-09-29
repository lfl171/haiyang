#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BTDebugHUD.generated.h"

/**
 * BT_DebugPanel —— 工程级实时调试 HUD。
 *
 * 屏幕左上角实时显示：
 *   FPS / FrameTime / 天气状态 / Rain / Wind / Wave / Fog / 船体速度与姿态
 *
 * 由 BTPlayerController 绑定 F1 开关。
 */
UCLASS()
class BLACKTIDE_API ABTDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** 开关调试面板 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleDebugPanel() { bShowDebugPanel = !bShowDebugPanel; }

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsDebugPanelVisible() const { return bShowDebugPanel; }

private:
	bool bShowDebugPanel = false;

	/** 平滑帧时间 */
	float SmoothedFrameTime = 16.6f;
};
