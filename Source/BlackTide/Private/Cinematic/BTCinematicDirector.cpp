#include "Cinematic/BTCinematicDirector.h"

#include "Core/BTStormManager.h"
#include "FX/BTLightningController.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABTCinematicDirector::ABTCinematicDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABTCinematicDirector::BeginPlay()
{
	Super::BeginPlay();

	// 未手动指定时自动查找场景中的闪电控制器
	if (!LightningController)
	{
		LightningController = Cast<ABTLightningController>(
			UGameplayStatics::GetActorOfClass(this, ABTLightningController::StaticClass()));
	}
}

void ABTCinematicDirector::PlayMainSequence()
{
	if (!MainSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTCinematicDirector] MainSequence not set."));
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;

	ALevelSequenceActor* SequenceActor = nullptr;
	ActivePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		this, MainSequence.Get(), Settings, SequenceActor);

	if (ActivePlayer)
	{
		ActivePlayer->Play();
	}
}

void ABTCinematicDirector::ScheduleLightningAt(float TimeSeconds, EBTLightningIntensity Intensity)
{
	if (!LightningController)
	{
		return;
	}

	FTimerHandle Handle;
	FTimerDelegate Delegate;
	Delegate.BindLambda([this, Intensity]()
	{
		LightningController->TriggerLightning(Intensity);
	});

	GetWorldTimerManager().SetTimer(Handle, Delegate, TimeSeconds, false);
	ScheduledEvents.Add(Handle);
}

void ABTCinematicDirector::ScheduleWeatherAt(float TimeSeconds, EBTWeatherState State)
{
	ABTStormManager* Storm = ABTStormManager::Get(this);
	if (!Storm)
	{
		return;
	}

	FTimerHandle Handle;
	FTimerDelegate Delegate;
	Delegate.BindLambda([Storm, State]()
	{
		Storm->SetWeatherState(State);
	});

	GetWorldTimerManager().SetTimer(Handle, Delegate, TimeSeconds, false);
	ScheduledEvents.Add(Handle);
}
