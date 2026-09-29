#include "Ship/ShipFXComponent.h"

#include "Core/BTStormManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

UShipFXComponent::UShipFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShipFXComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 船艏浪花
	if (BowSplashSystem)
	{
		BowSplashComp = NewObject<UNiagaraComponent>(Owner);
		BowSplashComp->SetupAttachment(Owner->GetRootComponent());
		BowSplashComp->SetAsset(BowSplashSystem);
		BowSplashComp->SetRelativeLocation(BowOffset);
		BowSplashComp->RegisterComponent();
	}

	// 船迹泡沫
	if (WakeFoamSystem)
	{
		WakeFoamComp = NewObject<UNiagaraComponent>(Owner);
		WakeFoamComp->SetupAttachment(Owner->GetRootComponent());
		WakeFoamComp->SetAsset(WakeFoamSystem);
		WakeFoamComp->RegisterComponent();
	}

	if (ABTStormManager* Storm = ABTStormManager::Get(this))
	{
		Storm->OnStormParamsUpdated.AddDynamic(this, &UShipFXComponent::OnStormParamsReceived);
		OnStormParamsReceived(Storm->GetCurrentParams());
	}
}

void UShipFXComponent::OnStormParamsReceived(const FBTStormParams& Params)
{
	CurrentSplashIntensity = Params.SplashIntensity;
	CurrentFoamIntensity = Params.FoamIntensity;
}

void UShipFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 船速决定浪花大小，风暴强度决定整体增益
	const float Speed = Owner->GetVelocity().Size();
	const float SpeedFactor = FMath::Clamp(Speed / FMath::Max(MinSpeedForSplash, 1.0f), 0.0f, 2.0f);

	if (BowSplashComp)
	{
		const float SpawnScale = SpeedFactor * CurrentSplashIntensity;
		BowSplashComp->SetVariableFloat(TEXT("SpawnScale"), SpawnScale);
		BowSplashComp->SetActive(SpawnScale > KINDA_SMALL_NUMBER);
	}

	if (WakeFoamComp)
	{
		const float FoamScale = FMath::Clamp(SpeedFactor, 0.0f, 1.0f) * CurrentFoamIntensity;
		WakeFoamComp->SetVariableFloat(TEXT("FoamScale"), FoamScale);
		WakeFoamComp->SetActive(FoamScale > KINDA_SMALL_NUMBER);
	}
}
