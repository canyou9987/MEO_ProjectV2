// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/GameMode/MEOPlayerState.h"
#include "Net/UnrealNetwork.h"

AMEOPlayerState::AMEOPlayerState()
{
	//	파이 UI 및 상태 동기화 주기를 초다 10회로 설정하여 대역폭 절약
	SetNetUpdateFrequency(10.0f);	
	//	후에 보면서 조정해야 한다면 
	//	flaot UpdateFrequency = 10.0f; 로 대체
}

void AMEOPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//	세션 내 모든 클라이언트에 동기화할 영속 데이터 등록
	DOREPLIFETIME(AMEOPlayerState, LevelData);
	DOREPLIFETIME(AMEOPlayerState, MainInventoryData);
	DOREPLIFETIME(AMEOPlayerState, ReplicatedEquippedItems);
	DOREPLIFETIME(AMEOPlayerState, EquippedSkillsData);
	DOREPLIFETIME(AMEOPlayerState, CommonPassiveNodes);
	DOREPLIFETIME(AMEOPlayerState, WeaponSet1PassiveNodes);
	DOREPLIFETIME(AMEOPlayerState, WeaponSet2PassiveNodes);
}

void AMEOPlayerState::SetEquippedItemsFromMaps(const TMap<EMEOEquipmentSlot, FMEOInventoryItemInstance>& NewEquipmentMap)
{
	EquippedItemsData = NewEquipmentMap;

	//	서버 권한일 경우 클라이언트 복제용 배열(ReplicatedEquippedItems) 도 함께 갱신
	if (HasAuthority())
	{
		ReplicatedEquippedItems.Empty(NewEquipmentMap.Num());
		for(const TPair<EMEOEquipmentSlot,FMEOInventoryItemInstance>& Pair : NewEquipmentMap)
		{ 
			FMEOEquippedItemEntry Entry;
			Entry.Slot = Pair.Key;
			Entry.ItemInstance = Pair.Value;
			ReplicatedEquippedItems.Add(Entry);
		}

		//	서버 로컬 환경에서도 UI 갱신 델리게이트 즉시 브로드캐스트
		OnEquipmentUpdated.Broadcast();
	}
}

void AMEOPlayerState::SyncDataToCharacter(APawn* TargetCharacter)
{
	if (!IsValid(TargetCharacter))
	{
		return;
	}

	// TODO: [AMEOPlayerCharacter 및 Layer 3 컴포넌트 구현 후 연동]
	// 1. TargetCharacter의 UMEOInventoryComponent를 조회해 MainInventoryData 및 EquippedItemsData 주입
	// 2. TargetCharacter의 UMEOSkillComponent를 조회해 EquippedSkillsData 주입 및 보조 젬 데코레이터 빌드
	// 3. TargetCharacter의 UMEOPassiveTreeComponent를 조회해 Common/WeaponSet1/WeaponSet2 노드 주입 후 스탯 재계산

	// 즉 아이템이나 패시브트리 등등의 스텟은 AMEOPlayerState으로 올것이고
	// 데이터는 다음과 같은 경로로 주입이 될듯
	// AMEOPlayerState->TargetCharacter->UMEOInventoryComponent
	
	// 쉽게 간다면 Getter를 만들어서 
	// TargetInventoryComponent = TargetCharacter->GetMEOInventoryComponent();
	// TargetInventoryComponent->SetMainInventoryData(MainInventoryData);
	// 를 해버리면 그만이고

	// PlayerState의 역활을 제한하고 싶다면
	// TargetCharacter->SetMainInventoryData(MainInventoryData);
	// TargetCharacter 내부에서 TargetInventoryComponent->SetMainInventoryData(MainInventoryData);
	// 를 하면 그만이고.

	// 차이점은 UMEOInventoryComponent를 누구까지 포함하는지 차이 인것 같다.
	// 인터페이스를 쓴다면 그 누구도 UMEOInventoryComponent를 포함을 안시킬수도 있다.
	// (예시: IMEOInventoryInterface)

	//	확실한건 스텟연산이나,스킬데미지 계산,회피율과 방어도로 따른 데미지 계산X
	//	이것들은 연산하기위해 데이터를 가지고 있고 업데이트 시켜주는곳 O
}

void AMEOPlayerState::OnRep_PlayerLevelData()
{
	//	경험치 바, 현재 레벨, 미사용 패시브 포인트 알림 UI 갱신 트리거
	OnPlayerLevelDataUpdated.Broadcast(LevelData);
}

void AMEOPlayerState::OnRep_EquippedItemsData()
{
	//	서버로부터 복제된 배열을 클라이언트 로컬 TMap으로 재구축
	EquippedItemsData.Empty(ReplicatedEquippedItems.Num());
	for (const FMEOEquippedItemEntry& Entry : ReplicatedEquippedItems)
	{
		EquippedItemsData.Add(Entry.Slot, Entry.ItemInstance);
	}

	OnEquipmentUpdated.Broadcast();
}

void AMEOPlayerState::OnRep_EquippedSkillsData()
{
	//	하단 스킬 퀵슬롯 아이콘 및 소켓 링크 UI 갱신 트리거
	OnSkillsUpdated.Broadcast();
}
