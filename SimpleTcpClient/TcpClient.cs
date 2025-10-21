using System.Net.Sockets;
using System.Text;
using System.Text.Json;

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

        public event Action<Message>? MessageReceived;
        public event Action<string>? StatusChanged;

        public async Task<bool> ConnectAsync(string host, int port)
        {
            try
            {
                _client = new TcpClient();
                await _client.ConnectAsync(host, port);
                _stream = _client.GetStream();
                _isConnected = true;

                _receiveTask = Task.Run(ReceiveMessagesAsync);
                StatusChanged?.Invoke("Connected");
                return true;
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke($"Connection failed: {ex.Message}");
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

            var json = Message.Serialize(message);
            var data = Encoding.UTF8.GetBytes(json);

            try
            {
                await _stream.WriteAsync(data, 0, data.Length);
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke($"Send error: {ex.Message}");
            }
        }

        private async Task ReceiveMessagesAsync()
        {
            var buffer = new byte[1024];

            try
            {
                while (_isConnected && _client?.Connected == true)
                {
                    var bytesRead = await _stream!.ReadAsync(buffer, 0, buffer.Length);
                    if (bytesRead == 0) break;

                    var json = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    var message = Message.Deserialize(json);

                    if (message != null)
                    {
                        MessageReceived?.Invoke(message);
                    }
                }
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke($"Receive error: {ex.Message}");
            }
        }

        public void Disconnect()
        {
            _isConnected = false;
            _stream?.Close();
            _client?.Close();
            StatusChanged?.Invoke("Disconnected");
        }
    }
}