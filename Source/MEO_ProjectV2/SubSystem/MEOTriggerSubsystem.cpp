// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOTriggerSubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEODiceSubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEOTimeSubsystem.h"
#include "Engine/World.h"

void UMEOTriggerSubsystem::RegisterTriggerBinding(AActor* OwnerActor, const FMEOMetaTriggerBinding& NewBinding)
{
    if (!IsValid(OwnerActor) || NewBinding.BindingId.IsNone() || NewBinding.TriggeredSkillRowName.IsNone())
    {
        return;
    }

    TArray<FMEOMetaTriggerBinding>& Bindings = ActorTriggerBindings.FindOrAdd(OwnerActor);

    //  동일 ID가 이미 존재하면 최신 설정으로 덮어 쓰기
    for (FMEOMetaTriggerBinding& Existing : Bindings)
    {
        if (Existing.BindingId == NewBinding.BindingId)
        {
            Existing = NewBinding;
            return;
        }
    }
    Bindings.Add(NewBinding);
}

void UMEOTriggerSubsystem::UnregisterTriggerBinding(AActor* OwnerActor, FName BindingId)
{
    if (!IsValid(OwnerActor) || BindingId.IsNone())
    {
        return;
    }
    TArray<FMEOMetaTriggerBinding>* Bindings = ActorTriggerBindings.Find(OwnerActor);
    if (!Bindings)
    {
        return;
    }

    Bindings->RemoveAll([BindingId](const FMEOMetaTriggerBinding& Item)
        {
            return Item.BindingId == BindingId;
        });

    if (Bindings->Num() == 0)
    {
        ActorTriggerBindings.Remove(OwnerActor);
    }

}

void UMEOTriggerSubsystem::ClearAllBindingForActor(AActor* OwnerActor)
{
    if (IsValid(OwnerActor))
    {
        ActorTriggerBindings.Remove(OwnerActor);
    }
}

void UMEOTriggerSubsystem::NotifyCombatEvent(const FMEOTriggerEventContext& EventContext)
{
    //  1.서버 권한 검증 및 유효성 검사
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !IsValid(EventContext.SourceActor))   //  월드가 있고 싱글 모드가 아니고,EventContext를 제공하는 액터가 존재하는 액터인가?
    {
        return;
    }

    //  2.무한 재귀 루프 차단 (트리거로 발동된 스킬이 다시 트리거를 연쇄 유발하는 스택 오버플로우 방지)
    if (CurrentRecursionDepth >= MAX_TRIGGER_RECURSION_DEPTH)
    {
        return;
    }

    TArray<FMEOMetaTriggerBinding>* Bindings = ActorTriggerBindings.Find(EventContext.SourceActor);
    if (!Bindings || Bindings->Num() == 0)
    {
        return;
    }

    UMEODiceSubsystem* DiceSub = World->GetSubsystem<UMEODiceSubsystem>();
    UMEOTimeSubsystem* TimeSub = World->GetSubsystem<UMEOTimeSubsystem>();

    //  재귀 깊이 카운터 증가 (함수 종료전 반드시 감소)
    CurrentRecursionDepth++;

    for (FMEOMetaTriggerBinding& Binding : *Bindings)
    {
        if (Binding.TriggerCondition != EventContext.Condition)
        {
            continue;
        }

        //  3.내부 쿨타임(Internal Cooldown) 검증
        if (TimeSub && TimeSub->IsOnCooldown(EventContext.SourceActor, Binding.BindingId))
        {
            continue;
        }

        //  4.피해/에너지 누적 임계치 검증 (예: Cast when Damage Taken)
        if (Binding.RequiredAccumulatedValue > 0.0f) // Binding이 누적 임계치란게 존재하는가?
        {
            Binding.CurrentAccumulatedValue += EventContext.EventValue;
            if (Binding.CurrentAccumulatedValue < Binding.RequiredAccumulatedValue)  //  현재 누적된 피해가 발동 임계치보다 낮다면 통과
            {
                continue;
            }
            //  임계치 초과분 이월 후 발동 진행
            Binding.CurrentAccumulatedValue -= Binding.RequiredAccumulatedValue;
        }
        //  5.발동 확률 검증 (순수 난수 롤링)
        if (DiceSub && !DiceSub->RollChance(Binding.TriggerChance))
        {
            continue;
        }

        //  6.내부 쿨타임 등록 및 스킬 발동 이벤트 브로드캐스트
        if (TimeSub && Binding.InternalCooldown > 0.0f)
        {
            //  해당ID 스킬의 쿨타임시작 등록
            TimeSub->StartCooldown(EventContext.SourceActor, Binding.BindingId, Binding.InternalCooldown, 1.0f);
        }
        OnMetaSkillTriggered.Broadcast(EventContext.SourceActor, Binding.TriggeredSkillRowName, EventContext);
    }

    CurrentRecursionDepth--;
    //  ??? CurrentRecursionDepth가지고 뭘한게 없는데 그냥 함수안에서 감소시키는게 맞아???
    //  재귀를 막기 위해서인데,뭐 동시에 CurrentRecursionDepth가 플러스 되는 경우가 있기라도 하는건가?
    //  뭐 NotifyCombatEvent(Event)가 동시에 3번 동작할일이라고 있는건가?
    //  한번 NotifyCombatEvent(Event A)가 뛰어지고 끝나고 NotifyCombatEvent(Event A)뛰어지고 끝나고 하면 재귀가 아닌건가?
}

//  흠...근데 이거 뭐하는 클래스인거지?
//  트리거를 관리하는 서브시스템인데,에초에 무엇의 트리거인거지?
//  그리고 레지스트리 등록하면서 관리할만큼 트리거가 통제가 필요한 작업인것인가?
//  그럼 여기서 통제하는 트리거는 어떤 일을 하는거지?
