#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/BTWeatherTypes.h"
#include "BTStormManager.generated.h"

class UBTStormProfile;
class UMaterialParameterCollection;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBTOnStormParamsUpdated, const FBTStormParams&, Params);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBTOnWeatherStateChanged, EBTWeatherState, OldState, EBTWeatherState, NewState);

/**
 * BT_StormManager —— 天气系统总控。
 *
 * 职责：
 *  1. 持有各天气状态对应的 StormProfile（DataAsset 驱动，参数不写死）
 *  2. 状态切换时对参数做平滑插值过渡
 *  3. 每帧将插值后的 FBTStormParams 广播给各子系统（Rain/Wind/Ocean/Fog/Lightning）
 *  4. 同步写入全局材质参数集（MPC），供海洋/湿润材质读取
 *
 * 关卡中只应存在一个实例，可通过 Get(World) 静态获取。
 */
UCLASS(BlueprintType)
class BLACKTIDE_API ABTStormManager : public AActor
{
	GENERATED_BODY()

public:
	ABTStormManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** 获取世界中的 StormManager 实例 */
	UFUNCTION(BlueprintCallable, Category = "Storm", meta = (WorldContext = "WorldContextObject"))
	static ABTStormManager* Get(const UObject* WorldContextObject);

	/** 请求切换天气状态（带过渡） */
	UFUNCTION(BlueprintCallable, Category = "Storm")
	void SetWeatherState(EBTWeatherState NewState);

	/** 立即切换天气状态（无过渡，用于镜头跳切） */
	UFUNCTION(BlueprintCallable, Category = "Storm")
	void SetWeatherStateImmediate(EBTWeatherState NewState);

	/** 当前插值后的实时参数 */
	UFUNCTION(BlueprintPure, Category = "Storm")
	const FBTStormParams& GetCurrentParams() const { return CurrentParams; }

	UFUNCTION(BlueprintPure, Category = "Storm")
	EBTWeatherState GetWeatherState() const { return CurrentState; }

	/** 是否正在过渡中 */
	UFUNCTION(BlueprintPure, Category = "Storm")
	bool IsTransitioning() const { return TransitionAlpha < 1.0f; }

	/** 参数更新广播（子系统订阅） */
	UPROPERTY(BlueprintAssignable, Category = "Storm")
	FBTOnStormParamsUpdated OnStormParamsUpdated;

	/** 天气状态切换广播 */
	UPROPERTY(BlueprintAssignable, Category = "Storm")
	FBTOnWeatherStateChanged OnWeatherStateChanged;

protected:
	/** 天气状态 -> Profile 映射。在编辑器中配置 DA_Calm/DA_Storm/... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Config")
	TMap<EBTWeatherState, TObjectPtr<UBTStormProfile>> Profiles;

	/** 启动时的默认天气 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Config")
	EBTWeatherState InitialState = EBTWeatherState::Storm;

	/** 全局材质参数集（含 WindStrength/Wetness/FoamIntensity 等标量） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storm|Config")
	TObjectPtr<UMaterialParameterCollection> GlobalMPC;

private:
	void ApplyParamsToMPC() const;
	const UBTStormProfile* FindProfile(EBTWeatherState State) const;

	EBTWeatherState CurrentState = EBTWeatherState::Storm;
	EBTWeatherState TargetState = EBTWeatherState::Storm;

	FBTStormParams CurrentParams;
	FBTStormParams TransitionStartParams;

	float TransitionAlpha = 1.0f;
	float TransitionDuration = 1.0f;
	float CachedLightningMin = 4.0f;
	float CachedLightningMax = 18.0f;
};
