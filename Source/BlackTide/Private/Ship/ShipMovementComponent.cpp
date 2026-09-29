#include "Ship/ShipMovementComponent.h"

#include "Ocean/BTOceanManager.h"

UShipMovementComponent::UShipMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 默认四点采样：船头、船尾、左舷、右舷（单位 cm，按 40m 船估算）
	BuoyancyProbeOffsets = {
		FVector( 1800.0f,     0.0f, 0.0f),  // 船头
		FVector(-1800.0f,     0.0f, 0.0f),  // 船尾
		FVector(    0.0f,  600.0f, 0.0f),  // 右舷
		FVector(    0.0f, -600.0f, 0.0f)   // 左舷
	};
}

void UShipMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	Ocean = ABTOceanManager::Get(this);

	if (!Ocean)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShipMovement] No BTOceanManager in level, buoyancy disabled."));
		SetComponentTickEnabled(false);
	}
}

void UShipMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || !Ocean || BuoyancyProbeOffsets.Num() < 3)
	{
		return;
	}

	const FTransform OwnerTransform = Owner->GetActorTransform();

	// 1. 采样各点水面高度
	TArray<float> WaterHeights;
	WaterHeights.SetNum(BuoyancyProbeOffsets.Num());
	float AvgHeight = 0.0f;

	for (int32 i = 0; i < BuoyancyProbeOffsets.Num(); ++i)
	{
		const FVector WorldPos = OwnerTransform.TransformPosition(BuoyancyProbeOffsets[i]);
		WaterHeights[i] = Ocean->GetWaterHeightAt(WorldPos);
		AvgHeight += WaterHeights[i];
	}
	AvgHeight /= WaterHeights.Num();

	// 2. Heave：船体 Z 向平均水面高度收敛
	const FVector Loc = Owner->GetActorLocation();
	const float TargetZ = FMath::Lerp(Loc.Z, AvgHeight, WaveInfluence);
	const float NewZ = FMath::FInterpTo(Loc.Z, TargetZ, DeltaTime, HeaveDampSpeed);

	// 3. Pitch：船头船尾高度差；Roll：左右舷高度差
	const float BowH   = WaterHeights[0];
	const float SternH = WaterHeights[1];
	const float StarH  = WaterHeights[2];
	const float PortH  = WaterHeights[3];

	const float BowSternDist = FMath::Abs(BuoyancyProbeOffsets[0].X - BuoyancyProbeOffsets[1].X);
	const float BeamDist     = FMath::Abs(BuoyancyProbeOffsets[2].Y - BuoyancyProbeOffsets[3].Y);

	const float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(BowH - SternH, BowSternDist)) * WaveInfluence;
	const float TargetRoll  = FMath::RadiansToDegrees(FMath::Atan2(StarH - PortH, BeamDist)) * WaveInfluence;

	const FRotator Rot = Owner->GetActorRotation();
	const float NewPitch = FMath::FInterpTo(Rot.Pitch, TargetPitch, DeltaTime, PitchRollDampSpeed);
	const float NewRoll  = FMath::FInterpTo(Rot.Roll, TargetRoll, DeltaTime, PitchRollDampSpeed);

	CurrentPitch = NewPitch;
	CurrentRoll = NewRoll;

	Owner->SetActorLocation(FVector(Loc.X, Loc.Y, NewZ));
	Owner->SetActorRotation(FRotator(NewPitch, Rot.Yaw, NewRoll));
}
