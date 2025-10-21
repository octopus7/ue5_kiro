#include "NetworkStructs.h"
#include "Generated/network_message.pb.h"

// FUserInfo implementation
FUserInfo::FUserInfo(const simpletcp::UserInfo& ProtoUserInfo)
{
    UserID = ProtoUserInfo.user_id();
    CharacterID = FString(ProtoUserInfo.character_id().c_str());
    CurrentX = ProtoUserInfo.current_x();
    CurrentY = ProtoUserInfo.current_y();
    TargetX = ProtoUserInfo.target_x();
    TargetY = ProtoUserInfo.target_y();
    Speed = ProtoUserInfo.speed();
    bIsMoving = ProtoUserInfo.is_moving();
}

void FUserInfo::ToProtobuf(simpletcp::UserInfo& OutProtoUserInfo) const
{
    OutProtoUserInfo.set_user_id(UserID);
    OutProtoUserInfo.set_character_id(TCHAR_TO_UTF8(*CharacterID));
    OutProtoUserInfo.set_current_x(CurrentX);
    OutProtoUserInfo.set_current_y(CurrentY);
    OutProtoUserInfo.set_target_x(TargetX);
    OutProtoUserInfo.set_target_y(TargetY);
    OutProtoUserInfo.set_speed(Speed);
    OutProtoUserInfo.set_is_moving(bIsMoving);
}

// FNetworkMessage implementation
FNetworkMessage::FNetworkMessage(const simpletcp::NetworkMessage& ProtoMessage)
{
    MessageType = ConvertFromProtobuf(ProtoMessage.type());
    CharacterID = FString(ProtoMessage.character_id().c_str());
    UserID = ProtoMessage.user_id();
    StartX = ProtoMessage.start_x();
    StartY = ProtoMessage.start_y();
    TargetX = ProtoMessage.target_x();
    TargetY = ProtoMessage.target_y();
    CurrentX = ProtoMessage.current_x();
    CurrentY = ProtoMessage.current_y();
    Speed = ProtoMessage.speed();
    bIsMoving = ProtoMessage.is_moving();

    // Convert all users
    AllUsers.Empty();
    for (int32 i = 0; i < ProtoMessage.all_users_size(); ++i)
    {
        const simpletcp::UserInfo& ProtoUser = ProtoMessage.all_users(i);
        AllUsers.Add(FUserInfo(ProtoUser));
    }
}

void FNetworkMessage::ToProtobuf(simpletcp::NetworkMessage& OutProtoMessage) const
{
    OutProtoMessage.set_type(ConvertToProtobuf(MessageType));
    OutProtoMessage.set_character_id(TCHAR_TO_UTF8(*CharacterID));
    OutProtoMessage.set_user_id(UserID);
    OutProtoMessage.set_start_x(StartX);
    OutProtoMessage.set_start_y(StartY);
    OutProtoMessage.set_target_x(TargetX);
    OutProtoMessage.set_target_y(TargetY);
    OutProtoMessage.set_current_x(CurrentX);
    OutProtoMessage.set_current_y(CurrentY);
    OutProtoMessage.set_speed(Speed);
    OutProtoMessage.set_is_moving(bIsMoving);

    // Convert all users
    OutProtoMessage.clear_all_users();
    for (const FUserInfo& UserInfo : AllUsers)
    {
        simpletcp::UserInfo* ProtoUser = OutProtoMessage.add_all_users();
        UserInfo.ToProtobuf(*ProtoUser);
    }
}

ENetworkMessageType FNetworkMessage::ConvertFromProtobuf(simpletcp::MessageType ProtoType)
{
    switch (ProtoType)
    {
        case simpletcp::MOVE:
            return ENetworkMessageType::MOVE;
        case simpletcp::POSITION:
            return ENetworkMessageType::POSITION;
        case simpletcp::USER_ID_ASSIGNMENT:
            return ENetworkMessageType::USER_ID_ASSIGNMENT;
        case simpletcp::ALL_USERS_INFO:
            return ENetworkMessageType::ALL_USERS_INFO;
        default:
            return ENetworkMessageType::MOVE;
    }
}

simpletcp::MessageType FNetworkMessage::ConvertToProtobuf(ENetworkMessageType UEType)
{
    switch (UEType)
    {
        case ENetworkMessageType::MOVE:
            return simpletcp::MOVE;
        case ENetworkMessageType::POSITION:
            return simpletcp::POSITION;
        case ENetworkMessageType::USER_ID_ASSIGNMENT:
            return simpletcp::USER_ID_ASSIGNMENT;
        case ENetworkMessageType::ALL_USERS_INFO:
            return simpletcp::ALL_USERS_INFO;
        default:
            return simpletcp::MOVE;
    }
}