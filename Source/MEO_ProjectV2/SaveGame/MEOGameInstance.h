// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEOGameInstance.generated.h"

class AMEOPlayerState;

UCLASS()
class MEO_PROJECTV2_API UMEOGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	//	앱 실행시 초기화 : 기본 세이브 슬룻이 존재할 경우 메모리에 사전 로드
	virtual void Init() override;

	//	앱 종료시 정리 : 메모리에 캐싱된 최신 진행 데이터를 디스크에 자동 백업
	virtual void Shutdown() override;

	//	PlayertState의 런타임 영속 데이터(SSOT)를 추출해 지정된 슬룻 파일 (.sav)애 저장
	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	bool SaveCharacterDataToSlot(const FString& SlotName, AMEOPlayerState* SourcePlayerState);

	//	지정된 슬룻 파일(.sav)을 역직렬화하여 OutSave 및 CachedSaveData에 로드
	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	bool LoadCharacterDataFromSlot(const FString& SlotName, FMEOSaveGameData& OutSaveData);

	//	현재 세션에서 메모리에 캐싱 중인 활성 세이브 데이터 (맵 전환 간 데이터 보존용)
	UPROPERTY(BlueprintReadOnly, Category = "SaveSystem")
	FMEOSaveGameData CachedSaveData;

	//	슬룻 이름이 비어있을 때 사용할 기본 로컬 세이브 슬룻 식별자
	UPROPERTY(BlueprintReadOnly, Category = "SaveSystem")
	FString DefaultSaveSlotName = TEXT("MEO_SaveSlot_01");

};
