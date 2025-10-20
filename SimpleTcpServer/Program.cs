using SimpleTcpServer;

Console.WriteLine("Simple TCP Server for Character Movement");
Console.WriteLine("Starting server on port 8085...");
Console.WriteLine("Press 'q' to quit");

var server = new TcpServer(8085);
var serverTask = Task.Run(() => server.StartAsync());

while (true)
{
    var key = Console.ReadKey(true);
    if (key.KeyChar == 'q' || key.KeyChar == 'Q')
    {
        break;
    }
}

server.Stop();
Console.WriteLine("Server stopped.");
