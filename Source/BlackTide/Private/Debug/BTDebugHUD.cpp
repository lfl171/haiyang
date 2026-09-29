#include "Debug/BTDebugHUD.h"

#include "Core/BTStormManager.h"
#include "Ship/ShipMovementComponent.h"
#include "Ship/BTShipActor.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void ABTDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowDebugPanel || !Canvas)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 平滑帧时间
	const float RawFrameTime = World->GetDeltaSeconds() * 1000.0f;
	SmoothedFrameTime = FMath::Lerp(SmoothedFrameTime, RawFrameTime, 0.05f);
	const float FPS = 1000.0f / FMath::Max(SmoothedFrameTime, 0.01f);

	// 风暴参数
	FBTStormParams Params;
	FString WeatherName = TEXT("N/A");
	if (ABTStormManager* Storm = ABTStormManager::Get(World))
	{
		Params = Storm->GetCurrentParams();
		WeatherName = UEnum::GetValueAsString(Storm->GetWeatherState());
	}

	// 船体数据
	float ShipPitch = 0.0f, ShipRoll = 0.0f, ShipSpeed = 0.0f;
	if (AActor* Ship = UGameplayStatics::GetActorOfClass(World, ABTShipActor::StaticClass()))
	{
		ShipSpeed = Ship->GetVelocity().Size() * 0.01f; // cm/s -> m/s
		if (const UShipMovementComponent* MoveComp = Ship->FindComponentByClass<UShipMovementComponent>())
		{
			ShipPitch = MoveComp->CurrentPitch;
			ShipRoll = MoveComp->CurrentRoll;
		}
	}

	TArray<FString> Lines;
	Lines.Add(TEXT("=========== BLACK TIDE DEBUG ==========="));
	Lines.Add(FString::Printf(TEXT("FPS            %6.1f"), FPS));
	Lines.Add(FString::Printf(TEXT("Frame Time     %6.2f ms"), SmoothedFrameTime));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("---- STORM ----"));
	Lines.Add(FString::Printf(TEXT("State          %s"), *WeatherName));
	Lines.Add(FString::Printf(TEXT("Rain           %5.0f %%"), Params.RainIntensity * 100.0f));
	Lines.Add(FString::Printf(TEXT("Wind           %5.0f %%"), Params.WindStrength * 100.0f));
	Lines.Add(FString::Printf(TEXT("Wave           %5.1f m"), Params.WaveHeight));
	Lines.Add(FString::Printf(TEXT("Fog            %5.0f %%"), Params.FogDensity * 100.0f));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("---- SHIP ----"));
	Lines.Add(FString::Printf(TEXT("Speed          %5.1f m/s"), ShipSpeed));
	Lines.Add(FString::Printf(TEXT("Pitch          %5.1f deg"), ShipPitch));
	Lines.Add(FString::Printf(TEXT("Roll           %5.1f deg"), ShipRoll));

	const float LineHeight = 20.0f;
	const float PadLeft = 12.0f;
	const float PadTop = 12.0f;
	const float PanelWidth = 340.0f;
	const float PanelHeight = Lines.Num() * LineHeight + PadTop * 2.0f;

	// 半透明黑色背景
	Canvas->K2_DrawBox(FVector2D(10.0f, 10.0f), FVector2D(PanelWidth, PanelHeight), 1.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));

	UFont* Font = GEngine->GetSmallFont();
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		Canvas->K2_DrawText(Font, Lines[i], FVector2D(10.0f + PadLeft, 10.0f + PadTop + i * LineHeight),
			FVector2D(1.0f, 1.0f), FLinearColor::White);
	}
}
