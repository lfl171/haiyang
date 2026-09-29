#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/BTWeatherTypes.h"
#include "BTOceanManager.generated.h"

/** 单层 Gerstner 波参数 */
USTRUCT(BlueprintType)
struct BLACKTIDE_API FBTWaveLayer
{
	GENERATED_BODY()

	/** 传播方向（XY 平面，自动归一化） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Direction = FVector2D(1.0f, 0.0f);

	/** 振幅（米），运行时由 WaveHeight 缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Amplitude = 1.0f;

	/** 波长（米） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0"))
	float Wavelength = 60.0f;

	/** 速度倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Speed = 1.0f;

	/** 波峰尖锐度 0~1（0 = 正弦波） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Steepness = 0.5f;
};

/**
 * BT_OceanManager —— 海洋系统总控。
 *
 * 三层频率结构：
 *   Large（大尺度涌浪）-> Medium（明显波峰）-> Detail（细碎水面）
 *
 * CPU 侧持有与材质完全一致的 Gerstner 参数，
 * 通过 GetWaterHeightAt() 向船体浮力/水花系统提供水面高度采样。
 * 材质侧（M_Ocean）从 MPC 读取同一组参数做顶点位移，保证视觉与物理一致。
 */
UCLASS(BlueprintType)
class BLACKTIDE_API ABTOceanManager : public AActor
{
	GENERATED_BODY()

public:
	ABTOceanManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** 获取世界中的 OceanManager 实例 */
	UFUNCTION(BlueprintCallable, Category = "Ocean", meta = (WorldContext = "WorldContextObject"))
	static ABTOceanManager* Get(const UObject* WorldContextObject);

	/** 采样指定世界坐标在当前时刻的水面高度（含全部波层叠加） */
	UFUNCTION(BlueprintCallable, Category = "Ocean")
	float GetWaterHeightAt(const FVector& WorldLocation) const;

	/** 采样水面法线（用于船体姿态对齐） */
	UFUNCTION(BlueprintCallable, Category = "Ocean")
	FVector GetWaterNormalAt(const FVector& WorldLocation) const;

	/** 海面基准高度（无波时的 Z） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean")
	float SeaLevel = 0.0f;

	/** 大尺度涌浪层（远处主导） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean|Waves")
	FBTWaveLayer LargeWave;

	/** 中尺度波峰层（中景主导） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean|Waves")
	FBTWaveLayer MediumWave;

	/** 细碎波层（近处细节） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean|Waves")
	FBTWaveLayer DetailWave;

	/** 当前浪高倍率（由 StormManager 驱动） */
	UPROPERTY(BlueprintReadOnly, Category = "Ocean")
	float WaveHeightScale = 1.0f;

	/** 当前浪速倍率（由 StormManager 驱动） */
	UPROPERTY(BlueprintReadOnly, Category = "Ocean")
	float WaveSpeedScale = 1.0f;

private:
	UFUNCTION()
	void OnStormParamsReceived(const FBTStormParams& Params);

	/** 计算单层波对指定点的高度贡献 */
	float SampleWaveLayer(const FBTWaveLayer& Layer, const FVector2D& Pos, float Time) const;

	float OceanTime = 0.0f;
};
