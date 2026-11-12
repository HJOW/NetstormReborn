# webpj 의식 보상 선택 수신 RawRitualChoice (WB04-E366, 2026-11-13, 7-P3 신전·제단·의식 부품 두 번째 항목)

3.1절 7-P3 항목 "RawRitualChoice 27줄"을 `core/raw-ritual-choice.js` 로 옮겼다. 의식(제단 희생·신전 의식) 뒤 보상 선택 창에서 고른 종류를 알리는 6b 패킷의 수신 몸체(00448e90)이다: 부모(선택 제단)에 이미 달려 있는 이전 선택 Form 을 모두 지우고, 새 선택 Form 하나를 같은 부모에 만든다. cpppj 원본(`RawRitualChoice.cpp/.h`)은 기준선(20261106) 뒤 바뀐 적이 없다(마지막 변경 `1d9f4f647 20261102-110 CP`, PORT_MAP base `1f00a25a`).

## 1. 규칙

- **진입:** `receiveRitualChoice(pool, finderState, create, packet, destroy)` — `pool` 은 10.78 SidPool, `finderState` 는 `ContainedFinderState`, `create` 는 **같은 풀**의 `RawContainedCreate`, `packet` 은 최소 10바이트(`RITUAL_CHOICE_MIN_PACKET`), `destroy(form, flags)` 는 호출자가 주는 실제 가상 삭제 경계(삭제할 후보가 있을 때만 필요)이다. 판본이 PATCH 가 아니거나, 생성기의 풀이 다르거나, 패킷이 10바이트 미만이면 아무 것도 건드리기 전에 `TypeError` 로 거부한다.
- **부모와 free 관문:** 부모 SID 는 패킷 `[4..5]` 의 리틀엔디언 16비트이고 앞 4바이트(헤더)와 보낸 쪽은 읽지 않는다. 부모 슬롯 상태(+11)의 free 비트(1)가 서 있으면(다른 비트와 함께여도) 조용히 반환하며 finder·삭제·생성을 모두 건너뛴다. 소유자·보낸 쪽·TYPE 조건은 **원문에 없어** 덧붙이지 않았다. 풀 밖 부모 번호는 슬롯 접근에서 `RangeError` 이다.
- **이전 선택 Form 삭제:** `RawContainedFinder.begin(parent)`(dead 허용 아님 — checking 이 켜진 권한 없는 dead 부모는 finder 의 원문 진단 0x13c 가 먼저 불린다)로 부모의 현재 종속 목록을 순서대로 돌며, 현재 contained TYPE 후보 중 `+20` 바이트에 mask 4 가 선 것만 `pool.allocated(후보)[22] = 1`(삭제 표식)을 쓰고 `destroy(후보, 0)` 을 부른다. finder 가 후보의 다음 링크를 반환 전에 저장하므로 삭제 효과가 후보 슬롯과 링크를 지워도 순회가 이어진다. `+20` 의 WORD 상위 바이트(`+21`)만 4 인 후보, 다른 TYPE 후보, 4 비트 없는 후보는 건드리지 않는다(표식도 쓰지 않는다). free 상태 후보는 `allocated` 가 거부한다(삭제 안 함).
- **삭제 경계 누락:** 삭제할 후보가 있는데 `destroy` 가 함수가 아니면 **표식을 쓴 뒤** `Error('의식 선택의 실제 가상 삭제 미연결')` 로 거부하고 새 Form 은 만들지 않는다(원문 순서). 삭제할 후보가 없으면 `destroy` 없이도 생성한다.
- **새 선택 Form:** 모든 삭제 효과가 끝난 **뒤에** 패킷 `[6..9]` 의 DWORD(부호 없음)를 읽어 `create.create(parent, kind, 0, 4, 0, 1)`(인자 0·flags 4·부착 flags 0·품질 1)을 한 번 부른다. 삭제 경계가 패킷 바이트를 바꾸면 바뀐 값이 쓰인다. 종류는 검사하지 않으며 0 이나 32비트 가득 찬 값도 그대로 넘긴다. 새 Form 의 SID 는 반환하지 않는다.

## 2. 웹 범위

- 6b 는 다른 클라이언트에서 오는 패킷이기도 하지만, cpppj `RawCampaignSession` 은 6b 를 `ReceiveFixedCarrierPacket(0x00448e90)` 로 등록(길이 10, 버전 1)해 `ReceiveRitualChoice` 로 보내고, 선택 요청(`RitualChoiceInputs()` 의 `hooks.send`)은 `SendLocalPacket` 으로 **자기 세션에 바로 돌려보낸다**. 그래서 단독(비미션) 진행의 의식 보상 선택도 이 수신 몸체를 지난다. 요청을 만드는 클라이언트 쪽 `ritual-choice-input.js`(PORT_MAP 355, partial)와 보상 창·제단 연결은 4-D 클라이언트 선행 항목 몫이다.
- 10.72(CD)는 범위 밖이다(cpppj 도 10.78 실제 body 만 정적으로 작성). DOM 을 쓰지 않으며 실제 삭제(`world.Destroy`)·부착은 호출자 훅이다. 훅 공장은 필요 없어(생성기·삭제 경계를 호출자가 그대로 넘김) 만들지 않았다.

## 3. 검사 (`tests/ritual-choice.test.mjs` 15건)

- cpppj 에도 이 함수의 관측 fixture 가 없어(CMake 주석상 MP 6b 사전 구현, 새 MP 검사 없음) **소스 계약을 그대로 옮긴 계약 시험**이다. 실제 `SidPool`·`RawContainedFinder`·`RawContainedCreate` 를 쓰고 삭제·부착·생성만 기록 훅이다. 시험은 첫 실행에서 모두 통과했다(모듈은 시험으로 바뀌지 않았다).
- 입력 거부 5갈래(다른 풀 생성기·생성기 없음·9바이트·빈/없는 패킷·비 10.78 풀)와 아무 변화 없음, 10/11/64바이트 수용, free 관문(free 비트 단독/다른 비트와 함께/free 아닌 상태), mask 4 선별(체인 순서·표식 시점·다른 TYPE·`+21` 상위 바이트·비트 4 없는 값), 새 Form 필드(`+18` 종류 하위 WORD·`+12`=0·`+30`=1·`+20`=4·부착 0), 생성 인자 부호 없는 DWORD 전체와 바이트 위치, 부모 LE 16비트·헤더 무시·풀 밖 거부, 종류는 삭제 뒤에 읽기, 삭제 경계 누락(표식 후 거부·함수 아님·후보 없으면 허용), 삭제가 링크를 지워도 순회 계속, 현재 contained TYPE 전역 추적, free 후보 거부, 권한 없는 dead 부모의 finder 진단 호출/장치 없음 거부, 같은 부모에 세 번 받아 교체.

## 4. 변형 확인

- 연산자·상수 변형 29 + 의미 변형 25 = 54회를 적용했다. 처음에 남은 1개(부모 free 판정을 `& 1` 대신 `=== 1` 로 바꾼 변형 — free 비트가 다른 비트와 함께 선 부모)는 **시험의 공백**이라 free 비트 마스크 시험을 보강해 잡았다. 종류를 삭제 전에 읽는 순서 변형도 `종류는 삭제 효과가 끝난 뒤에 읽는다` 가 잡는다. 남은 변형은 없다.
- 다음 7-P3 항목은 **RawDaisRegular**(374줄 +12; RawAssetRegular·RawPathStart·RawTempleRitual 선행은 끝남)이다.
