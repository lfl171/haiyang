#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/BTWeatherTypes.h"
#include "BTCinematicDirector.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ABTLightningController;

/**
 * BT_CinematicDirector —— 电影镜头导演。
 *
 * 串联 Sequencer 镜头与实时系统的时序：
 *   镜头时间点 -> 触发闪电 / 调整风暴强度 / 切换天气
 *
 * 镜头规划（45 秒宣传片）：
 *   Shot_01  0-7s   黑屏 -> 闪电揭示海盗船
 *   Shot_02  7-15s  超远景，船在暴雨中出现
 *   Shot_03  15-23s 贴海面，巨浪前景，船冲出
 *   Shot_04  23-31s 甲板，雨滴/绳索/油灯
 *   Shot_05  31-38s 仰视桅杆/船帆/海盗旗 + 闪电
 *   Shot_06  38-45s 英雄镜头：船冲巨浪，闪电照亮，黑屏出标题
 */
UCLASS(BlueprintType)
class BLACKTIDE_API ABTCinematicDirector : public AActor
{
	GENERATED_BODY()

public:
	ABTCinematicDirector();

	virtual void BeginPlay() override;

	/** 播放主宣传片 */
	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void PlayMainSequence();

	/** 在指定时间点（秒）安排一次闪电 */
	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void ScheduleLightningAt(float TimeSeconds, EBTLightningIntensity Intensity);

	/** 在指定时间点切换天气状态 */
	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void ScheduleWeatherAt(float TimeSeconds, EBTWeatherState State);

protected:
	/** 主 Sequencer 资产（SEQ_BlackTide_Main） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic")
	TObjectPtr<ULevelSequence> MainSequence;

	/** 闪电控制器引用（场景中拾取或手动指定） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic")
	TObjectPtr<ABTLightningController> LightningController;

private:
	/** 定时事件句柄管理 */
	TArray<FTimerHandle> ScheduledEvents;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActivePlayer;
};
