// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOItemTypes.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEODropSubsystem.generated.h"


UCLASS()
class MEO_PROJECTV2_API UMEODropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    //  몬스터 처치 시 루트 테이블과 IIQ/IIR 스텟을 반영하여 드랍 아이템 목록 생성
    UFUNCTION(BlueprintCallable, Category = "MEO|Loot")
    TArray<FMEOInventoryItemInstance> GenrateLootDrops(
        const TArray<FMEOLootTableEntry>& LootTable,
        const FMEOLootGenerationContext& Context,
        int32 BaseDropsRolls = 1
    );

    //  희귀도(IIR 보정 반영) 무작위 추첨
    UFUNCTION(BlueprintCallable, Category = "MEO|Loot")
    EMEOItemRarity RollItemRarity(float IncreadItmeRarity) const;

    //  접사 데이터 테이블에서 아이템 레벨(iLvl)과 슬룻 조건에 맞는 유효 접사 1개 롤링
    UFUNCTION(BlueprintCallable, Category = "MEO|Loot")
    bool RollRandomAffixFromTable(
        UDataTable* AffixDataTable,
        int32 ItemLevel,
        EMEOAffixSlot TargetSlot,
        const TArray<FName> ExcludedStatTags,
        FMEOItemModifierInstance& OutAffix
    )const;

};
