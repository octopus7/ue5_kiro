#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "ConnectionStatusWidget.generated.h"

/**
 * UI Widget for displaying connection status
 */
UCLASS()
class KIRO_TOPDOWN_API UConnectionStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UConnectionStatusWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// UI Components
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ConnectionStatusText;

	UPROPERTY(meta = (BindWidget))
	UBorder* StatusBorder;

public:
	// Update connection status display
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateConnectionStatus(bool bIsConnected, const FString& StatusMessage = TEXT(""));

	// Update user count display
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateUserCount(int32 UserCount);

private:
	// Current connection state
	bool bCurrentConnectionStatus;
	FString CurrentStatusMessage;
	int32 CurrentUserCount;

	// Update the visual display
	void RefreshDisplay();
};