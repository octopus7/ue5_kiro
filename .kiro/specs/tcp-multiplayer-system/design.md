# TCP 멀티플레이어 시스템 설계

## Overview

TCP 기반의 실시간 멀티플레이어 캐릭터 이동 시스템으로, C# .NET을 사용하여 구현됩니다. 서버는 콘솔 애플리케이션으로, 클라이언트는 WPF 애플리케이션으로 개발되었습니다.

## Architecture

```
┌─────────────────┐    TCP/JSON     ┌─────────────────┐
│   WPF Client    │◄──────────────►│   TCP Server    │
│                 │                 │                 │
│ - Character UI  │                 │ - Connection    │
│ - Input Handler │                 │   Management    │
│ - Settings Mgmt │                 │ - Message       │
│                 │                 │   Broadcasting  │
└─────────────────┘                 └─────────────────┘
         │                                   │
         │                                   │
    ┌────▼────┐                         ┌────▼────┐
    │ JSON    │                         │ Character│
    │Settings │                         │ State    │
    │ File    │                         │ Manager  │
    └─────────┘                         └─────────┘
```

## Components and Interfaces

### 서버 컴포넌트

#### TcpServer 클래스
- **책임**: 클라이언트 연결 관리, 메시지 브로드캐스팅
- **주요 메서드**:
  - `StartAsync()`: 서버 시작 및 클라이언트 수락
  - `HandleClientAsync()`: 개별 클라이언트 메시지 처리
  - `BroadcastMessageAsync()`: 모든 클라이언트에게 메시지 전송
  - `SendUserIdAssignmentAsync()`: 새 클라이언트에게 UserId 할당
  - `SendAllUsersInfoAsync()`: 기존 유저 정보 전송

#### Character 클래스
- **책임**: 서버 측 캐릭터 상태 관리
- **속성**: Id, UserId, Position, TargetPosition, Speed, IsMoving
- **메서드**: `StartMovement()`, `Update()`

#### MessageProtocol 클래스
- **책임**: 네트워크 메시지 직렬화/역직렬화
- **메시지 타입**: Move, Position, UserIdAssignment, AllUsersInfo

### 클라이언트 컴포넌트

#### MainWindow 클래스
- **책임**: UI 관리, 사용자 입력 처리, 서버 통신
- **주요 메서드**:
  - `ConnectButton_Click()`: 서버 연결
  - `GameCanvas_MouseLeftButtonDown()`: 캐릭터 이동 명령
  - `OnMessageReceived()`: 서버 메시지 처리
  - `UpdateTimer_Tick()`: 60FPS 캐릭터 업데이트

#### ClientCharacter 클래스
- **책임**: 클라이언트 측 캐릭터 이동 처리
- **기능**: 상수 속도 이동, 선형 보간, 실시간 위치 업데이트

#### GameTcpClient 클래스
- **책임**: TCP 통신 관리
- **이벤트**: MessageReceived, StatusChanged
- **메서드**: `ConnectAsync()`, `SendMoveCommandAsync()`, `Disconnect()`

#### ServerSettings 클래스
- **책임**: 연결 설정 저장/로드
- **저장 위치**: 실행 파일과 같은 디렉토리의 JSON 파일

## Data Models

### Message 구조
```csharp
public class Message
{
    public MessageType Type { get; set; }
    public string CharacterId { get; set; }
    public ushort UserId { get; set; }
    public float StartX, StartY { get; set; }
    public float TargetX, TargetY { get; set; }
    public float CurrentX, CurrentY { get; set; }
    public float Speed { get; set; }
    public bool IsMoving { get; set; }
    public List<UserInfo>? AllUsers { get; set; }
}
```

### UserInfo 구조
```csharp
public class UserInfo
{
    public ushort UserId { get; set; }
    public string CharacterId { get; set; }
    public float CurrentX, CurrentY { get; set; }
    public float TargetX, TargetY { get; set; }
    public float Speed { get; set; }
    public bool IsMoving { get; set; }
}
```

### ServerSettings 구조
```csharp
public class ServerSettings
{
    public string Host { get; set; } = "localhost";
    public int Port { get; set; } = 8085;
}
```

## Error Handling

### 서버 측 오류 처리
- 클라이언트 연결 끊김: 자동 정리 및 로그 출력
- 메시지 파싱 오류: 무시하고 계속 처리
- 네트워크 오류: 콘솔에 오류 메시지 출력

### 클라이언트 측 오류 처리
- 연결 실패: 사용자에게 오류 메시지 표시
- 설정 파일 오류: 기본값으로 대체
- 메시지 수신 오류: 연결 상태 업데이트

## Testing Strategy

### 단위 테스트 대상
- Character 클래스의 이동 로직
- ClientCharacter 클래스의 보간 계산
- Message 직렬화/역직렬화
- ServerSettings JSON 저장/로드

### 통합 테스트 시나리오
- 다중 클라이언트 연결 테스트
- 동시 이동 명령 처리 테스트
- 클라이언트 연결/해제 시나리오
- 네트워크 지연 상황에서의 동기화 테스트

### 성능 테스트
- 동시 접속자 수 한계 테스트
- 메시지 처리 지연 시간 측정
- 메모리 사용량 모니터링
- 60FPS 업데이트 성능 검증

## Security Considerations

### 현재 구현의 보안 제한사항
- 인증/인가 시스템 없음
- 메시지 검증 부족
- DoS 공격 방어 없음
- 암호화되지 않은 통신

### 향후 보안 개선 방안
- 클라이언트 인증 시스템 추가
- 메시지 유효성 검사 강화
- 연결 수 제한 및 Rate Limiting
- TLS/SSL 암호화 적용

## Performance Optimization

### 현재 최적화 사항
- 60FPS 타이머를 통한 부드러운 애니메이션
- 로컬 예측을 통한 반응성 향상
- JSON 직렬화 최적화
- 비동기 네트워크 처리

### 향후 최적화 방안
- 메시지 압축
- 델타 압축을 통한 대역폭 절약
- 공간 분할을 통한 관심 영역 관리
- 클라이언트 측 예측 및 서버 조정