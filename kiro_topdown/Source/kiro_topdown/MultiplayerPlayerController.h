#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/HitResult.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/DecalComponent.h"
#include "MultiplayerPlayerController.generated.h"

/**
 * Custom PlayerController for multiplayer top-down gameplay
 * Handles top-down camera setup and mouse interaction
 */
UCLASS()
class KIRO_TOPDOWN_API AMultiplayerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMultiplayerPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	// Camera components for top-down view
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* TopDownCamera;

	// Cursor decal for visual feedback
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cursor")
	UDecalComponent* CursorDecal;

	// Camera settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraHeight = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraAngle = -60.0f;

public:
	// Mouse input handling
	UFUNCTION(BlueprintCallable, Category = "Input")
	void OnMouseClick();

	// Get hit result under cursor
	bool GetHitResultUnderCursor(FHitResult& HitResult) const;

	// Camera access
	UFUNCTION(BlueprintPure, Category = "Camera")
	UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }

private:
	// Update cursor decal position
	void UpdateCursorDecal();
};