#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ConnectionStatusWidget.h"
#include "MultiplayerHUD.generated.h"

/**
 * Custom HUD for multiplayer game
 * Manages UI elements like connection status and visual feedback
 */
UCLASS()
class KIRO_TOPDOWN_API AMultiplayerHUD : public AHUD
{
	GENERATED_BODY()

public:
	AMultiplayerHUD();

protected:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	// Widget classes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UConnectionStatusWidget> ConnectionStatusWidgetClass;

	// Widget instances
	UPROPERTY()
	UConnectionStatusWidget* ConnectionStatusWidget;

public:
	// UI Management functions
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateConnectionStatus(bool bIsConnected, const FString& StatusMessage = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateUserCount(int32 UserCount);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMovementFeedback(const FVector& WorldLocation);

private:
	// Movement feedback
	struct FMovementFeedback
	{
		FVector WorldLocation;
		float TimeRemaining;
		FLinearColor Color;
	};

	TArray<FMovementFeedback> MovementFeedbacks;
	float MovementFeedbackDuration = 1.0f;

	// Initialize UI widgets
	void CreateUIWidgets();

	// Update movement feedback display
	void UpdateMovementFeedback(float DeltaTime);

	// Draw circle helper on 2D canvas
	void DrawCircleOverlay(const FVector2D& Center, float Radius, int32 NumSegments, const FLinearColor& Color);
};
