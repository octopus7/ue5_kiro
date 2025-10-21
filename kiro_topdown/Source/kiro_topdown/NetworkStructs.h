#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "NetworkStructs.generated.h"

// Forward declarations for protobuf classes
namespace simpletcp {
    class NetworkMessage;
    class UserInfo;
    enum MessageType : int;
}

UENUM(BlueprintType)
enum class ENetworkMessageType : uint8
{
    MOVE = 0,
    POSITION = 1,
    USER_ID_ASSIGNMENT = 2,
    ALL_USERS_INFO = 3
};

USTRUCT(BlueprintType)
struct KIRO_TOPDOWN_API FUserInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    uint32 UserID = 0;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString CharacterID;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float CurrentX = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float CurrentY = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float TargetX = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float TargetY = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Speed = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsMoving = false;

    // Default constructor
    FUserInfo() = default;

    // Constructor from protobuf UserInfo
    explicit FUserInfo(const simpletcp::UserInfo& ProtoUserInfo);

    // Convert to protobuf UserInfo
    void ToProtobuf(simpletcp::UserInfo& OutProtoUserInfo) const;
};

USTRUCT(BlueprintType)
struct KIRO_TOPDOWN_API FNetworkMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ENetworkMessageType MessageType = ENetworkMessageType::MOVE;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString CharacterID;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    uint32 UserID = 0;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float StartX = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float StartY = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float TargetX = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float TargetY = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float CurrentX = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float CurrentY = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Speed = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsMoving = false;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FUserInfo> AllUsers;

    // Default constructor
    FNetworkMessage() = default;

    // Constructor from protobuf NetworkMessage
    explicit FNetworkMessage(const simpletcp::NetworkMessage& ProtoMessage);

    // Convert to protobuf NetworkMessage
    void ToProtobuf(simpletcp::NetworkMessage& OutProtoMessage) const;

    // Helper function to convert enum
    static ENetworkMessageType ConvertFromProtobuf(simpletcp::MessageType ProtoType);
    static simpletcp::MessageType ConvertToProtobuf(ENetworkMessageType UEType);
};