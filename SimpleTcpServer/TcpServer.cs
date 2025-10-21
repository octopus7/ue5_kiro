using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Numerics;
using System.Text;
namespace SimpleTcpServer
{
    public class TcpServer
    {
        private readonly TcpListener _listener;
        private readonly ConcurrentDictionary<string, TcpClient> _clients = new();
        private readonly ConcurrentDictionary<string, Character> _characters = new();
        private readonly ConcurrentDictionary<string, ushort> _clientUserIds = new();
        private readonly Timer _updateTimer;
        private readonly IPAddress _serverIp;
        private const int MaxMessageSize = 1_048_576;
        private ushort _nextUserId = 1000; // 1000부터 시작
        private readonly object _userIdLock = new object();
        private bool _isRunning;
        public TcpServer(int port)
        {
            _serverIp = GetLocalIPAddress();
            _listener = new TcpListener(_serverIp, port);
            _updateTimer = new Timer(UpdateCharacters, null, TimeSpan.Zero, TimeSpan.FromMilliseconds(16)); // ~60 FPS
        }

        private static IPAddress GetLocalIPAddress()
        {
            try
            {
                // 네트워크 인터페이스를 통해 활성화된 IP 주소 찾기
                foreach (NetworkInterface ni in NetworkInterface.GetAllNetworkInterfaces())
                {
                    if (ni.OperationalStatus == OperationalStatus.Up && 
                        ni.NetworkInterfaceType != NetworkInterfaceType.Loopback)
                    {
                        foreach (UnicastIPAddressInformation ip in ni.GetIPProperties().UnicastAddresses)
                        {
                            if (ip.Address.AddressFamily == AddressFamily.InterNetwork)
                            {
                                return ip.Address;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error getting local IP address: {ex.Message}");
            }
            // 대체 방법: DNS를 통해 로컬 IP 주소 얻기
            try
            {
                using (Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, 0))
                {
                    socket.Connect("8.8.8.8", 65530);
                    IPEndPoint? endPoint = socket.LocalEndPoint as IPEndPoint;
                    if (endPoint != null)
                    {
                        return endPoint.Address;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error getting local IP address (fallback): {ex.Message}");
            }
            // 최후의 수단으로 localhost 반환
            return IPAddress.Loopback;
        }
        public async Task StartAsync()
        {
            _listener.Start();
            _isRunning = true;
            var endpoint = (IPEndPoint)_listener.LocalEndpoint;
            Console.WriteLine($"Server started on {endpoint.Address}:{endpoint.Port}");
            Console.WriteLine($"Clients can connect to: {_serverIp}:{endpoint.Port}");
            while (_isRunning)
            {
                try
                {
                    var tcpClient = await _listener.AcceptTcpClientAsync();
                    var clientId = Guid.NewGuid().ToString();
                    ushort userId;
                    
                    lock (_userIdLock)
                    {
                        userId = _nextUserId++;
                        if (_nextUserId > 65535) _nextUserId = 1000; // 16비트 범위 초과시 다시 1000부터
                    }
                    
                    _clients[clientId] = tcpClient;
                    _clientUserIds[clientId] = userId;
                    _characters[clientId] = new Character(clientId, userId);
                    
                    Console.WriteLine($"Client connected: {clientId} (UserId: {userId})");
                    
                    // 클라이언트에게 UserId 전송 및 모든 유저 정보 전송
                    _ = Task.Run(async () =>
                    {
                        await SendUserIdAssignmentAsync(tcpClient, userId);
                        await SendAllUsersInfoAsync(tcpClient);
                        await HandleClientAsync(clientId, tcpClient);
                    });
                }
                catch (ObjectDisposedException)
                {
                    break;
                }
            }
        }

        private async Task HandleClientAsync(string clientId, TcpClient client)
        {
            var stream = client.GetStream();
            try
            {
                while (client.Connected && _isRunning)
                {
                    var messageJson = await ReadMessageAsync(stream);
                    if (messageJson == null)
                    {
                        break;
                    }
                    var message = Message.Deserialize(messageJson);
                    if (message == null)
                    {
                        Console.WriteLine($"Failed to deserialize message from {clientId}");
                        continue;
                    }
                    if (message.Type == MessageType.Move &&
                        _characters.TryGetValue(clientId, out var character) &&
                        _clientUserIds.TryGetValue(clientId, out var userId))
                    {
                        var startPos = new Vector2(message.StartX, message.StartY);
                        var targetPos = new Vector2(message.TargetX, message.TargetY);
                        character.StartMovement(startPos, targetPos);
                        Console.WriteLine($"Character {userId} moving from ({startPos.X}, {startPos.Y}) to ({targetPos.X}, {targetPos.Y})");
                        message.UserId = userId;
                        message.CharacterId = clientId;
                        message.Speed = character.Speed;
                        message.CurrentX = character.Position.X;
                        message.CurrentY = character.Position.Y;
                        await BroadcastMessageAsync(message, clientId);
                    }
                }
            }
            catch (InvalidDataException ex)
            {
                Console.WriteLine($"Protocol error for client {clientId}: {ex.Message}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error handling client {clientId}: {ex.Message}");
            }
            finally
            {
                _clients.TryRemove(clientId, out _);
                _characters.TryRemove(clientId, out _);
                if (_clientUserIds.TryRemove(clientId, out var userId))
                {
                    Console.WriteLine($"Client disconnected: {clientId} (UserId: {userId})");
                }
                else
                {
                    Console.WriteLine($"Client disconnected: {clientId}");
                }
                client.Close();
            }
        }

        private async Task BroadcastMessageAsync(Message message, string senderId)
        {
            var messageJson = Message.Serialize(message);
            var payload = Encoding.UTF8.GetBytes(messageJson);
            var packet = BuildPacket(payload);
            var tasks = new List<Task>();
            foreach (var kvp in _clients)
            {
                if (kvp.Key != senderId && kvp.Value.Connected)
                {
                    tasks.Add(SendRawPacketAsync(kvp.Value, packet));
                }
            }
            await Task.WhenAll(tasks);
        }

        private async Task SendUserIdAssignmentAsync(TcpClient client, ushort userId)
        {
            try
            {
                var message = new Message
                {
                    Type = MessageType.UserIdAssignment,
                    UserId = userId
                };
                var messageJson = Message.Serialize(message);
                var payload = Encoding.UTF8.GetBytes(messageJson);
                var packet = BuildPacket(payload);
                await SendRawPacketAsync(client, packet);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error sending UserId assignment: {ex.Message}");
            }
        }

        private async Task SendAllUsersInfoAsync(TcpClient client)
        {
            try
            {
                var allUsers = new List<UserInfo>();
                foreach (var kvp in _characters)
                {
                    var character = kvp.Value;
                    if (_clientUserIds.TryGetValue(kvp.Key, out var userId))
                    {
                        allUsers.Add(new UserInfo
                        {
                            UserId = userId,
                            CharacterId = kvp.Key,
                            CurrentX = character.Position.X,
                            CurrentY = character.Position.Y,
                            TargetX = character.TargetPosition.X,
                            TargetY = character.TargetPosition.Y,
                            Speed = character.Speed,
                            IsMoving = character.IsMoving
                        });
                    }
                }
                var message = new Message
                {
                    Type = MessageType.AllUsersInfo,
                    AllUsers = allUsers
                };
                var messageJson = Message.Serialize(message);
                var payload = Encoding.UTF8.GetBytes(messageJson);
                var packet = BuildPacket(payload);
                await SendRawPacketAsync(client, packet);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error sending all users info: {ex.Message}");
            }
        }

        private static byte[] BuildPacket(byte[] payload)
        {
            var packet = new byte[sizeof(int) + payload.Length];
            BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(0, sizeof(int)), payload.Length);
            Buffer.BlockCopy(payload, 0, packet, sizeof(int), payload.Length);
            return packet;
        }

        private static async Task SendRawPacketAsync(TcpClient client, byte[] packet)
        {
            try
            {
                await client.GetStream().WriteAsync(packet, 0, packet.Length);
            }
            catch
            {
                // 클라이언트 연결이 끊어진 경우 무시
            }
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

        private async Task<string?> ReadMessageAsync(NetworkStream stream)
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
            return Encoding.UTF8.GetString(payload);
        }

        private void UpdateCharacters(object? state)
        {
            foreach (var character in _characters.Values)
            {
                character.Update();
            }
        }
        public void Stop()
        {
            _isRunning = false;
            _listener?.Stop();
            _updateTimer?.Dispose();
        }
    }
}

