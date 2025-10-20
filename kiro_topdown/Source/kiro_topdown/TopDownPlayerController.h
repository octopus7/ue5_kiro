#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/HitResult.h"
#include "TopDownPlayerController.generated.h"

UCLASS()
class KIRO_TOPDOWN_API ATopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATopDownPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// 마우스 클릭 처리
	void OnLeftMouseClick();
	void OnLeftMouseRelease();

	// 마우스 커서 히트 테스트
	bool GetHitResultUnderCursor(FHitResult& HitResult) const;

private:
	// 클릭 상태 추적
	bool bIsMousePressed;
	
	// 데칼 컴포넌트 (목표 지점 표시용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cursor", meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

public:
	virtual void Tick(float DeltaTime) override;
};