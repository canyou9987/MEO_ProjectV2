// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOCombatTypes.h"
#include "MEOTimeSubsystem.generated.h"


UCLASS()
class MEO_PROJECTV2_API UMEOTimeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
    // -------------------------------------------------------------------------
    //  [UTickableWorldSubsystem 필수 인터페이스 구현]
    // -------------------------------------------------------------------------
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMEOTimeSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override;

    // -------------------------------------------------------------------------
    //  [스킬 및 내부 쿨타임 제어 API]
    // -------------------------------------------------------------------------

    //  특정 액터의 스킬/트리거 쿨타임 시작 (쿨타임 회복 속도 배율 반영)
    UFUNCTION(BlueprintCallable, Category = "MEO|Time")
    void StartCooldown(AActor* TargetActor, FName SkillId, float BaseDuration, float CooldownRecoveryRate = 1.0f);

    //  해당 스킬이 현재 쿨타임 중인지 확인
    UFUNCTION(BlueprintPure, Category = "MEO|Time")
    bool IsOnCooldown(AActor* TargetActor, FName SkillId) const;

    //  남은 쿨타임(초) 조회 (쿨타임 아니면 0.0f 반환)
    UFUNCTION(BlueprintPure, Category = "MEO|Time")
    float GetRemainingCooldown(AActor* TargetActor, FName SkillId) const;

    // -------------------------------------------------------------------------
    //  [스킬 및 내부 쿨타임 제어 API]
    // -------------------------------------------------------------------------

    //  액터의 행동 속도 배율 등록 (예: 냉각 = 0.7f, 가속 오라 = 1.25f)
    UFUNCTION(BlueprintCallable, Category = "MEO|Time")
    void SetActorActionSpeedModifier(AActor* TargetActor, FName ModifierId, float SpeedMultiplier);

    //  행동 속도 배율 헤제
    UFUNCTION(BlueprintCallable, Category = "MEO|Time")
    void RemoveActorActionSpeedModifier(AActor* TargetActor, FName ModifierId);

    //  최종 행동 속도 배율 산출 (동결 상태이상 보유 시 무조건 0.0f 반환)
    UFUNCTION(BlueprintPure, Category = "MEO|Time")
    float GetEffectiveActionSpeed(AActor* TargetActor) const;

    //  강한 치명타/처치 시 타격감을 위한 단기 월드 시간 지연(Hit-Stop)트리거 <-(이거 멀티겜일때 해도 되는거 맞아? 시간 지연을??)
    UFUNCTION(BlueprintCallable, Category = "MEO|Time")
    void TriggerHitStop(float Duration = 0.04f, float TimeDilation = 0.05f);

private:
    //  액터별 활성 쿨타임 목록
    TMap<TWeakObjectPtr<AActor>, TArray<FMEOCooldownEntry>> ActiveCooldownsMap;

    //  액터별 행동 속도 모디파이어 맵 (Modifierld->SpeedMultiplier)
    TMap<TWeakObjectPtr<AActor>, TMap<FName, float>>ActionSpeedModifiersMap;

    //  히트스톱 복구용 타이머 핸들
    FTimerHandle HitStopTimerHandle;
};

//  POE2에는 공격 속도,이동 속도,스킬 속도,시전 속도,쿨타임 속도 등등으로 나뉘어져 있지않나?
//  근데 행동속도의 정의는 무엇이지? 대분류인건가?
//  행동속도가 50%퍼 (1.5f) 늘어나면 공속,이속,스킬속도,시전속도 전체가 50%늘어나는건가?
//  아닌가? 행동속도 = 스킬 쿨타임 속도 라고 보는게 맞는건가?
//  행동 속도 증가 =쿨타임 속도 증가 = (1.0f *  1.5f) ==   1.5f ==  50% 증가된 속도로 쿨타임 시간이 감소 로 받아들여야 하는건가?
//  행동 속도 감소 =쿨타임 속도 감소 = (1.0f * -1.5f) ==  -1.5f == -50% 감소된 속도로 쿨타임 시간이 증가 로 받아들여야 하는건가?
//  그러면 이해가 될지도? 왜 쿨타임 속도라고 적지않고 행동속도 증가라고 증가인거야?
//  진짜로 행동속도가 공격 속도,이동 속도,스킬 속도,시전 속도,쿨타임 속도 등등을 다포함하고 있는건가?
