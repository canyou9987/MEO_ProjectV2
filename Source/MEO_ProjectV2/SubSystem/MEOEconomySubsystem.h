// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEO_ProjectV2/DataStruct/MEOItemTypes.h"
#include "MEOEconomySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMEOItemCrafted, const FGuid&, ItemInstanceId, EMEOCurrencyType, UsedCurrency, EMEOCraftingResult, Result);

UCLASS()
class MEO_PROJECTV2_API UMEOEconomySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    //  대상 장비 아이템에 화폐 오브 1회를 적용하여 희귀도 및 접사 변형 수행
    UFUNCTION(BlueprintCallable, Category = "MEO|Economy")
    EMEOCraftingResult ApplyCurrencyOrb(
        UPARAM(ref) FMEOInventoryItemInstance& TargetItem,
        EMEOCurrencyType CurrencyType,
        UDataTable* AffixDataTable
    );

    //  특정 아이템에 해당 화폐를 사용할 수 있는지 사전 검증 (UI 하이라이트/툴팁용)
    UFUNCTION(BlueprintPure, Category = "MEO|Economy")
    EMEOCraftingResult CanApplyCurrencyOrb(const FMEOInventoryItemInstance& TargetItem, EMEOCurrencyType CurrencyType)const;

    //  크래프팅 완료 시 사운드/VFX/UI 갱신용 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
    FOnMEOItemCrafted OnItemCrafted;

private:
    //  현재 보유한 접두사(Prefix)  /   접미사(Suffix) 개수 집계
    void CountAffixSlots(const FMEOInventoryItemInstance& Item, int32& OutPrefixCount, int32& OutSuffixCount) const;

    //  빈 슬룻(Prefix 또는 Suffix) 중 하나를 무작위로 선택해 신규 접사 1개 추가
    bool TryAddRandomAffix(FMEOInventoryItemInstance& Item, UDataTable* AffixDataTable, int32 MaxPerSlot) const;

    //  기존 Prefix/Suffix 중 무작위 1개 제거 (Implicit 고정 속성은 보호)
    bool TryRemoveRandomAffix(FMEOInventoryItemInstance& Item)const;

    //  바알 오브(Vaal Orb) 타락 변이 처리
    EMEOCraftingResult ApplyVaalCorruption(FMEOInventoryItemInstance& Item, UDataTable* AffixDataTable)const;

};

//  화폐를 적용하는 시스템,아이템에 접두 접미사를 발라서 강화하는 시스템
//  여기는 UWorldSubsystem베이스로 만드는 서브 시스템이라서 오브를 방식에 대한 함수가 적혀들어갈텐데,
//  그럼이 오브를 바르는걸 호출하는것은 주체는 누구일까?
//  1. 어떤 오브가 발라지는알아야 하므로 오브에서 호출할까?
//  ->인벤토리에서는 오브형태로 저장되는게 아니라 슬룻에 데이터 형태로 저장되서 아닐것 같다.
//  2. 인벤토리에서 우클릭으로 잡은 다음에 무기에 바르는 것이므로 인벤토리에서 하는게 맞을까?
//  ->일리가 있긴한데,그 우클릭 해서 상호작용 하는 이벤트는 인벤토리가 아니라 인벤토리 UI에서 하는게 아닌가?
//  근데 그 UI작동도 인베토리에서 하는것이므로 결국 인벤토리 컴포넌트이지 않을까?
//  3. 플레이어 캐릭터에서 작동하는걸까?플레이어 마우스를 가지고 있으니깐 호출 메세지 정도는 보낼수 있지않을까?
//  4. 결국 뭐가됬는 게임모드베이스로 와서 동작 해야하는거 아닐까?
//  흠.....

