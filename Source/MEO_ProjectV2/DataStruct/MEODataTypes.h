// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MEO_ProjectV2/DataStruct/MEOItemTypes.h"
#include "MEODataTypes.generated.h"

//	UI 토글 가능
//	단축키 입력으로 열고 닫을 수 있는 모델(Model) UI 창에 대한 열거형
//	MEOPlayerController와 MEOHUD 간 순환 참조 방지를 위해 공용 해더에서 정의
UENUM(BlueprintType)
enum class EMEOUIWindowType : uint8
{
	Inventory,		//	인벤토리 및 장비창																			(단축키 : I )
	PassiveTree,	//	패시브 스킬 트리																			(단축키 : P )
	SkillMenu,		//	스킬 젬 및 보조 젬 소켓창																	(단축키 : G )
	CharacterStats, //	캐릭터 상세 스텟 창																			(단축키 : T )
	WorldMap		//	월드맵 및 인스턴스 정보																		(단축키 : M  ) 
	//	상세정보 를 출력하는 텍스트 (창 X) (인스턴스 정보,체력,보호막,경험치 등 숫자표기 안되있는것들 숫자료 표기)  (단축키 : Alt )
	//	인스턴스 맵(월드 맵 X )을 반투명하게 화면 중앙에 뛰우고 인스턴스 정보 오른쪽 상단 미니맵 옆에 표기			(단축키 : Tab ) 
	//	기타 UI창은 추후....
};

//	캐릭터 장비 착용 부위 식별자 (Dual Spec 1/2 번 무기세트 포함)
UENUM(BlueprintType)
enum class EMEOEquipmentSlot : uint8
{
	None,
	MainHand1,	//	주무기 세트1
	OffHand1,	//	보조무기/방패 세트1
	MainHand2,	//	주무기 세트2 (Weapon Swap 대상)
	OffHand2,	//	보조무기/방패 세트2
	Helmet,		//	핼맷
	BodyArmor,	//	갑옷
	Gloves,		//	장갑
	Boots,		//	신발
	Amulet,		//	목걸이
	RingLeft,	//	왼쪽 반지
	RingRight,	//	오른쪽 반지
	Belt,		//	허리띠

	HealthFlask, // 생명력 플라스크
	ManaFlask,   // 마나 플라스크
	Charm1,      // 호신부 슬롯 1
	Charm2,      // 호신부 슬롯 2
	Charm3       // 호신부 슬롯 3

	// 양손무기 일겨우 MainHand1랑 OffHand1 둘다 쓴다고 표기해야하나?
	// MainHand1과 OffHand1 양쪽에 똑같은 FMEOInventoryItemInstance를 넣어두면
	// 장비 해제(Unequip)나 세이브/로드 시 아이템이 2개로 복사될 가능성이 있으므로
	// 양손무기 데이터는 오직 MainHand에 저장하고 
	// MainHand1에 장착된 아이템의 ItemBaseRow가 TwoHanded 태그를 가지고 있으면 OffHand1 장착을 차단(또는 자동 해제)하도록 처리예정
};

//	플레이어 레벨, 경험치 및 투자 가능한 패시브 포인트 데이터
USTRUCT(BlueprintType)
struct FMEOPlayerLevelData
{
	GENERATED_BODY()

	//	현재 캐릭터 레벨 (1~100)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 Level = 1;

	//	현재 레벨에서 누적된 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int64 CurrentExp = 0;

	//	공용 패시브 트리에 투자할 수 있는 미사용 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 UnspentPassivePoints = 0;

	//	무기 세트(Dual Spec) 전용 분기 노드에 배분 가능한 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 WeaponSetPassivePoints = 0;
};

//	런타임에 생성되어 인벤토리,장비창,바닥에 존재하는 개별 아이템 인스턴스
USTRUCT(BlueprintType)
struct FMEOInventoryItemInstance
{
	GENERATED_BODY()
	
	//	서버 권한으로 발급되는 전역 고유 식별자 (복사 버그 방지 및 트랜직션 추적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGuid InstanceId;

	//	마스터 아이템 데이터 테이블(FItemBaseRow)의 행(Row) 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemBaseRowName = NAME_None;

	//	아이템 레벨 (부여될 수 있는 접두/접미사 티어 상한선 결정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 ItemLevel = 1;

	//	화폐 오브(Currency) 등 중첩 가능 아이템 현재 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 StackCount = 1;

	// 2D 격자 인벤토리 내 좌상단 기준 좌표 ((-1,-1이면 미배치 또는 장착 상태)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FIntPoint GridPosition = FIntPoint(-1, -1);

	// 바알 오브(Vaal Orb) 등에 의한 타락 여부 (true 시 추가 크래프팅 잠금)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsCorrupted = false;

    //  현재 아이템 희귀도 등급 (일반 / 마법 / 희귀 / 고유)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EMEOItemRarity Rarity = EMEOItemRarity::Normal;

    //  현재 아이템에 부여된 접사(Prefix / Suffix / Implicit) 인스턴스 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TArray<FMEOItemModifierInstance> Modifiers;

	// 유효한 아이템 인스턴스인지 검증하는 헬퍼
	bool IsValid() const { return InstanceId.IsValid() && !ItemBaseRowName.IsNone(); }
};

//	네트워크 복제용 장비 슬룻 매핑 엔트리
//	UE5 리플리케이션은 TMap 직접 복제를 지원하지 않으므로 배열로 복제한 뒤 클라이언트에서 TMap으로 재구축 예정
USTRUCT(BlueprintType)
struct FMEOEquippedItemEntry
{
	GENERATED_BODY()

	//	장비 종류 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EMEOEquipmentSlot Slot = EMEOEquipmentSlot::None;

	//	아이템 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FMEOInventoryItemInstance ItemInstance;

};

//	퀵슬룻에 장착된 액티브 스킬 젬 및 연결(Link)된 보조젬 구성 데이터
USTRUCT(BlueprintType)
struct FMEOSocketedSkillInstance
{
	GENERATED_BODY()

	//	스킬 퀵슬룻 인덱스 (0 ~ 8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 SkillSlotIndex = -1;

	//	액티브 스킬 데이터 테이블의 행 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FName ActiveSkillRowName = NAME_None;

	//	스킬 젬 레벨 (기본 데미지 계수 및 마나 소모량 결정 , 스텟 요구치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 SkillLevel = 1;

	//	해당 액티브 스킬에 직렬 연결된 보조젬 (Support Gem) 목록 (뮤테이터 체인 구성용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TArray<FName> LinkedSupportGemRowNames;

};

//	던전/필드 인스턴스에 부여되는 전역 맵 모디파이어(Affix) 데이터
USTRUCT(BlueprintType)
struct FMEOZoneModifierData
{
	GENERATED_BODY()

	//	맵 모디파이어 식별 태그/ID (예: 몬스터 치명타 확률 증가, 플레이어 원소 저항 감소 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FName ModifierId = NAME_None;

	//	모디파이어 적용 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	float Magnitude = 0.0f;

};

//	USaveGame 바이너리 직렬화 및 세션 간 영속 저장을 위한 통합 세이브 구조체
USTRUCT(BlueprintType)
struct FMEOSaveGameData
{
	GENERATED_BODY()
	
	//	캐릭터 식별 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	FString CharacterName = TEXT("WaystoneExile");

	//	레벨, 경험치 잔여 패시브 포인트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	FMEOPlayerLevelData SavedLevelData;

	//	2D 격자 인벤토리 보유 아이템 전체 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FMEOInventoryItemInstance> SavedInventory;

	//	부위별 장착 중인 장비 슬롯맵
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	TMap<EMEOEquipmentSlot, FMEOInventoryItemInstance> SavedEquipment;

	//	소켓에 장착된 액티브/보조 스킬 구성 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FMEOSocketedSkillInstance> SavedSkills;

	//	무기 세트와 무관하게 항상 활성화되는 공용 패시브 노드 ID 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FName> CommonPassiveNodes;

	//	1번 무기 세트 활성화 시 적용되는 전용 패시브 노드 ID 목록 (Dual Spec)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FName> WeaponSet1PassiveNodes;

	//	2번 무기 세트 활성화 시 적용되는 전용 패시브 노드 ID 목록 (Dual Spec)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FName> WeaponSet2PassiveNodes;

};
