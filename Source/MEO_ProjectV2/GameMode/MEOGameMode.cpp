// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/GameMode/MEOGameMode.h"
#include "MEO_ProjectV2/SaveGame/MEOGameInstance.h"
#include "MEO_ProjectV2/GameMode/MEOGameState.h"
#include "MEO_ProjectV2/GameMode/MEOPlayerState.h"
#include "MEO_ProjectV2/GameMode/MEOPlayerController.h"
#include "MEO_ProjectV2/GameMode/MEOCameraPawn.h"
#include "MEO_ProjectV2/GameMode/MEOHUD.h"
#include "Engine/World.h"

AMEOGameMode::AMEOGameMode()
{
	//	독립 카메라 아키텍처에 따라 DefaultPawn은 CameraPawn으로 설정
	DefaultPawnClass = AMEOCameraPawn::StaticClass();
	PlayerControllerClass = AMEOPlayerController::StaticClass();
	HUDClass = AMEOHUD::StaticClass();
	GameStateClass = AMEOGameState::StaticClass();
	PlayerStateClass = AMEOPlayerState::StaticClass();

	// 지금 C++ 클래스 들로 하드코딩이 되어 있는 상태이지만,
	// 추후 다른 클래스를 박아야하는 일이 생기거나(그럴일은 없다)
	// BP화된 클래스를 박아야한다면,소켓형태로 만들어서 BP_MEOGameMode에서 꼳는 형식으로
	// 변수화 시키는것도 좋은 방안일것 같다.
}

void AMEOGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	//	새 맵 인스턴스 시작시 포탈 잔여횟수 초기화
	RemainingPortalUses = MaxPortal;

	//	그럼 중고 맵 인스턴스는 포탈 잔여 횟수를 어떻게 체크하지?
	//	은신처 맵과 던전 맵을 왔다갔다 거릴텐데,던전 맵의 인스턴스 정보와 포탈횟수는 어따 저장해 놓는거지?
	//	던전 맵을 돌다가 잠시 은신처를 들어가도,인스턴스 정보를 최소 6~10분정도는 가지고 있어야하는데?
}

void AMEOGameMode::PostLogin(APlayerController* NewPlayer)
{
	//	Super::PostLogin 내부에서 DefaultPawnClass(AMEOCameraPawn) 스폰 및 OnPossess 실행
	Super::PostLogin(NewPlayer);

	AMEOPlayerState* MEOPS = NewPlayer ? NewPlayer->GetPlayerState<AMEOPlayerState>() : nullptr;
	UMEOGameInstance* MEOGI = GetGameInstance<UMEOGameInstance>();

	//	1.GameInstance의 저장 데이터를 로드하여 세션 영구 저장소(PlayerState SSOT)에 초기 데이터 주입
	if (MEOPS && MEOGI)
	{
		FMEOSaveGameData LoadedData;
		if (MEOGI->LoadCharacterDataFromSlot(MEOGI->DefaultSaveSlotName, LoadedData))
		{
			//	플레이어 스테이트 데이터 = 불러온 세이브 데이터
			MEOPS->LevelData = LoadedData.SavedLevelData;
			MEOPS->MainInventoryData = LoadedData.SavedInventory;
			MEOPS->SetEquippedItemsFromMaps(LoadedData.SavedEquipment);
			MEOPS->EquippedSkillsData = LoadedData.SavedSkills;
			MEOPS->CommonPassiveNodes = LoadedData.CommonPassiveNodes;
			MEOPS->WeaponSet1PassiveNodes = LoadedData.WeaponSet1PassiveNodes;
			MEOPS->WeaponSet2PassiveNodes = LoadedData.WeaponSet2PassiveNodes;
		}
	}

	//	2. 실제 전투 캐릭터 액터 스폰 및 카메라,HUD 바인딩
	RespawnPlayer(NewPlayer);
}

void AMEOGameMode::HandlePlayerDeath(AController* VictimController, AActor* Killer)
{
	AMEOPlayerController* MEOPC = Cast<AMEOPlayerController>(VictimController);
	if (!MEOPC)
	{
		return;
	}

	//	1. 독립 카메라 폰에 사망 통지 (카메라는 파괴되지 않고 사망 좌표 유지 또는 관전 전환)
	if (AMEOCameraPawn* CameraPawn = MEOPC->GetControlledCameraPawn())	//	플레이어 컨트롤러가 가진 AMEOCameraPawn를 Getter 한다.
	{
		CameraPawn->OnTargetCharacterDied();
	}

	// 2. TODO: [경험치/사망 페널티 서브시스템 연동]
	// 현재 지역 레벨(CurrentAreaLevel) 기준 경험치 차감 연산 수행
	(void)Killer;

	//	플레이어가 조종하는 캐릭터가 죽으면
	//	해야하는 일들을 정리하고 관리하는곳
	//	어찌보면 소멸자 역활을 하는건가?
	//	그럼 몬스터가 죽으면 관리는 누가해주지?안해줘도 되나?지 알아서 하나?
	
}

void AMEOGameMode::RespawnPlayer(APlayerController* TargetController)
{
	AMEOPlayerController* MEOPC = Cast<AMEOPlayerController>(TargetController);
	if (!MEOPC || !CombatCharacterClass)
	{
		return;
	}

	//	PlayerStart 지점 탐색
	AActor* StartSpot = FindPlayerStart(MEOPC);
	const FVector SpanwLoc = StartSpot ? StartSpot->GetActorLocation() : FVector::ZeroVector;
	const FRotator SpawnRot = StartSpot ? StartSpot->GetActorRotation() : FRotator::ZeroRotator;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = MEOPC;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	//	1. 독립 카메라 폰과 분리된 실제 전투 캐릭터 액터 스폰
	APawn* NewCharacter = GetWorld()->SpawnActor<APawn>(CombatCharacterClass, SpanwLoc, SpawnRot, SpawnParams);
	if (NewCharacter)
	{
		//	2. 카메라 폰의 추종 대상을 신규 캐릭터로 연결
		MEOPC->BindCameraToCharacter(NewCharacter);

		//	3. PlayerState의 영구 데이터(SSOT)를 신규 캐릭터 컴포넌트로 복원
		AMEOPlayerState* MEOPS = MEOPC->GetPlayerState<AMEOPlayerState>();
		if (MEOPS)
		{
			MEOPS->SyncDataToCharacter(NewCharacter);
		}

		//	4. 최상위 HUD 위젯의 델리게이트를 신규 캐릭터 컴포넌트에 재연결 (UI 재생성 튕김 방지)
		if (AMEOHUD* MEOHUD = MEOPC->GetMEOHUD())
		{
			MEOHUD->BindHUDToCharacter(NewCharacter, MEOPS);
			//	MEOHUD작성후 주석 해제 -> 작성완료,헤제
		}
	}
}

bool AMEOGameMode::RequestPortalUse(APlayerController* Player)
{
	//	유효하지 않은 요청이거나 잔여 포탈 횟수가 0이면 진입 차단
	if (!IsValid(Player) || RemainingPortalUses <= 0)
	{
		return false;
	}

	RemainingPortalUses--;
	return true;
}
