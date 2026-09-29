#include "Ship/BTShipActor.h"

#include "Ship/ShipMovementComponent.h"
#include "Ship/SailComponent.h"
#include "Ship/WetnessComponent.h"
#include "Ship/ShipFXComponent.h"
#include "Components/StaticMeshComponent.h"

ABTShipActor::ABTShipActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(RootComponent);

	Mast = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mast"));
	Mast->SetupAttachment(Hull);

	Sail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sail"));
	Sail->SetupAttachment(Mast);

	ShipMovement = CreateDefaultSubobject<UShipMovementComponent>(TEXT("ShipMovement"));
	SailControl  = CreateDefaultSubobject<USailComponent>(TEXT("SailControl"));
	Wetness      = CreateDefaultSubobject<UWetnessComponent>(TEXT("Wetness"));
	ShipFX       = CreateDefaultSubobject<UShipFXComponent>(TEXT("ShipFX"));

	// 帆网格注册到帆组件，自动获得风力驱动
	SailControl->SailMeshes.Add(Sail);
}
