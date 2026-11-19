# Layer 1 (프레임워크 & 세션 계층) 전체 클래스 명세와 핵심 데이터 파이프라인 완료

### 1. Layer 1 클래스 및 파일 전체 명세

| **분류** | **클래스 / 파일명** | **저장 경로** | **생명주기 (Lifecycle)** | **핵심 역할 및 포트폴리오 기술 포인트** |
| --- | --- | --- | --- | --- |
| **공용 타입** | `MEODataTypes.h` | `DataStruct/` | 전역 헤더 참조 | • **순환 참조 방지:** `EMEOUIWindowType`, `EMEOEquipmentSlot` 등 열거형 중앙 집중화

• **직렬화 구조체:** `FMEOInventoryItemInstance`(`FGuid` 기반 고유 ID), `FMEOSocketedSkillInstance`, `FMEOSaveGameData` 정의 |
| **직렬화 래퍼** | `UMEOSaveGame` | `SaveGame/` | 저장·로드 시 생성 | • 로컬 디스크(`.sav`) 바이너리 직렬화를 위한 `USaveGame` 데이터 컨테이너 |
| **앱 매니저** | `UMEOGameInstance` | `SaveGame/` | 앱 실행 ~ 종료 유지 | • **영속 데이터 I/O:** `Init()` 시 기본 세이브 슬롯 사전 로드, `Shutdown()` 시 `CachedSaveData` 자동 백업

• `SaveCharacterDataToSlot()` / `LoadCharacterDataFromSlot()`로 디스크 입출력 캡슐화 |
| **세션 SSOT** | `AMEOPlayerState` | `GameMode/` | 레벨 세션 유지

*(캐릭터 사망 시 보존)* | • **데이터 단일 원본(SSOT):** 캐릭터 액터가 사망·파괴되어도 레벨·인벤토리·장비·패시브(Dual Spec) 데이터 보존

• **네트워크 최적화:** `TMap` 복제 불가 제약을 우회하기 위해 `TArray<FMEOEquippedItemEntry>`로 복제 후 `OnRep`에서 로컬 `TMap` 재구축 (`NetUpdateFrequency = 10Hz`) |
| **월드 상태** | `AMEOGameState` | `GameMode/` | 맵 인스턴스 생명주기 | • **전역 난이도 및 보상 동기화:** `CurrentAreaLevel`, `ActiveZoneModifiers`, `ZoneItemQuantityBonus(IIQ)`, `ZoneItemRarityBonus(IIR)`를 모든 클라이언트에 복제 |
| **서버 규칙** | `AMEOGameMode` | `GameMode/` | 맵 인스턴스 생명주기

*(서버 전용)* | • **로그인 파이프라인:** `PostLogin()` 시 `GameInstance` 세이브 데이터를 `PlayerState`에 주입 후 캐릭터 스폰

• **규칙 통제:** 독립 카메라 폰과 전투 캐릭터 분리 스폰, 사망 시 카메라 시점 고정 처리, 맵당 포탈 진입 횟수(6회) 차감 검증 |
| **입력/RPC** | `AMEOPlayerController` | `GameMode/` | 레벨 세션 유지 | • **네트워크 보안 및 최적화:** 20Hz(0.05초) RPC 스로틀링으로 광클 패킷 차단, `WithValidation` 및 `FVector_NetQuantize100` 적용

• **조작감 보완:** 0.2초 단위 인풋 버퍼(Input Buffer)로 애니메이션 캔슬 구간 선입력 처리

• **결합도 분리:** UI 토글 입력을 직접 처리하지 않고 `AMEOHUD`로 위임 |
| **독립 카메라** | `AMEOCameraPawn` | `GameMode/` | 레벨 세션 유지

*(캐릭터 사망 시 보존)* | • **시점 분리 아키텍처:** 전투 캐릭터와 액터를 분리해 캐릭터 파괴 시에도 카메라 유지

• **부드러운 보간:** `VInterpTo`(위치 Lag) 및 `FInterpTo`(줌 보간) 적용, `PlayCameraShake()` 지원 |
| **UI 뷰 매니저** | `AMEOHUD` | `GameMode/` | 레벨 세션 유지

*(캐릭터 사망 시 보존)* | • **위젯 생명주기 보호:** 최상위 메인 HUD 위젯을 뷰포트에 단 1회 고정 부착하여 캐릭터 사망 시에도 UI 유지

• **델리게이트 재연결:** 리스폰 시 `BindHUDToCharacter()`를 통해 신규 캐릭터 컴포넌트 델리게이트만 재바인딩 |

### 2. Layer 1 핵심 연동 파이프라인

| **단계** | **실행 흐름** | **핵심 설계 의도 (해결하는 문제)** |
| --- | --- | --- |
| **1. 접속 및 로드** | `UMEOGameInstance::Init()` $\rightarrow$`AMEOGameMode::PostLogin()` $\rightarrow$`AMEOPlayerState` 데이터 주입 | 세이브 파일(`CachedSaveData`)을 세션 시작 시 단일 원본(`PlayerState`)에 적재하여 런타임 데이터 정합성 확보 |
| **2. 분리 스폰** | `AMEOPlayerController::OnPossess(CameraPawn)` $\rightarrow$`AMEOGameMode::RespawnPlayer()` $\rightarrow$`BindCameraToCharacter()` & `SyncDataToCharacter()` | 컨트롤러가 전투 캐릭터가 아닌 **독립 카메라 폰**을 소유(Possess)하도록 분리하여 캐릭터 사망·교체 시 시점 및 입력 끊김 방지 |
| **3. 캐릭터 사망** | 전투 캐릭터 `Destroy()` $\rightarrow$`AMEOGameMode::HandlePlayerDeath()` $\rightarrow$`CameraPawn::OnTargetCharacterDied()` | 캐릭터 액터가 파괴되어도 **`PlayerState`(데이터), `CameraPawn`(시점), `HUD`(위젯)는 파괴되지 않고 유지**됨 |
| **4. 리스폰 복원** | `AMEOGameMode::RespawnPlayer()` $\rightarrow$ 신규 캐릭터 스폰 $\rightarrow$`PlayerState::SyncDataToCharacter()` $\rightarrow$`HUD::BindHUDToCharacter()` | 위젯을 재생성하지 않고 기존 메인 HUD에 신규 캐릭터의 컴포넌트 델리게이트만 재연결하여 리스폰 시 UI 깜빡임(Flicker) 제거 |

## 추가내용 :

- **은신처 $\leftrightarrow$ 던전 이동 시 포탈 잔여 횟수 및 인스턴스 유지 (6~10분)**
    - 언리얼에서 맵을 전환(`OpenLevel` / `ServerTravel`)하면 현재 월드의 `GameMode`와 `GameState`는 파괴되고 새로 생성됩니다.
    - 따라서 포탈 횟수와 던전 시드(Seed)·클리어 상태를 유지하려면 **맵이 바뀌어도 파괴되지 않는 `UMEOGameInstance`에 던전 세션 구조체(`FMEOActiveDungeonSession`)를 캐싱**해 두거나, 포탈 진입 시 완전히 새로운 맵으로 이동하지 않고 **레벨 스트리밍(Level Streaming) / 서브 레벨 로드** 방식으로 은신처와 던전 구역을 한 월드 안에 띄워두는 방식을 사용합니다. (후자일 경우 `UMapManagerSubsystem`이 포탈 상태를 그대로 유지할 수 있습니다.)
- **몬스터 사망 관리 주체 (`HandlePlayerDeath`와의 차이)**
    - 플레이어는 사망 후 리스폰·경험치 페널티·카메라 고정·UI 팝업 등 세션 규칙이 얽혀 있어 `GameMode`가 개입합니다.
    - 반면 **몬스터는 자기 자신의 스탯 컴포넌트(`OnDeath` 델리게이트)에서 사망을 판정**한 뒤, `UMEODropSubsystem`에 드랍 테이블 연산을 요청하고, `UMEOTriggerSubsystem`에 처치 이벤트를 브로드캐스트한 후 풀링 반환 또는 `Destroy()`로 스스로 정리합니다.
- **`AMEOHUD`와 `ToggleUIWindow` 구조**
    - **`ToggleUIWindow`** 는 어떤 토글형 UI가 켜질지 EMEOUIWindowType유형으로 받아서 전달하는곳이다.그리고 클래스 외부에서 작동하기 위해 존재하는것이다.
    - `WindowVisibilityMap`을 캡슐화하고, 단축키 입력(`PlayerController`)이나 UI 버튼 클릭(`X` 닫기 버튼) 어느 쪽에서든 동일한 창 상태를 제어할 수 있도록 만든 공용 인터페이스입니다.