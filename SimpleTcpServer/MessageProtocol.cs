using System.Text.Json;

namespace SimpleTcpServer
{
    public enum MessageType
    {
        Move,
        Position,
        UserIdAssignment,
        AllUsersInfo
    }

    public class Message
    {
        public MessageType Type { get; set; }
        public string CharacterId { get; set; } = string.Empty;
        public ushort UserId { get; set; } // 16비트 부호없는 정수형 식별자
        public float StartX { get; set; }
        public float StartY { get; set; }
        public float TargetX { get; set; }
        public float TargetY { get; set; }
        public float CurrentX { get; set; }
        public float CurrentY { get; set; }
        public bool IsMoving { get; set; }

        public static string Serialize(Message message)
        {
            return JsonSerializer.Serialize(message);
        }

        public static Message? Deserialize(string json)
        {
            try
            {
                return JsonSerializer.Deserialize<Message>(json);
            }
            catch
            {
                return null;
            }
        }
    }
}