using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Text;

namespace SimpleTcpClient
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

    public class GameTcpClient
    {
        private TcpClient? _client;
        private NetworkStream? _stream;
        private bool _isConnected;
        private Task? _receiveTask;
        private const int MaxMessageSize = 1_048_576;

        public event Action<Message>? MessageReceived;
        public event Action<string>? StatusChanged;
        public event Action<string>? LogGenerated;

        private void Log(string message)
        {
            LogGenerated?.Invoke($"[{DateTime.Now:HH:mm:ss}] {message}");
        }

        public async Task<bool> ConnectAsync(string host, int port)
        {
            try
            {
                Log($"Connecting to {host}:{port}");
                _client = new TcpClient();
                await _client.ConnectAsync(host, port);
                _stream = _client.GetStream();
                _isConnected = true;

                _receiveTask = Task.Run(ReceiveMessagesAsync);
                StatusChanged?.Invoke("Connected");
                Log("Connection established");
                return true;
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke($"Connection failed: {ex.Message}");
                Log($"Connection failed: {ex.Message}");
                return false;
            }
        }

        public async Task SendMoveCommandAsync(float startX, float startY, float targetX, float targetY)
        {
            if (!_isConnected || _stream == null) return;

            var message = new Message
            {
                Type = MessageType.Move,
                StartX = startX,
                StartY = startY,
                TargetX = targetX,
                TargetY = targetY
            };

            var payload = Message.Serialize(message);

            try
            {
                Log($"Sending move command Start=({startX:F1},{startY:F1}) Target=({targetX:F1},{targetY:F1}) len={payload.Length} bytes");
                await SendPacketAsync(payload);
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke($"Send error: {ex.Message}");
                Log($"Send error: {ex.Message}");
            }
        }

        private async Task SendPacketAsync(byte[] payload)
        {
            var stream = _stream;
            if (stream == null) return;

            var lengthPrefix = new byte[sizeof(int)];
            BinaryPrimitives.WriteInt32LittleEndian(lengthPrefix, payload.Length);

            await stream.WriteAsync(lengthPrefix);
            await stream.WriteAsync(payload);
        }

        private static async Task<bool> ReadExactAsync(NetworkStream stream, byte[] buffer, int count)
        {
            var offset = 0;
            while (offset < count)
            {
                var read = await stream.ReadAsync(buffer.AsMemory(offset, count - offset));
                if (read == 0)
                {
                    return false;
                }

                offset += read;
            }

            return true;
        }

        private async Task<byte[]?> ReadMessageAsync(NetworkStream stream)
        {
            var lengthBuffer = new byte[sizeof(int)];
            if (!await ReadExactAsync(stream, lengthBuffer, lengthBuffer.Length))
            {
                return null;
            }

            var length = BinaryPrimitives.ReadInt32LittleEndian(lengthBuffer);
            if (length <= 0 || length > MaxMessageSize)
            {
                throw new InvalidDataException($"Invalid message length: {length}");
            }

            var payload = new byte[length];
            if (!await ReadExactAsync(stream, payload, length))
            {
                return null;
            }

            return payload;
        }

        private async Task ReceiveMessagesAsync()
        {
            try
            {
                while (_isConnected && _client?.Connected == true)
                {
                    var stream = _stream;
                    if (stream == null)
                    {
                        break;
                    }

                    var payload = await ReadMessageAsync(stream);
                    if (payload == null)
                    {
                        Log("Stream closed while waiting for message");
                        break;
                    }

                    Log($"Received message len={payload.Length} bytes");
                    var message = Message.Deserialize(payload);

                    if (message != null)
                    {
                        Log($"Parsed message: {message.Type}");
                        MessageReceived?.Invoke(message);
                    }
                    else
                    {
                        Log("Failed to parse incoming message");
                    }
                }
            }
            catch (InvalidDataException ex)
            {
                StatusChanged?.Invoke($"Protocol error: {ex.Message}");
                Log($"Protocol error: {ex.Message}");
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke($"Receive error: {ex.Message}");
                Log($"Receive error: {ex.Message}");
            }
        }

        public void Disconnect()
        {
            _isConnected = false;
            _stream?.Close();
            _client?.Close();
            StatusChanged?.Invoke("Disconnected");
            Log("Disconnected from server");
        }
    }
}

