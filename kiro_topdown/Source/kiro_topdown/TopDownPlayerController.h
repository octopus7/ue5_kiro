#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/HitResult.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TopDownPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class KIRO_TOPDOWN_API ATopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATopDownPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// Enhanced Input 액션들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClickAction;

	// 마우스 클릭 처리
	void OnClick(const FInputActionValue& Value);

	// 마우스 커서 히트 테스트
	bool GetHitResultUnderCursor(FHitResult& HitResult) const;

private:
	
	// 데칼 컴포넌트 (목표 지점 표시용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cursor", meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

public:
	virtual void Tick(float DeltaTime) override;
};