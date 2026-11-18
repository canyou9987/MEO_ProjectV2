// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEOGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMEOZoneModifiersUpdated);

UCLASS()
class MEO_PROJECTV2_API AMEOGameState : public AGameStateBase
{
	GENERATED_BODY()
	

public:
	AMEOGameState();
	
	//	네트워크 리플리케이션 변수 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//	현재 맵 인스턴스에 주입된 전역 패시브/액티브 모디파이어 목록
	UPROPERTY(ReplicatedUsing = OnRep_ZoneModifiers, VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FMEOZoneModifierData> ActiveZoneModifiers;

	//	현재 지역의 몬스터 및 드랍 아이템 레벨 기준치
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 CurrentAreaLevel = 1;

	//	지역 전역 아이템 수량 보너스 (Increased Item Quantity)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	float ZoneItemQuantityBonus = 0.0f;

	//	지역 전역 아이템 희귀도 보너스 (Increase Item Rarirty)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	float ZoneItemRarityBonus = 0.0f;

	//	맵 모디파이어 갱신 시 UI(Tab 오버레이 등)에서 구독할 이벤트 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FOnMEOZoneModifiersUpdated OnZoneModifiersUpdated;

protected:
	UFUNCTION()
	void OnRep_ZoneModifiers();

};
