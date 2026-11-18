// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/GameMode/MEOGameState.h"
#include "Net/UnrealNetwork.h"

AMEOGameState::AMEOGameState()
{
}

void AMEOGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//	모든 접속 클라이언트에 던전 난이도 및 보상 계수 복제
	DOREPLIFETIME(AMEOGameState, ActiveZoneModifiers);
	DOREPLIFETIME(AMEOGameState, CurrentAreaLevel);
	DOREPLIFETIME(AMEOGameState, ZoneItemQuantityBonus);
	DOREPLIFETIME(AMEOGameState, ZoneItemRarityBonus);

	//	해당 같은 인스턴스에 들어간 클라이언트에게 같은 던전 난이도 및 보상 계수를 복제하기 위해
	//	GameStateBase에서 관리한다.
	//	그렇다면 AGameStateBase가 받을 그 계수 변수들은 누구한테 받는거지? POE2 엔드게임 기준으로는 그 계수를 주는건 경로석이다.
	//	그럼 경로석을 박지않고 해당 인스턴스 자체가 변수를 품고있는 경우가 있을까?
	//	해당 인스턴스에 변수를 적용할려면 인스턴스 생성 또는 입장 전에 그 변수를 가지고있는 곳이 있어야할것 같은데?
	//	GameModeBase? 아니면 레벨? 그리고 주입한는 맵 모디파이어는 무엇이고,주입을 받는곳은 어디지? 여기가 GameModeBase인가?
	//	여기는 클라이언트에 던전 난이도 및 보상 계수 복제만 하는곳인가? 주입을 하고 주입을 받는곳이 아니라?
}

void AMEOGameState::OnRep_ZoneModifiers()
{
	//	클라이언트에서 맵 모디파이어 배열 복제 수신 시 UI 갱신 브로드캐스트
	OnZoneModifiersUpdated.Broadcast();
}
