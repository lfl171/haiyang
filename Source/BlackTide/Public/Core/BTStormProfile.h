#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/BTWeatherTypes.h"
#include "BTStormProfile.generated.h"

/**
 * DA_StormProfile —— 整个 BLACK TIDE 的参数驱动核心。
 * 每种天气一份资产（DA_Calm / DA_LightRain / DA_Storm / DA_Hurricane...），
 * StormManager 读取本资产后统一分发到 Rain/Wind/Ocean/Fog/Lightning 各子系统。
 */
UCLASS(BlueprintType)
class BLACKTIDE_API UBTStormProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 该 Profile 对应的天气状态 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm")
	EBTWeatherState WeatherState = EBTWeatherState::Storm;

	/** 风暴基础参数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm")
	FBTStormParams Params;

	/** 闪电最小触发间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning", meta = (ClampMin = "0.1"))
	float LightningMinInterval = 4.0f;

	/** 闪电最大触发间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lightning", meta = (ClampMin = "0.1"))
	float LightningMaxInterval = 18.0f;

	/** 切换到本状态所需的过渡时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm", meta = (ClampMin = "0.0"))
	float TransitionDuration = 8.0f;
};
