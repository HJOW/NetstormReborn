# webpj checking 진단(RawCheckingDiagnostics) 이식 — WB04-E240

2026-11-10, HJOW-Athlon. 진단 모드(`checking`, 005e4794)에서만 선택되는 두 순수 진단을 옮겼다. ① 공통 Form 검사(004ad730): 자산 부모의 Form 자식 체인에 free/void 나 자산 TYPE 이 끼어 있지 않은지 검사한다. ② 총(Gun)의 noIsland 소유자 진단(0046b830 case 1 의 41def0): 발자국 영역의 후보 중 현재 noIsland TYPE 이면서 부모와 소유자가 다른 후보마다 Gunprocess.cpp 0xad 진단을 보고한다. 둘 다 게임 동작·생성·공격을 실행하지 않고 같은 raw 자료만 읽는다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E239 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawCheckingDiagnostics.h`, 헤더 전용)·시험(`CheckingConsumersTests`)·fixture(`checking-forms-x86.tsv`·`checking-island-owner-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `8b467ca0`(2026-11-03).
- 원본 fixture(10.78 전용): `checking-forms` 288행(부모 TYPE/상태·head 개수 0/1/2·자식·마지막 후보의 TYPE/상태 조합 → 결과 1/0), `checking-island-owner` 192행(checking·부모 소유자·noIsland TYPE·첫 진단 뒤 변경 4종·발자국 상한 세 가지 → 진단 횟수·진단 뒤 checking/noIsland/부모 소유자·상한 정수 변환).
- 공유 장치: `raw-squid-finder.js`(`RawSquidFinder`), `squid-hash.js`, `sid-pool.js`.

## 1. 옮긴 것

`www/core/raw-checking-diagnostics.js`(DOM 없음, 10.78 전용)

- `checkRawObjectForms(pool, parent)`(004ad730): 부모가 자산 TYPE(70 이상)이고 free/void(상태 비트 1·4)가 아니어야 하며, head 부터 next 고리의 모든 자식이 free/void 가 아니고 TYPE 69 이하(Form)이면 참이다(dead 는 허용). 각 next 를 소비하며 결과를 캐시하지 않는다. 호스트만 풀 밖 번호와 순환 체인(`guard > capacity`)을 명시 오류로 거부한다(원문은 끝없이 돈다).
- `diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, checking, noIslandType, assertion)`: checking 이 꺼져 있으면 풀/해시/발자국을 읽기 전에 즉시 반환. 켜져 있으면 발자국(float 네 개)을 정수 칸으로 바꿔 일반 4단계 finder 로 훑는다: 왼쪽·위는 float 를 그대로 `ftol`, 오른쪽·아래(상한)는 보호 PE 502310 의 float 0.99999(0.9999899864…)를 더하고 int → float 저장 뒤 다시 `ftol`. 후보 TYPE 이 현재 noIsland TYPE 이고 checking 이 아직 켜져 있고 후보 소유자가 부모 소유자와 다르면 진단(줄 0xad)을 보고한다. `checking`·`noIslandType` 은 `() => 정수` getter 로 후보마다 다시 읽고 부모 소유자도 매번 다시 읽는다. 진단 장치가 없으면 그 지점에서 TypeError.
- `checkingInteger(value)`/`checkingStoredInteger(value, upper)`: 비유한·64비트 밖은 정수 불확정값의 하위 0, 정상은 DWORD 감김(`-(2^62) - 4096 → -4096` 등).

## 2. 검증

`tests/checking-diagnostics.test.mjs`(5건)

- 원본 관측 재생 2건: Form 검사 288행(결과와 풀 불변), noIsland 소유자 진단 192행(진단 횟수, 진단 뒤 checking·noIsland·부모 소유자, 발자국 상한의 정수 변환).
- cpppj 계약 포팅 1건 + 웹 보강: dead Form 자식은 허용, 자기 순환 체인과 풀 밖 next(65534)는 거부; 자식 TYPE 69 는 참·70 은 거짓, 부모 free/void/TYPE 69 는 거짓.
- 웹 계약 2건: checking 꺼짐이면 잘못된 인자도 읽지 않고 반환, 정수 변환(Infinity/NaN/±2^63 밖 → 0, 2^32+5 → 5, −1.9 → −1, 큰 음수의 하위 32비트), 진단 장치 누락 거부, 소유자가 같거나 noIsland 가 아닌 후보는 진단 없음; 발자국 오른쪽·아래는 편향으로 한 칸 넓게 읽어 그 칸의 후보를 진단하고 왼쪽·위는 편향 없이 자름.
- 생략: cpppj 앞선 세 시험(`RawCheckingFlag` 의 DWORD 뷰/임시 값 수명/Client owner 재빌림)은 C++ 참조 수명 계약이라 웹에서는 getter 함수로 대체해 해당 없다.

최초 실패와 교정: 모듈은 처음부터 fixture 480행이 모두 통과했다. 시험 쪽 결함 하나를 고쳤다(계약 시험에서 자식 TYPE 69 를 거짓으로 적었는데 원문은 자식이 TYPE ≤ 69 인 Form 이어야 참이다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 33건 중 순환 방지 `guard` 제거는 무한 순환으로 시험이 끝나지 않아 제한 시간 뒤 종료(탐지로 판정)했고, 6건이 살아남아 3건은 웹 계약을 더해 잡고 3건은 동등 변형이다(정수 변환의 큰 음수 경계, 오른쪽·아래 편향).

- 실패한 변형: 편향 상수·필드 위치·상태 비트·자산 TYPE 상한·진단 줄, 부모 검사 둘, 자식 검사 둘, 정수 변환(유한·하한·절삭·감김), 상한 인자(왼쪽/위/오른쪽/아래 각각), 바깥 관문, finder flags, TYPE/checking 재조회/소유자 비교, 순환 방지(각 1건 모두 실패 또는 종료되지 않음).
- 적용되지만 통과한 변형(동등 변형): ① 풀 밖 next 검사 제거 — `SidPool.slot` 이 같은 오류를 던진다. ② 진단 장치 존재 검사 제거 — 호출 자체가 TypeError 다. ③ 상한 정수 변환 뒤 `Math.fround(first)` 제거 — 입력이 float32 이면 감긴 정수 결과가 항상 float32 로 정확히 표현되어 같은 값이다(웹은 JS 수라 float32 인자를 호출자가 보장).

## 3. 경계와 남은 것

- 실제 총/객체 흐름(Gunprocess)에 연결하지 않았다. `checking` 전역(005e4794)의 실제 소유자, noIsland TYPE 전역, 진단 보고 장치와 호출 지점(총 처리·객체 Form 검사)은 순서4 몫이다(PORT_MAP session 0).

## 4. 다음 시작점

WB04-E241 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawColorMapCache`·`RawContentPreviewProcess`)를 권한다.
