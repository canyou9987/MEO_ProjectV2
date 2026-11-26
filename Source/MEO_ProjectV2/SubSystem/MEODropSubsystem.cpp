// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEODropSubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEODiceSubsystem.h"
#include "Engine/World.h"

TArray<FMEOInventoryItemInstance> UMEODropSubsystem::GenrateLootDrops(const TArray<FMEOLootTableEntry>& LootTable, const FMEOLootGenerationContext& Context, int32 BaseDropsRolls)
{
    TArray<FMEOInventoryItemInstance> DroppedItems;

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || LootTable.Num() == 0 || BaseDropsRolls <= 0)
    {
        return DroppedItems;
    }

    UMEODiceSubsystem* DiceSub = World->GetSubsystem<UMEODiceSubsystem>();
    if (!DiceSub)
    {
        return DroppedItems;
    }

    //  1.  아이템 수량 증가(IIQ) 반영하여 실제 드랍 시도 횟수 산출
    //  예: BaseDropRolls = 2 IIQ = 0.5f(50%) -> 기대 횟수 3.0회 (소수점은 확률 굴림)
    const float ExpeectedRolls = static_cast<float>(BaseDropsRolls) * FMath::Max(0.0f, 1.0f + Context.IncreasedItemQuantity);
    int32 FinalDropCount = FMath::FloorToInt(ExpeectedRolls);
    const float FractionalChance = ExpeectedRolls - static_cast<float>(FinalDropCount);
    if(DiceSub->RollChance(FractionalChance))
    {
        FinalDropCount++;
    }

    //  2.루트 테이블 가중치 배열 추출
    TArray<float> Weights;
    Weights.Reserve(LootTable.Num());
    for (const FMEOLootTableEntry& Entry : LootTable)
    {
        Weights.Add(Entry.DropWeight);
    }

    //  3.결정된 횟수만큼 가중치 추첨 및 서버 권한 전역 고유 식별자(FGuid) 발급
    for (int32 i = 0; i < FinalDropCount; ++i)
    {
        const int32 PickedIndex = DiceSub->RollWeightedIndex(Weights);
        if (!LootTable.IsValidIndex(PickedIndex))
        {
            continue;
        }

        const FMEOLootTableEntry& ChosenEntry = LootTable[PickedIndex];
        if (ChosenEntry.ItemBaseRowName.IsNone())
        {
            continue;
        }

        FMEOInventoryItemInstance NewItem;
        NewItem.InstanceId = FGuid::NewGuid();
        NewItem.ItemBaseRowName = ChosenEntry.ItemBaseRowName;
        NewItem.ItemLevel = FMath::Max(1, Context.AreaLevel);

        //  화폐/소모품 중첩 수량 결정
        const int32 MinS = FMath::Max(1, ChosenEntry.MinStack);
        const int32 MaxS = FMath::Max(MinS, ChosenEntry.MaxStack);
        NewItem.StackCount = (MinS == MaxS)
            ? MinS
            : FMath::RoundToInt(DiceSub->RollDice(1, MaxS - MinS + 1, static_cast<float>(MinS - 1)));

        DroppedItems.Add(NewItem);
    }

    return DroppedItems;
}

EMEOItemRarity UMEODropSubsystem::RollItemRarity(float IncreadItmeRarity) const
{
    UWorld* World = GetWorld();
    UMEODiceSubsystem* DiceSub = World ? World->GetSubsystem<UMEODiceSubsystem>() : nullptr;
    if (!DiceSub)
    {
        return EMEOItemRarity::Normal;
    }

    //  기본 가중치: 일반(1000),마법(300),희귀(80),고유(5)
    //  IIR이 높을수록 상위 희귀도 가중치가 비례 증폭됨
    const float RarityFactor = FMath::Max(0.0f, 1.0f + IncreadItmeRarity);
    TArray<float> RarityWeights = {
        1000.0f,
        300.0f * RarityFactor,
        80.0f * RarityFactor,
        5.0f * RarityFactor
    };
    const int32 Index = DiceSub->RollWeightedIndex(RarityWeights);
    switch (Index)
    {
    case 1:   return EMEOItemRarity::Magic;
    case 2:   return EMEOItemRarity::Rare;
    case 3:   return EMEOItemRarity::Unique;
    default: return EMEOItemRarity::Normal;
    }
}

bool UMEODropSubsystem::RollRandomAffixFromTable(UDataTable* AffixDataTable, int32 ItemLevel, EMEOAffixSlot TargetSlot, const TArray<FName> ExcludedStatTags, FMEOItemModifierInstance& OutAffix) const
{
    UWorld* World = GetWorld();
    UMEODiceSubsystem* DiceSub = World ? World->GetSubsystem<UMEODiceSubsystem>() : nullptr;
    if (!AffixDataTable || !DiceSub)
    {
        return false;
    }

    TArray<FName> CandidateRowNames;
    TArray<const FMEOAffixTableRow*> CandidateRows;
    TArray<float> CandidateWeights;

    //  1.슬룻 타입, 아이템 레벨 요구치,동일 옵션 그룹 중복 여부를 필터링하여 유효 후보군 수집
    const TArray<FName>AllRowNames = AffixDataTable->GetRowNames();
    for (const FName& RowName : AllRowNames)
    {
        const FMEOAffixTableRow* Row = AffixDataTable->FindRow<FMEOAffixTableRow>(RowName, TEXT("RollRandomAffix"));
        if (!Row)
        {
            continue;
        }

        if (Row->SlotType != TargetSlot || Row->RequiredItemLevel > ItemLevel || Row->SpawnWeight <= 0.0f)
        {
            continue;
        }

        //  이미 붙어있는 동일 스텟 그룹(예: 최대 생명력 +50이 있는데 또 최대 생명력 +20 이 붙는 현상)베제
        if (ExcludedStatTags.Contains(Row->StatTag))
        {
            continue;
        }

        CandidateRowNames.Add(RowName);
        CandidateRows.Add(Row);
        CandidateWeights.Add(Row->SpawnWeight);
    }

    if (CandidateRows.Num() == 0)
    {
        return false;
    }

    //  2.가중치 추첨 및 최소~최대 범위 내 스텟 수치 굴림
    const int32 PickedIdx = DiceSub->RollWeightedIndex(CandidateWeights);
    if (!CandidateRows.IsValidIndex(PickedIdx))
    {
        return false;
    }

    const FMEOAffixTableRow* Chosen = CandidateRows[PickedIdx];
    OutAffix.AffixRowName = CandidateRowNames[PickedIdx];
    OutAffix.SlotType = Chosen->SlotType;
    OutAffix.StatTag = Chosen->StatTag;
    OutAffix.Tier = Chosen->Tier;

    //  정수형/실수형 모두 대응 가능한 균등 범위 보간
    const float RollAlpha = DiceSub->RollDice(1, 1000, -1.0f) / 999.0f;
    OutAffix.RolledValue = FMath::Lerp(Chosen->MinValue, Chosen->MaxValue, FMath::Clamp(RollAlpha, 0.0f, 1.0f));

    return true;
}

//  추후에 GenrateLootDrops에서 떨어트린 아이템이 무엇인지 정의하는 데이터 테이블 도 필요할것이고
//  떨어트린 아이템이 장비품이거나 그런거라면 RollRandomAffixFromTable()를 할수있도록 체크도 어디서 해줘야겠네? 근데 어디서 반영하지?
//  RollItemRarity로 희귀도가 결정되면 RollRandomAffixFromTable를 통해 적어도 최소 갯수만큼은(마법이면 1~2개 희귀면 3~6개 박아줘야겠지?)
//  지금 보기에는 딱 저거네,아이템을 드랍하고 (GenrateLootDrops)그게 화폐가 아니라 장비품이라면 희귀도를 결정하고(RollItemRarity)
//  희귀도가 결정되면 그안에 어떤 접사가 들어갈지 결정하는걸 하고(RollRandomAffixFromTable)
//  그런데? 이건 서브시스템 이므로 연산을 하는법만 여기서 적는거지 실행하는 주체가 아니잖아?
//  그러면 이것들을 실행하는 주체는 GM인가?(GameModeBase) 아니면 죽는 몬스터가 하는건가? 그것도 아니라면 죽이는 플레이어가 하는건가?
//  확실한건 적어도 플레이어가 호출하는건 말인 안되고,GameModeBase같은 게임 매니저 계열이 유력해 보이는데?
