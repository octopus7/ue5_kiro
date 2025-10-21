#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Containers/Queue.h"
#include "Sockets.h"

/**
 * Network receiver thread for handling incoming TCP messages
 * Runs on a separate thread to avoid blocking the main game thread
 */
class KIRO_TOPDOWN_API FNetworkReceiver : public FRunnable
{
public:
    FNetworkReceiver(FSocket* InSocket, TQueue<TArray<uint8>, EQueueMode::Mpsc>* InMessageQueue);
    virtual ~FNetworkReceiver();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // Thread control
    void StartThread();
    void StopThread();
    bool IsThreadRunning() const { return bIsRunning; }

private:
    // Socket reference
    FSocket* Socket;
    
    // Message queue for thread-safe communication with main thread
    TQueue<TArray<uint8>, EQueueMode::Mpsc>* MessageQueue;
    
    // Thread control
    FRunnableThread* Thread;
    bool bIsRunning;
    bool bShouldStop;
    
    // Message receiving
    bool ReceiveMessage(TArray<uint8>& OutMessage);
    bool ReceiveExactBytes(uint8* Buffer, int32 BytesToReceive);
    
    // Error handling
    void HandleReceiveError(const FString& ErrorMessage);
    
    // Connection status
    bool IsConnectionValid() const;
};