// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOEconomySubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEODropSubsystem.h"
#include "MEO_ProjectV2/SubSystem/MEODiceSubsystem.h"
#include "Engine/World.h"

EMEOCraftingResult UMEOEconomySubsystem::CanApplyCurrencyOrb(const FMEOInventoryItemInstance& TargetItem, EMEOCurrencyType CurrencyType) const
{
    if (!TargetItem.IsValid())
    {
        return EMEOCraftingResult::InvalidItem;
    }

    if (TargetItem.bIsCorrupted)
    {
        return EMEOCraftingResult::ItemIsCorrupted;
    }

    int32 PrefixCount = 0;
    int32 SuffixCount = 0;
    CountAffixSlots(TargetItem, PrefixCount, SuffixCount);
    const int32 TotalExplicitAffixes = PrefixCount + SuffixCount;

    switch (CurrencyType)
    {
    case EMEOCurrencyType::Transmutation:
        return (TargetItem.Rarity == EMEOItemRarity::Normal)
            ? EMEOCraftingResult::Success
            : EMEOCraftingResult::InvalidRarityForOrb;

    case EMEOCurrencyType::Augmentation:
        if (TargetItem.Rarity != EMEOItemRarity::Magic) return EMEOCraftingResult::InvalidRarityForOrb;
        return(TotalExplicitAffixes < 2) ? EMEOCraftingResult::Success : EMEOCraftingResult::InvalidRarityForOrb;
    
    case EMEOCurrencyType::Alteration:
        return (TargetItem.Rarity == EMEOItemRarity::Magic)
            ? EMEOCraftingResult::Success
            : EMEOCraftingResult::InvalidItem;

    case EMEOCurrencyType::Regal:
        return (TargetItem.Rarity == EMEOItemRarity::Magic)
            ? EMEOCraftingResult::Success
            : EMEOCraftingResult::InvalidRarityForOrb;

    case EMEOCurrencyType::Exalted:
        if (TargetItem.Rarity != EMEOItemRarity::Rare) return EMEOCraftingResult::InvalidRarityForOrb;
        return (TotalExplicitAffixes < 6) ? EMEOCraftingResult::Success : EMEOCraftingResult::AffixSlotFull;

    case EMEOCurrencyType::Chaos:
        if (TargetItem.Rarity != EMEOItemRarity::Rare)return EMEOCraftingResult::InvalidRarityForOrb;
        return (TotalExplicitAffixes > 0) ? EMEOCraftingResult::Success : EMEOCraftingResult::NoAffixToModify;

    case EMEOCurrencyType::Vaal:
        //  타락되지 않은 모든 등급(고유 포함)에 적용 가능
        return EMEOCraftingResult::Success;

    default:
        return EMEOCraftingResult::InvalidRarityForOrb;
    }
}

EMEOCraftingResult UMEOEconomySubsystem::ApplyCurrencyOrb(FMEOInventoryItemInstance& TargetItem, EMEOCurrencyType CurrencyType, UDataTable* AffixDataTable)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return EMEOCraftingResult::NotServerAuthority;
    }

    const EMEOCraftingResult PreCheck = CanApplyCurrencyOrb(TargetItem, CurrencyType);
    if (PreCheck != EMEOCraftingResult::Success)
    {
        OnItemCrafted.Broadcast(TargetItem.InstanceId, CurrencyType, PreCheck);
        return PreCheck;
    }

    UMEODiceSubsystem* DiceSub = World->GetSubsystem<UMEODiceSubsystem>();
    EMEOCraftingResult FinalResult = EMEOCraftingResult::Success;

    switch (CurrencyType)
    {
    case EMEOCurrencyType::Transmutation:
    {
        //  일반 -> 마법 승급 후 접사 1~2개 부여
        TargetItem.Rarity = EMEOItemRarity::Magic;
        const int32 RollCount = (DiceSub && DiceSub->RollChance(0.5f)) ? 2 : 1;
        for (int32 i = 0; i < RollCount; ++i)
        {
            if (!TryAddRandomAffix(TargetItem, AffixDataTable, 1) && i == 0)
            {
                FinalResult = EMEOCraftingResult::NoVaildAffixInPool;
            }
        }
        break;
    }
    case EMEOCurrencyType::Augmentation:
    {
        //  마법 아이템의 빈 슬룻(접두 또는 접미)에 접사 1개 추가
        if (!TryAddRandomAffix(TargetItem, AffixDataTable, 1))
        {
            FinalResult = EMEOCraftingResult::NoVaildAffixInPool;
        }
        break;
    }
    case EMEOCurrencyType::Alteration:
    {
        //  기존 접두/접미사 제거(고정 속성 유지) 후 마법 접사 1~2개 재굴림
        TargetItem.Modifiers.RemoveAll([](const FMEOItemModifierInstance& Mod)
            {
                return Mod.SlotType != EMEOAffixSlot::Implicit;
            }
        );

        const int32 RollCount = (DiceSub && DiceSub->RollChance(0.5f)) ? 2 : 1;
        for (int32 i = 0; i < RollCount; ++i)
        {
            if (!TryAddRandomAffix(TargetItem, AffixDataTable, 1) && i == 0)
            {
                FinalResult = EMEOCraftingResult::NoVaildAffixInPool;
            }
        }
        //  마법 접사 1개 굴리는걸로 생각했는데,2개 굴리는것도 확률로 넣어놧네;
        break;
    }
    case EMEOCurrencyType::Regal:
    {
        // 마법->희귀 승급 후 기존 옵션 보존 상태에서 접사 1개 추가 (최대 3/3 슬룻)
        TargetItem.Rarity = EMEOItemRarity::Rare;
        if (!TryAddRandomAffix(TargetItem, AffixDataTable, 3))
        {
            FinalResult = EMEOCraftingResult::NoVaildAffixInPool;
        }
        break;
    }
    case EMEOCurrencyType::Exalted:
    {
        //  희귀 아이템에 접사 1개 추가 (최대 6) (3/3 슬룻)
        if (!TryAddRandomAffix(TargetItem, AffixDataTable, 3))
        {
            FinalResult = EMEOCraftingResult::NoVaildAffixInPool;
        }
        break;
    }
    case EMEOCurrencyType::Chaos:
    {
        //  POE2 방식 : 희귀 아이템의 무작위 접사 1개를 제거한 뒤 새로운 접사 1개 추가
        if (TryRemoveRandomAffix(TargetItem))
        {
            if (!TryAddRandomAffix(TargetItem, AffixDataTable, 3))
            {
                FinalResult = EMEOCraftingResult::NoVaildAffixInPool;
            }
        }
        else
        {
            FinalResult = EMEOCraftingResult::NoAffixToModify;
        }
        break;
    }
    case EMEOCurrencyType::Vaal:
    {
        FinalResult = ApplyVaalCorruption(TargetItem, AffixDataTable);
        break;
    }
    default:
    {
        FinalResult = EMEOCraftingResult::InvalidRarityForOrb;
        break;
    }
    }

    OnItemCrafted.Broadcast(TargetItem.InstanceId, CurrencyType, FinalResult);
    return FinalResult;
}

void UMEOEconomySubsystem::CountAffixSlots(const FMEOInventoryItemInstance& Item, int32& OutPrefixCount, int32& OutSuffixCount) const
{
    OutPrefixCount = 0;
    OutSuffixCount = 0;

    for (const FMEOItemModifierInstance& Mod : Item.Modifiers)
    {
        if (Mod.SlotType == EMEOAffixSlot::Prefix)
        {
            OutPrefixCount++;
        }
        else if (Mod.SlotType == EMEOAffixSlot::Suffix)
        {
            OutSuffixCount++;
        }
    }
}

bool UMEOEconomySubsystem::TryAddRandomAffix(FMEOInventoryItemInstance& Item, UDataTable* AffixDataTable, int32 MaxPerSlot) const
{
    UWorld* World = GetWorld();
    UMEODropSubsystem* DropSub = World ? World->GetSubsystem<UMEODropSubsystem>() : nullptr;
    UMEODiceSubsystem* DiceSub = World ? World->GetSubsystem<UMEODiceSubsystem>() : nullptr;
    if (!DropSub || !DiceSub || !AffixDataTable)
    {
        return false;
    }

    int32 PrefixCount = 0;
    int32 SuffixCount = 0;
    CountAffixSlots(Item, PrefixCount, SuffixCount);

    //  슬룻이 꽉찼는지 확인
    TArray<EMEOAffixSlot> AvailableSlots;
    if (PrefixCount < MaxPerSlot)AvailableSlots.Add(EMEOAffixSlot::Prefix);
    if (SuffixCount < MaxPerSlot)AvailableSlots.Add(EMEOAffixSlot::Suffix);

    if (AvailableSlots.Num() == 0)
    {
        return false;
    }

    //  비어 있는 슬룻 중 하나를 무작위 선택
    const EMEOAffixSlot ChosenSlot = (AvailableSlots.Num() == 1)
        ? AvailableSlots[0]                                         //  비어있는 슬룻이 1개라면 그 슬룻을 선택(EMEOAffixSlot[1]이 없다는뜻)
        : AvailableSlots[DiceSub->RollChance(0.5f) ? 1 : 0];        //  Prefix 와 Suffix 둘중에 빈슬룻을 선택 AvailableSlots[0] , AvailableSlots[1]

    //  이미 붙어있는동일 스텟 그룹 제외 목록 수집
    TArray<FName> ExcludedTags;
    ExcludedTags.Reserve(Item.Modifiers.Num());
    for (const FMEOItemModifierInstance& Existing : Item.Modifiers)
    {
        ExcludedTags.Add(Existing.StatTag);
    }

    FMEOItemModifierInstance NewAffix;
    if (DropSub->RollRandomAffixFromTable(AffixDataTable, Item.ItemLevel, ChosenSlot, ExcludedTags, NewAffix))  //  여기서 false가 나는 경우는 도데체 어떤경우지?
    {
        Item.Modifiers.Add(NewAffix);
        return true;
        //  왜 화폐적용해서 랜덤 돌리는게 DropSub에 있는지 잠깐 고민을 해봤는데
        //  에초에 드랍 아이템에 마법이나 희귀 아이템에 접사 붙여야하니깐 대분류로 드랍서브시스템에 있는건가?
        //  어차피 NewAffix에 랜덤 접사가 붙기만 하면 되니깐,RollRandomAffixFromTable위치 같은건 어디여도 상관없는건가?
    }

    return false;
}

bool UMEOEconomySubsystem::TryRemoveRandomAffix(FMEOInventoryItemInstance& Item) const
{
    UMEODiceSubsystem* DiceSub = GetWorld()  ? GetWorld()->GetSubsystem<UMEODiceSubsystem>() : nullptr;

    if (!DiceSub)
    {
        return false;
    }

    //  고정 속성(Implicit)을 제외한 접두/접미사 인덱스만 수집
    TArray<int32> RemovableIndices;
    for (int32 i = 0; i < Item.Modifiers.Num(); ++i)
    {
        if (Item.Modifiers[i].SlotType != EMEOAffixSlot::Implicit)
        {
            RemovableIndices.Add(i);
        }
    }
    if (RemovableIndices.Num() == 0)
    {
        return false;
    }

    const int32 Pick = FMath::Clamp(FMath::RoundToInt(DiceSub->RollDice(1, RemovableIndices.Num(), -1.0f)), 0, RemovableIndices.Num() - 1);
    Item.Modifiers.RemoveAtSwap(RemovableIndices[Pick]);
    return true;
}

EMEOCraftingResult UMEOEconomySubsystem::ApplyVaalCorruption(FMEOInventoryItemInstance& Item, UDataTable* AffixDataTable) const
{
    //  타락 플래그 영구 활성화
    Item.bIsCorrupted = true;

    UMEODropSubsystem* DropSub = GetWorld() ? GetWorld()->GetSubsystem<UMEODropSubsystem>() : nullptr;
    UMEODiceSubsystem* DiceSub = GetWorld() ? GetWorld()->GetSubsystem<UMEODiceSubsystem>() : nullptr;
    if (!DiceSub || !DropSub)
    {
        return::EMEOCraftingResult::Success;
    }

    //  4가지 타락 결과 균등 추첨 (25% 확률 분배)
    const int32 Outcome = FMath::RoundToInt(DiceSub->RollDice(1, 4, 0.0f));
    switch (Outcome)
    {
    case 1:
        //  변화 없음(타락 잠금만 적용)
        break;
    case 2:
    {
        //  타락 전용 고정 속성(implicit)
        TArray<FName> EmptyExclusions;
        FMEOItemModifierInstance VaalImplict;
        if (DropSub->RollRandomAffixFromTable(AffixDataTable, Item.ItemLevel, EMEOAffixSlot::Implicit, EmptyExclusions, VaalImplict))
        {
            Item.Modifiers.Add(VaalImplict);
        }
        break;
    }
    case 3:
    {
        //  고유 등급이 아닐 경우 접사 1개 제거 후 1개 추가 변이
        if (Item.Rarity != EMEOItemRarity::Unique)
        {
            const int32 MaxSlot = (Item.Rarity == EMEOItemRarity::Rare) ? 3 : 1;    //  와 어차피 고유 빼면 접두/접미 가 각각 3개 아니면 1개라는걸 이렇게 작성하네?
            TryRemoveRandomAffix(Item);
            TryAddRandomAffix(Item, AffixDataTable, MaxSlot);
        }
        break;
    }
    case 4:
    default:
    {
        //  고유 등급이 아닐 경우 희귀 등급으로 강제 변환 후 접사 재굴림(Brick 또는 대박)
        if (Item.Rarity != EMEOItemRarity::Unique)
        {
            Item.Rarity = EMEOItemRarity::Rare;
            Item.Modifiers.RemoveAll([](const FMEOItemModifierInstance& Mod)
                {
                    return Mod.SlotType != EMEOAffixSlot::Implicit;
                });

            const int32 NewCount = FMath::RoundToInt(DiceSub->RollDice(1, 3, 3.0f));    //  4~6개
            for (int32 i = 0; i < NewCount; ++i)
            {
                TryAddRandomAffix(Item, AffixDataTable, 3);
            }
        }
        break;

        //  아 접사의 수치를 재굴림 하는게 아니라,그냥 고유 접사 빼고 지워버리고 새롭게 박는거구나?
        //  그럼 타락을 시켰을때 수치를 재굴림 하는건 어디갓지?
        //  예를 들어 화염 피해(1~34) 추가 를 화염 피해 (4~28) 로 만든다든가
        //  물리 피해 추가 200% (100%~400%)인곳에서 410%를 찍게한다든가?
        //  이런건 어디서 해줘야하는거지?
    }
    }
    return EMEOCraftingResult::Success;
}
