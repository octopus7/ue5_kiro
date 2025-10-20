#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TopDownCharacter.generated.h"

UCLASS()
class KIRO_TOPDOWN_API ATopDownCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATopDownCharacter();

protected:
	virtual void BeginPlay() override;

	// 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	// 스프링 암 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 목표 위치로 이동
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveToLocation(FVector TargetLocation);

	// 현재 이동 중인지 확인
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMovingToTarget() const { return bIsMovingToTarget; }

private:
	// 목표 위치
	FVector TargetLocation;
	
	// 이동 중인지 여부
	bool bIsMovingToTarget;
	
	// 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MovementSpeed = 600.0f;
	
	// 목표 지점 도달 허용 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float AcceptanceRadius = 50.0f;

public:
	// 카메라 컴포넌트 접근자
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
};