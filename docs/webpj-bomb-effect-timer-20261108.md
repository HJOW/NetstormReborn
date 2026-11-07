# webpj Bomb 상태 타이머·가변 패킷(RawBombEffectTimer·RawBombTimerPacket) 이식 — WB04-E52

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰두 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBombEffectTimer`(10.78; 원본 420a40 생성/00420960 기본 생성/41e100 Frame)와 `o/RawBombTimerPacket`(41e280 writer, 41e230 누적 읽기, 40b390 송신 observer, 41e0a0 부모 선택, 41e320 신규 수신)을 옮겼다. 117/118 상태 주문(은신/마비)의 대상 목록·기한·만료 해제를 다룬다. 다음 단위의 `RawBombStatus`가 이 패킷 observer를 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bomb-effect-timer.js`

- `BombEffectTimerFields`: 순서·중복을 보존하는 `members`(DWORD SID), `flags`(1 은닉 extra `0x10`, 2 마비 extra `0x20`, 상위 비트 보존), 고정 `capacity`(100), 기한 `endTime`(double)과 정확한 `endBits`(BigInt). JS Number는 NaN 페이로드를 보장하지 않으므로 비트를 같이 두고, `endTime`을 대입하면 비트를 수치에서 다시 만든다.
- `new RawBombEffectTimer(pool, types, now, hooks)`: `now`는 호출마다 현재 게임 시각을 돌려주는 함수(효과 뒤에도 현재 값을 읽음). 훅 `{unpop, repop, refresh, fence}`는 도달한 지점에서만 요구한다.
  - `constructEmpty(initialFlags = 0)`: 빈 100칸·기한 0(미작성 flags는 호출자 heap 입력 그대로).
  - `construct(delay, members, flags)`: 기한 = `(int32)delay + 현재 시각`, 목록은 최대 100개 복사(중복·비활성 미리 거르지 않음).
  - `frame(fields, finish)`: ① 현재 목록을 돌며 상태 비트 7이 없는 대상마다 `unpop(0)` → `repop(0x40)`(만료와 무관) ② `endTime <= now`가 거짓이면 종료(NaN 포함) ③ 목록을 다시 읽으며 은닉(flags 1·extra `0x10`)은 비트를 지우고 `refresh`, 마비(flags 2·extra `0x20`)는 비트를 지운 뒤 **현재 TYPE**이 genus `0x200000`이 아니면 `refresh`, 맞으면 `unpop`/`repop`, 현재 TYPE이 136이면 `fence` ④ `finish()`.
- `BombEffectTimerProcess extends SquidProcess`: 기본 생성(`initialFlags`)은 Init 없이 빈 목록·기한 0, 정적 `withTimer(…)`가 지연/목록/flags 생성(원본 420a40). `runFrame()`은 `frame`을 부르고 `finish`에서 부착한 호스트의 `kill(this, 0)`을 호출(호스트가 없으면 오류). `notify()`는 빈 함수(VT 50330c+1c). `readPacket`/`writePacket`은 현재 form/부모로 아래 함수를 부른다.

`www/core/raw-bomb-timer-packet.js`

- `writeBombTimerPacket(fields, form, parent, packet, declaredLength, protocol = 1, message = 0x7d)`: `[protocol&0x7f, message, 선언 길이 WORD(4 미만은 4), form, 부모, 기한 double 8바이트, 대상 수 WORD, flags DWORD, 대상 WORD 목록]`, 반환 길이는 `22 + 2×수`이며 헤더 선언 길이를 반환 길이로 덮어쓰지 않는다. 대상 수는 하위 WORD, SID는 이 경계에서만 WORD로 자른다.
- `readBombTimerPacket(fields, packet)`: flags를 먼저 쓰고 대상 수만큼 WORD를 읽어 `capacity`까지만 덧붙이며(가득 찬 뒤에도 입력은 끝까지 읽음) 기한 비트를 복사. 길이가 모자라면 앞선 쓰기는 남기고 거부.
- `finalizeBombTimerPacket(packet, writing)`: 송신일 때만 선언 길이 = `(수 + 11) × 2`(수 65535는 원문 진단 bda).
- `bombTimerParent(pool, types, members)`: 목록 순서로 현재 TYPE genus `0x40000000`인 첫 물리 SID(상태 필터 없음, 없으면 0).
- `receiveBombTimerPacket(pool, state, packet, hooks)`: `state = {boss, processType}`는 현재 값. boss면 진단 cc → 패킷 form이 점유 Form이 아니면 진단 e6 뒤 종료 → `create(processType, 0, 0)` → `read(token, packet)` → `attach(token, 현재 processType, 패킷 부모, 현재 패킷 form, 0)`.

## 2. 검증 (`tests/bomb-effect-timer.test.mjs`, 5건)

독립 원본 관측 3종(10.78 단일 관측):

- `bomb-effect-timer-x86.tsv` 4416행: 은닉/마비 flags × 상태 × extra × genus × 대상 TYPE × 지연(NaN 기한 포함) × 목록(없음/단일/중복/비활성 포함) × 콜백 변형(목록 첫 항목 교체·기한 연장·flags/extra 변경·TYPE 136 교체·상태 비트 추가). 효과 문자열, 최종 기한 비트, 목록, 세 슬롯 전체.
- `bomb-timer-constructor-x86.tsv` 12행: 기본 생성이 Init을 실행하지 않음(시각 0/100.25/NaN, 생성 후 시각 변경에도 기한 0), 미작성 flags, 부착 전 form/부모 0, 풀 자유 슬롯 불변, 미사용 목록 칸의 원문 바이트.
- `bomb-timer-packet-x86.tsv` 602행: C(생성) · W(writer, 수·선언 길이·프로토콜 비트) · R(포화 뒤 반복 수신, NaN 기한 비트) · A(수신: boss 진단·비점유 Form·생성 뒤 종류 변경·읽기 뒤 패킷 form/부모 변경) · P(부모 선택).

계약: 짧은 버퍼/손상된 입력 거부, 송신 observer의 선언 길이 확정, 수신 훅 누락, 구성 오류, 호스트 없는 프로세스의 만료 거부와 통지/패킷 기록.

최초 실패와 교정: 모듈은 5030행 전부를 첫 실행에 통과했다. NaN 기한 페이로드(`0x7ff8000000001234`)가 필요해 `endBits`를 같이 두는 설계로 시작했다.

## 3. 경계

- 실제 현재 가상 호출(48/4c/88), 썬 바리케이드 재연결(457890→457460), factory 생성(현재 TYPE+1d0)·부착(41bd20)·진단은 월드 조립에서 공급한다. `BombEffectTimerProcess.runFrame`의 호스트 `kill`은 `SquidProcessHost`와 함께 도는 시험을 월드 조립에서 한다(여기서는 호스트 없음 거부만 확인).
- `bomb-packet-x86.tsv`는 `CampaignSessionTests`(송신 observer)가 읽으므로 세션 조립 단위에서 대조한다.

## 4. 다음 시작점

`RawBombStatus`(+`bomb-status-x86` 612행, 이 단위의 `finalizeBombTimerPacket`을 H 모드로 사용)와 Bomb 나머지(`RawBombRegular`·`RawBombBlast`·`RawBombTargets`·`RawBombEffect`), Player 나머지, 종류별 PostPop 본체, `SquidDisplay`/`SquidFrame`이다.
