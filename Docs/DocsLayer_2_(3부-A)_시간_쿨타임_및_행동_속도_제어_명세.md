# Layer 2 (3부-A): 시간, 쿨타임 및 행동 속도 제어 명세 (`UMEOTimeSubsystem`)

`UMEOTimeSubsystem`은 월드 내 모든 액터의 스킬 재사용 대기시간(Cooldown), 행동 속도(Action Speed) 가감속 배율, 군중제어(동결)에 의한 행동 정지, 그리고 타격감을 위한 히트스톱(Hit-Stop)을 중앙에서 통제하는 시간 관리 서브시스템입니다.

`UTickableWorldSubsystem`을 상속받아 개별 스킬이나 액터마다 타이머를 생성하지 않고 단일 맵(`ActiveCooldownsMap`)에서 쿨타임을 일괄 차감하며, 활성화된 쿨타임이 없을 때는 틱 연산을 자동으로 비활성화하여 CPU 오버헤드를 최소화합니다.

---

## 1. 클래스 및 주요 API 종합 명세표

| 구분 | 항목명 | 시그니처 / 타입 | 핵심 역할 및 동작 방식 |
| :--- | :--- | :--- | :--- |
| **서브시스템** | `UMEOTimeSubsystem` | `UTickableWorldSubsystem` | • 월드 단위로 액터별 스킬/트리거 쿨타임과 행동 속도 모디파이어를 통합 관리<br>• `FMEOCooldownEntry` 구조체를 통해 남은 시간(`RemainingTime`) 및 전체 기준치(`TotalDuration`) 추적 |
| **조건부 틱 제어** | `IsTickable()` | `bool () const` | • CDO(`IsTemplate()`) 제외, 서버 권한(`GetNetMode() != NM_Client`) 환경이면서 **활성 쿨타임이 1개 이상(`ActiveCooldownsMap.Num() > 0`)일 때만** 틱 활성화 |
| **쿨타임 시작** | `StartCooldown()` | `void (AActor* TargetActor, FName SkillId, float BaseDuration, float CooldownRecoveryRate)` | • 쿨타임 회복 속도 배율을 반영한 최종 대기시간을 산출하여 등록(이미 존재 시 갱신) |
| **쿨타임 조회** | `IsOnCooldown()` / `GetRemainingCooldown()` | `bool` / `float` | • 특정 스킬이 현재 대기시간 중인지 여부 및 남은 시간(초) 반환 |
| **행동 속도 등록/해제** | `SetActorActionSpeedModifier()` / `RemoveActorActionSpeedModifier()` | `void (AActor*, FName ModifierId, float SpeedMultiplier)` | • 버프·디버프별 고유 `ModifierId`를 키로 행동 속도 배율 등록 및 해제 |
| **최종 행동 속도 산출** | `GetEffectiveActionSpeed()` | `float (AActor* TargetActor)` | • `UMEOAilmentSubsystem`을 조회해 동결(`Freeze`) 상태면 `0.0f` 반환, 그 외에는 등록된 모디파이어 곱연산 누적 반환 |
| **타격감 시간 지연** | `TriggerHitStop()` | `void (float Duration, float TimeDilation)` | • 강한 치명타·처치 시 짧은 시간 동안 월드 시간 배율을 낮췄다가 타이머로 복구 |

---

## 2. 시간 및 속도 연산 공식 규칙표

| 연산 항목 | 적용 공식 | 경계값 및 예외 처리 규칙 |
| :--- | :--- | :--- |
| **최종 쿨타임 산출** | $\text{FinalDuration} = \frac{\text{BaseDuration}}{\text{CooldownRecoveryRate}}$ | • `CooldownRecoveryRate`는 최소 `0.1f` 하한선 보장 (0으로 나누기 방지)<br>• 회복 속도가 `1.5`(50% 증가)일 경우 기본 쿨타임이 `66.7%`로 단축 |
| **동결(Freeze) 오버라이드** | `if (HasAilment(TargetActor, Freeze)) return 0.0f;` | • 다른 가속 버프나 오라가 적용 중이더라도 동결 상태이상 보유 시 행동 속도 `0.0f` 최우선 고정 |
| **최종 행동 속도 배율** | $\text{FinalSpeed} = \text{Clamp}\left(\prod \text{SpeedMultiplier}, 0.1, 3.0\right)$ | • 모든 유효 모디파이어를 곱연산으로 누적하며, 완전 정지를 막기 위해 최소 `10%(0.1f)` ~ 최대 `300%(3.0f)` 범위로 제한 |
| **히트스톱 복구 타이머** | $\text{RealDelay} = \text{Duration} \times \text{TimeDilation}$ | • `SetGlobalTimeDilation`으로 월드 시간이 느려진 만큼 타이머 지연 시간에도 `TimeDilation`을 곱해 실제 경과 시간 기준으로 복구 |

---

## 3. 핵심 엔지니어링 및 아키텍처 설계 포인트

* **유휴 상태 틱 자동 정지 (`ActiveCooldownsMap.Num() > 0`):** `IsTickable()` 조건에 활성 쿨타임 개수를 포함시켜, 전투가 종료되어 쿨타임 중인 스킬이 없을 때는 엔진 틱 루프에서 완전히 제외되도록 최적화했습니다.
* **메모리 안전성 및 자동 정리 (`TWeakObjectPtr` + `CreateIterator`):** `ActiveCooldownsMap`과 `ActionSpeedModifiersMap` 모두 `TWeakObjectPtr<AActor>`를 키로 사용하며, `Tick()` 순회 중 액터가 파괴되거나 쿨타임 배열이 비워지면 즉시 맵 엔트리를 삭제해 메모리 누수를 차단합니다.
* **서브시스템 간 단방향 조회 구조:** `UMEOTimeSubsystem`은 상태이상 데이터를 직접 소유하지 않고 `GetEffectiveActionSpeed()` 호출 시점에만 `UMEOAilmentSubsystem::HasAilment()`를 단방향으로 질의하여 순환 참조 없이 동결 상태를 반영합니다.

---

## 4. 설계 Q&A 및 검증 노트

### Q1. 멀티플레이 게임에서 `TriggerHitStop`의 `SetGlobalTimeDilation`을 그대로 사용해도 되나요?
* `SetGlobalTimeDilation`은 서버 월드 전체의 시간을 지연시키므로, 멀티플레이 환경에서는 한 유저의 치명타가 다른 유저의 화면까지 끊기게 만듭니다. 따라서 `SetGlobalTimeDilation`은 싱글플레이(`NM_Standalone`) 전용으로 제한하고, 멀티플레이에서는 타격한 공격자와 피격 대상 두 액터에게만 `Actor->CustomTimeDilation`을 적용·복구하는 개별 액터 히트스톱으로 확장하는 것이 안전합니다.

### Q2. 행동 속도(Action Speed)와 쿨타임 회복 속도(Cooldown Recovery Rate)는 어떻게 다른가요?
* **행동 속도 (`Action Speed`):** 캐릭터의 공격 속도, 시전 속도, 이동 속도(애니메이션 및 이동 재생 배율) 전체에 최종적으로 곱해지는 상위 대분류 배율입니다.
* **쿨타임 회복 속도 (`Cooldown Recovery Rate`):** 애니메이션 속도와 무관하게 스킬 재사용 대기시간만 단축시키는 독립 스탯으로, `StartCooldown()` 시점에 `BaseDuration`을 나누는 방식으로 적용됩니다.

### Q3. `StartCooldown()`과 `Tick()`의 역할은 어떻게 나뉘나요?
* `StartCooldown()`은 스킬이 시전되는 순간 `CooldownRecoveryRate`를 반영한 최종 쿨타임을 계산해 `ActiveCooldownsMap`에 등록(또는 덮어쓰기)하는 역할만 수행하며, 매 프레임 남은 시간을 차감(`RemainingTime -= DeltaTime`)하고 만료된 항목을 제거하는 연산은 `Tick()`이 전담합니다.

### Q4. `FMEOCooldownEntry`, `FMEOSpiritReservationEntry`에서 `Entry`는 무슨 뜻인가요?
* 배열이나 맵 같은 컨테이너 안에 등록되는 **"개별 항목 / 기록(레코드)"**을 의미합니다. 즉, `FMEOCooldownEntry`는 쿨타임 관리 목록에 들어가는 개별 스킬의 쿨타임 기록 구조체입니다.