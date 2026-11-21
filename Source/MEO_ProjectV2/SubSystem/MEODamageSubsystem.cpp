// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEODamageSubsystem.h"

FMEODamagePacket UMEODamageSubsystem::CalculateDamage(FMEODamagePacket InputPacket)
{
	//-------------------------------------------------------------------------
	//	[Stage 1] 기본 데미지(Base) + 플랫 추가 데미지(Added Flat) 합산
	//-------------------------------------------------------------------------
	FMEODamagePool Stage1Pool;
	Stage1Pool.Physical =		InputPacket.BaseDamage.Physical		+ InputPacket.AddedFlatDamage.Physical;
	Stage1Pool.Fire=			InputPacket.BaseDamage.Fire			+ InputPacket.AddedFlatDamage.Fire;
	Stage1Pool.Cold =			InputPacket.BaseDamage.Cold			+ InputPacket.AddedFlatDamage.Cold;
	Stage1Pool.Lightning =		InputPacket.BaseDamage.Lightning	+ InputPacket.AddedFlatDamage.Lightning;
	Stage1Pool.Chaos =			InputPacket.BaseDamage.Chaos		+ InputPacket.AddedFlatDamage.Chaos;

	//-------------------------------------------------------------------------
	//	[Stage 2] 속성 전환 (Damage Conversion)
	//-------------------------------------------------------------------------

	const FMEODamagePool Stage2Pool = ApplyDamageConversion(Stage1Pool,InputPacket.ConverasionRules);
	
	//	합산된 데미지를 ConverasionRules에 따라 데미지 속성을 전환한다.
	//	근데 합산은 하지만 바꾸지 않는 데미지들은 어떻게 되는거지?
	//	"물리피해를 100%로 화염피해로 전환"이면 나머지 번개피해나 냉기피해는 그대로 남나?

	//-------------------------------------------------------------------------
	//	[Stage 3] 합연산(Increased) 및 곱연산(More) 배율 적용
	//-------------------------------------------------------------------------
	FMEODamagePool Stage3Pool = ApplyIncreaseAndMoreModifiers(Stage2Pool, InputPacket.StatSnapShot);

	//-------------------------------------------------------------------------
	//	[Stage 4] (Accuracy vs Evasion) 및 치명타 (Critical Strike) 판정
	//-------------------------------------------------------------------------
	if (InputPacket.bCanBeEvaded)
	{
		const bool bHit = RollHitCheck(InputPacket.StatSnapShot.AccuracyRating, InputPacket.StatSnapShot.EvasionRating);
		if (!bHit)
		{
			//	회피 성공시 이후 방어 연산 없이 0데미지로 즉시 종료
			InputPacket.bIsEvaded = true;
			InputPacket.TotalFinalDamage = 0.0f;
			return InputPacket;
		}

	}

	//	치명타 확률 롤링 (추후 UMEODiceSubsystem 엔트로피 롤링 연동 가능)
	const float CritRoll = FMath::FRand();
	if (CritRoll < FMath::Clamp(InputPacket.StatSnapShot.CriticalStrikeChance, 0.0f, 1.0f))
	{
		InputPacket.bIsCriticalHit = true;
		const float CritMult = FMath::Max(1.0f, InputPacket.StatSnapShot.CriticalDamageMultiplier);

		Stage3Pool.Physical *= CritMult;
		Stage3Pool.Fire *= CritMult;
		Stage3Pool.Cold *= CritMult;
		Stage3Pool.Lightning *= CritMult;
		Stage3Pool.Chaos *= CritMult;
	}
	//	롤링을 여기서 하긴 하지만,추후 UMEODiceSubsystem를 구독을 해서 RollDice() 같은 형태로 굴리게 할예정
	//	여기서 그냥 FRand()로 범위값을 해도 상관은 없지만,
	//	최소 최대를 받아 균등 분포 난수를 생성하고 RandomEngine를 돌려 난수를 얻어오는 
	//	C++ 표준 라이브러리인 <random> 기반 난수 엔진인 std::mt19937를 가져오는것도 괜찮은 생각일것 같다.
	//	(난수 엔진을 처음부터 만들 생각은 없다.공부를 위해 만든다면 모를까)

	//-------------------------------------------------------------------------
	//	[Stage 5] 방어 기제 감쇄 (방어도 및 원소/카오스 저항,관통력 연산)
	//-------------------------------------------------------------------------
    InputPacket.MitigatedDamage = Stage3Pool;
	InputPacket = EvaluateMitigation(InputPacket);

	//-------------------------------------------------------------------------
	//	[Stage 6] 에너지 보호막(ES) 및 생명력(Life) 최종 피해 배분
	//-------------------------------------------------------------------------
	DistributeFinalDamageToPools(InputPacket);

	return InputPacket;
}

FMEODamagePacket UMEODamageSubsystem::EvaluateMitigation(FMEODamagePacket DamagePacket)
{
    const FMEOCombatStatSnapshot& Stats = DamagePacket.StatSnapShot;
    FMEODamagePool& DmgPool = DamagePacket.MitigatedDamage;

    //  1.물리 피해: 방어도(Armor) 기반 경감률 적용 (최대 90% 경감 캡)
    if (DmgPool.Physical > 0.0f)
    {
        const float PhysReduction = CalculateArmorReduction(Stats.Armor, DmgPool.Physical);
        DmgPool.Physical *= (1.0f - PhysReduction);
    }

    //  2.원소 및 카오스 피해: 유효 저항력 = Clamp(저항력, - 1.0,0.75) - 관통력
    auto ApplyResistanceMitigation = [](float RawDmg, float Resist, float Penetration) -> float
        {
            if (RawDmg <= 0.0f) return 0.0f;    //  데미지 없으면 그냥 0.0f

            const float CappedResist = FMath::Min(Resist, 0.75f);
            const float EffectiveResist = FMath::Clamp(CappedResist - Penetration, -2.0f, 0.75f);
            return RawDmg * (1.0f - EffectiveResist);
        };
    DmgPool.Fire        = ApplyResistanceMitigation(DmgPool.Fire,       Stats.Resitances.Fire,      Stats.ElementalPentration.Fire);
    DmgPool.Cold        = ApplyResistanceMitigation(DmgPool.Cold,       Stats.Resitances.Cold,      Stats.ElementalPentration.Cold);
    DmgPool.Lightning   = ApplyResistanceMitigation(DmgPool.Lightning,  Stats.Resitances.Lightning, Stats.ElementalPentration.Lightning);
    DmgPool.Chaos       = ApplyResistanceMitigation(DmgPool.Chaos,      Stats.Resitances.Chaos,     Stats.ElementalPentration.Chaos);

	return DamagePacket;
    //  ...? DamagePacket에 다시 넣은게 없는데?
    //  아 FMEODamagePool& 와 FMEOCombatStatSnapshot&로 주소값으로 값을 넣었지?
    //  아하
}

FMEODamagePool UMEODamageSubsystem::ApplyDamageConversion(const FMEODamagePool& RawPool, const TArray<FMEODamageConversionRule>& Rules) const
{
	FMEODamagePool ResultPool = RawPool;

    //  무한 루프 방지를 위한 단방향 전환 순서 고정: Physical -> Lightning -> Cold -> Fire -> Chaos
    const TArray<EMEODamageType> ConversionOrder = {
        EMEODamageType::Physical,
        EMEODamageType::Lightning,
        EMEODamageType::Cold,
        EMEODamageType::Fire
    };

    for (EMEODamageType SourceType : ConversionOrder)
    {
        const float SourceAmount = ResultPool.GetByType(SourceType);
        if(SourceAmount <= 0.0f)    //  사실상 데미지가 없으면 탈출
        {
            continue;
        }

        //  현재 SourceType에서 다른 속성으로 나가는 전환 규칙 필터링 및 총 비율 합산
        float TotalRatio = 0.0f;
        TArray<FMEODamageConversionRule> ValidRules;

        for (const FMEODamageConversionRule& Rule : Rules)
        {
            //  역방향 전환 및 동일 속성 전환 차단
            if (Rule.SourceType == SourceType && Rule.TargetType > SourceType && Rule.ConversionRation > 0.0f)
            {
                ValidRules.Add(Rule);
                TotalRatio += Rule.ConversionRation;
            }
        }

        if (ValidRules.Num() == 0)  //  확정된 룰이 없으면 탈출
        {
            continue;
        }

        //  전환 비율 총합이 100%(1.0)를 초과할 경우 비율대로 스케일 다운(정규화)하여 데미지 복사 버그를 방지
        const float ScaleFactor = (TotalRatio > 1.0f) ? (1.0f / TotalRatio) : 1.0f;
        float TotalConvertedOut = 0.0f;

        for (const FMEODamageConversionRule& Rule : ValidRules)
        {
            const float EffectiveRation = Rule.ConversionRation * ScaleFactor;
            const float ConvertedAmount = SourceAmount * EffectiveRation;

            //  1. 목표 속성에 전환된 데미지 누적
            ResultPool.SetByType(Rule.TargetType, ResultPool.GetByType(Rule.TargetType) + ConvertedAmount);
            TotalConvertedOut += ConvertedAmount;

        }
        //  2. 루프 종료 후 전환된 총량 만큼 원본 속성 풀에서 차감
        ResultPool.SetByType(SourceType, FMath::Max(0.0f, SourceAmount - TotalConvertedOut));
        //  FMath::Max(0.0f, SourceAmount - TotalConvertedOut)값을 SourceType으로 전환
        //  근데 전환을 한다고 하더라도, 번개피해나 냉기 피해가 조금씩 추가되는 경우가 있는데
        //  이건 그냥 데미지 전부를 변환시키는게 아닌가?
        //  (편해서 좋긴한데;)
    }
	return ResultPool;
}

FMEODamagePool UMEODamageSubsystem::ApplyIncreaseAndMoreModifiers(const FMEODamagePool& ConvertedPool, const FMEOCombatStatSnapshot& Stats) const
{
    FMEODamagePool OutPool;

    //  공식: ConvertedDamage * (1.0 + SumOfIncreased) * ProductOfMore
    auto ComputeTypeDamage = [](float BaseVal, float IncPct, float MoreMult) -> float
        {
            if (BaseVal <= 0.0f) return 0.0f;
            return BaseVal * FMath::Max(0.0f, 1.0f + IncPct) * FMath::Max(0.0f, MoreMult);
        };
    OutPool.Physical    = ComputeTypeDamage(ConvertedPool.Physical,     Stats.IncreasedDamagePercent.Physical,  Stats.MoreDamageMultiplier.Physical);
    OutPool.Fire        = ComputeTypeDamage(ConvertedPool.Fire,         Stats.IncreasedDamagePercent.Fire,      Stats.MoreDamageMultiplier.Fire);
    OutPool.Cold        = ComputeTypeDamage(ConvertedPool.Cold,         Stats.IncreasedDamagePercent.Cold,      Stats.MoreDamageMultiplier.Cold);
    OutPool.Lightning   = ComputeTypeDamage(ConvertedPool.Lightning,    Stats.IncreasedDamagePercent.Lightning, Stats.MoreDamageMultiplier.Lightning);
    OutPool.Chaos       = ComputeTypeDamage(ConvertedPool.Chaos,        Stats.IncreasedDamagePercent.Chaos,     Stats.MoreDamageMultiplier.Chaos);

    return OutPool;
}

bool UMEODamageSubsystem::RollHitCheck(float Accuracy, float Evasion) const
{
    if (Evasion <= 0.0f)
    {
        return true;
    }
    //  명중 확률 공식 Accuracy / (Accuracy + (Evasion * 0.25)^0.8). 최소 5% ~ 최대 100%  보장
    const float RawHitChance = Accuracy / (Accuracy + FMath::Pow(Evasion * 0.25f, 0.8f));
    const float ClampedHitChacne = FMath::Clamp(RawHitChance, 0.05f, 1.0f);

    return FMath::FRand() <= ClampedHitChacne;
    //  UMEODiceSubsystem을 통해 난수엔진으로 대체 예정

}

float UMEODamageSubsystem::CalculateArmorReduction(float Armor, float RawPhysicalDamage) const
{
    if (Armor <= 0.0f || RawPhysicalDamage <= 0.0f)
    {
        return 0.0f;    //  Armor 또는 RawPhysicalDamage이 0이면 감쇄 연산은 없다.
    }

    //  방어도 공식: Armor / (Armor + 5*RawPhysicalDamage), 최대 90% 감쇄제한
    const float Reduction = Armor / (Armor + (5.0f * RawPhysicalDamage));
    return FMath::Clamp(Reduction, 0.0f, 0.90f);
}

void UMEODamageSubsystem::DistributeFinalDamageToPools(FMEODamagePacket& InOutPacket) const
{
    const FMEODamagePool& FinalPool = InOutPacket.MitigatedDamage;
    const float AvailableES = FMath::Max(0.0f, InOutPacket.StatSnapShot.CurrentEnergyShield);

    //  물리 및 원소 피해는 에너지 보호막(ES)이 먼저 흡수하고,초과분만 생명력(Life)으로 전달
    const float NonChaosDamage = FinalPool.Physical + FinalPool.Fire + FinalPool.Cold + FinalPool.Lightning;
    const float AbsorbedByES = FMath::Min(AvailableES, NonChaosDamage);
    const float OverflowToLife = FMath::Max(0.0f, NonChaosDamage - AbsorbedByES);

    //  카오스 피해는 에너지 보호막을 우회하여 생명력에 직접피해
    InOutPacket.FinalDamageToEnergyShield = AbsorbedByES;
    InOutPacket.FinalDamageToLife = OverflowToLife + FinalPool.Chaos;
    InOutPacket.TotalFinalDamage = InOutPacket.FinalDamageToEnergyShield + InOutPacket.FinalDamageToLife;
}
