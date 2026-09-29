#include "FX/BTLightningController.h"

#include "Core/BTStormManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABTLightningController::ABTLightningController()
{
	PrimaryActorTick.bCanEverTick = true;

	LightningLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("LightningLight"));
	LightningLight->SetupAttachment(RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	LightningLight->SetIntensity(0.0f);
	LightningLight->SetLightColor(FLinearColor(0.75f, 0.85f, 1.0f)); // 冷白
	LightningLight->SetCastShadows(false); // 闪光瞬间不开阴影，控制性能峰值

	FlashSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("FlashSkyLight"));
	FlashSkyLight->SetupAttachment(RootComponent);
	FlashSkyLight->SetIntensity(0.0f);
	FlashSkyLight->SetLightColor(FLinearColor(0.7f, 0.8f, 1.0f));
}

void ABTLightningController::BeginPlay()
{
	Super::BeginPlay();

	// 订阅天气参数，用 LightningFrequency 控制触发节奏
	if (ABTStormManager* Storm = ABTStormManager::Get(this))
	{
		Storm->OnStormParamsUpdated.AddDynamic(this, &ABTLightningController::OnStormParamsReceived);
	}
	ScheduleNextStrike();
}

void ABTLightningController::OnStormParamsReceived(const FBTStormParams& Params)
{
	CurrentLightningFrequency = Params.LightningFrequency;
}

void ABTLightningController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ElapsedTime += DeltaTime;

	// 到达触发时间且当前天气允许闪电
	if (!bFlashing && CurrentLightningFrequency > KINDA_SMALL_NUMBER && ElapsedTime >= NextStrikeTime)
	{
		// 频率越高，越容易掷出高强度闪电
		const float Roll = FMath::FRand();
		EBTLightningIntensity Intensity = EBTLightningIntensity::Normal;
		if (Roll > 0.95f - CurrentLightningFrequency * 0.2f)      { Intensity = EBTLightningIntensity::Extreme; }
		else if (Roll > 0.75f - CurrentLightningFrequency * 0.3f) { Intensity = EBTLightningIntensity::Strong; }
		else if (Roll < 0.25f)                                     { Intensity = EBTLightningIntensity::Weak; }

		TriggerLightning(Intensity);
		ScheduleNextStrike();
	}

	if (bFlashing)
	{
		PlayFlashCurve(DeltaTime);
	}
}

void ABTLightningController::TriggerLightning(EBTLightningIntensity Intensity)
{
	CurrentStrikeIntensity = Intensity;
	FlashPeak = BaseFlashIntensity * GetIntensityMultiplier(Intensity);
	bFlashing = true;
	FlashClock = 0.0f;

	// 闪电出现在随机方位，让镜头内反光方向有变化
	const float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
	LightningLight->SetWorldRotation(FRotator(-60.0f, RandomYaw, 0.0f));

	OnLightningStrike.Broadcast(Intensity);
	PlayThunder(Intensity);
}

void ABTLightningController::PlayFlashCurve(float DeltaTime)
{
	FlashClock += DeltaTime;

	// 多段闪光包络：爆闪 -> 暗 -> 二次闪 -> 恢复
	float Env = 0.0f;
	if (FlashClock < 0.03f)
	{
		Env = FlashClock / 0.03f;
	}
	else if (FlashClock < 0.08f)
	{
		Env = 1.0f - (FlashClock - 0.03f) / 0.05f * 0.8f; // 快速衰减到 20%
	}
	else if (FlashClock < 0.15f)
	{
		Env = 0.2f + (FlashClock - 0.08f) / 0.07f * 0.8f; // 第二次攀升
	}
	else if (FlashClock < 0.30f)
	{
		Env = 1.0f - (FlashClock - 0.15f) / 0.15f; // 完全衰减
	}
	else
	{
		bFlashing = false;
		LightningLight->SetIntensity(0.0f);
		FlashSkyLight->SetIntensity(0.0f);
		return;
	}

	LightningLight->SetIntensity(FlashPeak * Env);
	FlashSkyLight->SetIntensity(FlashPeak * Env * 0.0001f); // 天光只需微弱反弹
}

void ABTLightningController::PlayThunder(EBTLightningIntensity Intensity)
{
	USoundBase* Cue = (Intensity == EBTLightningIntensity::Weak) ? ThunderWeak : ThunderStrong;
	if (!Cue)
	{
		return;
	}

	// 雷声延迟 = 距离感
	const float Delay = FMath::FRandRange(ThunderDelayMin, ThunderDelayMax);
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, [this, Cue]()
	{
		UGameplayStatics::PlaySound2D(this, Cue);
	}, Delay, false);
}

void ABTLightningController::ScheduleNextStrike()
{
	// LightningFrequency 0~1 插值触发间隔：频率越高间隔越短
	const float Interval = FMath::Lerp(StrikeIntervalMax, StrikeIntervalMin, CurrentLightningFrequency);
	NextStrikeTime = ElapsedTime + FMath::FRandRange(Interval * 0.5f, Interval * 1.5f);
}

float ABTLightningController::GetIntensityMultiplier(EBTLightningIntensity Intensity) const
{
	switch (Intensity)
	{
	case EBTLightningIntensity::Weak:    return 0.35f;
	case EBTLightningIntensity::Normal:  return 0.7f;
	case EBTLightningIntensity::Strong:  return 1.2f;
	case EBTLightningIntensity::Extreme: return 2.0f;
	default:                             return 1.0f;
	}
}
