#include "MultiplayerPlayerController.h"
#include "MultiplayerCharacter.h"
#include "MultiplayerHUD.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"

AMultiplayerPlayerController::AMultiplayerPlayerController()
{
	// Enable mouse cursor
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	// Create camera boom (spring arm component)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = CameraHeight;
	CameraBoom->SetRelativeRotation(FRotator(CameraAngle, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false; // Disable collision test for top-down view

	// Create top-down camera
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// Create cursor decal for visual feedback
	CursorDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("CursorDecal"));
	CursorDecal->SetupAttachment(RootComponent);
	
	// Set up decal material (using engine default)
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("/Engine/EngineMaterials/M_Cursor_Decal"));
	if (DecalMaterialAsset.Object)
	{
		CursorDecal->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	
	// Configure decal size and rotation
	CursorDecal->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorDecal->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());
}

void AMultiplayerPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("MultiplayerPlayerController BeginPlay"));
	
	// Set initial camera position
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = CameraHeight;
		CameraBoom->SetRelativeRotation(FRotator(CameraAngle, 0.0f, 0.0f));
	}
}

void AMultiplayerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Bind mouse click action
	if (InputComponent)
	{
		InputComponent->BindAction("MouseClick", IE_Pressed, this, &AMultiplayerPlayerController::OnMouseClick);
	}
}

void AMultiplayerPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Update cursor decal position
	UpdateCursorDecal();
}

void AMultiplayerPlayerController::OnMouseClick()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(HitResult))
	{
		UE_LOG(LogTemp, Log, TEXT("Mouse clicked at world position: %s"), *HitResult.Location.ToString());
		
		// Show movement feedback in HUD
		if (AMultiplayerHUD* MultiplayerHUD = Cast<AMultiplayerHUD>(GetHUD()))
		{
			MultiplayerHUD->ShowMovementFeedback(HitResult.Location);
		}
		
		// Get the controlled multiplayer character
		if (AMultiplayerCharacter* MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetPawn()))
		{
			// Only handle input for local player
			if (MultiplayerCharacter->GetIsLocalPlayer())
			{
				MultiplayerCharacter->OnMouseClick();
			}
		}
	}
}

bool AMultiplayerPlayerController::GetHitResultUnderCursor(FHitResult& HitResult) const
{
	// Use the base class method to get hit result under cursor
	return Super::GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult);
}

void AMultiplayerPlayerController::UpdateCursorDecal()
{
	// Update cursor decal position to follow mouse cursor
	FHitResult HitResult;
	if (GetHitResultUnderCursor(HitResult))
	{
		CursorDecal->SetWorldLocation(HitResult.Location);
	}
}