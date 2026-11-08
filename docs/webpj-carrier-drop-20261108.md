# webpj 골렘 Cast/Drop(RawCarrierDrop) 이식 — WB04-E93

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔세 번째 부분이다. 골렘(Carrier)의 Cast/Drop 제어 흐름 — 요청(`o/RawCarrierDrop`: 427070)·메뉴 명령(42aa50)·서버 수신(444bc0) — 을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 내용물 검색은 E56 의 `raw-contained-finder.js` 를 재사용한다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-drop.js` — `RawCarrierDrop(pool, types, contained, state, hooks)`

- `request(parent, content = 0)`: content 0 이면 부모의 첫 내용물부터 주문(내용물 kind 의 flags2&0x100)을 건너뛴 첫 비주문을 고른다(없으면 반환). 주문이면 부모 extra bit20 → 지면 정지(`immobile`) → 비용 승인(`afford`, float)을 거친 뒤 **현재 비용을 다시 읽어** 음수 비용(signed32 감김)으로 SP 차감 → 카메라 투영 좌표로 비용 표시 → 0.1초 Regular 예약 → 부착 체인에서 통지 프로세스 타입에 통지(3, 0, 0) → 선택 해제를 하고, flags2&0x8000 이면 내용물 +22 를 1 로 표시한 뒤 9바이트 요청 `[프로토콜&0x7f, 메시지, 길이(2), 부모(2), 내용물(2), +22 여부]` 를 송신한다.
- `command(parent, code, listParent)`: 부모 소유자가 로컬이면 선택(1)·시전(처음 내용물 SID 를 **보관한 채** 플래그를 검색하고 보관한 SID 로 요청)·내려놓기·포획 명령을 처리해 1 을 돌려주고, 그 밖은 공통 메뉴(`baseMenu`)로 넘긴다.
- `receive(sender, packet)`: 9바이트 미만 거부. 서버가 아니면 진단(0x4cf) 뒤 계속. 부모 dead/free, 내용물 free/dead/void 이거나 내용물 TYPE 불일치면 반환. 내용물 kind 가 0 이거나 TYPE 수 이상이면(checking 이면 진단 0x4e8) 반환. 내려놓기 위치가 맵 안 (0, 한계)가 아니면 반환, 맞으면 정수 절삭해 입력 큐 명령을 보낸 뒤: 주문이면 송신자 불일치 거부·비용(로컬이면 0)보다 돈이 적으면 환불 통지(중단하지 않음)·이동 프로세스 제거·0.1초 예약·분리/Unpop·주문 생성/소유자/QA/Pop·활성화 6바이트 로컬 수신 뒤 배포. 아니면 일반 배치가 성공할 때 내용물 +22 를 패킷 값으로 표시하고 파괴한 뒤 부모가 비행 genus(0x20000)가 아니고 정지도 아니면 회피(1.0, −1). 패킷·부모 소유자는 효과 뒤 다시 읽는다.
- 큰 월드/프로세스/장치 효과는 훅이며 선택된 경계의 누락은 도달 시 거부한다(거부 관문에서는 요구하지 않음).

## 2. 검증

- `tests/carrier-drop.test.mjs`(2건): `carrier-drop-x86.tsv` 413행(평탄 JSON 입력: 요청/메뉴/서버 수신 세 종류, 소유자·서버·checking·TYPE 수·헤더·내용물 종류/플래그/표시·연결 체인 길이·genus·비용·돈·승인·정지·생성·외부 변이 6종)의 외부 효과 순서(29종 경계 표식)·송신 패킷 hex·일곱 슬롯 전체 raw 가 같다. 계약: 거부 관문에서는 도달하지 않은 경계를 요구하지 않고, 짧은 패킷·메뉴 공통 경계 누락·CD 풀·TYPE 크기를 거부한다.

최초 실패와 교정: 시험 장면의 활성화/요청 헤더 기본값을 `{ message, protocol, length }` 순서가 아니라 반대로 적어 활성화 패킷 앞 두 바이트가 뒤바뀌었다. 시험 입력의 필드 순서를 바로잡았다(제품 코드는 변경 없음).

## 3. 경계와 남은 것

- 주문 비용 복호화(41df60)·승인(425e90)·SP 증감(41dff0)·화면 투영(497220)·표시(43dd20)·Regular 예약(425ef0)·부착 순회(425e60)·프로세스 통지·선택(4d5c90)·송신(4e2dc0)·포획(428e00)·공통 메뉴(4ae010)·내려놓기 위치(426a70)·입력 큐(452fe0)·환불(442b50)·생성/배치(4af530/444760)·파괴·정지(40f100)·회피(429010)의 실제 본체 연결은 해당 단위에서 한다. 내려놓기 위치 계산(`RawCarrierDropPoint`, client)은 별도 단위이다.

## 4. 다음 시작점

`RawCarrierDropPoint`(client), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
