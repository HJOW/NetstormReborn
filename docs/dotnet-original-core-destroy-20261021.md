# 원본 월드 코어 W03 첫째 조각 — 공통 삭제 본체(RawSquidDestroy)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 첫째 조각이다.
W02 에서 끝낸 `RawGraph` 의 삭제 후처리(`PostDestroy`·`Frame`)에 이어, 삭제 수명주기의 가장 안쪽 본체인
공통 삭제(`RawSquidDestroy`, 원본 004af780)를 옮겼다. 근거는 커밋된 cpppj 소스(`RawSquidDestroy.cpp`·`RawSquidDestroy.h`)와
원본 fixture(`destroy-x86.tsv`)뿐이다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다.
`cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다.
실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawSquidDestroy.cs` (신규) | `RawSquidDestroy`(죽음 표시→종속 끊기·죽음 알림→전송·선택 알림→Unpop→후 알림→반납, `CompletePreDestroy`·`CompletePostDestroy`·`PreDepth`·`PostDepth`·`ValidateGraph`), `SquidDestroyEffect`·`SquidDestroyEvent`·`SquidDestroyHooks` | `RawSquidDestroy.cpp`·`RawSquidDestroy.h` 의 `Destroy`·완료 호출·깊이·풀 검사 그대로 |
| `X86RawDestroyTests.cs` (신규) | `destroy-x86.tsv` 10.78 Destroy 600행 재생(효과 문자열·슬롯 50바이트·풀 카운터·머리·꼬리·체크섬·고른 번호·깊이) | `DestroyGraphTests.cpp`·`RawDestroyTests.cpp` 의 장면(풀 7슬롯 확보·Take·가상 표·좌표·해시 체인·갈래별 종속 사슬·효과 안 풀 직접 수정)과 `Check` 열 |
| `RawDestroyTests.cs` (신규) | 합성 경계 4건(죽음 재진입·완료 누락·권한/풀/뿌리 거부·종속 불량) | `RawDestroyTests.cpp` 의 경계 4건(`DeadPrevents…`·`RejectsMissing…`·`ValidatesAuthority…`·`RejectsInvalid…`) |

동작은 원본 순서를 그대로 따른다. 이미 죽은 번호는 바깥 효과 없이 돌아오고, Form 뿌리(타입 70 미만)는
`UnpopForm` 으로, 자산 뿌리는 `RawUnpop` 으로 푼다. Pre·Post 안에서 완료 호출을 빼먹으면 예외로 멈추고 뒷정리를 건너뛴다.
서버가 아닌 풀에서 서버 쪽 뿌리는 깃발 8이 없으면 거부하고, 서버 풀에서 클라이언트 쪽 뿌리는 깃발에 0x10 을 더한다.
다리(Bridge) 64행은 `RawBridgeLifecycle` 연결이 필요해서 다음 조각으로 남겼다. 기준 판본은 10.78 뿐이다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,229개 통과**(Assets 363 + Core 866; 이번 +5).

| 검사 | 내용 |
| --- | --- |
| 600행 재생 | `Destroy` 10.78 몫 **600회**(서버×역할×갈래)가 효과 문자열·슬롯 바이트·13개 숫자까지 전부 일치한다 |
| 합성 4건 | 죽음 재진입 무효과·Pre/Post 완료 누락의 그 자리 멈춤·권한/풀/빈·내용물·Form 뿌리 거부·종속 불량(빈·비내용물·순환)의 뿌리 죽음 유지를 확인한다 |

**변이 확인:** 구현 2곳을 하나씩 망가뜨려 모두 실패로 잡혔다(죽음 표시를 void 로 — 504행 불일치로 실패,
종속 파괴 알림의 종속 깃발 누락 — 176행 불일치로 실패). 소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `Bridge` 64행 재생(`RawBridgeLifecycle` 의 다리 사전 파괴·후 파괴 연결이 필요).
- `SquidDestroyLifecycle`(Pre·Post 수명주기·장부·보상·선택)·`destroylifecycle`(Pair 384·Destroy 192)·`destroygraph`(Pre·Post·Destroy 각 192).
- `Damageable`(피격·사전/사후 파괴·해방)·종속 조회·내용물·Form 수명·신전 삭제, CD 10.72·10.37, 파생 분배 허브·실제 세션 연결.
- `Deletions` 빈 기록 판정은 첫 기록 번호가 0인 것으로 본다(고정 20칸이라 비어 있음을 셀 수 없다).

## 4. 다음 조각

다리 삭제 연결(`RawBridgeLifecycle` + `Bridge` 64행) → `SquidDestroyLifecycle`(Pre·Post)·`destroylifecycle` 재생 →
`destroygraph` 전체 재생(Pre·Post·Destroy 각 192행) → Damageable·내용물·Form·신전 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
