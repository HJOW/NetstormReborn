# 원본 월드 코어 W03 여섯째 조각 — 피해가능객체 종속 해방(RawDamageableRelease)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 여섯째 조각이다.
다섯째 조각에서 이식한 피해가능객체 사전 파괴 접두는 "권한이 있고 해방 금지 플래그가 없으면 해방 몸체를 부른다"까지만 옮겼다.
이번 조각은 그 **종속 해방(일반 자산 재생성·사제 재생성) 몸체**를 이었다.
근거는 커밋된 cpppj 소스(`RawDamageableRelease.cpp`·`RawDamageableRelease.h`)와 원본 fixture(`damageablerelease-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawDamageableRelease.cs` (신규) | `DamageableReleaseState`(전투·허용 genus·다리 타입), `DamageableReleaseHooks`(사제 생성·일반 자산 생성·소유자 지정·Pop·표면 알림), `RawDamageableRelease`(`Release`), `MakeDamageableReleaseHooks` | `RawDamageableRelease.cpp` 의 부모 상태 검사 → 중심 계산과 스냅 → 종속 순회 → 해방 가능 마스크 → 허용 genus → 사제/일반 자산 분기 순서 |
| `X86RawDamageableReleaseTests.cs` (신규) | 원본 10.78 몫 1,504행 재생 1건 | `cpppj/tests/DamageableReleaseTests.cpp` 의 `ReleaseScene`·`Hooks`·`Mutation`·`Replay` 열과 같다 |
| `RawDamageableReleaseTests.cs` (신규) | 합성 4건(미연결 효과·자료 크기·잘못된 부모·비유한 좌표·다른 풀 연결) | `DamageableReleaseTests.cpp` 의 `damageable_release_guards_and_binding` 와 같은 구성 |

### 순서와 규칙

`RawDamageableRelease.Release(parent)` 는 원본 순서대로 다음을 실행한다.

1. 부모 슬롯 상태(free·contained)와 타입 번호(자산 70 이상)를 검사한다.
2. 부모 타입의 발자국이 1×1 이면 현재 좌표를, 아니면 **발자국 절반을 뺀 중심**을 쓴다(계산은 double 로 하고 float 에 저장한다).
3. 두 좌표를 각각 `0.99999` 편향으로 더한 뒤 0 방향 절삭해 정수 칸으로 스냅한다(스냅 값은 float 로 저장된다).
4. 내용물 커서를 `allowDead=true` 로 돌려 종속을 순회한다. 반환 전 저장한 다음 링크를 쓰므로 현재 후보의 연결이 바뀌어도 원래 다음 후보로 간다.
5. 후보의 mask WORD 에 해방 가능 비트(`0x20`)가 없으면 건너뛴다.
6. 전투가 아니면 후보 kind 의 타입 플래그2가 허용 genus 마스크와 겹쳐야 한다.
7. 사제 비트(`0x200000`)가 켜져 있으면 사제 생성 몸체를 부른다. 부모 타입에 중심 보정 비트(`0x80400000`)가 있으면 중심에 `0.5` 를 더한 좌표를 쓴다.
8. 그렇지 않으면 spot 바이트의 `0x10` 비트가 켜져 있거나 `0x06` 비트가 꺼져 있으면 건너뛴다. 일반 자산을 생성하고 후보의 WORD(+12)를 새 슬롯에 그대로 복사한 뒤, 다리 타입이면 표면 알림 → 소유자 지정(0) → Pop 순서로 부른다.

좌표 스냅은 `0.99999`, 공간 조회는 `0.9999` 편향을 쓰고 중간 float 반올림을 넣지 않는다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,257개 통과**(Assets 363 + Core 894; 이번 +5).

| 검사 | 내용 |
| --- | --- |
| `damageablerelease` 10.78 몫 1,504행 재생 | "D" 절 752행은 Damageable 직접 호출, "C" 절 752행은 Carrier 경유로 부른다. 매 행마다 외부 호출 순서·부모/후보 슬롯 50바이트 ×5·새 슬롯 50바이트 ×4·두 종류의 현재 전역이 전부 일치한다 |
| 합성 4건 | 미연결 하위 효과·spot 지도 크기·타입 표 크기·할당되지 않은 번호·빈/내용물 상태·자산 밖 타입·비유한 좌표·다른 풀 연결을 효과 전에 거부한다 |

`damageablerelease-x86.tsv` 는 총 4,512행(3 판본 × D/C 2절 × 752행)이며 10.78 몫 1,504행만 재생했다.
`originalCD`(CD 10.72)·`original1037` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

10.78 몫에서 실제로 잡히는 경로는 사제 생성(`P:`)·일반 자산 생성(`N:`)·다리 표면 알림(`S:`)·소유자 지정(`O:`)·
Pop(`L:`)·사제 해방 소리(`F`)·공통 본체(`B:`)이며, 생성·알림·Pop 이 함께 나오는 행이 18행 이상 있다.

**변이 확인:** 구현 11곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 해방 가능 마스크 비트 반전 | 원본 재생 |
| 허용 genus 검사 제거 | 원본 재생 |
| spot 안쪽/통행 불가 검사 제거 | 원본 재생 |
| 발자국 1×1 판정 무시(항상 현재 좌표) | 원본 재생 |
| 스냅 편향 `0.99999` → `0.9999` | 원본 재생 |
| WORD 복사의 상위/하위 바이트 교환 | 원본 재생 |
| 다리 타입 판정을 상수 82 로 고정 | 원본 재생 |
| 사제 중심 보정 `+0.5` 제거 | 원본 재생 |
| 중심 계산에서 발자국 절반 빼기 제거 | 원본 재생 |
| 사제 비트 `0x200000` → `0x400000` | 원본 재생 |
| 부모 상태의 contained 비트 검사 제거 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **부모 상태의 contained 비트 거부는 10.78 행에 없다**(대상 슬롯의 상태 바이트가 0/2/4/6 뿐이다).
  합성 검사가 이 분기를 잡는다.
- Damageable **사후** 파괴(드롭·섬 정리·주변 HP 예약·다리 약화)는 HP·보상·이웃·프레임 몸체를 함께 요구하므로
  다음 조각으로 미뤘다.
- Damageable **피격**(hit), 사제 경계, 내용물 조회(`contained`)·내용물 수명, Form 수명, 신전 삭제는 아직 옮기지 않았다.
- CD 10.72·10.37 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

Damageable 사후 파괴(`damageablepostdestroy`) → Damageable 피격(`damageablehit`) →
종속 조회·내용물(`contained`·`contentlifetime`) → Form 수명(`formlifecycle`·`formtake`·`form-packet`) → 신전 삭제 →
파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
