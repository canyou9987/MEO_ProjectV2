// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEOSaveGame.generated.h"

UCLASS()
class MEO_PROJECTV2_API UMEOSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	//	로컬 디스크(.sav)에 직렬화 되어 기록되는 실제 플레이어 영속 데이터
	UPROPERTY(VisibleAnywhere, Category = "SaveSystem")
	FMEOSaveGameData SaveData;

};
