#include "Core/BTStormManager.h"

#include "Core/BTStormProfile.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/GameplayStatics.h"

ABTStormManager::ABTStormManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

ABTStormManager* ABTStormManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}
	return Cast<ABTStormManager>(UGameplayStatics::GetActorOfClass(World, ABTStormManager::StaticClass()));
}

void ABTStormManager::BeginPlay()
{
	Super::BeginPlay();
	SetWeatherStateImmediate(InitialState);
}

void ABTStormManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 状态过渡插值
	if (TransitionAlpha < 1.0f)
	{
		TransitionDuration = FMath::Max(TransitionDuration, 0.01f);
		TransitionAlpha = FMath::Min(TransitionAlpha + DeltaTime / TransitionDuration, 1.0f);

		if (const UBTStormProfile* TargetProfile = FindProfile(TargetState))
		{
			CurrentParams = FBTStormParams::Lerp(TransitionStartParams, TargetProfile->Params, TransitionAlpha);
		}
	}

	ApplyParamsToMPC();
	OnStormParamsUpdated.Broadcast(CurrentParams);
}

void ABTStormManager::SetWeatherState(EBTWeatherState NewState)
{
	if (NewState == TargetState && !IsTransitioning())
	{
		return;
	}

	const UBTStormProfile* Profile = FindProfile(NewState);
	if (!Profile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTStormManager] No profile registered for state %d"), static_cast<int32>(NewState));
		return;
	}

	const EBTWeatherState OldState = CurrentState;

	// 从当前实时参数开始过渡，避免切换中途再切换时参数跳变
	TransitionStartParams = CurrentParams;
	CurrentState = NewState;
	TargetState = NewState;
	TransitionAlpha = 0.0f;
	TransitionDuration = Profile->TransitionDuration;
	CachedLightningMin = Profile->LightningMinInterval;
	CachedLightningMax = Profile->LightningMaxInterval;

	OnWeatherStateChanged.Broadcast(OldState, NewState);
}

void ABTStormManager::SetWeatherStateImmediate(EBTWeatherState NewState)
{
	const UBTStormProfile* Profile = FindProfile(NewState);
	if (!Profile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTStormManager] No profile registered for state %d"), static_cast<int32>(NewState));
		return;
	}

	const EBTWeatherState OldState = CurrentState;
	CurrentState = NewState;
	TargetState = NewState;
	TransitionAlpha = 1.0f;
	CurrentParams = Profile->Params;
	CachedLightningMin = Profile->LightningMinInterval;
	CachedLightningMax = Profile->LightningMaxInterval;

	ApplyParamsToMPC();
	OnStormParamsUpdated.Broadcast(CurrentParams);
	OnWeatherStateChanged.Broadcast(OldState, NewState);
}

const UBTStormProfile* ABTStormManager::FindProfile(EBTWeatherState State) const
{
	const TObjectPtr<UBTStormProfile>* Found = Profiles.Find(State);
	return Found ? Found->Get() : nullptr;
}

void ABTStormManager::ApplyParamsToMPC() const
{
	if (!GlobalMPC)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UMaterialParameterCollectionInstance* MPC = World->GetParameterCollectionInstance(GlobalMPC);
	if (!MPC)
	{
		return;
	}

	// 全局标量：材质（海洋泡沫/木头湿润/天空暗度）与 Niagara（雨量）统一从这里读取
	MPC->SetScalarParameterValue(TEXT("RainIntensity"), CurrentParams.RainIntensity);
	MPC->SetScalarParameterValue(TEXT("WindStrength"), CurrentParams.WindStrength);
	MPC->SetScalarParameterValue(TEXT("WaveHeight"), CurrentParams.WaveHeight);
	MPC->SetScalarParameterValue(TEXT("WaveSpeed"), CurrentParams.WaveSpeed);
	MPC->SetScalarParameterValue(TEXT("FogDensity"), CurrentParams.FogDensity);
	MPC->SetScalarParameterValue(TEXT("OceanDarkness"), CurrentParams.OceanDarkness);
	MPC->SetScalarParameterValue(TEXT("FoamIntensity"), CurrentParams.FoamIntensity);
	MPC->SetScalarParameterValue(TEXT("SplashIntensity"), CurrentParams.SplashIntensity);
}
