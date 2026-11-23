// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOAilmentSubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEODiceSubsystem.h"
#include "Engine/World.h"

void UMEOAilmentSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    ProcessAilmentTicks(DeltaTime);
}

bool UMEOAilmentSubsystem::IsTickable() const
{
    //  CDO(기본 객체) 제외 및 서버 권한 환경에서만 상태이상 틱 연산 실행
    UWorld* World = GetWorld();
    return !IsTemplate() && World && World->GetNetMode() != NM_Client;

    //  2026.11.22
    //  이게 뭔말이지? IsTemplate()가 아니라면 뭐가 되는거지?
    //  월드의 NetMode가 NM_Client가 아니여야한다는데 이 NM_Client라는 상태가 뭐지?
    //  그리고 이 모든게 아니여야지 서버 권환 환경이라는 말인거야?
    //  그럼 여기서 true가 되면 이 서브시스템의 Tick을 키는곳은 어디지? 상태이상 걸린 액터가 키나?
}

void UMEOAilmentSubsystem::EvaluateAndApplyAilments(const FMEODamagePacket& DamagePakcet, const FMEOAilmentEvaluationParams& EvalParams)
{
    //  회피된 공격이거나 피격 대상이 유효하지 않으면 상태이상 판정 생략
    if (DamagePakcet.bIsEvaded || !IsValid(DamagePakcet.TargerActor))
    {
        return;
    }
    UMEODiceSubsystem* DiceSub = GetWorld()->GetSubsystem<UMEODiceSubsystem>();
    if (!DiceSub)
    {
        return;
    }

    const FMEODamagePool& Dmg = DamagePakcet.MitigatedDamage;
    const float Threshold = FMath::Max(1.0f, EvalParams.TargetAilmentThreshold);

    //  1. 화염 피해 -> 점화(Ignite) 판정 (치명타 적중 시 확정 유발,일반 타격 시 순수 확률 굴림)
    if (Dmg.Fire > 0.0f)
    {
        const float IgniteChance = DamagePakcet.bIsCriticalHit
            ? 1.0f
            : FMath::Clamp((Dmg.Fire / Threshold) + EvalParams.AilmentChanceBonus, 0.0f, 1.0f);
        if (DiceSub->RollChance(IgniteChance))
        {
            FMEOActiveAilment Ignite;
            Ignite.AilmentType = EMEOAilmentType::Ignite;
            Ignite.InstigatorActor = DamagePakcet.InstigatorActor;
            //  기본 초당 화염 실피해의 50%를 4초간 부여
            Ignite.Magnitude = (Dmg.Fire * 0.5f) * EvalParams.MagnitudeMultiplier;
            Ignite.RemainingDuration = 4.0f * EvalParams.DurationMultiplier;

            ApplyAilmentInstance(DamagePakcet.TargerActor, Ignite);
        }
    }

    //  2. 냉기 피해 -> 동결(Freeze) 판정 (피해량이 상태이상 한계치의 5% 이상일 때 축적/발동)
    if (Dmg.Cold > 0.0f)
    {
        const float ColdRatio = Dmg.Cold / Threshold;
        if (ColdRatio >= 0.05f && DiceSub->RollChance(FMath::Clamp(ColdRatio + EvalParams.AilmentChanceBonus, 0.0f, 1.0f)))
        {
            FMEOActiveAilment Freeze;
            Freeze.AilmentType = EMEOAilmentType::Freeze;
            Freeze.InstigatorActor = DamagePakcet.InstigatorActor;
            Freeze.Magnitude = 1.0f;
            //  피해 비중에 비례해 최소 0.5초 ~ 최대 3.0초 하드 CC 부여
            Freeze.RemainingDuration = FMath::Clamp(ColdRatio * 1.0f, 0.5f, 3.0f) * EvalParams.DurationMultiplier;

            ApplyAilmentInstance(DamagePakcet.TargerActor, Freeze);
        }
    }

    //  3. 번개 피해 -> 감전(Shock)판정 (받는 번개 피해 10% ~ 50% 증폭 디버프)
    if (Dmg.Lightning > 0.0f)
    {
        const float ShockChance = DamagePakcet.bIsCriticalHit
            ? 1.0f
            : FMath::Clamp((Dmg.Lightning / Threshold) + EvalParams.AilmentChanceBonus, 0.0f, 1.0f);
        if (DiceSub->RollChance(ShockChance))
        {
            FMEOActiveAilment Shock;
            Shock.AilmentType = EMEOAilmentType::Shock;
            Shock.InstigatorActor = DamagePakcet.InstigatorActor;
            const float RawShockEffect = FMath::Clamp((Dmg.Lightning / Threshold) * 0.5f, 0.10f, 0.50f);
            Shock.Magnitude = RawShockEffect * EvalParams.MagnitudeMultiplier;
            Shock.RemainingDuration = 4.0f * EvalParams.DurationMultiplier;

            ApplyAilmentInstance(DamagePakcet.TargerActor, Shock);
        }
    }

    //  카오스 + 물리 피해 -> 중독(Poison) 판정 (중독 확률 보유 시 무한 중첩 부여)
    const float PoisonBaseDmg = Dmg.Chaos + Dmg.Physical;
    if (PoisonBaseDmg > 0.0f && EvalParams.AilmentChanceBonus > 0.0f)
    {
        if (DiceSub->RollChance(EvalParams.AilmentChanceBonus))
        {
            FMEOActiveAilment Poison;
            Poison.AilmentType = EMEOAilmentType::Poison;
            Poison.InstigatorActor = DamagePakcet.InstigatorActor;
            //  기본 초당 (카오스 + 물리) 실피해의 20%를 2초간 부여
            Poison.Magnitude = (PoisonBaseDmg * 0.2f) * EvalParams.MagnitudeMultiplier;
            Poison.RemainingDuration = 2.0f * EvalParams.DurationMultiplier;

            ApplyAilmentInstance(DamagePakcet.TargerActor, Poison);
        }
    }

    //  DamagePakcet.MitigatedDamage여기에 화염,냉기,번개,카오스,물리가 다들어가 있는거구나?
    //  신기하네,Total로 합쳐서 전달될줄알았는데 FMEODamagePool이 구조체 안에 있었구나?
    //  거의 말그대로 데미지 관련된 하나의 패키지네?
}

void UMEOAilmentSubsystem::ProcessAilmentTicks(float DeltaTime)
{
    //  순회 중 파괴된 액터나 만료된 맵 엔트리를 안전하게 제거하기 위해 Iterator 사용
    for (auto It = ActiveAilmentsMap.CreateIterator(); It; ++It)
    {
        AActor* TargetActor = It.Key().Get();
        if (!IsValid(TargetActor))
        {
            It.RemoveCurrent();
            continue;
        }

        TArray<FMEOActiveAilment>& Ailments = It.Value();
        for (int32 i = Ailments.Num() - 1; i >= 0; --i)
        {
            FMEOActiveAilment& Current = Ailments[i];
            Current.RemainingDuration -= DeltaTime;
            Current.TickAccumulator += DeltaTime;

            //  DoT 계열(점화,중독,출혈)은 설정된 틱 주기(0.25초)마다 피해 정산
            if (Current.AilmentType == EMEOAilmentType::Ignite ||
                Current.AilmentType == EMEOAilmentType::Poison ||
                Current.AilmentType == EMEOAilmentType::Bleed
                )
            {
                if (Current.TickAccumulator >= DOT_TICK_INTERVAL)
                {
                    ExcuteDoTTickDamage(TargetActor, Current, DOT_TICK_INTERVAL);
                    Current.TickAccumulator -= DOT_TICK_INTERVAL;
                }
            }

            //  지속시간 만료 시 상태이상 헤제 및 이벤트 통지
            if (Current.RemainingDuration <= 0.0f)
            {
                const EMEOAilmentType ExpiredType = Current.AilmentType;
                Ailments.RemoveAtSwap(i);
                OnAilmentStateChanged.Broadcast(TargetActor, ExpiredType, false);
            }
        }

        //  대상 액터의 모든 상태이상이 만료되었다면 맵 키까지 정리하여 메모리 누수 방지
        if (Ailments.Num() == 0)
        {
            It.RemoveCurrent();
        }
    }

}

bool UMEOAilmentSubsystem::HasAilment(AActor* TargetActor, EMEOAilmentType AilmentType) const
{
    if (!IsValid(TargetActor)) return false;

    const TArray<FMEOActiveAilment>* Ailments = ActiveAilmentsMap.Find(TargetActor);
    if (!Ailments) return false;

    for (const FMEOActiveAilment& Entry : *Ailments)
    {
        if (Entry.AilmentType == AilmentType)
        {
            return true;
        }
    }

    return false;
}

float UMEOAilmentSubsystem::GetShockDamageTakenMultiplier(AActor* TargetActor) const
{
    if (!IsValid(TargetActor)) return 1.0f;

    const TArray<FMEOActiveAilment>* Ailments = ActiveAilmentsMap.Find(TargetActor);
    if (!Ailments) return 1.0f;

    for (const FMEOActiveAilment& Entry : *Ailments)
    {
        if (Entry.AilmentType == EMEOAilmentType::Shock)
        {
            return 1.0f + Entry.Magnitude;
        }
    }
    return 1.0f;

    //  데미지를 받아서 곱해서   전달하는 X
    //  데미지를 얼마나 곱할지를 전달하는 O
}

void UMEOAilmentSubsystem::ApplyAilmentInstance(AActor* TargetActor, const FMEOActiveAilment& NewAilment)
{
    TArray<FMEOActiveAilment>& Ailments = ActiveAilmentsMap.FindOrAdd(TargetActor);
    if (NewAilment.AilmentType != EMEOAilmentType::Poison)
    {
        for (FMEOActiveAilment& Existing : Ailments)
        {
            if (Existing.AilmentType == NewAilment.AilmentType)
            {
                if (NewAilment.Magnitude >= Existing.Magnitude)
                {
                    Existing = NewAilment;
                    OnAilmentStateChanged.Broadcast(TargetActor, NewAilment.AilmentType, true);
                }
                return;
            }
        }
    }

    Ailments.Add(NewAilment);
    OnAilmentStateChanged.Broadcast(TargetActor, NewAilment.AilmentType, true);

}

void UMEOAilmentSubsystem::ExcuteDoTTickDamage(AActor* TargetActor, const FMEOActiveAilment& Ailment, float TickDelta)
{
    float TickDamage = Ailment.Magnitude * TickDelta;

    //  출혈(Bleed) 상태일 때 대상이 이동 중이면 피해량 2배 증폭
    if (Ailment.AilmentType == EMEOAilmentType::Bleed && TargetActor->GetVelocity().SizeSquared2D() > 100.0f)
    {
        TickDamage *= 2.0f;
    }

    // TODO: [Layer 3 UMEOStatusComponent 구현 후 연동]
    // TargetActor의 UMEOStatusComponent를 조회하여 TickDamage만큼 생명력 직접 차감 호출
    (void)TickDamage;

    //  화염,중독, 등등이 있을텐데 이것은 UMEOStatusComponent를 구현한뒤에 할수있는 문제인가?

}
