// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOCombatTypes.h"
#include "MEOResourceSubsystem.generated.h"


UCLASS()
class MEO_PROJECTV2_API UMEOResourceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

    //  보조 젬 배율 및 소모 감소율을 반영한 최종 스킬 소모량 계산
    UFUNCTION(BlueprintPure, Category = "MEO|Resource")
    float CalculateFinalResourceCost(const FMEOResourceCost& CostSpec) const;

    //  스킬 시전 가능 여부 검증 후 실제 자우너 차감 실행 (InOutCurrentResource를 직접)
    UFUNCTION(BlueprintCallable, Category = "MEO|Resource")
    bool ValidateAndConsumeCost(AActor* InstigatorActor, const FMEOResourceCost& CostSpec, UPARAM(ref) float& InOutCurrentResource);    //  UPARAM이 뭐지? 입력핀으로 작동???,그러면 원래 출력핀인건가?

    //  오라,지속 버프,소환수 활성화 시 정신력(Spirit) 점유 등록 (잔여 정신력 부족 시 false 반환)
    UFUNCTION(BlueprintCallable, Category = "MEO|Resource")
    bool ReserveSpirit(AActor* TargetActor, FName Reservationld,float SpiritAmount,float MaxSpiritPool);

    //  오라 헤제 또는 소환수 소멸 시 점유 중이던 정신력 반환
    UFUNCTION(BlueprintCallable, Category = "MEO|Resource")
    bool UnreserveSpirit(AActor* TargetActor, FName Reservationld);

    //  특정 액터가 현재 점유 중인 총 정신력 합계 조회
    UFUNCTION(BlueprintPure, Category = "MEO|Resource")
    float GetTotalReservedSpirit(AActor* TargetActor) const;

    //  최대 정신력 대비 현재 사용 가능한 잔여 정신력 조회
    UFUNCTION(BlueprintPure, Category = "MEO|Resource")
    float GetAvailableSpirit(AActor* TargetActor, float MaxSpiritPool)const;

private:
    // 액터별 활성화된 정신력 점유 목록 캐시 (WeakObjectPtr를 사용해 액터 파괴 시 대글링 포인터 방지)
    TMap<TWeakObjectPtr<AActor>, TArray<FMEOSpiritReservationEntry>> ActiveSpiritReservations;
};
