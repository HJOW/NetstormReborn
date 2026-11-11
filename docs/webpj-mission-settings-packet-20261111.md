# webpj 미션 설정 패킷(RawMissionSettingsPacket) — WB04-E316 (2026-11-11, 4-C)

3.1절 4-C 의 **RawMissionSettingsPacket**(`o/`, 기준선 91줄 = `.cpp` 54 + `.h` 37, 기준선 뒤 +19줄)을 `core/raw-mission-settings-packet.js` 로 옮겼다. 실제 로그인 마지막의 7DWORD 현재 설정 송신(0x32)과 수신(10.78: 0044dbc0→0040a700/004df230 송신 작성, 0044dc20 수신 직접 저장, observer 0040b380)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- 필드 번호 `MISSION_SETTINGS_FIELD`(payload 순서): GeneratorTimer(+4)·HackMover(+8)·NoDamageDebug(+c)·ShootMyFriends(+10)·FillMyTank(+14)·CombatGumpColumns(+18)·CombatGumpRows(+1c). 실제 현재 조회 순서는 이 번호 순서와 다르다.
- `new RawMissionSettingsPacket({ current(field), setCurrent(field, value), send(target, bytes) })`: 설정 사본/캐시를 두지 않고 매번 같은 현재 DWORD 를 읽는다. `current` 가 없으면 성공 상수로 채우지 않고 구성을 거부한다.
- `send(target)`: `[1, 0x32, 32]` + 7DWORD 를 작성해 실제 송신을 요청한다. 원문 asm 의 load 순서는 **1, 2, 0, 4, 5, 3, 6** 이며 읽은 값은 다음 callback 이 현재 설정을 바꿔도 보존된다. 송신 장치가 없으면 읽은 뒤 거부한다.
- `receive(packet)`: payload 순서 **0..6** 으로 `packet+4` 부터 각 DWORD 를 **그때 현재 바이트에서** 다시 읽어 같은 설정에 쓴다(앞선 store 는 물리 부족에도 보존: 15바이트면 필드 0,1 만 쓰고 셋째 DWORD 에서 거부). 헤더 검증은 실제 분배기가 담당한다.
- `receiveCurrentMissionSettingsPacket(current7, packet)`: 비로그인 경로의 같은 직접 쓰기. `registerMissionSettingsPacket(dispatch, settings | current7)`: 설정 객체면 observer(인자를 버리는 ret4)·버전 1/정책 0·수신기, 7칸 배열이면(Editor/현재 Session) observer 를 비우고 길이 32 를 요구하며 같은 원문 7DWORD 수신을 등록한다. 길이 32 의 기본값은 분배기의 기존 길이 표가 소유한다.

## 2. 기준선 뒤 변경 판정 (+19줄)

- **옮김(② 적용)**: 비로그인(Editor/현재 Session)의 송신 객체 없는 7DWORD 수신 등록과 직접 쓰기(`52eae6bc0`).
- **옮기지 않음 ③(4-D)**: 실제 장기 Native 32h 수신기(`nativeEntry 0x0044dc20`)를 대여하는 분기(`a526af5e4`, E306 의 ③ 고정 Native 수신기 묶음). 나머지 `ae0e58b3a` 는 개행 보존뿐이다.
- CD(10.72)는 옮기지 않았다(판본 인자 없음).

## 3. 검사 (`tests/raw-mission-settings-packet.test.mjs` 4건)

- **원본 독립 관측 `mission-settings-packet-x86.tsv` 164행**(송신 117·수신 45·observer 2 × 현재 설정 프로필(영/전부 0xffffffff/경계값/필드별 구분값/한 필드만 1·0x80000000·0xffffffff) × seed × 대상 × 호출 도중 변경(첫 효과만/매 효과)): packet 32바이트 전체, 현재 설정 7DWORD, 읽기·쓰기·송신 효과 순서를 모두 대조한다. 수신에서는 callback 이 packet 의 나머지 payload 를 바꾸고 다음 DWORD 가 그 현재 바이트에서 읽히는지 확인한다.
- 손 시험(cpppj `MissionSettingsPacketTests.cpp` 이식 + 기준선 뒤 경로): 실제 분배기 등록(길이 32·버전 1·정책 0·observer 무효과)과 분배 경로의 시각/기록 한 번씩, 송신 왕복, 비로그인 7DWORD 직접 등록(observer null)과 물리 부족 시 부분 쓰기 보존, 선택한 getter/store/send 누락 위치.
- 변형 확인 36개 중 **1개만 남았고 동등**하다(송신 중 `fill` 읽기와 로컬 packet 쓰기의 순서 — 서로 독립이라 외부 효과 순서가 같음). 물리 범위 경계(15바이트)는 시험을 보강해 잡았다.

## 4. 남은 경계

- 3.1절 4-C 는 13/30 이다. 다음: RawMissionLoginPackets(현재 Template SaveImage 와 sendFort 의 0x2a 예약/필드/길이 확정).
