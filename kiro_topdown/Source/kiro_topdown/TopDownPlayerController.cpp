#include "TopDownPlayerController.h"
#include "TopDownCharacter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ATopDownPlayerController::ATopDownPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	// 데칼 컴포넌트 생성 (목표 지점 표시용)
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>(TEXT("CursorToWorld"));
	CursorToWorld->SetupAttachment(RootComponent);
	
	// 데칼 설정
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("/Engine/EngineMaterials/M_Cursor_Decal"));
	if (DecalMaterialAsset.Object)
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());
}

void ATopDownPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Enhanced Input 서브시스템에 매핑 컨텍스트 추가
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ATopDownPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input 컴포넌트로 캐스팅
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 클릭 액션 바인딩
		if (ClickAction)
		{
			EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Completed, this, &ATopDownPlayerController::OnClick);
		}
	}
}

void ATopDownPlayerController::OnClick(const FInputActionValue& Value)
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(HitResult))
	{
		// 플레이어 캐릭터에게 이동 명령
		if (ATopDownCharacter* TopDownCharacter = Cast<ATopDownCharacter>(GetPawn()))
		{
			TopDownCharacter->MoveToLocation(HitResult.Location);
		}
	}
}

bool ATopDownPlayerController::GetHitResultUnderCursor(FHitResult& HitResult) const
{
	return Super::GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult);
}

void ATopDownPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 마우스 커서 위치에 데칼 업데이트
	FHitResult HitResult;
	if (GetHitResultUnderCursor(HitResult))
	{
		CursorToWorld->SetWorldLocation(HitResult.Location);
	}
}