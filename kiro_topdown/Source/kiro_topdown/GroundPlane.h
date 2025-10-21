#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "GroundPlane.generated.h"

/**
 * Simple ground plane actor for the test level
 */
UCLASS()
class KIRO_TOPDOWN_API AGroundPlane : public AActor
{
	GENERATED_BODY()
	
public:	
	AGroundPlane();

protected:
	virtual void BeginPlay() override;

	// Ground mesh component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GroundMesh;

	// Ground plane size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Settings")
	FVector GroundSize = FVector(20.0f, 20.0f, 1.0f);

public:	
	virtual void Tick(float DeltaTime) override;
};