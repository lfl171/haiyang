#include "Ocean/BTOceanManager.h"

#include "Core/BTStormManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABTOceanManager::ABTOceanManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// 默认三层波参数：大涌浪 / 中波峰 / 细波
	LargeWave.Direction  = FVector2D(1.0f, 0.15f);
	LargeWave.Amplitude  = 3.0f;
	LargeWave.Wavelength = 120.0f;
	LargeWave.Speed      = 1.0f;
	LargeWave.Steepness  = 0.4f;

	MediumWave.Direction  = FVector2D(0.9f, -0.4f);
	MediumWave.Amplitude  = 1.2f;
	MediumWave.Wavelength = 32.0f;
	MediumWave.Speed      = 1.3f;
	MediumWave.Steepness  = 0.55f;

	DetailWave.Direction  = FVector2D(0.6f, 0.8f);
	DetailWave.Amplitude  = 0.25f;
	DetailWave.Wavelength = 7.0f;
	DetailWave.Speed      = 1.8f;
	DetailWave.Steepness  = 0.3f;
}

ABTOceanManager* ABTOceanManager::Get(const UObject* WorldContextObject)
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
	return Cast<ABTOceanManager>(UGameplayStatics::GetActorOfClass(World, ABTOceanManager::StaticClass()));
}

void ABTOceanManager::BeginPlay()
{
	Super::BeginPlay();

	if (ABTStormManager* Storm = ABTStormManager::Get(this))
	{
		Storm->OnStormParamsUpdated.AddDynamic(this, &ABTOceanManager::OnStormParamsReceived);
		// 立即同步一次，避免首帧浪高为默认值
		OnStormParamsReceived(Storm->GetCurrentParams());
	}
}

void ABTOceanManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	OceanTime += DeltaTime * WaveSpeedScale;
}

void ABTOceanManager::OnStormParamsReceived(const FBTStormParams& Params)
{
	// WaveHeight 直接作为振幅倍率；WaveSpeed 作为时间倍率
	WaveHeightScale = Params.WaveHeight;
	WaveSpeedScale = Params.WaveSpeed;
}

float ABTOceanManager::SampleWaveLayer(const FBTWaveLayer& Layer, const FVector2D& Pos, float Time) const
{
	const FVector2D Dir = Layer.Direction.GetSafeNormal();
	const float K = 2.0f * PI / Layer.Wavelength;               // 波数
	const float C = FMath::Sqrt(9.8f / K) * Layer.Speed;        // 深水波相速度
	const float Phase = K * (FVector2D::DotProduct(Dir, Pos) - C * Time);

	// Steepness 控制波峰尖锐度：正弦与尖锐波形混合
	const float SineH = FMath::Sin(Phase);
	const float SharpH = FMath::Pow(FMath::Abs(SineH), 1.0f + Layer.Steepness * 2.0f) * FMath::Sign(SineH);
	const float H = FMath::Lerp(SineH, SharpH, Layer.Steepness);

	return H * Layer.Amplitude * WaveHeightScale;
}

float ABTOceanManager::GetWaterHeightAt(const FVector& WorldLocation) const
{
	// 波参数为米制（设计文档：振幅 3m/1.2m/0.25m、波长 120m/32m/7m），
	// 而世界坐标单位为厘米 —— 先换算到米采样，结果再乘回厘米
	const FVector2D PosM(WorldLocation.X * 0.01f, WorldLocation.Y * 0.01f);

	float HeightMeters = 0.0f;
	HeightMeters += SampleWaveLayer(LargeWave, PosM, OceanTime);
	HeightMeters += SampleWaveLayer(MediumWave, PosM, OceanTime);
	HeightMeters += SampleWaveLayer(DetailWave, PosM, OceanTime);

	return SeaLevel + HeightMeters * 100.0f;
}

FVector ABTOceanManager::GetWaterNormalAt(const FVector& WorldLocation) const
{
	// 有限差分法估法线
	const float Eps = 50.0f; // 半米采样间距
	const float HC  = GetWaterHeightAt(WorldLocation);
	const float HX  = GetWaterHeightAt(WorldLocation + FVector(Eps, 0.0f, 0.0f));
	const float HY  = GetWaterHeightAt(WorldLocation + FVector(0.0f, Eps, 0.0f));

	const FVector TangentX(Eps, 0.0f, HX - HC);
	const FVector TangentY(0.0f, Eps, HY - HC);
	return FVector::CrossProduct(TangentY, TangentX).GetSafeNormal();
}
