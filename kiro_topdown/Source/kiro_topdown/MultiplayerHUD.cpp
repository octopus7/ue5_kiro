#include "MultiplayerHUD.h"
#include "ConnectionStatusWidget.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AMultiplayerHUD::AMultiplayerHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Set default widget class (can be overridden in Blueprint)
	ConnectionStatusWidgetClass = UConnectionStatusWidget::StaticClass();
}

void AMultiplayerHUD::BeginPlay()
{
	Super::BeginPlay();
	
	// Create UI widgets
	CreateUIWidgets();
}

void AMultiplayerHUD::DrawHUD()
{
	Super::DrawHUD();
	
	// Update movement feedback
	UpdateMovementFeedback(GetWorld()->GetDeltaSeconds());
	
	// Draw movement feedback circles
	for (const FMovementFeedback& Feedback : MovementFeedbacks)
	{
		FVector2D ScreenLocation;
		if (UGameplayStatics::ProjectWorldToScreen(GetOwningPlayerController(), Feedback.WorldLocation, ScreenLocation))
		{
			float Alpha = Feedback.TimeRemaining / MovementFeedbackDuration;
			FLinearColor DrawColor = Feedback.Color;
			DrawColor.A = Alpha;
			
			// Draw a circle at the target location
			float Radius = 20.0f * (1.0f + (1.0f - Alpha));
			DrawCircle(FVector(ScreenLocation.X, ScreenLocation.Y, 0.0f), Radius, 16, DrawColor);
		}
	}
}

void AMultiplayerHUD::UpdateConnectionStatus(bool bIsConnected, const FString& StatusMessage)
{
	if (ConnectionStatusWidget)
	{
		ConnectionStatusWidget->UpdateConnectionStatus(bIsConnected, StatusMessage);
	}
}

void AMultiplayerHUD::UpdateUserCount(int32 UserCount)
{
	if (ConnectionStatusWidget)
	{
		ConnectionStatusWidget->UpdateUserCount(UserCount);
	}
}

void AMultiplayerHUD::ShowMovementFeedback(const FVector& WorldLocation)
{
	// Add new movement feedback
	FMovementFeedback NewFeedback;
	NewFeedback.WorldLocation = WorldLocation;
	NewFeedback.TimeRemaining = MovementFeedbackDuration;
	NewFeedback.Color = FLinearColor::White;
	
	MovementFeedbacks.Add(NewFeedback);
	
	UE_LOG(LogTemp, Log, TEXT("Added movement feedback at location: %s"), *WorldLocation.ToString());
}

void AMultiplayerHUD::CreateUIWidgets()
{
	if (ConnectionStatusWidgetClass && !ConnectionStatusWidget)
	{
		ConnectionStatusWidget = CreateWidget<UConnectionStatusWidget>(GetWorld(), ConnectionStatusWidgetClass);
		if (ConnectionStatusWidget)
		{
			ConnectionStatusWidget->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("Connection status widget created and added to viewport"));
		}
	}
}

void AMultiplayerHUD::UpdateMovementFeedback(float DeltaTime)
{
	// Update and remove expired feedback
	for (int32 i = MovementFeedbacks.Num() - 1; i >= 0; i--)
	{
		MovementFeedbacks[i].TimeRemaining -= DeltaTime;
		if (MovementFeedbacks[i].TimeRemaining <= 0.0f)
		{
			MovementFeedbacks.RemoveAt(i);
		}
	}
}
