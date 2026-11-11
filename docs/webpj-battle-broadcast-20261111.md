# webpj 전투 방송(RawBattleBroadcast) — WB04-E311 (2026-11-11, 4-C)

3.1절 4-C 의 **RawBattleBroadcast**(`o/`, 기준선 90줄 = `.cpp` 53 + `.h` 37, 기준선 뒤 +28줄)를 `core/raw-battle-broadcast.js` 로 옮겼다. 원본 Ztrans 의 현재 Player/연결 표를 읽는 두 송신 분배(10.78: 004e2e30 전체, 004e2e80 제외)와 훨리기그 명령(0x86)·섬 정리(0x35) 20BYTE 패킷 작성기이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawBattleBroadcast(players, peers, localOwner, server[, assertion])`: `players` 는 Player9 기록 9×180BYTE(`Uint8Array`, 접속 BYTE 는 각 `+0x50` bit 0), `peers` 는 길이 9 의 연결 송신 장치 표(가상+4, NULL 연결은 null), `localOwner`·`server` 는 현재값을 매번 다시 읽는 `{ value }` 상자이다. 모두 빌리므로 이 객체보다 오래 살아야 한다. 크기·모양이 다르면 거부한다(CD 판본의 Player 폭 0xac 은 웹 범위 밖).
- `send(packet)`: 1..8 중 접속(bit 0)이고 NULL 이 아닌 연결 중 **현재** 로컬이 아닌 owner 에게 같은 packet 객체를 보낸다(Player 0 은 순회하지 않음). 로컬은 후보마다 다시 읽어 가상 송신의 상태 변경이 다음 후보에 반영된다.
- `sendExcept(excluded, packet)`: server 가 아니면 진단 장치(`assertion`)를 부르고(없으면 오류), 정상 복귀하면 서버 값을 재검사하지 않고 계속한다. 시작 때 로컬 owner 를 보관해 이후에는 접속/연결 표만 다시 읽으며 지정 제외 번호(int32 비트열)와 로컬을 뺀다.
- `bindMutableSend(send)`/`sendMutable(packet)`: 같은 선택 순서 뒤 target 마다 `send(owner, packet)` 로 같은 버퍼를 넘겨 현재 observer 가 수정하게 한다(앞 대상의 수정은 다음 대상에도 보존). 대상이 있는데 장치가 없으면 도달한 위치에서 오류이고, NULL 연결뿐인 독립 호출은 장치 없이 자연 반환한다.
- `writeFlyerCommandPacket(packet, parent, command)`: `[1, 0x86, 20, 0]` + `+4` 부모 SID + `+12` 명령 DWORD(little endian). 호출자가 쓰지 않는 `+8`/`+16` 은 입력 바이트를 유지한다. `writeDropIslandPacket(packet, x, y, flags, owner)`: `[1, 0x35, 20, 0]` + `+4/+8/+12/+16` 네 DWORD(좌표는 16.16 비트 그대로). 둘 다 20BYTE 미만이면 **쓰기 전에** 거부하고 20BYTE 밖은 건드리지 않는다.

## 2. 기준선 뒤 변경 판정 (+28줄, 마지막 `77a0c3a47`)

- **옮김(② 적용)**: `BindMutableSend`/`SendMutable`(77a0c3a47: 실제 등록소의 가상 송신으로 같은 writer 예약을 넘기는 소비자), 제외 송신의 server 진단 장치 복귀(167e7b79c: 기준선은 server 가 아니면 즉시 오류, 이후는 진단 장치 정상 복귀 시 계속).
- 옮기지 않은 변경은 없다. CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-battle-broadcast.test.mjs` 7건)

- **원본 독립 관측 3종**: `battle-bcast-x86.tsv` originals **678행**(전체 171·제외 507; 접속 mask × 연결 mask × 로컬 × 제외 번호 × 첫 송신 중 로컬/연결/접속 변경)의 후보 송신 순서와 packet 객체 동일성, `flyer-command-x86.tsv` **30행**(24BYTE 버퍼에서 20BYTE 전체와 두 정밀도 열, 20BYTE 밖 canary), `drop-island-packet-x86.tsv` **336행**(좌표 DWORD 원문값, 20BYTE 전체와 canary). originalCD 행은 읽지 않는다.
- 손 시험(cpppj `BattleBroadcastTests.cpp` 이식): NULL 연결·비접속 생략과 제외 송신의 server 진단(복귀하면 계속, 시작 시 로컬 보관, 전체 송신은 후보마다 로컬 재조회, 제외 번호 int32 감김), mutable 송신의 대상 순서·packet 보존·장치 연결, 구성 검사, 짧은 출력의 쓰기 전 거부와 원격 연결에만 전달, unsigned 32비트 쓰기.
- 변형 확인 45개 중 시험 보강 뒤 **2개만 남았고 모두 동등**하다(섬 정리 DWORD 의 `>>> 0` 은 쓰기 함수의 시프트가 같은 절삭, 송신 장치 존재 재확인은 바로 앞 조건의 중복).

## 4. 남은 경계

- 3.1절 4-C 는 8/30 이다. 다음: RawFormPacket(partial, Form 가상+54·공통 삭제 가상+58 의 실제 패킷 writer).
