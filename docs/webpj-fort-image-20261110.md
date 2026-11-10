# webpj Fort 이미지 직렬화(RawFortImage) — WB04-E277 (2026-11-10, 4-B)

3.1절 4-B 의 **RawFortImage**(87줄)를 `core/raw-fort-image.js` 로 옮겼다. 현재 Template 의 35개 원문 섹션을 PATCH DataManager SaveImage 형식(원본 `004bf520` → `0044d640` / `004bcd80`)으로 직렬화한다. 기준선(`1f00a25a`) 뒤 `RawFortImage.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 형식과 규칙

- 첫 바이트 `0x46`, 둘째 바이트는 파일 flag(`0xfe` 만 남고 그 밖은 0; 실제 manager 의 서명 flag 를 주는 `signatureFlag` 훅이 있으면 그 값), 이어서 고정 35 이름 순서로 `[길이 WORD(자기 2바이트 포함, 낮은 바이트 먼저) + payload]`. 없는 tail 섹션도 빈 stream 의 길이 2 를 쓴다.
- 원본 `0x8000` 메모리 stream 의 고정 배열 전체를 0 으로 시작하고 논리 끝(`end`)과 할당의 0 tail 을 구별한다. 이전 길이 필드는 다음 섹션 진입 직전과 마지막 반환 직전에 수정한다.
- 진단 3종(선택된 위치에서만 알린 뒤 계속): 현재 타입 개수 **signed** ≥ 256, 길이 필드 위치가 용량을 넘음(알린 뒤 물리 범위 거부), payload 가 남은 공간 이상(엄격한 `<` 진단: 정확히 32768 인 쓰기는 막지 않고 초과만 거부).
- 해석된 Money/Technology 필드나 예전 파일 바이트로 대체하지 않고 현재 raw 섹션을 끝까지 원문 그대로 쓴다. 10.78(`patch1078`) 전용이며 섹션은 35개 이하, 현재 타입 개수 장치가 필수이다.

## 2. 검사 (`tests/fort-image.test.mjs` 3건)

- `fort-image-packets-x86.tsv` 의 이미지 행 **116개 전부**: 합성 payload 패턴(0 · 가변 · 32696 · 32695 바이트)과 실제 보호 Fort 8개(Early Missions 여섯 + 캠페인 1-1/1-2)의 현재 섹션(웹 `parseFort`)을 입력해 이미지 전체 바이트와 외부 읽기/진단 순서(타입 개수 읽기 G · 진단 types/section/stream)를 대조하고 논리 끝 밖 고정 할당이 0 인지 확인했다. 첫 실행에 모두 통과했다.
- 손 시험(cpppj `FortImage_CurrentProtectedSections` · `PhysicalCapacityAndRequiredConsumers`): 해석 필드(Money)를 바꿔도 같은 이미지, 섹션 12 변경이 반영됨, 이미지를 `parseFort` 로 다시 읽으면 같은 섹션, 정확히 32768 은 섹션 진단만, 한 바이트 초과는 거부, 진단 소비자·판본·타입 개수 장치·섹션 수 계약.
- 변형 확인 14개 중 3개가 처음에 살아남아 손 시험으로 잡았다: 타입 개수의 signed 비교(0x80000000), 길이 필드 위치가 용량 끝(32766)에 정확히 닿는 경계와 한 바이트 넘는 경계(stream 진단), payload 초과의 거부 메시지(`Uint8Array.set` 의 자체 범위 검사가 같은 종류 오류를 내서 메시지까지 확인).

## 3. 남은 경계

- 같은 `fort-image-packets-x86.tsv` 의 `S`(SendFort 93행)·`F`(FinalizeMissionFortPacket 10행)는 **4-C 로그인 패킷**(RawMissionLoginPackets)에서 이 이미지 직렬화를 `imageHooks` 로 쓰며 대조한다.
- 실제 manager 의 이름 검색/current 효과(`section`/`signatureFlag` 훅)는 DataManager 이미지(RawDataManagerImage) · Template 항목에서 연결한다.
- 3.1절 4-B 는 6/30 이다. 다음은 **RawFortMoneyLoad**(163줄)이다.
