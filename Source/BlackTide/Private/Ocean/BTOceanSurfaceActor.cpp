#include "Ocean/BTOceanSurfaceActor.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"

ABTOceanSurfaceActor::ABTOceanSurfaceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	OceanMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OceanMesh"));
	RootComponent = OceanMesh;

	// 海面不投影（避免遮挡海底/船体阴影开销），不参与碰撞
	OceanMesh->SetCastShadow(false);
	OceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABTOceanSurfaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 编辑器里只生成低精度预览网格。
	// 全精度网格的顶点数据会随关卡一起序列化，512 段实测让 .umap 膨胀到 162MB，
	// 因此真正的海面网格推迟到 BeginPlay 生成（运行时数据不落盘）。
	if (EditorPreviewResolution > 0 && GetWorld() && !GetWorld()->IsGameWorld())
	{
		BuildGrid(EditorPreviewResolution);
	}

	ApplyMaterial();
}

void ABTOceanSurfaceActor::BeginPlay()
{
	Super::BeginPlay();

	// 运行时重建全精度网格，替换掉编辑器留下的低精度预览
	BuildGrid(GridResolution);
	ApplyMaterial();
}

void ABTOceanSurfaceActor::ApplyMaterial()
{
	if (OceanMaterial)
	{
		OceanMesh->SetMaterial(0, OceanMaterial);
	}
}

void ABTOceanSurfaceActor::BuildGrid(int32 Resolution)
{
	const int32 Res = FMath::Clamp(Resolution, 8, 1024);
	const float SizeUU = GridSizeMeters * 100.0f;
	const float Half = SizeUU * 0.5f;
	const float Step = SizeUU / Res;

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	Vertices.Reserve((Res + 1) * (Res + 1));
	Normals.Reserve((Res + 1) * (Res + 1));
	UVs.Reserve((Res + 1) * (Res + 1));
	Triangles.Reserve(Res * Res * 6);

	for (int32 Y = 0; Y <= Res; ++Y)
	{
		for (int32 X = 0; X <= Res; ++X)
		{
			Vertices.Add(FVector(X * Step - Half, Y * Step - Half, 0.0f));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(static_cast<float>(X) / Res, static_cast<float>(Y) / Res));
		}
	}

	// 左手坐标系：逆时针从上方看为正面（法线 +Z）
	for (int32 Y = 0; Y < Res; ++Y)
	{
		for (int32 X = 0; X < Res; ++X)
		{
			const int32 I0 = Y * (Res + 1) + X;
			const int32 I1 = I0 + 1;
			const int32 I2 = I0 + Res + 1;
			const int32 I3 = I2 + 1;
			Triangles.Append({ I0, I1, I2, I1, I3, I2 });
		}
	}

	OceanMesh->ClearAllMeshSections();
	OceanMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}
