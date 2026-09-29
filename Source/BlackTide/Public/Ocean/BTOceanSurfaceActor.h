#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BTOceanSurfaceActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * BT_OceanSurface —— 海面网格。
 *
 * 按 GridSize/GridResolution 生成高细分平面网格，配合 M_Ocean 材质的
 * Gerstner WPO 顶点位移形成动态海面。网格以 Actor 位置为中心展开，
 * 海面高度由材质与 BTOceanManager 共同决定。
 *
 * 说明：引擎自带 Plane 仅 4 顶点，无法承载 WPO 波形，故用 ProceduralMesh 生成。
 *
 * 生成时机分两档，这是本类最需要注意的一点：
 *   - 编辑器（OnConstruction）：只生成 EditorPreviewResolution 段低精度网格。
 *   - 运行时（BeginPlay）        ：生成 GridResolution 段全精度网格。
 *
 * 之所以不在 OnConstruction 里直接生成全精度网格，是因为 ProceduralMesh 的顶点数据
 * 会被序列化进 .umap：512 段 = 513×513 顶点 + 157 万索引，实测把关卡从约 2MB
 * 撑到 162MB，远超 GitHub 单文件 100MB 上限。编辑器预览改成 96 段后关卡回到几 MB。
 */
UCLASS(BlueprintType)
class BLACKTIDE_API ABTOceanSurfaceActor : public AActor
{
	GENERATED_BODY()

public:
	ABTOceanSurfaceActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** 海面网格组件（挂载 M_Ocean 材质） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean")
	TObjectPtr<UProceduralMeshComponent> OceanMesh;

	/** 海面覆盖范围（米，正方形边长） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean", meta = (ClampMin = "10.0"))
	float GridSizeMeters = 1200.0f;

	/** 运行时每边分段数。Detail 波长 7m 至少需要约 3 顶点/波长，故默认 512（单元格约 2.3m） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean", meta = (ClampMin = "8", ClampMax = "1024"))
	int32 GridResolution = 512;

	/**
	 * 编辑器预览网格的分段数，仅在编辑器中生效。
	 * 精度足够看清 WPO 波形即可，调大会直接增大 .umap；设为 0 关闭预览。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean", meta = (ClampMin = "0", ClampMax = "256"))
	int32 EditorPreviewResolution = 96;

	/** 海面材质（M_Ocean，含 Gerstner WPO 顶点位移） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean")
	TObjectPtr<UMaterialInterface> OceanMaterial;

private:
	/** 按给定分段数重建海面网格（会先清空已有 Section） */
	void BuildGrid(int32 Resolution);

	/** 套用海面材质到网格组件 */
	void ApplyMaterial();
};
