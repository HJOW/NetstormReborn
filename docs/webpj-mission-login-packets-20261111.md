# webpj 미션 로그인 Fort 패킷(RawMissionLoginPackets) — WB04-E317 (2026-11-11, 4-C)

3.1절 4-C 의 **RawMissionLoginPackets**(`o/`, 기준선 104줄 = `.cpp` 62 + `.h` 42, 기준선 뒤 +4줄)를 `core/raw-mission-login-packets.js` 로 옮겼다. 현재 Template SaveImage([E277](webpj-fort-image-20261110.md))와 로그인 sendFort 의 실제 0x2a 예약/필드/길이 확정(10.78: 004a7240 sendFort, 004df590 예약 헤더, 0040a540 길이 observer)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `makeMissionFortPacket(owner, predictable, watcher, image, initialLength)`: 전체 예약을 **0xff 로 채우고**(imageSize + 20 바이트) `[1, 0x2a, 초기 길이 WORD(최소 4)]`, `+4` owner, `+8` 예측수, `+12` 이미지 크기, `+16` watcher, `+17` 이미지만 쓴다. **끝 3바이트 꼬리는 0xff 로 보존**하며 최종 wire 는 imageSize + 17 이다. 이미지가 고정 할당(0x8000)을 넘으면 거부한다. 초기 길이는 WORD 로 취급한다.
- `finalizeMissionFortPacket(packet, writeLength, assertion)`(0040a540): `writeLength` 가 거짓이면 아무것도 읽지 않고, 참이면 17바이트 미만을 거부한다. 이미지 크기 DWORD 가 `0xffffffff` 면 진단(없으면 거부, 복귀하면 계속)하고, **현재 하위 WORD 를 다시 읽어** `+0x11` 을 선언 WORD 에 쓴다(앞서 검사한 DWORD 사본이 아님).
- `MissionFortPacket.bytes()`: 현재 선언 길이 WORD(4 이상, 예약 길이 이하)만 wire 로 빌린다. 길이 확정 전/변조된 선언은 거부한다.
- `new RawMissionLoginPackets(players, reads, imageHooks, hooks)`: `reads = { current(owner), initialLength() }`(현재 Template 조회와 00544c48 의 WORD), `imageHooks` 는 `RawFortImage` 의 현재 `typeCount`/진단, `hooks = { log, send, missingTemplate }`. 조회·타입 개수·Player9 크기가 없으면 구성을 거부한다.
- `sendFort(owner, target, predictable)`(004a7240): owner 9 이상 거부 → Template 이 없으면 **자연 false** → 로그 `sendFort from %d to %d` → (manager 가 다시 읽어 없으면 누락 진단 뒤 현재 manager 재조회) → 현재 image 작성 → 현재 길이 WORD 조회 → 헤더 작성 뒤 **현재 watcher**(Player+0x58) → 전체 예약 송신 후 true. 길이는 미리 확정하지 않고 송신 장치가 gate 뒤 observer 로 확정한다.

## 2. 기준선 뒤 변경 판정 (+4줄, `ede61f05d`)

로그인 writer 의 실제 mutable 송신 예약(image+20 전체를 송신 장치에 넘김)을 3.6절 ② 로 반영했다. 그 밖의 변경은 없다. CD(10.72)는 옮기지 않았다(판본 인자 없음).

## 3. 검사 (`tests/raw-mission-login-packets.test.mjs` 2건)

- **원본 독립 관측 `fort-image-packets-x86.tsv` 의 S 93행·F 10행**(이미지 행 I 116은 앞 단위가 사용): 합성 payload 패턴(0·가변·32696·32695 바이트)과 실제 보호 Fort 8개의 현재 섹션을 입력해 예약(image+20) 전체 BYTE·observer 전후·최종 wire(image+17)와 외부 읽기 순서(Template 조회 `C`, 길이 `H`, 로그 `L`, 누락 진단, 송신 `S`, 타입 개수 `G`, 진단 types/section/stream)를 모두 대조한다. 호출 도중 Template 교체/제거, 길이 조회 중 watcher 변경도 포함한다.
- 손 시험(cpppj `FortImagePackets_SelectedBoundaries` 이식 + 경계): Template 없음의 자연 반환과 조회 횟수, 로그/송신 장치 누락 위치, owner 범위, 구성 검사, 길이 observer 의 거짓·짧은 입력·센티넬 진단·복귀 뒤 하위 WORD 재조회, 변조된 선언 길이, 예약 tail 과 초기 길이 최소 4·WORD 취급, 이미지 32768/32769 경계.
- 변형 확인 47개 중 보강 뒤 **2개만 남았고 동등**하다(`bytes()` 의 헤더 길이 검사는 길이 0 이 하한 검사에서 어차피 거부, 현재 manager 가 계속 NULL 이면 `RawFortImage` 구성이 먼저 거부).

## 4. 남은 경계

- 3.1절 4-C 는 14/30 이다. 다음: RawMissionLogin(PATCH 로그인 004a7800 의 Player/일반 전투·미션/패킷/Ready 부모 순서) — 앞서 보류한 Ready boss 6행과 이 단위의 송신을 함께 연결한다.
