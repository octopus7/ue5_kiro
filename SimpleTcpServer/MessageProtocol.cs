using System.Collections.Generic;
using System.IO;
using System.Text;

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
        public float Speed { get; set; } = 100.0f; // 이동 속도
        public bool IsMoving { get; set; }
        public List<UserInfo>? AllUsers { get; set; } // 모든 유저 정보

        public static byte[] Serialize(Message message)
        {
            using var stream = new MemoryStream();
            using (var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: true))
            {
                writer.Write((byte)message.Type);
                WriteString(writer, message.CharacterId);
                writer.Write(message.UserId);
                writer.Write(message.StartX);
                writer.Write(message.StartY);
                writer.Write(message.TargetX);
                writer.Write(message.TargetY);
                writer.Write(message.CurrentX);
                writer.Write(message.CurrentY);
                writer.Write(message.Speed);
                writer.Write(message.IsMoving);

                var allUsers = message.AllUsers;
                if (allUsers == null)
                {
                    writer.Write(0);
                }
                else
                {
                    writer.Write(allUsers.Count);
                    foreach (var user in allUsers)
                    {
                        writer.Write(user.UserId);
                        WriteString(writer, user.CharacterId);
                        writer.Write(user.CurrentX);
                        writer.Write(user.CurrentY);
                        writer.Write(user.TargetX);
                        writer.Write(user.TargetY);
                        writer.Write(user.Speed);
                        writer.Write(user.IsMoving);
                    }
                }
            }

            return stream.ToArray();
        }

        public static Message? Deserialize(byte[] data)
        {
            try
            {
                using var stream = new MemoryStream(data, writable: false);
                using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);

                var message = new Message
                {
                    Type = (MessageType)reader.ReadByte(),
                    CharacterId = ReadString(reader),
                    UserId = reader.ReadUInt16(),
                    StartX = reader.ReadSingle(),
                    StartY = reader.ReadSingle(),
                    TargetX = reader.ReadSingle(),
                    TargetY = reader.ReadSingle(),
                    CurrentX = reader.ReadSingle(),
                    CurrentY = reader.ReadSingle(),
                    Speed = reader.ReadSingle(),
                    IsMoving = reader.ReadBoolean()
                };

                var count = reader.ReadInt32();
                if (count < 0)
                {
                    throw new InvalidDataException("Invalid AllUsers count");
                }

                if (count > 0)
                {
                    var users = new List<UserInfo>(count);
                    for (var i = 0; i < count; i++)
                    {
                        var user = new UserInfo
                        {
                            UserId = reader.ReadUInt16(),
                            CharacterId = ReadString(reader),
                            CurrentX = reader.ReadSingle(),
                            CurrentY = reader.ReadSingle(),
                            TargetX = reader.ReadSingle(),
                            TargetY = reader.ReadSingle(),
                            Speed = reader.ReadSingle(),
                            IsMoving = reader.ReadBoolean()
                        };
                        users.Add(user);
                    }

                    message.AllUsers = users;
                }

                if (stream.Position != stream.Length)
                {
                    throw new InvalidDataException("Trailing data detected");
                }

                return message;
            }
            catch
            {
                return null;
            }
        }

        private static void WriteString(BinaryWriter writer, string? value)
        {
            var text = value ?? string.Empty;
            var bytes = Encoding.UTF8.GetBytes(text);
            writer.Write(bytes.Length);
            writer.Write(bytes);
        }

        private static string ReadString(BinaryReader reader)
        {
            var length = reader.ReadInt32();
            if (length < 0)
            {
                throw new InvalidDataException("Negative string length");
            }

            var bytes = reader.ReadBytes(length);
            if (bytes.Length != length)
            {
                throw new EndOfStreamException("Unexpected end of stream while reading string");
            }

            return bytes.Length == 0 ? string.Empty : Encoding.UTF8.GetString(bytes);
        }
    }

    public class UserInfo
    {
        public ushort UserId { get; set; }
        public string CharacterId { get; set; } = string.Empty;
        public float CurrentX { get; set; }
        public float CurrentY { get; set; }
        public float TargetX { get; set; }
        public float TargetY { get; set; }
        public float Speed { get; set; }
        public bool IsMoving { get; set; }
    }
}
