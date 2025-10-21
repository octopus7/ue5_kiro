using System.Buffers.Binary;
using System.IO;
using System.Net.Sockets;
using Google.Protobuf;
using SimpleTcp.Shared;

await BotClient.RunAsync(args);

internal static class BotClient
{
    private const double MinX = 0.0;
    private const double MaxX = 700.0;
    private const double MinY = 0.0;
    private const double MaxY = 400.0;
    private const int MaxMessageSize = 1_048_576;

    public static async Task RunAsync(string[] args)
    {
        if (args.Length != 3)
        {
            PrintUsage();
            return;
        }

        var serverIp = args[0];

        if (!int.TryParse(args[1], out var port) || port is < 1 or > 65535)
        {
            Console.WriteLine("포트 번호는 1~65535 범위의 정수여야 합니다.");
            return;
        }

        if (!int.TryParse(args[2], out var botCount) || botCount < 1)
        {
            Console.WriteLine("봇 수량은 1 이상의 정수여야 합니다.");
            return;
        }

        using var cts = new CancellationTokenSource();

        Console.CancelKeyPress += (sender, eventArgs) =>
        {
            // Ctrl+C 시 즉시 종료되지 않도록 취소를 억제하고 종료 토큰을 발행한다.
            eventArgs.Cancel = true;
            cts.Cancel();
            Console.WriteLine("종료 요청을 받아 정리 중입니다. 잠시만 기다려 주세요...");
        };

        Console.WriteLine($"서버: {serverIp}:{port} / 봇 수: {botCount}");
        Console.WriteLine("Ctrl+C 를 눌러 종료할 수 있습니다.");

        var tasks = Enumerable.Range(1, botCount)
            .Select(id => RunBotAsync(id, serverIp, port, cts.Token))
            .ToArray();

        try
        {
            await Task.WhenAll(tasks);
        }
        catch (OperationCanceledException)
        {
            // 취소는 정상 종료 시나리오이므로 무시한다.
        }
        catch (AggregateException ex) when (ex.InnerExceptions.All(e => e is OperationCanceledException))
        {
            // 모든 태스크가 취소로 종료된 경우도 동일하게 무시한다.
        }

        Console.WriteLine("모든 봇 동작이 종료되었습니다.");
    }

    private static async Task RunBotAsync(int botId, string serverIp, int port, CancellationToken cancellationToken)
    {
        var rng = new Random(unchecked(Environment.TickCount ^ (botId * 397)));
        var position = new Position(
            NextDouble(rng, MinX, MaxX),
            NextDouble(rng, MinY, MaxY));

        BotConnection? connection = null;

        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                if (connection is null || connection.IsClosed)
                {
                    connection?.Dispose();
                    connection = await TryConnectAsync(botId, serverIp, port, cancellationToken, quiet: connection != null);

                    if (connection is null)
                    {
                        await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken);
                        continue;
                    }
                }

                var target = new Position(
                    NextDouble(rng, MinX, MaxX),
                    NextDouble(rng, MinY, MaxY));

                var travelSeconds = NextDouble(rng, 1.5, 4.5);
                Console.WriteLine($"봇 {botId}: 이동 명령 {Format(position)} → {Format(target)} (예상 {travelSeconds:F1}초)");

                var sendSuccess = await TrySendMoveCommandAsync(connection, botId, position, target, cancellationToken);
                if (!sendSuccess)
                {
                    connection.Dispose();
                    connection = null;
                    continue;
                }

                await Task.Delay(TimeSpan.FromSeconds(travelSeconds), cancellationToken);

                position = target;
                Console.WriteLine($"봇 {botId}: 이동 완료 {Format(position)}");

                var restMilliseconds = rng.Next(1_000, 2_001);
                Console.WriteLine($"봇 {botId}: 휴식 {restMilliseconds / 1000.0:F1}초");

                await Task.Delay(restMilliseconds, cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // 외부에서 취소되면 조용히 종료한다.
        }
        finally
        {
            connection?.Dispose();
        }
    }

    private static async Task<BotConnection?> TryConnectAsync(
        int botId,
        string serverIp,
        int port,
        CancellationToken cancellationToken,
        bool quiet = false)
    {
        try
        {
            using var linkedCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            linkedCts.CancelAfter(TimeSpan.FromSeconds(5));

            var tcpClient = new TcpClient();
            await tcpClient.ConnectAsync(serverIp, port, linkedCts.Token);
            tcpClient.NoDelay = true;

            var connection = new BotConnection(tcpClient);
            connection.StartListening(botId, cancellationToken);

            if (!quiet)
            {
                Console.WriteLine($"봇 {botId}: 서버 연결 성공 ({serverIp}:{port})");
            }

            return connection;
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            if (!quiet)
            {
                Console.WriteLine($"봇 {botId}: 서버 연결 실패 - {ex.Message}");
            }

            return null;
        }
    }

    private static async Task<bool> TrySendMoveCommandAsync(
        BotConnection connection,
        int botId,
        Position start,
        Position target,
        CancellationToken cancellationToken)
    {
        try
        {
            var message = new NetworkMessage
            {
                Type = MessageType.Move,
                StartX = (float)start.X,
                StartY = (float)start.Y,
                TargetX = (float)target.X,
                TargetY = (float)target.Y
            };

            var payload = message.ToByteArray();
            var prefix = new byte[sizeof(int)];
            BinaryPrimitives.WriteInt32LittleEndian(prefix, payload.Length);

            await connection.Stream.WriteAsync(prefix.AsMemory(0, prefix.Length), cancellationToken);
            await connection.Stream.WriteAsync(payload.AsMemory(0, payload.Length), cancellationToken);

            return true;
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex) when (ex is IOException or ObjectDisposedException or SocketException)
        {
            Console.WriteLine($"봇 {botId}: 이동 명령 전송 실패 - {ex.Message}");
            return false;
        }
    }

    private static double NextDouble(Random rng, double min, double max)
        => rng.NextDouble() * (max - min) + min;

    private static string Format(Position position)
        => $"({position.X:F1}, {position.Y:F1})";

    private static void PrintUsage()
    {
        Console.WriteLine("사용법: BotConsoleClient <ip> <port> <botCount>");
        Console.WriteLine("예시:  BotConsoleClient 127.0.0.1 5000 10");
    }

    private static async Task<bool> ReadExactAsync(NetworkStream stream, byte[] buffer, int count, CancellationToken cancellationToken)
    {
        var offset = 0;
        while (offset < count)
        {
            var read = await stream.ReadAsync(buffer.AsMemory(offset, count - offset), cancellationToken);
            if (read == 0)
            {
                return false;
            }

            offset += read;
        }

        return true;
    }

    private static async Task<byte[]?> ReadMessageAsync(NetworkStream stream, CancellationToken cancellationToken)
    {
        var lengthBuffer = new byte[sizeof(int)];
        if (!await ReadExactAsync(stream, lengthBuffer, lengthBuffer.Length, cancellationToken))
        {
            return null;
        }

        var length = BinaryPrimitives.ReadInt32LittleEndian(lengthBuffer);
        if (length <= 0 || length > MaxMessageSize)
        {
            throw new InvalidDataException($"잘못된 메시지 길이: {length}");
        }

        var payload = new byte[length];
        if (!await ReadExactAsync(stream, payload, payload.Length, cancellationToken))
        {
            return null;
        }

        return payload;
    }

    private readonly record struct Position(double X, double Y);

    private sealed class BotConnection : IDisposable
    {
        private readonly TcpClient _client;
        private readonly NetworkStream _stream;
        private Task? _receiveTask;

        public BotConnection(TcpClient client)
        {
            _client = client;
            _stream = client.GetStream();
        }

        public NetworkStream Stream => _stream;
        public bool IsClosed { get; private set; }
        public ushort? AssignedUserId { get; private set; }

        public void StartListening(int botId, CancellationToken cancellationToken)
        {
            _receiveTask = Task.Run(() => ReceiveLoopAsync(botId, cancellationToken), CancellationToken.None);
        }

        private async Task ReceiveLoopAsync(int botId, CancellationToken cancellationToken)
        {
            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    var payload = await ReadMessageAsync(_stream, cancellationToken);
                    if (payload == null)
                    {
                        Console.WriteLine($"봇 {botId}: 서버 연결이 종료되었습니다.");
                        IsClosed = true;
                        break;
                    }

                    NetworkMessage? message;
                    try
                    {
                        message = NetworkMessage.Parser.ParseFrom(payload);
                    }
                    catch (InvalidProtocolBufferException ex)
                    {
                        Console.WriteLine($"봇 {botId}: 프로토콜 파싱 실패 - {ex.Message}");
                        continue;
                    }

                    switch (message.Type)
                    {
                        case MessageType.UserIdAssignment:
                            AssignedUserId = (ushort)message.UserId;
                            Console.WriteLine($"봇 {botId}: 사용자 ID {AssignedUserId} 할당됨");
                            break;
                        case MessageType.AllUsersInfo:
                            // 필요 시 참고용으로만 로그 출력
                            Console.WriteLine($"봇 {botId}: 전체 사용자 정보 수신 ({message.AllUsers.Count}명)");
                            break;
                        default:
                            Console.WriteLine($"봇 {botId}: 메시지 수신 {message.Type}");
                            break;
                    }
                }
            }
            catch (OperationCanceledException)
            {
                // 취소 시 조용히 종료
            }
            catch (IOException ex)
            {
                Console.WriteLine($"봇 {botId}: 수신 오류 - {ex.Message}");
                IsClosed = true;
            }
            catch (SocketException ex)
            {
                Console.WriteLine($"봇 {botId}: 소켓 수신 오류 - {ex.Message}");
                IsClosed = true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"봇 {botId}: 알 수 없는 수신 오류 - {ex.Message}");
                IsClosed = true;
            }
        }

        public void Dispose()
        {
            try
            {
                _stream.Dispose();
            }
            catch
            {
                // 무시
            }

            _client.Dispose();

            if (_receiveTask is { IsCompleted: false })
            {
                try
                {
                    _receiveTask.Wait(TimeSpan.FromSeconds(1));
                }
                catch
                {
                    // 무시
                }
            }
        }
    }
}
