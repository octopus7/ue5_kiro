#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "CharacterManager.generated.h"

class AMultiplayerCharacter;

/**
 * Character Manager Subsystem
 * Manages spawning, removing, and tracking of multiplayer characters
 */
UCLASS()
class KIRO_TOPDOWN_API UCharacterManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Character Management
	UFUNCTION(BlueprintCallable, Category = "Character Management")
	void SpawnLocalCharacter(int32 UserID);

	UFUNCTION(BlueprintCallable, Category = "Character Management")
	void SpawnRemoteCharacter(int32 UserID, const FString& CharacterID);

	UFUNCTION(BlueprintCallable, Category = "Character Management")
	void RemoveCharacter(int32 UserID);

	// Character Access
	UFUNCTION(BlueprintCallable, Category = "Character Management")
	AMultiplayerCharacter* GetCharacterByUserID(int32 UserID);

	UFUNCTION(BlueprintCallable, Category = "Character Management")
	AMultiplayerCharacter* GetLocalCharacter();

	UFUNCTION(BlueprintCallable, Category = "Character Management")
	int32 GetLocalUserID() const { return LocalUserID; }

	UFUNCTION(BlueprintCallable, Category = "Character Management")
	void SetLocalUserID(int32 UserID) { LocalUserID = UserID; }

	// Network integration
	UFUNCTION(BlueprintCallable, Category = "Network Integration")
	void SendLocalCharacterMovement(const FVector& StartPosition, const FVector& TargetPosition);

private:
	// Character storage
	UPROPERTY()
	TMap<int32, AMultiplayerCharacter*> Characters;

	// Local player info
	UPROPERTY()
	int32 LocalUserID;

	// Character class to spawn
	UPROPERTY()
	TSubclassOf<AMultiplayerCharacter> CharacterClass;

	// Network integration methods
	UFUNCTION(BlueprintCallable, Category = "Network Integration")
	void HandleUserIDAssignment(int32 UserID);

	UFUNCTION(BlueprintCallable, Category = "Network Integration")
	void HandleAllUsersInfo(const TArray<struct FUserInfo>& AllUsers);

	UFUNCTION(BlueprintCallable, Category = "Network Integration")
	void HandleUserConnected(int32 UserID, const FString& CharacterID);

	UFUNCTION(BlueprintCallable, Category = "Network Integration")
	void HandleUserDisconnected(int32 UserID);

	UFUNCTION(BlueprintCallable, Category = "Network Integration")
	void BindToNetworkManager();

private:
	// Helper methods
	AMultiplayerCharacter* SpawnCharacterAtLocation(int32 UserID, const FString& CharacterID, bool bIsLocal, const FVector& SpawnLocation = FVector::ZeroVector);
	FVector GetDefaultSpawnLocation();

	// Coordinate system conversion utilities
	UFUNCTION(BlueprintCallable, Category = "Coordinate Conversion")
	FVector ConvertServerToUECoordinates(float ServerX, float ServerY);
	
	UFUNCTION(BlueprintCallable, Category = "Coordinate Conversion")
	void ConvertUEToServerCoordinates(const FVector& UEPosition, float& OutServerX, float& OutServerY);

	// Game initialization flow
	UFUNCTION(BlueprintCallable, Category = "Game Initialization")
	void InitializeGame(const FString& ServerIP = TEXT("127.0.0.1"), int32 ServerPort = 8080);
	
	UFUNCTION(BlueprintCallable, Category = "Game Initialization")
	bool IsGameInitialized() const { return bGameInitialized; }

private:
	// Game initialization state
	bool bGameInitialized;
	bool bWaitingForUserID;
	FString PendingServerIP;
	int32 PendingServerPort;

	// Network event handlers
	UFUNCTION()
	void OnNetworkMessageReceived(const FNetworkMessage& Message);

	UFUNCTION()
	void OnUserIDAssigned(int32 UserID);

	UFUNCTION()
	void OnConnectionStatusChanged(bool bIsConnected);

	// Reference to NetworkManager
	UPROPERTY()
	class UNetworkManager* NetworkManager;
};
