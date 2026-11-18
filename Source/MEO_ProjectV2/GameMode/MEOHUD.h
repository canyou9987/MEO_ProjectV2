// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEOHUD.generated.h"

class UUserWidget;
class AMEOPlayerState;

UCLASS()
class MEO_PROJECTV2_API AMEOHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	AMEOHUD();

	virtual void BeginPlay() override;

	//	캐릭터 스폰/리스폰 시 신규 컴포넌트 및 PlayerState 델리게이트를 HUD 위젯에 재연결
	UFUNCTION(BlueprintCallable, Category = "HUD System")
	void BindHUDToCharacter(APawn* TargetCharacter, AMEOPlayerState* TargetPlayerState);

	//	단축키 입력에 따른 UI 창 Open/Close 토글 (PlayerController 호출)
	UFUNCTION(BlueprintCallable, Category = "HUD System")
	void ToggleUIWindow(EMEOUIWindowType WindowType);

	//	특정 UI창 표시 여부 직접 제어
	UFUNCTION(BlueprintCallable, Category = "HUD System")
	void SetUIWindowVisibility(EMEOUIWindowType WindowType, bool bVisible);

	//	마우스 커서 호버 중인 몬스터 HP 바 및 Affix 툴팁 정보 갱신
	UFUNCTION(BlueprintCallable, Category = "HUD System")
	void SetTargetMonsterInfo(AActor* TargetMonster);

protected:
	//	최상위 HUD 루트 위젯 클래스 (블루프린트에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Config")
	TSubclassOf<UUserWidget> MainHUDWidgetClass;

	//	뷰포트에 부착된 메인 HUD 인스턴스 포인터 (사망 시에도 파괴되지 않고 유지)
	UPROPERTY(BlueprintReadOnly, Category = "HUD System")
	TObjectPtr<UUserWidget> MainHUDWidgetInstance = nullptr;

private:
	void InitializeMainHUDWidget();

	//	각 모달 UI 창별 현재 열림 상태 캐싱
	TMap<EMEOUIWindowType, bool> WindowVisibilityMap;

};
