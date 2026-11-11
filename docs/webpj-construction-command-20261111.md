# webpj 건설 요청 0x3e 작성·서버 수신(RawConstructionCommand) — WB04-E322 (2026-11-11, 4-C)

3.1절 4-C 의 **RawConstructionCommand**(`o/`, 기준선 97줄, 기준선 뒤 +6줄)를 `core/raw-construction-command.js` 로 옮겼다. 로컬 건설 요청 0x3e 의 25바이트 직렬화(PATCH 0052ede6/7/8 메시지·프로토콜·선언 길이, 40a7c0/4433b0 작성)와 서버 수신 444af0 이다. 이미 옮긴 `RawConstructionLocal`(서버 요청 본문을 만드는 쪽)·`RawConstructionConfirm`(Request 이후 확정)과 `RawPacketDispatch`(등록) 사이를 잇는 단위이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 이 모듈은 DOM 을 쓰지 않는다.

## 1. 규칙

- `writeConstructionCommand(message, packet, header?)`: 25바이트를 쓰고 25 를 돌려준다. 위치는 `[0]` 프로토콜 하위 7비트(상위 비트 제거), `[1]` 메시지 0x3e, `[2..3]` 선언 길이(WORD, 기본 25), `[4]` 타입 BYTE, `[5..8]` x float, `[9..12]` y float, `[13..14]` 인자 WORD, `[15]` 방향 BYTE, `[16]` 품질 BYTE, `[17..20]`·`[21..24]` 시각 두 DWORD(메시지 생성자의 0, 로컬 배치는 덮어쓰지 않음)이다. 각 필드는 폭만 쓰므로 큰 값은 잘리고(타입 0x1ff → 0xff, 인자 0x12345 → 0x2345), 25바이트 뒤 호출자 버퍼는 보존한다. 저장소가 25바이트 미만이거나 `Uint8Array` 가 아니면 아무것도 쓰지 않고 거부한다. float 는 단정도로 반올림해 쓴다.
- `new RawConstructionCommand(types, state, hooks)`: `types` 는 `cost` 를 가진 타입 레코드 정확히 188개(PATCH), `state` 는 속성으로 매번 현재값을 읽는 객체, `hooks` 는 실제 하위 네 개(`money`·`shortfall`·`request`·`reject`) 전부 필수이다(고정 성공값을 공급하지 않음). `state` = `blocked`(558e74, DWORD 비영이면 본문도 읽지 않고 반환)·`chargeBlocked`(594fa4)·`localPlayer`(540c70)·선택 `chargeBlockedRead()`(있으면 `chargeBlocked` 대신 실제 전역을 비용 관문에서 조회).
- `receive(sender, packet)`(444af0): 차단 → 본문 17바이트 이상(미만이면 거부) → 패킷의 현재 타입 번호가 표 안(`>= 188` 이면 거부; 비용 관문 전역을 읽기 전) → 비용 관문 전역 조회 → **차단이 아니고 sender 가 로컬 번호와 다를 때만**(전체 DWORD 비교, BYTE 로 줄이지 않음) `money(sender)` 를 부르고 **호출 뒤의 현재 타입 비용**과 ordered less 비교(`money < cost`; NaN 잔액/비용은 부족이 아님, 잔액은 단정도로 반올림) → 부족이면 `shortfall` 뒤 반환 → 아니면 `request` → 거짓이면 `reject`. 하위 호출마다 패킷의 현재 필드(타입·x·y·인자 WORD·방향·품질)를 다시 읽으며 sender 는 부호 없는 DWORD, 플래그와 시각 8바이트는 읽지 않고 0 이다.
- `registerConstructionCommand(dispatch, command, prepare?)`: 0x3e 의 빈 observer(종류만 등록)·버전 1/정책 0·길이 25 를 등록하고, 수신 처리기는 `prepare` 가 있으면 먼저 불러 Session 의 같은 현재 전역을 공급한 뒤 분배기가 넘긴 sender 로 `receive` 한다.

## 2. 기준선 뒤 변경 판정 (+6줄)

- **옮김(② 적용)**: `5f4b9d304`(현재 건설 전역과 공유 마스크 및 효과 뒤 조회 순서): 차단·로컬 번호를 속성으로 매번 읽고 비용 관문은 `chargeBlockedRead` 로 현재 전역을 조회하며 음수 sender 와 TYPE/차단 순서를 보존한다.
- 옮기지 않은 변경은 없다. 실제 `money`·`shortfall`·`request`·`reject` 몸체(40ef30·442b50·444760·442af0)는 hooks 이며 Request 이후의 확정은 이미 옮긴 `RawConstructionConfirm` 이 맡는다. CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-construction-command.test.mjs` 8건)

- **원본 독립 관측** `construction-command-write-x86.tsv` 64행(25바이트 전송 버퍼와 호출자 뒤 0xa5 보존)과 `construction-command-receive-x86.tsv` 144행(차단/로컬/부족/NaN 잔액·NaN 비용/Request 반환값과 Money·Request 호출 도중 패킷·비용 변경 2종의 하위 호출 인자·순서; 이벤트 열의 `T:` 표식은 직접 TYPE 조회라 외부 호출이 없어 제외)을 전부 재생한다. 첫 실행에서 일치했다.
- 손 시험(cpppj `ConstructionCommandTests.cpp` 이식 + 경계): 실제 분배기 등록(길이 25·버전·빈 observer·선언 길이 24 거부·protocol 상위 비트 허용·준비 순서·음수 sender), 현재 비용 관문 재조회와 TYPE 범위 순서(188·255 거부, 187 허용, 차단 시 본문 무시), 송신 버퍼의 헤더·필드 폭·시각·저장소 크기, 수신 세부(DWORD 차단/로컬 비교·잔액 반올림·NaN·`-Infinity`·본문 17바이트 경계·요청 본문 형식), 구성 검사, 등록 수신기의 준비 순서와 sender 인자.

## 4. 변형 확인과 남은 경계

- 변형 76개 중 첫 실행에서 9개가 남았고 시험을 보강한 뒤 **2개만 남았으며 모두 동등**하다: 송신 인자 WORD 를 4바이트로 쓰는 변형(바로 뒤에서 방향/품질이 같은 자리를 덮어씀)·부족 요청을 만들 때 sender 를 부호 없이 바꾸는 위치(읽기 함수가 이미 부호 없는 DWORD 로 바꿈).
- 보강한 경계: 잔액의 단정도 반올림(`9.99999999` → 10 이라 부족이 아님), 비용을 패킷의 현재 타입 번호로 읽기, 타입 표·상태·패킷의 종류 검사, 등록 처리기의 준비 순서와 sender 인자.
- 로컬 sender/차단 값은 C++ 에서 `uint32_t` 이므로 JS 의 32비트를 넘는 수는 DWORD 로 감겨 비교한다(예: 2³²+1 은 1).
- 3.1절 4-C 는 19/30 이다. 다음: RawGeyserMined(간헐천 채집 통지, partial 142줄).
