// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MEO_ProjectV2/DataStruct/MEODataTypes.h"
#include "MEOPlayerController.generated.h"

class AMEOCameraPawn;
class AMEOHUD;


UCLASS()
class MEO_PROJECTV2_API AMEOPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AMEOPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	// -------------------------------------------------------------------------
	// [1. 클라이언트 액션 요청 및 입력 버퍼링 API]
	// -------------------------------------------------------------------------
	
	//	스킬 시전 요청 (RPC 스로틀링 검증 및 불가시 인풋 버퍼 저장)
	UFUNCTION(BlueprintCallable, Category = "MEO|Input")
	void RequestSkillCast(int32 SkillSlotIndex, const FVector& TargetLocation , AActor* TargetActor );

	//	구르기(회피) 요청 (커서 방향 또는 이동 방향으로 회포)
	UFUNCTION(BlueprintCallable, Category = "MEO|Input")
	void RequestDodgeRoll();

	//	바닥 아이템 루팅, 포탈 진입, 상자 열기 등 상호작용 요청
	UFUNCTION(BlueprintCallable, Category = "MEO|Input")
	void RequestInteract();

	//	주무기(1번 세트) <-> 보조무기 (2번 세트) Dual Spec 스왑 요청
	UFUNCTION(BlueprintCallable, Category = "MEO|Input")
	void RequestWeaponSwap();

	//	애니메이션 선/후딜레이 중 입력된 스킬 커맨드를 일정 시간(0.2초) 동안 버퍼를 보관
	void QueueInputBuffer(int SkillSlotIndex, const FVector& TargetLoc, AActor* TargetActor);

	// -------------------------------------------------------------------------
	// [1. 클라이언트 액션 요청 및 입력 버퍼링 API]
	// -------------------------------------------------------------------------

	//	스폰 또는 리스폰된 플레이어 캐릭터를 독립 카메라 폰에 추종 대상으로 등록
	UFUNCTION(BlueprintCallable, Category = "MEO|Camera")
	void BindCameraToCharacter(AActor* CharacterActor);

	//	현재 컨트롤러에 연결된 MEOHUD 액터를 캐스팅하여 반환합니다.
	UFUNCTION(BlueprintPure, Category = "MEO|UI")
	AMEOHUD* GetMEOHUD() const;

protected:
	// -------------------------------------------------------------------------
	// [3. 서버 권한 검증 RPC (Client -> Server)]
	// -------------------------------------------------------------------------

	//	[RPC] 서버에 스킬 시전을 요청 (WithValidation을 통해 핵/치트 검증 수행(?))
	//	FVector_NetQuantize100을 사용하여 소수점 2까지만 네트워크로 전송해 좌표 패킷 대역폭을 절약 (?)
	UFUNCTION(Server,Reliable,WithValidation)
	void Server_RequestSkillCast(int SkillSlotIndex, FVector_NetQuantize100 TargetLocation, AActor* TargetActor);
	
	//	구르기 실행 서버 요청 (i-Frame 무적 판정 및 스태미나 차감은 서버 권한으로 처리)
	UFUNCTION(Server,Reliable,WithValidation)
	void Server_RequestDodgeRoll(FVector_NetQuantize RollDirection);

	//	월드 오브젝트 상호작용 서버 요청 (루팅 거리 및 아이템 소유권 검증)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestInteract(AActor* TargetInteractable);

	//	무기 세트 스왑 서버 요청 (패시브 트리 Dual Spec 재계산 트리거)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestWeaponSwap();

	//	실제 플레이어가 조종하는 전투 캐릭터 액터 참조 (CameraPawn과 분리 관리)
	UPROPERTY(BlueprintReadOnly, Category = "MEO|Character")
	TObjectPtr<APawn> ControlledCombatCharacter = nullptr;

	//	컨트롤러가 소유(Possess)하여 시점 제어하는 쿼터뷰 전용 카메라 폰
	UPROPERTY(BlueprintReadOnly, Category = "MEO|Camera")
	TObjectPtr<AMEOCameraPawn> ControlledCameraPawn = nullptr;

private:
	// -------------------------------------------------------------------------
	// [4. 내부 입력 핸들러 및 유틸리티 함수]
	// -------------------------------------------------------------------------

	// 매 프레임 인풋 버퍼 타이머를 갱신하고, 캔슬 가능 구간(Recovery) 진입 시 즉시 스킬을 발동
	void ProcessInputBuffer(float DeltaTime);

	//	마우스 휠 입력을 받아 카메라 줌(Zoom In/Out )을 조절
	void OnInputZoom(float AxisValue);

	//	마우스 커서 아래에 위치한 몬스터 또는 상호작용 액터를 레이 트레이스(Picking)함
	AActor* GetActorUnderCursor() const;

	//	마우스 커서가 가리키는 지면(World)의 3D 좌표를 반환합니다.
	bool GetMouseCursorWorldLocation(FVector& OutWorldLocation) const;

	//	UI 단축키 바인딩용 래퍼 함수들
	void RequestToggleUI(EMEOUIWindowType WindowType);
	void Input_ToggleInventory() { RequestToggleUI(EMEOUIWindowType::Inventory); }
	void Input_PassiveTree() { RequestToggleUI(EMEOUIWindowType::PassiveTree); }
	void Input_SkillMenu() { RequestToggleUI(EMEOUIWindowType::SkillMenu); }
	void Input_CharacterUI() { RequestToggleUI(EMEOUIWindowType::CharacterStats); }

	//	--- 입력 버퍼링 (Input Buffering) 관련 변수 ---
	int32 BufferedSkillSlot = -1;		// 예약된 스킬 슬롯 인덱스 (-1이면 비어있음)
	FVector BufferedTargetLocation;		//	예약된 사전 목표 좌표
	UPROPERTY()
	TObjectPtr<AActor> BufferedTargetActor = nullptr;	//	예약된 타겟 액터
	float BufferTimer = 0.0f;			//	남은 버퍼 유효시간
	const float MAX_BUFFER_TIME = 0.2;	//	최대 0.2초 전의 선입력 허용

	//	--- 네트워크 최적화 (RPC Throttling) 관련 변수 ----
	//	RPC 과도 호출(광클) 차단을 위한 타임스탬프 , 마지막으로 스킬 RPC를 서버에 전송한 월드 시간
	float LastSkillRPCExecutionTime = 0.0f;
	//	서버 보호를 위한 최소 RPC 호출 간격 (20Hz 제한 = 0.05초)
	const float MIN_RPC_INTERVAL = 0.05;

public:
	// -------------------------------------------------------------------------
	//	외부에서 엑세스 하기위한 Getter 함수
	// -------------------------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "MEO|Camera")
	AMEOCameraPawn* GetControlledCameraPawn() { return ControlledCameraPawn;  }
};
