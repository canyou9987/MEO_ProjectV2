// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/GameMode/MEOPlayerController.h"
#include "MEO_ProjectV2/GameMode/MEOCameraPawn.h"
#include "MEO_ProjectV2/GameMode/MEOHUD.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"

AMEOPlayerController::AMEOPlayerController()
{
	//	쿼터뷰 핵앤슬레시 조작 환경을 위해 마우스 커서는 항상 화면에 표시
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	//	입력 버퍼 타이머 및 커서 호버 감지를위해 tick은 활성화
	PrimaryActorTick.bCanEverTick = true;
}

void AMEOPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//	쿼터뷰 게임플레이에 맞게 게임 입력과 UI 클릭을 동시에 허용하는 입력모드로 설정.
	if (IsLocalController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);	//	마우스 클릭 드래그 시에도 커서가 사라지지 않도록 고정
		SetInputMode(InputMode);
	}
}

void AMEOPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	//	1.	선입력(Input Buffer) 타이머 갱신 및 실행 조건 검증
	ProcessInputBuffer(DeltaTime);

	//	2.	로컬 플레이어 환경에서 마우스 커서 아래의 몬스터/아이템 호버(Hover) 정보 갱신
	if (IsLocalController())
	{
		AActor* HoverActored = GetActorUnderCursor();

		// TODO: [몬스터 타깃 프레임 및 바닥 아이템 하이라이트 연동 후 구현]
		// 1. HoveredActor가 AMEOMonsterBase라면 GetMEOHUD()->SetTargetMonsterInfo()를 호출해 화면 상단 HP바를 갱신합니다.
		// 2. HoveredActor가 IMEOInteractableInterface를 구현했다면 외곽선(Custom Depth Stencil) 하이라이트를 활성화합니다.	
		
		(void)HoverActored;
	}
}

void AMEOPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) return;

	//	1. 카메라 줌 인/아웃 축 바인딩
	InputComponent->BindAxis("MouseWheelAxis", this, &AMEOPlayerController::OnInputZoom);

	//	2.액션 및 전투 입력 바인딩
	InputComponent->BindAction("DodgeRoll", IE_Pressed, this, &AMEOPlayerController::RequestDodgeRoll);
	InputComponent->BindAction("Interact", IE_Pressed, this, &AMEOPlayerController::RequestInteract);
	InputComponent->BindAction("WeaponSwap", IE_Pressed, this, &AMEOPlayerController::RequestWeaponSwap);

	//	3. UI 패널 토글 단축키 바인딩 (I, P, G, T, )
	InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AMEOPlayerController::Input_ToggleInventory);
	InputComponent->BindAction("TogglePassiveTree", IE_Pressed, this, &AMEOPlayerController::Input_PassiveTree);
	InputComponent->BindAction("ToggleSkillMenu", IE_Pressed, this, &AMEOPlayerController::Input_SkillMenu);
	InputComponent->BindAction("ToggleCharacterUI", IE_Pressed, this, &AMEOPlayerController::Input_CharacterUI);

	// TODO: [Enhanced Input System 전환 및 스킬 퀵슬롯 바인딩]
	// 프로젝트 확장 시 UE5 Enhanced Input(UEnhancedInputComponent)으로 마이그레이션하여 (IA와 IMC)
	// 마우스 좌/중/우클릭 및 Q, E, R, F ,C, 슬롯 입력을 RequestSkillCast(SlotIndex, ...)와 연결합니다.
	
	// (단 IA_Q,IA_W 하나씩 바인드를 걸기 보다는 TArray 반복문 + BindActionValueLambda 바인딩으로 코드상으로는 한번에 바인딩 건다)
	// (예를 좌클릭의 경우 스킬시전,상호작용,UI클릭 처럼 복합기능이 붙어있어서 우선순위를 선정해야하는 경우 예외로 따로 바인딩 걸수있다.)
	// 참고로 이동은 WASD 이다.마우스 클릭으로 이동하지 않는다.스킬시전시 버튼 누르면 마우스 커서 방향으로 대상으로 바로나간다.
	// 마우스 커서를 창 끝으로 밀어서 카메라가 이동하는 경우는 없을 예정이다.(카메라 고정) 롤,스타 X , POE2 키보드 마우스 조작 O
}

void AMEOPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//	독립 카메라 아키텍쳐에 따라 컨트롤러가 AMEOCameraPawn을 소유(Possess)한 경우 참조를 캐싱한다.
	if (AMEOCameraPawn* CameraPawn = Cast<AMEOCameraPawn>(InPawn))
	{
		ControlledCameraPawn = CameraPawn;
	}
	// TODO: [GameMode 스폰 파이프라인 연동 후 구현]
	// 1. AMEOGameMode에서 스폰한 실제 전투 캐릭터(AMEOPlayerCharacter) 참조를 ControlledCombatCharacter에 등록합니다.
	// 2. BindCameraToCharacter(ControlledCombatCharacter)를 호출해 카메라가 캐릭터를 추종하도록 연결합니다.
	// 3. GetMEOHUD()->BindHUDToCharacter(...)를 호출해 체력/마나 구체 및 인벤토리 델리게이트를 UI에 바인딩합니다.
	// 어떤 정보를 UI에 넘길지는 전투 캐릭터(AMEOPlayerCharacter)에서 구현
}

void AMEOPlayerController::RequestSkillCast(int32 SkillSlotIndex, const FVector& TargetLocation,AActor* TargetActor)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	//	[RPC Rate Limiting / 스로틀링]
	//	단축키 광클이나 매크로로 인한 네트워크 패킷 폭증을 막기 위해 0.05초(20Hz) 이내의 연속 호출을 차단
	if (CurrentTime - LastSkillRPCExecutionTime < MIN_RPC_INTERVAL)
	{
		//	패킷 전송은 막되, 조작감이 씹히지 않도록 입력 버퍼에 커맨드를 임시 저장
		QueueInputBuffer(SkillSlotIndex,TargetLocation, TargetActor);
		return;
	}

	// TODO: [애니메이션 캔슬 윈도우(ActionWindow) 검증 연동 후 구현]
	// 1. ControlledCombatCharacter에서 UMEOAnimControlComponent를 조회합니다.
	// 2. 현재 애니메이션이 WindUp(선딜레이) 또는 Active(타격 중) 상태라 캔슬이 불가능하다면:
	//    서버에 불필요한 실패 RPC를 보내지 않고 즉시 QueueInputBuffer()에 저장한 뒤 return 합니다.
	// 3. 로컬 예측(Client-Side Prediction)을 위해 클라이언트에서 먼저 시전 몽타주를 재생할 수 있습니다.

	LastSkillRPCExecutionTime = CurrentTime;

	//	검증을 통과 했으므로 버퍼를 비우고 서버로 스킬 시전 RPC를 전송
	BufferedSkillSlot = -1;
	BufferTimer = 0.0f;

	Server_RequestSkillCast(SkillSlotIndex, TargetLocation, TargetActor);

	//	-------* 주의 *------- 
	//	RequestSkillCast는 플레이어가 캐릭터나 서버,또는 서브시스템 한테 스킬사용을 요청하는 곳이지,
	//	해당 스킬이 캐릭터 정보에 따라 사용 가능한지 검증하는 곳이 아니고,
	//	스킬 시전이 어떻게 작동하는지 구현하는 구현부가 될수도 있지만 아니며
	//	당연히 스킬 데미지 계산같은걸 하는곳이 아니라
	//	"A플레이어, 스킬[SkillSlotIndex]를 TargetLocation위치에 TargetActor를 대상으로 사용 요청 합니다!"
	//	(근데 TargetActor가 왜있지? 대부분의 스킬은 특정 대상을 노리는게 없는데....?)
}

void AMEOPlayerController::RequestDodgeRoll()
{
	FVector CurSorWorldLoc;
	FVector RollDirection = FVector::ForwardVector; //	(컨트롤러인데 앞백터의 기준이....?)

	//	마우스 커서 위치를 기준으로 캐릭터가 구를 방향 백터를 계산
	if (GetMouseCursorWorldLocation(CurSorWorldLoc) && IsValid(ControlledCombatCharacter))
	{
		RollDirection = (CurSorWorldLoc - ControlledCombatCharacter->GetActorLocation()).GetSafeNormal2D();
	}
	else if (IsValid(ControlledCombatCharacter))
	{
		//	마우스 커서를 가져오지 못했을 경우 캐릭터의 앞백터를 가져옴
		RollDirection = ControlledCombatCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	// TODO: [선딜레이(WindUp) 캔슬 불가 검증 연동]
	// 클라이언트에서 UMEOAnimControlComponent::CanCancelCurrentAction()을 먼저 체크해 
	// WindUp 구간일 경우 불필요한 RPC 호출을 사전에 방지합니다.

	Server_RequestDodgeRoll(RollDirection);

}

void AMEOPlayerController::RequestInteract()
{
	AActor* TargetActor = GetActorUnderCursor();
	if (!IsValid(TargetActor))
	{
		return;
	}

	// TODO: [상호작용 인터페이스(IMEOInteractableInterface) 검증 연동]
	// if (TargetActor->Implements<UMEOInteractableInterface>())
	// {
	//     Server_RequestInteract(TargetActor);
	// }

	Server_RequestInteract(TargetActor);
}

void AMEOPlayerController::RequestWeaponSwap()
{
	//	공격 또는 시전자 동작 중에는 무기 스왑을 제한할 수 있도록 추후 ActionWindow 체크를 추가
	Server_RequestWeaponSwap();

	// 스킬에서 직접 무기세트를 지정할경우,
	// 예를 들어 스킬 A는 I세트와 II세트에서 사용 체크,B 는 II세트에서 사용가능,C 는 I세트에서 사용가능,D는 사용가능 세트 체크 안함
	// 이렇게 세팅을하고 플레이를 할수가 있는데,과연 이걸 사용해서 구현 가능한지 의심이 든다.
	// RequestWeaponSwap()은 그냥 무기 세트 A<->B , B<->A 로 바꾸고 패시브트리 다시 계산하는건지 지켜봐야할듯 
}

void AMEOPlayerController::QueueInputBuffer(int SkillSlotIndex, const FVector& TargetLoc, AActor* TargetActor)
{
	BufferedSkillSlot = SkillSlotIndex;
	BufferedTargetLocation = TargetLoc;
	BufferedTargetActor = TargetActor;
	BufferTimer = MAX_BUFFER_TIME;	//	0.2초 동안 선입력 유지
}

void AMEOPlayerController::ProcessInputBuffer(float DeltaTime)
{
	if (BufferTimer <= 0.0f || BufferedSkillSlot < 0)
	{
		return;
	}

	BufferTimer -= DeltaTime;

	//	버퍼 유효 시간(0.2초)이 만료되면 예약된 입력을 폐기
	if (BufferTimer <= 0.0f)
	{
		BufferedSkillSlot = -1;
		BufferedTargetActor = nullptr;
		return;
	}

	// TODO: [애니메이션 Recovery 구간 진입 감지 후 자동 발동 구현]
	// 1. ControlledCombatCharacter의 UMEOAnimControlComponent::GetCurrentActionWindow()를 확인합니다.
	// 2. 상태가 Idle 또는 Recovery(후딜레이 캔슬 허용 구간)로 전환되는 즉시:
	//    int32 SlotToCast = BufferedSkillSlot;
	//    BufferedSkillSlot = -1; // 무한 재귀 호출 방지를 위해 먼저 초기화
	//    RequestSkillCast(SlotToCast, BufferedTargetLocation, BufferedTargetActor);

}

// =============================================================================
// [Server RPC Implementations & Validations]
// =============================================================================

bool AMEOPlayerController::Server_RequestWeaponSwap_Validate()
{
	return true;
}

void AMEOPlayerController::Server_RequestWeaponSwap_Implementation()
{
	// TODO: [Dual Spec 무기 스왑 연동]
	// ControlledCombatCharacter의 Server_SwapWeaponSet()을 호출하여 1번/2번 무기 세트를 교체하고,
	// UMEOPassiveTreeComponent::OnWeaponSetSwapped()를 트리거해 패시브 스탯을 원자적으로 재계산합니다.

	// 고려 해봐야 할것이 패시브 스텟을 정말로 재계산을 할것인가?
	// 아니면 계산은 패시브 스텟이 바뀔때만 계산하고 저장해놓은 다음에 계산값만 바꿔서 쓸것인가.
	// 여기서 재계산의 범위가 너무 넓은 느낌이 든다.
	// 패시브 스텟이 스킬DPS,원소저항치,회피 와 방어력 등등의 여러가지 스텟을 복합적으로 가지고 있는 건데
	// 그럼 무기세트가 바뀔때마다 스킬DPS,회피,원소 저항치를 각각 재계산 때린다는건가?
	// 아니면 해당 연산식에는 패시브 스텟 변수가 존재하여 그 변수값만 교체하는건가?
	// 그래서 '원자적으로 재계산'이라는 말을 쓴건가? 패시브 스텟값을 받아오는 변수(float PassiveStat)가 존재해서 교체만 하는게 맞는건가?
}

bool AMEOPlayerController::Server_RequestInteract_Validate(AActor* TargetInteractable)
{
	//	대상 액터가 유효하지 않으면 요청을 기각
	return IsValid(TargetInteractable);
}

void AMEOPlayerController::Server_RequestInteract_Implementation(AActor* TargetInteractable)
{
	// TODO: [거리 검증 및 상호작용 실행]
	// 1. 서버 기준으로 ControlledCombatCharacter와 TargetInteractable 간의 거리가 허용 범위(예: 250.0f) 이내인지 검증합니다.
	// 2. IMEOInteractableInterface::Execute_ExecuteInteraction(TargetInteractable, ControlledCombatCharacter)를 실행해
	//    바닥 아이템을 인벤토리(UMEOInventoryComponent)에 추가하거나 던전 포탈을 작동시킵니다.

	//	상호작용이 실행하더라도 다른키가 눌리면 취소 되어야한다.
	//	상호작용은 하나만 실행된다.
	//	뭐 인벤토리 추가해야하는데 꽉찼을때,던전 포탈은 열려잇는데 횟수가 없을때,상자가 잠겨있을때(POE2에는 잠긴상자는 없다),일회용 성소를 사용했을때 등등
	//	상호작용을 하더라도 각각의 조건이 맞지 않는다면 행동이 취소되어야만 한다.
	//	상호작용은 좌클릭으로 진행하며 우선권이 제일높다.
}

bool AMEOPlayerController::Server_RequestDodgeRoll_Validate(FVector_NetQuantize RollDirection)
{
	//	방향 백터가 유효하지 않거나 NaN인 경우 차단
	return !RollDirection.ContainsNaN();
}

void AMEOPlayerController::Server_RequestDodgeRoll_Implementation(FVector_NetQuantize RollDirection)
{
	// TODO: [구르기 컴포넌트 연동 후 구현]
	// ControlledCombatCharacter의 UMEODodgeRollComponent::TryDodgeRoll(RollDirection)을 호출하여
	// 스태미나 차감, 충돌 채널(ECC_Pawn) 무시 처리, 및 초기 60% 구간 i-Frame 무적 타이머를 가동합니다.

	// 구르기를 하는데,몬스터한테 막히면 구르기는 되는데 이동이 똑바로 되지 않거나 막히는
	// 몬스터에게 둘러싸여있을때 무한 구르기를 해서 계속피하는것도 이상한데,이것에 대한 해결은 어떻게 해야하는거지?
	// 그리고 구르기로 회피하는 피해유형은 어떻게 되는거지?직사 사격은 당연히 회피를 해야하나?범위 사격은? 저주는? 
	// 함정은?그리고 이속 디버프 같은거라도 받은 상태에서의 구르기는 어떻게 작동해야하지?구르기 입력 속도나 구르는 속도,구르는 거리도 줄어드나?
	// 구르기 중에 방향키를 입력하면 구르는방향에 영향이 가게 어떻게하지?그냥 되나?
	// 구르는중에 행동을 못하게 막을려면?스킬이나 상호작용을 눌렀을때 굴러버리면?
	// '물리데미지를 100을 받습니다'를 선택하고 이벤트가 일어나기전에 구르기 해버리면 데미지 안입을까?
}

bool AMEOPlayerController::Server_RequestSkillCast_Validate(int SkillSlotIndex, FVector_NetQuantize100 TargetLocation, AActor* TargetActor)
{
	//	유효하지 않은 스킬 슬룻 인덱스(0~8 범위 이탈)
	if (SkillSlotIndex < 0 || SkillSlotIndex>8)
	{
		return false;
	}

	//	NaN(Not a Number) 등 비정상적인 좌표 패킷 공격은 차단
	if (TargetLocation.ContainsNaN())
	{
		return false;
	}

	return true;
}

void AMEOPlayerController::Server_RequestSkillCast_Implementation(int SkillSlotIndex, FVector_NetQuantize100 TargetLocation, AActor* TargetActor)
{
	// 오직 서버 권한(Authority)에서만 실행됩니다.
	// TODO: [전투 컴포넌트 및 서브시스템 연동 후 구현]
	// 1. ControlledCombatCharacter에서 UMEOSkillComponent를 가져옵니다.
	// 2. UMEOResourceSubsystem을 통해 마나/정신력 잔여량을 검증하고 차감합니다.
	// 3. 스킬 쿨타임(ICD)을 검증한 뒤 실제 투사체 스폰(UMEOPoolingSubsystem) 및 데미지 연산을 집행합니다.
	// 4. 검증 실패 시 Client RPC(Client_CancelSkillCast)를 호출하여 클라이언트의 예측 연출을 롤백시킵니다.

	// 스킬사용 검증을 컨트롤러의 이곳인데,가능한지 확인하기위한 데이터를 받을려면 인터페이스나 델리게이트로 연결해야할듯.
	// 캐릭터한테 받을 데이터는 AControlledCombatCharacter->GetStat() 같은 형식으로 처리하더라도
	// 어차피 플레이어가 조작가능한 캐릭터는 AControlledCombatCharacter 밖에 없으므로,AMonster나 ANPC를 조종할게 아니라면 괜찮을지도?
	// 설령 몬스터를 조작하더라도 그건 껍대기(매쉬나 스킬UI)만 AMonster이고 AControlledCombatCharacter 일듯?
}

// =============================================================================
// [Camera, Cursor Raycasting & UI Helpers]
// =============================================================================

void AMEOPlayerController::BindCameraToCharacter(AActor* CharacterActor)
{
	if (IsValid(ControlledCameraPawn))
	{
		ControlledCameraPawn->SetTargetCharacter(CharacterActor);
	}
}

void AMEOPlayerController::OnInputZoom(float AxisValue)
{
	if (IsValid(ControlledCameraPawn) && FMath::Abs(AxisValue)>0.0f)
	{
		ControlledCameraPawn->AdjustZoom(AxisValue);
	}
}

AActor* AMEOPlayerController::GetActorUnderCursor() const
{
	FHitResult HitResult;
	//	Visibility 채널을 기준으로 마우스 커서 아래에 몬스터,상자,드랍 아이템을 트레이싱을 함
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return HitResult.GetActor();
	}

	return nullptr;
}

bool AMEOPlayerController::GetMouseCursorWorldLocation(FVector& OutWorldLocation) const
{
	FHitResult HitResult;
	//	지형 및 바닥 판정을 위해 ECC_Visibility 채널로 레이케스트를 수행.
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		OutWorldLocation = HitResult.ImpactPoint;
		return true;
	}

	return false;

	// 이동 방식을 WASD로 할것이기 때문에 지형및 바닥 판정을 마우스커서로 알아야 하는 이유가 일단 딱히 없을것으로 보임
	// 혹시모르니 코드는 넣어두고 주석처리 할듯
}

AMEOHUD* AMEOPlayerController::GetMEOHUD() const
{
	// TODO: [AMEOHUD 클래스 구현 후 Cast 반환]
	// return Cast<AMEOHUD>(GetHUD());

	return nullptr;
}
void AMEOPlayerController::RequestToggleUI(EMEOUIWindowType WindowType)
{
	// 플레이어 컨트롤러가 직접 위젯을 생성/제어하지 않고, HUD 액터로 토글 명령을 위임하여 결합도를 낮춥니다.
	// TODO: [AMEOHUD 클래스 구현 후 연동]
	// if (AMEOHUD* MEOHUD = GetMEOHUD())
	// {
	//     MEOHUD->ToggleUIWindow(WindowType);
	// } (void)WindowType;

	// UI출력을 구현을 하는게 아니라 구현된 UI를 출력하도록 요청을 보내는 곳이다.
}
