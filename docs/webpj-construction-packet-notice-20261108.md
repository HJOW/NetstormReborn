# webpj 건설 패킷(RawConstructionPacket)·배치 통지(RawConstructionNotice) 이식 — WB04-E134

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른네 번째 부분이다. 건설 확정(메시지 0x3f)의 실제 0x200바이트 임시 패킷과 그 위에서 도는 배치 통지 처리기(원본 `004441b0`) — 놓을 칸의 로컬 예측 조각 정리 → 최근 건설 위치 이력 → 조각 배치 → 건설 사제 이동 / 소유 사제 이동·환불 / 자동 건설 시작 → 갱신 — 를 옮긴 `o/RawConstructionPacket`·`o/RawConstructionNotice`(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일 모두 변경 없음)이다. 선행은 E132의 `makeConstructionProcessNoticeHooks`(통지의 건설 시작 경계)와 기존 `construction-history-key.js`(최근 건설 위치 장부), `raw-priest-state.js`(사제 이동 불가 조회)다.

## 1. 옮긴 것

`www/core/raw-construction-packet.js`

- `createConstructionPacketHeader()` — 실제 Confirm 입구의 현재 메시지 등록값 `{message 0x3f, protocol 1, declaredLength 0}`.
- `ConstructionPacketView(packet)` — 통지 처리기와 SID 읽기(40a840)가 같은 원문 BYTE 버퍼를 빌려 쓰는 읽기 전용 뷰. 필드(type[+4]·x[+5]·y[+9]·argument[+13]·direction[+14]·player[+15]·flags[+16]·quality[+17]·timeLow[+18]·timeHigh[+22]·sidCount[+26]·SID WORD[+0x1b+2i])를 생성 때가 아니라 소비 위치마다 현재 바이트에서 다시 읽는다. 선언 개수나 19개 제한을 앞세우지 않고 요청한 칸만 읽으며 물리 범위 밖은 거부한다(`xBits()`/`yBits()` 는 NaN 페이로드 보존용).
- `initializeConstructionPacket(header, packet)` (004df590) — 512바이트를 0xff 로 채우고 protocol 하위 7비트·message·선언 길이 WORD(최소 4)만 쓴다.
- `writeConstructionPacket(notice, header, packet)` (00444590→004df590/0040a840) — 초기화 뒤 SID 목록을 먼저 쓰고 나머지 필드를 쓴다. 512를 반환하며 512바이트 뒤 호출자 버퍼는 건드리지 않는다. 512바이트 미만 버퍼·SID 20개 이상은 효과 전에 거부한다. 좌표는 수 또는 비트(`xBits`/`yBits` 우선).
- `finalizeConstructionPacket(packet, sending, invalidCount)` (0040a800) — sending 이 거짓이면 아무것도 읽지 않고, 참이면 sidCount 255 의 원본 진단(`invalidCount`, 연결되지 않으면 길이를 쓰기 전에 거부)을 부른 뒤에도 선언 길이를 `sidCount×2+27` 로 쓴다.

`www/core/raw-construction-notice.js`

- `ConstructionNoticeState` — 통지가 읽는 전역: `localPlayer`(00540c70)·`authority`(00540bc4)·`moveLocalBuilder`(00544d10)·`daisType`(005412ec)·`priestType`(005412d0)·독립 값형 `history`(5좌표)/`historyCounter` 와 공유 getter(`sharedAuthority`·`sharedHistory{points,index}`·`sharedLocalPlayer`·`sharedDaisType`·`sharedPriestType`). typed 경로는 값형 필드를, 실제 wire 경로는 공유 getter 를 쓰는 `*Now()` 접근자를 소비 위치마다 읽는다(원본 C++ 과 같은 구분).
- `RawConstructionNotice(pool, types, state, hooks)` — 10.78 `SidPool`·타입 표 188칸·필수 경계 13개(`clear`·`place`·`moveBuilder`·`findPriest`·`immobile`·`approach`·`tryMove`·`refund`·`destroy`·`start`·`refresh`·`containedType`·`removeContained`)를 생성 때 확인한다(`actualPlace` 는 실제 wire 경로에서 처음 쓸 때 확인).
  - `handle(notice)` — 값형 통지. 타입 번호 표 밖·SID 0개/20개 이상/0/풀 밖은 정리나 이력 변경 전에 거부. 순서: `clear` → abstract(HP 보유 비트 또는 Dais 타입, `flags` 비트0 없음)·needsBuilder(0x8000) 계산 → group≠10 이고 로컬 플레이어 통지면 이력을 뒤에서부터 밀고 첫 칸에 새 좌표, 순번 0 → `place` → abstract 면: needsBuilder 는 로컬이 아니거나 moveLocalBuilder 일 때 `moveBuilder`, 아니고 권한이 있으면 첫 객체 genus 가 사제 이동 대상(0x404200)이면 [`findPriest`→`containedType`→`findPriest`→`removeContained`→사제 상태 low2=0·genus 사제·이동 가능이면 접근점(플래그 7, 실패 시 3)+`tryMove`, 못 움직이면 `refund`(첫 객체의 현재 TYPE·PATCH 소유자 +34)와 `destroy`] 아니면 `start(첫 SID, 0)` → `refresh`. 접근점은 `Snap`(0.9999899864196777f 더해 `ftol` 하위 DWORD→float)해서 `tryMove` 로 넘기며, 재시도에서도 사제 좌표를 새로 읽는다.
  - `actualHandle(packet)` — 4441b0 의 실제 BYTE 경로. 진입 때 TYPE·방향·인자를 고정하고 그 TYPE 레코드를 Clear 뒤에도 같이 읽으며, Clear 호출 직전 인자·Place 호출 직전 품질/개수/좌표/소유자/타입·Builder 이동의 시각/첫 SID/현재 소유자·사제 분기의 첫 SID 를 선택된 분기에서만 현재 패킷에서 읽는다. 배치는 `actualPlace(요청, 선언 개수, 현재 SID 읽기 함수)` 로 넘긴다. 사제 분기는 항상 `containedType`/`removeContained` 를 부르고(PATCH 전용), 실패 환불·삭제 뒤 `refresh` 하고 돌아온다.
- `makeConstructionNoticeHooks(pool, clear, place, hooks, priest)` — 정리(`clear.clear`)·배치(`place.place`/`place.actualPlace`)·환불(`place.refund`)과 priest 가 있으면 이동 불가(`priest.immobile`)를 같은 풀의 실제 모듈로 잇는다. 풀이 다르면 거부한다. 이름 계약만 확정했으며 실제 `RawConstructionClear`/`RawConstructionPlace` 이식 뒤 연결 시험한다.
- `makeConstructionNoticeConfirmHooks(pool, notice, hooks)` — 서버 확정의 직접 처리 경계(`handle`)를 같은 풀의 통지 처리기로 잇는다.

`tools/fixture-manifest.mjs` — `--write --only 아이디,아이디` 를 추가했다(지정한 항목과 아직 작업 사본 표시인 항목만 다시 계산). 전체 계산은 항목마다 git 조회를 하므로 같은 PC 에서 다른 작업이 돌 때 14분이 걸려서 단위마다 쓰기 어려웠다.

## 2. 검증

`tests/construction-packet.test.mjs`(5건):

1. `construction-packet-x86.tsv` 86행 전부(Write 72: 통지 10개 값·헤더 3개 값·SID 목록 → 512바이트 전체, Finalize 14: 송신 여부·최종화 전 512바이트 → 진단 1회[sidCount 255, 1행]/0회와 최종화 뒤 512바이트). 512바이트 뒤 0xcc 5바이트는 그대로다.
2. 좌표를 수로 주어도 같은 비트로 쓰이고 `xBits`/`yBits` 는 NaN 페이로드(`7fc00034`·`ffc00099`)를 보존한다.
3. 511바이트 버퍼·SID 20개를 효과 전에 거부하고 버퍼를 보존하며, 선언 길이 최소 4·protocol 상위 비트 제거.
4. 최종화: sending 0 은 빈 버퍼도 허용, 27바이트 미만 거부, 255 진단 없이 거부, 진단 뒤 길이 537(0x0219)·sidCount 3 이면 33.
5. 뷰: 필드를 현재 바이트에서 읽고(버퍼 변경 반영), 범위 밖 SID·6바이트 버퍼의 좌표를 거부한다.

`tests/construction-notice.test.mjs`(8건):

1. `constructionnotice-x86.tsv` 의 `originals` 537행(`originalCD`·`original1037` 은 다른 판본이라 제외) — 36개 입력(통지·전역·객체/사제 raw·접근점·이동 성공 여부·좌표 변동·이력 초기 순번)에서 효과 순서(Q/P/H/B/R/T/X/I/A/M/F/D/S/W), 배치 시점 이력(H)과 최종 이력 40바이트·순번이 모두 같다. y 좌표가 페이로드를 가진 조용한(quiet) NaN(비트 `2143294004`)인 1행도 비트가 그대로 같다.
2. 잘못된 통지 5종과 필수 경계 13개 각각의 누락·타입 표 171칸·다른 판본 거부(효과·이력 변경 없음), 확정 어댑터의 풀 불일치 거부.
3. 배치 경계에서 로컬 플레이어가 바뀌면 Builder 분기가 현재 값을 다시 읽는다(`;B:3:50:305419896:1074270772;W`).
4. 공유 이력 장부는 배치 전에 갱신되고(`index` 0, 첫 칸 새 좌표, 둘째 칸 이전 좌표) 독립 값형 사본은 변하지 않는다.
5. 배치 효과가 바꾼 권한(공유 boss)을 자동 건설 시작 관문에서 다시 읽는다(첫 호출 시작 0, 둘째 호출 시작 1).
6. 실제 wire 경로: 같은 관측 537행 전부를 `writeConstructionPacket` 으로 직렬화해 `actualHandle` 로 돌린 효과 순서·최종 이력·순번이 typed 경로와 같다.
7. 실제 wire 경로의 고정 TYPE 레코드(Clear 가 패킷 TYPE 을 바꿔도 진입 때 선택한 레코드)와 소비 위치마다 현재 패킷 읽기(Clear 뒤 TYPE/첫 SID 변경이 배치 요청과 자동 건설 시작에 반영), `actualPlace` 없음·패킷 TYPE 표 밖 거부.
8. 어댑터: 정리·배치·실제 wire 배치·환불·이동 불가 호출이 해당 모듈로 간다. 풀이 다른 모듈은 어느 쪽이든 거부, priest 가 없으면 기존 `immobile` 을 유지한다.

최초 실패와 교정: 새 모듈은 패킷 86행·통지 537행 전부를 첫 실행에 통과했다. 시험 쪽 기대(자동 건설 시작을 보려면 배치가 쓰는 첫 객체 genus 가 사제 이동 대상이 아니어야 한다)를 한 번 고쳤다.

## 3. 경계와 남은 것

- 통지 처리기의 실제 정리·배치(`RawConstructionClear`·`RawConstructionPlace`)와 확정 요청(`RawConstructionConfirm`/`Request`/`Local`/`Command`)은 별도 단위다. 어댑터는 이름 계약(`clear.clear`·`place.place`·`place.actualPlace`·`place.refund`·`pool`)만 시험했다.
- 10.72(CD) 경로(소유자 +32, contained 없음)와 10.37 은 범위 밖이다. 같은 cpppj 시험의 `ConstructionNotice_ReplaysCd`/`Extra1037` 은 옮기지 않았다.
- 통지의 건설 시작 경계는 E132 `makeConstructionProcessNoticeHooks`(지속 시간 비트 0 → 10초 건설)로 이어진다.

## 4. 다음 시작점

`RawConstructionEffects`·`RawConstructionClear`·`RawConstructionPlace`·`RawConstructionConfirm`·`Request`·`Local`·`Command`·`Data`·`Builder`·`Selection` 등 건설 계열 나머지(선행 순서: Effects/Clear → Place → Confirm → Request → Local/Command), `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
