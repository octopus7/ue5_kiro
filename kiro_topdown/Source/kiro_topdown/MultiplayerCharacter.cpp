#include "MultiplayerCharacter.h"
#include "CharacterManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

AMultiplayerCharacter::AMultiplayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Initialize properties
	UserID = 0;
	CharacterID = TEXT("");
	bIsLocalPlayer = false;
	MovementSpeed = 100.0f;

	// Initialize movement state
	CurrentPosition = FVector::ZeroVector;
	TargetPosition = FVector::ZeroVector;
	bIsMoving = false;
	LastUpdateTime = 0.0f;

	// Create collision component
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	CollisionComponent->SetSphereRadius(50.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);

	// Create mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	
	// Set a default cube mesh - this can be changed in Blueprint or through asset assignment
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMeshAsset.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.0f)); // Make it smaller and flatter
	}

	// Set collision for mesh
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMultiplayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize current position
	CurrentPosition = GetActorLocation();
	TargetPosition = CurrentPosition;
	
	UE_LOG(LogTemp, Log, TEXT("MultiplayerCharacter BeginPlay - UserID: %d, CharacterID: %s, IsLocal: %s"), 
		UserID, *CharacterID, bIsLocalPlayer ? TEXT("true") : TEXT("false"));
}

void AMultiplayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Update movement if character is moving
	if (bIsMoving)
	{
		UpdateMovement(DeltaTime);
	}
}

void AMultiplayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Only set up input for local player
	if (bIsLocalPlayer && PlayerInputComponent)
	{
		// Bind mouse click action
		PlayerInputComponent->BindAction("MouseClick", IE_Pressed, this, &AMultiplayerCharacter::OnMouseClick);
	}
}

void AMultiplayerCharacter::StartMovementToLocation(const FVector& TargetLocation)
{
	UE_LOG(LogTemp, Log, TEXT("Starting movement to location: %s"), *TargetLocation.ToString());
	
	CurrentPosition = GetActorLocation();
	TargetPosition = TargetLocation;
	bIsMoving = true;
	LastUpdateTime = GetWorld()->GetTimeSeconds();
}

void AMultiplayerCharacter::UpdateRemoteMovement(const FVector& CurrentPos, const FVector& TargetPos, float Speed)
{
	// Update movement parameters for remote character
	CurrentPosition = CurrentPos;
	TargetPosition = TargetPos;
	MovementSpeed = Speed;
	bIsMoving = !CurrentPos.Equals(TargetPos, 1.0f); // 1 unit tolerance
	
	if (bIsMoving)
	{
		LastUpdateTime = GetWorld()->GetTimeSeconds();
	}
	
	// Set actor location to current position
	SetActorLocation(CurrentPosition);
}

void AMultiplayerCharacter::OnMouseClick()
{
	if (!bIsLocalPlayer)
	{
		return;
	}
	
	FVector WorldPosition = GetMouseWorldPosition();
	if (!WorldPosition.IsZero())
	{
		FVector StartPosition = GetActorLocation();
		
		// Start movement to clicked location
		StartMovementToLocation(WorldPosition);
		
		// Send movement command to server through CharacterManager
		if (UCharacterManager* CharacterManager = GetGameInstance()->GetSubsystem<UCharacterManager>())
		{
			CharacterManager->SendLocalCharacterMovement(StartPosition, WorldPosition);
		}
		
		UE_LOG(LogTemp, Log, TEXT("Local player clicked at world position: %s"), *WorldPosition.ToString());
	}
}

FVector AMultiplayerCharacter::GetMouseWorldPosition()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return FVector::ZeroVector;
	}
	
	FVector WorldLocation, WorldDirection;
	if (PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		// Perform line trace to find intersection with ground plane (Z = 0)
		FVector Start = WorldLocation;
		FVector End = Start + (WorldDirection * 10000.0f); // Trace far enough
		
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			return HitResult.Location;
		}
		else
		{
			// If no hit, calculate intersection with Z=0 plane
			if (FMath::Abs(WorldDirection.Z) > 0.001f)
			{
				float T = -WorldLocation.Z / WorldDirection.Z;
				if (T > 0)
				{
					FVector IntersectionPoint = WorldLocation + (WorldDirection * T);
					return FVector(IntersectionPoint.X, IntersectionPoint.Y, 0.0f);
				}
			}
		}
	}
	
	return FVector::ZeroVector;
}

void AMultiplayerCharacter::UpdateMovement(float DeltaTime)
{
	if (!bIsMoving)
	{
		return;
	}
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastUpdate = CurrentTime - LastUpdateTime;
	
	// Calculate distance to move this frame
	float DistanceToMove = MovementSpeed * DeltaTime;
	
	// Get direction to target
	FVector DirectionToTarget = TargetPosition - CurrentPosition;
	float DistanceToTarget = DirectionToTarget.Size();
	
	if (DistanceToTarget <= DistanceToMove || DistanceToTarget < 1.0f)
	{
		// Reached target
		CurrentPosition = TargetPosition;
		bIsMoving = false;
		UE_LOG(LogTemp, Log, TEXT("Character reached target position: %s"), *TargetPosition.ToString());
	}
	else
	{
		// Move towards target
		DirectionToTarget.Normalize();
		CurrentPosition += DirectionToTarget * DistanceToMove;
	}
	
	// Update actor location
	SetActorLocation(CurrentPosition);
	LastUpdateTime = CurrentTime;
}