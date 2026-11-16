// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/SubSystem/MEOPoolingSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UMEOPoolingSubsystem::PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count, int MaxSize)
{
	if (!ActorClass || !GetWorld()) return;

	//	해당 클래스의 풀 컨테이너가 없다면 생성하고, 최대 본관 갯수를 설정
	FActorPoolContainer& PoolContainer = PoolMap.FindOrAdd(ActorClass);
	PoolContainer.MaxPoolSize = MaxSize;

	FActorSpawnParameters SpawnParams;
	// 풀링용 액터는 스폰 중 충돌이 발새해도 무조건 스폰하돌고 강제
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	for (int32 i = 0; i < Count; ++i)
	{
		AActor* NewActor = GetWorld()->SpawnActor<AActor>(ActorClass, FVector::ZeroVector,FRotator::ZeroRotator,SpawnParams);
		if (NewActor)
		{
			//	스폰 직허 액터의 연산(Tick),랜더링(Hidden),물리 콜리존(Collision)를 모두 끄고 대기열에 넣는다.
			NewActor->SetActorHiddenInGame(true);
			NewActor->SetActorEnableCollision(false);
			NewActor->SetActorTickEnabled(false);
		}

		PoolContainer.InactiveActors.Add(NewActor);
	}
}

AActor* UMEOPoolingSubsystem::AcquireFromPool(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation)
{
	if (!ActorClass || !GetWorld()) return nullptr;

	FActorPoolContainer* PoolContainer = PoolMap.Find(ActorClass);

	AActor* SpawnedActor = nullptr;

	//	1. 풀에 사용 가능한 대기 액터가 있는지 확인
	if (PoolContainer && PoolContainer->InactiveActors.Num() > 0)
	{
		//	배열의 맨 마지막 요소를 꺼냅니다.(TArray의 Pop은 0(1) 성능을 가짐)
		SpawnedActor = PoolContainer->InactiveActors.Pop();

		//	위치와 회전값 갱신
		SpawnedActor->SetActorLocationAndRotation(Location, Rotation);
	}
	else
	{
		//	2. 풀이 비어있거나 아직 생성되지않았다면, 어쩔 수 없이 새로 스폰
		//	(Prewarm이 충분했다면 게임 플레이 중 이 구문이 후출되는 일이 없어야함)
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParams);
	}

	if (SpawnedActor)
	{
		//	랜더링,물리,연산을 다시 활성화 하여 게임 월드에 다시 복귀
		SpawnedActor->SetActorHiddenInGame(false);
		SpawnedActor->SetActorEnableCollision(true);
		SpawnedActor->SetActorTickEnabled(true);

		//	TODO: 만약 액터가 특정 인터페이스(예시 : IMEOPoolable)를 상속받는다면
		//	여기서 OnAcquried()같은 초기화 함수를 호출하여 생명 주기(LifeSpan)나 스텟을 리셋 가능.
	}
	return SpawnedActor;
}

void UMEOPoolingSubsystem::ReturnToPool(AActor* ActorToReturn)
{
	if (!ActorToReturn) return;

	UClass* ActorClass = ActorToReturn->GetClass();
	FActorPoolContainer* PoolContainer = PoolMap.Find(ActorClass);

	//	풀 컨테이너가 존재하고, 현재 보관중인 액터 수가 MaxSize를 넘지 않았을때만 회수.
	if (PoolContainer && PoolContainer->InactiveActors.Num() < PoolContainer->MaxPoolSize)
	{
		//	화면에서 숨기고 틱과 충돌을 꺼서 CPU/메모리 부하를 제거
		ActorToReturn->SetActorHiddenInGame(true);
		ActorToReturn->SetActorEnableCollision(false);
		ActorToReturn->SetActorTickEnabled(false);

		//	이동중 이던 물리(Velocity) 잔재가 남지 않도록 위치를 원점으로 지워버린다.
		ActorToReturn->SetActorLocation(FVector::ZeroVector);

		PoolContainer->InactiveActors.Add(ActorToReturn);
	}
	else
	{
		//	풀 용량이 꽉 찼다면, 메모리 누수(Memory Leak) 방지를 위해 가차없이 파괴(Destroy)한다
		ActorToReturn->Destroy();
	}
}
