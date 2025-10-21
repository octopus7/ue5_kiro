#include "MultiplayerGameMode.h"
#include "MultiplayerCharacter.h"
#include "MultiplayerPlayerController.h"
#include "MultiplayerHUD.h"
#include "NetworkManager.h"
#include "CharacterManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AMultiplayerGameMode::AMultiplayerGameMode()
{
	// Set default pawn class to MultiplayerCharacter
	DefaultPawnClass = AMultiplayerCharacter::StaticClass();
	
	// Set default player controller class to MultiplayerPlayerController
	PlayerControllerClass = AMultiplayerPlayerController::StaticClass();
	
	// Set default HUD class to MultiplayerHUD
	HUDClass = AMultiplayerHUD::StaticClass();
	
	UE_LOG(LogTemp, Log, TEXT("MultiplayerGameMode initialized with MultiplayerCharacter, MultiplayerPlayerController, and MultiplayerHUD"));
}

void AMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("MultiplayerGameMode BeginPlay - Initializing network connection"));
	
	// Initialize network connection when game starts
	InitializeNetworkConnection();
}

void AMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	UE_LOG(LogTemp, Log, TEXT("Player logged in: %s"), NewPlayer ? *NewPlayer->GetName() : TEXT("NULL"));
}

void AMultiplayerGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	UE_LOG(LogTemp, Log, TEXT("Player logged out: %s"), Exiting ? *Exiting->GetName() : TEXT("NULL"));
}

void AMultiplayerGameMode::InitializeNetworkConnection()
{
	// Get NetworkManager subsystem and connect to server
	if (UNetworkManager* NetworkManager = GetGameInstance()->GetSubsystem<UNetworkManager>())
	{
		// Bind to connection status events
		NetworkManager->OnConnectionStatusChanged.AddDynamic(this, &AMultiplayerGameMode::OnConnectionStatusChanged);
		NetworkManager->OnUserIDAssigned.AddDynamic(this, &AMultiplayerGameMode::OnUserIDAssigned);
		NetworkManager->OnMessageReceived.AddDynamic(this, &AMultiplayerGameMode::OnMessageReceived);
		
		// Connect to local server (can be configured later)
		FString ServerIP = TEXT("127.0.0.1");
		int32 ServerPort = 8080;
		
		UE_LOG(LogTemp, Log, TEXT("Attempting to connect to server %s:%d"), *ServerIP, ServerPort);
		
		bool bConnected = NetworkManager->ConnectToServer(ServerIP, ServerPort);
		if (bConnected)
		{
			UE_LOG(LogTemp, Log, TEXT("Successfully initiated connection to server"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to initiate connection to server"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkManager subsystem not found"));
	}
}
void
 AMultiplayerGameMode::OnConnectionStatusChanged(bool bIsConnected)
{
	UE_LOG(LogTemp, Log, TEXT("Connection status changed: %s"), bIsConnected ? TEXT("Connected") : TEXT("Disconnected"));
	UpdateHUDConnectionStatus();
}

void AMultiplayerGameMode::OnUserIDAssigned(int32 UserID)
{
	UE_LOG(LogTemp, Log, TEXT("User ID assigned: %d"), UserID);
	UpdateHUDConnectionStatus();
}

void AMultiplayerGameMode::OnMessageReceived(const FNetworkMessage& Message)
{
	// Handle specific message types that affect UI
	if (Message.MessageType == ENetworkMessageType::ALL_USERS_INFO)
	{
		// Update user count in HUD based on AllUsers array
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AMultiplayerHUD* MultiplayerHUD = Cast<AMultiplayerHUD>(PC->GetHUD()))
			{
				MultiplayerHUD->UpdateUserCount(Message.AllUsers.Num());
			}
		}
	}
}

void AMultiplayerGameMode::UpdateHUDConnectionStatus()
{
	// Get the first player controller and update HUD
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AMultiplayerHUD* MultiplayerHUD = Cast<AMultiplayerHUD>(PC->GetHUD()))
		{
			if (UNetworkManager* NetworkManager = GetGameInstance()->GetSubsystem<UNetworkManager>())
			{
				bool bIsConnected = NetworkManager->IsConnected();
				FString StatusMessage;
				
				if (bIsConnected)
				{
					int32 UserID = NetworkManager->GetAssignedUserID();
					StatusMessage = FString::Printf(TEXT("Connected (ID: %d)"), UserID);
				}
				else
				{
					int32 ReconnectAttempts = NetworkManager->GetReconnectAttempts();
					if (ReconnectAttempts > 0)
					{
						StatusMessage = FString::Printf(TEXT("Reconnecting... (%d)"), ReconnectAttempts);
					}
					else
					{
						StatusMessage = TEXT("Disconnected");
					}
				}
				
				MultiplayerHUD->UpdateConnectionStatus(bIsConnected, StatusMessage);
				
				// Update user count (this would need to be tracked from ALL_USERS_INFO messages)
				// For now, just show 1 if connected
				MultiplayerHUD->UpdateUserCount(bIsConnected ? 1 : 0);
			}
		}
	}
}