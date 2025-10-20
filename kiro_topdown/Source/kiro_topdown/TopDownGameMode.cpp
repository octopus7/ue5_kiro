#include "TopDownGameMode.h"
#include "TopDownCharacter.h"
#include "TopDownPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ATopDownGameMode::ATopDownGameMode()
{
	// 기본 폰 클래스를 TopDownCharacter로 설정
	DefaultPawnClass = ATopDownCharacter::StaticClass();
	
	// 기본 플레이어 컨트롤러를 TopDownPlayerController로 설정
	PlayerControllerClass = ATopDownPlayerController::StaticClass();
}