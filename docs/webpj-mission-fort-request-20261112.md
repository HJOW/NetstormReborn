# webpj 미션 Fort 요청 — MissionFortRequest·MissionFortSource (WB04-E337, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획(미션 시작 경로)의 첫 두 파일인 client **MissionFortSource**(헤더 20줄)와 **MissionFortRequest**(기준선 `.h` 47 + `.cpp` 41 = 88줄, 기준선 뒤 `.h` +11줄)를 `core/mission-fort-source.js`·`core/mission-fort-request.js` 로 옮겼다. 앞 단위(E325~E336)처럼 **client 계약**을 모듈 머리글에 적고 시험이 그 모양의 장치를 만든다. 실제 `Client`·`State` 는 4-D E 구획에서 조립한다.

## 1. 규칙

- **writeMissionFortRequest(name, globals, hooks):** Init 이 State7(미션 시작)을 요청하는 몸체(원본 `004b51e0`). 순서는 ① 큰 Fort 선택 `hooks.selectFort(이름의 첫 NUL 앞)` → ② Zone 512BYTE 패킷 작성(`makeLocalMissionZone`, 선언 길이 4로 낮춤 — 원본 관찰기는 뒤의 복제 진입 때 길이를 확정) → ③ **선택이 끝난 뒤의** 현재 로컬 주소 9BYTE 를 요청 `+0x54`/`+0x58` 의 DWORD 두 개와 `+0x5c` 의 낮은 BYTE 로 복사(윗 3BYTE 는 stale 그대로) → ④ `+0 = 7`, `+0x7c = 현재 포트` → ⑤ `hooks.duplicate(packet)` 이 돌려준 Zone 핸들을 `+0x80` 에 → ⑥ 복제 효과 뒤의 현재 순번을 1 증가(32비트 감김). 요청 144BYTE(36DWORD) 중 이 필드 밖은 그대로 보존한다. 큰 효과가 주소·포트·순번을 바꿔도 그 뒤의 현재 값을 읽는다(C++ 참조를 `() => 포트`·`{ value }` 순번 상자·같은 저장소를 보는 `Uint8Array` 주소로 옮겼다).
  - 거부: 블록이 36DWORD `Uint32Array` 가 아니거나 순번이 상자가 아니거나 주소가 9BYTE 미만이면 아무것도 쓰기 전에 거부한다. 효과 누락은 도달한 위치에서만 요구한다 — 선택이 없으면 아무것도 쓰지 않고, 복제가 없으면 선택 뒤 주소·상태·포트까지만 쓴 채 핸들과 순번은 그대로 둔다.
- **captureMissionFortZone(request, packet):** 원본 `0040a1e0(1)` 의 길이 확정(15 + 첫 슬롯 수 × 71 을 하위 WORD 로 선언 길이에 기록)과 `004dece0` 의 소유 복제를 같은 버퍼에 수행한다. 복제 이미지는 요청이 단독 소유(패킷 저장소와 다른 버퍼)하고 핸들은 1 이다. 이미 소비됐거나 복제가 남아 있거나 패킷이 512BYTE 가 아니면 거부하고, 슬롯 수가 길이를 512 밖으로 보내면(8개 이상) `RangeError` 이며 복제를 만들지 않는다.
- **releaseMissionFortLoad(pending, requestBox):** Guide 교체/전체 종료에서 이전 미션의 지연 로드만 `LoadMissionFort` → `LoadMissionWorld` 순서로 취소하고 요청 참조를 놓는다. State 의 이전 World 반납에서는 호출하지 않는다. `pending` 은 client `State.cancel(이름)` 계약이다(State 클래스는 4-D 이후).
- **createMissionFortLoadRequest():** 원본 `MissionFortLoadRequest` 의 빈 값(144BYTE 요청 캡처·순번·Fort 이름·Zone 복제본·소비 표식·State7 앞단 캡처 `starting`).
- **MissionFortSource(`createMissionFortSourceImage`/`createMissionFortFileSelection`):** 미션이 선택한 같은 파일 읽기의 원문 이미지와 실제 Template 파일 이름을 묶는 값(헤더만). 선택은 이름만 만들고 읽기는 호출할 때만 같은 파일을 읽는다.

## 2. 웹 범위와 기준선 뒤 변경

- 기준선 뒤 `MissionFortRequest.h` +11줄(`a76499e3`, `GeneralBattleStartRequest`)은 **일반 LAN State7 요청**을 인계하는 구조체라 웹에서 옮기지 않는다(3.6절 ③ LAN 제외). 소스·시험이 쓰지 않는 단순 구조체이다.
- `WriteCurrentGuideFortRequest`·`GuideFortRequestHooks`(실제 Guide 가 살아 있는 State1 의 요청) 는 **호출자가 `UberEditor.inc:107` 한 곳뿐인 Edit 전용**이라 옮기지 않았다(편집기는 웹 제외). 이 때문에 `GuideFortRequestTests`·`guide-fort-request-x86.tsv`(R 144 + S 54행)와 그 밑의 `o/RawFortSubscriberChange`(PORT_MAP exclude-edit)도 이 단위에서 쓰지 않는다. 같은 헤더의 `makeGuideMissionZone`(E324)은 그대로 남아 있다.
- PORT_MAP: `MissionFortRequest.cpp` port(base `a76499e3` — 헤더 +11줄을 판정한 커밋), `MissionFortSource.h` port(헤더만). 앱 수명 연결(Init 이 요청을 만들고 State 지연 큐가 소비)은 B 구획 `Mission`·4-D 조립 때 한다(PORT_MAP session 0).

## 3. 검사 (`tests/mission-fort-request.test.mjs` 11건)

- **원본 독립 관측:** `mission-fort-request-x86.tsv` 144행(네 이름 슬롯 × 요청 전체 144BYTE 양옆 이웃 8BYTE × Fort 선택 뒤/복제 뒤 현재 주소·포트·순번 변형 3종): 요청 후 160BYTE·순번·Zone 512BYTE·복제 86BYTE·효과 호출 순서를 전부 대조한다. **첫 실제 실행에서 144행이 모두 일치했다.** manifest 에 `mission-fort-request` 항목을 추가했다(fixture 는 `cpppj/tests/fixtures` 를 직접 읽음).
- **cpppj 시험 이식:** `MissionFortRequest_Native144`·`MissionFortRequest_OwnedImageAndCurrentFields`(복제 직전 상태 7·포트·+0x5c 윗 3BYTE stale, 복제본 86BYTE 단독 소유, 중복 캡처 거부).
- **웹 계약:** 이름 첫 NUL 절단, 형식 거부 시 무변경, 효과 누락의 부분 쓰기, 순번 32비트 감김과 핸들 그대로 저장, Zone 캡처의 길이 경계(슬롯 0/2/4/7/8)·소비됨·크기 거부·복제 뒤 원본 변경 무영향, 지연 로드 취소 순서, 빈 요청 값, Edit 전용 Guide 요청 미이식 확인, MissionFortSource 값 검증.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 48개를 돌려 첫 실행에서 6개가 남았다. 시험을 보강(복제 전 선언 길이 4와 확정 후 86 단언, 슬롯 2·4 로 선언 길이 낮은/높은 BYTE 경계)한 뒤 **2개만 남고 모두 동등**하다.
  1. `Number(duplicate(packet)) >>> 0` → `| 0`: 값이 `Uint32Array` 에 저장되며 같은 비트로 감기므로 결과가 같다.
  2. `releaseMissionFortLoad` 의 `typeof pending?.cancel !== 'function'` 방어: 제거해도 `pending.cancel(...)` 호출이 같은 종류(`TypeError`)로 실패한다.
- 남은 경계: `starting`(State7 앞단 캡처)은 RawLocalMissionState(E320)가 쓰는 값 모양이며 이 모듈은 보관만 한다. 실제 State 지연 큐(`Post`/`Cancel`/`Take`)와 Init 호출자는 4-D 이후.
