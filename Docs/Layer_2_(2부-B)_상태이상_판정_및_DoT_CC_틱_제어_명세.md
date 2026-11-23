# Layer 2 (2부-B): 상태이상 판정 및 DoT/CC 틱 제어 명세 (`UMEOAilmentSubsystem`)

`UMEOAilmentSubsystem`은 데미지 파이프라인(`UMEODamageSubsystem`)의 연산 결과 패킷인 `FMEODamagePacket`을 전달받아 5대 속성별 실피해(`MitigatedDamage`)를 분석하고, 점화·중독·출혈·감전·동결·기절 등 6대 상태이상의 부여와 지속시간 틱(Tick)을 중앙에서 통제하는 서브시스템입니다.

언리얼 엔진의 `UTickableWorldSubsystem`을 상속받아 월드 단위로 상태이상 타이머와 DoT(지속 피해) 정산을 일괄 처리하며, 액터마다 개별 타이머나 틱을 돌릴 때 발생하는 오버헤드와 네트워크 동기화 부하를 최소화합니다.

---

## 1. 클래스 및 주요 API 종합 명세표

| 구분 | 항목명 | 시그니처 / 타입 | 핵심 역할 및 동작 방식 |
| :--- | :--- | :--- | :--- |
| **서브시스템** | `UMEOAilmentSubsystem` | `UTickableWorldSubsystem` | 월드 초기화 시 엔진 전역 틱 시스템(`FTickableGameObject`)에 자동 등록되어 활성 상태이상 일괄 갱신 |
| **틱 실행 조건** | `IsTickable()` | `bool () const` | CDO(`IsTemplate()`)를 제외하고, 싱글플레이 및 서버 권한(`GetNetMode() != NM_Client`) 환경에서만 틱 연산 허용 |
| **상태이상 판정** | `EvaluateAndApplyAilments()` | `void (const FMEODamagePacket&, const FMEOAilmentEvaluationParams&)` | 회피 여부를 검증한 뒤, 패킷 내 5대 속성별 실피해 비중과 치명타 여부를 기반으로 상태이상 유발 확률 굴림 및 부여 |
| **틱 및 DoT 정산** | `ProcessAilmentTicks()` | `void (float DeltaTime)` | 월드 내 모든 대상의 활성 상태이상 잔여 시간을 차감하고, `0.25초` 주기로 DoT 피해 정산 및 만료 시 해제 통지 |
| **상태 조회** | `HasAilment()` | `bool (AActor* TargetActor, EMEOAilmentType AilmentType)` | 특정 액터에게 지정된 상태이상이 현재 활성화되어 있는지 여부 반환 (조건부 스킬/패시브 판정용) |
| **감전 배율 조회** | `GetShockDamageTakenMultiplier()` | `float (AActor* TargetActor)` | 대상이 감전 상태일 경우 받는 **번개 피해** 증폭 배율(`1.0f + Magnitude`) 반환 (미적용 시 `1.0f` 반환) |
| **상태 변화 알림** | `OnAilmentStateChanged` | `FOnMEOAilmentChanged` (Multicast Delegate) | 상태이상 부여(`bApplied = true`) 및 만료(`bApplied = false`) 시 UI 디버프 아이콘과 캐릭터 머티리얼/VFX 이펙트에 이벤트 전파 |

---

## 2. 6대 상태이상 및 군중제어(CC) 판정·적용 규칙표

| 상태이상 구분 | 유발 기반 속성 | 발동 조건 및 확률 규칙 | 중첩(Stacking) 규칙 | 효과 및 연산 공식 |
| :--- | :--- | :--- | :--- | :--- |
| **점화 (Ignite)** | `Fire` (화염) | 치명타 적중 시 `100%` 확정 유발, 일반 타격 시 `(Fire / Threshold) + Bonus` 확률 굴림 | **비중첩** (동일 대상에게 더 높은 초당 피해량 위력만 덮어쓰기 유지) | 초당 화염 실피해의 `50%`만큼 `4초`간 화염 지속 피해(DoT) 적용 |
| **중독 (Poison)** | `Chaos` + `Physical` | 중독 유발 확률(`AilmentChanceBonus > 0`) 보유 시 순수 확률(`RollChance`) 판정 | **무한 중첩** (개별 인스턴스가 독립된 남은 지속시간과 위력을 가지고 동시 틱 연산) | 초당 (카오스+물리) 실피해의 `20%`만큼 `2초`간 카오스 지속 피해(DoT) 적용 |
| **출혈 (Bleed)** | `Physical` (물리) | 물리 타격 및 출혈 확률 보유 시 판정 | **비중첩** (최고 위력 우선 유지) | 기본 물리 지속 피해 적용, 대상이 이동 중(`Velocity > 100`)일 경우 **틱 피해량 2배 증폭** |
| **감전 (Shock)** | `Lightning` (번개) | 치명타 적중 시 `100%` 확정 유발, 일반 타격 시 `(Lightning / Threshold) + Bonus` 확률 굴림 | **비중첩** (가장 높은 증폭률 디버프 우선 유지) | `4초`간 대상이 받는 **번개 피해**를 `10% ~ 50%` 증폭 (`1.0f + Magnitude` 배율 제공) |
| **동결 (Freeze)** | `Cold` (냉기) | 냉기 실피해가 대상 한계치의 `5%` 이상(`ColdRatio >= 0.05f`)일 때 축적/발동 판정 | **비중첩** (더 긴 지속시간 또는 강한 효과 갱신) | 냉기 피해 비중에 비례해 `0.5초 ~ 3.0초`간 행동 속도 `0%` 고정 하드 CC |
| **기절 (Stun)** | `Physical` / 강타 | 피해량 대비 기절 한계치 초과 시 발동 | **비중첩** (단발성 경직) | 순간 경직 및 진행 중인 스킬 시전 캔슬 CC |

---

## 3. 핵심 엔지니어링 및 아키텍처 설계 포인트

* **중앙 집중식 틱 정산 (`DOT_TICK_INTERVAL = 0.25f`):** 중독이 수십 중첩 쌓인 몬스터가 다수 존재하더라도 매 프레임(60~144Hz) 체력 차감과 네트워크 리플리케이션을 발생시키지 않고, `TickAccumulator`를 통해 **초당 4회(0.25초 주기)로 일괄 정산**하여 서버 CPU 및 네트워크 대역폭 부하를 절감했습니다.
* **메모리 안전성 및 맵 자동 정리 (`TWeakObjectPtr` + `CreateIterator`):** `ActiveAilmentsMap`의 키를 `TWeakObjectPtr<AActor>`로 관리하고, `ProcessAilmentTicks()` 순회 중 대상 액터가 파괴(`!IsValid`)되거나 모든 상태이상이 만료(`Ailments.Num() == 0`)되면 `It.RemoveCurrent()`로 즉시 맵 엔트리를 제거해 메모리 누수와 댕글링 포인터 참조를 원천 차단했습니다.
* **패킷 기반 계층 간 결합도 분리:** `UMEOAilmentSubsystem`은 공격자의 스탯이나 방어자의 저항력을 다시 계산하지 않고, `UMEODamageSubsystem`이 이미 감쇄 연산을 마친 `FMEODamagePacket.MitigatedDamage` 구조체만 읽어 상태이상 위력을 산출합니다.

---

## 4. 설계 Q&A 및 검증 노트

### Q1. `IsTickable()`의 `!IsTemplate()`와 `World->GetNetMode() != NM_Client`는 어떤 의미이며, 누가 `Tick()`을 호출하나요?
* `!IsTemplate()`은 언리얼 엔진이 클래스 기본값을 보관하기 위해 생성하는 원본 객체(CDO)가 실제 월드 인스턴스처럼 틱이 도는 것을 막는 안전장치입니다.
* `GetNetMode() != NM_Client`는 클라이언트 접속 환경을 제외하고 **싱글플레이(`NM_Standalone`) 및 서버 권한(`NM_ListenServer`, `NM_DedicatedServer`) 환경에서만** 상태이상 연산을 수행하겠다는 의미입니다.
* 상태이상 걸린 액터가 틱을 켜는 것이 아니라, 부모 클래스인 `UTickableWorldSubsystem`이 월드 생성 시 엔진 전역 틱 매니저에 자신을 자동 등록하므로 엔진이 매 프레임 `IsTickable()`을 확인해 `Tick()`을 실행합니다.

```cpp
bool UMEOAilmentSubsystem::IsTickable() const
{
    // CDO(기본 객체) 제외 및 서버 권한 환경에서만 상태이상 틱 연산 실행
    UWorld* World = GetWorld();
    return !IsTemplate() && World && World->GetNetMode() != NM_Client;
}
```

### Q2. `GetShockDamageTakenMultiplier()`는 데미지를 받아서 직접 곱해주는 함수인가요?
* 직접 피해를 차감하지 않고, 대상이 감전 상태일 때 **"후속 번개 피해를 몇 배로 증폭시킬지 배율(`1.0f + Magnitude`, 예: 20% 감전 시 `1.2f`)"**만 반환하는 조회(Query) 전용 함수입니다. 실제 피격 처리부에서 번개 속성 피해에 이 배율을 곱해 최종 피해를 증폭합니다.

### Q3. `ExcuteDoTTickDamage()`에서 계산된 `TickDamage`는 어떻게 실제 체력을 깎나요?
* `UMEOAilmentSubsystem`은 0.25초마다 입혀야 할 지속 피해량(`TickDamage`)을 산출하는 책임까지만 담당합니다. 실제 생명력(`CurrentLife`) 차감과 사망 판정은 Layer 3에서 플레이어·몬스터 공용으로 구현할 `UMEOStatusComponent`와 연동하여 처리합니다.