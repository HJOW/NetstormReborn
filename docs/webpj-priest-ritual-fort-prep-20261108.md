# webpj 사제 의식 프레임·의식 동작·Fort 사제 준비(RawPriestRitualFrame·RawPriestRitualActions·RawFortPriestPreparation) 이식 — WB04-E70

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔 번째 부분이다. 사제 의식(신전 기도 애니메이션)과 일반 Fort 설치 때의 사제 준비 세 파일을 cpppj 기준선 `1f00a25a` 의 `o/RawPriestRitualFrame`·`o/RawPriestRitualActions`·`o/RawFortPriestPreparation` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 프레임/inline 지도, 별도 명령 구현)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-priest-ritual-frame.js` — `RawPriestRitualFrame(pool, frames, hooks)`

- `tick(사제)`(00494160 과 실제 도달하는 내부 49417a, 604 사건): 현재 프레임의 방향(side − 'A')이 K..O(10..14)가 아니면 아무 효과 없이 0 을 반환한다. 맞으면 그 글자 구간의 첫 번호와 개수로 다음 프레임 = 현재 + 1(DWORD 감김)을 계산하고, `다음 − first` 가 signed 로 count 이상이면 `first + 4` 로 되돌린다. 다음 프레임이 `first + 14` 이고 현재 605 사건이 없으면 조회 뒤의 현재 좌표로 `beamOver03.wav` 를 한 번 요청한 뒤, 다음 프레임을 지정하고 0.08초(float32)를 반환한다.

`www/core/raw-priest-ritual-actions.js` — `RawPriestRitualActions(pool, frames, hooks)`, `makePriestRitualActionHooks`

- `begin(sid, 단계)`(00491ca0): 유효 번호(1..capacity−1, WORD 범위)만 현재 raw 타입의 `K + 단계` 글자 구간 첫 프레임으로 바꾼다(free/dead/void 를 별도 조건으로 쓰지 않음). 단계 0..5 밖은 거부, SID 0·범위 밖은 자연 반환한다.
- `continue(sid, 단계)`(00491cf0): 같은 범위 검사 → `begin` → 예약(new 0x28) 성공 시 604(0x25c, payload float32 0.01) 공유 Regular 를 예약한다. 기존 같은 사건은 취소하지 않고 프레임 효과 뒤 상태가 달라도 처음 유효했던 번호로 계속 요청한다.
- `makePriestRitualActionHooks(pool, frame, host, hooks)`: 프레임 지정을 실제 `SquidFrame.set`(E59)에, 공유 예약을 `SquidProcessHost.addSharedRegular` 에 연결한다(다른 풀 거부, 예약 확보는 호출자가 공급).

`www/core/raw-fort-priest-preparation.js` — `RawFortPriestPreparation(pool, priests, globals, hooks)`

- `run()`(패치 004923e0): 미션 후처리 전역이 꺼져 있으면 기술 네 가지(앞 세 개 기본 TYPE, 네 번째 `subscriber % 3` 의 테마 TYPE)를 TYPE 먼저·현재 기술 부모 나중 순서로 읽어 같은 TYPE 내용물이 없으면 만든다. 편집기이거나 미션 후처리이면 끝, 고정 Template 의 State+17 이 켜져 있어도 끝. 그 밖에는 Territory 의 20×6 BYTE 기록 중 BYTE1 bit0 이 켜진 지역마다 섬 번호(127 이면 건너뜀)를 조회해 사제 목록을 모아(`collect`, 최대 20개, 콜백이 바꾼 개수/항목을 매번 다시 읽음) bit 0x80 표시(`marked`)가 있는 첫 항목을 고르고, 표시도 목록도 없으면 초기 배치, 아니면 표시가 없을 때 첫 항목의 소유자를 지정한 뒤 같은 번호를 안정 제거하고 남은 사제마다 소유자 0 을 지정하며 남은 수가 양수이면 안내한다. 전역(`globals`)은 속성을 매번 읽으므로 getter 로 줄 수 있다.

## 2. 검증

- `tests/priest-ritual-frame.test.mjs`(2건): `priest-ritual-frame-x86.tsv` 1050행(반환 float 4바이트, 호출 기록 E/S/F, 사제 슬롯 50 raw; 모드 0 은 원본 `netstorm.tarc` 의 사제 타입 정의에서 만든 실제 코드표), 연결 오류·범위 밖 타입 거부.
- `tests/priest-ritual-actions.test.mjs`(2건): `priestritualactions-x86.tsv` 10.78 876행(프레임 지정/예약/604 공유 예약 호출 기록과 지정 슬롯 raw; 실제 `SquidFactory` 의 client 생성과 서버 영역 수신 Take 로 슬롯 확보), 실제 `SquidFrame` 쓰기/표시 요청과 같은 `SquidProcessHost`/`Kernel` 의 604 공유 예약·중복 예약·범위 밖 SID·손상 단계·누락 효과 거부.
- `tests/fort-priest-preparation.test.mjs`(2건): `fort-priest-preparation-x86.tsv` 13행(라벨별 하위 호출 trace 와 마지막 missionPost), 선택된 기술 생성의 예외가 앞선 효과를 보존하고 뒤 장치 누락을 미리 요구하지 않음과 손상된 목록 SID 의 접근 시 거부.

최초 실패와 교정: 새 모듈은 위 fixture 1939행 전부를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 기술 생성, 섬 조회, 소유자 지정, 초기 배치(`populate`), 안내와 의식 장면의 실제 연결(신전 의식 → 604 진행 → `RawPriestRitualFrame`)은 해당 파일 이식과 월드 조립 때 대조한다.

## 4. 다음 시작점

사제 배치 계열(`RawPriestPlacement*`), `RawAssetPostPop`, 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission` 이다.
