// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOTimeSubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEOAilmentSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UMEOTimeSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //  월드 내 모든 액터의 쿨타임 잔여 시간 차감 및 만료 엔트리 정리
    for (auto It = ActiveCooldownsMap.CreateIterator(); It; ++It)
    {
        AActor* TargetActor = It.Key().Get();
        if (!IsValid(TargetActor))
        {
            It.RemoveCurrent();
            continue;
        }
        TArray<FMEOCooldownEntry>& Cooldowns = It.Value();
        for (int32 i = Cooldowns.Num() - 1; i >= 0; --i)
        {
            Cooldowns[i].RemainingTime -= DeltaTime;
            if (Cooldowns[i].RemainingTime <= 0.0f)
            {
                Cooldowns.RemoveAtSwap(i);
            }
        }
        if (Cooldowns.Num() == 0)
        {
            It.RemoveCurrent();
        }
    }
}

bool UMEOTimeSubsystem::IsTickable() const
{
    UWorld* World = GetWorld();
    return !IsTemplate() && World && World->GetNetMode() != NM_Client && ActiveCooldownsMap.Num() > 0;
}

void UMEOTimeSubsystem::StartCooldown(AActor* TargetActor, FName SkillId, float BaseDuration, float CooldownRecoveryRate)
{
    if (!IsValid(TargetActor) || SkillId.IsNone() || BaseDuration <= 0.0f)
    {
        return;
    }

    //  공식: 최종 쿨타임 = 기본 쿨타임 / 쿨타임 회복 속도 (예: 회복 속도 1.5 -> 쿨타임 33% 단축)
    const float SafeRecoveryRate = FMath::Max(0.1f, CooldownRecoveryRate);
    const float FinalDuration = BaseDuration / SafeRecoveryRate;

    TArray<FMEOCooldownEntry>& Cooldowns = ActiveCooldownsMap.FindOrAdd(TargetActor);
    for (FMEOCooldownEntry& Entry : Cooldowns)
    {
        if (Entry.SkillId == SkillId)
        {
            Entry.TotalDuration = FinalDuration;
            Entry.RemainingTime = FinalDuration;
            return;
            //  아 맨처음 스킬의 쿨타임을 적용하는 곳이구나?
            //  그래서 TotalDuration과 RemainingTime 똑같이 계산이 적용된 FinalDuration인거고?
            //  그래서 여기 함수 이름이 StartCooldown이고?
            //  그럼 스킬쿨을 감소시키는 곳이 어디지?
            //  Tick()에서 등록된 쿨타임들의 시간을 빼는 Cooldowns[i].RemainingTime -= DeltaTime에서 하는구나
        }
    }

    FMEOCooldownEntry NewEntry;
    NewEntry.SkillId = SkillId;
    NewEntry.TotalDuration = FinalDuration;
    NewEntry.RemainingTime = FinalDuration;
    Cooldowns.Add(NewEntry);

    //  아예 진짜 등록도 안한놈은 여기서 스킬ID를 등록하고 쿨타임을 넣어주는거고?
}

bool UMEOTimeSubsystem::IsOnCooldown(AActor* TargetActor, FName SkillId) const
{
    return GetRemainingCooldown(TargetActor,SkillId)> 0.0f;
}

float UMEOTimeSubsystem::GetRemainingCooldown(AActor* TargetActor, FName SkillId) const
{
    if (!IsValid(TargetActor) || SkillId.IsNone())
    {
        return 0.0f;
    }

    const TArray<FMEOCooldownEntry>* Cooldowns = ActiveCooldownsMap.Find(TargetActor);
    if (!Cooldowns)
    {
        return 0.0f;
    }

    for (const FMEOCooldownEntry& Entry : *Cooldowns)
    {
        if (Entry.SkillId == SkillId)
        {
            return FMath::Max(0.0f, Entry.RemainingTime);
        }
    }

    return 0.0f;
    
    //  이 Entry가 한국어로 뭐를 지칭하는 말일까?
    //  FMEOCooldownEntry,FMEOSpiritReservationEntry 뭔뜻이지? 한국어로? 쿨타임진입? 정신력점유 기록?
}

void UMEOTimeSubsystem::SetActorActionSpeedModifier(AActor* TargetActor, FName ModifierId, float SpeedMultiplier)
{
    if (!IsValid(TargetActor) || ModifierId.IsNone())
    {
        return;
    }

    TMap<FName, float>& Modifiers = ActionSpeedModifiersMap.FindOrAdd(TargetActor);
    Modifiers.Add(ModifierId, FMath::Max(0.0f, SpeedMultiplier));
}

void UMEOTimeSubsystem::RemoveActorActionSpeedModifier(AActor* TargetActor, FName ModifierId)
{
    if (!IsValid(TargetActor) || ModifierId.IsNone())
    {
        return;
    }

    if (TMap<FName, float>* Modifiers = ActionSpeedModifiersMap.Find(TargetActor))
    {
        Modifiers->Remove(ModifierId);
        if (Modifiers->Num() == 0)
        {
            ActionSpeedModifiersMap.Remove(TargetActor);
        }
    }
}

float UMEOTimeSubsystem::GetEffectiveActionSpeed(AActor* TargetActor) const
{
    if (!IsValid(TargetActor))
    {
        return 1.0f;
    }

    //  1.동결(Freeze) 상태이상 보유 시 다른 가속 버프와 무관하게 행동 속도 0.0f 고정
    if (const UMEOAilmentSubsystem* AilmentSub = GetWorld()->GetSubsystem<UMEOAilmentSubsystem>())
    {
        if (AilmentSub->HasAilment(TargetActor, EMEOAilmentType::Freeze))
        {
            return 0.0f;
        }
    }

    //  2.등록된 모든 행동 속도 모디파이어 곱연산 누적 (최소 10% 하한선 보장)
    const TMap<FName, float>* Modifiers = ActionSpeedModifiersMap.Find(TargetActor);
    if (!Modifiers)
    {
        return 1.0f;
    }

    float FinalSpeed = 1.0f;
    for (const TPair<FName, float>& Pair : *Modifiers)
    {
        FinalSpeed *= Pair.Value;
    }

    return FMath::Clamp(FinalSpeed, 0.1f, 3.0f);
}

void UMEOTimeSubsystem::TriggerHitStop(float Duration, float TimeDilation)
{
    UWorld* World = GetWorld();
    if (!World || Duration <= 0.0f)
    {
        return;
    }

    UGameplayStatics::SetGlobalTimeDilation(World, FMath::Clamp(TimeDilation, 0.01f, 1.0f));

    //  실제 경과 시간 기준으로 시간 배율 복구
    World->GetTimerManager().ClearTimer(HitStopTimerHandle);
    World->GetTimerManager().SetTimer(
        HitStopTimerHandle,
        [World]()
        {
            if (IsValid(World))
            {
                UGameplayStatics::SetGlobalTimeDilation(World,1.0f);
            }
        },
        Duration * TimeDilation,
        false
    );

}
