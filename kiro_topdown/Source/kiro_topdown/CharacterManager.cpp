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
	
	// Set default character class to AMultiplayerCharacter
	CharacterClass = AMultiplayerCharacter::StaticClass();
	
	// Bind to NetworkManager
	BindToNetworkManager();
	
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
vo
id UCharacterManager::HandleUserIDAssignment(uint32 UserID)
{
	UE_LOG(LogTemp, Log, TEXT("Handling UserID assignment: %d"), UserID);
	
	// Set local user ID and spawn local character
	LocalUserID = UserID;
	SpawnLocalCharacter(UserID);
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
			SpawnRemoteCharacter(UserInfo.UserID, UserInfo.CharacterID);
			
			// Set initial position and movement state
			AMultiplayerCharacter* Character = GetCharacterByUserID(UserInfo.UserID);
			if (Character)
			{
				FVector CurrentPos(UserInfo.CurrentX, UserInfo.CurrentY, 0.0f);
				FVector TargetPos(UserInfo.TargetX, UserInfo.TargetY, 0.0f);
				Character->UpdateRemoteMovement(CurrentPos, TargetPos, UserInfo.Speed);
			}
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
		
		UE_LOG(LogTemp, Log, TEXT("CharacterManager bound to NetworkManager"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NetworkManager not found - will retry binding"));
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
					FVector CurrentPos(Message.CurrentX, Message.CurrentY, 0.0f);
					FVector TargetPos(Message.TargetX, Message.TargetY, 0.0f);
					Character->UpdateRemoteMovement(CurrentPos, TargetPos, Message.Speed);
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
					FVector CurrentPos(Message.CurrentX, Message.CurrentY, 0.0f);
					FVector TargetPos(Message.TargetX, Message.TargetY, 0.0f);
					Character->UpdateRemoteMovement(CurrentPos, TargetPos, Message.Speed);
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
}void
 UCharacterManager::SendLocalCharacterMovement(const FVector& StartPosition, const FVector& TargetPosition)
{
	if (NetworkManager && NetworkManager->IsConnected())
	{
		// Convert 3D coordinates to 2D for server
		float StartX = StartPosition.X;
		float StartY = StartPosition.Y;
		float TargetX = TargetPosition.X;
		float TargetY = TargetPosition.Y;
		
		NetworkManager->SendMoveCommand(StartX, StartY, TargetX, TargetY);
		
		UE_LOG(LogTemp, Log, TEXT("Sent movement command: Start(%.2f, %.2f) -> Target(%.2f, %.2f)"), 
			StartX, StartY, TargetX, TargetY);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send movement - NetworkManager not connected"));
	}
}