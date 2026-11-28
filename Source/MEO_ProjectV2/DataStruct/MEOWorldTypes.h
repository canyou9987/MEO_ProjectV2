// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
struct FMEOWaystoneMapData
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
    int32 RemainingPortals = 6;

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

//  퀘스트 생명주기 상태
UENUM(BlueprintType)
enum class EMEOQuestState : uint8
{
    Locked,             //  선행 퀘스트 미완료로 잠금 상태
    Available,          //  수락 가능 상태
    InProgress,         //  진행 중 (목표 카운트 누적 중)
    ReadyToTurnIn,      //  모든 목표 달성 완료, NPC 보상 수령 대기
    Completed           //  보상 수령까지 최종 완료
};

//  퀘스트 목표 유형 분류
UENUM(BlueprintType)
enum class EMEOQuestObjectiveType : uint8
{
    KillTarget,         //  특정 몬스터/보스 처치 
    ReachZone,          //  특정 구역/던전 도달               (TargetId = MapZoneRowName)
    CollectItem,        //  퀘스트 아이템 획득                (TargetId = ItemBaseRowName)
    InteractNPC,        //  특정 NPC와 대화/상호작용          (TargetId = NPC 식별자)
    ClearWaystoneTier   //  특정 티어 이상의 경로석 맵 클리어 (TargetId = "Any",RequiredCount = 목표 횟수)
};

//  데이터 테이블용 개별 목표 정의 단위
USTRUCT(BlueprintType)
struct FMEOQuestObjectiveDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EMEOQuestObjectiveType ObjectiveType = EMEOQuestObjectiveType::KillTarget;

    //  목표 대상 식별 키 (몬스터 ID,구역 ID,아이템 ID 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName TargetId = NAME_None;

    //  목표 달성 요구 수치 (예: 몬스터 10마리 처치 ->10)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 RequiredCount = 1;

    //  목표 달성 최소 등급 조건 (예: ClearWaystone)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 MinimumTier = 1;
};

//  데이터 테이블(UDataTable)용 퀘스트 마스터 정의 행 구조체
USTRUCT(BlueprintType)
struct FMEOQuestTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText QuestTitle;

    //  선행 완료가 필요한 퀘스트 RowName 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FName> PrerequisiteQuestIds;

    //  퀘스트 완료를 위해 달성해야 하는 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FMEOQuestObjectiveDef> Objectives;

    //  완료 보상 경험치
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 RewardExp = 0;

    //  완료 보상 패시브 스킬 포인트 (POE2 캠패인 보스/퀘스트 영구 보상)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 RewardPassivePoints = 0;

    //  완료 보상 아이템 RowName 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FName> RewardItemRowNames;
};

//  런타임 및 세이브 직렬화용 활성 퀘스트 진행 인스턴스
USTRUCT(BlueprintType)
struct FMEOActiveQuestEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName QuestId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EMEOQuestState State = EMEOQuestState::InProgress;

    //  Objective 배열과 1:1 인덱스로 매칭되는 현재 달성 카운트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<int32> CurrentCounts;
};
