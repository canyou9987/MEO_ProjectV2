# Layer 2 (5부-A): 경로석 인스턴스 및 맵 진행 제어 명세 (`UMEOMapManagerSubsystem`)

`UMEOMapManagerSubsystem`은 엔드게임 경로석(Waystone) 아이템을 소모해 던전 인스턴스를 초기화하고, 경로석에 부여된 접사(Modifiers)를 전역 맵 보너스(IIQ·IIR·무리 크기)로 집계하며, 희귀 몬스터·보스 처치 진행도와 잔여 포탈 수명을 중앙에서 통제하는 월드 진행 서브시스템입니다.

맵 클리어 조건 판정과 드랍 컨텍스트 조립(`BuildLootContextForCurrentZone`)을 캡슐화하여, 몬스터 처치 시 `UMEODropSubsystem`이 현재 던전의 지역 레벨과 수량·희귀도 보너스를 즉시 참조할 수 있도록 파이프라인을 연결했습니다.

### 경로석(Waystone) 인게임 레퍼런스
* **경로석(15등급) 아이템 모디파이어 및 부활 횟수 예시:**
  ![경로석 레퍼런스](./Path_of_Exile_2_Screenshot_2026.11.28_-_13.49.55.74.png)

---

## 1. 월드·경로석 핵심 데이터 구조 (`MEOWorldTypes.h`)

| 구조체 / 열거형 | 구분 | 주요 필드 및 역할 |
| :--- | :--- | :--- |
| **`EMEOZoneState`** | `Enum` | • `Hub`(거점/은신처), `Inprogress`(던전 진행 중), `BossEncounter`(보스 조우), `Completed`(목표 완료), `Failed`(포탈 소진 실패) |
| **`FMEOWaystoneMapData`** | `Struct` | • `WaystoneIstanceId`, `MapZoneRowName`, `WaystoneTier`(1~16티어), `AreaLevel`, `RemainingPortals`(기본 6회)<br>• `ActiveModifiers` 배열 및 집계된 `BonusItemQuantity`, `BonusItemRarity`, `BonusPackSize` |
| **`FMEOMapProgressData`** | `Struct` | • `TotalRareMonsters`, `RemainingRareMonsters`, `bHasMapBoss`, `bBossDefeated`<br>• `GetCompletionRatio()`: 전체 목표(희귀 몬스터 수 + 보스) 대비 달성률(`0.0 ~ 1.0`) 산출 헬퍼 |

---

## 2. 클래스 및 주요 API 종합 명세표

| 구분 | 항목명 | 시그니처 / 타입 | 핵심 역할 및 동작 방식 |
| :--- | :--- | :--- | :--- |
| **서브시스템** | `UMEOMapManagerSubsystem` | `UWorldSubsystem` | • 서버 권한(`GetNetMode() != NM_Client`) 하에 현재 월드 인스턴스의 상태 전이와 목표 달성 여부 통제 |
| **인스턴스 초기화** | `InitializeWaystoneSession()` | `bool (WaystoneItem, MapZoneRowName, WaystoneTier, InitialPortals)` | • 경로석 접사를 전역 모디파이어로 변환하고 IIQ·IIR·PackSize 합산 후 `Inprogress` 상태로 전환 |
| **목표 등록** | `RegisterMapObjectives()` | `void (int32 InTotalRareMonsters, bool bInHasMapBoss)` | • 레벨 스폰 완료 시 배치된 총 희귀 몬스터 수와 맵 보스 존재 여부를 등록 |
| **진행도 보고** | `NotifyRareMonsterKilled()` / `NotifyMapBossDefeated()` | `void ()` | • 희귀 몬스터 또는 맵 보스 처치 시 잔여 목표를 갱신하고 클리어 조건(`EvaluateCompletionCondition`) 검사 |
| **포탈 차감** | `ConsumePortal()` | `bool ()` | • 플레이어 사망 또는 입장 시 잔여 포탈 1회 차감, 미완료 상태에서 `0` 도달 시 `Failed` 전환 |
| **모디파이어 조회** | `GetAggregatedZoneModifierValue()` | `float (FName ModifierId) const` | • 특정 태그(예: 몬스터 치명타 확률 증가)의 맵 모디파이어 수치를 모두 합산 반환 |
| **드랍 컨텍스트 조립** | `BuildLootContextForCurrentZone()` | `FMEOLootGenerationContext (PlayerBonusIIQ, PlayerBonusIIR) const` | • 현재 던전의 `AreaLevel`과 맵+플레이어 IIQ/IIR 보너스를 결합한 드랍 연산 패킷 생성 |
| **상태 알림 이벤트** | `OnZoneStateChanged` / `OnMapProgressUpdated` / `OnPortalCountChanged` | Multicast Delegates | • 구역 상태 전이, 목표 달성률 갱신, 잔여 포탈 수 변화를 UI 미니맵·HUD로 전파 |

---

## 3. 인스턴스 수명주기 및 클리어 판정 규칙

### 경로석 티어 및 보상 보너스 집계 규칙
* **지역 레벨 산출:**
  $$\text{AreaLevel} = 64 + \text{Clamp}(\text{WaystoneTier}, 1, 16)$$
  * 1티어 경로석은 `65레벨`, 16티어 경로석은 `80레벨` 인스턴스로 자동 매핑됩니다.
* **접사 태그 자동 분류:** `InitializeWaystoneSession()` 호출 시 `WaystoneItem.Modifiers`를 순회하며 모든 접사를 `ActiveModifiers`에 등록하고, 보상 관련 태그는 별도 누적 변수에 자동 합산합니다.
  * `Map.IIQ` $\rightarrow$ `BonusItemQuantity` (아이템 수량 증가)
  * `Map.IIR` $\rightarrow$ `BonusItemRarity` (아이템 희귀도 증가)
  * `Map.PackSize` $\rightarrow$ `BonusPackSize` (몬스터 무리 크기 증가)

### 구역 상태(`EMEOZoneState`) 전이 흐름
1. **`Hub` $\rightarrow$ `Inprogress`:** 경로석 활성화 시 잔여 포탈(`RemainingPortals`)을 설정하고 던전 진행 시작
2. **`Inprogress` / `BossEncounter` $\rightarrow$ `Completed`:** `RemainingRareMonsters <= 0`이고 보스 처치 조건(`!bHasMapBoss || bBossDefeated`)이 충족되면 즉시 완료 처리
3. **`Inprogress` / `BossEncounter` $\rightarrow$ `Failed`:** 완료(`Completed`)되기 전에 `ConsumePortal()`로 잔여 포탈이 `0`이 되면 공략 실패 처리 (추가 포탈 입장 차단)

---

## 4. 핵심 엔지니어링 및 아키텍처 설계 포인트

* **서브시스템 간 데이터 파이프라인 결합 (`BuildLootContextForCurrentZone`):** 몬스터 사망 시 `GameMode`가 플레이어 스탯과 맵 데이터를 일일이 꺼내 조합할 필요 없이, `UMEOMapManagerSubsystem`이 `FMEOLootGenerationContext`를 완성된 형태로 조립해 `UMEODropSubsystem::GenrateLootDrops()`에 바로 전달할 수 있도록 브릿지 헬퍼를 제공했습니다.
* **동적 목표 구성 대응 (`RegisterMapObjectives`):** 희귀 몬스터만 존재하는 일반 구역과 맵 보스가 함께 배치된 보스 구역을 단일 완료 판정(`EvaluateCompletionCondition`)으로 처리하며, `FMEOMapProgressData::GetCompletionRatio()`를 통해 UI 진행률 게이지를 즉시 계산할 수 있습니다.
* **거점(`Hub`) 안전 기본값 보장:** 거점 상태에서는 `BuildLootContextForCurrentZone()`이 `AreaLevel = 1`을 반환하도록 분기하여, 테스트나 거점 이벤트 중 의도치 않은 고레벨 드랍이 발생하지 않도록 방어했습니다.

---

## 5. 설계 Q&A 및 검증 노트

### Q1. `InitializeWaystoneSession()` 내부의 `MapProgress = FMEOMapProgressData();` 코드는 어떤 역할을 하나요?
* 구조체의 기본 생성자를 호출해 이전 던전에서 누적되었던 희귀 몬스터 처치 수와 보스 처치 플래그(`bBossDefeated`)를 모두 초기값(`0`, `false`)으로 깨끗하게 리셋하는 구문입니다. 연속으로 다음 경로석 인스턴스를 열 때 이전 진행도가 남는 버그를 방지합니다.

### Q2. 1레벨부터 시작하는 캠페인/저티어 구간으로 게임 스코프를 조정하려면 어떻게 확장하나요?
* 현재 `AreaLevel = 64 + WaystoneTier`로 고정된 엔드게임 공식을 `BaseAreaLevel + (WaystoneTier - 1) * LevelStep` 형태로 데이터 테이블이나 변수로 분리하면, 1레벨 시작 저티어 던전부터 80레벨 엔드게임 티어까지 동일한 서브시스템 코드로 유연하게 대응할 수 있습니다.