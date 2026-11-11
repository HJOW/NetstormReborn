# webpj 패킷 분배기(RawPacketDispatch) — WB04-E306 (2026-11-11, 4-C)

3.1절 4-C 의 **RawPacketDispatch**(`o/`, 기준선 135줄 = `.cpp` 63 + `.h` 72, 기준선 뒤 +68줄)를 `core/raw-packet-dispatch.js` 로 옮겼다. 패킷 등록·길이 검증·버전 정책·시각/기록·즉시 수신기 분배(004ded50/70/90·004df240·004df740·004df010)이며 앞 단위 PacketRegistrationTable(E305)의 소비자이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawPacketDispatch({ now, record, terminate, close, release[, sent, players] }, table?)`: 필수 효과 다섯과 같은 판본 등록 표가 아니면 거부한다. 표는 여러 분배기가 공유한다(`registrations`).
- 등록: `register(type, receive)`·`registerDirect(type, receive)`(NULL 연결을 허용하는 내부 직접 수신기, 일반 프레임 입구와 같은 함수)·`observe(type, observer)`/`observePacket(type, observer)`(같은 원문 observer 슬롯의 두 표현, 마지막 등록이 이전 장치를 완전히 교체)·`version(type, version, mismatch)`·`requireLength(type, length)`. 타입 0·지원 밖은 변경 전에 거부하고, 변경마다 `table.touch(type)` 로 현재 대여의 변경 칸을 기록한다. 일반 등록은 이전 직접 수신기를 함께 지운다.
- `validate(packet)`: 헤더 4바이트 미만은 오류, 타입 0/지원 밖은 false, 타입 10 이상은 처리기 필요, 선언 길이는 표와 정확히 같아야 한다(0xffff 가변, little endian).
- `dispatch(connection, packet)`: 닫힌 연결(flags 0x40)은 false → 무효 패킷은 무시(true) → 타입 4 집합은 거부 → 버전 정책(높은 버전 4 종료/1 무시, 낮은 버전 8 종료/2 무시, incoming 의 0x80 비트는 가림) → 등록 핀 → 시각 기록(타입 6 은 heartbeat, 그 밖은 received) → 타입 10 이상 선언 길이와 함께 기록 → 현재 슬롯 수신기 한 번 선택 호출(깊이 증가, 정상/예외 복구) → 깊이 0 에서 닫힘이면 소멸/해제 후 false. 기록 효과가 등록을 바꾸면 그 슬롯의 현재 수신기를 쓴다.
- `invokeDirect(type, sender, connection, packet)`: 원문 함수 포인터 호출처럼 검증/기록/시각/깊이/닫힘을 추가하지 않고 등록 핀만 유지한다. 직접 수신기는 null 연결도 받고 일반 수신기는 실제 연결이 필요하다.
- `registerConstructionData(dispatch, data)`: 0x37 의 observer=null, 버전 1/정책 0, 수신기 = `data.receive(packet)`. `packetPlayerActive(fallback, current, owner)`: 현재(또는 fallback) Player9 의 owner 기록 +0x50 낮은 BYTE.

## 2. 기준선 뒤 변경 판정 (+68줄, 마지막 `ec0b6e973`)

- **옮김(②/①)**: `RegisterDirect`/`InvokeDirect`·`directReceive`(내부 직접 호출), `sent`/`players` 선택 장치와 `PacketPlayerActive`, 지연 표시 연결에 수신기가 있으면 거부(기준선의 "지연 분배 미지원"과 같은 결과), 타입 4 집합 오류 문구.
- **옮기지 않음(웹 범위 밖/후속)**: `CanonicalPacketReceiver`·`RegisterCanonical*`·`SnapshotReceiver`·`PersistCanonicalBaseline`(고정 Native 수신기 승격: 실소켓 지연 FIFO·장기 Native 등록, 3.6절 ③ 4-D), `RawPacketDelay` 큐(실제 소켓 지연 FIFO, LAN 전용으로 웹 제외). 타입 4 집합 순회는 새 `RawPacketFrame`(대응표 밖 신규 파일, 재기준선 때 등록)에서 다룬다.

## 3. 검사 (`tests/raw-packet-dispatch.test.mjs` 10건)

원본 관측 파일이 없는 단위 계약 검사이다(cpppj 는 건설/연결/로그인 시험 안에서 간접 확인): 구성/필수 효과, 등록 API 의 범위·교체·대여 기록과 반납 복원, 검증(타입 9/10 경계·선언 길이), 닫힘/무효/집합/버전 정책 조합, 시각·기록 순서와 경계, 콜백 깊이(정상·예외)·닫힘 후처리·중첩, 등록 핀과 지연 연결, 직접 호출, 건설 수신 등록, Player9 활성 조회. 변형 확인 72개 중 **3개만 남았고 동등**하다(등록 표 판본/종류 검사는 중복 방어, 비정수 타입은 표 접근이 같은 RangeError).

## 4. 남은 경계

- 3.1절 4-C 는 2/30 이다. 다음: RawConnectionRegistry(연결 토큰/고정표/수명).
