#include "GroundPlane.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"

AGroundPlane::AGroundPlane()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create ground mesh component
	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	RootComponent = GroundMesh;

	// Set default cube mesh and scale it to be a flat plane
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		GroundMesh->SetStaticMesh(CubeMeshAsset.Object);
		GroundMesh->SetRelativeScale3D(GroundSize);
	}

	// Set collision to block all
	GroundMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GroundMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Set a basic material (can be overridden in Blueprint)
	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (DefaultMaterial.Succeeded())
	{
		GroundMesh->SetMaterial(0, DefaultMaterial.Object);
	}
}

void AGroundPlane::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("Ground plane created with size: %s"), *GroundSize.ToString());
}

void AGroundPlane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}