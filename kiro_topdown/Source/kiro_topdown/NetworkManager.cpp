#include "NetworkManager.h"
#include "NetworkReceiver.h"
#include "Generated/network_message.pb.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UNetworkManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    TCPSocket = nullptr;
    NetworkReceiver = nullptr;
    bIsConnected = false;
    AssignedUserID = 0;
    
    // Initialize reconnection settings
    bShouldReconnect = false;
    ReconnectAttempts = 0;
    MaxReconnectAttempts = 5;
    ReconnectDelay = 1.0f;
    MaxReconnectDelay = 30.0f;
    
    UE_LOG(LogTemp, Log, TEXT("NetworkManager initialized"));
}

void UNetworkManager::Deinitialize()
{
    StopReconnection();
    DisconnectFromServer();
    Super::Deinitialize();
    
    UE_LOG(LogTemp, Log, TEXT("NetworkManager deinitialized"));
}

bool UNetworkManager::ConnectToServer(const FString& InServerIP, int32 Port)
{
    if (bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("Already connected to server"));
        return true;
    }
    
    ServerIP = InServerIP;
    ServerPort = Port;
    
    // Get socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get socket subsystem"));
        return false;
    }
    
    // Create TCP socket
    TCPSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TCPSocket"), false);
    if (!TCPSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create TCP socket"));
        return false;
    }
    
    // Create server address
    FIPv4Address IPv4Address;
    if (!FIPv4Address::Parse(ServerIP, IPv4Address))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid server IP address: %s"), *ServerIP);
        CleanupSocket();
        return false;
    }
    
    TSharedRef<FInternetAddr> ServerAddress = SocketSubsystem->CreateInternetAddr();
    ServerAddress->SetIp(IPv4Address.Value);
    ServerAddress->SetPort(Port);
    
    // Connect to server
    bool bConnected = TCPSocket->Connect(*ServerAddress);
    if (!bConnected)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect to server %s:%d"), *ServerIP, Port);
        CleanupSocket();
        return false;
    }
    
    bIsConnected = true;
    ReconnectAttempts = 0; // Reset reconnect attempts on successful connection
    UE_LOG(LogTemp, Log, TEXT("Successfully connected to server %s:%d"), *ServerIP, Port);
    
    // Start network receiver thread
    NetworkReceiver = new FNetworkReceiver(TCPSocket, &IncomingMessageQueue);
    NetworkReceiver->StartThread();
    
    // Start timers
    if (UWorld* World = GetWorld())
    {
        // Message processing timer
        World->GetTimerManager().SetTimer(MessageProcessingTimer, this, &UNetworkManager::ProcessMessageQueue, 0.016f, true);
        
        // Connection monitoring timer
        World->GetTimerManager().SetTimer(ConnectionMonitorTimer, this, &UNetworkManager::CheckConnectionStatus, 1.0f, true);
    }
    
    // Broadcast connection status change
    OnConnectionStatusChanged.Broadcast(true);
    
    return true;
}

void UNetworkManager::DisconnectFromServer()
{
    if (!bIsConnected)
    {
        return;
    }
    
    bIsConnected = false;
    
    // Clear timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MessageProcessingTimer);
        World->GetTimerManager().ClearTimer(ConnectionMonitorTimer);
        World->GetTimerManager().ClearTimer(ReconnectionTimer);
    }
    
    // Stop network receiver thread
    if (NetworkReceiver)
    {
        NetworkReceiver->StopThread();
        delete NetworkReceiver;
        NetworkReceiver = nullptr;
    }
    
    CleanupSocket();
    
    UE_LOG(LogTemp, Log, TEXT("Disconnected from server"));
    
    // Broadcast connection status change
    OnConnectionStatusChanged.Broadcast(false);
}

void UNetworkManager::SendMoveCommand(float StartX, float StartY, float TargetX, float TargetY)
{
    if (!bIsConnected)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot send move command - not connected to server"));
        return;
    }
    
    FNetworkMessage Message;
    Message.MessageType = ENetworkMessageType::MOVE;
    Message.UserID = AssignedUserID;
    Message.StartX = StartX;
    Message.StartY = StartY;
    Message.TargetX = TargetX;
    Message.TargetY = TargetY;
    
    SendMessage(Message);
}

void UNetworkManager::SendMessage(const FNetworkMessage& Message)
{
    if (!bIsConnected || !TCPSocket)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot send message - not connected to server"));
        return;
    }
    
    TArray<uint8> SerializedData = SerializeMessage(Message);
    if (SerializedData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to serialize message"));
        return;
    }
    
    if (!SendData(SerializedData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to send message to server"));
        // Connection might be lost, disconnect and attempt reconnection
        DisconnectFromServer();
        StartReconnection();
    }
}

TArray<uint8> UNetworkManager::SerializeMessage(const FNetworkMessage& Message)
{
    TArray<uint8> Result;
    
    try
    {
        // Convert UE message to protobuf
        simpletcp::NetworkMessage ProtoMessage;
        Message.ToProtobuf(ProtoMessage);
        
        // Serialize protobuf message
        std::string SerializedString;
        if (!ProtoMessage.SerializeToString(&SerializedString))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to serialize protobuf message"));
            return Result;
        }
        
        // Create message with 4-byte length prefix
        uint32 MessageLength = SerializedString.size();
        Result.SetNum(sizeof(uint32) + MessageLength);
        
        // Write length prefix (little-endian)
        FMemory::Memcpy(Result.GetData(), &MessageLength, sizeof(uint32));
        
        // Write message data
        FMemory::Memcpy(Result.GetData() + sizeof(uint32), SerializedString.data(), MessageLength);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Serialized message: Type=%d, Length=%d"), 
               (int32)Message.MessageType, MessageLength);
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("Exception during message serialization: %s"), 
               UTF8_TO_TCHAR(e.what()));
        Result.Empty();
    }
    
    return Result;
}

FNetworkMessage UNetworkManager::DeserializeMessage(const TArray<uint8>& Data)
{
    FNetworkMessage Result;
    
    if (Data.Num() < sizeof(uint32))
    {
        UE_LOG(LogTemp, Error, TEXT("Message data too small for length prefix"));
        return Result;
    }
    
    try
    {
        // Read length prefix
        uint32 MessageLength;
        FMemory::Memcpy(&MessageLength, Data.GetData(), sizeof(uint32));
        
        if (Data.Num() != sizeof(uint32) + MessageLength)
        {
            UE_LOG(LogTemp, Error, TEXT("Message length mismatch: Expected=%d, Actual=%d"), 
                   sizeof(uint32) + MessageLength, Data.Num());
            return Result;
        }
        
        // Deserialize protobuf message
        simpletcp::NetworkMessage ProtoMessage;
        std::string MessageData(reinterpret_cast<const char*>(Data.GetData() + sizeof(uint32)), MessageLength);
        
        if (!ProtoMessage.ParseFromString(MessageData))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to parse protobuf message"));
            return Result;
        }
        
        // Convert protobuf to UE message
        Result = FNetworkMessage(ProtoMessage);
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Deserialized message: Type=%d, UserID=%d"), 
               (int32)Result.MessageType, Result.UserID);
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("Exception during message deserialization: %s"), 
               UTF8_TO_TCHAR(e.what()));
    }
    
    return Result;
}

bool UNetworkManager::SendData(const TArray<uint8>& Data)
{
    if (!TCPSocket || !bIsConnected)
    {
        return false;
    }
    
    int32 BytesSent = 0;
    int32 TotalBytes = Data.Num();
    
    while (BytesSent < TotalBytes)
    {
        int32 BytesSentThisTime = 0;
        bool bSent = TCPSocket->Send(Data.GetData() + BytesSent, TotalBytes - BytesSent, BytesSentThisTime);
        
        if (!bSent || BytesSentThisTime <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to send data to server"));
            return false;
        }
        
        BytesSent += BytesSentThisTime;
    }
    
    return true;
}

void UNetworkManager::CleanupSocket()
{
    if (TCPSocket)
    {
        TCPSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(TCPSocket);
        TCPSocket = nullptr;
    }
}

void UNetworkManager::ProcessIncomingMessage(const TArray<uint8>& MessageData)
{
    FNetworkMessage Message = DeserializeMessage(MessageData);
    
    // Handle specific message types
    switch (Message.MessageType)
    {
        case ENetworkMessageType::USER_ID_ASSIGNMENT:
            HandleUserIDAssignment(Message);
            break;
            
        case ENetworkMessageType::ALL_USERS_INFO:
            HandleAllUsersInfo(Message);
            break;
            
        default:
            break;
    }
    
    // Broadcast message to listeners
    OnMessageReceived.Broadcast(Message);
}

void UNetworkManager::HandleUserIDAssignment(const FNetworkMessage& Message)
{
    AssignedUserID = Message.UserID;
    UE_LOG(LogTemp, Log, TEXT("Assigned User ID: %d"), AssignedUserID);
    
    OnUserIDAssigned.Broadcast(AssignedUserID);
}

void UNetworkManager::HandleAllUsersInfo(const FNetworkMessage& Message)
{
    UE_LOG(LogTemp, Log, TEXT("Received ALL_USERS_INFO with %d users"), Message.AllUsers.Num());
    
    for (const FUserInfo& UserInfo : Message.AllUsers)
    {
        UE_LOG(LogTemp, Log, TEXT("User %d (%s) at position (%.2f, %.2f)"), 
               UserInfo.UserID, *UserInfo.CharacterID, UserInfo.CurrentX, UserInfo.CurrentY);
    }
}

void UNetworkManager::ProcessMessageQueue()
{
    // Process all queued messages from the receiver thread
    TArray<uint8> MessageData;
    while (IncomingMessageQueue.Dequeue(MessageData))
    {
        ProcessIncomingMessage(MessageData);
    }
}v
oid UNetworkManager::StartReconnection()
{
    if (bShouldReconnect)
    {
        UE_LOG(LogTemp, Warning, TEXT("Reconnection already in progress"));
        return;
    }
    
    if (ReconnectAttempts >= MaxReconnectAttempts)
    {
        UE_LOG(LogTemp, Error, TEXT("Maximum reconnection attempts reached (%d)"), MaxReconnectAttempts);
        return;
    }
    
    bShouldReconnect = true;
    
    // Calculate exponential backoff delay
    float CurrentDelay = FMath::Min(ReconnectDelay * FMath::Pow(2.0f, ReconnectAttempts), MaxReconnectDelay);
    
    UE_LOG(LogTemp, Log, TEXT("Starting reconnection attempt %d/%d in %.1f seconds"), 
           ReconnectAttempts + 1, MaxReconnectAttempts, CurrentDelay);
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(ReconnectionTimer, this, &UNetworkManager::AttemptReconnection, CurrentDelay, false);
    }
}

void UNetworkManager::StopReconnection()
{
    bShouldReconnect = false;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReconnectionTimer);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Reconnection stopped"));
}

void UNetworkManager::CheckConnectionStatus()
{
    if (!bIsConnected || !TCPSocket)
    {
        return;
    }
    
    // Check if the receiver thread is still running
    if (NetworkReceiver && !NetworkReceiver->IsThreadRunning())
    {
        UE_LOG(LogTemp, Warning, TEXT("Network receiver thread stopped - connection lost"));
        DisconnectFromServer();
        StartReconnection();
        return;
    }
    
    // Check socket connection state
    ESocketConnectionState ConnectionState = TCPSocket->GetConnectionState();
    if (ConnectionState != SCS_Connected)
    {
        UE_LOG(LogTemp, Warning, TEXT("Socket connection lost - state: %d"), (int32)ConnectionState);
        DisconnectFromServer();
        StartReconnection();
    }
}

void UNetworkManager::AttemptReconnection()
{
    if (!bShouldReconnect)
    {
        return;
    }
    
    ReconnectAttempts++;
    UE_LOG(LogTemp, Log, TEXT("Attempting reconnection %d/%d to %s:%d"), 
           ReconnectAttempts, MaxReconnectAttempts, *ServerIP, ServerPort);
    
    bool bReconnected = ConnectToServer(ServerIP, ServerPort);
    
    if (bReconnected)
    {
        UE_LOG(LogTemp, Log, TEXT("Reconnection successful"));
        bShouldReconnect = false;
        ReconnectAttempts = 0;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Reconnection attempt %d failed"), ReconnectAttempts);
        
        if (ReconnectAttempts < MaxReconnectAttempts)
        {
            // Schedule next reconnection attempt
            StartReconnection();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("All reconnection attempts failed"));
            bShouldReconnect = false;
        }
    }
}