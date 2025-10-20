using System.Collections.Concurrent;
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
        private readonly Timer _updateTimer;
        private readonly IPAddress _serverIp;
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
                    _clients[clientId] = tcpClient;
                    _characters[clientId] = new Character(clientId);
                    
                    Console.WriteLine($"Client connected: {clientId}");
                    _ = Task.Run(() => HandleClientAsync(clientId, tcpClient));
                }
                catch (ObjectDisposedException)
                {
                    break;
                }
            }
        }

        private async Task HandleClientAsync(string clientId, TcpClient client)
        {
            var buffer = new byte[1024];
            var stream = client.GetStream();

            try
            {
                while (client.Connected && _isRunning)
                {
                    var bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                    if (bytesRead == 0) break;

                    var messageJson = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    var message = Message.Deserialize(messageJson);

                    if (message != null && message.Type == MessageType.Move)
                    {
                        if (_characters.TryGetValue(clientId, out var character))
                        {
                            var startPos = new Vector2(message.StartX, message.StartY);
                            var targetPos = new Vector2(message.TargetX, message.TargetY);
                            character.StartMovement(startPos, targetPos);

                            Console.WriteLine($"Character {clientId} moving from ({startPos.X}, {startPos.Y}) to ({targetPos.X}, {targetPos.Y})");

                            // 다른 클라이언트들에게 이동 정보 전달
                            await BroadcastMessageAsync(message, clientId);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error handling client {clientId}: {ex.Message}");
            }
            finally
            {
                _clients.TryRemove(clientId, out _);
                _characters.TryRemove(clientId, out _);
                client.Close();
                Console.WriteLine($"Client disconnected: {clientId}");
            }
        }

        private async Task BroadcastMessageAsync(Message message, string senderId)
        {
            var messageJson = Message.Serialize(message);
            var data = Encoding.UTF8.GetBytes(messageJson);

            var tasks = new List<Task>();
            foreach (var kvp in _clients)
            {
                if (kvp.Key != senderId && kvp.Value.Connected)
                {
                    tasks.Add(SendToClientAsync(kvp.Value, data));
                }
            }

            await Task.WhenAll(tasks);
        }

        private async Task SendToClientAsync(TcpClient client, byte[] data)
        {
            try
            {
                await client.GetStream().WriteAsync(data, 0, data.Length);
            }
            catch
            {
                // 클라이언트 연결이 끊어진 경우 무시
            }
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