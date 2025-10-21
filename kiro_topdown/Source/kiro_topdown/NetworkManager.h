#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "NetworkStructs.h"
#include "NetworkReceiver.h"
#include "NetworkManager.generated.h"

// Forward declarations
class FSocket;
class FRunnableThread;
class FNetworkReceiver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FNetworkMessage&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChanged, bool, bIsConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserIDAssigned, uint32, UserID);

/**
 * TCP Network Manager for handling communication with the C# server
 * Implements GameInstanceSubsystem for persistent connection across levels
 */
UCLASS(BlueprintType, Blueprintable)
class KIRO_TOPDOWN_API UNetworkManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Connection Management
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool ConnectToServer(const FString& ServerIP = TEXT("127.0.0.1"), int32 Port = 8080);
    
    UFUNCTION(BlueprintCallable, Category = "Network")
    void DisconnectFromServer();
    
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool IsConnected() const { return bIsConnected; }

    // Message Sending
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendMoveCommand(float StartX, float StartY, float TargetX, float TargetY);
    
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendMessage(const FNetworkMessage& Message);

    // Event Delegates
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnMessageReceived OnMessageReceived;
    
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnConnectionStatusChanged OnConnectionStatusChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Network Events")
    FOnUserIDAssigned OnUserIDAssigned;

    // Connection Management
    UFUNCTION(BlueprintCallable, Category = "Network")
    void StartReconnection();
    
    UFUNCTION(BlueprintCallable, Category = "Network")
    void StopReconnection();

    // Getters
    UFUNCTION(BlueprintCallable, Category = "Network")
    uint32 GetAssignedUserID() const { return AssignedUserID; }
    
    UFUNCTION(BlueprintCallable, Category = "Network")
    int32 GetReconnectAttempts() const { return ReconnectAttempts; }

protected:
    // TCP Socket
    FSocket* TCPSocket;
    
    // Network receiver thread
    FNetworkReceiver* NetworkReceiver;
    
    // Connection state
    bool bIsConnected;
    FString ServerIP;
    int32 ServerPort;
    uint32 AssignedUserID;
    
    // Reconnection management
    bool bShouldReconnect;
    int32 ReconnectAttempts;
    int32 MaxReconnectAttempts;
    float ReconnectDelay;
    float MaxReconnectDelay;
    
    // Message processing
    void ProcessIncomingMessage(const TArray<uint8>& MessageData);
    TArray<uint8> SerializeMessage(const FNetworkMessage& Message);
    FNetworkMessage DeserializeMessage(const TArray<uint8>& Data);
    
    // Socket utilities
    bool SendData(const TArray<uint8>& Data);
    void CleanupSocket();
    
    // Connection monitoring
    void CheckConnectionStatus();
    void AttemptReconnection();
    
    // Message handling
    void HandleUserIDAssignment(const FNetworkMessage& Message);
    void HandleAllUsersInfo(const FNetworkMessage& Message);
    
private:
    // Thread-safe message queue for main thread processing
    TQueue<TArray<uint8>, EQueueMode::Mpsc> IncomingMessageQueue;
    
    // Process queued messages on main thread
    void ProcessMessageQueue();
    
    // Timer for processing messages
    FTimerHandle MessageProcessingTimer;
    
    // Timer for connection monitoring and reconnection
    FTimerHandle ConnectionMonitorTimer;
    FTimerHandle ReconnectionTimer;
};