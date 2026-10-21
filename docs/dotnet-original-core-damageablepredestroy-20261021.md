# 원본 월드 코어 W03 다섯째 조각 — 피해가능객체 사전 파괴와 종속 커서

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 다섯째 조각이다.
넷째 조각까지는 공통 삭제 본체와 공통 pre/postDestroy 수명주기만 이식돼 있었다.
이번 조각은 그 위에 얹히는 파생 경계 가운데 **피해가능객체(Damageable)의 사전 파괴 접두**와
그가 쓰는 **내용물(종속) 커서**, **캐리어(Carrier) 경계**를 이었다.
근거는 커밋된 cpppj 소스(`RawDamageablePreDestroy.cpp`·`RawContainedFinder.cpp`·`RawCarrierPreDestroy.cpp`)와
원본 fixture(`damageablepredestroy-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawContainedFinder.cs` (신규) | `ContainedFinderState`(권한·내용물 타입·dead assert), `RawContainedFinder`(`Begin`·`Next`) | `RawContainedFinder.cpp` 의 head WORD 시작·다음 링크 선저장·타입 전역 비교·순환 거부 |
| `RawDamageablePreDestroy.cs` (신규) | `DamageableSound`·`DamageableSoundEvent`·`DamageablePreDestroyState`·`DamageablePreDestroyHooks`·`RawDamageablePreDestroy`(`Validate`·`PreDestroy`) | `RawDamageablePreDestroy.cpp` 의 붕괴/폭발 → 소리 → 사제 해방 소리 → 해방 몸체 → 공통 pre 순서 |
| `RawCarrierPreDestroy.cs` (신규) | `CarrierPreDestroyHooks`·`RawCarrierPreDestroy`(`Validate`·`PreDestroy`·`FindContained`) | `RawCarrierPreDestroy.cpp` 의 Damageable 연결 → 권한 종속 조회(1,0,0) → 전역 후처리 |
| `X86RawDamageablePreDestroyTests.cs` (신규) | 원본 10.78 몫 1,632행 재생 1건 | `cpppj/tests/DamageablePreDestroyTests.cpp` 의 `DamageableScene`·`Hooks`·`Mutation`·`Replay` 열과 같다 |
| `RawDamageablePreDestroyTests.cs` (신규) | 합성 7건(미연결 효과·잘못된 자산·비유한 좌표·abstract 우회·공통 검사 거부·손상 체인·dead 부모) | `DamageablePreDestroyTests.cpp` 의 `damageable_predestroy_guards_and_abstract_coordinates` 와 같은 구성 |
| `tools/sum_fixture.py` (신규) | fixture 절 구성·열 개수 요약 도구 | 여러 fixture 의 절 분포를 빠르게 확인할 때 쓴다 |

### 순서와 규칙

`RawDamageablePreDestroy.PreDestroy` 는 원본 순서 그대로 다음을 실행한다.

1. `Validate`: 자산 상태(free·contained)와 타입 번호(70 이상 188 미만) 검사 → abstract·매림이 아니면 spot 좌표 확인 → 공통 사전 검사 호출.
2. 붕괴 효과(플래그 `0x200000`) 후 붕괴 소리.
3. 폭발 효과(플래그 `0x100000`) 후 폭발 소리.
4. spot 바이트의 2/4 비트가 켜져 있으면 내용물 커서를 `allowDead=true` 로 돌려 **종속 kind WORD 가 사제 타입(기본 158)인 후보마다** 전역 해방 소리를 낸다.
5. 권한(boss) 전역이 켜져 있고 해방 금지 플래그(`0x800`)가 없으면 해방 몸체를 부른다.
6. 공통 preDestroy 본체를 부른다.

extra 의 abstract·매림 비트가 켜져 있으면 2~5번을 건너뛰고 6번만 실행한다(좌표를 읽지 않는다).
spot 좌표는 원본대로 현재 좌표에 float `0.9999` 를 double 로 더한 뒤 0 방향 절삭해 칸을 잡는다.
소리는 효과 뒤의 **현재** raw 좌표를 다시 읽어 만든다(붕괴·폭발 효과가 좌표를 바꿀 수 있다).

`RawContainedFinder` 는 부모 슬롯의 head WORD 에서 시작해 **반환할 후보의 다음 링크를 먼저 저장**하고,
타입이 전역 내용물 타입과 같은 후보만 돌려준다. 후보의 free/dead/void/contained 상태는 거르지 않는다.

`RawCarrierPreDestroy` 는 자산 상태를 검사한 뒤 Damageable 사전 파괴를 부르고,
권한이 있으면 `FindContained(sid, 1, 0, 0)` 으로 종속을 찾아 전역 후처리를 부른다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,252개 통과**(Assets 363 + Core 889; 이번 +8).

| 검사 | 내용 |
| --- | --- |
| `damageablepredestroy` 10.78 몫 1,632행 재생 | "D" 절 816행은 Damageable 직접 호출, "C" 절 816행은 Carrier 경유로 부른다. 매 행마다 외부 호출 순서·슬롯 50바이트 ×5·현재 권한/타입/사제 kind 전역·공통 사전 검사 횟수까지 전부 일치한다 |
| 합성 7건 | 미연결 하위 효과·spot 지도 크기 오류·빈/내용물 상태·자산 밖 타입·타입 표 밖 번호·NaN·지도 밖 좌표·순환 체인·권한 없는 dead 부모를 효과 전에 거부하고, abstract·매림이면 좌표를 읽지 않고 공통 본체까지 전달하는 것을 확인한다 |

`damageablepredestroy-x86.tsv` 는 총 4,896행(3 판본 × D/C 2절 × 816행)이며 10.78 몫 1,632행만 재생했다.
`originalCD`(CD 10.72)·`original1037` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

10.78 몫에서 실제로 잡히는 경로를 세면 붕괴(`C:`) 320행·폭발(`X:`) 64행·사제 해방 소리(`F`) 56행·
해방 몸체(`R:`) 20행·캐리어 종속 후처리(`T`) 432행·공통 본체(`B:`) 1,172행이다.

**변이 확인:** 구현 14곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 내용물 커서의 타입 필터 반전 | 원본 재생 |
| 내용물 커서의 head 대신 next 에서 시작 | 원본 재생 |
| spot 안쪽/통행 불가 마스크 반전 | 원본 재생 |
| 내용물 조회에 죽은 부모 허용 제거 | 원본 재생 |
| 해방 금지 플래그(`0x800`) 무시 | 원본 재생 |
| spot 좌표 편향 `0.9999` → `1.0` | 원본 재생 |
| 캐리어 권한(boss) 검사 제거 | 원본 재생 |
| 붕괴 비트 `0x200000` ↔ 폭발 비트 `0x100000` 교환 | 원본 재생 |
| abstract·매림 진입 조건 무시 | 원본 재생·합성 |
| 사제 타입 비교 반전 | 원본 재생 |
| raw 상태의 contained 비트 검사 제거 | 합성 |
| 사전 검사의 abstract·매림 좌표 우회 제거 | 합성 |
| dead 부모 조회 assert 제거 | 합성 |
| 캐리어 하위 효과 누락 검사 제거 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **`FindContained` 의 kind·mask 조건은 이 fixture 로는 구별되지 않는다.** 캐리어 경로가 부르는 값이
  `mask=1, kind=0` 이고 대상 후보들의 mask WORD 가 모두 1 이라 조건을 없애도 결과가 같다
  (변이 검사에서 확인함). 구현은 원본 규칙 그대로 옮겼다.
- **raw 상태의 contained 비트 거부는 10.78 행에 없다**(대상 슬롯의 상태 바이트가 0/2/4/6 뿐이다).
  합성 검사가 이 분기를 잡는다.
- Damageable **사후** 파괴(드롭·섬 정리·주변 HP 예약·다리 약화)와 **해방**(release) 몸체,
  Damageable **피격**(hit), 사제 경계, 내용물 조회(`contained`)·내용물 수명, Form 수명, 신전 삭제는 아직 옮기지 않았다.
- `RawDamageablePostDestroy` 는 HP·이웃·프레임·보상 몸체를 함께 요구하므로 다음 조각으로 미룬다.
- CD 10.72·10.37 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

Damageable 사후 파괴·해방(`damageablepostdestroy`·`damageablerelease`) → Damageable 피격(`damageablehit`) →
종속 조회·내용물(`contained`·`contentlifetime`) → Form 수명(`formlifecycle`·`formtake`·`form-packet`) → 신전 삭제 →
파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
