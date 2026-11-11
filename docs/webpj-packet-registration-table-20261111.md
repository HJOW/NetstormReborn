# webpj 패킷 등록 표(PacketRegistrationTable) — WB04-E305 (2026-11-11, 4-C)

3.1절 4-C 의 첫 항목 **PacketRegistrationTable**(`o/`, 기준선 113줄 = `.cpp` 62 + `.h` 51, 기준선 뒤 +24줄(`899c934dc`))을 `core/packet-registration-table.js` 로 옮겼다. 원문 전역 수신/관찰/버전/길이 표 256칸의 단일 owner 와, 한 Session 이 등록한 칸만 되돌리는 대여(Lease)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new PacketRegistrationTable('patch1078')`: 마지막 지원 타입 137, 0..137 칸의 길이는 `PATCH_PACKET_LENGTHS`(E232)에서 복사하고 receiver/observer/version/정책은 0(`null`), 그 밖의 물리 칸은 가변 길이(0xffff)이다. 다른 판본은 거부한다.
- `PacketRegistration`: `receive(sender, connection, packet)`·`observer(sending)`·`packetObserver(packet, sending)`(같은 원문 observer 슬롯의 두 표현, 동시 등록 없음)·`length`·`version`·`mismatch`. `pin()` 은 대여가 등록한 항목이면 그 대여의 활동 표식을, 전역 기본 등록이면 빈 표식을 돌려주고 반납된 등록이면 오류이다. `notifyObserver(packet, sending)` 은 패킷 관찰기가 있으면 그것, 없으면 방향 전용 관찰기를 호출하며 호출 동안 핀을 유지한다(콜백이 표를 바꿔도 이미 읽은 함수를 끝까지 씀).
- `bindSession(owner)`: owner 필수, 중복 대여 거부, 반납된 세대의 콜백이 아직 실행 중이면 새 대여 거부. 대여는 시작 때 표 전체를 snapshot 한다. `touch(type)` 은 등록 API(RawPacketDispatch)가 변경 전에 부르며 현재 대여의 변경 칸과 항목의 대여 표식을 기록한다(대여가 없으면 전역 등록; 반납된 세대의 콜백 안에서는 전역 수정 거부). `lease.detach()` 는 **이 대여가 변경한 칸만** 같은 항목 객체에서 시작 값으로 되돌리고(전역 칸·다른 세대 칸 보존) 한 번만 동작한다. `lease.pin()` 은 활동 수를 올리며 반납된 대여는 거부한다.
- 기준선 뒤 cpppj 추가분: `registration(type)`(0..last 읽기; 호스트 256 물리 폭으로 넓히지 않음)과 `resetCallbacks()`(R00482b60: 0..last 의 함수/관찰기/버전/정책을 비우고 길이표·항목 객체 보존, 대여·콜백 실행 중 거부)를 **옮겼다**. `PersistCanonicalBaseline`/`canonicalReceive`(장기 고정 수신기 승격)는 Native 장기 등록이라 **3.6절 ③ 4-D** 로 둔다.
- 웹에는 소멸자·약한 참조가 없어 C++ 의 "Session 이 사라지면 대여가 스스로 반납"을 `lease.detach()` 명시 호출로 대신한다(소유자 소멸 뒤 남은 등록이 호출되는 오류 경로는 도달할 수 없음).

## 2. 검사 (`tests/packet-registration-table.test.mjs` 7건)

원본 관측 파일이 없는 단위 계약 검사이다(cpppj 는 표를 쓰는 Dispatch/Session 시험에서 간접 확인): 구성(0..137 길이 표·0 초기값·물리 폭), 대여의 owner/중복/활동 중 거부와 반납 뒤 재대여, 변경 칸만 같은 항목에서 복원하는 반납과 전역 직접 수정 보존, touch 의 전역/반납 도중 규칙, 활동 핀(전역은 빈 표식·반납 뒤 거부), 관찰기 선택과 예외에도 핀 반납, 전역 초기화의 비움 범위와 거부 조건.
변형 확인 42개 중 **4개만 남았고 모두 동등**하다: 반납 도중(current 가 비활성인 순간)의 전역 수정 거부와 전역 touch 의 표식 지움(JS 는 반납이 동기라 그 순간에 호출될 수 없음), 반납의 현재 대여 확인과 detach 멱등 검사(서로 중복된 두 방어).

## 3. 남은 경계

- 3.1절 4-C 는 1/30 이다. 다음: RawPacketDispatch(이 표의 소비자: Register/Observe/Version/RequireLength/Validate/Dispatch).
