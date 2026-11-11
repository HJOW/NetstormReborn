# webpj 연결 등록소(RawConnectionRegistry) — WB04-E308 (2026-11-11, 4-C)

3.1절 4-C 의 **RawConnectionRegistry**(`o/`, 기준선 110줄 = `.cpp` 56 + `.h` 54, 기준선 뒤 +145줄)를 `core/raw-connection-registry.js` 로 옮겼다. Client 와 단독 미션의 두 연결 끝이 같은 22 연결·22 송신버퍼·32 pending 표와 동기화 토큰을 빌려 쓰는 **숫자 토큰 → 실제 연결/장치 해석과 수명** 장치이다. store 순서는 앞 단위 [Cleanup](webpj-connection-cleanup-20261111.md)·[Registration](webpj-connection-registration-20261111.md)의 자유 함수(원본 관측 fixture 로 검증)가 맡고, 이 클래스는 그 hooks 를 실제 노드/장치에 이어 준다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- **토큰**: `track(node, destroy, release[, send, readiness, buffered])` 가 `0x6d000000` 부터 재사용하지 않는 숫자 토큰을 발급한다(원문 주소 아님). 같은 노드의 중복 등록, 필수 소멸/해제 장치 누락, 소진(unsigned 감김으로 0), 응용 종료 뒤 발급은 거부한다. 실제 할당이 끝난 뒤에만 공개한다. `owns`(해제 뒤 노드를 읽지 않는 존재 조회)·`node`(해제된 토큰은 오류)·`tokenFor`(노드 객체 → 토큰, sender 번호로 대신하지 않음).
- **표 묶음**: `cleanup()` 은 복사가 아니라 같은 표/상자의 보기이다. `bindSynchronization(box)` 로 실제 동기화 토큰 상자를 빌릴 수 있다(값은 쓰지 않음). 명시 상자를 주면 그것을 동기화로 쓰는 보기를 돌려준다.
- **수락/재배치**: `canAccept`(pending 개수가 **signed** 로 31 미만) → `accept(token)`(첫 빈 pending 1..31 칸·sender = `-slot` unsigned·개수 증가, 음수 id 반환). `register`/`assign`/`assignNative` 는 Registration 자유 함수에 실제 노드와 선택 buffered 를 이어 준다. 일반 경로의 진단은 오류가 되고(`원문 연결 등록 진단:<줄>`), Native 경로는 줄 b0/b3/b6/bb 의 원문 식과 파일 `\Ns\O\Ztrans.cpp` 로 선택한 Client 진단 장치를 부른 뒤 **반환하면 이어서 진행**한다. `assignNative` 의 buffered 를 생략하면 같은 토큰의 공통 buffered 장치(`setBufferedNative`, 4df470(this,1))를 쓴다.
- **정리**: `disconnect`/`disconnectNative`/`closeAll`/`endPlayers`/`closePending` 은 Cleanup 자유 함수를 정리 부모(`runCleanup`) 안에서 실행한다. 부모 안에서는 해제(`release`)가 일반 별칭 정리를 하지 않고 부모가 자신의 칸·개수를 직접 쓴다. 부모 밖의 `release` 는 성공한 뒤에만 같은 토큰의 양수 칸(동기화 토큰이 같으면 함께 0)과 pending 1..31 칸(칸마다 개수 1 감소, **0 칸은 제외**)을 지우고 등록을 지운다. `forget(token)` 은 호스트 owner 의 최종 소멸용(0 은 무시).
- **송신**: `sendMutable`(004e2dc0)은 슬롯을 읽고 → Player 관문(1..8 은 +0x50 bit 0) → 빈 슬롯 false → 같은 가상 송신 장치를 부르고 true(전달 성공이 아님). 번호 범위는 -32..21 이고 -32 는 실제 pending 표 밖이라 표 범위 오류. `sendMutableNative` 는 범위 밖에서 줄 1e 진단 뒤 실제 표 범위 오류가 이어진다. `sendDirect` 는 번호/Player 관문을 반복하지 않는다. `address(id)` 는 연결이 없으면 소켓 없이 `0.0.0.0:0` 주소(첫 BYTE 2). `socketReadiness`·`publishConnect`(칸만 쓰기).
- **응용 최종 종료**: `shutdownTrackedDevices()` 는 진행 중 호출 범위/정리 부모나 수신 중(깊이>0) 노드가 있으면 거부하고, 이후 새 발급을 막은 채 `closeAll`(슬롯 순서) → `closePending`(개수 유지) → 전역 칸에서 덮여 남은 노드를 토큰 오름차순으로 소멸 → 해제한다.

## 2. 기준선 뒤 변경 판정 (+145줄, 마지막 `50a5a1845`)

- **옮김(② 적용)**: `SocketReadiness`·readiness 장치·`PublishConnect`(31f08f769), 대기 상한을 **signed** 로 읽는 `CanAccept` 교정(기준선은 unsigned `<31`, 6b700418c: 004e02f1 의 jge/4e038c 의 jl 은 signed), `SendMutableNative`·`AssignNative`·`DisconnectNative`·Native 진단(230528e31), `SetBufferedNative`·`ShutdownTrackedDevices`(50a5a1845), `TokenFor`·`SendDirect`(392517096). `BindSynchronization`·`Track`·`Release` 등 나머지는 기준선에 이미 있던 것이다.
- **옮기지 않음 ①(웹에서 효과 없음)**: 장치 소유자 고정(`BindDevicePin`·`PinBinding`·`deviceOwner`·`ReleaseDevicePins`, C++ `shared_ptr` 수명 보유)은 GC 가 대신한다. 이 때문에 `retainNodes` 도 없다.
- **옮기지 않음 ③(재기준선 때 4-D)**: 접속 통지 `StoreConnectPacket`·`SendConnectPacket`·`RegisterConnectPacketReceiver`(621efab7e·efc78ee56). 실제 TCP 접속의 초기 패킷 복제(004e2620·004e2f00)라 LAN 전용이고, 대응표 밖 신규 파일 `RawPacketCopy`(`DuplicateRawPacket`)와 고정 Native 수신기(`RegisterCanonicalDirect`, E306 ③)에 기댄다. 그 때까지 송신버퍼 표 `buffers` 의 토큰은 등록소가 해제 장치를 알 수 없으므로 `shutdownTrackedDevices` 가 비어 있지 않은 칸을 오류로 거부한다.
- 단독 미션 경로(`LocalMissionTransport`)는 baseline 부분집합(Track/Register/Accept/CanAccept/Assign/Node/Owns/Destroy/Release/Forget/Cleanup)만 쓴다. CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-connection-registry.test.mjs` 14건)

등록소는 호스트 장치라 원본 PE 관측이 없다. cpppj `ConnectionRegistrationTests.cpp` 의 등록소 단독 시험 세 건(같은 노드 재배치+owner 종료, 해제 실패의 prefix 보존, buffered 중단의 임시 별칭 유지)을 옮기고 메서드별 계약을 직접 검사한다: 토큰 발급·중복·소진, 표 묶음 보기·동기화 대여, pending 수락(첫 빈 칸·signed 관문·불일치), 해제의 별칭 정리(0 칸 제외·정리 부모 안 유예), 송신 범위·Player 관문·빈 슬롯·장치 누락, Native 송신 줄 1e, 주소·준비·접속 공개, 재배치 일반/Native 진단(b0/b3/b6/bb 식과 파일 문자열), 줄 fa, 전체/owner/pending 정리, 최종 종료의 닫기 순서·가드. Editor/LocalMission/Client 연결 시험(같은 파일의 나머지 넷)은 해당 단위에서 옮긴다. 변형 확인 93개 중 **4개만 남았고 모두 동등**하다: `entry` 의 음수 검사(호출부가 음수 index 를 만들지 않음), 종료 가드의 `cleanupDepth` 단독 검사(정리 깊이는 항상 호출 범위 안에서만 올라감), 종료의 버퍼 칸 0 쓰기(위 ③ 때문에 아직 도달 불가), 등록 hooks 의 buffered 노드 확인(바로 앞의 `node` 해석이 먼저 거부).

## 4. 남은 경계

- 3.1절 4-C 는 5/30 이다. 다음: RawConnectionActivity(수신 활동 기록·0x64 등록).
