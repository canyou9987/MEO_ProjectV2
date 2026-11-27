# Layer 2 (4부-B): 화폐 크래프팅 및 아이템 변이 엔진 명세 (`UMEOEconomySubsystem`)

`UMEOEconomySubsystem`은 7종의 화폐 오브(Currency Orb)를 장비 아이템 인스턴스(`FMEOInventoryItemInstance`)에 적용하여 희귀도 승급, 접두·접미사(Prefix/Suffix) 추가·제거·재굴림, 그리고 타락(Corruption) 변이를 수행하는 아이템 제작 서브시스템입니다.

접사 추첨 로직은 `UMEODropSubsystem::RollRandomAffixFromTable()`을 재사용하여 드랍 장비와 크래프팅 장비 간의 규칙 일관성을 유지하며, 사전 검증 API(`CanApplyCurrencyOrb`)를 분리해 클라이언트 UI 툴팁과 서버 권한 실행 로직을 명확히 구분했습니다.

### 인게임 크래프팅 동작 레퍼런스 (`시퀀스 01.mp4`)
* 마법 등급 지팡이에 **제왕의 오브**를 적용해 희귀 등급으로 승급한 뒤, **엑잘티드 오브**로 접두·접미사를 최대 슬롯까지 채우고 **신비학자의 식각기**로 퀄리티를 부여하며 상세 정보(`Alt`)로 각 접사의 티어(`T1~T5`)와 수치 롤링 범위를 검증하는 흐름을 기준으로 설계했습니다.

---

## 1. 클래스 및 주요 API 종합 명세표

| 구분 | 항목명 | 시그니처 / 타입 | 핵심 역할 및 동작 방식 |
| :--- | :--- | :--- | :--- |
| **서브시스템** | `UMEOEconomySubsystem` | `UWorldSubsystem` | • 서버 권한(`GetNetMode() != NM_Client`) 하에 화폐 오브 사용 검증 및 아이템 데이터 변형 총괄 |
| **사전 검증** | `CanApplyCurrencyOrb()` | `EMEOCraftingResult (const FMEOInventoryItemInstance&, EMEOCurrencyType)` | • 아이템 유효성, 타락 여부(`bIsCorrupted`), 희귀도, 잔여 접사 슬롯을 검사해 사용 가능 여부 반환 (UI 하이라이트 및 비활성화 판정용) |
| **화폐 적용** | `ApplyCurrencyOrb()` | `EMEOCraftingResult (FMEOInventoryItemInstance&, EMEOCurrencyType, UDataTable*)` | • `CanApplyCurrencyOrb` 통과 시 화폐별 크래프팅 규칙을 실행하고 `OnItemCrafted` 델리게이트 발행 |
| **크래프팅 알림** | `OnItemCrafted` | `FOnMEOItemCrafted` (Multicast Delegate) | • 결과 코드(`EMEOCraftingResult`)와 아이템 `FGuid`를 전파하여 사운드·VFX 이펙트 및 인벤토리 UI 갱신 트리거 |
| **내부 헬퍼** | `TryAddRandomAffix()` / `TryRemoveRandomAffix()` | `bool` (Private) | • 빈 접두/접미 슬롯 무작위 선택 후 `UMEODropSubsystem`을 통해 중복 없는 신규 접사 추가, 또는 고정 속성(`Implicit`)을 제외한 무작위 접사 제거 |
| **타락 변이** | `ApplyVaalCorruption()` | `EMEOCraftingResult (FMEOInventoryItemInstance&, UDataTable*)` | • 아이템을 영구 타락(`bIsCorrupted = true`) 상태로 전환하고 4가지 변이 결과 중 하나를 균등 추첨 적용 |

---

## 2. 7대 화폐 오브 크래프팅 규칙 및 결과 코드표

### 화폐 오브(`EMEOCurrencyType`)별 적용 규칙

| 화폐 구분 | 대상 희귀도 조건 | 실행 동작 및 슬롯 제한 | 비고 |
| :--- | :--- | :--- | :--- |
| **진화의 오브 (`Transmutation`)** | `Normal` (일반) | • `Magic`(마법) 등급으로 승급 후 접사 `1~2개` 무작위 부여 (`50%` 확률로 2개) | 접두 최대 1 / 접미 최대 1 |
| **확장의 오브 (`Augmentation`)** | `Magic` (마법) | • 비어 있는 접두 또는 접미 슬롯에 접사 `1개` 추가 (최대 2개 제한) | 슬롯 만석 시 `AffixSlotFull` |
| **변화의 오브 (`Alteration`)** | `Magic` (마법) | • 기존 접두·접미사를 모두 제거한 뒤 새로운 마법 접사 `1~2개` 재굴림 | 고정 속성(`Implicit`)은 보존 |
| **제왕의 오브 (`Regal`)** | `Magic` (마법) | • 기존 옵션을 유지한 채 `Rare`(희귀) 등급으로 승급하고 신규 접사 `1개` 추가 | 접두 최대 3 / 접미 최대 3 |
| **엑잘티드 오브 (`Exalted`)** | `Rare` (희귀) | • 빈 접두 또는 접미 슬롯에 신규 접사 `1개` 추가 (최대 6개 제한) | 슬롯 만석 시 `AffixSlotFull` |
| **카오스 오브 (`Chaos`)** | `Rare` (희귀) | • 기존 접두·접미사 중 무작위 `1개를 제거`한 뒤 새로운 접사 `1개 추가` | PoE2 방식의 점진적 변이 규칙 |
| **바알 오브 (`Vaal`)** | 비타락 전체 등급 (`Unique` 포함) | • `bIsCorrupted = true` 영구 잠금 후 4가지 변이(`25%` 균등) 중 1개 실행:<br>① 변화 없음 ② 타락 전용 `Implicit` 추가 ③ 접사 1개 교체 ④ 희귀 등급 강제 변환 및 4~6개 재굴림 | 타락 이후 모든 화폐 사용 차단 |

### 크래프팅 결과 상태 코드 (`EMEOCraftingResult`)

* `Success`: 크래프팅 정상 완료
* `InvalidItem` / `ItemIsCorrupted`: 유효하지 않은 아이템이거나 이미 타락(`bIsCorrupted == true`)되어 수정 불가
* `InvalidRarityForOrb`: 선택한 화폐가 허용하지 않는 희귀도 등급
* `AffixSlotFull` / `NoAffixToModify`: 접두·접미 슬롯이 가득 찼거나 제거할 접사가 없음
* `NoVaildAffixInPool`: 아이템 레벨(`ItemLevel`) 또는 중복 제외(`ExcludedTags`) 조건을 통과한 접사 후보가 테이블에 없음
* `NotServerAuthority`: 클라이언트 환경에서 호출되어 차단됨

---

## 3. 핵심 엔지니어링 및 아키텍처 설계 포인트

* **고정 속성(`Implicit`) 무결성 보호:** 변화의 오브(`Alteration`), 카오스 오브(`Chaos`), 바알 오브(`Vaal`)가 접사를 제거하거나 초기화할 때 `Mod.SlotType != EMEOAffixSlot::Implicit` 조건을 필터링하여, 아이템 베이스 고유 옵션이나 기존 타락 고정 속성이 삭제되지 않도록 보호했습니다.
* **드랍·크래프팅 접사 생성 파이프라인 단일화 (DRY 원칙):** 크래프팅 시 신규 접사를 뽑는 로직을 별도로 중복 구현하지 않고 `UMEODropSubsystem::RollRandomAffixFromTable()`을 직접 호출하여, 아이템 레벨 상한 제한과 동일 `StatTag` 중복 배제 규칙이 드랍과 제작 양쪽에서 100% 동일하게 작동하도록 통합했습니다.
* **UI 프리뷰와 서버 실행 로직의 분리 (`CanApplyCurrencyOrb`):** `BlueprintPure`로 노출된 사전 검증 함수를 분리하여, 인벤토리 UI에서 화폐를 집었을 때 적용 가능한 장비만 하이라이트하고 불가 사유를 툴팁으로 즉시 안내할 수 있습니다.

---

## 4. 설계 Q&A 및 검증 노트

### Q1. 화폐 크래프팅(`ApplyCurrencyOrb`)을 호출하는 주체는 누구인가요?
* 전역 게임 규칙을 관리하는 `GameMode`가 아니라, 개별 플레이어의 인벤토리 소유권을 가진 **`UMG UI` → `MEOPlayerController` (Server RPC) → `UMEOInventoryComponent` (또는 `PlayerState`)** 경로로 호출합니다.
* 플레이어가 UI에서 화폐 우클릭 후 대상 아이템을 클릭하면 `Server_RequestCraftItem(CurrencyId, TargetId)` RPC를 전송하고, 서버의 인벤토리 컴포넌트가 두 아이템의 존재 여부와 화폐 수량(`StackCount >= 1`)을 검증·차감한 뒤 `UMEOEconomySubsystem::ApplyCurrencyOrb()`를 실행합니다.

### Q2. `TryAddRandomAffix()` 내부에서 `RollRandomAffixFromTable()`이 `false`를 반환하는 경우는 언제인가요?
* ① `AffixDataTable` 에셋이 유효하지 않거나(`nullptr`), ② 대상 아이템의 `ItemLevel`이 너무 낮아 데이터 테이블 내 최소 요구 레벨을 충족하는 접사가 없거나, ③ 해당 슬롯(`Prefix`/`Suffix`)에 존재하는 모든 `StatTag`가 이미 아이템에 부여되어 중복 없이 뽑을 수 있는 후보가 모두 고갈된 경우 `false`를 반환합니다.

### Q3. 타락(Vaal Orb) 또는 신성한 오브로 기존 접사의 수치 한계치(예: `400%` → `410%`)를 돌파하게 만들려면 어떻게 확장하나요?
* 데이터 테이블의 `MinValue ~ MaxValue` 범위에서 새로 뽑는 방식 대신, 이미 아이템에 부여된 `Item.Modifiers` 배열을 순회하면서 `Mod.RolledValue`에 직접 무작위 배율(예: `0.78f ~ 1.22f`)을 곱해 덮어쓰면 테이블 상한선(`400%`)을 넘어서는 오버롤(Over-roll) 변이를 구현할 수 있습니다.

---

## 5. 추후 확장 로드맵 (레퍼런스 기반 크래프팅·소켓·퀄리티 확장안)

코어 7종 화폐 파이프라인 구축 이후, 인게임 레퍼런스 분석을 바탕으로 다음 4가지 보조 크래프팅 시스템을 확장 후보로 정의했습니다.

| 확장 분류 | 레퍼런스 화폐 예시 | 동작 메커니즘 | 데이터 구조 확장 포인트 (`MEODataTypes.h`) |
| :--- | :--- | :--- | :--- |
| **확정 접사 부여 (Essence)** | 상위 전투의 에센스 (`Greater Essence of Battle`) | • 마법 → 희귀 등급 업그레이드와 동시에 무작위 추첨 대신 **에센스에 지정된 확정 `AffixRowName` 1개를 강제 부여** | `ApplyEssenceOrb(TargetItem, GuaranteedAffixRow)` 오버로드 추가 |
| **소켓 및 룬 홈 확장** | • 하위 쥬얼러 오브 (`Lesser Jeweller's Orb`)<br>• 숙련공의 오브 (`Artificer's Orb`) | • 스킬 젬의 보조 젬 소켓 상한 확장 (`2 → 3 → 4 → 5홈`)<br>• 무기·방어구에 룬/영혼 핵 장착용 증강물 홈 추가 | `FMEOSocketedSkillInstance`에 `MaxSupportSockets`, `FMEOInventoryItemInstance`에 `RuneSockets` 필드 추가 |
| **퀄리티 향상 (Quality)** | • 대장장이의 숫돌 (`Blacksmith's Whetstone`)<br>• 세공사의 프리즘 (`Gemcutter's Prism`)<br>• 유리직공의 방울 (`Glassblower's Bauble`) | • 무기·방어구·스킬 젬·플라스크의 `Quality` 수치를 상한(`+20%`)까지 단계적 증가시켜 기본 물리 피해·방어도·젬 효과 증폭 | `FMEOInventoryItemInstance` 및 `FMEOSocketedSkillInstance`에 `int32 Quality = 0` 필드 추가 |
| **미확인 아이템 식별** | 감정 주문서 (`Scroll of Wisdom`) | • 희귀/고유 등급 드랍 시 `bIsIdentified = false` 상태로 생성 후, 감정 주문서 사용 시 접사 목록(`Modifiers`) 공개 | `FMEOInventoryItemInstance`에 `bool bIsIdentified = true` 플래그 추가 |