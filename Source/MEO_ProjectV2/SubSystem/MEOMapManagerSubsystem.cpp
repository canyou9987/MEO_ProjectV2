// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOMapManagerSubsystem.h"
#include "Engine/World.h"

bool UMEOMapManagerSubsystem::InitializeWaystoneSession(const FMEOInventoryItemInstance& WaystoneItme, FName MapZoneRowName, int32 WaystoneTier, int32 InitialPortals)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !WaystoneItme.IsValid())
    {
        return false;
    }

    CurrentMapData = FMEOWaystoneMapData();
    CurrentMapData.WaystoneIstanceId = WaystoneItme.InstanceId;
    CurrentMapData.MapZoneRowName = MapZoneRowName;
    CurrentMapData.WaystoneTier = FMath::Clamp(WaystoneTier,1, 16);
    //  티어 1 = 65레벨 부터 티어당 1 레벨씩 상승 (티어 16 = 80레벨) <- 근데 만약 1레벨 시작으로 만든다?? 그러면 (65 ~ 80) -> (1 ~ 15 또는 30 )이 될수도 있다.(아니면 1 ~ 80을하고 1티어당 레벨 구간을 설정해도 되고.....)
    //  (개발 스코프의 문제)
    CurrentMapData.AreaLevel = 64 + CurrentMapData.WaystoneTier;
    CurrentMapData.RemainingPortals = FMath::Max(1, InitialPortals);

    //  경로석에 부여된 접사(Modifiers)를 맵 전역 모디파이어로 변환하고 보상 보너스(IIQ/IIR/PackSize) 집계
    for (const FMEOItemModifierInstance& Mod : WaystoneItme.Modifiers)
    {
        FMEOZoneModifierData ZoneMod;
        ZoneMod.ModifierId = Mod.StatTag;
        ZoneMod.Magnitude = Mod.RolledValue;
        CurrentMapData.ActiveModifiers.Add(ZoneMod);

        if (Mod.StatTag == FName("Map.IIQ"))
        {
            CurrentMapData.BonusItemQuantity += Mod.RolledValue;
        }
        else if (Mod.StatTag == FName("Map.IIR"))
        {
            CurrentMapData.BonusItemRarity += Mod.RolledValue;
        }
        else if (Mod.StatTag == FName("Map.PackSize"))
        {
            CurrentMapData.BonusPackSize += Mod.RolledValue;
        }
    }

    MapProgress = FMEOMapProgressData();    //  이게 무슨말이지? 그냥 초기화 빈데이터로 초기화 한다는말인가?
    SetZoneState(EMEOZoneState::Inprogress);
    OnPortalCountChanged.Broadcast(CurrentMapData.RemainingPortals);

    return true;
}

void UMEOMapManagerSubsystem::RegisterMapObjectives(int32 InTotalRareMonsters, bool bInHasMapBoss)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    MapProgress.TotalRareMonsters = FMath::Max(0, InTotalRareMonsters);
    MapProgress.RemainingRareMonsters = MapProgress.TotalRareMonsters;
    MapProgress.bHasMapBoss = bInHasMapBoss;
    MapProgress.bBossDefeated = false;

    OnMapProgressUpdated.Broadcast(MapProgress);
    EvaluateCompletionCondition();
}

void UMEOMapManagerSubsystem::NotifyRareMonsterKilled()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || CurrentZoneState != EMEOZoneState::Inprogress)
    {
        return;
    }

    if (MapProgress.RemainingRareMonsters > 0)
    {
        MapProgress.RemainingRareMonsters--;
        OnMapProgressUpdated.Broadcast(MapProgress);
        EvaluateCompletionCondition();
    }
}

void UMEOMapManagerSubsystem::NotifyMapBossDefeated()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (CurrentZoneState == EMEOZoneState::Inprogress || CurrentZoneState == EMEOZoneState::BossEncounter)
    {
        MapProgress.bBossDefeated = true;
        OnMapProgressUpdated.Broadcast(MapProgress);
        EvaluateCompletionCondition();
    }
}

bool UMEOMapManagerSubsystem::ConsumePortal()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return false;
    }

    if (CurrentMapData.RemainingPortals <= 0)
    {
        return false;
    }

    CurrentMapData.RemainingPortals--;
    OnPortalCountChanged.Broadcast(CurrentMapData.RemainingPortals);

    if (CurrentMapData.RemainingPortals == 0 && CurrentZoneState != EMEOZoneState::Completed)
    {
        SetZoneState(EMEOZoneState::Failed);
    }

    return true;
}

float UMEOMapManagerSubsystem::GetAggregatedZoneModifierValue(FName Modifierld) const
{
    float TotalValue = 0.0f;
    //  모디파이 수치를 검색해서 다 더하는것,그게 어떤 모디파이인가? CurrentMapData.ActiveModifiers에 있는 FName Modifierld 이름을 찾아서 더한다.
    for (const FMEOZoneModifierData& Mod : CurrentMapData.ActiveModifiers)
    {
        if (Mod.ModifierId == Modifierld)
        {
            TotalValue += Mod.Magnitude;
        }
    }
    return TotalValue;
}

FMEOLootGenerationContext UMEOMapManagerSubsystem::BuildLootContextForCurrentZone(float PlayerBonusIIQ, float PlayerBonusIIR) const
{
    FMEOLootGenerationContext Context;
    Context.AreaLevel = (CurrentZoneState == EMEOZoneState::Hub) ? 1 : CurrentMapData.AreaLevel;
    Context.IncreasedItemQuantity = CurrentMapData.BonusItemQuantity + PlayerBonusIIQ;
    Context.IncreasedItemRarity = CurrentMapData.BonusItemRarity + PlayerBonusIIR;

    return Context;
}

void UMEOMapManagerSubsystem::SetZoneState(EMEOZoneState NewState)
{
    if (CurrentZoneState == NewState)
    {
        return;
    }
    const EMEOZoneState OldState = CurrentZoneState;
    CurrentZoneState = NewState;
    OnZoneStateChanged.Broadcast(OldState, NewState);

}

void UMEOMapManagerSubsystem::EvaluateCompletionCondition()
{
    if (CurrentZoneState != EMEOZoneState::Inprogress && CurrentZoneState != EMEOZoneState::BossEncounter)
    {
        return;
    }

    const bool bAllRareCleared = (MapProgress.RemainingRareMonsters <= 0);
    const bool bBossRequirementMet = (!MapProgress.bHasMapBoss || MapProgress.bBossDefeated);

    if (bAllRareCleared && bBossRequirementMet)
    {
        SetZoneState(EMEOZoneState::Completed);
    }
}
