// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEO_ProjectV2/DataStruct/MEOItemTypes.h"
#include "MEOWorldTypes.generated.h"

//  현재 월드/경로석 인스턴스 진행 상태
UENUM(BlueprintType)
enum class EMEOZoneState : uint8
{
    Hub,                //  거점/은신처 (전투 비활성, 포탈 개방 대기)
    Inprogress,         //  경로석(Waystone) 던전 진행 중
    BossEncounter,      //  맵 보스 조우 상태
    Completed,          //  모든 핵심 목표(희귀 몬스터/보스) 처치 완료
    Failed              //  포탈 소진으로 인한 인스턴스 공략 실패
};

//  활성화된 경로석(Waystone) 인스턴스의 런타임 구성 데이터
USTRUCT(BlueprintType)
struct FEMOWaystoneMapData
{
    GENERATED_BODY()

    //  사용된 경로석 아이템의 전역 고유 식별자
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    FGuid WaystoneIstanceId;

    //  이동할 던전/필드 레벨 식별 RowName
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    FName MapZoneRowName = NAME_None;

    //  경로석 등급 (1 ~ 16 티어)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 WaystoneTier = 1;

    //  해당 인스턴스의 몬스터 및 드랍 아이템 레벨 (예: Tier 1 = 65레벨)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 AreaLevel = 65;

    //  잔여 부활 /입장 포탈 횟수 (기본 6회,0 도달 시 인스턴스 실패)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 RemainingPortal = 6;

    //  경로석에 부여된 전역 맵 모디파이어 목록 (몬스터 강화 + 보상 보너스)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    TArray<FMEOZoneModifierData> ActiveModifiers;

    //  맵 모디파이어로부터 합산된 아이템 수량 증가(IIQ) 보너스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    float BonusItemQuantity = 0.0f;

    //  맵 모디파이어로부터 합산된 아이템 희귀도 증가(IIR) 보너스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    float BonusItemRarity = 0.0f;

    //  몬스터 무리 크기(Pack SIze) 증가 배율
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    float BonusPackSize = 0.0f;
};

//  POE2 앤드게임 방식의 맵 클리어 목표(희귀 몬스터 및 보스 처치) 추적 구조체
USTRUCT(BlueprintType)
struct FMEOMapProgressData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progress")
    int32 TotalRareMonsters = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progress")
    int32 RemainingRareMonsters = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progress")
    bool bHasMapBoss = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progress")
    bool bBossDefeated = false;

    //  전체 목표 달성률 (0.0 ~ 1.0) 반환
    float GetCompletionRatio() const
    {
        const int32 TotalObjectives = TotalRareMonsters + (bHasMapBoss ? 1 : 0);
        if (TotalObjectives <= 0)
        {
            return 1.0f;
        }

        const int32 ClearedRare = FMath::Max(0, TotalRareMonsters - RemainingRareMonsters);
        const int32 ClearedBoss = (bHasMapBoss && bBossDefeated) ? 1 : 0;
        return static_cast<float>(ClearedRare + ClearedBoss) / static_cast<float>(TotalObjectives);
    }
};
