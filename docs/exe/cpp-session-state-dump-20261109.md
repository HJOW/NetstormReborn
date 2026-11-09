# cpppj 실제 Session 상태 덤프 (`session-dump`)와 고정 시각 실행 (`--fixed-frame-ms`)

2026-11-09~10, HJOW-Athlon. webpj([LEFT_JOBS.webpj.md](../../LEFT_JOBS.webpj.md) 3.5절 1번)와 dotnetpj([이식 계획](../dotnet-cpp-port-plan-20261016.md) 8절 요청 1·2)가 요청한 **L2(시작 상태)·L3(같은 입력 단계표로 진행한 상태) 대조용 cpppj 관측**이다. 둘 다 원본 게임에는 없는 검사 전용 기능이며, 지정하지 않으면 기존 동작이 바뀌지 않는다.

## 1. 사용법

`--run` 실행의 UI 스크립트(`--ui-script steps.tsv`)에 단계를 넣는다. 형식은 다른 단계와 같은 `프레임⇥동작⇥인자`다.

```text
5	click-if	Play Mission
...
90	session-dump	C:\out\thewarbegins-090.tsv
```

```powershell
# 결정론 비교용 권장 실행(가상 시계 25ms/프레임). 창이 뜨므로 AGENTS.md 실행 확인 규칙(예외 호스트 포함)을 따른다.
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj\build\bin\Release\Netstorm.exe --run assets\game-data --mission thewarbegins --fixed-frame-ms 25 --window --mute-audio --frames 250 --set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0" --ui-script steps.tsv --ui-report report.tsv
```

- `session-dump`: 그 프레임의 **그리기 뒤**(`snapshot`·`report`와 같은 afterFrame 시점)에 현재 상태를 인자 경로에 쓴다. 인자가 비면 오류다. 이 단계는 입력 큐에 아무것도 넣지 않는다. 미션 월드가 없으면 `world 0` 덤프(머리·난수·Kernel까지)를 쓴다. Session 선택은 입력과 같은 `UberGump::InputSession()`(현재 GameWorld 또는 준비된 MetaMap Session)이며 공개 경로는 `UberGump::SessionStateDump()`다.
- `--fixed-frame-ms N`(1~60000): Client가 읽는 OS 밀리초(`timeGetTime`)와 유닉스 초(`time32`)를 가상 시계로 바꾼다. 가상 시계는 **메인 루프 한 바퀴마다, 그리고 메인 루프 밖 대기 펌프(`Client::PumpMessages`, 시작·타이틀 그림)마다** N ms씩만 전진한다. maxFPS 바쁜 대기는 건너뛴다(가장 빠르게 진행). 가상 원점은 timeGetTime 1,000,000ms, 유닉스 초 946684800(2000-01-01 UTC)이며 `--fixed-unix-seconds S`로 유닉스 초만 바꿀 수 있다(고정 모드에서만 허용).
- 구현: [ClientTimeSource](../../cpppj/src/client/ClientTimeSource.h)(시각 단일 공급원)·[SessionStateDump](../../cpppj/src/client/SessionStateDump.h)·[main.cpp](../../cpppj/src/app/main.cpp)의 옵션/UI 스크립트 연결. 기본 모드는 이전과 같은 `timeGetTime()`/`std::time()`을 그대로 읽는다.
- MCP 도구로는 아직 노출하지 않았다(필요하면 같은 `UberGump::SessionStateDump()`를 MCP 도구에 연결한다).

## 2. 형식 (판번호 1)

ASCII·LF·탭 구분이다. 숫자는 별도 표시가 없으면 10진 부호 없는 정수이고, float/double은 **비트 그대로의 정수**(float는 uint32, double은 uint64)로 쓴다. 행 순서는 아래 순서로 고정이다.

| 행 | 열 | 뜻 |
| --- | --- | --- |
| `netstorm_session_dump` | 판번호 | 형식 판번호(현재 1). 열 의미가 바뀌면 올린다 |
| `edition` | `originals`/`originalCD` | 판본(fixture 첫 열과 같은 이름). 월드가 있으면 풀의 판본 |
| `frame` | 프레임 번호, 게임 시각 double 비트 | `Client::Time().number`·`.game` |
| `random` | 게임 난수 상태, CRT holdrand | 00532710의 `GameRandom` 상태와 CRT 상태 DWORD |
| `world` | 0/1 | 실제 Session이 있으면 1 |
| `kernel` | 등록 수, 끝 번호 | 현재 Kernel |
| `process` | pid, Form SID, 부모 SID | pid 오름차순. SquidProcess가 아니면 SID 칸은 0 |
| `regular` | pid, 사건, 반복 수, payload float 비트, 다음 시각 double 비트 | 바로 앞 `process`가 RegularProcess일 때만 |
| (이하 `world 1`일 때만) | | |
| `pool` | 크기, stride, free 수, 첫 free(서버), 첫 free(client), 예측 커서, FNV-1a 64 해시(16진) | 해시는 풀 **전체 원문**(free 슬롯 잔여 포함)이다 |
| `slot` | SID, 원문 16진(stride 바이트) | 예약 0~4번과 free 비트(+11 &1)가 없는 슬롯 전부. 원문에 TYPE·상태·소유자·프레임·좌표·HP·링크·가상 표가 모두 들어 있다 |
| `object` | SID, TYPE, 상태 BYTE, 소유자, 프레임, x float 비트, y float 비트, HP | `slot` 중 TYPE ≥ 70(자산)을 판본별 폭으로 해석한 읽기용 행. 10.78: 소유자 +34·프레임 +36 DWORD·HP +26 DWORD, 10.72: 소유자 +32·프레임 +34 BYTE·HP +26 WORD |
| `player` | 번호 0~8, 원문 16진(180바이트) | Player 9기록 |
| `hidden_players` | 16진 9바이트 | Chat 숨김 BYTE |
| `alliances` | 81개 DWORD | 방향별 9×9 동맹 표(행 우선) |
| `sp` | 초기화 0/1, offset, 소유자 0~8의 SP float 비트 | 분산 SP 저장소. 미초기화면 값 칸은 `-`(덤프가 초기화를 일으키지 않는다) |
| `graph` | 번호, surfaces, inUse, reserved | Graph 표 251칸 전부(예약 0번 포함, signed WORD) |
| `totals` | 총 비용(signed), postPop 깊이, 생산 재계산 알림 | 장부 |
| `totalmade` | TYPE, 로컬, 누적 로컬, 전역 | TotalMade 세 표에서 하나라도 0이 아닌 TYPE만 |
| `end` | — | 끝 표시. 중간에 실패하면 이 행이 없다 |

## 3. 결정론과 비교할 때의 주의

**실제 실행 확인(2026-11-10, 캠페인 1-1 `thewarbegins`, 같은 단계표 두 번, 프레임 90·240 덤프):**

| 시계 | 두 실행의 차이 행 |
| --- | --- |
| 기본 OS 시계 | `frame`(게임 시각)·`random`·`regular` 25·`pool` 해시·`slot` 259~275·`object` 244~258·`sp`·`graph` 1. 미션 시작 게임 시각도 9.491초/9.479초로 달랐다 |
| `--fixed-frame-ms 25` | **`sp` 한 줄(소유자 0)만 다름.** 미션 시작 9.2초·풀 해시·슬롯 1941/1943·객체 1881·Kernel·Player·Graph·TotalMade 모두 같음 |

- **`sp` 소유자 0 칸:** SP를 쓴 적이 없는 소유자의 분산 배열은 원본처럼 미초기화 힙 내용(`operator_new(0x24)`, cpppj `CurrentAllocationBytes`)을 그대로 담으므로 실행마다 다르다. 비교는 SP를 설정한 소유자(미션 참가자)만 한다.
- **고정 시각에도 남는 실시간 원천:** `std::clock()`(CRT 프로세스 시각)을 읽는 로그인 `clock` 장치·동맹 입력·게임 시간 표시(`GameTimeDisplay`)는 바꾸지 않았다. 위 1-1 240프레임 비교에서는 결과에 나타나지 않았으나, 해당 경로(시간 표시·동맹 입력 등)를 비교할 때는 따로 확인한다. 소리 장치 스레드 시각도 비교 대상이 아니다.
- **OS 창 메시지:** 실제 마우스가 창 위에서 움직이면 OS 사건이 입력 큐에 들어갈 수 있다. 비교 실행 중에는 창 위에서 마우스를 움직이지 않는다(UI 스크립트의 클릭/포인터는 같은 실제 입력 큐를 쓴다).
- **같은 조건:** 같은 자료·언어·화면·설정(`--set`)·고정 시각 값·프레임 단계표에서 얻은 덤프끼리만 비교한다(webpj 3.3절 "같은 조건").
- **cpppj 고유 칸:** `process`의 pid는 원본과 같은 Kernel 슬롯 규칙이지만 C++ 파생 클래스 이름은 쓰지 않는다. Player 원문 가운데 원본에서 포인터였던 칸(예: factory 배열·AI brain 주소)은 cpppj가 `ClientPlayerRecords`의 opaque32 식별자(호스트 포인터를 절삭하지 않은 보관 블록 번호)로 보관하므로, 웹/닷넷은 같은 식별자 규칙을 옮기기 전까지 그 칸을 별도로 다룬다.
- **free 슬롯:** 원문 전체는 `pool` 해시로만 비교한다. 해시가 다르면 웹/닷넷 쪽에서 같은 형식의 전체 슬롯 목록을 따로 만들어 차이 SID를 찾는다.
- **범위 밖:** 공간 해시(`SquidHash`)·spots·Kencloud·화면 상태·소리·GameAuxState 장기 전역·AI brain 본체(0xb08)는 이번 판에 넣지 않았다. 필요해지면 판번호를 올려 행을 추가한다. 화면은 기존 `snapshot`, 메뉴/세션 관측은 기존 `report`를 함께 쓴다.
- **오라클 기준선:** webpj의 저장 오라클 실행 파일(`extracted/webpj-oracle/1f00a25a`)은 기준선 빌드라 이 두 기능이 없다. 이 기능을 쓰려면 이 커밋 이후 cpppj로 오라클을 다시 빌드·수집해야 한다(webpj 3.6절 재기준선 판단 대상).

## 4. 검증

- 단위 시험(`netstorm_pop_context_tests`, 통합 `netstorm_tests`에도 등록): `SessionStateDumpTests.cpp` 4건(순수 서식·분산 SP 복호가 `ScrambledSpStore::Get`과 같은 비트이며 회전 뒤 유지·FNV-1a 64 공개 시험값·판본별 필드 폭·free 슬롯 생략·world 0) 통과, `ClientTimeSourceTests.cpp` 2건(기본 OS 시계·고정 모드 전진/감김/유닉스 초/거부) 통과.
- 실제 실행(HJOW-Athlon, AGENTS.md 예외 호스트): 메인 메뉴 `world 0` 덤프, 1-1 미션 덤프(위 3절 표). 고정 시각 모드 소요는 메뉴 61프레임 1.0초, 미션 61프레임 2.4초였다.
- 최초 실패와 교정: ① `main.cpp`에서 private `UberGump::InputSession()`을 직접 불러 C2248 → 공개 `SessionStateDump()` 추가. ② 시험 파일의 지역 `Throws` 누락 C3861 → 다른 시험 파일과 같은 지역 도우미 추가. ③ 고정 시각 첫 판은 가상 시계를 메인 루프에서만 전진시켜, 시작 타이틀 그림의 벽시각 대기(`TitleBitmapLifetime`의 `for(;;){ if (end<=wall()) break; pump(); … }`)가 실제 OS 입력이 들어올 때까지 돌아 메뉴 시작 28.6초·미션 6분 이상/시간 초과가 났다 → `PumpMessages()`마다 가상 시계를 한 프레임 전진하도록 고쳐 결정론과 속도를 함께 맞췄다.
