# webpj 운반체 이동(RawCarrierMove) 이식 — WB04-E99

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔아홉 번째 부분이다. 운반체(Carrier)의 이동 요청(427340)·서버 수신(428570)과 AI/사용자가 공유하는 18바이트 이동 명령 작성기(40a850)를 cpppj 기준선 `1f00a25a` 의 `o/RawCarrierMove` 에서 옮겼다. E98 의 `raw-path-runtime.js`(경로 이미지)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-move.js` — `CarrierMoveState`, `writeCarrierMoveCommand(state, parent, destination, sacrifice, packet)`, `RawCarrierMove(pool, types, fort, state, hooks)`

- `writeCarrierMoveCommand`: 현재 요청 헤더(`state.request` = `{ message, protocol, length }`, 초기 0x40/1/18)로 18바이트를 쓴다. byte0 = protocol & 0x7f, byte1 = message, +2 length(WORD, 4 로 보정하지 않음), +4 부모 SID(WORD), +6/+10 목적지 float 비트(NaN 페이로드·−0 보존), +14 희생 DWORD. 저장소가 18바이트 미만이면 쓰기 전에 거부하고 뒤 바이트는 보존한다.
- `request(parent, destination, sacrifice)`: 현재 선택이 이 부모면 먼저 선택을 해제한다(`select(0, 0, 1)`). genus(flags2)에 0x30000(이동/운반 계열)이 없으면 진단(0x571)을 부르고 끝낸다. 비행(0x20000)이 아니면 부모 위치에서 목적지까지 지면 연결(`connected`)이 있어야 한다. 이동 종류(`movement`)→경로(`path`) 조회 뒤 기존 경로의 목적지 float 쌍(+0x186dc/+0x186e0)이 요청과 같으면 억제하고, 아니면 18바이트 명령을 만들어 송신(`send(9, packet)`)한다. 서버를 직접 실행하지 않는다.
- `receive(owner, packet)`: 패킷이 18바이트 미만이면 거부한다. 부모(WORD +4, 효과마다 다시 읽음)가 삭제/죽음(state&3)이거나 이동 계열이 아니거나 정지(0x5105)·차단(0x493) 사건이 있으면 끝낸다. 소유자가 다르면 fort 모드(편집 전역과 별개)에서만 허용한다. 포획 후보가 없을 때: 귀환 장소가 있으면 접근(`target`)만, 없으면 간헐천(genus 0x10000000)의 경로에 대상을 쓰거나(경로가 있으면 끝), 간헐천이 없으면 제단(genus 0x400000)이 있고 부모가 제단 계열이며 허용되고 패킷의 희생 DWORD 가 0 이 아니면 희생을 시작한다. 그 밖은 경로 시작(`start`, 마지막 인자 reserved 1)을 부르고 경로가 만들어지면 이동 회피 깊이 0·대상 (0, 0)을 쓴다. 패킷 필드와 부모 TYPE 은 외부 효과 뒤 원본 위치에서 다시 읽는다.
- 도달한 훅이 없으면 오류이다(`selected·select·assertion·connected·movement·path·send·regular·candidate·returnPlace·target·genusAt·geyser·altarAllowed·sacrifice·start`).

## 2. 검증

- `tests/carrier-move.test.mjs`(3건):
  1. `carrier-move-x86.tsv` 982행(10.78 전용 단일 판본, 53/64비트 열 동일). 입력 26 DWORD(연산 0 요청/1 수신·TYPE flags2·부모 상태·소유자·보낸 소유자·fort·선택·연결·경로·후보·귀환·간헐천/제단 genus·허용·희생·정지/차단 사건·변이 20종·목적지·경로 목적지·헤더)로 외부 효과 순서(S 선택/C 해제/I 진단/L 연결/M 이동 종류/P 경로/E 사건/Q 후보/R 귀환/D 접근/G genus/Y 간헐천 경로/A 허용/F 희생/B 시작/T 송신)·현재 전역(fort·TYPE flags2·요청 헤더 세 값)·송신 패킷 hex·두 부모 슬롯 raw·경로 이미지(앞 0x186ec바이트)의 Adler-32 가 같다.
  2. 짧은 수신 패킷(17바이트)·미연결 하위·판본/TYPE 표 크기의 지연 거부.
  3. 공유 작성기의 헤더(protocol 의 bit7 제거)·NaN 페이로드(0x7fc01234)와 −0 의 float 비트·꼬리 바이트 보존·짧은 버퍼 쓰기 전 거부.

최초 실패와 교정: 새 모듈은 위 fixture 982행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 선택·연결·경로 조회·후보·귀환 장소·genus 조회·간헐천 경로·희생 의식·경로 시작(`RawPathStart`)·송신은 훅 경계이다. 경로 시작과 도착(`RawPathStart`/`RawPathProcess`/`RawPathData`)은 후속 단위이고 AI 의 이동 명령 호출자도 이 작성기를 쓴다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`·`RawPathRetarget`, `RawCarrierTarget`/`Choice`/`Escape`(운반체 표적/선택/탈출), `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
