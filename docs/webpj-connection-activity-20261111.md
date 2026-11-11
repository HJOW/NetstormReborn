# webpj 연결 수신 활동(RawConnectionActivity) — WB04-E309 (2026-11-11, 4-C)

3.1절 4-C 의 **RawConnectionActivity**(`o/`, 기준선 76줄 = `.cpp` 39 + `.h` 37, 기준선 뒤 +1줄)를 `core/raw-connection-activity.js` 로 옮겼다. 10.78 의 004b6180 수신 활동 기록과 0040be79..0040be97 의 실제 0x64 등록이다. 패킷 분배기([E306](webpj-packet-dispatch-20261111.md))와 연결 등록소([E308](webpj-connection-registry-20261111.md))의 소비자이며, 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawConnectionActivity({ frames, wallSeconds }, { wallSeconds, frameNumber, server, eraseStatus })`: 두 표는 같은 Client owner 의 **22칸 `Uint32Array`**(005c84e8 프레임·005c8548 벽시각 초)를 빌린다. 같은 배열이거나 같은 버퍼의 같은 오프셋(= 같은 Native 주소)이면 거부하고, 사이의 Zone 두 DWORD(005c8540/544)는 표에 포함하지 않는다. 네 장치(`wallSeconds()`=00460d70, `frameNumber()`=0055b4a8 하위 DWORD, `server()`=00540bc0, `eraseStatus(kind, player)`=004d3090)는 모두 필수이며 원문 위치에서 매번 새로 조회한다.
- `receive(sender)`(004b6180): 정상 인덱스 0..21 만 처리한다. 범위 밖(음수·22 이상·비정수·비수치)은 원문의 메모리 덮어쓰기를 흉내 내지 않고 **첫 효과 전에** RangeError. 순서는 벽시각 조회 → signed64 절삭 하위 DWORD(비유한/범위 밖은 0, 음수는 2의 보수 하위 32비트) → 프레임 조회 → **벽시각 표 store** → server 조회(앞선 벽시각 효과 뒤 새로 읽음) → **프레임 표 store**(벽시각 store 전에 캡처한 같은 DWORD, 0 도 그대로) → 상태 삭제(server 면 `(2, sender)`, 아니면 `(1, 0)`). 호출 뒤 표를 다시 동기화하거나 값을 복구하지 않는다. 패킷 본문·연결 객체는 읽지 않는다.
- `registerConnectionActivity(dispatch, activity)`: 관찰기(0040b380, 아무 효과 없음)·버전 1/정책 0·수신 본문(`sender | 0` 으로 signed 해석)을 등록한다. 길이 5 는 기존 길이 표의 현재값이며 이 등록이 덮어쓰지 않는다. 일반 분배기 경로(깊이·시각 기록·길이 검증)를 그대로 쓴다.

## 2. 기준선 뒤 변경 판정 (+1줄, `eeae45c69`)

등록 함수가 시작할 때 0x64 에 이미 Native 고정 수신기(`canonicalReceive.nativeEntry == 0x004b6180`)가 있으면 같은 값을 대여하고 독립 typed 등록을 건드리지 않는 분기이다. 이는 E306 에서 3.6절 ③ 4-D 로 넘긴 **고정 Native 수신기 승격**과 같은 묶음이라 옮기지 않았다(분배기에 `canonicalReceive` 가 생길 때 함께 추가). 그 밖의 변경은 없고 CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-connection-activity.test.mjs` 7건)

원본 관측 파일이 없는 단위 계약 검사이다(cpppj `ConnectionActivityTests.cpp` 여섯 건 이식 + 구성 검사): 벽시각·프레임 store 와 server 조회의 원문 순서(각 장치 시점에 보이는 표 값 확인, 마지막 효과가 바꾼 표를 복구하지 않음), 비서버 삭제 인자·프레임 0·음수 벽시각, **signed64 절삭 하위 DWORD 20점**(0, ±0.75, ±1.5, 2³¹/2³² 경계, 2⁶³ 바로 앞·뒤, ±1e100, ±Infinity, NaN), 두 표 사이 Zone DWORD 와 이웃 칸 보존(한 메모리의 배치), 잘못된 sender 11종의 첫 효과 전 거부, 구성의 별개 표/22칸/필수 장치, 기존 분배기를 통한 0x64 등록(순서 `now → record → wall → frame → server → erase`, 선언 길이 불일치 생략, 정책 0 의 다른 버전 분배, 길이 7 보존, 음수 sender 거부). 같은 C++ 파일의 표 수명 시험(`ClientConnectionActivityGlobals` 소유·복제/이동 금지)은 해당 단위에서 옮긴다. 변형 확인 29개 중 **4개만 남았고 모두 동등**하다(벽시각 변환의 ±2⁶³ 경계는 같은 하위 DWORD 0, `>>> 0` 은 Uint32Array 저장이 같은 절삭, `sender | 0` 은 범위 밖이면 어느 쪽이든 거부).

## 4. 남은 경계

- 3.1절 4-C 는 6/30 이다. 다음: RawTransportPolling(헤더만, 여러 전송 owner 의 현재 ID 순회 도구).
