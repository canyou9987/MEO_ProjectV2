# Layer 2 (3부-B): 이벤트 트리거 및 메타 젬 파이프라인 명세 (`UMEOTriggerSubsystem`)

`UMEOTriggerSubsystem`은 치명타 적중, 적 처치, 누적 피격 피해, 상태이상 유발 등 전투 이벤트 발생 시 연결된 보조 스킬을 자동으로 시전하는 **메타 젬(Meta Gem / 발동형 보조 젬) 파이프라인**을 중앙에서 통제하는 월드 서브시스템입니다.

액터별 트리거 바인딩의 등록·해제 생명주기를 관리하며, **재귀 깊이 제한(`MAX_TRIGGER_RECURSION_DEPTH`)**과 **내부 쿨타임(`UMEOTimeSubsystem`)**을 결합한 이중 안전장치를 통해 자동 발동 스킬 간 무한 연쇄 루프로 인한 서버 크래시를 원천 차단합니다.

---

## 1. 클래스 및 주요 API 종합 명세표

| 구분 | 항목명 | 시그니처 / 타입 | 핵심 역할 및 동작 방식 |
| :--- | :--- | :--- | :--- |
| **서브시스템** | `UMEOTriggerSubsystem` | `UWorldSubsystem` | • 월드 단위로 액터별 메타 젬 바인딩(`FMEOMetaTriggerBinding`)과 누적 게이지 상태 관리<br>• `TWeakObjectPtr<AActor>` 키 사용으로 액터 파괴 시 댕글링 포인터 참조 방지 |
| **바인딩 등록** | `RegisterTriggerBinding()` | `void (AActor* OwnerActor, const FMEOMetaTriggerBinding& NewBinding)` | • 장비·스킬 소켓 장착 시 고유 `BindingId` 기준으로 트리거 조건과 발동 대상 스킬 등록(중복 시 덮어쓰기) |
| **개별 해제** | `UnregisterTriggerBinding()` | `void (AActor* OwnerActor, FName BindingId)` | • 특정 젬 소켓 해제 시 해당 `BindingId` 항목 제거 및 빈 배열 자동 정리 |
| **일괄 초기화** | `ClearAllBindingForActor()` | `void (AActor* OwnerActor)` | • 무기 세트 스왑(Dual Spec) 또는 캐릭터 리스폰 시 액터에 등록된 모든 트리거 바인딩 일괄 초기화 |
| **이벤트 통지** | `NotifyCombatEvent()` | `void (const FMEOTriggerEventContext& EventContext)` | • 데미지·상태이상·사망 처리부에서 전투 이벤트 발생 시 호출하는 단일 진입점(Entry Point) |
| **실행 알림** | `OnMetaSkillTriggered` | `FOnMEOMetaSkillTriggered` (Multicast Delegate) | • 모든 검증을 통과한 스킬 실행 요청(`CasterActor`, `SkillRowName`, `EventContext`)을 Layer 3 `UMEOSkillComponent`로 전파 |

---

## 2. 메타 젬 트리거 조건 및 6단계 발동 검증 파이프라인

### 지원 트리거 조건 (`EMEOTriggerCondition`)

| 조건 열거형 | 메타 젬 유형 예시 | 이벤트 발생 주체 (`SourceActor`) | 주요 동작 특징 |
| :--- | :--- | :--- | :--- |
| **`OnCriticalHit`** | 치명타 시 시전 (Cast on Critical Strike) | 치명타를 적중시킨 공격자 | • 적중 대상(`TargetActor`) 또는 발생 좌표를 향해 연결된 주문 즉시 시전 |
| **`OnKill`** | 처치 시 시전 (Cast on Kill) | 적을 처치한 공격자 | • 사망한 적 위치(`EventLocation`)에서 연쇄 폭발·소환 스킬 발동 |
| **`OnDamageTaken`** | 피격 시 시전 (Cast when Damage Taken) | 피해를 입은 피격자 | • 받은 피해량(`EventValue`)을 누적하다 `RequiredAccumulatedValue` 도달 시 방어 버프·반격기 발동 |
| **`OnAilmentInflicted`** | 상태이상 시 시전 (Cast on Freeze / Shock / Ignite) | 상태이상을 유발한 공격자 | • 동결·감전·점화 성공 시 후속 원소 스킬 연쇄 발동 |

### `NotifyCombatEvent()` 6단계 검증 파이프라인

1. **서버 권한 및 유효성 검사:** `World->GetNetMode() == NM_Client`인 클라이언트 환경을 조기 차단하고 싱글플레이 및 서버 권한(`Authority`) 환경에서만 연산 수행
2. **재귀 깊이 상한 검사:** `CurrentRecursionDepth >= MAX_TRIGGER_RECURSION_DEPTH(2)` 도달 시 즉시 반환하여 연쇄 폭주 차단
3. **내부 쿨타임(ICD) 검사:** `UMEOTimeSubsystem::IsOnCooldown()`을 조회해 동일 바인딩이 재발동 대기시간(`InternalCooldown`, 기본 `0.15초`) 중이면 스킵
4. **누적 임계치 검증 및 이월:** `RequiredAccumulatedValue > 0`인 경우 `CurrentAccumulatedValue`에 피해량을 누적하고, 임계치 도달 시 초과 잔여분을 다음 주기로 이월(`CurrentAccumulatedValue -= RequiredAccumulatedValue`)
5. **순수 확률 굴림:** `UMEODiceSubsystem::RollChance(TriggerChance)`를 통해 최종 발동 확률 판정
6. **쿨타임 등록 및 시전 전파:** `UMEOTimeSubsystem::StartCooldown()`으로 내부 쿨타임을 기록한 뒤 `OnMetaSkillTriggered.Broadcast()` 호출

---

## 3. 핵심 엔지니어링 및 아키텍처 설계 포인트

* **이중 무한 루프 방지 메커니즘 (Recursion Depth + Internal Cooldown):** 메타 젬으로 자동 발동된 스킬이 다시 치명타나 상태이상을 유발해 자기 자신이나 다른 메타 젬을 끝없이 호출하는 무한 재귀를 막기 위해, 동기 호출 스택 깊이를 `2단계`로 제한하고 바인딩별 최소 `0.15초` 내부 쿨타임을 강제했습니다.
* **초과 누적 에너지 이월 (Overflow Carry-over):** 피격 시 시전(CWDT) 트리거에서 요구 임계치가 `500`일 때 한 번에 `800`의 피해를 입더라도 `500`만 차감하고 `300`을 다음 누적치로 이월하여, 큰 단발 피해를 입었을 때 트리거 에너지가 손실되는 불합리를 방지했습니다.
* **실행 계층과의 완벽한 결합도 분리:** `UMEOTriggerSubsystem`은 스킬의 마나 소모, 투사체 스폰, 애니메이션 재생을 직접 수행하지 않고 `OnMetaSkillTriggered` 델리게이트로 시전 명령만 발행하므로, 전투 판정 계층(Layer 2)이 스킬 실행 컴포넌트(Layer 3)에 의존하지 않습니다.

---

## 4. 설계 Q&A 및 검증 노트

### Q1. `UMEOTriggerSubsystem`은 무엇의 트리거이며, 왜 레지스트리로 등록하며 통제해야 하나요?
* 플레이어가 직접 누르는 스킬이 아니라 *"치명타 적중 시 화염구 자동 발사"*, *"누적 피해 500 피격 시 방어막 자동 시전"* 같은 **조건부 자동 발동 링크(메타 젬)**를 통제하는 서브시스템입니다.
* 무기 세트 스왑 시 이전 무기 소켓의 트리거 규칙을 즉시 해제(`ClearAllBindingForActor`)하고, 피격 누적 게이지(`CurrentAccumulatedValue`)를 바인딩별로 보존하며, 무한 연쇄 루프를 서버 단에서 일괄 차단하기 위해 중앙 레지스트리 관리가 필수적입니다.

### Q2. `NotifyCombatEvent()` 안에서 `CurrentRecursionDepth++` 했다가 함수 끝에서 `--` 하는 것이 어떻게 재귀를 막나요?
* C++ 동기 함수 호출 스택에서는 `OnMetaSkillTriggered.Broadcast()`가 실행되는 순간 함수가 종료되기 전에 연쇄 스킬이 적중하여 다시 `NotifyCombatEvent()`가 중첩 호출될 수 있습니다.
* 평상시 단발 공격은 `0 → 1 → 0`으로 왕복하지만, *"치명타 시 화염구 → 화염구가 점화 유발 → 점화 시 얼음폭발 → 얼음폭발이 치명타 유발"*처럼 한 이벤트가 꼬리를 물고 파고들면 `0 → 1 → 2`까지 깊이가 중첩되며, 3번째 진입 시 입구의 `CurrentRecursionDepth >= 2` 조건에 걸려 연쇄가 차단된 뒤 차례로 `--` 되며 안전하게 복구됩니다.

```cpp
// 재귀 깊이 카운터 증가 (함수 종료 전 반드시 감소)
CurrentRecursionDepth++;

for (FMEOMetaTriggerBinding& Binding : *Bindings)
{
    // ... 내부 쿨타임, 누적 임계치, 발동 확률 검증 ...
    OnMetaSkillTriggered.Broadcast(EventContext.SourceActor, Binding.TriggeredSkillRowName, EventContext);
}

CurrentRecursionDepth--;
```

### Q3. `World->GetNetMode() == NM_Client` 조건은 어떤 의미인가요?
* 해당 `if`문은 실행을 중단하고 빠져나가는 조기 반환(Early Return) 필터이므로, **"일반 클라이언트 접속 환경이면 실행하지 말고 빠져나가라"**는 뜻입니다. 결과적으로 싱글플레이(`NM_Standalone`) 및 서버 권한(`NM_ListenServer`, `NM_DedicatedServer`) 환경에서만 트리거 판정을 수행하도록 보장합니다.