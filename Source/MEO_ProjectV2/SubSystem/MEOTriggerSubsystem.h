// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOCombatTypes.h"
#include "MEOTriggerSubsystem.generated.h"

//  트리거 조건 충족 시 Layer 3 스킬 컴포넌트가 실제 스킬을 시전하도록 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMEOMetaSkillTriggered, AActor*, CasterActor, FName, SkillRowName, const FMEOTriggerEventContext&, EventContext);

UCLASS()
class MEO_PROJECTV2_API UMEOTriggerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    //  액터에 메타 젬 트리거 바인딩 등록 (장비/스킬 세팅시 호출)
    UFUNCTION(BlueprintCallable, Category = "MEO|Trigger")
    void RegisterTriggerBinding(AActor* OwnerActor, const FMEOMetaTriggerBinding& NewBinding);

    //  특정 트리거 바인딩 해제
    UFUNCTION(BlueprintCallable, Category = "MEO|Trigger")
    void UnregisterTriggerBinding(AActor* OwnerActor, FName BindingId);

    //  액터의 모든 트리거 바인딩 초기화 (무기 스왑 또는 리스폰 시 호출)
    UFUNCTION(BlueprintCallable, Category = "MEO|Trigger")
    void ClearAllBindingForActor(AActor* OwnerActor);

    //  전투 시스템(데미지/상태이상/사망 처리부)에서 이벤트 발생 시 호출하는 중앙 통지 함수
    UFUNCTION(BlueprintCallable, Category = "MEO|Trigger")
    void NotifyCombatEvent(const FMEOTriggerEventContext& EventContext);

    //  트리거된 스킬 실행 요청 브로드캐스트 (UMEOSkillComponent 구독)
    UPROPERTY(BlueprintAssignable, Category = "MEO|Events")
    FOnMEOMetaSkillTriggered OnMetaSkillTriggered;

private:
    //  액터별 등록된 메타 젬 바인딩 목록 (액터 파괴 시 댕글링 방지를 위해 WeakObjectPtr 사용)
    TMap<TWeakObjectPtr<AActor>, TArray<FMEOMetaTriggerBinding>>ActorTriggerBindings;

    //  무한 연쇄 발동(트리거 A->스킬 B ->트리거 A ) 방지를 위한 현재 재귀 호출 깊이
    int32 CurrentRecursionDepth = 0;

    //  단일 이벤트 체인 내 최대 연쇄 발동 허용 깊이
    const int32 MAX_TRIGGER_RECURSION_DEPTH = 2;
};
