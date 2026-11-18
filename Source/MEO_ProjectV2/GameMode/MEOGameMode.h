// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MEOGameMode.generated.h"


UCLASS()
class MEO_PROJECTV2_API AMEOGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMEOGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	//	플레이어 캐릭터 사망 시 호출 (경험치 패널티 연산 및 카메라 시점 고정 처리)
	UFUNCTION(BlueprintCallable, Category = "Rule Engine")
	void HandlePlayerDeath(AController* VictimController, AActor* Killer);

	//	신규 플레이어 캐릭터 액터 스폰 및 PlayerState SSOT 데이터 주입
	UFUNCTION(BlueprintCallable, Category = "Rule Engine")
	void RespawnPlayer(APlayerController* TargetController);

	//	맵 포탈 사용 요청 및 잔여 횟수 차감 검증
	UFUNCTION(BlueprintCallable, Category = "Portal System")
	bool RequestPortalUse(APlayerController* Player);

protected:
	//	맵 인스턴스당 허용되는 최대 포탈 진입 횟수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Rules")
	int32 MaxPortal = 6;

	//	현재 남아있는 포탈 사용 가능 횟수
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Map Rules")
	int32 RemainingPortalUses = 6;

	//	실제 스폰할 플레이어 전투 캐릭터 클래스 (AMEOPlayerCharacter 구현 후 블루프린트 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<APawn> CombatCharacterClass;
};
