// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Math/RandomStream.h"
#include "MEODiceSubsystem.generated.h"

UCLASS()
class MEO_PROJECTV2_API UMEODiceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    //  디버깅 및 리플레이 검증을 위한 난수 스트림 시드(Seed) 수동 고정
    UFUNCTION(BlueprintCallable, Category = "MEO|Dice")
    void SetRandomSeed(int32 NewSeed);

    //  TRPG 표준 NdX + C 주사위 굴림 (예: 3d6 + 5 -> NumDice = 3, DiceSides = 6, Modifier 5.0f)
    UFUNCTION(BlueprintCallable, Category = "MEO|Dice")
    float RollDice(int32 NumDice, int32 DiceSides, float Modifier = 0.0f);

    //  순수 확률 (0.0~1.0) 단발성 성공 여부 판정
    UFUNCTION(BlueprintCallable, Category = "MEO|Dice")
    bool RollChance(float Probablity);

    //  앤트로피(PRNG) 기반 불운 방지 판정 (회피,치명타 상태이상 등의 연속 0% / 100% 편향 제거)
    UFUNCTION(BlueprintCallable, Category = "MEO|Dice")
    bool EvaluateEntropyRoll(AActor* TargetActor, FName EntropyPoolId, float Probability);

    //  가중치(Weight) 배열을 입력받아 확률에 비례한 인덱스 반환 (루트 테이블 및 접사 추첨용)
    UFUNCTION(BlueprintCallable, Category = "MEO|Dice")
    int32 RollWeightedIndex(const TArray<float>& Weights);

    //  TRPG식 '행운(Advantage)' / '불운(Disadvantage)' 주사위 규칙 추가
    UFUNCTION(BlueprintCallable, Category = "MEO|Dice")
    float RollDiceWithAdvantage(int32 NumDice, int32 DiceSides, float Modifier = 0.0f, bool bIsAdvantage = true);

private:
    //  언리얼 표준 난수 스트림 엔진
    FRandomStream RandomStream;

    //  액터 및 판정 채널별(예: "Evasion"."Crit") 누적 엔트로피 게이지 (0.0 ~ 100.0)
    TMap<TWeakObjectPtr<AActor>, TMap<FName, float>> EntropyAccumulators;

};

//  음...TRPG 베이스일경우 일종의 "행운","불운"으로 주사위 굴리는규칙이 있는데
//  여기는 딱히 안보이는것 같네? 여기서 하는게 아닌가?
//  행운은 주사위 두개를 굴려서 높은값이 나온 주사위를 쓰는거고
//  불운은 주사위 두개를 굴려서 낮은값이 나온 주사위를 쓰는건데
