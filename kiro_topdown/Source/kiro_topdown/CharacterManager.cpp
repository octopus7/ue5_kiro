#include "CharacterManager.h"
#include "MultiplayerCharacter.h"
#include "NetworkManager.h"
#include "NetworkStructs.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void UCharacterManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LocalUserID = 0;
	Characters.Empty();
	NetworkManager = nullptr;
	
	// Initialize game state
	bGameInitialized = false;
	bWaitingForUserID = false;
	PendingServerIP = TEXT("127.0.0.1");
	PendingServerPort = 8080;
	
	// Set default character class to AMultiplayerCharacter
	CharacterClass = AMultiplayerCharacter::StaticClass();
	
	// Bind to NetworkManager
	BindToNetworkManager();
	
	// Start automatic game initialization after a short delay to ensure all systems are ready
	if (UWorld* World = GetWorld())
	{
		FTimerHandle InitTimer;
		World->GetTimerManager().SetTimer(InitTimer, [this]()
		{
			InitializeGame();
		}, 2.0f, false);
	}
	
	UE_LOG(LogTemp, Log, TEXT("CharacterManager initialized"));
}

void UCharacterManager::Deinitialize()
{
	// Clean up all characters
	for (auto& CharacterPair : Characters)
	{
		if (IsValid(CharacterPair.Value))
		{
			CharacterPair.Value->Destroy();
		}
	}
	Characters.Empty();
	
	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("CharacterManager deinitialized"));
}

void UCharacterManager::SpawnLocalCharacter(uint32 UserID)
{
	UE_LOG(LogTemp, Log, TEXT("Spawning local character for UserID: %d"), UserID);
	
	// Remove existing character if any
	RemoveCharacter(UserID);
	
	// Set as local user
	LocalUserID = UserID;
	
	// Spawn local character
	FString CharacterID = FString::Printf(TEXT("LocalPlayer_%d"), UserID);
	AMultiplayerCharacter* NewCharacter = SpawnCharacterAtLocation(UserID, CharacterID, true);
	
	if (NewCharacter)
	{
		Characters.Add(UserID, NewCharacter);
		UE_LOG(LogTemp, Log, TEXT("Local character spawned successfully for UserID: %d"), UserID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn local character for UserID: %d"), UserID);
	}
}

void UCharacterManager::SpawnRemoteCharacter(uint32 UserID, const FString& CharacterID)
{
	UE_LOG(LogTemp, Log, TEXT("Spawning remote character for UserID: %d, CharacterID: %s"), UserID, *CharacterID);
	
	// Don't spawn if this is the local player
	if (UserID == LocalUserID)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to spawn remote character for local UserID: %d"), UserID);
		return;
	}
	
	// Remove existing character if any
	RemoveCharacter(UserID);
	
	// Spawn remote character
	AMultiplayerCharacter* NewCharacter = SpawnCharacterAtLocation(UserID, CharacterID, false);
	
	if (NewCharacter)
	{
		Characters.Add(UserID, NewCharacter);
		UE_LOG(LogTemp, Log, TEXT("Remote character spawned successfully for UserID: %d"), UserID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn remote character for UserID: %d"), UserID);
	}
}

void UCharacterManager::RemoveCharacter(uint32 UserID)
{
	if (AMultiplayerCharacter** FoundCharacter = Characters.Find(UserID))
	{
		if (IsValid(*FoundCharacter))
		{
			UE_LOG(LogTemp, Log, TEXT("Removing character for UserID: %d"), UserID);
			(*FoundCharacter)->Destroy();
		}
		Characters.Remove(UserID);
	}
}

AMultiplayerCharacter* UCharacterManager::GetCharacterByUserID(uint32 UserID)
{
	if (AMultiplayerCharacter** FoundCharacter = Characters.Find(UserID))
	{
		return IsValid(*FoundCharacter) ? *FoundCharacter : nullptr;
	}
	return nullptr;
}

AMultiplayerCharacter* UCharacterManager::GetLocalCharacter()
{
	return GetCharacterByUserID(LocalUserID);
}

AMultiplayerCharacter* UCharacterManager::SpawnCharacterAtLocation(uint32 UserID, const FString& CharacterID, bool bIsLocal, const FVector& SpawnLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid world found for character spawning"));
		return nullptr;
	}
	
	if (!CharacterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterClass not set"));
		return nullptr;
	}
	
	FVector ActualSpawnLocation = SpawnLocation.IsZero() ? GetDefaultSpawnLocation() : SpawnLocation;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	AMultiplayerCharacter* NewCharacter = World->SpawnActor<AMultiplayerCharacter>(CharacterClass, ActualSpawnLocation, SpawnRotation, SpawnParams);
	
	if (NewCharacter)
	{
		// Initialize character properties
		NewCharacter->SetUserID(UserID);
		NewCharacter->SetCharacterID(CharacterID);
		NewCharacter->SetIsLocalPlayer(bIsLocal);
		
		UE_LOG(LogTemp, Log, TEXT("Character spawned at location: %s for UserID: %d (Local: %s)"), 
			*ActualSpawnLocation.ToString(), UserID, bIsLocal ? TEXT("true") : TEXT("false"));
	}
	
	return NewCharacter;
}

FVector UCharacterManager::GetDefaultSpawnLocation()
{
	// Default spawn location - can be customized based on game requirements
	return FVector(0.0f, 0.0f, 100.0f);
}
void UCharacterManager::HandleUserIDAssignment(uint32 UserID)
{
	UE_LOG(LogTemp, Log, TEXT("Handling UserID assignment: %d"), UserID);
	
	// Set local user ID and spawn local character
	LocalUserID = UserID;
	SpawnLocalCharacter(UserID);
	
	// Complete game initialization if we were waiting for user ID
	if (bWaitingForUserID)
	{
		bWaitingForUserID = false;
		bGameInitialized = true;
		UE_LOG(LogTemp, Log, TEXT("Game initialization completed with UserID: %d"), UserID);
	}
}

void UCharacterManager::HandleAllUsersInfo(const TArray<FUserInfo>& AllUsers)
{
	UE_LOG(LogTemp, Log, TEXT("Handling AllUsersInfo with %d users"), AllUsers.Num());
	
	// Clear existing remote characters (keep local character)
	TArray<uint32> UsersToRemove;
	for (auto& CharacterPair : Characters)
	{
		if (CharacterPair.Key != LocalUserID)
		{
			UsersToRemove.Add(CharacterPair.Key);
		}
	}
	
	for (uint32 UserIDToRemove : UsersToRemove)
	{
		RemoveCharacter(UserIDToRemove);
	}
	
	// Spawn characters for all users except local player
	for (const FUserInfo& UserInfo : AllUsers)
	{
		if (UserInfo.UserID != LocalUserID)
		{
			UE_LOG(LogTemp, Log, TEXT("Processing user %d (%s) at server position (%.2f, %.2f)"), 
				UserInfo.UserID, *UserInfo.CharacterID, UserInfo.CurrentX, UserInfo.CurrentY);
			
			SpawnRemoteCharacter(UserInfo.UserID, UserInfo.CharacterID);
			
			// Set initial position and movement state with coordinate conversion
			AMultiplayerCharacter* Character = GetCharacterByUserID(UserInfo.UserID);
			if (Character)
			{
				// Convert 2D server coordinates to 3D UE coordinates
				FVector CurrentPos = ConvertServerToUECoordinates(UserInfo.CurrentX, UserInfo.CurrentY);
				FVector TargetPos = ConvertServerToUECoordinates(UserInfo.TargetX, UserInfo.TargetY);
				Character->UpdateRemoteMovement(CurrentPos, TargetPos, UserInfo.Speed);
				
				UE_LOG(LogTemp, Log, TEXT("Set character %d position: Current(%.2f, %.2f, %.2f), Target(%.2f, %.2f, %.2f)"), 
					UserInfo.UserID, CurrentPos.X, CurrentPos.Y, CurrentPos.Z, TargetPos.X, TargetPos.Y, TargetPos.Z);
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Skipping local user %d in AllUsersInfo"), UserInfo.UserID);
		}
	}
}

void UCharacterManager::HandleUserConnected(uint32 UserID, const FString& CharacterID)
{
	UE_LOG(LogTemp, Log, TEXT("Handling user connected: UserID %d, CharacterID %s"), UserID, *CharacterID);
	
	// Don't spawn if this is the local player
	if (UserID != LocalUserID)
	{
		SpawnRemoteCharacter(UserID, CharacterID);
	}
}

void UCharacterManager::HandleUserDisconnected(uint32 UserID)
{
	UE_LOG(LogTemp, Log, TEXT("Handling user disconnected: UserID %d"), UserID);
	
	RemoveCharacter(UserID);
}

void UCharacterManager::BindToNetworkManager()
{
	// Get NetworkManager subsystem
	NetworkManager = GetGameInstance()->GetSubsystem<UNetworkManager>();
	
	if (NetworkManager)
	{
		// Bind to network events
		NetworkManager->OnMessageReceived.AddDynamic(this, &UCharacterManager::OnNetworkMessageReceived);
		NetworkManager->OnUserIDAssigned.AddDynamic(this, &UCharacterManager::OnUserIDAssigned);
		NetworkManager->OnConnectionStatusChanged.AddDynamic(this, &UCharacterManager::OnConnectionStatusChanged);
		
		UE_LOG(LogTemp, Log, TEXT("CharacterManager bound to NetworkManager"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NetworkManager not found - will retry binding"));
		
		// Retry binding after a short delay
		if (UWorld* World = GetWorld())
		{
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(RetryTimer, this, &UCharacterManager::BindToNetworkManager, 1.0f, false);
		}
	}
}

void UCharacterManager::OnNetworkMessageReceived(const FNetworkMessage& Message)
{
	// Handle different message types
	switch (Message.MessageType)
	{
		case ENetworkMessageType::ALL_USERS_INFO:
			HandleAllUsersInfo(Message.AllUsers);
			break;
			
		case ENetworkMessageType::MOVE:
			// Handle movement updates for remote characters
			if (Message.UserID != LocalUserID)
			{
				AMultiplayerCharacter* Character = GetCharacterByUserID(Message.UserID);
				if (Character)
				{
					// Convert 2D server coordinates to 3D UE coordinates
					FVector CurrentPos = ConvertServerToUECoordinates(Message.CurrentX, Message.CurrentY);
					FVector TargetPos = ConvertServerToUECoordinates(Message.TargetX, Message.TargetY);
					Character->UpdateRemoteMovement(CurrentPos, TargetPos, Message.Speed);
				}
				else
				{
					// Character doesn't exist, spawn it if we have character ID
					if (!Message.CharacterID.IsEmpty())
					{
						UE_LOG(LogTemp, Log, TEXT("Spawning character for unknown user %d during MOVE message"), Message.UserID);
						SpawnRemoteCharacter(Message.UserID, Message.CharacterID);
						
						// Try to update movement again
						Character = GetCharacterByUserID(Message.UserID);
						if (Character)
						{
							FVector CurrentPos = ConvertServerToUECoordinates(Message.CurrentX, Message.CurrentY);
							FVector TargetPos = ConvertServerToUECoordinates(Message.TargetX, Message.TargetY);
							Character->UpdateRemoteMovement(CurrentPos, TargetPos, Message.Speed);
						}
					}
				}
			}
			break;
			
		case ENetworkMessageType::USER_ID_ASSIGNMENT:
			// This is handled by OnUserIDAssigned delegate
			break;
			
		case ENetworkMessageType::POSITION:
			// Handle position updates for remote characters
			if (Message.UserID != LocalUserID)
			{
				AMultiplayerCharacter* Character = GetCharacterByUserID(Message.UserID);
				if (Character)
				{
					// Convert 2D server coordinates to 3D UE coordinates
					FVector CurrentPos = ConvertServerToUECoordinates(Message.CurrentX, Message.CurrentY);
					FVector TargetPos = ConvertServerToUECoordinates(Message.TargetX, Message.TargetY);
					Character->UpdateRemoteMovement(CurrentPos, TargetPos, Message.Speed);
				}
				else if (!Message.CharacterID.IsEmpty())
				{
					// Character doesn't exist, spawn it
					UE_LOG(LogTemp, Log, TEXT("Spawning character for unknown user %d during POSITION message"), Message.UserID);
					SpawnRemoteCharacter(Message.UserID, Message.CharacterID);
					
					// Try to update position again
					Character = GetCharacterByUserID(Message.UserID);
					if (Character)
					{
						FVector CurrentPos = ConvertServerToUECoordinates(Message.CurrentX, Message.CurrentY);
						FVector TargetPos = ConvertServerToUECoordinates(Message.TargetX, Message.TargetY);
						Character->UpdateRemoteMovement(CurrentPos, TargetPos, Message.Speed);
					}
				}
			}
			break;
			
		default:
			UE_LOG(LogTemp, Log, TEXT("Unhandled message type: %d"), (int32)Message.MessageType);
			break;
	}
}

void UCharacterManager::OnUserIDAssigned(uint32 UserID)
{
	HandleUserIDAssignment(UserID);
}

void UCharacterManager::OnConnectionStatusChanged(bool bIsConnected)
{
	UE_LOG(LogTemp, Log, TEXT("Connection status changed: %s"), bIsConnected ? TEXT("Connected") : TEXT("Disconnected"));
	
	if (!bIsConnected)
	{
		// Clear all remote characters when disconnected, but keep local character
		TArray<uint32> UsersToRemove;
		for (auto& CharacterPair : Characters)
		{
			if (CharacterPair.Key != LocalUserID)
			{
				UsersToRemove.Add(CharacterPair.Key);
			}
		}
		
		for (uint32 UserIDToRemove : UsersToRemove)
		{
			RemoveCharacter(UserIDToRemove);
		}
		
		UE_LOG(LogTemp, Log, TEXT("Removed %d remote characters due to disconnection"), UsersToRemove.Num());
		
		// Reset game initialization state if we were in the middle of initializing
		if (bWaitingForUserID)
		{
			bWaitingForUserID = false;
			bGameInitialized = false;
			UE_LOG(LogTemp, Warning, TEXT("Game initialization interrupted by disconnection"));
		}
	}
	else
	{
		// Connection established
		UE_LOG(LogTemp, Log, TEXT("Connection established successfully"));
	}
}

void UCharacterManager::SendLocalCharacterMovement(const FVector& StartPosition, const FVector& TargetPosition)
{
	if (NetworkManager && NetworkManager->IsConnected())
	{
		// Convert 3D UE coordinates to 2D server coordinates
		float StartX, StartY, TargetX, TargetY;
		ConvertUEToServerCoordinates(StartPosition, StartX, StartY);
		ConvertUEToServerCoordinates(TargetPosition, TargetX, TargetY);
		
		NetworkManager->SendMoveCommand(StartX, StartY, TargetX, TargetY);
		
		UE_LOG(LogTemp, Log, TEXT("Sent movement command: UE Start(%.2f, %.2f, %.2f) -> Server(%.2f, %.2f), UE Target(%.2f, %.2f, %.2f) -> Server(%.2f, %.2f)"), 
			StartPosition.X, StartPosition.Y, StartPosition.Z, StartX, StartY,
			TargetPosition.X, TargetPosition.Y, TargetPosition.Z, TargetX, TargetY);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send movement - NetworkManager not connected"));
	}
}

FVector UCharacterManager::ConvertServerToUECoordinates(float ServerX, float ServerY)
{
	// Convert 2D server coordinates (X, Y) to 3D UE coordinates (X, Y, Z)
	// Server uses 2D coordinate system, UE uses 3D with Z=0 for ground plane
	return FVector(ServerX, ServerY, 0.0f);
}

void UCharacterManager::ConvertUEToServerCoordinates(const FVector& UEPosition, float& OutServerX, float& OutServerY)
{
	// Convert 3D UE coordinates (X, Y, Z) to 2D server coordinates (X, Y)
	// Simply use X and Y components, ignore Z for 2D plane movement
	OutServerX = UEPosition.X;
	OutServerY = UEPosition.Y;
}

void UCharacterManager::InitializeGame(const FString& ServerIP, int32 ServerPort)
{
	UE_LOG(LogTemp, Log, TEXT("Initializing game with server %s:%d"), *ServerIP, ServerPort);
	
	if (bGameInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("Game already initialized"));
		return;
	}
	
	// Store connection parameters
	PendingServerIP = ServerIP;
	PendingServerPort = ServerPort;
	
	// Ensure NetworkManager is bound
	if (!NetworkManager)
	{
		BindToNetworkManager();
	}
	
	if (NetworkManager)
	{
		// Connect to server
		bWaitingForUserID = true;
		bool bConnected = NetworkManager->ConnectToServer(ServerIP, ServerPort);
		
		if (bConnected)
		{
			UE_LOG(LogTemp, Log, TEXT("Successfully connected to server, waiting for user ID assignment"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect to server %s:%d"), *ServerIP, ServerPort);
			bWaitingForUserID = false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkManager not available for game initialization"));
	}
}