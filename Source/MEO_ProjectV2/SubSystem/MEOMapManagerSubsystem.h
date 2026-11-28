// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOWorldTypes.h"
#include "MEOMapManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMEOZoneStateChanged, EMEOZoneState, OldState, EMEOZoneState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMEOMapProgressUpdated, const FMEOMapProgressData&, NewProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMEOPortalCountChanged, int32, RemainingPortals);

UCLASS()
class MEO_PROJECTV2_API UMEOMapManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    //  경로석(Waystone) 아이템 인스턴스를 소모하여 신규 맵 인스턴스 세션 초기화
    UFUNCTION(BlueprintCallable, Category = "MEO|Map")
    bool InitializeWaystoneSession(
        const FMEOInventoryItemInstance& WaystoneItme,
        FName MapZoneRowName,
        int32 WaystoneTier,
        int32 InitialPortals = 6
    );

    //  맵 스폰 완료 후 전체 희귀 몬스터 수 및 맵 보스 유무 등록
    UFUNCTION(BlueprintCallable, Category = "MEO|Map")
    void RegisterMapObjectives(int32 InTotalRareMonsters, bool bInHasMapBoss);

    //  희귀(Rare) 몬스터 처치 보고 -> 잔여 수 차감 및 클리어 판정
    UFUNCTION(BlueprintCallable, Category = "MEO|Map")
    void NotifyRareMonsterKilled();

    //  맵 보스 처치 보고 -> 클리어 판정
    UFUNCTION(BlueprintCallable, Category = "MEO|Map")
    void NotifyMapBossDefeated();

    //  플레이어 사망/입장 시 포탈 1회 차감 (0 도달 시 Failed 전환)
    UFUNCTION(BlueprintCallable, Category = "MEO|Map")
    bool ConsumePortal();

    //  특정 맵 모디파이어 태그의 누적 수치 조회 (예: 몬스터 치명타/원소 피해 보너스)
    UFUNCTION(BlueprintPure, Category = "MEO|Map")
    float GetAggregatedZoneModifierValue(FName Modifierld)const;

    //  현재 맵의 지역 레벨 + IIQ/IIR 보너스를 합산한 드랍 연산 컨텍스트 생성 헬퍼
    UFUNCTION(BlueprintPure, Category = "MEO|Map")
    FMEOLootGenerationContext BuildLootContextForCurrentZone(float PlayerBonusIIQ = 0.0f, float PlayerBonusIIR = 0.0f)const;

    UFUNCTION(BlueprintPure, Category = "MEO|Map")
    EMEOZoneState GetCurrentZoneState() const { return CurrentZoneState; }

    UFUNCTION(BlueprintPure, Category = "MEO|Map")
    const FMEOWaystoneMapData& GetCurrentMapData() const {return CurrentMapData;}

    UFUNCTION(BlueprintPure, Category = "MEO|Map")
    const FMEOMapProgressData& GetMapProgress() const { return MapProgress; }

    UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
    FOnMEOZoneStateChanged OnZoneStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
    FOnMEOMapProgressUpdated OnMapProgressUpdated;

    UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
    FOnMEOPortalCountChanged OnPortalCountChanged;

private:
    void SetZoneState(EMEOZoneState NewState);

    void EvaluateCompletionCondition();

    UPROPERTY(VisibleAnywhere, Category = "MEO|Map")
    EMEOZoneState CurrentZoneState = EMEOZoneState::Hub;

    UPROPERTY(VisibleAnywhere, Category = "MEO|Map")
    FMEOWaystoneMapData CurrentMapData;

    UPROPERTY(VisibleAnywhere, Category = "MEO|Map")
    FMEOMapProgressData MapProgress;
};
