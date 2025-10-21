#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NetworkStructs.h"
#include "MultiplayerGameMode.generated.h"

/**
 * Custom GameMode for multiplayer setup
 * Configures the game to use MultiplayerCharacter and MultiplayerPlayerController
 */
UCLASS()
class KIRO_TOPDOWN_API AMultiplayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMultiplayerGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

private:
	// Initialize network connection when game starts
	void InitializeNetworkConnection();
	
	// Network event handlers
	UFUNCTION()
	void OnConnectionStatusChanged(bool bIsConnected);
	
	UFUNCTION()
	void OnUserIDAssigned(int32 UserID);
	
	UFUNCTION()
	void OnMessageReceived(const FNetworkMessage& Message);
	
	// Update HUD with connection info
	void UpdateHUDConnectionStatus();
};