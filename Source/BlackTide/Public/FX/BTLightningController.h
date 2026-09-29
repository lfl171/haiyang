#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/BTWeatherTypes.h"
#include "BTLightningController.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UPostProcessComponent;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBTOnLightningStrike, EBTLightningIntensity, Intensity);

/**
 * BT_LightningController —— 闪电系统。
 *
 * 闪电不是单点光源，而是一条完整事件链：
 *   定时触发 -> 天空光/平行光爆闪 -> 雾/后处理响应 -> 海面反射 -> 延迟雷声
 *
 * 闪光曲线（强烈闪电）：
 *   0.00s 黑暗 / 0.03s 爆闪 / 0.08s 暗下 / 0.15s 二次闪 / 0.30s 恢复
 */
UCLASS(BlueprintType)
class BLACKTIDE_API ABTLightningController : public AActor
{
	GENERATED_BODY()

public:
	ABTLightningController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** 手动触发一次闪电（电影镜头用） */
	UFUNCTION(BlueprintCallable, Category = "Lightning")
	void TriggerLightning(EBTLightningIntensity Intensity = EBTLightningIntensity::Normal);

	/** 闪电触发广播（音效/镜头抖动可订阅） */
	UPROPERTY(BlueprintAssignable, Category = "Lightning")
	FBTOnLightningStrike OnLightningStrike;

protected:
	/** 天空中的闪电主光（默认禁用，仅在闪光瞬间启用） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lightning")
	TObjectPtr<UDirectionalLightComponent> LightningLight;

	/** 闪电期间增强天光反弹 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lightning")
	TObjectPtr<USkyLightComponent> FlashSkyLight;

	/** 闪电 Niagara 特效（天空电弧，可选） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning|FX")
	TObjectPtr<UNiagaraSystem> LightningFX;

	/** 各强度对应的雷声 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning|Audio")
	TObjectPtr<USoundBase> ThunderWeak;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning|Audio")
	TObjectPtr<USoundBase> ThunderStrong;

	/** 闪光基础强度（lux） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning")
	float BaseFlashIntensity = 100000.0f;

	/** 雷声最小/最大延迟（模拟光速远快于声速） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning|Audio")
	float ThunderDelayMin = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning|Audio")
	float ThunderDelayMax = 3.5f;

private:
	/** 接收 StormManager 参数广播 */
	UFUNCTION()
	void OnStormParamsReceived(const FBTStormParams& Params);

	/** 根据当前天气参数重掷下次闪电时间 */
	void ScheduleNextStrike();
	/** 播放多段闪光曲线 */
	void PlayFlashCurve(float DeltaTime);
	float GetIntensityMultiplier(EBTLightningIntensity Intensity) const;
	void PlayThunder(EBTLightningIntensity Intensity);

	float NextStrikeTime = 10.0f;
	float ElapsedTime = 0.0f;

	/** 闪光曲线播放状态 */
	bool bFlashing = false;
	float FlashClock = 0.0f;
	float FlashPeak = 0.0f;
	EBTLightningIntensity CurrentStrikeIntensity = EBTLightningIntensity::Normal;
	float CurrentLightningFrequency = 0.0f;
	float StrikeIntervalMin = 4.0f;
	float StrikeIntervalMax = 18.0f;
};
