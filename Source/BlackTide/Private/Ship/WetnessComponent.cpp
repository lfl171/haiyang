#include "Ship/WetnessComponent.h"

#include "Core/BTStormManager.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UWetnessComponent::UWetnessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWetnessComponent::BeginPlay()
{
	Super::BeginPlay();

	// 为船主 Actor 下所有 Mesh 创建动态材质实例
	if (AActor* Owner = GetOwner())
	{
		TArray<UMeshComponent*> Meshes;
		Owner->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
			{
				if (UMaterialInstanceDynamic* DynMat = Mesh->CreateDynamicMaterialInstance(i))
				{
					DynamicMats.Add(DynMat);
				}
			}
		}
	}

	if (ABTStormManager* Storm = ABTStormManager::Get(this))
	{
		Storm->OnStormParamsUpdated.AddDynamic(this, &UWetnessComponent::OnStormParamsReceived);
		OnStormParamsReceived(Storm->GetCurrentParams());
	}
}

void UWetnessComponent::OnStormParamsReceived(const FBTStormParams& Params)
{
	CurrentRain = Params.RainIntensity;
}

void UWetnessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 雨中湿润累积，雨停缓慢干燥
	const float Delta = (CurrentRain > KINDA_SMALL_NUMBER)
		? CurrentRain * WettingSpeed * DeltaTime
		: -DryingSpeed * DeltaTime;

	Wetness = FMath::Clamp(Wetness + Delta, 0.0f, 1.0f);

	// 水膜在超过阈值后逐渐出现
	const float WaterFilm = FMath::Clamp((Wetness - WaterFilmThreshold) / (1.0f - WaterFilmThreshold), 0.0f, 1.0f);

	for (UMaterialInstanceDynamic* Mat : DynamicMats)
	{
		if (!Mat)
		{
			continue;
		}
		Mat->SetScalarParameterValue(TEXT("Wetness"), Wetness);
		Mat->SetScalarParameterValue(TEXT("WaterFilm"), WaterFilm);
	}
}
