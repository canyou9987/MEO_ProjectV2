// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SaveGame/MEOGameInstance.h"
#include "MEO_ProjectV2/SaveGame/MEOSaveGame.h"
#include "MEO_ProjectV2/GameMode/MEOPlayerState.h"
#include "Kismet/GameplayStatics.h"

void UMEOGameInstance::Init()
{
	Super::Init();

	//	게임 부팅 시 기존 세이브 파일이 있으면 로드하여 CachedSaveData 초기화
	FMEOSaveGameData LoadedData;
	if (LoadCharacterDataFromSlot(DefaultSaveSlotName, LoadedData))
	{
		CachedSaveData = LoadedData;
	}
}

void UMEOGameInstance::Shutdown()
{
	//	게임 종료 또는 강제 이탈 작전 마지막 캐시 데이터를 디스크에 직렬화 백업
	if (UMEOSaveGame* SaveObj = Cast<UMEOSaveGame>(UGameplayStatics::CreateSaveGameObject(UMEOSaveGame::StaticClass())))
	{
		SaveObj->SaveData = CachedSaveData;
		UGameplayStatics::SaveGameToSlot(SaveObj, DefaultSaveSlotName, 0);
	}

	Super::Shutdown();
}

bool UMEOGameInstance::SaveCharacterDataToSlot(const FString& SlotName , AMEOPlayerState* SourcePlayerState)
{
	if (!IsValid(SourcePlayerState))
	{
		return false;
	}
	
	//	PlayerState(SSOT)가 보여훈 최신 성장/인벤토리/장비/패시브 데이터를 SaveGameData 구조체로 동기화
	
	CachedSaveData.SavedLevelData = SourcePlayerState->LevelData;
	CachedSaveData.SavedInventory = SourcePlayerState->MainInventoryData;
	CachedSaveData.SavedEquipment = SourcePlayerState->EquippedItemsData;
	CachedSaveData.SavedSkills = SourcePlayerState->EquippedSkillsData;
	CachedSaveData.CommonPassiveNodes = SourcePlayerState->CommonPassiveNodes;
	CachedSaveData.WeaponSet1PassiveNodes = SourcePlayerState->WeaponSet1PassiveNodes;
	CachedSaveData.WeaponSet2PassiveNodes = SourcePlayerState->WeaponSet2PassiveNodes;
	

	//	AMEOPlayerState가 구성되기 전까지는 주석 처리. -> 해제 완료
	//	플레이어가 플레이중에 데이터를 가지고 있는곳은 PlayerState구나?
	//	캐릭터가 사망하면 물리와 애니매이션 상태 정리를 위해 Destroy()를 하여 파괴하는데,
	//	그 과정에서 캐릭터가 아이템정보를 가지고 있다면 그 정보는 증발하기 때문에
	//	레벨(세션)이 끝날때까지 유지시킬려면 PlayerState에서 정보를 관리,멀티플레이 파티원 정보 공유도 하면서
	//	(단, 체력/마나/이동속도 같은 실시간 전투 연산은 캐릭터에 붙는 Layer 3 컴포넌트들이 PlayerState에서 데이터를 복사받아 수행)

	//	USaveGame 직렬화 객체 생성 및 데이터 주입
	UMEOSaveGame* SaveObj = Cast<UMEOSaveGame>(UGameplayStatics::CreateSaveGameObject(UMEOSaveGame::StaticClass()));
	if (!SaveObj)
	{
		return false;
	}

	SaveObj->SaveData = CachedSaveData;

	const FString TargetSlot = SlotName.IsEmpty() ? DefaultSaveSlotName : SlotName;

	return UGameplayStatics::SaveGameToSlot(SaveObj, TargetSlot, 0);
}

bool UMEOGameInstance::LoadCharacterDataFromSlot(const FString& SlotName, FMEOSaveGameData& OutSaveData)
{
	const FString TargetSlot = SlotName.IsEmpty() ? DefaultSaveSlotName : SlotName;

	//	해당 슬룻에 세이브 파일이 존재하는지 먼저 검증
	if (!UGameplayStatics::DoesSaveGameExist(TargetSlot, 0)) 
	{
		return false;
	}

	// 바이너리 파일 역직렬화 및 메모리 캐시 갱신
	if (UMEOSaveGame* LoadObj = Cast<UMEOSaveGame>(UGameplayStatics::LoadGameFromSlot(TargetSlot, 0)))
	{
		OutSaveData = LoadObj->SaveData;
		CachedSaveData = OutSaveData;
		return true;
	}

	return false;
}
