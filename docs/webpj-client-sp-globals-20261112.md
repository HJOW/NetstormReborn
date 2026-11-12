# webpj Client 수명 SP 전역 — ClientSpGlobals·ClientTimeSource (WB04-E336, 2026-11-12, 4-D A 구획)

3.1절 4-D A 구획의 client **ClientSpGlobals**(기준선 `.h` 22 + `.cpp` 22 = 42줄, 기준선 뒤 +3줄)를 `core/client-sp-globals.js` 로 옮겼다. 기준선 뒤 +3줄은 `ClientCurrentUnixSeconds` 가 새 파일 **ClientTimeSource**(2026-11-09 추가, 검사 전용 고정 시각 모드)로 위임하도록 바뀐 것이어서, 그 시각 공급원을 `core/client-time-source.js` 로 함께 옮겼다(3.6절 ③ 반영). 앞 단위(E325·E331~E335)와 같은 "참조를 빌려 주는 소유자, 값 사본 없음" 방식이다.

## 1. 규칙

- **ClientSpGlobals(random, unixSeconds, allocation?):** 같은 Client 의 `ClientRandomGlobals`(게임/CRT 난수·모드)와 `GameRandomSeed`(004559d0 재시드)·`ScrambledSpStore`(분산 32×9 SP 저장소)·사제 추적 offset(0054d4e0, 최초 BSS 0 `Int32Box`)을 한 번 조립한다. **생성자·접근자·Session 반납은 시각·난수·할당을 쓰지 않고 숨은 재시드나 초기화를 하지 않는다.** 실제 초기화(재시드 → offset → 순열 → 32개 배열 확보)는 원문 호출자(공통 시작 004391ed)가 `store().initialize()` 로 명시한다. 접근자 `store()`/`randomGlobals()`/`tracking()` 은 호출마다 같은 객체이며, `tracking()` 은 저장소의 `trackingOffset` 이 읽는 같은 칸이다. 난수 소유자가 `ClientRandomGlobals` 가 아니면 `TypeError`, 시각 공급자가 함수가 아니면 `GameRandomSeed` 가 `TypeError` 로 거부한다. 할당 입력을 주지 않으면 36BYTE 새 할당의 0 이 입력이다(웹 적응, `currentAllocationBytes`).
  - 재시드는 난수 모드가 0 이어도 시각을 읽는다: `reseed()` 가 모드 DWORD 를 잠시 1 로 바꿔 시각 시드를 쓰고, 공급자가 예외(예: signed 32비트 밖의 초)를 던져도 원래 모드를 되돌리며 저장소는 초기화 완료로 보지 않는다.
- **ClientTimeSource:** 기본은 호스트 시계(밀리초는 `performance.now()` 정수부를 부호 없는 32비트로 감음, 유닉스 초는 `Date.now()` 의 초)이고, `enableFixed({ frameMilliseconds, originMilliseconds = 1000000, unixSeconds = 946684800 })` 로 고정 시각 모드를 켜면 `advanceFixed()` 한 번마다 프레임 밀리초만큼만 전진한다. 프레임 밀리초는 1~60000 정수, 유닉스 초는 0 이상 정수이고, 이미 켜져 있으면 거부하며 거부한 호출은 상태를 바꾸지 않는다. 고정 모드의 `milliseconds()` 는 `(원점 + 경과)` 의 부호 없는 32비트 감김, `unixSeconds()` 는 `설정 초 + floor(경과/1000)` 이고, 두 모드 모두 time32 범위(0~2^31−1)를 벗어난 초는 `RangeError` 이다. `disableFixedForTests()` 는 설정과 경과를 처음으로 되돌린다.

## 2. 웹 범위와 기준선 뒤 변경

- 기준선 뒤 +3줄과 새 파일 ClientTimeSource(+80줄)를 모두 반영했다(① 반영 / ③ 해소). C++ 는 프로세스 전역(`EnableFixedClientTime` 등)이지만 웹은 Client 가 만드는 인스턴스로 두고 호스트 시계를 주입할 수 있게 했다. 비Windows `ClientMilliseconds` 의 "OS 밀리초 시계 미연결" 오류는 웹이 `performance.now()` 를 쓰므로 없다.
- PORT_MAP: `ClientSpGlobals.cpp` port(웹 칸에 `client-time-source.js` 를 함께 적음). 앱 수명 연결(Client 가 한 번 만듦)과 `--fixed-frame-ms` 에 해당하는 웹 시험 장치 연결은 4-D 조립·L2 판정 몫이다(PORT_MAP session 0).

## 3. 검사 (`tests/client-sp-globals.test.mjs` 11건)

- **원본 독립 관측:** `sp-initialization-x86.tsv` 48행(시드 0/42/4294967295 × 이전 완전 초기화 0/1 × 중단 지점 0/1/16/32번째 할당 × x87 정밀도 모드 두 가지): 할당 호출 수·초기화 완료 여부·비트 offset·추적 offset·게임 난수 상태·첫 배열 확보 여부와 실제 1152BYTE 분산 배열 전체(중단된 부분 초기화 포함)를 대조한다. **첫 실제 실행에서 48행이 모두 일치했다**(x87 두 모드 행의 값이 같아 웹이 구별하지 않아도 된다). manifest 에 `sp-initialization` 항목을 추가했다.
- **cpppj 시험 이식:** `ClientSpGlobals_ExplicitStartupUsesSameRandomAndLongLivedTracking`(생성/접근자는 초기화하지 않음, 같은 난수·장기 추적 offset), `ClientTimeSource_DefaultUsesOperatingSystemClock`·`ClientTimeSource_FixedModeAdvancesOnlyPerFrame`. 나머지 `ActualClientCreationAndGetterDoNotInitialize` 는 실제 Client 생성이 필요해 Client 조립(4-D E)에서 옮긴다.
- **웹 계약:** 생성자 거부(메시지 포함)·기본 할당 입력 0, 모드 0 에서도 재시드가 시각을 읽고 모드 복원, 공급자 범위 오류 시 모드 복원과 미초기화, 소유자를 거치지 않은 직접 조립과의 일치(offset·분산 배열·게임/CRT 난수 상태), 두 소유자 독립, 호스트 시계 정수부, 고정 모드의 기본값/경계/내림/time32 상한.
- 첫 실행의 실패 2건은 시험 쪽 오류였다: ① `readRows` 가 열 이름 줄을 데이터로 세어 49행이 되었다(헤더 제외) ② time32 상한 시험의 산술(60초 프레임 기준으로 상한 직전 값을 잘못 잡음). 모듈은 그대로이다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 40개(SP 전역 11·시각 공급원 29)를 돌려 첫 실행에서 11개가 남았다(소유자 난수 정체성, 호스트 정수부 내림, 고정 초 내림, 생성자 메시지 등). 시험을 보강(직접 조립 대조·호스트 시계 임시 교체·600ms 경계·메시지)한 뒤 **7개가 남았고 모두 동등**하다(모두 `client-time-source.js`).
  1. `enableFixed` 의 `originMilliseconds >>> 0`: `milliseconds()` 가 합을 `>>> 0` 으로 감으므로 같은 값이다.
  2. `enableFixed`/`disableFixedForTests` 안의 `#elapsed = 0` 중복과 `disableFixedForTests` 의 설정 기본값 복원: 켤 때 세 설정과 경과를 모두 다시 쓰고, 꺼진 동안은 호스트 시계를 읽으므로 관측되지 않는다.
  3. `unixSeconds()` 의 `Math.floor(경과/1000)` 대 `경과/1000`: 뒤따르는 `| 0` 이 소수부를 버려 같은 값이다. 마지막 `| 0` 제거도 이미 정수라 같다.
  4. `advanceFixed` 의 `if (프레임밀리초)` 보호: 꺼진 상태에서는 0 을 더하므로 같다.
- 3.1절 4-D 는 7/28 이다(`ClientSpGlobals`·`ClientNsInfoGlobals` 묶음 줄을 풀어 분모가 27 → 28; A 구획: Fort·PlayerRecords/Runtime·CoreData·Camera·AiDebug·LocalState·**SpGlobals** 완료). 다음: A 구획 `ClientNsInfoGlobals`(+44줄, 메타 서버 목록의 MP 제외 판정 필요), `ClientStateWorld*` 묶음.
