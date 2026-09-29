#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipMovementComponent.generated.h"

class ABTOceanManager;

/**
 * ShipMovementComponent —— 船体浮力与姿态。
 *
 * 不用简单的 Sin(Time) 假摆动，而是在船底布置多个采样点，
 * 每帧向 OceanManager 查询各点水面高度，合成：
 *   上下浮动（Heave）/ 前后倾斜（Pitch）/ 左右摇摆（Roll）/ 轻微偏航（Yaw）
 * 不同船体区域受不同波浪影响，大船体感自然。
 */
UCLASS(ClassGroup = (BlackTide), meta = (BlueprintSpawnableComponent))
class BLACKTIDE_API UShipMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 船底浮力采样点（相对船体原点，单位 cm）。至少 3 点才能稳定解算姿态 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	TArray<FVector> BuoyancyProbeOffsets;

	/** 姿态跟随水面的平滑速度（越大越僵硬，越小越肉） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy", meta = (ClampMin = "0.1"))
	float HeaveDampSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy", meta = (ClampMin = "0.1"))
	float PitchRollDampSpeed = 2.0f;

	/** 船体受波影响的幅度 0~1（0 = 完全不受浪影响） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WaveInfluence = 1.0f;

	/** 当前船体姿态（供 Debug 面板读取） */
	UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
	float CurrentPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Buoyancy")
	float CurrentRoll = 0.0f;

private:
	ABTOceanManager* Ocean = nullptr;
};
