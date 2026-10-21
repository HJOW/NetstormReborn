# 원본 월드 코어 W03 셋째 조각 — 공통 삭제 수명주기(RawSquidDestroyLifecycle)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 셋째 조각이다.
첫째 조각의 공통 삭제 본체(`RawSquidDestroy`)와 둘째 조각의 다리 삭제 연결(`RawBridgeLifecycle`) 사이에 있던
공통 pre/postDestroy 수명주기(원본 `SquidDestroyLifecycle`, 004b0950 ↔ CD 004add20 / 004b0840 ↔ CD 004adbe0)를 이었다.
근거는 커밋된 cpppj 소스(`SquidDestroyLifecycle.cpp`·`SquidDestroyLifecycle.h`)와 원본 fixture(`destroylifecycle-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawSquidDestroyLifecycle.cs` (신규) | `RawSquidDestroyLifecycle`(`ValidatePre`·`ValidatePost`·`PreDestroy`·`PostDestroy`·`Hooks`), `SquidDeletionEffect`·`SquidDeletionEvent`·`SquidDeletionHooks`·`SquidDeletionState` | `SquidDestroyLifecycle.cpp`·`.h` 의 선택 해제→그래프 분할→보상→통계→소리·좌표→공급원/작업장 순서와 비용 인코딩 그대로 |
| `OriginalTypeRecord.cs` (확장) | 타입 그룹 번호 `Group`(기본 10 = 그룹 없음)과 값 복사 `Clone()` | `RiftTypeRecord.group`(+0x9c, 기본 `kTypeNoGroup`=10). 수명주기가 타입 입력을 고정하는 값 복사를 대신한다 |
| `X86RawDestroyLifecycleTests.cs` (신규) | `Pair`·`Destroy` 10.78 재생 2건 | `DestroyLifecycleTests.cpp` 의 `Scene`·`Run`·`Check` 열과 같다 |
| `RawDestroyLifecycleTests.cs` (신규) | 합성 4건(생성/삭제 통계 공유·거부 경계·억제와 비용·풀/자산 밖 루트) | `DestroyLifecycleTests.cpp` 의 합성 4건과 같은 구성(10.78) |

수명주기는 다음 순서를 지킨다.

- 사전(`PreDestroy`): 선택 해제(배치 타입이면 배치 좌표 기억) → 그래프 분할/반납 → 소유자별 작업장 목록 제거 →
  삭제 보상 알림 → 로컬/전역 현재 수 감소 → 잃어버린 소리 알림과 좌표 기록 → 공급원·작업장 목록 제거와 재계산 알림 → 깊이 감소.
- 사후(`PostDestroy`): 주변 표면 다시 추가 → ordinary 비용 차감 → 깊이 감소.

사전 통계는 억제(`Suppressed`)하면 생략하지만 사후 비용 차감은 억제와 무관하다. 잃어버린 소리는
타입 그룹이 "없음"(10)이 아니고, 보상 금지·그래프 파괴 플래그가 없고, 소유자가 로컬 소유자와 같고,
종류 소리 제외 비트가 껐고, 두 억제 전역이 모두 꺼졌을 때만 난다. 비용은 패치판 인코딩(타입 번호 × 23 편향)을
원본처럼 걷어낸 뒤 전역 집계에서 빼고 0 방향 절삭과 DWORD 감김을 보존한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,240개 통과**(Assets 363 + Core 877; 이번 +6).

| 검사 | 내용 |
| --- | --- |
| `Pair` 10.78 몫 384회 재생 | 공통 사전·사후를 직접 부른 384행이 효과 문자열·슬롯 50바이트·풀/해시/점유/삭제기록 체크섬·장부 18칸·좌표 5칸까지 전부 일치한다 |
| `Destroy` 10.78 몫 192회 재생 | 공통 삭제 본체(004af780)를 통해 삭제 본체가 내보내는 효과 문자열·슬롯·체크섬·장부가 전부 일치한다 |
| 합성 4건 | 생성은 현재 수와 비용을 되돌리고 누적 생산 수만 남긴다(재계산 알림 4회)·그래프/AI/손상 목록/누락 콜백은 장부 쓰기 전에 거부한다·억제 상태에서도 사후 비용은 차감한다·다른 풀과 자산 밖 루트는 거부한다 |

`destroylifecycle-x86.tsv` 는 총 1,728행(Pair 1,152 + Destroy 576)이며 10.78 몫 576행만 재생했다.
`originalCD`(CD 10.72)·`original1037` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).
비용 규칙은 10.78 몫뿐 아니라 CD 몫까지 별도 파이썬 계산(1,728행 전체)으로 대조해
`decoded = trunc(cost + type×23) - type×23`(패치)와 `decoded = cost`(CD)를 쓰고 마지막에 0 방향 절삭·DWORD 감김을 적용하는
원본 규칙이 두 판본 모두에서 일치함을 확인했다.

**변이 확인:** 구현 10곳을 하나씩 망가뜨려 모두 실패로 잡혔다
(목록 압축 무시·비용 편향 무시·전역 통계 감소 누락·그룹 판정 무시·재계산 알림 누락·소유자 작업장 목록 제거 누락·
보상 알림 누락·무음 타입 판정 무시·로컬 통계 감소 누락·배치 타입 미기록). 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `destroygraph` 전체 재생(Pre·Post·Destroy 각 192행)과 섬 H 치환 24행.
- `Damageable`(피격·사전/사후 파괴·해방)·종속 조회·내용물·Form 수명·신전 삭제, `bridgeeffects` 전체(W06 다리 효과) 재생.
- 삭제 보상(SP·지갑·샘 풀) 계산: 원본 C++ 수명주기는 `SquidReward` 연결을 받지만 이식에는 바깥 보상 알림까지만 옮겼다.
- 그래프 분기(사전 분할·사후 주변 추가)는 `RawGraph` 연결 경로만 마련했고, 이 조각의 재생 장면은 그래프를 끈 상태다.
- CD 10.72·10.37 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

`destroygraph` 전체 재생(Pre·Post·Destroy 각 192행) → Damageable·종속 조회·내용물·Form·신전 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
