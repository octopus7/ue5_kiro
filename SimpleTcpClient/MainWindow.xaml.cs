using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace SimpleTcpClient;

public partial class MainWindow : Window
{
    private GameTcpClient _tcpClient;
    private Dictionary<string, Ellipse> _otherCharacters = new();
    private float _currentX = 100;
    private float _currentY = 100;

    public MainWindow()
    {
        InitializeComponent();
        _tcpClient = new GameTcpClient();
        _tcpClient.MessageReceived += OnMessageReceived;
        _tcpClient.StatusChanged += OnStatusChanged;
    }

    private async void ConnectButton_Click(object sender, RoutedEventArgs e)
    {
        var host = ServerTextBox.Text;
        if (!int.TryParse(PortTextBox.Text, out int port))
        {
            MessageBox.Show("Invalid port number");
            return;
        }

        var success = await _tcpClient.ConnectAsync(host, port);
        if (success)
        {
            ConnectButton.IsEnabled = false;
            DisconnectButton.IsEnabled = true;
            ServerTextBox.IsEnabled = false;
            PortTextBox.IsEnabled = false;
        }
    }

    private void DisconnectButton_Click(object sender, RoutedEventArgs e)
    {
        _tcpClient.Disconnect();
        ConnectButton.IsEnabled = true;
        DisconnectButton.IsEnabled = false;
        ServerTextBox.IsEnabled = true;
        PortTextBox.IsEnabled = true;
        
        // Clear other characters
        foreach (var character in _otherCharacters.Values)
        {
            GameCanvas.Children.Remove(character);
        }
        _otherCharacters.Clear();
    }

    private async void GameCanvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (!DisconnectButton.IsEnabled) return; // Not connected

        var position = e.GetPosition(GameCanvas);
        var targetX = (float)position.X;
        var targetY = (float)position.Y;

        await _tcpClient.SendMoveCommandAsync(_currentX, _currentY, targetX, targetY);
        
        // Move our character immediately for responsive feel
        Canvas.SetLeft(PlayerCharacter, targetX - 10);
        Canvas.SetTop(PlayerCharacter, targetY - 10);
        _currentX = targetX;
        _currentY = targetY;
        
        UpdatePositionDisplay();
    }

    private void OnMessageReceived(Message message)
    {
        Dispatcher.Invoke(() =>
        {
            if (message.Type == MessageType.Move)
            {
                // Show other player's movement
                if (!_otherCharacters.ContainsKey(message.CharacterId))
                {
                    var character = new Ellipse
                    {
                        Width = 20,
                        Height = 20,
                        Fill = Brushes.Red
                    };
                    GameCanvas.Children.Add(character);
                    _otherCharacters[message.CharacterId] = character;
                }

                var otherCharacter = _otherCharacters[message.CharacterId];
                Canvas.SetLeft(otherCharacter, message.TargetX - 10);
                Canvas.SetTop(otherCharacter, message.TargetY - 10);
            }
        });
    }

    private void OnStatusChanged(string status)
    {
        Dispatcher.Invoke(() =>
        {
            StatusTextBlock.Text = status;
        });
    }

    private void UpdatePositionDisplay()
    {
        PositionTextBlock.Text = $"Position: ({_currentX:F0}, {_currentY:F0})";
    }

    protected override void OnClosed(EventArgs e)
    {
        _tcpClient.Disconnect();
        base.OnClosed(e);
    }
}