// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEODiceSubsystem.h"

void UMEODiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //  월드 인스턴스 생성 시 고유 랜덤 시드 초기화
    RandomStream.GenerateNewSeed();

}
void UMEODiceSubsystem::SetRandomSeed(int32 NewSeed)
{
    RandomStream.Initialize(NewSeed);
}

float UMEODiceSubsystem::RollDice(int32 NumDice, int32 DiceSides, float Modifier)
{
    if (NumDice <= 0 || DiceSides <= 0)
    {
        return Modifier;
    }

    float Total = Modifier;
    for (int32 i = 0; i < NumDice; ++i)
    {
        //  1 ~ DiceSides 범위의 균등 정수 난수 합산
        Total += static_cast<float>(RandomStream.RandRange(1, DiceSides));
    }

    return Total;
}

bool UMEODiceSubsystem::RollChance(float Probablity)
{
    if (Probablity <= 0.0f) return false;
    if (Probablity >= 1.0f) return true;

    return RandomStream.FRand() <= Probablity;

    //  Probablity가 0.0f ~ 1.0f 의 숫자가 될거고 RandomStream.FRand()도 0.0f ~ 1.0f중 난수하나를 뽑아서 비교하는거니깐
    //  RandomStream.FRand()보다 크기만 하면 true가 될수있는건가?
    //  그래서 Probablity가 0.75이면 75퍼 확률인거고 난수도보다 크면 75퍼 확률이 성공했다고 볼수있는거네?
    //  그래서 0.0f면 그냥 실패인거고 1.0f면 그냥 성공인것인거고??
    //  그 TRPG특유의 그 명중굴림과 AC를 비교하는 맛이 안살긴하는데 이것도 나쁘진않네?
}

bool UMEODiceSubsystem::EvaluateEntropyRoll(AActor* TargetActor, FName EntropyPoolId, float Probability)
{
    if (!IsValid(TargetActor) || EntropyPoolId.IsNone())
    {
        return RollChance(Probability);
    }

    const float ClampedProb = FMath::Clamp(Probability, 0.0f, 1.0f);
    if (ClampedProb <= 0.0f) return false;
    if (ClampedProb >= 1.0f) return true;

    TMap<FName, float>& ActorPools = EntropyAccumulators.FindOrAdd(TargetActor);

    //  해당 채널의 첫 판정일 경우 0~99.9 사이의 무작위 초기 앤트로피 값을 부여해 첫 타격 예측 불가능성 확보
    float* CurrentEntorpy = ActorPools.Find(EntropyPoolId);
    if (!CurrentEntorpy)
    {
        const float InitialSeed = RandomStream.FRandRange(0.0f, 99.9f); //  타격이 처음 생길대마다 Seed를 다시 조정하는건가? 근데 왜 99.9f지? 앤트로피 누적으로 100퍼가 만들어져야지 그런건가?
        CurrentEntorpy = &ActorPools.Add(EntropyPoolId, InitialSeed);
    }
    //  매 시도마다 (확률 * 100)만큼 앤트로피 누적
    *CurrentEntorpy += (ClampedProb * 100.0f);

    //  누적치가 100 이상 도달시 판정 성공 및 100 차감 (잔여 오차는 다음 판정으로 이월)
    if (*CurrentEntorpy >= 100.0f)
    {
        *CurrentEntorpy -= 100.0f;
        return true;
    }

    return false;

    //  2026.11.22 06:24
    //  앤트로피의 개념이 뭐지? 누적되는 수치같은건가?
    //  앤트로피 누적은 *CurrentEntorpy += (ClampedProb * 100.0f); 여기서 하는건데, 왜하는거지?
    //  뭐 25퍼를 계속 실패,실패,실패,실패 하는걸 방지하기 위해서인가?
    //  확률이니깐 실패할수도 있고 성공할수있는건데 이걸 앤트로피로 보정할 필요가 있는건가?
    //  에초에 엔트로피 풀 EntropyPoolId이 있어야지 앤트로피를 쌓는건데??
    //  그니깐 ClampedProb = 0.25 인데 첫판정으로 첫시도에서 ID와 초기 엔트로피 값 34.5f 를 받는다고 쳤을때,
    //  CurrentEntorpy가 25->50->75->100이되면 성공판정을 내보내고,앤트로피가 차질않으면 실패를 반환한다는거 아니야?
    //  그럼 EntropyPoolId가 있는 순간부터 앞에 3번은 무조건 실패한다는 말 아니야?
    //  그리고 기껏 랜덤을 돌려서 InitialSeed를 만들어 줬는데 어디다 쓰는거지?
    //  아? CurrentEntorpy는 &ActorPools.Add(EntropyPoolId, InitialSeed)의 주소값이지?
    //  그러면 *CurrentEntorpy에 값을 더한다는거는 ActorPools의 값을 더한다는 말이구나?
    //  그래서 InitialSeed가 랜덤값을 넣어줘서 시도할때마다 2번 또는 4번만에 성공할수있는 확률이 탄생하는구나?
    //  그러면 *CurrentEntorpy >= 100.0f 성공을 했으면 잔여 오차는 다음 판정으로 이월한다는데,
    //  이게 과연 맞을까? 100을한번 채웠으면 다시 InitialSeed를 받아와야하는거 아닌가?
    //  다시 받아 오려면 ActorPools.Find(EntropyPoolId)가 false가 나오야하는거고,
    //  false가 나올려면 EntropyAccumulators.FindOrAdd(TargetActor)를 했을때 TMap<FName, float>값을 비워줘야 하는거 아닌가?
    //  정리가 어렵긴한데, 내가 보기에는 앤트로피를 초기화하는 곳이 없는것처럼 느껴져.
}

int32 UMEODiceSubsystem::RollWeightedIndex(const TArray<float>& Weights)
{
    if (Weights.Num() == 0)
    {
        return INDEX_NONE;
    }

    float TotalWeight = 0.0f;
    for (float Weight : Weights)
    {
        if (Weight > 0.0f)
        {
            TotalWeight += Weight;
        }
    }

    if (TotalWeight <= 0.0f)
    {
        return INDEX_NONE;
    }

    //  0~TotalWeight 사이 난수를 뽑아 누적 구간 탐색
    const float RollValue = RandomStream.FRandRange(0.0f, TotalWeight);
    float Accumulated = 0.0f;

    for (int32 i = 0; i < Weights.Num(); ++i)
    {
        if (Weights[i] <= 0.0f) continue;

        Accumulated += Weights[i];
        if (RollValue <= Accumulated)
        {
            return i;
        }
    }

    return Weights.Num()-1;
}

float UMEODiceSubsystem::RollDiceWithAdvantage(int32 NumDice, int32 DiceSides, float Modifier, bool bIsAdvantage)
{
    const float Roll1 = RollDice(NumDice, DiceSides, Modifier);
    const float Roll2 = RollDice(NumDice, DiceSides, Modifier);
    return bIsAdvantage ? FMath::Max(Roll1, Roll2) : FMath::Min(Roll1, Roll2);
}
