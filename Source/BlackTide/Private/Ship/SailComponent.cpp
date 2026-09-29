#include "Ship/SailComponent.h"

#include "Core/BTStormManager.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

USailComponent::USailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USailComponent::BeginPlay()
{
	Super::BeginPlay();

	// 为每面帆创建动态材质实例
	for (UStaticMeshComponent* Mesh : SailMeshes)
	{
		if (!Mesh)
		{
			continue;
		}
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			DynamicMats.Add(Mesh->CreateDynamicMaterialInstance(i));
		}
	}

	if (ABTStormManager* Storm = ABTStormManager::Get(this))
	{
		Storm->OnStormParamsUpdated.AddDynamic(this, &USailComponent::OnStormParamsReceived);
		OnStormParamsReceived(Storm->GetCurrentParams());
	}
}

void USailComponent::OnStormParamsReceived(const FBTStormParams& Params)
{
	CurrentWind = Params.WindStrength;
}

void USailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 帆的飘动相位：风力越大摆动越快
	SailPhase += DeltaTime * (0.5f + CurrentWind * 2.0f);

	const float Sway = FMath::Sin(SailPhase) * CurrentWind * WindSwayGain;
	const float Billow = CurrentWind * BillowGain;

	for (UMaterialInstanceDynamic* Mat : DynamicMats)
	{
		if (!Mat)
		{
			continue;
		}
		Mat->SetScalarParameterValue(TEXT("WindSway"), Sway);
		Mat->SetScalarParameterValue(TEXT("Billow"), Billow);
	}

	// 兜风横摇：给船体施加一个随阵风缓慢变化的额外横摇
	if (AActor* Owner = GetOwner())
	{
		const float GustNoise = FMath::PerlinNoise1D(SailPhase * 0.3f) * 0.5f + 0.5f;
		const float Heel = CurrentWind * GustNoise * HeelMomentGain;
		Owner->AddActorLocalRotation(FRotator(0.0f, 0.0f, Heel * DeltaTime * 0.2f));
	}
}
