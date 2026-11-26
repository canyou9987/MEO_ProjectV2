// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MEOItemTypes.generated.h"

//  아이템 희귀도 등급 (접사 개수 및 드랍 가중치 결정)
UENUM(BlueprintType)
enum class EMEOItemRarity : uint8
{
    Normal,         //  일반 (흰색 - 접사 0개)
    Magic,          //  마법 (파란색 - 접두 1개 + 접미 1개, 최대 2개)
    Rare,           //  희귀 (노란색 - 접두 최대  3개 + 접미 최대  3개, 최대 6개)
    Unique          //  고유 (주황색 - 고정 고유 옵션)    
};

//  아이템 접사(Affix) 분류
UENUM(BlueprintType)
enum class EMEOAffixSlot : uint8
{
    Implicit,       //  고정 속성 (아이템 베이스 자체에 붙는 옵션)
    Prefix,         //  접두사    (주로 공격력, 최대 생명력, 방어도 등 핵심 수치)
    Suffix          //  접미사    (주로 원소 저항, 공격/시전 속도, 치명타 확률 등 보조 수치)
};

//  화폐 오브(Currency Orb) 크래프팅 가능 구분
UENUM(BlueprintType)
enum class EMEOCurrencyType : uint8
{
    Transmutation,  //  진화의 오브  : 일반 -> 마법 등급 업그레이드 (접사 1~2개 부여)
    Augmentation,   //  확장의 오브  : 마법 아이템에 빈 접사 1개 추가
    Alteration,     //  변화의 오브  : 마법 아이템의 접사를 무작위 재굴림 <- POE2에는 없는 오브임,추가할지는 추후결정
    Regal,          //  제왕의 오브  : 마법 -> 희귀 등급 업그레이드 (접사 1개 추가)
    Exalted,        //  엑잘티드 오브: 희귀 아이템에 빈 접사 1개 추가 (최대 6개)
    Chaos,          //  카오스 오브  : 희귀 아이템의 접사 1개를 제거하고 새로운 접사 1개 부여 (POE2 방식)
    Vaal            //  바알 오브    : 아이템을 타락(Corrupted)시켜 추가 크래프팅을 잠그고 변이 유발
};

//  런타임 아이템에 부여된 개별 스텟 모디파이어 인스턴스
USTRUCT(BlueprintType)
struct FMEOItemModifierInstance
{
    GENERATED_BODY()

    //  스텟 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixRowName = NAME_None;

    //  스텟 슬롯타입
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EMEOAffixSlot SlotType = EMEOAffixSlot::Prefix;

    //  스텟 식별 태그 (예: "Stat.Life.Flat","Stat.FireRes.Percent")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName StatTag = NAME_None;

    //  주사위 굴림으로 결정된 실제 롤링 수치
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float RolledValue = 0.0f;

    //  접사 티어 (1티어가 최고 등급)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 Tier = 1;
};


//  데이터 테이블용 접사 풀(Pool) 정의 행 구조체
USTRUCT(BlueprintType)
struct FMEOAffixTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EMEOAffixSlot SlotType = EMEOAffixSlot::Prefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName StatTag = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 Tier = 1;

    //  해당 티어 접사가 등장하기 위한 최소 아이템 레벨(iLvl) 요구치
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    int32 RequiredItemLevel = 1;

    //  등장 가중치 (높을수록 자주 출현)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float SpawnWeight = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float MinValue = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    float MaxValue = 10.0f;
};

//  루트 테이블 개별 드랍 후보 항목
USTRUCT(BlueprintType)
struct FMEOLootTableEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    FName ItemBaseRowName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    float DropWeight = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    int32 MinStack = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    int32 MaxStack = 1;
};

//  몬스터 처치 시 드랍 연산에 전달하는 컨텍스트 파라미터
USTRUCT(BlueprintType)
struct FMEOLootGenerationContext
{
    GENERATED_BODY()

    //  몬스터 레벨 (드랍되는 아이템의 ItemLevel 결정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    int32 AreaLevel = 1;

    //  플레이어 + 맵 모디파이어의 아이템 수량 증가(IIQ) 합산 (예 0.4f = 40% 증가)
    float IncreasedItemQuantity = 0.0f;

    //  플레이어 + 맵 모디파이어의 아이템 희귀도 증가(IIR)합산 (예: 0.8f = 80% 증가)
    float IncreasedItemRarity = 0.0f;
};
