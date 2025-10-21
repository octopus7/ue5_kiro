using System.Buffers.Binary;
using System.IO;
using System.Net.Sockets;
using SimpleTcp.Shared;

namespace SimpleTcpClient
{
    public class GameTcpClient
    {
        private TcpClient? _client;
        private NetworkStream? _stream;
        private bool _isConnected;
        private Task? _receiveTask;
        private const int MaxMessageSize = 1_048_576;

        public event Action<NetworkMessage>? MessageReceived;
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

            var message = new NetworkMessage
            {
                Type = MessageType.Move,
                StartX = startX,
                StartY = startY,
                TargetX = targetX,
                TargetY = targetY
            };

            var payload = NetworkMessageCodec.Serialize(message);

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
                    var message = NetworkMessageCodec.Deserialize(payload);

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

