# Project Waystone: Overdrive (MEO_ProjectV2)
Unreal Engine 5 C++ 기반 쿼터뷰 액션 RPG 프레임워크 포트폴리오입니다.

---

## 1. 시스템 아키텍처 및 기술 명세 (Docs)
구현 완료된 계층별 설계 의도와 데이터 파이프라인 문서입니다.

* **[Layer 1: 프레임워크 & 세션 계층 명세](./Docs/Layer_1_(프레임워크_&_세션_계층).md)**
  * `GameInstance` - `GameMode` - `PlayerState` 간 세이브 데이터 로드 및 단일 원본(SSOT) 파이프라인
  * `TMap` 네트워크 복제 우회(`TArray` 직렬화 후 `OnRep` 재구축) 및 20Hz RPC 스로틀링
  * 캐릭터 사망 시 시점·UI 유지를 위한 `CameraPawn` 및 `HUD` 생명주기 분리
* **[Layer 2 (1부): 전투 데이터 구조 및 핵심 연산 서브시스템 명세](./Docs/Layer_2_(1부)_전투_데이터_구조_및%20_핵심_연산_서브시스템_명세_2026.11.21.md)**
  * `UMEODamageSubsystem`: 6단계 데미지 연산 파이프라인(단방향 속성 전환, 합/곱연산 증폭, 방어도·저항 감쇄, ES/생명력 배분)
  * `UMEOResourceSubsystem`: 스킬 마나·생명력 소모 검증 및 오라·소환수 정신력(Spirit) 영구 점유 관리
  * `FMEODamagePacket` 기반 5대 속성별 실피해 보존을 통한 후속 상태이상 계층 결합도 분리
* **[SubSystem: 오브젝트 풀링 시스템](./Docs/SubSystem_ObjectPooling.md)** *(작성 예정)*
  * `UMEOPoolingSubsystem` 기반 런타임 액터 재사용 및 GC 스파이크 방지

---

## 2. C++ 소스 코드 구조 (`Source/MEO_ProjectV2/`)
* **`DataStruct/`**
  * `MEODataTypes.h`: 공용 열거형(`EMEOUIWindowType`, `EMEOEquipmentSlot`) 및 세이브/인벤토리 직렬화 구조체
  * `MEOCombatTypes.h`: 5대 피해 속성(`EMEODamageType`), 데미지 패킷(`FMEODamagePacket`), 자원·정신력 구조체
* **`GameMode/`**: 세션 코어(`MEOGameMode`, `MEOGameState`, `MEOPlayerState`), 입력/RPC 제어(`MEOPlayerController`), 독립 카메라(`MEOCameraPawn`), 뷰 매니저(`MEOHUD`)
* **`SaveGame/`**: 로컬 바이너리 직렬화(`MEOSaveGame`) 및 앱 생명주기 데이터 캐싱(`MEOGameInstance`)
* **`SubSystem/`**
  * `MEODamageSubsystem`: 6단계 데미지 파이프라인 연산 엔진 (`UWorldSubsystem`)
  * `MEOResourceSubsystem`: 스킬 자원 차감 및 정신력(Spirit) 점유 관리 (`UWorldSubsystem`)
  * `MEOPoolingSubsystem`: 월드 서브시스템 기반 액터 풀링 매니저 (`UWorldSubsystem`)

---

## 3. 형상 관리 규칙
* **Branch:** `main`(빌드 무결성 유지) / `feature/*`(기능 단위 개발)
* **Commit:** Conventional Commits (`feat:`, `fix:`, `refactor:`, `docs:`, `perf:`) 준수