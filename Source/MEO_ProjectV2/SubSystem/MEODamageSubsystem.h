// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEO_ProjectV2/DataStruct/MEOCombatTypes.h"
#include "MEODamageSubsystem.generated.h"

UCLASS()
class MEO_PROJECTV2_API UMEODamageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	//	6단계 데미지 파이프라인 전체 실행 (기본 합산 -> 전환 -> 증폭 -> 명중/치명타 -> 방어감쇄 -> ES/생명력 배분)
	UFUNCTION(BlueprintCallable, Category = "MEO|Combat")
	FMEODamagePacket CalculateDamage(FMEODamagePacket InputPacket);

	//	[Stage 5] 방어자의 방어도(물리 감쇠) 및 원소/카오스 저항 관통력 감쇄 연산
	UFUNCTION(BlueprintCallable, Category = "MEO|Combat")
	FMEODamagePacket EvaluateMitigation(FMEODamagePacket DamagePacket);

private:
	//	[Stage 2 ] 속성 전환 처리 (Physical -> Lightning -> Cold -> Fire -> Chaos 단방향 전환 및 100% 초과시 정규화)
	FMEODamagePool ApplyDamageConversion(const FMEODamagePool& RawPool, const TArray<FMEODamageConversionRule>& Rules) const;

	//	[Stage 3 ] 합연산(Increased) 및 곱연산(More) 적용
	FMEODamagePool ApplyIncreaseAndMoreModifiers(const FMEODamagePool& ConvertedPool, const FMEOCombatStatSnapshot& Stats)const;

	//	[Stage 4 ] 공격자 명중률(Accurancy) 대비 방어자 회피율(Evasion) 명중 확률 계산
	bool RollHitCheck(float Accuracy, float Evasion)const;

	//	[Stage 5 헬퍼 ] 물리 방어도 피해 감쇄 공식: Armor / (Armor + 5 *RawPhysicalDamage)
	float CalculateArmorReduction(float Armor, float RawPhysicalDamage) const;

	//	[Stage 6 ] 에너지 보호막(ES) 우선 차감 및 카오스 피해 생명력 직격 배분
	void DistributeFinalDamageToPools(FMEODamagePacket& InOutPacket) const;
};
