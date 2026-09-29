#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/BTWeatherTypes.h"
#include "WetnessComponent.generated.h"

class UMeshComponent;
class UMaterialInstanceDynamic;

/**
 * WetnessComponent —— 船体湿润系统。
 *
 * 暴雨场景的核心质感来源：
 *   干木头 -> 淋雨 -> 湿木头（颜色更深 + Roughness 降低 + 高光增强）-> 表面水膜
 *
 * 湿润度随降雨强度累积、雨停后缓慢干燥，
 * 通过动态材质实例写入 Wetness / WaterFilm 参数。
 */
UCLASS(ClassGroup = (BlackTide), meta = (BlueprintSpawnableComponent))
class BLACKTIDE_API UWetnessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWetnessComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 湿润速度（满雨时从全干到全湿的秒数倒数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.0"))
	float WettingSpeed = 0.05f;

	/** 干燥速度（远慢于湿润） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.0"))
	float DryingSpeed = 0.005f;

	/** 表面水膜出现的湿润度阈值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WaterFilmThreshold = 0.6f;

	/** 当前湿润度 0~1 */
	UPROPERTY(BlueprintReadOnly, Category = "Wetness")
	float Wetness = 0.0f;

private:
	UFUNCTION()
	void OnStormParamsReceived(const FBTStormParams& Params);

	float CurrentRain = 0.0f;
	TArray<UMaterialInstanceDynamic*> DynamicMats;
};
