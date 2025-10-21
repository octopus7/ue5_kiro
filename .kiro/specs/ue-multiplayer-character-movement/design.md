# Design Document

## Overview

UE 5.5 프로젝트에 기존 C# TCP 서버와 호환되는 멀티플레이어 캐릭터 이동 시스템을 구현합니다. 이 시스템은 protobuf 기반 네트워크 통신을 통해 실시간 멀티플레이어 캐릭터 이동을 지원하며, 기존 서버 인프라를 그대로 활용합니다.

## Architecture

### High-Level Architecture

```
┌─────────────────┐    TCP + Protobuf    ┌──────────────────┐
│   UE 5.5 Client │ ◄─────────────────► │ C# TCP Server    │
│                 │                      │ (SimpleTcpServer)│
│ ┌─────────────┐ │                      │                  │
│ │Network      │ │                      │ ┌──────────────┐ │
│ │Manager      │ │                      │ │Character     │ │
│ └─────────────┘ │                      │ │Manager       │ │
│ ┌─────────────┐ │                      │ └──────────────┘ │
│ │Character    │ │                      │                  │
│ │Manager      │ │                      │                  │
│ └─────────────┘ │                      │                  │
│ ┌─────────────┐ │                      │                  │
│ │Movement     │ │                      │                  │
│ │System       │ │                      │                  │
│ └─────────────┘ │                      │                  │
└─────────────────┘                      └──────────────────┘
```

### Component Architecture

```
UE Client Components:
┌─────────────────────────────────────────────────────────┐
│                    Game Instance                        │
│  ┌─────────────────────────────────────────────────────┐│
│  │              Network Manager                        ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ ││
│  │  │TCP Socket   │  │Protobuf     │  │Message      │ ││
│  │  │Handler      │  │Serializer   │  │Queue        │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────┐│
│  │              Character Manager                      ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ ││
│  │  │Local Player │  │Remote       │  │Character    │ ││
│  │  │Character    │  │Characters   │  │Factory      │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────┘

Game World:
┌─────────────────────────────────────────────────────────┐
│                    Player Pawn                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │              Movement Component                     ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ ││
│  │  │Click Input  │  │Smooth       │  │Position     │ ││
│  │  │Handler      │  │Movement     │  │Sync         │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. Network Manager (C++)

**Purpose**: TCP 서버와의 통신 및 protobuf 메시지 처리

**Key Methods**:
```cpp
class KIRO_TOPDOWN_API UNetworkManager : public UGameInstanceSubsystem
{
public:
    // Connection Management
    UFUNCTION(BlueprintCallable)
    bool ConnectToServer(const FString& ServerIP, int32 Port);
    
    UFUNCTION(BlueprintCallable)
    void DisconnectFromServer();
    
    // Message Sending
    UFUNCTION(BlueprintCallable)
    void SendMoveCommand(float StartX, float StartY, float TargetX, float TargetY);
    
    // Event Delegates
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FNetworkMessage&, Message);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChanged, bool, bIsConnected);
    
    UPROPERTY(BlueprintAssignable)
    FOnMessageReceived OnMessageReceived;
    
    UPROPERTY(BlueprintAssignable)
    FOnConnectionStatusChanged OnConnectionStatusChanged;

private:
    // TCP Socket handling
    class FSocket* TCPSocket;
    FRunnableThread* ReceiverThread;
    
    // Message processing
    void ProcessIncomingMessage(const TArray<uint8>& MessageData);
    TArray<uint8> SerializeMessage(const FNetworkMessage& Message);
    FNetworkMessage DeserializeMessage(const TArray<uint8>& Data);
};
```

### 2. Character Manager (C++)

**Purpose**: 로컬 및 원격 캐릭터 관리

**Key Methods**:
```cpp
class KIRO_TOPDOWN_API UCharacterManager : public UGameInstanceSubsystem
{
public:
    // Character Management
    UFUNCTION(BlueprintCallable)
    void SpawnLocalCharacter(uint32 UserID);
    
    UFUNCTION(BlueprintCallable)
    void SpawnRemoteCharacter(uint32 UserID, const FString& CharacterID);
    
    UFUNCTION(BlueprintCallable)
    void RemoveCharacter(uint32 UserID);
    
    // Character Access
    UFUNCTION(BlueprintCallable)
    class AMultiplayerCharacter* GetCharacterByUserID(uint32 UserID);
    
    UFUNCTION(BlueprintCallable)
    class AMultiplayerCharacter* GetLocalCharacter();

private:
    UPROPERTY()
    TMap<uint32, class AMultiplayerCharacter*> Characters;
    
    UPROPERTY()
    uint32 LocalUserID;
    
    UPROPERTY()
    TSubclassOf<class AMultiplayerCharacter> CharacterClass;
};
```

### 3. Multiplayer Character Pawn (C++)

**Purpose**: 플레이어가 제어하는 캐릭터 객체

**Key Methods**:
```cpp
class KIRO_TOPDOWN_API AMultiplayerCharacter : public APawn
{
public:
    AMultiplayerCharacter();

    // Movement
    UFUNCTION(BlueprintCallable)
    void StartMovementToLocation(const FVector& TargetLocation);
    
    UFUNCTION(BlueprintCallable)
    void UpdateRemoteMovement(const FVector& CurrentPos, const FVector& TargetPos, float Speed);
    
    // Properties
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float MovementSpeed = 100.0f;
    
    UPROPERTY(BlueprintReadOnly)
    uint32 UserID;
    
    UPROPERTY(BlueprintReadOnly)
    FString CharacterID;
    
    UPROPERTY(BlueprintReadOnly)
    bool bIsLocalPlayer;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    // Movement state
    FVector CurrentPosition;
    FVector TargetPosition;
    bool bIsMoving;
    float LastUpdateTime;
    
    // Input handling
    void OnMouseClick();
    FVector GetMouseWorldPosition();
    
    // Movement processing
    void UpdateMovement(float DeltaTime);
};
```

### 4. Network Message Structures (C++)

**Purpose**: Protobuf 메시지와 UE 구조체 간 변환

```cpp
// UE Struct matching protobuf NetworkMessage
USTRUCT(BlueprintType)
struct KIRO_TOPDOWN_API FNetworkMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 MessageType;
    
    UPROPERTY(BlueprintReadWrite)
    FString CharacterID;
    
    UPROPERTY(BlueprintReadWrite)
    uint32 UserID;
    
    UPROPERTY(BlueprintReadWrite)
    float StartX;
    
    UPROPERTY(BlueprintReadWrite)
    float StartY;
    
    UPROPERTY(BlueprintReadWrite)
    float TargetX;
    
    UPROPERTY(BlueprintReadWrite)
    float TargetY;
    
    UPROPERTY(BlueprintReadWrite)
    float CurrentX;
    
    UPROPERTY(BlueprintReadWrite)
    float CurrentY;
    
    UPROPERTY(BlueprintReadWrite)
    float Speed;
    
    UPROPERTY(BlueprintReadWrite)
    bool bIsMoving;
    
    UPROPERTY(BlueprintReadWrite)
    TArray<FUserInfo> AllUsers;
};

USTRUCT(BlueprintType)
struct KIRO_TOPDOWN_API FUserInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    uint32 UserID;
    
    UPROPERTY(BlueprintReadWrite)
    FString CharacterID;
    
    UPROPERTY(BlueprintReadWrite)
    float CurrentX;
    
    UPROPERTY(BlueprintReadWrite)
    float CurrentY;
    
    UPROPERTY(BlueprintReadWrite)
    float TargetX;
    
    UPROPERTY(BlueprintReadWrite)
    float TargetY;
    
    UPROPERTY(BlueprintReadWrite)
    float Speed;
    
    UPROPERTY(BlueprintReadWrite)
    bool bIsMoving;
};
```

## Data Models

### Character State Model

```cpp
struct FCharacterState
{
    uint32 UserID;
    FString CharacterID;
    FVector CurrentPosition;
    FVector TargetPosition;
    float MovementSpeed;
    bool bIsMoving;
    float LastUpdateTime;
    bool bIsLocalPlayer;
};
```

### Network Connection Model

```cpp
struct FNetworkConnectionState
{
    bool bIsConnected;
    FString ServerIP;
    int32 ServerPort;
    uint32 AssignedUserID;
    float LastPingTime;
    int32 ReconnectAttempts;
};
```

## Error Handling

### Network Error Handling

1. **Connection Failures**:
   - 자동 재연결 시도 (최대 5회)
   - 사용자에게 연결 상태 알림
   - 오프라인 모드로 전환 옵션

2. **Message Parsing Errors**:
   - 잘못된 protobuf 메시지 무시
   - 로그에 오류 기록
   - 연결 상태 확인

3. **Timeout Handling**:
   - 5초 이상 응답 없으면 연결 끊김으로 판단
   - 자동 재연결 시도

### Game Logic Error Handling

1. **Character Spawn Failures**:
   - 기본 위치에 캐릭터 스폰
   - 서버에 상태 동기화 요청

2. **Movement Validation**:
   - 유효하지 않은 이동 명령 필터링
   - 클라이언트 측 예측과 서버 권한 조화

## Testing Strategy

### Unit Tests

1. **Network Manager Tests**:
   - Protobuf 직렬화/역직렬화 테스트
   - TCP 연결 및 메시지 전송 테스트
   - 재연결 로직 테스트

2. **Character Manager Tests**:
   - 캐릭터 스폰/제거 테스트
   - 다중 캐릭터 관리 테스트

3. **Movement System Tests**:
   - 이동 계산 정확성 테스트
   - 보간 알고리즘 테스트

### Integration Tests

1. **Client-Server Communication**:
   - 실제 C# 서버와 연결 테스트
   - 메시지 송수신 테스트
   - 다중 클라이언트 테스트

2. **Gameplay Tests**:
   - 캐릭터 이동 시나리오 테스트
   - 네트워크 지연 상황 테스트
   - 연결 끊김/재연결 테스트

### Functional Tests

1. **Basic Functionality**:
   - 캐릭터 이동 정확성 테스트
   - 네트워크 메시지 송수신 테스트
   - 다중 플레이어 동기화 테스트

## Implementation Notes

### Protobuf Integration

- UE 5.5에서 protobuf 라이브러리 통합 필요
- Build.cs 파일에 protobuf 의존성 추가
- 플랫폼별 라이브러리 링크 설정

### TCP Socket Implementation

- UE의 FSocket 클래스 사용
- 별도 스레드에서 메시지 수신 처리
- 메인 스레드와 안전한 데이터 교환

### Coordinate System Conversion

- C# 서버: 2D 좌표계 (X, Y)
- UE 클라이언트: 3D 좌표계 (X, Y, Z)
- Z축은 0으로 고정하여 2D 평면에서 이동

### Basic Implementation Focus

- 핵심 멀티플레이어 기능 구현에 집중
- 성능 최적화는 추후 단계에서 고려
- 안정적인 기본 동작 우선