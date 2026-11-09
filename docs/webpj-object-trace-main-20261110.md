# webpj 객체 Main/Task trace(RawObjectTraceMain) 이식 — WB04-E235

2026-11-10, HJOW-Athlon. 83개 자산의 공통 Main 진단(004b0fb0)과 Dude Task 진단(00450d60), 슬롯의 가상 표 기록값으로 원문 +0x70 본체를 고르는 분배, 그리고 원본 CRT `%2.5f` 좌표 서식을 옮겼다. E230 `RawObjectTraceFields`(Base·Form·Process·Depend·Content)와 같은 로그 장치 하나가 공통/파생 줄을 같은 순서로 받는다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E230 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawObjectTraceMain.cpp/.h`)·시험(`ObjectTraceMainTests`)·fixture 셋(`object-trace-main-x86.tsv`·`object-trace-vtables-x86.tsv`·`object-trace-coordinate-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `a5c411c3`(2026-10-29).
- 원본 fixture(10.78 전용): `object-trace-main` 373행(Main/Task 입력 7칸: 진입 본체·SID·덤프 모드·TYPE flags1/flags2·로그 직후 변경·슬롯 raw 50 BYTE; 출력은 `레벨:서식 바이트 hex` 열과 수치 인자), `object-trace-vtables` 87행(PE 에서 독립 추출한 +0x70 표 주소 → 불리는 본체), `object-trace-coordinate` 887행(float32 비트 → 원본 CRT `%2.5f` 바이트).
- 공유 장치: E230 `raw-object-trace-fields.js`, `squid-constructors.js`(`supportsConstructedVtable`: base 표와 검증된 자산 83개 표), `sid-pool.js`.

## 1. 옮긴 것

`www/core/raw-object-trace-main.js`(DOM 없음, 10.78 전용)

- `traceRawObjectMain(pool, types, suppress, sid, task, log)`: 공통 Base 줄을 기록한 **뒤에** 슬롯을 다시 읽어 Main 줄을 만든다: `Main( x y) hl=… q=… r=… z=… [fram=…] plId=… [HP=…] [GrNum=…]` + 상태(Astral·Abstract·OvDraw·Selected·Invisible·Paralyzed·InWorld/`!InWorld`)·부모/이전(contained)·섬 id(bridge 면 127)·genus 대문자 16진. 덤프 모드는 Base 로그 뒤에 다시 읽는다. `log(level, line, numbers)` 의 `numbers` 는 Main 줄일 때만 `{ x, y, xBits, yBits, highlight, quantity, region, z }`(float32→double 승격 좌표와 그 64비트, 부호 확장 BYTE, 부호 없는 WORD)이다. task 이면 Main 로그 복귀 뒤 현재 작업 WORD(+12)를 다시 읽어 0x5101..0x5107 로 자른 `Task "<이름>"` 줄(`TEND_MANA_PRODUCERS`·`GO_HOME`·`GO_TO`·`FALL`·`BLEED`·`RunBurning`·`\x87YN`)을 더한다.
- `traceRawObjectVirtual(pool, types, suppress, sid, log)`: 슬롯의 가상 표(+0)로 본체를 고른다. 네 내장 Form 표(512110 Depend·512098 Process·512188 Form·512208 Content)는 필드 서식, base 표와 검증된 자산 83개 표는 Main(표 508cc8 만 Task 포함)이다. 미등록 표는 임의 Base 로 대체하지 않고 로그 전에 거부한다.
- `formatOriginalTraceCoordinateBits(bits)`/`formatOriginalTraceCoordinate(value)`: 0·음의 0 → `0.00000`, 특수값 `1.#INF0`·`-1.#INF0`·`-1.#IND0`·`1.#QNAN`·`-1.#QNAN`, 일반 값은 float32 의 정확한 double 승격을 17자리 가수로 만든 뒤 소수 5자리(decpt+5 자리)까지 유지하고 다음 자리가 5 이상이면 올림(연속 9 전파, 맨 앞 캐리는 `1` 삽입)한다.

## 2. 검증

`tests/object-trace-main.test.mjs`(5건)

- 원본 관측 재생 3건: Main/Task 373행(로그 바이트 전체와 수치 인자, 첫 로그 직후 필드·덤프 모드 변경 두 종류와 두 번째 로그 직후 작업 WORD 변경 포함), +0x70 표 87행(Dude 한 표만 Task, 나머지는 Main 또는 네 Form 중 하나와 같은 출력)과 미등록 표 거부(로그 0건), 좌표 887행(float32 비트별 `%2.5f` 바이트).
- 웹 계약 2건(원본 fixture 밖 경계): 수에서 만든 좌표 서식 몇 건과 로그 장치 부재(TypeError)·현재 TYPE 표 밖(RangeError, Main 줄 미기록), 첫 로그 직후 TYPE 이 표 밖이 되는 경우의 Main 줄 거부, 좌표 NaN 의 double 승격 비트(부호·가수 보존, 조용한 NaN 비트 켬), 연속 9 올림이 맨 앞까지 전파되는 `0.99999994 → 1.00000`.
- 생략: cpppj 의 네 번째 시험(재귀 분배 `RawObjectTrace`+`TraceRawObjectVirtual`)은 `RawObjectTrace` 가 미이식이라 그 단위를 옮길 때 대조한다.

최초 실패와 교정: 모듈과 세 fixture 재생은 처음부터 통과했다(JS `toExponential(16)` 의 17자리 변환이 반올림 동률 포함 887개와 모두 같았다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 53건 중 8건이 살아남아 4건은 웹 계약을 더해 잡았다.

- 실패한 변형: 로그 레벨·Task 상/하한·작업 이름·버퍼 한도·표 상수·Form 표↔종류 대응, 정수 부호/채움, 좌표 서식(0·음의 0·INF/IND/QNAN·정밀도·유지 자릿수·올림 임계/범위·0 패딩·부호), Main 줄의 모든 분기(contained·HP(다이스)·GrNum·fram 덤프·islandId·상태 비트 여섯·genus·plId·수치 인자 부호 확장·quantity/region), Task 재조회·하한, 가상 표 미등록 거부·Task 표(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): ① 모듈의 슬롯 범위 검사 제거 — `SidPool.slot` 이 같은 RangeError 를 던진다. ② 로그 함수 존재 검사 제거 — 함수가 아니면 호출 자체가 TypeError 다. ③ 호스트 서식 버퍼 길이 검사 — 유한 float 좌표 둘과 고정 문구로는 1024 자에 이르지 않는다. ④ 판본 검사 — `SidPool` 은 항상 10.78 이다.

## 3. 경계와 남은 것

- 실제 로그 장치(레벨 29, 서식 파서)와 호출 지점(객체 vtable +70, 개발자 덤프 명령)은 연결하지 않았다. 파생 자산별 Main/Task 연결은 순서4 몫이다.
- 좌표 NaN 의 double 승격 페이로드(조용한 NaN 비트)는 SSE 승격 규칙에 따른 것이며 fixture 에는 NaN 좌표 Main 줄이 없어 웹 계약으로만 확인했다.

## 4. 다음 시작점

WB04-E236 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawTotalMade`·`RawTutorialOne`·`RawObjectTrace`)를 권한다. `RawObjectTrace` 를 옮길 때 이 모듈의 재귀 분배 시험을 함께 대조한다.
