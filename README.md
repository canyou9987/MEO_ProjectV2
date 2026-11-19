# Project Waystone: Overdrive (MEO_ProjectV2)
Unreal Engine 5 C++ 기반 쿼터뷰 액션 RPG 프레임워크 포트폴리오입니다.

---

## 1. 시스템 아키텍처 및 기술 명세 (Docs)
구현 완료된 계층별 설계 의도와 데이터 파이프라인 문서입니다.

* **[Layer 1: 프레임워크 & 세션 계층 명세](./Docs/Layer 1 (프레임워크 & 세션 계층).md)**
  * `GameInstance` - `GameMode` - `PlayerState` 간 세이브 데이터 로드 및 단일 원본(SSOT) 파이프라인
  * `TMap` 네트워크 복제 우회(`TArray` 직렬화 후 `OnRep` 재구축) 및 20Hz RPC 스로틀링
  * 캐릭터 사망 시 시점·UI 유지를 위한 `CameraPawn` 및 `HUD` 생명주기 분리
* **[SubSystem: 오브젝트 풀링 시스템](./Docs/SubSystem_ObjectPooling.md)** *(작성 예정)*
  * `UMEOPoolingSubsystem` 기반 런타임 액터 재사용 및 GC 스파이크 방지

---

## 2. C++ 소스 코드 구조 (`Source/MEO_ProjectV2/`)
* **`DataStruct/`**: 공용 열거형(`EMEOUIWindowType`, `EMEOEquipmentSlot`) 및 직렬화 구조체(`MEODataTypes.h`)
* **`GameMode/`**: 세션 코어(`MEOGameMode`, `MEOGameState`, `MEOPlayerState`), 입력/RPC 제어(`MEOPlayerController`), 독립 카메라(`MEOCameraPawn`), 뷰 매니저(`MEOHUD`)
* **`SaveGame/`**: 로컬 바이너리 직렬화(`MEOSaveGame`) 및 앱 생명주기 데이터 캐싱(`MEOGameInstance`)
* **`SubSystem/`**: 월드 서브시스템 기반 풀링 매니저(`MEOPoolingSubsystem`)

---

## 3. 형상 관리 규칙
* **Branch:** `main`(빌드 무결성 유지) / `feature/*`(기능 단위 개발)
* **Commit:** Conventional Commits (`feat:`, `fix:`, `refactor:`, `docs:`, `perf:`) 준수