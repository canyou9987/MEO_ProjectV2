// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MEOCombatTypes.generated.h"

//	5대 피해 속성 구분
UENUM(BlueprintType)
enum class EMEODamageType : uint8
{
	Physical,			//	물리
	Fire,				//	화염
	Cold,				//	냉기
	Lightning,			//	번개
	Chaos				//	카오스 (ES 관통 속성)
};

UENUM(BlueprintType)
enum class EMEOResource : uint8
{
	Life,				//	생명력 
	Mana,				//	마나
	EnergyShiel,		//	에너지 보호막  = Es
	Sprit				//	정신력 (오라,버프,소환수,영구 점유 자원)
};

//	5대 속성별 데미지 수치 컨테이너
USTRUCT(BlueprintType)
struct FMEODamagePool
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Physical = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Fire = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Cold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Lightning = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Chaos = 0.0f;

	//	특정 속성 수치 조회 핼퍼
	float GetByType(EMEODamageType type) const
	{
		switch (type)
		{
		case EMEODamageType::Physical:			return Physical;
		case EMEODamageType::Fire:				return Fire;
		case EMEODamageType::Cold:				return Cold;
		case EMEODamageType::Lightning:			return Lightning;
		case EMEODamageType::Chaos:				return Chaos;
		default:	return 0.0f;
		}
	}
	//	EMEODamageType 타입을 조회하면 FMEODamagePool에서 값을 조회한다?

	//	특정 속성 수치 설정 핼퍼
	void SetByType(EMEODamageType type,float Value)
	{
		switch (type)
		{
		case EMEODamageType::Physical:			Physical = Value; break;
		case EMEODamageType::Fire:				Fire = Value; break;
		case EMEODamageType::Cold:				Cold = Value; break;
		case EMEODamageType::Lightning:			Lightning = Value; break;
		case EMEODamageType::Chaos:				Chaos = Value; break;
		default:	break;
		}
	}
	//	설정할 EMEODamageType 타입을 조회하면 FMEODamagePool의 값을 Value만큼 설정한다?

	//	모든 속성 피해 합계 반환
	float GeTotal() const
	{
		return Physical + Fire + Cold + Lightning + Chaos;
	}
	//	이렇게 모든 피해를 합계해서 보내버리면,
	//  플레이어가 A몬스터에게 얼만큼의 화염피해를 줬는지를 알수가있나?아니면 번개피해를?
	//	그래야지 감전 확율과 인화성 확률과 점화 확률,동결 퍼센트를 어떻게 아는거지?
	//	그렇다고 데미지를 한번에 안입히는것도 이상한데?
	//	여기서는 걱정안해도 되는일인가? 나중에 데미지 설정할때만 보면 되나?
};

//	속성 전환 규칙 (예 : 물리 피해의 50%를 화염 피해로 전환)
USTRUCT(BlueprintType)
struct FMEODamageConversionRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
	EMEODamageType SourceType = EMEODamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
	EMEODamageType TargetType = EMEODamageType::Fire;

	//	전환 비율 (0.0 ~ 1.0, 동일 소스 합산 1.0 초과시 자동 정규화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
	float ConversionRation = 0.0f;

	//	전환 에 쓰이는 구조체, 구현된 함수 X
};

//	공격자/방어자 스텟 스냅샷 (데미지 파이프라인 입력용)
USTRUCT(BlueprintType)
struct FMEOCombatStatSnapshot
{
	GENERATED_BODY()

	//	--- 공격자 스텟 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
	float AccuracyRating = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
	float CriticalStrikeChance = 0.05f;		//	기본 5% (0.05)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
	float CriticalDamageMultiplier = 1.5f;	//	기본 150% (1.5)

	//	합연산 증가(%) 총합 (예: 120% 증가 = 1.2f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
	FMEODamagePool IncreasedDamagePercent;

	//	곱연산 증폭(%) 누적 배율 (보조 젬 및 키스톤 노드, 기본값 1.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
	FMEODamagePool MoreDamageMultiplier;

	//	원소 저항 관통력 (예: 0.15f = 15% 관통)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offense")
	FMEODamagePool ElementalPentration;

	//	--- 방어자 스텟 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float Armor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float EvasionRating = 0.0f;

	//	속성별 저항력 (-1.0 ~ 0.75 , 최대 저항 상환 75%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	FMEODamagePool Resitances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float CurrentEnergyShield = 0.0f;

	FMEOCombatStatSnapshot()
	{
		//	곱연산 초기 배율 1.0 으로 세팅
		MoreDamageMultiplier.Physical = 1.0f;
		MoreDamageMultiplier.Fire = 1.0f;
		MoreDamageMultiplier.Cold = 1.0f;
		MoreDamageMultiplier.Lightning = 1.0f;
		MoreDamageMultiplier.Chaos = 1.0f;
	}
};

//	6단계 데미지 연산 파이프라인 입출력 패킷
USTRUCT(BlueprintType)
struct FMEODamagePacket
{
	GENERATED_BODY()

	//	공격 주체 및 피격 대상 참조
	UPROPERTY(BlueprintReadWrite, Category = "Context")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Context")
	TObjectPtr<AActor> TargerActor = nullptr;

	//	스킬 및 피해 성격 태그 (예: Attack, Spell, Projectile,  Area, DoT)
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Context")
	FGameplayTagContainer DamageTags;

	//	주문/지속 피해 등 회피 불가능 판정 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	bool bCanBeEvaded = true;

	//	[Stage 1 입력] 무기/스킬 기본 피해 + 플랫(Flat) 추가 피해
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage1")
	FMEODamagePool BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage1")
	FMEODamagePool AddedFlatDamage;

	//	[Stage 2 입력] 속성 전환 규칙 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage2")
	TArray<FMEODamageConversionRule> ConverasionRules;

	//	[Stage 3~5 입력] 공격자,방어자 전투 스텟 스냅샷
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FMEOCombatStatSnapshot StatSnapShot;

	//	--- 파이프라인 연산 최종 출력 결과 ---
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bIsEvaded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bIsCriticalHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	FMEODamagePool MitigatedDamage;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float FinalDamageToEnergyShield = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float FinalDamageToLife = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float TotalFinalDamage = 0.0f;
};

//	스킬 1회 시전 시 소모 자원 명세 구조체
USTRUCT(BlueprintType)
struct FMEOResourceCost
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	EMEOResource ResourceType = EMEOResource::Mana;

	//	스킬 젬 기본 소모량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float BaseCost = 0.0f;

	//	연결된 보조 젬들의 소모 배율 누적곱 (예 : 1.3 * 1.2 = 1.56)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float SupportCostMultiplier = 1.0f;

	//	패시브/장비 자원 소모 감소율 (0.0~1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float CostReductionPercent = 0.0f;
};

//	오라/버프/소환수의 정신력(Spirit) 영구 점유 항목
USTRUCT(BlueprintType)
struct FMEOSpiritReservationEntry
{
	GENERATED_BODY()

	//	점유 스킬 또는 버프의 고유 식별 키
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	FName ReservationId = NAME_None;

	//	점유 중인 정신력 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	float ReservedAmount = 0.0f;
};
