# TCP 멀티플레이어 시스템 요구사항

## Introduction

실시간 TCP 통신을 기반으로 한 멀티플레이어 캐릭터 이동 시스템입니다. 서버는 여러 클라이언트의 연결을 관리하고, 각 클라이언트는 WPF 기반의 2D 캔버스에서 캐릭터를 제어할 수 있습니다.

## Glossary

- **TCP_Server**: 클라이언트 연결을 관리하고 메시지를 중계하는 서버 시스템
- **WPF_Client**: Windows Presentation Foundation 기반의 클라이언트 애플리케이션
- **Character**: 게임 내에서 플레이어가 제어하는 캐릭터 객체
- **UserId**: 각 클라이언트에게 할당되는 16비트 부호없는 정수형 고유 식별자
- **Movement_Message**: 캐릭터 이동 정보를 담은 네트워크 메시지
- **JSON_Settings**: 클라이언트 설정을 저장하는 JSON 형식 파일

## Requirements

### Requirement 1: 서버 연결 관리

**User Story:** As a game server administrator, I want the server to manage multiple client connections simultaneously, so that multiple players can participate in the game.

#### Acceptance Criteria

1. WHEN a client connects to the server, THE TCP_Server SHALL assign a unique 16-bit unsigned integer UserId starting from 1000
2. WHEN the UserId reaches 65535, THE TCP_Server SHALL wrap around to 1000 to maintain the 16-bit range
3. WHEN a client disconnects, THE TCP_Server SHALL remove the client from active connections and log the disconnection with UserId
4. THE TCP_Server SHALL listen on a configurable port (default 8085) using the local machine's IP address
5. THE TCP_Server SHALL display the server IP address and port information in the console upon startup

### Requirement 2: 실시간 캐릭터 이동

**User Story:** As a player, I want to move my character by clicking on the game canvas, so that I can navigate in the game world.

#### Acceptance Criteria

1. WHEN a player clicks on the WPF_Client canvas, THE WPF_Client SHALL immediately start local character movement toward the target position
2. WHEN local movement starts, THE WPF_Client SHALL send a movement command to the TCP_Server with start and target coordinates
3. THE Character SHALL move at a constant speed of 100 units per second (configurable by server)
4. WHEN the TCP_Server receives a movement message, THE TCP_Server SHALL broadcast the movement information to all other connected clients
5. THE WPF_Client SHALL ignore movement messages from the server for its own character to prevent conflicts with local movement

### Requirement 3: 유저 식별 및 표시

**User Story:** As a player, I want to see unique identifiers for all players, so that I can distinguish between different players in the game.

#### Acceptance Criteria

1. WHEN a client connects, THE TCP_Server SHALL send a UserIdAssignment message containing the assigned UserId
2. THE WPF_Client SHALL display the received UserId as a text label above the player's character
3. WHEN other players move, THE WPF_Client SHALL display their UserId labels above their characters
4. THE WPF_Client SHALL use blue color for the local player character and red color for other players
5. THE WPF_Client SHALL display the current UserId in the status panel at the bottom of the window

### Requirement 4: 신규 클라이언트 동기화

**User Story:** As a new player joining the game, I want to see all existing players and their current positions, so that I have complete game state information.

#### Acceptance Criteria

1. WHEN a new client connects, THE TCP_Server SHALL send an AllUsersInfo message containing all currently connected users
2. THE AllUsersInfo message SHALL include UserId, current position, target position, speed, and movement status for each user
3. THE WPF_Client SHALL create visual representations for all existing players upon receiving AllUsersInfo
4. THE WPF_Client SHALL update its own character's speed based on the server-provided speed value
5. THE WPF_Client SHALL position existing characters at their current locations with proper movement states

### Requirement 5: 설정 저장 및 복원

**User Story:** As a player, I want my last used server connection settings to be remembered, so that I don't have to re-enter them every time I start the application.

#### Acceptance Criteria

1. WHEN the WPF_Client successfully connects to a server, THE WPF_Client SHALL save the server IP and port to a JSON file in the executable directory
2. THE JSON_Settings file SHALL be named "server_settings.json" and located in the same directory as the executable
3. WHEN the WPF_Client starts, THE WPF_Client SHALL load the saved server settings and populate the connection form fields
4. IF the settings file does not exist or cannot be read, THE WPF_Client SHALL use default values (localhost:8085)
5. THE JSON_Settings SHALL use a readable format with proper indentation for manual editing if needed

### Requirement 6: 네트워크 메시지 프로토콜

**User Story:** As a system, I want a reliable message protocol for client-server communication, so that all game state changes are properly synchronized.

#### Acceptance Criteria

1. THE system SHALL use JSON serialization for all network messages
2. THE Movement_Message SHALL include MessageType, UserId, CharacterId, start coordinates, target coordinates, current coordinates, speed, and movement status
3. THE TCP_Server SHALL include current position and speed information in movement broadcasts
4. THE system SHALL support UserIdAssignment, AllUsersInfo, and Move message types
5. THE WPF_Client SHALL handle network errors gracefully and display connection status to the user

### Requirement 7: 실시간 업데이트 시스템

**User Story:** As a player, I want smooth character movement animations, so that the game feels responsive and visually appealing.

#### Acceptance Criteria

1. THE WPF_Client SHALL update character positions at 60 FPS using a DispatcherTimer
2. THE Character movement SHALL use linear interpolation based on elapsed time and movement speed
3. WHEN a character reaches its target position, THE Character SHALL stop moving and set IsMoving to false
4. THE WPF_Client SHALL update both the character sprite and UserId label positions during movement
5. THE position display SHALL show real-time coordinates and UserId information in the status panel