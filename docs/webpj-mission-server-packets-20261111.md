# webpj 미션 서버 패킷(RawMissionServerPackets) — WB04-E315 (2026-11-11, 4-C)

3.1절 4-C 의 **RawMissionServerPackets**(`o/`, 기준선 119줄 = `.cpp` 76 + `.h` 43, 기준선 뒤 +27줄)를 `core/raw-mission-server-packets.js` 로 옮겼다. 로그인 서버의 현재 Player(0x2b)/중앙섬 metadata(0x72) 작성과 원본 송신 순서(10.78: 0048f9f0, 004a6b40, 미션 Zone 의 004b3bc0 수신 자연 경로, 40b110 observer)이다. 이름 정제와 Player 송신 observer 는 앞 단위 [RawMissionClientPackets](webpj-mission-client-packets-20261111.md)의 같은 몸체를 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawMissionServerPackets(players, state, hooks)`: `players` 는 Player9 1620바이트(이름 정제 결과도 직접 반영), `state` 는 원본 현재 전역 묶음(`server`·`metamap`·`localOwner`·`centralCreated`·`archipelago`·`zoneAddress` 6바이트; 읽는 시점마다 현재값 조회), `hooks` 는 큰 전송/주소 포맷과 중앙섬 생성만 실제 외부 하위이다(`assertion`·`peerAddress`·`send`·`log`·`creatingIslands`·`setArchipelago`·`setCentralCreated`·`layout`·`randomCentral`·`surroundedCentral`·`predictableCount`). 선택한 장치는 도달한 위치에서만 요구하고 없으면 앞선 부분 효과를 남긴 채 거부한다.
- `sendPlayer(owner, target)`: 서버가 아니면 진단 `'server'` 뒤에도 작성을 계속 → 0xff 로 채운 100BYTE 예약(`[1, 0x2b, 4, 0]`) → 번호(`+0x32`, **metamap 진단은 번호를 조회할 때마다**) → 이름 정제(주소 장치 누락보다 먼저 수행) → 이름 길이 `+0x33`(NUL 포함)·이름 `+0x3b` → 구독자 `+0x26` → 접속 bit 가 없으면 `+4 = 0`, 있으면 주소 장치 문자열 → flags `+0x2a`·동맹 mask `+0x22`·`+0x34`·`+0x35` WORD·`+0x37`·`+0x38`·`+0x39`·`+0x3a` → 송신 장치(전체 예약을 넘기고 장치가 observer 뒤 선언 길이를 확정). 번호 9 이상은 거부한다.
- `sendMetadata(target)`: 실제 `$Random`(`[1, 0x72, 4, 0]`, `+4 = 0`, `+5` 군도 낮은 BYTE, `+6` 이름 길이 DWORD 8, `+10` 이름) 작성 → 중앙섬이 아직 없으면(서버가 아니면 진단) 같은 수신 몸체를 먼저 실행 → 대상 ≠ 현재 로컬 번호(**unsigned** 비교)이면 송신.
- `receiveMetadata(packet)`: Zone 앞 DWORD 의 bit 0x100 이 없으면 로그 `No Central Island.\n` 후 반환 → 이름이 `$` 가 아니면 반환 → 안내(`Creating Islands.`)·로그 `createArchipelago... `·군도 기록 → 배치 선택 → 특수 배치 2 는 지역 Template SURROUNDED 배치로 `centralCreated`/예측수 로그 없이 반환, 아니면 Zone 6바이트로 중앙섬 생성 → `centralCreated = 1` → 로그 `predictableCount = <signed>\n`.
- `observeMissionMetadataPacket(packet, sending, assertion)`(40b110): 송신에서만 길이 DWORD 가 -1 이면 진단(복귀하면 현재 값을 다시 읽음), 선언 WORD = 낮은 WORD + 10. 수신은 무동작. `registerMissionMetadata(dispatch, packets)` 는 0x72 의 mutable observer·버전 1/정책 0·수신기를 등록한다.

## 2. 기준선 뒤 변경 판정 (+27줄)

- **옮김(② 적용)**: 중앙섬/군도 분리와 metadata 상태(`9cb0547f5`), 로그인 writer 의 전체 mutable 송신 예약과 0x72 observer 본체(`ede61f05d`).
- **옮기지 않음 ③(4-D)**: 고정 Native 수신기 대여 분기(`eeae45c69`, E306 의 ③ 과 같은 묶음). 중앙섬 생성 자식(`randomCentral`/`surroundedCentral`)의 실제 몸체는 월드 생성(4-D 이후)에서 공급한다.
- CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-mission-server-packets.test.mjs` 7건)

- **원본 독립 관측 `mission-server-packets-x86.tsv` 187행**(Player 송신 P·metadata 송신 M·metadata 수신 R × 서버/metamap/생성 표시/Zone bit/군도 × 이름·flags·대상): 전체 Player 1620바이트·현재 전역 5개·외부 효과 순서와 송신된 wire 바이트(0x72 는 서버 observer, 0x2b 는 클라이언트 패킷 observer 후 캡처)를 모두 대조한다. 내부 정제 진입(`N,`)은 외부 효과 비교에서만 제외한다.
- 손 시험(cpppj `MissionServerPacketsTests.cpp` 이식 + 경계): 도달한 위치에서만 미연결 하위를 거부하고 이름 정제는 주소 경계 누락 전에 이미 수행됨, 이미 생성한 자기 metadata 는 소비자를 부르지 않음, 중앙섬 수신의 분기·순서(일반/특수 배치 2/Zone bit 없음/`$` 아님/짧은 원문), 0x72 observer, Player 송신의 필드 배치·주소 gate·metamap 진단 횟수, 이름 40자·주소 95자 경계, 송신 직전 metadata 바이트와 unsigned 대상 비교, 구성 검사.
- 변형 확인 71개 중 **6개만 남았고 모두 동등**하다(`put` 범위 — 호출 위치가 고정이라 도달 불가, 복사 시 NUL 탐색 — 호출자가 NUL 없는 구간만 줄, 번호 범위·BYTE 마스크·`read` 범위 — 뒤의 물리 범위 검사가 먼저 거부하거나 번호가 9 미만, 이름 NUL 누락 — 정제가 먼저 거부).

## 4. 남은 경계

- 3.1절 4-C 는 12/30 이다. 다음: RawMissionSettingsPacket(로그인 마지막 7DWORD 현재 설정 송신).
