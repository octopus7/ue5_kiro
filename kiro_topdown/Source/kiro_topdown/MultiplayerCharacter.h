#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "MultiplayerCharacter.generated.h"

/**
 * Multiplayer Character Pawn
 * Represents a player character in the multiplayer game
 */
UCLASS()
class KIRO_TOPDOWN_API AMultiplayerCharacter : public APawn
{
	GENERATED_BODY()

public:
	AMultiplayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// Movement functions
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartMovementToLocation(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpdateRemoteMovement(const FVector& CurrentPos, const FVector& TargetPos, float Speed);

	// Property setters and getters
	UFUNCTION(BlueprintCallable, Category = "Character Properties")
	void SetUserID(int32 InUserID) { UserID = InUserID; }

	UFUNCTION(BlueprintCallable, Category = "Character Properties")
	int32 GetUserID() const { return UserID; }

	UFUNCTION(BlueprintCallable, Category = "Character Properties")
	void SetCharacterID(const FString& InCharacterID) { CharacterID = InCharacterID; }

	UFUNCTION(BlueprintCallable, Category = "Character Properties")
	FString GetCharacterID() const { return CharacterID; }

	UFUNCTION(BlueprintCallable, Category = "Character Properties")
	void SetIsLocalPlayer(bool bInIsLocalPlayer) { bIsLocalPlayer = bInIsLocalPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Character Properties")
	bool GetIsLocalPlayer() const { return bIsLocalPlayer; }

	// Input handling (for local player)
	void OnMouseClick();

	// Movement properties
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	float MovementSpeed = 100.0f;

	// Character properties
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	int32 UserID;

	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	FString CharacterID;

	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	bool bIsLocalPlayer;

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

private:
	// Movement state
	FVector CurrentPosition;
	FVector TargetPosition;
	bool bIsMoving;
	float LastUpdateTime;

	FVector GetMouseWorldPosition();

	// Movement processing
	void UpdateMovement(float DeltaTime);
};
