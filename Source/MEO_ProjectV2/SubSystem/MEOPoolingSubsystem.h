// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MEOPoolingSubsystem.generated.h"

//	클래스 별로 비활성화된 액터들을 모아두는 풀링 컨테이너 구조체
USTRUCT()
struct FActorPoolContainer
{
	GENERATED_BODY()

	//	현재 비활성화되어 풀에서 대기 중인 액터 배열
	UPROPERTY()
	TArray<AActor*> InactiveActors;

	//	해당 클래스 풀의 최대 보관 허용 갯수,	초과하여 반환될 경우 Destroy 해서 메모리 오버(Memory Leak)을 방지
	int MaxPoolSize = 200;
};

UCLASS()
class MEO_PROJECTV2_API UMEOPoolingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	//	로딩 화면이나 게임 시작시, 지정된 갯수 만큼 액터를 미리 메모리에 할당(Pre-warm)
	UFUNCTION(BlueprintCallable, Category = "MEO|Pooling")
	void PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count, int MaxSize = 200);

	//	풀에서 액터를 대여합니다. (풀이 비어있다면 내부적으로 신규 스폰하여 반환)
	UFUNCTION(BlueprintCallable, Category = "MEO|Pooling", meta = (WorldContext = "WorldContextObject"))
	AActor* AcquireFromPool(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation);

	//	사용이 끝난 액터를 파괴(Destory)하지 않고 풀로 반환하여 틱/콜리전을 비활성화
	UFUNCTION(BlueprintCallable, Category = "MEO|Pooling")
	void ReturnToPool(AActor* ActorToReturn);

private:
	//	액터 클래스 타입 키(key)로 하여 풀 컨테이너를 관리하는 해시맵
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPoolContainer> PoolMap;
};
