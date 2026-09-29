#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BTShipActor.generated.h"

class UShipMovementComponent;
class USailComponent;
class UWetnessComponent;
class UShipFXComponent;
class UStaticMeshComponent;

/**
 * BP_PirateShip 的 C++ 基类。
 *
 * 结构（蓝图继承后在编辑器中补全网格）：
 *   Hull / Deck / Mast / Sail / Rope / Rudder / Cannon / Lantern / Flag / Props
 *
 * 挂载组件：
 *   ShipMovementComponent  - 浮力与姿态
 *   SailComponent          - 帆风力驱动
 *   WetnessComponent       - 湿润材质
 *   ShipFXComponent        - 船艏浪花/船迹泡沫
 */
UCLASS(BlueprintType)
class BLACKTIDE_API ABTShipActor : public AActor
{
	GENERATED_BODY()

public:
	ABTShipActor();

	/** 船体主网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> Hull;

	/** 桅杆 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> Mast;

	/** 船帆（SailComponent 驱动其材质） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> Sail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UShipMovementComponent> ShipMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USailComponent> SailControl;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWetnessComponent> Wetness;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UShipFXComponent> ShipFX;
};
