# webpj AI 부족 좌표 목록(RawAiDrySpots) 이식 — WB04-E170

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔 번째 부분이다. AI 가 에너지 부족으로 건물을 놓지 못한 좌표를 모아 두는 목록(패치 `0040f1c0` 추가·`0040f260` 제거·`0040f2b0` 순환 검사·`00417c70` 유지·`004133e0` 가짜 좌표 선택)을 옮긴 `o/RawAiDrySpots`(cpppj 기준선 `1f00a25a`, 기준선 뒤 이 파일과 시험의 변경 없음을 `git diff --numstat` 으로 확인)이다. AI 계열(33개 파일) 여덟 번째 단위이며 뒤의 `RawAiFakeDrySpots`·`RawAiDropEvaluation` 이 이 목록을 쓴다.

## 1. 옮긴 것

`www/core/raw-ai-dry-spots.js`(DOM 없음)

- `new RawAiDrySpots(memory, pool, hooks)` — `memory` 는 E164 `RawAiBrainMemory`, `pool` 은 같은 PATCH SidPool(월드 정체성), `hooks = { energy(box, x, y, type, owner), log(token, fake) }`(`energy` 는 초기 빈 문자열 상자 `{ text: '' }` 와 원래 DWORD 반환을 bool 로 줄이지 않는다). 별도 좌표 사본 없이 본체의 +0xadc(용량)·+0xae0(개수)·+0xae4(기록 포인터)·+0xae8(cursor)·+0xaec(owner)와 같은 소유 backing(12바이트 기록: x/y float 비트 + TYPE DWORD)을 쓴다.
- `add(token, x, y, type, bits = {})` — 용량이 우선이라 가득 차면 포인터/중복 검색을 읽지 않고 false, 아니면 좌표만 중복 검사(float 동등 비교: signed 0 은 같고 NaN 은 서로 다르며 TYPE 이 달라도 같은 좌표면 중복)한 뒤 원문 12바이트를 개수 위치에 추가한다. NaN 페이로드 보존이 필요하면 `bits = { xBits, yBits }` 를 준다.
- `remove(token, index)` — 뒤 기록을 x → y → TYPE 순서로 안정 압축하고 개수만 줄인다(index 가 끝 이상이면 복사 없이 줄이고 미사용 꼬리/cursor 는 초기화하지 않는다, 목록 포인터 0 의 개수 −1 도 접근 없이 감김).
- `scan(token)` — 개수 0 이면 cursor 불변 0, 아니면 `(cursor + 1) % 개수`(signed) 한 항목만 검사한다. cursor 를 먼저 쓴 뒤 에너지 승인을 묻고(선택 하위 누락/물리 오류 앞에서도 cursor 는 이미 쓰여 있다), 승인되면 에너지 효과 뒤의 현재 backing/cursor 에서 기록 TYPE 를 다시 읽어 제거하고 그 TYPE 를 돌려준다. 제거 뒤 cursor 보정은 없고 개수 −1 의 signed 나눗셈 넘침은 명시 거부한다.
- `maintain(token)` — `scan` 의 TYPE 가 0 이 아니면 현재 가짜 TYPE(+0x280)와 비교한 분류와 현재 디버그(+0xb00)로 로그를 분배한다. `seed(token, random)` — 능력 BYTE(+0x248 의 0x20)가 있을 때 공용 난수 `next(8)` 한 번으로 여덟 방향(합계/자기 경계 조합) 중 하나를 고르고, 네 float 의 합/곱을 전부 마친 뒤에만 float 로 저장해 +0xaf0/+0xaf4 에 쓴 다음 현재 가짜 TYPE 로 등록한다(가득 참/중복/물리 오류에서도 좌표와 난수 소비를 되돌리지 않는다).

## 2. 검증

`tests/ai-dry-spots.test.mjs`(15건). 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다). 같은 입력과 단언을 옮겼다.

- 추가의 좌표 중복(signed 0/NaN/TYPE 비트 보존), 가득 참/빈 목록의 포인터 접근 없음과 cursor 불변, 한 항목 조회와 제거 뒤 cursor 불변(순환 3개 목록), 제거의 안정 압축·미사용 꼬리·끝 이상 index·포인터 0 의 −1 감김, 에너지 효과가 backing/cursor/count/owner/fake 를 바꾼 뒤의 현재 기록 TYPE 재조회와 해제된 이전 backing, 현재 fake/디버그로 로그 분배와 TYPE 0 제거, 선택 하위 누락/물리 오류 앞의 cursor 보존과 signed 나눗셈 넘침, 능력 BYTE 금지 상태 무효과, 여덟 방향 전수(시드 1..8)와 원본 TYPE DWORD/float 비트, 합 전체 뒤 float 저장(2^24 경계), 가득 참/중복/영 TYPE·음수 좌표/물리 실패에서의 보존, 생성자 거부.

최초 실패와 교정: 새 모듈과 시험은 15건 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 에너지 판정(`004734d0`)과 부족 좌표 로그(`004105b0`)·실제 공용 난수 연결은 후속 AI 세션 연결 단위이며 지금은 명시 hook/객체이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiPriestActions`(사제 행동 `00411cd0` 전체)와 AI 드롭 계열(`RawAiDropTargets`·`RawAiDropEvaluation`·`RawAiDropScan`·`RawAiDropSelection`·`RawAiDropActions`)·`RawAiFakeDrySpots`(`AiPriestPointValid` 필요) 등 AI 계열 나머지, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
