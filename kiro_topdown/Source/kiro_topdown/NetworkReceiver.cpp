#include "NetworkReceiver.h"
#include "Engine/Engine.h"
#include "HAL/PlatformProcess.h"

FNetworkReceiver::FNetworkReceiver(FSocket* InSocket, TQueue<TArray<uint8>, EQueueMode::Mpsc>* InMessageQueue)
    : Socket(InSocket)
    , MessageQueue(InMessageQueue)
    , Thread(nullptr)
    , bIsRunning(false)
    , bShouldStop(false)
{
}

FNetworkReceiver::~FNetworkReceiver()
{
    StopThread();
}

bool FNetworkReceiver::Init()
{
    if (!Socket || !MessageQueue)
    {
        UE_LOG(LogTemp, Error, TEXT("NetworkReceiver: Invalid socket or message queue"));
        return false;
    }
    
    bShouldStop = false;
    UE_LOG(LogTemp, Log, TEXT("NetworkReceiver thread initialized"));
    return true;
}

uint32 FNetworkReceiver::Run()
{
    bIsRunning = true;
    UE_LOG(LogTemp, Log, TEXT("NetworkReceiver thread started"));
    
    while (!bShouldStop)
    {
        TArray<uint8> MessageData;
        
        if (ReceiveMessage(MessageData))
        {
            // Add message to queue for main thread processing
            MessageQueue->Enqueue(MoveTemp(MessageData));
        }
        else
        {
            // If receive failed, check if connection is still valid
            if (!bShouldStop && !IsConnectionValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Connection lost, stopping receiver thread"));
                break;
            }
            
            if (!bShouldStop)
            {
                // Small delay before retrying to avoid busy waiting
                FPlatformProcess::Sleep(0.01f);
            }
        }
    }
    
    bIsRunning = false;
    UE_LOG(LogTemp, Log, TEXT("NetworkReceiver thread stopped"));
    return 0;
}

void FNetworkReceiver::Stop()
{
    bShouldStop = true;
    UE_LOG(LogTemp, Log, TEXT("NetworkReceiver thread stop requested"));
}

void FNetworkReceiver::Exit()
{
    bIsRunning = false;
    UE_LOG(LogTemp, Log, TEXT("NetworkReceiver thread exited"));
}

void FNetworkReceiver::StartThread()
{
    if (Thread)
    {
        UE_LOG(LogTemp, Warning, TEXT("NetworkReceiver thread already running"));
        return;
    }
    
    Thread = FRunnableThread::Create(this, TEXT("NetworkReceiver"), 0, TPri_Normal);
    if (!Thread)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create NetworkReceiver thread"));
    }
}

void FNetworkReceiver::StopThread()
{
    if (Thread)
    {
        bShouldStop = true;
        
        // Wait for thread to finish
        Thread->WaitForCompletion();
        
        // Clean up thread
        delete Thread;
        Thread = nullptr;
    }
}

bool FNetworkReceiver::ReceiveMessage(TArray<uint8>& OutMessage)
{
    if (!Socket || bShouldStop)
    {
        return false;
    }
    
    // First, receive the 4-byte length prefix
    uint32 MessageLength = 0;
    if (!ReceiveExactBytes(reinterpret_cast<uint8*>(&MessageLength), sizeof(uint32)))
    {
        return false;
    }
    
    // Validate message length (prevent excessive memory allocation)
    const uint32 MaxMessageSize = 1024 * 1024; // 1MB max message size
    if (MessageLength == 0 || MessageLength > MaxMessageSize)
    {
        HandleReceiveError(FString::Printf(TEXT("Invalid message length: %d"), MessageLength));
        return false;
    }
    
    // Allocate buffer for message data
    OutMessage.SetNum(sizeof(uint32) + MessageLength);
    
    // Copy length prefix to output buffer
    FMemory::Memcpy(OutMessage.GetData(), &MessageLength, sizeof(uint32));
    
    // Receive the actual message data
    if (!ReceiveExactBytes(OutMessage.GetData() + sizeof(uint32), MessageLength))
    {
        HandleReceiveError(TEXT("Failed to receive message data"));
        return false;
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Received message: Length=%d"), MessageLength);
    return true;
}

bool FNetworkReceiver::ReceiveExactBytes(uint8* Buffer, int32 BytesToReceive)
{
    if (!Socket || !Buffer || BytesToReceive <= 0)
    {
        return false;
    }
    
    int32 BytesReceived = 0;
    
    while (BytesReceived < BytesToReceive && !bShouldStop)
    {
        uint32 PendingDataSize = 0;
        
        // Check if there's data available
        if (!Socket->HasPendingData(PendingDataSize))
        {
            // No data available, check connection status
            ESocketConnectionState ConnectionState = Socket->GetConnectionState();
            if (ConnectionState != SCS_Connected)
            {
                HandleReceiveError(TEXT("Socket connection lost"));
                return false;
            }
            
            // Small delay to avoid busy waiting
            FPlatformProcess::Sleep(0.001f);
            continue;
        }
        
        int32 BytesRead = 0;
        bool bReceived = Socket->Recv(Buffer + BytesReceived, BytesToReceive - BytesReceived, BytesRead);
        
        if (!bReceived)
        {
            HandleReceiveError(TEXT("Socket receive failed"));
            return false;
        }
        
        if (BytesRead <= 0)
        {
            // Connection might be closed
            ESocketConnectionState ConnectionState = Socket->GetConnectionState();
            if (ConnectionState != SCS_Connected)
            {
                HandleReceiveError(TEXT("Connection closed by remote host"));
                return false;
            }
            
            // Continue trying
            FPlatformProcess::Sleep(0.001f);
            continue;
        }
        
        BytesReceived += BytesRead;
    }
    
    return BytesReceived == BytesToReceive;
}

void FNetworkReceiver::HandleReceiveError(const FString& ErrorMessage)
{
    if (!bShouldStop)
    {
        UE_LOG(LogTemp, Error, TEXT("NetworkReceiver error: %s"), *ErrorMessage);
    }
}

bool FNetworkReceiver::IsConnectionValid() const
{
    if (!Socket)
    {
        return false;
    }
    
    ESocketConnectionState ConnectionState = Socket->GetConnectionState();
    return ConnectionState == SCS_Connected;
}
