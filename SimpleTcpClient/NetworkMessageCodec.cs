using Google.Protobuf;
using SimpleTcp.Shared;

namespace SimpleTcpClient;

internal static class NetworkMessageCodec
{
    public static byte[] Serialize(NetworkMessage message)
    {
        return message.ToByteArray();
    }

    public static NetworkMessage? Deserialize(byte[] data)
    {
        try
        {
            return NetworkMessage.Parser.ParseFrom(data);
        }
        catch (InvalidProtocolBufferException)
        {
            return null;
        }
    }
}
