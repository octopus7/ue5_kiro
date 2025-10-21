using System.Numerics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace SimpleTcpClient;

public partial class MainWindow : Window
{
    private GameTcpClient _tcpClient;
    private Dictionary<string, (Ellipse character, TextBlock label, ClientCharacter clientChar)> _otherCharacters = new();
    private ClientCharacter? _myCharacter;
    private ushort _myUserId = 0;
    private DispatcherTimer _updateTimer;

    public MainWindow()
    {
        InitializeComponent();
        _tcpClient = new GameTcpClient();
        _tcpClient.MessageReceived += OnMessageReceived;
        _tcpClient.StatusChanged += OnStatusChanged;
        
        // 60 FPS 업데이트 타이머
        _updateTimer = new DispatcherTimer();
        _updateTimer.Interval = TimeSpan.FromMilliseconds(16); // ~60 FPS
        _updateTimer.Tick += UpdateTimer_Tick;
        _updateTimer.Start();
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
        foreach (var (character, label, _) in _otherCharacters.Values)
        {
            GameCanvas.Children.Remove(character);
            GameCanvas.Children.Remove(label);
        }
        _otherCharacters.Clear();
        _myUserId = 0;
        _myCharacter = null;
    }

    private async void GameCanvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (!DisconnectButton.IsEnabled || _myCharacter == null) return; // Not connected

        var position = e.GetPosition(GameCanvas);
        var targetX = (float)position.X;
        var targetY = (float)position.Y;

        // 즉시 로컬에서 이동 시작 (반응성 향상)
        var startPos = _myCharacter.Position;
        var targetPos = new Vector2(targetX, targetY);
        _myCharacter.StartMovement(startPos, targetPos, _myCharacter.Speed);

        // 서버에 이동 명령 전송
        await _tcpClient.SendMoveCommandAsync(startPos.X, startPos.Y, targetX, targetY);
    }

    private void OnMessageReceived(Message message)
    {
        Dispatcher.Invoke(() =>
        {
            if (message.Type == MessageType.UserIdAssignment)
            {
                // 서버로부터 받은 UserId 설정
                _myUserId = message.UserId;
                PlayerIdLabel.Text = _myUserId.ToString();
                _myCharacter = new ClientCharacter(_myUserId, "");
                _myCharacter.Position = new Vector2(100, 100);
                _myCharacter.Speed = 100.0f; // 기본 속도 설정
                UpdatePositionDisplay();
            }
            else if (message.Type == MessageType.AllUsersInfo)
            {
                // 모든 유저 정보 처리
                if (message.AllUsers != null)
                {
                    foreach (var userInfo in message.AllUsers)
                    {
                        if (userInfo.UserId != _myUserId) // 자신이 아닌 경우만
                        {
                            CreateOrUpdateOtherCharacter(userInfo);
                        }
                        else if (_myCharacter != null)
                        {
                            // 자신의 속도 정보 업데이트
                            _myCharacter.Speed = userInfo.Speed;
                        }
                    }
                }
            }
            else if (message.Type == MessageType.Move)
            {
                // 이동 메시지 처리
                if (message.UserId == _myUserId && _myCharacter != null)
                {
                    // 자신의 이동 메시지는 무시 (이미 로컬에서 처리됨)
                    // 필요시 서버와 동기화를 위한 검증 로직을 여기에 추가할 수 있음
                }
                else
                {
                    // 다른 플레이어의 이동
                    if (_otherCharacters.ContainsKey(message.CharacterId))
                    {
                        var (_, _, clientChar) = _otherCharacters[message.CharacterId];
                        var startPos = new Vector2(message.CurrentX, message.CurrentY);
                        var targetPos = new Vector2(message.TargetX, message.TargetY);
                        clientChar.StartMovement(startPos, targetPos, message.Speed);
                    }
                    else
                    {
                        // 새로운 캐릭터 생성
                        var userInfo = new UserInfo
                        {
                            UserId = message.UserId,
                            CharacterId = message.CharacterId,
                            CurrentX = message.CurrentX,
                            CurrentY = message.CurrentY,
                            TargetX = message.TargetX,
                            TargetY = message.TargetY,
                            Speed = message.Speed,
                            IsMoving = true
                        };
                        CreateOrUpdateOtherCharacter(userInfo);
                    }
                }
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

    private void CreateOrUpdateOtherCharacter(UserInfo userInfo)
    {
        if (!_otherCharacters.ContainsKey(userInfo.CharacterId))
        {
            var character = new Ellipse
            {
                Width = 20,
                Height = 20,
                Fill = Brushes.Red
            };
            
            var label = new TextBlock
            {
                Text = userInfo.UserId.ToString(),
                Foreground = Brushes.White,
                FontSize = 10,
                FontWeight = FontWeights.Bold,
                HorizontalAlignment = HorizontalAlignment.Center,
                VerticalAlignment = VerticalAlignment.Center
            };
            
            var clientChar = new ClientCharacter(userInfo.UserId, userInfo.CharacterId);
            clientChar.Position = new Vector2(userInfo.CurrentX, userInfo.CurrentY);
            clientChar.TargetPosition = new Vector2(userInfo.TargetX, userInfo.TargetY);
            clientChar.Speed = userInfo.Speed;
            clientChar.IsMoving = userInfo.IsMoving;
            
            GameCanvas.Children.Add(character);
            GameCanvas.Children.Add(label);
            _otherCharacters[userInfo.CharacterId] = (character, label, clientChar);
        }
        else
        {
            var (_, _, clientChar) = _otherCharacters[userInfo.CharacterId];
            if (userInfo.IsMoving)
            {
                var startPos = new Vector2(userInfo.CurrentX, userInfo.CurrentY);
                var targetPos = new Vector2(userInfo.TargetX, userInfo.TargetY);
                clientChar.StartMovement(startPos, targetPos, userInfo.Speed);
            }
        }
    }

    private void UpdateTimer_Tick(object? sender, EventArgs e)
    {
        // 자신의 캐릭터 업데이트
        if (_myCharacter != null)
        {
            _myCharacter.Update();
            Canvas.SetLeft(PlayerCharacter, _myCharacter.Position.X - 10);
            Canvas.SetTop(PlayerCharacter, _myCharacter.Position.Y - 10);
            Canvas.SetLeft(PlayerIdLabel, _myCharacter.Position.X - 10);
            Canvas.SetTop(PlayerIdLabel, _myCharacter.Position.Y - 25);
            UpdatePositionDisplay();
        }

        // 다른 캐릭터들 업데이트
        foreach (var (character, label, clientChar) in _otherCharacters.Values)
        {
            clientChar.Update();
            Canvas.SetLeft(character, clientChar.Position.X - 10);
            Canvas.SetTop(character, clientChar.Position.Y - 10);
            Canvas.SetLeft(label, clientChar.Position.X - 10);
            Canvas.SetTop(label, clientChar.Position.Y - 25);
        }
    }

    private void UpdatePositionDisplay()
    {
        if (_myCharacter != null)
        {
            var userIdText = _myUserId > 0 ? $" | My ID: {_myUserId}" : "";
            PositionTextBlock.Text = $"Position: ({_myCharacter.Position.X:F0}, {_myCharacter.Position.Y:F0}){userIdText}";
        }
    }

    protected override void OnClosed(EventArgs e)
    {
        _updateTimer?.Stop();
        _tcpClient.Disconnect();
        base.OnClosed(e);
    }
}