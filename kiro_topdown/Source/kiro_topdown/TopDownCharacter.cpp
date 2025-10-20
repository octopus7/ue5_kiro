#include "TopDownCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"

ATopDownCharacter::ATopDownCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 캐릭터 회전 설정 - 이동 방향으로 회전하지 않음
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 무브먼트 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// 캡슐 컴포넌트 설정
	GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
	GetCapsuleComponent()->SetCapsuleRadius(34.0f);

	// 스프링 암 컴포넌트 생성
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // 절대 회전 사용
	CameraBoom->TargetArmLength = 800.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false; // 충돌 테스트 비활성화

	// 탑다운 카메라 생성
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // 폰 컨트롤 회전 사용 안함

	// 초기값 설정
	bIsMovingToTarget = false;
	TargetLocation = FVector::ZeroVector;
}

void ATopDownCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ATopDownCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 목표 위치로 이동 중인 경우
	if (bIsMovingToTarget)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
		
		// 목표 지점에 도달했는지 확인
		float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
		if (DistanceToTarget <= AcceptanceRadius)
		{
			bIsMovingToTarget = false;
			GetCharacterMovement()->StopMovementImmediately();
		}
		else
		{
			// 목표 방향으로 이동
			AddMovementInput(Direction, 1.0f);
		}
	}
}

void ATopDownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATopDownCharacter::MoveToLocation(FVector NewTargetLocation)
{
	TargetLocation = NewTargetLocation;
	TargetLocation.Z = GetActorLocation().Z; // Z축은 현재 위치 유지
	bIsMovingToTarget = true;
}