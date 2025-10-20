using System.Collections.Concurrent;
using System.Net;
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
        private bool _isRunning;

        public TcpServer(int port)
        {
            _listener = new TcpListener(IPAddress.Any, port);
            _updateTimer = new Timer(UpdateCharacters, null, TimeSpan.Zero, TimeSpan.FromMilliseconds(16)); // ~60 FPS
        }

        public async Task StartAsync()
        {
            _listener.Start();
            _isRunning = true;
            Console.WriteLine($"Server started on port {((IPEndPoint)_listener.LocalEndpoint).Port}");

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