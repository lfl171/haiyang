#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/BTWeatherTypes.h"
#include "SailComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * SailComponent —— 船帆风力驱动。
 *
 * 从 StormManager 读取 WindStrength，驱动船帆材质的
 * WindSway / Billow 参数（通过动态材质实例），
 * 同时计算船帆兜风产生的额外船体横摇力矩。
 */
UCLASS(ClassGroup = (BlackTide), meta = (BlueprintSpawnableComponent))
class BLACKTIDE_API USailComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USailComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 需要驱动的帆网格（会在其上创建动态材质实例） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail")
	TArray<TObjectPtr<UStaticMeshComponent>> SailMeshes;

	/** 风对帆摆动的增益 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail", meta = (ClampMin = "0.0"))
	float WindSwayGain = 1.0f;

	/** 帆面鼓风程度增益 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail", meta = (ClampMin = "0.0"))
	float BillowGain = 1.0f;

	/** 兜风造成的额外横摇角增益（度/单位风力） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sail")
	float HeelMomentGain = 6.0f;

private:
	UFUNCTION()
	void OnStormParamsReceived(const FBTStormParams& Params);

	float CurrentWind = 0.0f;
	float SailPhase = 0.0f;
	TArray<UMaterialInstanceDynamic*> DynamicMats;
};
