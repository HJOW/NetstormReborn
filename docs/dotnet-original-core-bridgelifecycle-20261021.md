# 원본 월드 코어 W03 둘째 조각 — 다리 삭제 연결(RawBridgeLifecycle)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 둘째 조각이다.
첫째 조각의 공통 삭제 본체(`RawSquidDestroy`)에 다리 사전·사후 파괴(`RawBridgeLifecycle`, 원본 00422290·004221b0·00422300)를 이었다.
근거는 커밋된 cpppj 소스(`RawBridgeLifecycle.cpp`·`RawBridgeLifecycle.h`·`RawSquidDestroy.cpp` 장면)와
원본 fixture(`destroy-x86.tsv` Bridge 행)뿐이다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다.
`cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다.
실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawBridgeLifecycle.cs` (신규) | `RawBridgeLifecycle`(`LinkNeedsDestroy`·`PreDestroy`·`PostDestroy`·`DelayedFallPayload`·`MakeHooks`), `BridgeLifecycleEffect`·`BridgeLifecycleEvent`·`BridgeLifecycleHooks`·`BridgeLifecycleSearch` | `RawBridgeLifecycle.cpp`·`RawBridgeLifecycle.h` 의 탐색 사각형·ftol·참조 판정 그대로 |
| `X86RawDestroyTests.cs` (확장) | 장면에 다리 모드(다리 가상 표·연결 타입·죽음 종속·참조·찾기·다리 훅) 추가, `BridgeMatchOriginalX86` 1건 | `RawDestroyTests.cpp` 의 다리 장면(`bridgeMode`)·`EmitBridge`·`Check` 열과 같다 |
| `RawBridgeLifecycleTests.cs` (신규) | 합성 4건(지연 페이로드 5값·연결 판정·훅 거부·범위 밖 번호/타입) | `BridgeLifecycleTests.cpp` 의 경계 의미를 10.78 경로로 옮김 |

사전은 일반(매립·추상 아님) 다리만 주변 3×3 칸에서 끊긴 연결(지정 타입)을 찾아 삭제를 알리고 공통 사전을 알린다.
사후는 제거 알림·추락 소리·같은 칸 보행자 추락을 순서대로 알리고 공통 사후를 알린다.
매립·추상은 탐색을 건너뛰고 공통 사전·사후만 알린다. 기준 판본은 10.78 뿐이다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,234개 통과**(Assets 363 + Core 871; 이번 +5).

| 검사 | 내용 |
| --- | --- |
| 64행 재생 | `Bridge` 10.78 몫 **64회**(서버 풀·역할 0~1·갈래 0~3)가 효과 문자열·슬롯 50바이트·13개 숫자까지 전부 일치한다 |
| 합성 4건 | 지연 페이로드 비트(파이썬 struct 독립 계산)·연결 끊김 판정·훅 미연결 거부(풀 불변)·범위 밖 번호·넘치는 타입을 확인한다 |

**변이 확인:** 구현 2곳을 하나씩 망가뜨려 모두 실패로 잡혔다(사전 탐색 왼쪽 한 칸 축소 — 48행 불일치로 실패,
페이로드 y 묶음 8→7비트 — 합성 1건이 불일치로 실패). 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**고친 검사 오류 1건:** 다리 장면에 비상황 종속 사슬(+6 머리) 구성을 그대로 둬서 갈래 3 행 16개가 종속 상태 오류로 실패했다.
원본 장면(`!isBridge` 조건)을 따라 다리 모드에서는 사슬 구성을 건너뛰도록 바로잡았다. 소스 변경이 아니다.

## 3. 이번에 하지 않은 것 (경계)

- `SquidDestroyLifecycle`(Pre·Post 수명주기·장부·보상·선택)·`destroylifecycle`(Pair 384·Destroy 192)·`destroygraph`(Pre·Post·Destroy 각 192) 재생.
- `Damageable`(피격·사전/사후 파괴·해방)·종속 조회·내용물·Form 수명·신전 삭제, `bridgeeffects` 전체(W06 다리 효과) 재생.
- `DelayedFallPayload` 의 실제 사건 0x2692 연결(페이로드 계산만 이식, 사건 분배는 W06·W13).
- CD 10.72·10.37, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

`SquidDestroyLifecycle`(Pre·Post)·`destroylifecycle` 재생(`Pair` 384·`Destroy` 192) → `destroygraph` 전체 재생(Pre·Post·Destroy 각 192행) →
Damageable·내용물·Form·신전 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
