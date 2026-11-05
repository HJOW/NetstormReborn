# webpj 세션 장기 상태 1단계: 난수·게임 시계·Kernel — WB04-C1

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서2(WB04-C)의 첫 하위 단위다. WB04-C는 한 번에 완료 처리하지 않고 실제 호출 관계에 따라 나눈다. 이번에는 세션보다 오래 사는 기반인 **CRT/게임 난수·시드 초기화·클라이언트 난수 전역·게임 시계·Kernel**을 옮겼다. 앱/세션 연결과 화면 변경은 없다.

## 1. 근거

- 시작 트리 `512621f2`. PC는 마지막 전체 정밀 디컴파일 PC(HJOW-Athlon / 2026-11-03)와 같다. 디컴파일 산출물·dotnetpj 구현과 다른 세션의 cpppj **미커밋** 변경은 근거로 쓰지 않았다(1절 "진행 중인 cpppj 기준" 결정).
- 착수 절차대로 `node webpj/tools/fixture-manifest.mjs --write`로 WB04-B 항목의 `webCommit`을 `0563c07f`로 갱신했다. cpppj 변경 보고 7곳(3.5절)은 이번 대상과 겹치지 않았다.

| 웹 | cpppj 근거(마지막 변경) | 원본 10.78 |
| --- | --- | --- |
| `core/random.js` `CrtRandom` | `o/CrtRandom.cpp`(`954d1942`) | `004e64ab` CRT rand |
| `core/random.js` `GameRandom` | `o/SpStore.cpp`의 GameRandom(`a1bfa97d`) | `004558c0` |
| `core/random.js` `GameRandomSeed` | `o/RawGameRandomSeed.cpp`(`a24db9dc`) | `00455970` Initialize, `004559d0` Reseed |
| `core/random.js` `ClientRandomGlobals` | `client/ClientRandomGlobals.h`(`d207940c`) | CRT holdrand, `00532710` 게임 난수, `005c89a8` 모드, `005c89b4`, `005c89a4` |
| `core/game-clock.js` `GameClock` | `o/GameClock.cpp/.h`(`adb9739e`) | `00460cd0`~`00460e90`, Reset `00460e70` |
| `core/kernel.js` `Kernel`·`BaseProcess` | `o/Kernel.cpp`(`a5afdafb`)·`.h`·`o/BaseProcess.h`(`225f2322`) | `00471950` 등록, `004719e0` 삭제, `00471a30` 실행, `00471af0` Clear |

원본 관측: `cpppj/tests/fixtures/campaign-seed-x86.tsv`(`a24db9dc`, 96행), `game-clock-reset-x86.tsv`(`adb9739e`, 138행). 이식 추적 목록에 `game-random-seed`·`game-clock`으로 등록했다. Kernel은 원본 관측 fixture가 없어 cpppj `RecoveryTests.cpp`의 `Kernel_MutationUsesLiveSlots_InOriginalOrder` 장면과 계약을 옮겼다.

## 2. 사용 계약

```javascript
// 클라이언트 생성 때 한 번 만든다. 세션/미션 생성·반납으로 다시 만들지 않는다.
const globals = new ClientRandomGlobals();            // CRT 1, 게임 난수 55, 모드 0
const seed = new GameRandomSeed(globals, () => unixSeconds); // signed 32비트 Unix 초 공급자
seed.initialize();                                    // 모드 0 이면 고정 시드 0x38d535
const clock = new GameClock(startMilliseconds);       // 32비트 밀리초 입력
const frame = clock.capture(nowMilliseconds);         // { game, wall, delta, number }
const kernel = new Kernel();                          // 1..39999 슬롯
```

- **같은 상태 공유:** C++의 `uint32_t&`/`Borrow`는 JS에서 같은 객체(`globals.crt`·`globals.game`)를 빌리는 것으로 옮겼다. 상태를 복사한 별도 스트림을 만들지 않는다. `gameMode`·`fortCoreSeed`·`subscriberIdInitialized`는 같은 전역 객체의 DWORD 칸이며, 앞의 둘 외 소비자(Fort 핵심 시드 정리, Subscriber ID)는 후속 단위에서 연결한다.
- **CRT 통합:** 지형 코드(`assets/terrain-tiles.js`)의 `CrtRandom`은 이제 같은 core 구현을 다시 내보낸다. API(`new CrtRandom(seed)`, `state`, `next()`)와 결과는 같고 `setState`만 늘었다. 기존 지형 검사는 그대로 통과한다. 개발용 지형 미리보기의 `seedGameRandoms`(독립 시드)와 청크 지역 난수 `LocalRandom`(원본 `004bca20`, 게임 난수와 같은 식이지만 별도 함수)은 그대로 두었다. 실제 세션에서 지형이 클라이언트 CRT/게임 난수를 공유하는 연결은 WB05다.
- **GameRandomSeed:** 모드가 0이면 시각을 읽지 않는다. `reseed()`는 모드를 1로 바꿨다가 성공/예외와 관계없이 되돌린다. 시각은 signed 32비트 정수만 받아 같은 비트의 unsigned로 CRT에 넣는다.
- **GameClock:** 원본과 같은 순서의 double 연산이다(`floor(경과/1000) + (경과%1000)×0.001`, unsigned 32비트 감김). `reset()`은 누적 정지만 지우고 원점·정지 깊이·저장 게임 시각·마지막 프레임은 그대로다. 표시용 `FrameClock`(`netstorm.js`)과 섞지 않는다. 숨겨진 탭 정지(1절 웹 적응)를 이 시계의 `pause/resume`에 연결할지는 앱 연결 단계에서 정한다.
- **Kernel:** 프로세스는 `runFrame()`을 가진 객체이고 `destroy()`(원본 가상 소멸자)는 선택이다. `runFrame()`은 그 순간의 슬롯을 앞에서부터 읽는다(뒤 슬롯 신규 등록은 같은 프레임, 지난 앞 슬롯은 다음 프레임). `remove()`는 `destroy()`를 먼저 부르고 슬롯을 비우므로 소멸 중에는 원래 슬롯이 보인다. `clear()`는 한 번만 순회하고, `dispose()`는 남은 것이 없을 때까지 반복한다(cpppj 소멸자와 같음). 같은 객체 재등록은 기존 번호를 돌려준다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 시드 초기화 | 원본 **96행** 전체: 모드·게임 상태(=반환값)·CRT 상태·시각 호출 수 일치, CRT 소비 104 |
| 게임 시계 | 원본 공개 호출 **114행**(두 x87 정밀도, Game/Wall double 비트·IsPaused·Pause/Resume/Reset 22회) 일치. raw 24행은 객체 바이트 배치 증거라 개수만 확인 |
| 난수 계약 | CRT 1→41(상태 2745024), CRT 0→38(상태 2531011), 게임 난수 0 대체, limit 0 거부, 새 클라이언트마다 별도 전역, 지형 고정 시드 경로와 같은 결과 |
| 시계 계약 | Reset 뒤 프레임 이력 보존, 중첩 정지의 마지막 resume만 누적, 밀리초 감김 |
| Kernel | cpppj 슬롯 변경 장면(1,2 → 2 → 4,2), 재등록·소멸 중 슬롯·clear 한 번 순회·dispose·소진·범위 밖 거부 |

- 음성 대조: 임시 사본에서 `wallSeconds`를 `경과 × 0.001`로 바꾸면 시계 114행 대조가 실패함을 확인하고 임시 파일을 지웠다.
- 신규 `tests/session-runtime.test.mjs` 7/7. 전체 결과는 인계 문서 0.1절에 기록한다. 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 남은 WB04-C 하위 단위

| 하위 단위 | 범위 |
| --- | --- |
| WB04-C2 | 세션이 장기 소유하는 현재 TYPE/모드 전역(cpppj Factory 타입 전역·`RawGameAuxState` 등 현재 커밋 기준)과 Regular 공통 등록/실행 경계. 생성/Pop(WB04-D/E)이 빌릴 전역만 옮긴다 |
| WB04-C3 | 입력/예약 큐(cpppj `client/InputEvent.h`의 `InputQueue`, 명령 예약·취소 순서)와 게임 시계·Kernel의 프레임 순서 연결 |
| 앱 연결 | `ClientRandomGlobals`를 클라이언트(앱) 수명으로 만들고 GameClock을 앱 시계 입력에 연결하는 일은 첫 실제 소비자(세션 조립 WB05 또는 C3의 프레임 순서)와 함께 한다. 사용처 없이 빈 연결을 먼저 만들지 않는다 |
