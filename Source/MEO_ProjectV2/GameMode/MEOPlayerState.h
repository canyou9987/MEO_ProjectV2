// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEOPlayerState.generated.h"

//	FOnMEOLevelDataUpdated 라는 이름의 '파라미터 1개짜리 다이나믹 멀티캐스트 델리게이트 타입'을 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMEOLevelDataUpdated, const FMEOPlayerLevelData&, NewLevelData);

//	파라미터가 없는 델리게이트 타입 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMEOEquipmentUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMEOSkillsUpdated);

UCLASS()
class MEO_PROJECTV2_API AMEOPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AMEOPlayerState();
	
	//	네트워크 리플리케이션 변수 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//-------------------------------------------------------------------------
	//	[영구 데이터 보존소 (Single Source of Truth)]	
	//	캐릭터 액터 사망 및 파괴 시에도 세션 내 데이터를 보존하는 단일 원본
	//-------------------------------------------------------------------------

	//	현재 레벨, 누적 경험치 잔여 패시브 포인트
	UPROPERTY(ReplicatedUsing = OnRep_PlayerLevelData, VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	FMEOPlayerLevelData LevelData;

	//	2D 격자 인벤토리 보유 아이템 목록
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	TArray<FMEOInventoryItemInstance> MainInventoryData;

	//	부위별 장착 중인 장비 슬룻 맵 (C++ 및 블루프린트 빠른 조회용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	TMap<EMEOEquipmentSlot, FMEOInventoryItemInstance> EquippedItemsData;

	//	퀵슬룻에 장착된 액티브 스킬 및 연결된 보조 젬 구성 목록
	UPROPERTY(ReplicatedUsing = OnRep_EquippedSkillsData, VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	TArray<FMEOSocketedSkillInstance> EquippedSkillsData;

	//	무기 세트와 무관하게 항상 활성화되는 공용 패시브 노드 ID 목록
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	TArray<FName> CommonPassiveNodes;
	//	1번 무기 세트 활성화시 적동되는 패시브 노드 ID 목록 (Dual Spec)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	TArray<FName> WeaponSet1PassiveNodes;

	//	2번 무기 세트 활성화시 적동되는 패시브 노드 ID 목록 (Dual Spec)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Persistence")
	TArray<FName> WeaponSet2PassiveNodes;

	// -------------------------------------------------------------------------
	//	[데이터 동기화 및 상태 갱신 API]
	// -------------------------------------------------------------------------

	//	서버에서 장비 맵 갱신 시 복제용 배열(ReplicatedEquippedItems)과 동기화
	UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly, Category = "Persistence")
	void SetEquippedItemsFromMaps(const TMap<EMEOEquipmentSlot, FMEOInventoryItemInstance>& NewEquipmentMap);

	//	스폰 또는 리스폰된 캐릭터 액터의 Layer 3 컴포넌트로 세션 데이터 복사 및 초기화
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	void SyncDataToCharacter(APawn* TargetCharacter);

	// -------------------------------------------------------------------------
	//	[UI 바인딩용 델리게이트]
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
	FOnMEOLevelDataUpdated OnPlayerLevelDataUpdated;

	UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
	FOnMEOEquipmentUpdated OnEquipmentUpdated;

	UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
	FOnMEOSkillsUpdated OnSkillsUpdated;

protected:
	//	UE5 UHT는 TMap 직접 복제를 지원하지 않으므로 배열로 복제한 뒤 클라이언트에서 TMap으로 재구축
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItemsData)
	TArray<FMEOEquippedItemEntry> ReplicatedEquippedItems;

	UFUNCTION()
	void OnRep_PlayerLevelData();

	UFUNCTION()
	void OnRep_EquippedItemsData();

	UFUNCTION()
	void OnRep_EquippedSkillsData();
};
