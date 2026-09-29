#pragma once

#include "CoreMinimal.h"
#include "BTWeatherTypes.generated.h"

/** 天气状态机：Clear -> Cloudy -> LightRain -> HeavyRain -> Storm -> ExtremeStorm */
UENUM(BlueprintType)
enum class EBTWeatherState : uint8
{
	Clear        UMETA(DisplayName = "Clear"),
	Cloudy       UMETA(DisplayName = "Cloudy"),
	LightRain    UMETA(DisplayName = "Light Rain"),
	HeavyRain    UMETA(DisplayName = "Heavy Rain"),
	Storm        UMETA(DisplayName = "Storm"),
	ExtremeStorm UMETA(DisplayName = "Extreme Storm")
};

/** 闪电强度等级 */
UENUM(BlueprintType)
enum class EBTLightningIntensity : uint8
{
	Weak    UMETA(DisplayName = "Weak"),
	Normal  UMETA(DisplayName = "Normal"),
	Strong  UMETA(DisplayName = "Strong"),
	Extreme UMETA(DisplayName = "Extreme")
};

/**
 * 风暴运行时参数集合。
 * 所有数值均为 0~1 归一化强度或实际物理量，由 StormManager 插值后分发给各子系统。
 */
USTRUCT(BlueprintType)
struct BLACKTIDE_API FBTStormParams
{
	GENERATED_BODY()

	/** 降雨强度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RainIntensity = 0.0f;

	/** 风力强度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindStrength = 0.0f;

	/** 浪高（米） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float WaveHeight = 1.0f;

	/** 浪速倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float WaveSpeed = 1.0f;

	/** 雾密度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogDensity = 0.0f;

	/** 闪电触发频率 0~1（控制最小/最大间隔插值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightningFrequency = 0.0f;

	/** 海面暗度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OceanDarkness = 0.0f;

	/** 浪花飞溅强度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SplashIntensity = 0.0f;

	/** 海面泡沫强度 0~1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FoamIntensity = 0.0f;

	/** 线性插值，用于天气状态平滑过渡 */
	static FBTStormParams Lerp(const FBTStormParams& A, const FBTStormParams& B, float Alpha)
	{
		FBTStormParams R;
		R.RainIntensity       = FMath::Lerp(A.RainIntensity, B.RainIntensity, Alpha);
		R.WindStrength        = FMath::Lerp(A.WindStrength, B.WindStrength, Alpha);
		R.WaveHeight          = FMath::Lerp(A.WaveHeight, B.WaveHeight, Alpha);
		R.WaveSpeed           = FMath::Lerp(A.WaveSpeed, B.WaveSpeed, Alpha);
		R.FogDensity          = FMath::Lerp(A.FogDensity, B.FogDensity, Alpha);
		R.LightningFrequency  = FMath::Lerp(A.LightningFrequency, B.LightningFrequency, Alpha);
		R.OceanDarkness       = FMath::Lerp(A.OceanDarkness, B.OceanDarkness, Alpha);
		R.SplashIntensity     = FMath::Lerp(A.SplashIntensity, B.SplashIntensity, Alpha);
		R.FoamIntensity       = FMath::Lerp(A.FoamIntensity, B.FoamIntensity, Alpha);
		return R;
	}
};
