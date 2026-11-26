# Layer 2 (4부-A): 루트 드랍 및 접사 추첨 엔진 명세 (`UMEODropSubsystem`)

`UMEODropSubsystem`은 몬스터 처치 시 아이템 수량 증가(IIQ)와 희귀도 증가(IIR) 스탯을 반영해 루트 테이블에서 드랍 아이템 인스턴스를 생성하고, 장비 아이템의 등급(일반·마법·희귀·고유) 결정 및 아이템 레벨(`iLvl`)에 따른 유효 접사(Prefix/Suffix) 롤링을 수행하는 보상 생성 서브시스템입니다.

난수 연산은 `UMEODiceSubsystem`의 가중치 추첨(`RollWeightedIndex`) 및 주사위 굴림(`RollDice`)에 위임하여 서버 권한(Server Authority) 하에 일관된 확률 분배와 전역 고유 식별자(`FGuid`) 발급을 보장합니다.

### 레퍼런스 시각화 (화폐 크래프팅 및 접사 구조)

* **화폐 오브(Currency) 보관함 레퍼런스:**
  ![화폐 종류 예시](./Path_of_Exile_2_Screenshot_2026.11.11_-_13.29.25.842.png)

* **희귀 장비 고정 속성 및 접두/접미사(Tier) 레퍼런스:**
  ![접사 구조 예시](./Path_of_Exile_2_Screenshot_2026.11.03_-_10.49.08.1811.png)

---

## 1. 아이템·드랍 핵심 데이터 구조 (`MEOItemTypes.h`)

| 구조체 / 열거형 | 구분 | 주요 필드 및 역할 |
| :--- | :--- | :--- |
| **`EMEOItemRarity`** | `Enum` | • `Normal`(접사 0개), `Magic`(접두 1 + 접미 1, 최대 2개), `Rare`(접두 최대 3 + 접미 최대 3, 최대 6개), `Unique`(고정 고유 옵션) |
| **`EMEOAffixSlot`** | `Enum` | • `Implicit`(고정 속성), `Prefix`(접두사: 공격력·생명력·방어도 등), `Suffix`(접미사: 저항·속도·치명타 등) |
| **`EMEOCurrencyType`** | `Enum` | • `Transmutation`(진화), `Augmentation`(확장), `Alteration`(변화), `Regal`(제왕), `Exalted`(엑잘티드), `Chaos`(카오스), `Vaal`(바알) |
| **`FMEOItemModifierInstance`** | `Struct` | • `AffixRowName`, `SlotType`, `StatTag`, `RolledValue`(최종 결정 수치), `Tier`(1티어 최고 등급) |
| **`FMEOAffixTableRow`** | `FTableRowBase` | • 데이터 테이블용 접사 풀 정의 (`SlotType`, `StatTag`, `Tier`, `RequiredItem`, `SpawnWeight`, `MinValue`, `MaxValue`) |
| **`FMEOLootTableEntry`** | `Struct` | • 루트 테이블 개별 후보 항목 (`ItemBaseRowName`, `DropWeight`, `MinStack`, `MaxStack`) |
| **`FMEOLootGenerationContext`** | `Struct` | • 드랍 연산 컨텍스트 (`AreaLevel`, `IncreasedItemQuantity`, `IncreasedItemRarity`) |

---

## 2. 클래스 및 주요 API 종합 명세표

| 구분 | 항목명 | 시그니처 / 타입 | 핵심 역할 및 동작 방식 |
| :--- | :--- | :--- | :--- |
| **서브시스템** | `UMEODropSubsystem` | `UWorldSubsystem` | • 서버 권한(`GetNetMode() != NM_Client`) 환경에서만 아이템 인스턴스 및 고유 `FGuid` 생성 |
| **드랍 목록 생성** | `GenrateLootDrops()` | `TArray<FMEOInventoryItemInstance> (LootTable, Context, BaseDropsRolls)` | • IIQ를 반영해 드랍 횟수 산출 후 가중치 추첨 및 스택 수량 결정 |
| **희귀도 추첨** | `RollItemRarity()` | `EMEOItemRarity (float IncreadItmeRarity)` | • 기본 가중치(`1000 : 300 : 80 : 5`)에 IIR 배율을 곱해 등급 결정 |
| **접사 롤링** | `RollRandomAffixFromTable()` | `bool (UDataTable*, int32 ItemLevel, EMEOAffixSlot, ExcludedStatTags, OutAffix)` | • 슬롯·아이템 레벨·중복 스탯 그룹을 필터링한 유효 후보군에서 가중치 추첨 및 범위 보간 |

---

## 3. 확률 및 드랍 연산 파이프라인

### 아이템 수량(IIQ) 기반 드랍 횟수 산출
* **기대 드랍 횟수:**
  $$\text{ExpectedRolls} = \text{BaseDropsRolls} \times \max(0.0, 1.0 + \text{IncreasedItemQuantity})$$
* **정수부 + 소수점 보정:** `FloorToInt(ExpectedRolls)`만큼 확정 드랍을 수행하고, 나머지 소수점(`FractionalChance`)은 `DiceSub->RollChance()`로 굴려 성공 시 `FinalDropCount`를 1회 추가합니다. (예: 기대 횟수 `2.4회` → 2회 확정 + 40% 확률로 1회 추가)

### 희귀도(IIR) 가중치 분배 규칙
* `RarityFactor = Max(0.0f, 1.0f + IncreasedItemRarity)`
* **가중치 비율:** `Normal (1000.0)` : `Magic (300.0 × RarityFactor)` : `Rare (80.0 × RarityFactor)` : `Unique (5.0 × RarityFactor)`
* 일반(Normal) 등급의 가중치를 고정하고 상위 등급의 가중치만 비례 증폭시켜, IIR 스탯이 높을수록 상위 희귀도 출현 확률이 자연스럽게 상승합니다.

### 접사 풀(Pool) 3중 필터링 및 수치 보간
1. **슬롯 일치 검증:** 요청된 `TargetSlot`(`Prefix` 또는 `Suffix`)과 일치하는 행만 추출
2. **아이템 레벨(`iLvl`) 상한 제한:** `Row->RequiredItem <= ItemLevel` 조건으로 고티어 옵션 출현 제한
3. **동일 스탯 그룹 중복 방지:** `ExcludedStatTags`에 이미 보유한 `StatTag`가 포함되어 있으면 후보에서 제외 (예: 동일 장비에 최대 생명력 옵션 중복 부여 차단)
4. **균등 수치 보간:** `RollDice(1, 1000, -1.0f) / 999.0f`로 산출된 `RollAlpha`를 이용해 `FMath::Lerp(MinValue, MaxValue, RollAlpha)`로 최종 옵션 수치를 결정합니다.

---

## 4. 설계 Q&A 및 검증 노트

### Q1. 드랍 생성(`GenrateLootDrops`) 이후 장비의 희귀도와 접사 부여는 어떤 흐름으로 연결되나요?
* ① `GenrateLootDrops()`로 드랍할 베이스 아이템 목록을 생성합니다.
* ② 마스터 아이템 데이터 테이블(`FItemBaseRow`)에서 카테고리를 조회하여, 화폐·소모품이면 그대로 드랍하고 **장비품일 경우 `RollItemRarity()`로 희귀도를 결정**합니다.
* ③ 결정된 등급(마법 1~2개, 희귀 3~6개)에 맞춰 **`RollRandomAffixFromTable()`을 반복 호출해 접사를 부여**합니다.

### Q2. 이 드랍 파이프라인은 몬스터, 플레이어, GameMode 중 누가 호출해야 하나요?
* 치트 방지 및 서버 권한 보장을 위해 플레이어(클라이언트)가 직접 호출해서는 안 되며, **몬스터 보고 → `GameMode` 중앙 실행** 구조로 역할을 분리합니다.
* **몬스터(`OnDeath`):** 사망 시 자신의 루트 테이블, 몬스터 레벨(`AreaLevel`), 막타 공격자 참조만 담아 `GameMode`에 처치 이벤트를 알립니다.
* **`GameMode` (중앙 중재자):** 막타 플레이어의 IIQ/IIR 스탯과 현재 던전 모디파이어(`FMEOZoneModifierData`)를 합산해 `FMEOLootGenerationContext`를 조립한 뒤 `UMEODropSubsystem`을 호출하고, 생성된 아이템을 `UMEOPoolingSubsystem`의 바닥 아이템 액터에 담아 스폰합니다.