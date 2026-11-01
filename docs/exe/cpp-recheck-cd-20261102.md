# RecheckCD 현재 serial 정리 연결 — 단위696

2026-11-02, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 시작했다. 마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-10-23이며 현재 PC가 같아 재디컴파일은 필요 없었다. AGENTS.md(SHA `544643cf…`)는 수정하지 않았다.

**실행 제품696/11.01에 RecheckCD 이름·메타·큐 명령을 연결했다.** 원문 기계어 332입력/996관측(차이0), 집중 검사4개(실패0), 변이7종 모두 검출, 메인 메뉴 실제 실행 최종1회 통과다. 이 단위는 695의 "재개 시 다음 메타" 항목이다.

## 원문이 하는 일 (10.78)

`RecheckCD`의 이름표는 `00543370`이며 `00542ae8` 전역(초기값 `0x020000fd`)을 가리킨다. 일반 명령 분배기 `004d62b0`의 체인 `004d8b85..004d8b9e`는 다음과 같다.

| 순서 | 원문 | 의미 |
| --- | --- | --- |
| 0 | 뗌(최상위 비트) | 명령 체인 전체를 건너뛴다. 아무것도 읽거나 쓰지 않는다. |
| 1 | `push 1; call 00435060` | 상수 `1`을 돌려주는 함수다. 호출자가 결과를 쓰지 않으므로 효과가 없다. |
| 2 | `push 005c85ec; call 004c8d90` | 현재 serial을 `00464020`으로 조회한다. 있으면 `00466480`(공통 Kill)한 뒤 **항상 serial에 0을 쓴다**. |

`005c85ec`는 바이너리 전체에서 `push 5c85ec`(4d8b95) **단 한 곳**만 직접 참조한다. 즉 10.78에서 이 값을 0이 아닌 값으로 쓰는 코드가 없다. 또한 모든 Gump는 생성자에서 `serial=++state.serial`로 0이 아닌 고유 serial을 받는다(`GumpCreate.cpp:25`). 따라서 **실제 게임에서 RecheckCD는 최초 serial 0으로는 어떤 Gump도 Kill하지 않는 사실상 무효과 명령**이다. 조회는 serial 0도 그대로 수행하므로(`464020(0)`은 serial 0인 살아 있는 첫 Gump를 찾는다) 이를 임의로 건너뛰지 않았다.

실제 CD 탐색(`00440c60`)이나 종료된 온라인 검증은 이 분기에 없고 연결하지 않았다. 684의 외부 접속·검증 비활성 정책과 충돌하는 부분이 없다.

### 10.72(CD) 비교

CD판 `NETSTORM.EXE`도 같은 모양이다. 이름표 `00513458`→`00512c70`, 분기 `0041cdd9..0041cdfc`는 `push 1; call 00433bf0` 뒤 `push 00518a5c; call 004c3820`이다. 두 가지가 다르다.

- 첫 호출 `00433bf0(1)`은 상수가 아니라 상태(`00564e20`, `00564eb8/ebc`)와 시간 값을 쓰는 함수다. 10.78의 `00435060`이 이를 상수 1로 줄인 것으로 보인다. 10.72 복원 때 이 함수를 따로 대조해야 한다.
- serial 전역은 `00518a5c`이고 `004c3820`은 `004c8d90`과 같은 모양(조회 `00492490`→Kill `00493a40`→0쓰기)이다.

## 구현

| 파일 | 내용 |
| --- | --- |
| `client/RecheckCdInput.{h,cpp}` | `kRecheckCdCommand=0x020000fd`, `DispatchRecheckCdInput`. 뗌 소비 → 현재 serial 조회(0 포함) → 있으면 Kill → 항상 0쓰기. Kill 예외는 0쓰기 앞에서 전파한다. 필수 장치가 없으면 빈 성공 대신 명시 오류다. |
| `client/ClientCdGlobals.h` | `RecheckSerial()` — 005c85ec를 Client 수명 동안 최초 BSS 0으로 보관한다. |
| `client/GameMenuCommands.cpp` | 전역 `00542ae8`→명령 값, 이름 `recheckcd`(대소문자 무시)를 이름표/메타/큐 표에 등록. |
| `client/UberGump.cpp` | `Event`에서 `SerialGump`(464020)/`KillGump`(466480)에 연결. 뗌이 아닌 명령은 `rebuild_`로 소비한다. |

## 검증

**원문 동결 (`tools/decomp_recheck_cd.py`, 제품 변경 전 실행)**
실제 `004d62b0` 분기와 `00435060`·`004c8d90`·`00464020`의 기계어를 Unicorn으로 그대로 실행하고 `00466480`만 대상 객체 번호를 기록하는 명시 경계로 뒀다. 332입력/996관측(두 x87 정밀도·반복), 차이 0이다. 제품 변경 뒤 도구의 반복문 주석만 보태고 다시 실행해 fixture가 바이트 단위로 같음(SHA `dcc1a2f2…`)을 확인했고, 보고서의 도구 해시는 이 최종 파일 기준이다.
- serial 4종 × 등록 배치 11종 = 44입력: 첫 일치만 Kill, dead(0x10000)는 제외, 다른 플래그 비트는 영향 없음, 마지막 조회 칸(998)은 Kill·범위 밖 칸(999)은 조회 안 함, serial 0은 살아 있는 serial 0 객체를 Kill.
- 입력 변형 288입력(뗌/수정키/첫째·둘째 인자/Editor·missionPost 모드)은 결과에 영향이 없다. 뗌이면 serial 쓰기 0회이고 아니면 1회다.
- 원문 Kill은 105행(그중 serial 0 Kill 50행)이다. 근거는 `cpppj/recheck-cd-native-20261102.json`·`cpppj/tests/fixtures/recheck-cd-x86.tsv`다.

**C++ 집중 검사 `netstorm_recheck_cd_tests`: 4개/실패0**
(1) 332행을 실제 제품 `FindCurrentGumpSerial`로 재생해 Kill 대상·serial 최종값·쓰기 횟수를 대조, (2) 필수 장치 누락·Kill 예외 시 serial 보존, (3) 이름·HTML 메타·실제 큐 경로, (4) 실제 `Client`의 장기 owner가 0으로 시작하고 같은 주소를 유지. 최초 빌드·실행부터 통과했고 빌드 경고/오류는 0이다.

**변이 7종 모두 검출 (`tools/cpp_mutation_check.py --focused --test-target netstorm_recheck_cd_tests`)**
뗌에서도 실행(CHECK 242개 실패), serial 0 조회 생략(51), 항상 0 조회(63), 0쓰기 제거(261), 이름표(3)·명령표(1)·owner 초기값(1) 변경. 저장소 소스는 바꾸지 않고 작업 사본에서 실행했다.

**실제 실행 (`tools/cpp_recheck_cd_smoke.py`, 1024×768 창, 메인 메뉴 전용)**
최종 제품 SHA `13a2a7d0ccd66336463ea94a241759c7b337b7eb3f1a3e6d690c0018bf7f9089`로 `live-v3`가 통과했다.
- 최초 serial 0에서 `RecheckCD`/`RecheckCD,0,1`/`recheckcd` 3회 입력: 등록 항목 184 유지, `recheck_cd` 계속 0, 표시 RGB 완전 동일 → 원문대로 무효과.
- 실제 `BeginCapture` 모달을 연 뒤(`current_dialog` 216, 등록 180) 검사 전용 연산으로 그 모달의 실제 serial(202)을 005c85ec에 넣고 `RecheckCD` 1회: 모달이 Kill/프레임 정리로 반납되어 `current_dialog` 0, `recheck_cd` 0, 메인 페이지 복귀, 등록 184, 표시 RGB가 처음 메인 화면과 동일.

실행은 **총3회, 모두 종료0**이다. 앞의 두 실행(`live-v1`, `live-v2`)은 제 검사 스크립트의 기대가 틀려 단언에서 멈췄다. `live-v1`은 "serial 0이면 첫 Gump가 Kill돼 항목 수가 줄어든다"고 가정했으나 위 생성자 때문에 틀렸고, `live-v2`는 "모달이 열리면 항목 수가 늘어난다"고 가정했으나 메인 컨트롤이 교체되어 줄었다. 제품 동작은 두 실행 모두 의도대로였다(`live-v2`의 armed/killed 보고가 최종과 같다). 기대를 결과에 맞춰 고친 근거는 각각 위 두 사실이다. 원시 기록은 `extracted/cpp-recheck-cd-696/`에 모두 남겼다.

**보호:** 원본 1394 + 원본CD 425 + 기존 fixture 796 = 2615개 변경 0을 재해시로 확인했다. 새 fixture 1개로 현재 797개이고 지침을 포함하면 2617개다. AGENTS.md는 변경 없음(`544643cf…`). 세 실행의 전후 보호 스냅샷이 모두 같고 설정 파일은 복원했다.

## 검사 전용 장치 (원본 동작이 아님)

- ui-script `command <이름[,첫째[,둘째]]>`: 게임의 이름→명령 표로 일반 명령을 실제 입력 큐에 넣는다.
- ui-script `test-recheck-serial current-dialog`: 열린 모달의 실제 serial을 005c85ec에 넣는다. 원문에는 이 값을 쓰는 다른 지점이 없으므로 Kill 경로를 확인하기 위한 검사용 입력일 뿐이다.
- `UberGump::CurrentDialogSerial()`(읽기 전용)과 검사 보고의 `recheck_cd` 줄.

## 한계

- 실제 게임 안에서 005c85ec를 0이 아닌 값으로 쓰는 경로가 없으므로, Kill 경로의 GUI 확인은 검사 전용 입력으로 한 것이다.
- 전체34 진입·전체 C++·새 ZIP·물리 입력·PCM·멀티플레이 검사는 하지 않았다. 포획/희생·특정 적 공격·승리·공중3종 전용 검사도 하지 않았다.
- 10.72의 `00433bf0`(시간 기반 CD 검사)는 대조하지 않았다.
- `RequestMenu`(4d712e..4d717c, 서버 전송)는 3차 멀티플레이 정적 사전 구현에 남긴다.

다음은 LEFT_JOBS.md 3절의 나머지 Edit/시나리오/수명/호환성 항목이다.
