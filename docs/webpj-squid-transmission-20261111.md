# webpj 공통 Squid 전송(RawSquidTransmission) — WB04-E313 (2026-11-11, 4-C)

3.1절 4-C 의 **RawSquidTransmission**(`o/`, 기준선 106줄 = `.cpp` 63 + `.h` 43, 기준선 뒤 +33줄)을 `core/raw-squid-transmission.js` 로 옮겼다. 공통 Squid 전송의 같은 2048BYTE 버퍼·현재 헤더·압축 상태와 실제 가상 writer 분배(10.78: 004ac260 송신 선택, 004ad170 35BYTE 갱신 쓰기, 00432fe0 압축 갱신 쓰기, 00433100 같은 owner 모드)이다. 앞 단위 [RawFormPacket](webpj-form-packet-20261111.md)(감시/내용물/삭제 writer)와 [RawBattleBroadcast](webpj-battle-broadcast-20261111.md)(방송 분배)의 소비자이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `createSquidTransmissionState()`: 월드 대여/반납에서 초기화하지 않는 공통 전역이다 — 2048BYTE 버퍼(반환 크기 뒤를 지우지 않음), 기존 Form 헤더·삭제 표식, 갱신 헤더 `0x2c/1/27`(고정 WORD 선언 길이는 35BYTE 반환과 별개), 압축 헤더 `0x7a/1/length`(**DWORD** 입력, 최소 4 비교 뒤 낮은 WORD), 압축 모드(signed, 초기 1), 임시 허용 관문(초기 0), 같은 owner 모드/마지막 owner(`CRAZY_PLAYER_ID = 0xffffff9d` = signed -99)/압축 횟수·바이트, 동기화 대상, 공유 Regular 헤더 `0x7e/1/28`.
- `new RawSquidTransmission(pool, state, hooks)`: 10.78 풀과 2048BYTE 버퍼가 필요하다. hooks(`type`·`defaultType`·`maxHp`·`broadcast`·`trace`·`assertion`·`connected`·`send`·`processPacket`·`broadcastMutable`·`sendMutable`)는 쓰는 위치에서만 요구하고 없으면 거부한다.
- `transmit(sid, target, selector, writerArgument, flags)`(004ac260): (임시 허용 관문이 꺼져 있을 때) client SID(5..14999)면 진단 `'!isClientSid()'`(줄 0x2d8)을 부르고(정상 복귀하면 계속) → 선택자 `-2` 만 현재 +58(공통 삭제 8바이트, flags 전달), 나머지는 +54(writerArgument 전달) → 반환 크기 ≥ 2048 이면 진단(줄 0x2e3, 복귀하면 계속; 2048 은 그대로 전송되고 2049 이상은 버퍼 범위 오류) → 크기가 양수면 대상 `-1` 은 방송(`broadcastMutable` 이 있으면 전체 2048BYTE 를 넘김), 그 밖은 `connected(target)` 관문 뒤 송신(`sendMutable` 이 있으면 전체 2048BYTE 를 넘김). `pop(sid, flags)` = `(sid, -1, -1, 0, flags)`, `sendTo(sid, target, arg)` = `(sid, target, -3, arg, 1)` — 방송으로 대체하지 않는다. `sameOwnerMode(mode)` 는 모드를 쓰고 마지막 owner 를 -99 로 초기화한다(모드가 같아도 항상).
- 현재 +54 분배: ProcessForm 가상 표 `0x512098` → 실제 `processPacket(sid, buffer, writerArgument)` 장치 / 빈 writer 표 `0x5092d0` → 0(송신 없음) / base 또는 검증된 자산 생성자 최종 표 → 공통 35BYTE 갱신(헤더 `[1,0x2c,27,0]`, `+4` SID, `+6` 좌표 8, `+14` 수량, `+16` 타입, `+17` 상태, `+18` owner, `+19` 꼬리 8, `+27` HP, `+29` z, `+30` 4, `+34` 1) / 그 밖은 Form writer(감시·내용물, 미복원은 명시 실패). 압축 모드가 켜져 있으면 같은 버퍼의 35BYTE 를 보관한 뒤 압축 출력으로 바꾼다.
- 압축 인코더(00432fe0): 필드 0..15 를 순서대로 평가하며 14/15 는 무효과 — 비트 0 바이트 좌표 둘(0..255 정수) / 1 실수 좌표 8 / 2·3 수량 1·2바이트 / 4 기본 타입이 아닐 때 타입 / 5 상태(`& 0xef` 가 0 이 아닐 때) / 6 상태 0x10 / 7 같은 owner(모드 켜짐·캐시가 -99 가 아님·같음) 아니면 8 owner 를 싣고 캐시 갱신 / 9·10 최대 HP(HP 플래그 타입)와 다를 때 HP 1·2바이트 / 11 타입 z 와 다를 때 z / 12 `+30` DWORD 가 0 이 아닐 때 낮은 바이트 / 13 마지막 바이트(항상). 출력은 `[헤더 4][SID 2][마스크 2][개수 1][데이터]` 이고 길이는 `9 + 개수`. 압축 횟수·바이트를 누적하고 횟수의 낮은 BYTE 가 255 가 될 때마다 수준 4 로그 `compressedUpdateSquid <누적바이트*100/(횟수*35)>%\n` 를 남긴다(정수 나눗셈).

## 2. 기준선 뒤 변경 판정 (+33줄, 마지막 `be567d46b`)

- **옮김(② 적용)**: 대상 지정 송신 `SendTo`·연결 관문·송신 장치(1cd2b05a5), 공통 삭제 선택자 `-2` 의 +58 분배와 `Transmit` 의 네 인자·ProcessForm writer 인자·`FormPacket` 삭제 진단 전달(b8cadde14), mutable 방송/송신(전체 2048BYTE 를 대상별 observer 에 넘김)·같은 owner 모드·공유 Regular 헤더·동기화 대상 전역(be567d46b).
- **옮기지 않음 ③(재기준선 때)**: 이 상태 전역을 읽는 수신 복호기 `RawSquidCompressedReceive`(압축 수신)와 월드 동기화 `RawPeerWorldSync` 는 대응표 밖 신규 파일이다. 시험은 수신 쪽 필드 순서를 시험 안에 독립 구현한 복호기로 왕복 일치만 확인한다.
- 세션 연결: 월드의 일반 Pop 송신기는 `world.bindSquidTransmission((sid, flags) => wire.pop(sid, flags))` 한 줄이며, 필요한 Player9/연결 표/현재 로컬·서버 상자와 타입 조회는 Session 조립(4-D)이 공급한다(3.6절 ③ 표의 4-C 항목을 해소하고 연결 자체는 4-D).
- CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-squid-transmission.test.mjs` 10건)

이 몸체의 35BYTE 갱신 쓰기와 압축 인코더에는 cpppj 에도 전용 원본 관측 fixture 가 없다(독립 프레임 검사가 NULL 연결 표로 간접 통과). 그래서 다음을 확인한다: 35BYTE 갱신의 슬롯 필드 위치(손 계산 기대 바이트)와 반환 크기 뒤 버퍼 보존, 압축 출력의 손 계산 바이트·길이, 13개 사례의 **독립 복호기 왕복**(꼬리 제외 일치), **마스크·데이터의 정확한 생략 판단 9사례**(왕복만으로는 불필요하게 실어도 같아 잡히지 않는 판단), 같은 owner 모드·캐시 초기화, 255 번째 누적 로그(정수 나눗셈·분모 35), 선택자/대상/진단/mutable 경로와 장치 누락 위치, 가상 표별 writer 분배(빈 표·ProcessForm·두 Form·미복원)와 2048/2049 크기, 헤더 bit 7·SID 14999/15000 경계·`-99` 상수·삭제 진단 전달, 구성·전역 초기값. 변형 확인 99개 중 **5개만 남았고 모두 동등**하다(좌표 `isFinite` 는 JS 비교가 이미 거부, `sendTo` 의 flags 는 선택자 -3 에서 쓰이지 않음, 압축 입력을 복사하지 않아도 읽기가 쓰기보다 먼저, 압축 데이터 64BYTE 검사는 최대 18바이트라 도달 불가, 마지막 owner 가 -99 일 때의 중복 검사는 owner BYTE 가 255 이하라 도달 불가). 누적 비율의 `idiv` 범위 밖 검사 두 가지(분모 0·INT_MIN/-1)도 횟수의 낮은 BYTE 가 255 인 시점에는 도달할 수 없다.

## 4. 남은 경계

- 3.1절 4-C 는 10/30 이다. 다음: RawMissionServerPackets(로그인 서버의 Player/중앙섬 metadata 작성과 송신 순서).
