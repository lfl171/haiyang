#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/BTWeatherTypes.h"
#include "ShipFXComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * ShipFXComponent —— 船体特效控制。
 *
 * 管理船艏浪花（NS_BowSplash）、船迹泡沫（NS_ShipFoam），
 * 根据船速与浪强实时调整 Niagara 发射参数。
 */
UCLASS(ClassGroup = (BlackTide), meta = (BlueprintSpawnableComponent))
class BLACKTIDE_API UShipFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipFXComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 船艏浪花特效资产 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	TObjectPtr<UNiagaraSystem> BowSplashSystem;

	/** 船迹泡沫特效资产 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	TObjectPtr<UNiagaraSystem> WakeFoamSystem;

	/** 船艏浪花挂点（相对船体） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FVector BowOffset = FVector(1800.0f, 0.0f, -100.0f);

	/** 产生浪花的最低船速（cm/s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	float MinSpeedForSplash = 100.0f;

private:
	UFUNCTION()
	void OnStormParamsReceived(const FBTStormParams& Params);

	float CurrentSplashIntensity = 0.0f;
	float CurrentFoamIntensity = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BowSplashComp;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> WakeFoamComp;
};
