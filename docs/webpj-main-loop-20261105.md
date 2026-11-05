# webpj 세션 장기 상태 3단계: 입력/예약 큐와 메인 루프 순서 — WB04-C3

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서2(WB04-C)의 세 번째이자 마지막 하위 단위다. 원본 10.78의 **40칸 입력 사건 큐**(명령 예약 포함)와 **메인 루프 한 바퀴의 단계 순서**를 옮겼다. 이로써 WB04-C(C1 난수·시계·Kernel, C2 보조 전역, C3 큐·순서)를 마쳤다. 앱/세션 연결과 화면 변경은 없다.

## 1. 근거와 범위

- 시작 트리 `e993112b`. 착수 때 `node webpj/tools/fixture-manifest.mjs --write`로 C2 항목(`original-pe-data`)의 `webCommit`을 `e993112b`로 갱신했다.
- 입력 큐: cpppj `src/client/InputEvent.h`(`225f2322`)·`.cpp`(`bfb780c4`), 원본 `00452f90` 넣기·`00452fe0` 명령 넣기·`00452f00` 꺼내기. 원본 관측 fixture는 없고 cpppj `tests/ScreenTests.cpp`의 `InputQueue_*` 세 검사를 옮겼다.
- 메인 루프: [docs/exe/main-loop.md](exe/main-loop.md) 2절의 **패치판 호출 순서 표**(원본 `FUN_00438dc0`)와 cpppj `src/client/ClientMain.cpp`의 루프(옮긴 단계·자리 표시). cpppj 루프는 갱신 목록 일부(상태 기계·커서·간헐천·제단 등)를 아직 다른 자리에서 처리하거나 옮기지 않았으므로, 단계 순서는 원본 문서 표를 기준으로 삼았다.
- **범위 밖:** 대화상자 명령 수명(cpppj `DialogCommandLifetime`·`dialog-command-lifetime-x86.tsv`)은 웹 메뉴(WB03)가 이미 다른 경로로 처리하며, 3.4절 결정대로 완료한 UI를 재복원 과제로 세지 않는다. 브라우저 키/포인터를 원본 코드로 바꾸는 창 프로시저(`00436550`/`00436020`)·폴링(`00452e00`)은 실제 미션 입력(WB03-M2/WB05 표시·입력)과 함께 옮긴다.

## 2. 사용 계약

### 2.1 `core/input-queue.js` — `InputQueue`, `InputCode`

- 40칸 고리, 39개까지 담고 가득 차면 **방금 넣은 사건을 버린다**. 사건은 `{code, x, y}`(코드 DWORD, 좌표 signed DWORD)이며 꺼낸 사건은 사본이다.
- `pushCommand(code, x, y)`: 코드를 `0x0100ffff`만 남기고 명령 표시 `0x02000000`을 붙여 넣는다(원본 명령 예약).
- `pop(skipReleases)`: 비었으면 0 사건. 글자 DEL(0x7f)은 Ctrl+Backspace(`0x10000008`, 원래 보조 키 비트 유지)로 바꾼다. `skipReleases`이면 뗌 사건도 큐에서 빠지지만 0 사건을 돌려준다(원본은 꺼낸 뒤 판단).

### 2.2 `core/main-loop.js` — `MAIN_LOOP_STEPS`, `MainLoop`

- `MAIN_LOOP_STEPS`: 0~13 단계와 10번 갱신 목록 20개를 원본 순서·패치판 주소·설명으로 둔 표다. 웹에 없는 단계는 사유와 함께 `excluded`다: 스레드 우선순위·커서 재배치/Sleep(OS), 시간 동기화·주기 내보내기·전투 생존 신호(네트워크), 메시지 펌프(브라우저 사건 처리기가 프레임 사이에 큐에 넣음), CD 확인(패치판 빈 함수), monitor·validateGrid(디버그). 4번 송수신기 폴링 자리는 cpppj처럼 **싱글/로컬 미션 패킷 진행** 자리로 남겼다.
- `new MainLoop({ handlers, movieUpdatePauseDepth })`: 처리기는 단계 id → 함수다. 표에 없는 id·제외 단계·함수 아님은 생성 때 거부한다.
- `runFrame()` → `{ran, missing, quit, updatesPaused}`. 처리기가 있는 단계만 표 순서로 실행하고, **없는 단계는 실행하지 않고 `missing`으로 보고**한다(빈 성공 함수로 채우지 않음).
- 영상 갱신 정지 깊이(원본 `DAT_0054dc58`, signed)가 1 이상이면 갱신 목록 중 `idleGumps`(00464170)만 실행하고 그리기·알림은 계속한다.
- `input` 처리기의 반환: 원본 int처럼 `true`/0이 아닌 정수면 종료 절차이며 이 프레임의 갱신·그리기·알림을 하지 않는다. `undefined`/`false`/0은 계속이다. 그 밖의 값은 처리기 오류(`TypeError`)로 알린다.
- **순서의 의미:** 입력(9)이 갱신(10)보다 앞이다. 따라서 입력에서 내린 명령은 같은 프레임 Kernel에 반영되고, 갱신 단계(예: `cleanupGumps`)에서 `pushCommand`로 예약한 명령은 다음 프레임 입력에서 소비된다. 프로세스는 1번에서 고정한 프레임 시각을 읽는다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 입력 큐 | 순서·0 사건·39개 한도와 새 사건 버림·한 바퀴 뒤 순서·뗌 거르기·DEL 변환(가상 키 0x7f 제외)·명령 코드 다듬기·사본 반환·범위 거부 |
| 단계 표 | 원본 순서 번호 비감소, 갱신 목록 20개 주소가 문서 순서와 같음, 모든 주소가 `main-loop.md`에 `FUN_xxxxxxxx`로 적혀 있음(옮겨 적기 확인), 시각<입력<Kernel<그리기 |
| 실행기 | 처리기 순서·미연결 보고·제외 단계 거부, 영상 갱신 정지 분기(signed −3은 정지 아님), 입력 종료 절차(true·−1 종료, 0 계속, 배열 반환 거부) |
| 통합 | GameClock·InputQueue·Kernel을 함께 돌려 1프레임: 클릭 입력 → cleanupGumps 예약 → Kernel(명령 없음), 2프레임: 예약 명령 입력 → 같은 프레임 Kernel이 명령과 게임 시각 2.5를 봄 |

- 처음 실행 때 검사의 기록 처리기가 `push` 반환값(참)을 돌려 입력이 종료 절차로 처리되는 실패가 있었다. 검사를 고치고, 같은 실수를 막도록 입력 처리기 반환값 검사를 실행기에 추가했다.
- 신규 `tests/main-loop.test.mjs` 7/7. 전체 결과는 인계 문서 0.1절에 기록한다. 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 남은 경계와 다음

- 앱 연결: `ClientRandomGlobals`·`GameAuxState`·`GameClock`·`Kernel`·`InputQueue`·`MainLoop`를 앱(클라이언트) 수명으로 만들고 `requestAnimationFrame` 표시 기회와 게임 프레임을 잇는 일은 첫 실제 미션 소비자(WB05)와 함께 한다. 숨겨진 탭 정지(1절 웹 적응)를 GameClock `pause/resume`과 영상 갱신 정지 중 무엇에 연결할지도 그때 정한다.
- 다음은 3.2절 순서3 **WB04-D**(생성자/가상 배치·Create/Take·Unpop·Form·종속 삭제/반납과 Regular 공통)다. C2의 `GameAuxState` Form/예약 번호와 C1의 Kernel·C3의 입력 큐를 빌린다.
