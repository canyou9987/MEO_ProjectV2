// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOResourceSubsystem.h"

float UMEOResourceSubsystem::CalculateFinalResourceCost(const FMEOResourceCost& CostSpec) const
{
    //  공식 : BaseCost * SupprotCostMultiplier * (1.0 - CostReductionPercent)
    const float ReductionFactor = FMath::Clamp(1.0f - CostSpec.CostReductionPercent, 0.0f, 1.0f);
    const float RawFinalCost = CostSpec.BaseCost * FMath::Max(0.0f, CostSpec.SupportCostMultiplier) * ReductionFactor;

    //  소수점 단위로 인한 미세 잔여량 오차 방지를 위해 반올림 처리
    return FMath::RoundToFloat(RawFinalCost);
}

//  UPARAM(ref)는 UHT(언리얼 헤더 툴) 전용 매크로이므로 .h에만 적고 .cpp 함수 정의부에서는 뺀다.
bool UMEOResourceSubsystem::ValidateAndConsumeCost(AActor* InstigatorActor, const FMEOResourceCost& CostSpec, float& InOutCurrentResource)
{
    if (!IsValid(InstigatorActor))
    {
        return false;
    }

    const float RequiredCost = CalculateFinalResourceCost(CostSpec);

    //  소모량이 0 이한인 무소모 스킬은 즉시 통과
    if (RequiredCost <= 0.0f)
    {
        return true;
    }

    //  현재 보유 자원(마나/생명력/ES)이 요구치보다 부족하다면 시전 거절
    if (InOutCurrentResource < RequiredCost)
    {
        return false;
    }

    // 자원 차감 집행
    InOutCurrentResource -= RequiredCost;

    // TODO: [Layer 3 UMEOStatusComponent 구현 후 연동]
    // InstigatorActor에서 UMEOStatusComponent를 조회하여 차감된 자원 수치 복제 및 UI 델리게이트 브로드캐스트 트리거

    //  실제로 차감해야하는 수치는 InstigatorActor의 UMEOStatusComponent에서 차감을 하고(아마 이쪽에 마나/생명력/ES가 저장되있다고 상정하고 있지만,이것들은 PlayerState에 저장된 정보가 아닌가?)
    //  (아 에초에 마나/생명력/ES를 RPC로 복제할 이유가 없나? 아니지 사용조건을 통과할수 있는지를 복제할 필요가없는거지 않을까?)
    //  차감된걸 UI에 보내줘야 하기 때문에  UI 델리게이트 브로드캐스트 트리거 가 필요.

    return true;

}

bool UMEOResourceSubsystem::ReserveSpirit(AActor* TargetActor, FName Reservationld, float SpiritAmount, float MaxSpiritPool)
{
    if (!IsValid(TargetActor) || Reservationld.IsNone() || SpiritAmount < 0.0f)
    {
        return false;
    }

    TArray<FMEOSpiritReservationEntry>& Entries = ActiveSpiritReservations.FindOrAdd(TargetActor);

    //  이미 동일한 ID로 점유 중인지 확인 (중복 점유 방지)
    for (const FMEOSpiritReservationEntry& Existing : Entries)
    {
        if (Existing.ReservationId == Reservationld)
        {
            return false;
        }
    }

    //  잔여 정신력이 요구 점유량보다 부족하다면 버프/소환수 활성화 차단
    const float AvailableSpirit = GetAvailableSpirit(TargetActor, MaxSpiritPool);
    if (AvailableSpirit < SpiritAmount)
    {
        return false;
    }

    FMEOSpiritReservationEntry NewEntry;
    NewEntry.ReservationId = Reservationld;
    NewEntry.ReservedAmount = SpiritAmount;
    Entries.Add(NewEntry);

    // TODO: [UMEOStatusComponent 연동] 잔여 Spirit 변경 델리게이트 브로드캐스트
    //  흠...MEOStatusComponent의 정의를 한번 정의하고 나갈 필요가 있겟는데?
    //  PlayerStatus와의 역활이 겹치면 안되니깐?(안겹치고 있긴하다만)

    return true;
}

bool UMEOResourceSubsystem::UnreserveSpirit(AActor* TargetActor, FName Reservationld)
{
    if (!IsValid(TargetActor) || Reservationld.IsNone())
    {
        return false;
    }

    TArray<FMEOSpiritReservationEntry>* Entries = ActiveSpiritReservations.Find(TargetActor);
    if (!Entries)
    {
        return false;
    }

    const int32 RemovedCount = Entries->RemoveAll([Reservationld](const FMEOSpiritReservationEntry& Entry)
        {
            return Entry.ReservationId == Reservationld;
        });

    //  2026.11.21
    //  음.....RemovedCount는 이거 어떻게 작동하는거지??
    //  Entry.ReservationId == Reservationld가 true 나버리면 다 지워버린다는 말인가?
    //  음.......?? RemovedCount는 왜쓰는거지? 여기 정신력 점유를 하는걸 지우는곳 아닌가?? 지운숫자를 알아서 뭐하는거지?
    //  아닌가? 지운숫자가 아닌가 이거?
    //  아?! TargetActor가 점유하고있는 걸 ActiveSpiritReservations.Find(TargetActor)로 찾아서 Entries에 넣은다음에
    //  지우려고 하는 Entry.ReservationId 이게 Reservationld가 맞으면 Entries를 지워버린다는 말인거지??
    //  그러면 지운걸 RemovedCount int32에 넣어서 0보다 크면 지운게 있다는 뜻이니깐? true가 되어버리는
    //  즉 지운게 있다! 라는 뜻인거구나?

    //  해당 액터의 점유 목록이 모두 비워졌다면 맵 앤티리도 함게 정리
    if (Entries->Num() == 0)
    {
        ActiveSpiritReservations.Remove(TargetActor);
    }

    return RemovedCount>0;
}

float UMEOResourceSubsystem::GetTotalReservedSpirit(AActor* TargetActor) const
{
    if (!IsValid(TargetActor))
    {
        return 0.0f;
    }

    const TArray<FMEOSpiritReservationEntry>* Entries = ActiveSpiritReservations.Find(TargetActor);
    if (!Entries)
    {
        return 0.0f;
    }

    float TotalReserved = 0.0f;
    for (const FMEOSpiritReservationEntry& Entry : *Entries)
    {
        TotalReserved += Entry.ReservedAmount;
    }

    return TotalReserved;
}

float UMEOResourceSubsystem::GetAvailableSpirit(AActor* TargetActor, float MaxSpiritPool) const
{
    const float Reserved = GetTotalReservedSpirit(TargetActor);
    return FMath::Max(0.0f, MaxSpiritPool - Reserved);
}
