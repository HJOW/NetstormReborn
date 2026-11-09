# webpj 내용물 미리보기 프로세스(RawContentPreviewProcess) 이식 — WB04-E241

2026-11-10, HJOW-Athlon. Carrier 가 만든 일시적 내용물 그림(미리보기)을 만료 시각에 부모 삭제로 정리하는 10.78 프로세스를 옮겼다. 생성(428700)은 24바이트에 VT·Form·부모를 0 으로 쓰고 double 만료 시각(float 로 줄인 지속 시간 + 현재 게임 시각)을 기록한 뒤 같은 필드·부모·현재 프로세스 타입·flags 0x10 으로 부착을 요청하고, 프레임(428770)은 `now >= 만료`일 때만 부모를 지우며(NaN 비교는 건너뜀), 통지(480260)는 빈 몸체이고, 소멸(468230→41bd00)은 소멸 표식을 쓴 뒤 flags 하위 bit0 이면 할당 해제 경계를 요구한다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "공통 수명·그 밖" 묶음에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E240 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawContentPreviewProcess.cpp/.h`)·시험(`ContentPreviewProcessTests`)·fixture(`content-preview-process-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `2c79a9b8`(2026-10-17).
- 원본 fixture(10.78 전용) `content-preview-process-x86.tsv` 957행: 동작(생성 405·프레임 540·통지 6·소멸 6) × 현재 시각 비트 × 인자 float 비트(0, 1, 비정규 최소, 최대 유한, ±무한대, NaN, 음수 0 등) × 저장된 만료 시각 비트 × 부모(0·50·23999) × 현재 타입(10·47·61·0xffffffff) × 부모 head × 효과 변형(부착 중 시각 변경·만료 시각 변경·진단 중 부모 변경·삭제 중 필드 덮어쓰기) → 24바이트와 효과 순서. 열은 입력, 53비트 기본 CRT 정밀도의 결과(바이트·효과), 64비트 정밀도의 결과(바이트·효과)이다. 웹은 JS double(53비트)이라 앞의 두 열만 재생하고 64비트 열은 별도 관측으로 보존한다(cpppj 시험과 같은 규칙).
- 공유 장치: `squid-process.js`(`SquidProcess`·`SquidProcessHost`·`SquidProcessState`), `kernel.js`, `squid-destroy.js`, `sid-pool.js`.

## 1. 옮긴 것

`www/core/raw-content-preview-process.js`(DOM 없음, 10.78 전용)

- `ContentPreviewFields`: 원문 24바이트(+0 VT·+4 Form·+8 부모·+c 미정 패딩·+10 double 만료 시각)를 `bytes` 로 그대로 보관해 생성자가 건드리지 않는 바이트를 보존한다. `deadline()` 은 +10 의 현재 double, `parent()` 는 +8 의 부모 번호이며 SID 범위(0xffff)를 넘으면 `RangeError`.
- `RawContentPreviewProcess(pool, globals, hooks)`: 풀은 10.78 이어야 한다(아니면 `TypeError`). `globals = { now(), processType() }` 는 소비 위치마다 다시 읽는 `() => 값` getter 이다(C++ 의 `const double&`/`const uint32_t&` 대체). `hooks = { attach, destroy, assertion, release }` 는 도달한 위치에서만 요구하고 함수가 아니면(undefined·null 포함) 그 지점에서 `TypeError`.
  - `construct(fields, parent, duration)`: +8/+4/+0 을 0/0/0x503c1c 로 쓰고 +10 을 double 0 으로 초기화한 뒤 `Math.fround(duration) + now()` 를 +10 에 쓰고 `attach(fields, parent, processType() >>> 0, 0x10)` 를 호출한다. 부착 효과가 없으면 앞선 쓰기를 남긴 채 거부한다. 피연산자에 NaN 이 없는데 합이 NaN(∞ + −∞)이면 호스트와 무관하게 x87 의 실수 불확정 비트 `0xfff8000000000000` 을 쓴다. 피연산자에 NaN 이 있으면 호스트의 NaN 전파를 따른다(원본 관측과 같음).
  - `run(fields)`: `now() >= deadline()` 일 때만 동작한다. 부모를 먼저 고정하고 슬롯 head(+6 WORD)가 0 이면 `assertion()` 을 부른 뒤(반환하면 계속) 고정한 부모에 `destroy(parent, 0x400000)` 를 부른다.
  - `RawContentPreviewProcess.notify`: 세 인자를 읽거나 쓰지 않는 빈 정적 함수이다. `destruct(fields, flags)`: +0/+4/+8 에 0x5019ec/0xffffcfc7/0xffffcfc7 을 쓰고 `flags & 1` 이면 `release(fields)` 를 요구한다.
- `ContentPreviewProcess extends SquidProcess`: `runFrame`(Host 에 부착된 때만 `run`)·`onAttached`(Host 의 풀이 로직의 풀과 같을 때만 Form/부모를 +4/+8 에 기록, 아니면 `Error`)·`notify`(빈 통지)·`destroy`(Kernel 소멸 시 flags 0 소멸 쓰기)·`fields`.

## 2. 검증

`tests/content-preview-process.test.mjs`(8건)

- 원본 관측 재생 1건: 957행 전부(24바이트와 효과 순서). 모듈은 처음부터 모두 통과했다. NaN·무한대 행의 비트(`7ff8…`·`fff8…`·페이로드 보존)도 일치한다.
- cpppj 계약 포팅 2건: 실제 Host 에서 자연 만료와 외부 부모 삭제가 실행 중인 자신과 다른 만료 타이머의 Form/Kernel 을 한 번씩만 제거(깊이 0 복귀·삭제 flags 0x400000), 선택된 효과만 요구하고 10.72 판본을 거부.
- 웹 계약 5건: 생성 인자(float 절삭 `0.2f + 1.0`·`1.2` 아님, 음수 타입의 DWORD 전달, ∞ + −∞ 의 불확정 비트, 무한대끼리의 합은 그대로), 프레임의 부모 DWORD 범위·head 상위 바이트·진단 뒤 고정 부모 삭제, 부착 전/후와 풀 불일치, 부모 DWORD 경계(0xffff 는 SID, 0x10000·부호 비트·0xffffffff 는 거부), 함수가 아닌 효과(null·숫자·문자열·객체)의 미연결 거부.
- 생략: cpppj `ContentPreviewProcess_ActualHostExpiry…` 의 C++ `Kernel.Clear()` 소멸 순서 계약은 JS 에 소멸자가 없어 해당 없다(Kernel 소멸은 `destroy()` 호출로 시험).

모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 50건 중 7건이 살아남았다. 그중 1건은 동작을 바꾸지 않는 무의미한 변형이어서 실제 순서 변형 셋(부모 재조회·슬롯 늦은 읽기·삭제와 진단 순서 교환)으로 바꿨고, 5건은 경계 시험 둘(부모 DWORD 경계, 함수가 아닌 효과)을 더해 잡았으며, 새 변형 하나를 포함한 2건은 동등 변형이다.

- 동등 변형: ① `put` 의 `>>>` 를 `>>` 로 바꿈 — 쓰는 값이 int32 로 변환돼도 0xff 로 마스킹한 하위 8비트(최대 24비트 이동)는 같다. ② 헤드 읽기를 `slot` 사본 대신 두 번 읽도록 바꿈 — head 는 어떤 효과보다 먼저 한 번만 읽으므로 시점이 관측되지 않는다.
- 실패한 변형: 필드 위치와 모든 상수(VT·flags·소멸 표식), 부모 범위 상·하한과 경계, 부호 없는 읽기, 효과 검사, 만료 시각 저·고 DWORD, 지속 시간 float 절삭·덧셈, 불확정 NaN 비트와 세 조건, 판본 검사, 생성의 세 초기화 쓰기, 현재 타입 DWORD, 만료 비교(부등호·부정), head 저·고 바이트와 반전, 진단 생략·순서, 삭제 flags, 소멸 세 쓰기와 release 조건, 프레임의 Host 검사, 부착의 Host/풀 검사와 Form/부모 쓰기, Kernel 소멸 flags.

## 3. 경계와 남은 것

- 실제 Carrier 내용물 흐름(`RawCarrier*`/Dais 통지가 `construct` 를 부르는 지점)에는 연결하지 않았다. 실제 `attach` 는 장면의 Host, `destroy` 는 그 Host 의 `RawSquidDestroy` 로 연결하고, 진단 보고 장치(Regular.cpp 164행)와 `release` 할당 해제 경계는 순서4 몫이다(PORT_MAP session 0).
- NaN 피연산자의 페이로드 전파(두 NaN 이면 어느 쪽, 신호 NaN 의 조용한 형태 등)는 호스트(x64 V8)가 원본과 같은 결과를 내는 것을 fixture 로 확인했을 뿐 모델링하지 않았다. 일반 게임 흐름에서는 지속 시간과 시각이 유한하므로 영향이 없다.

## 4. 다음 시작점

WB04-E242 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 `RawXlat`(웹 `Xlat` 용어표의 10.78 다중 장부, 시험 `XlatFileReadTests`·`XlatRuntimeTests`)과 `RawTerrainGrowth`·`RawColorMapCache`(adapt)를 먼저 확인한다.
