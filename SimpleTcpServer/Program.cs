using SimpleTcpServer;

Console.WriteLine("Simple TCP Server for Character Movement");
Console.WriteLine("Press 'q' to quit");

var server = new TcpServer(8080);
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
