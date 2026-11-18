// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/GameMode/MEOHUD.h"
#include "MEO_ProjectV2/GameMode/MEOPlayerState.h"
#include "Blueprint/UserWidget.h"

AMEOHUD::AMEOHUD()
{
}

void AMEOHUD::BeginPlay()
{
	Super::BeginPlay();

	//	레벨 진입시 최상위 메인 HUD 위젯 뷰포트에 단 1회 생성 및 고정 부착
	InitializeMainHUDWidget();

	//	메인 HUD는 파괴되지 않는다.
}

void AMEOHUD::BindHUDToCharacter(APawn* TargetCharacter, AMEOPlayerState* TargetPlayerState)
{
	//	HUD 미생성시 복구
	if (!MainHUDWidgetInstance)
	{
		InitializeMainHUDWidget();
	}

	// TODO: [UMG 메인 HUD 위젯(UMEOMainHUDWidget) 및 Layer 3 컴포넌트 구현 후 연동]
	// 신규 스폰된 캐릭터의 체력/마나/ES 변경 델리게이트와 인벤토리·스킬 델리게이트를 위젯에 재연결
	(void)TargetCharacter;
	(void)TargetPlayerState;

	//	델리게이트로 받은 정보를 HUD에 출력하는 UI에 어떤 정보를 보낼지 관리하는곳인가?
}

void AMEOHUD::ToggleUIWindow(EMEOUIWindowType WindowType)
{
	const bool bCurrentVisble = WindowVisibilityMap.FindRef(WindowType);
	SetUIWindowVisibility(WindowType, !bCurrentVisble);

	//	어떤 토글형 UI가 켜질지 EMEOUIWindowType유형으로 받아서 전달하는곳?
	//	클래스 외부에서도 작동시킬려고 약간 버튼같은 느낌으로 만든것 같다.
	//	안그랬으면 그냥 MEOHUD->WindowVisibilityMap.Add(WindowType, bVisible);
	//	를 해도 동작은 하니깐 말이다.(WindowVisibilityMap가 private이므로 불가능)
}

void AMEOHUD::SetUIWindowVisibility(EMEOUIWindowType WindowType, bool bVisible)
{
	WindowVisibilityMap.Add(WindowType, bVisible);

	// TODO: [UMG 메인 HUD 위젯 구현 후 해당 패널 위젯의 SetVisibility(ESlateVisibility) 호출]
	// 토글형 UI(인벤토리 같은거)가 켜지도록 값을 지정하는곳?
}

void AMEOHUD::SetTargetMonsterInfo(AActor* TargetMonster)
{
	// TODO: [AMEOMonsterBase 구현 후 상단 몬스터 체력바 및 Affix 태그 UI 갱신]
	(void)TargetMonster;

	//	Affix 태그 UI 갱신은 마법 몬스터 이상만 적용 대상이다.
	//	일반 몬스터(하수인 제외)는 Affix 태그를 가지지 않는다.(아마도)
}

void AMEOHUD::InitializeMainHUDWidget()
{
	if (!MainHUDWidgetInstance && MainHUDWidgetClass)
	{
		MainHUDWidgetInstance = CreateWidget<UUserWidget>(GetOwningPlayerController(), MainHUDWidgetClass);
		if (MainHUDWidgetInstance)
		{
			MainHUDWidgetInstance->AddToViewport(0);
		}
	}
}
