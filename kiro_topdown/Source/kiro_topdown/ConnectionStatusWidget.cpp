#include "ConnectionStatusWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"

UConnectionStatusWidget::UConnectionStatusWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCurrentConnectionStatus = false;
	CurrentStatusMessage = TEXT("Disconnected");
	CurrentUserCount = 0;
}

void UConnectionStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Initialize the display
	RefreshDisplay();
}

void UConnectionStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// Could add periodic updates here if needed
}

void UConnectionStatusWidget::UpdateConnectionStatus(bool bIsConnected, const FString& StatusMessage)
{
	bCurrentConnectionStatus = bIsConnected;
	
	if (!StatusMessage.IsEmpty())
	{
		CurrentStatusMessage = StatusMessage;
	}
	else
	{
		CurrentStatusMessage = bIsConnected ? TEXT("Connected") : TEXT("Disconnected");
	}
	
	RefreshDisplay();
}

void UConnectionStatusWidget::UpdateUserCount(int32 UserCount)
{
	CurrentUserCount = UserCount;
	RefreshDisplay();
}

void UConnectionStatusWidget::RefreshDisplay()
{
	if (ConnectionStatusText)
	{
		FString DisplayText = FString::Printf(TEXT("Status: %s"), *CurrentStatusMessage);
		if (bCurrentConnectionStatus && CurrentUserCount > 0)
		{
			DisplayText += FString::Printf(TEXT(" | Users: %d"), CurrentUserCount);
		}
		
		ConnectionStatusText->SetText(FText::FromString(DisplayText));
	}
	
	if (StatusBorder)
	{
		// Set border color based on connection status
		FLinearColor BorderColor = bCurrentConnectionStatus ? 
			FLinearColor::Green : FLinearColor::Red;
		StatusBorder->SetBrushColor(BorderColor);
	}
}