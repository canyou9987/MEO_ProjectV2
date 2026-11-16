// Fill out your copyright notice in the Description page of Project Settings.


#include "MEO_ProjectV2/GameMode/MEOCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Math/UnrealMath.h"

// Sets default values
AMEOCameraPawn::AMEOCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // 1. 루트 컴포넌트 생성 및 설정
    RootSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComp"));
    RootComponent = RootSceneComp;

    // 2. 쿼터뷰 시점 유지를 위한 스프링 암(CameraBoom) 설정
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = DesiredArmLength; // 초기 줌 거리(1500.0f) 적용

    // 쿼터뷰(Top-Down / Isometric) 특유의 고정 각도(-55도 내리쬐는 시점) 설정
    CameraBoom->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));

    // 캐릭터가 회전하더라도 카메라 시점은 고정되어야 하므로 폰/컨트롤러 회전 상속을 모두 끕니다.
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;

    // 지형지물에 카메라가 가려져 줌이 갑자기 당겨지는 현상을 막기 위해 카메라 충돌 테스트를 비활성화합니다.
    CameraBoom->bDoCollisionTest = false;

    // 3. 실제 화면을 렌더링할 카메라 컴포넌트 부착
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}


// Called every frame
void AMEOCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    //  매 프레임 타겟 위치 추종 및 거리 보간을 수행
	UpdateCameraPosition(DeltaTime);
}

void AMEOCameraPawn::SetTargetCharacter(AActor* NewTargetCharacter)
{
    TargetActor = NewTargetCharacter;
    
    //  새로운 캐릭터가 바인딩 되었다면 사망 시점 고정 상태를 해제하고 즉시 해당 위치로 카메라를 스냅(Snap)한다
    //  (리스폰 지후 카메라가 이전 사망 위치에서부터 먼 거리를 날아오는 어지러운 연출 방지 )
    if (IsValid(TargetActor))
    {
        SetActorLocation(TargetActor->GetActorLocation());

        //  TODO : [캐릭터 및 상태 컴포넌트 연동 후 구현]
        // 1. NewTargetCharacter를 캐릭터(예시 : AMEOCharacterBase)로 캐스팅하여 유효성을 검증
        // 2. 해당 캐릭터의 사망 델리게이트(예: OnCharacterDied)에 현재 카메라 폰의 OnTargetCharacterDied() 함수를 동적 바인딩
        // 3. 피격/강타 연출을 위해 캐릭터의 공격 이벤트와 카메라 셰이크(PlayCameraShake) 호출을 연결
    }
}

void AMEOCameraPawn::AdjustZoom(float ZoomDelta)
{
    //  휠 입력값에 줌 속도를 곱하여 줌 거리를 계산하고, 기획된 최소/최대 반경(800~2200) 안으로 제한
    const float ZoomStep = ZoomDelta * 100.0f;
    DesiredArmLength = FMath::Clamp(DesiredArmLength + ZoomStep, MinArmLength, MaxArmLength);
}

void AMEOCameraPawn::OnTargetCharacterDied()
{
    //  캐릭터가 Destroy() 되더라도 카메라는 파괴되지 않고, 사망한 마지막 좌표에 시점을 고정(아마 리스폰 시키더라도 캐릭터 파괴까지는 X)
    TargetActor = nullptr;

    // TODO: [사망 연출 및 멀티플레이어(Co-op) 관전 시스템 연동 후 구현]
    // 1. 사망 시 시각적 피드백을 위해 FollowCamera의 PostProcessSettings(채도 감소/비네트 효과)를 활성화합니다.
    // 2. 카메라 줌을 살짝 당겨(DesiredArmLength 축소) 캐릭터의 쓰러지는 모션을 강조합니다.
    // 3. 멀티플레이어 파티 플레이 중일 경우: 
    //    AMEOGameState에서 생존해 있는 다른 파티원(AMEOPlayerCharacter) 목록을 조회하고,
    //    마우스 클릭 입력 시 다음 생존 파티원에게 SetTargetCharacter()를 호출하여 관전 시점으로 전환하는 로직을 추가합니다.
}

void AMEOCameraPawn::UpdateCameraPosition(float DeltaSeconds)
{
    //  1.  타겟 캐릭터 위치 추종 로직
    if (IsValid(TargetActor))
    {
        const FVector CurrentLoc = GetActorLocation();
        const FVector TargetLoc = TargetActor->GetActorLocation();
        
        //  FMath::VInterpTo를 사용해 캐릭터의 급격한 이동(구르기, 돌진) 시에도 카메라가 부드럽게(Lag,Lerp) 따라가도록 보간
        const FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc,DeltaSeconds,TargetFollowSpeed);
        SetActorLocation(NewLoc);

        // TODO: [쿼터뷰 마우스 커서 오프셋(Look-Ahead) 확장 예정]
        // 플레이어 시야 확보를 위해 AMEOPlayerController에서 마우스 커서의 월드 좌표를 가져온 뒤,
        // 캐릭터 위치에서 커서 방향으로 일정 비율(예: 최대 150~200 유닛)만큼 치우친 지점을 최종 TargetLoc으로 보정할 수 있습니다.
        // 근데 엑자일2는 시점 고정이라 이걸 구현할지는 미지수
    }

    //  2.  마우스 휠 줌(Zoom) 거리 보간 로직
    if (CameraBoom && !FMath::IsNearlyEqual(CameraBoom->TargetArmLength, DesiredArmLength))
    {
        CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredArmLength,DeltaSeconds,10.0f);
    }
}

