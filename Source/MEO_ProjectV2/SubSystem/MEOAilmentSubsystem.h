// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOCombatTypes.h"
#include "MEOAilmentSubsystem.generated.h"

//  상태이상 부여 및 만료시 UI,이팩트 시스템에서 구독할 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMEOAilmentChanged, AActor*, TargetActor, EMEOAilmentType, AilmentType, bool, bApplied);

UCLASS()
class MEO_PROJECTV2_API UMEOAilmentSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
    // -------------------------------------------------------------------------
    //  [UTickableWorldSubsystem 필수 인터페이스 구현]
    // -------------------------------------------------------------------------
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMEOAilmentSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override;

    // -------------------------------------------------------------------------
    //  [상태이상 판정 및 틱 제어 API]
    // -------------------------------------------------------------------------

    //  데미지 파이프라인 결과 패킷(5대 속성 실피해)을 분석하여 상태이상 유발 검증 및 부여
    UFUNCTION(BlueprintCallable, Category = "MEO|Ailment")
    void EvaluateAndApplyAilments(const FMEODamagePacket& DamagePakcet, const FMEOAilmentEvaluationParams& EvalParams);

    //  월드 내 모든 액터의 활성 상태이상 지속시간 차감 및 DoT 틱 정산
    UFUNCTION(BlueprintCallable, Category = "MEO|Ailment")
    void ProcessAilmentTicks(float DeltaTime);

    //  특정 액터에게 지정된 상태이상이 현재 활성화되어 있는지 조회
    UFUNCTION(BlueprintPure, Category = "MEO|Ailment")
    bool HasAilment(AActor* TargetActor, EMEOAilmentType AilmentType) const;

    //  감전(Shock) 등으로 인한 대상의 받는 피해 증폭 배율 조회 (미적용 시 1.0f 반한)
    UFUNCTION(BlueprintPure, Category = "MEO|Ailment")
    float GetShockDamageTakenMultiplier(AActor* TargetActor) const;

    //  상태이상 상태 변화 알림 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
    FOnMEOAilmentChanged OnAilmentStateChanged;

private:
    //  개별 상태이상 인스턴스 등록 (중독은 무한 중첨,비 중첩 상태이상은 최고 위력 우선 덮어쓰기)
    void ApplyAilmentInstance(AActor* TargetActor, const FMEOActiveAilment& NewAilment);

    //  DoT(점화/중독/출혈) 틱 발생시 실제 피해 전달 처리
    void ExcuteDoTTickDamage(AActor* TargetActor, const FMEOActiveAilment& Ailment, float TickDelta);

    //  액터별 활성 상태이상 목록 캐시 (액터 파괴 시 댕글링 방지를 위해 WeakObjectPtr 사용)
    TMap<TWeakObjectPtr<AActor>, TArray<FMEOActiveAilment>> ActiveAilmentsMap;

    //  DoT 데미지 정산 주기 (0.25초 = 초당 4회 정산으로 네트워크 및 UI 갱신 부하 최적화)
    const float DOT_TICK_INTERVAL = 0.25;

};
