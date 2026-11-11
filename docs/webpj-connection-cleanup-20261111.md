# webpj 연결 정리(RawConnectionCleanup) — WB04-E307 (2026-11-11, 4-C)

3.1절 4-C 의 **RawConnectionCleanup**(`o/`, 기준선 81줄 = `.cpp` 46 + `.h` 35, 기준선 뒤 변경 없음)을 `core/raw-connection-cleanup.js` 로 옮겼다. 연결표와 수신 깊이의 정리 순서(004df440·004df520·004e2940·004e29c0·004e2a10·004df4a0/004e2a00)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 체크 목록 순서는 RawConnectionRegistry → Registration → Cleanup 이었으나 Registry 의 cpp 가 이 부모와 [Registration](webpj-connection-registration-20261111.md)의 자유 함수를 호출하므로 **Cleanup → Registration → Registry** 로 바꿨다(3.1절 4-C).

## 1. 규칙

- `createConnectionCleanupGlobals()`: 장기 표 묶음 `{ transceivers(22), buffers(22), pending(32), synchronization: { value }, pendingCount: { value } }` 를 만든다. 실제 Client 는 같은 표/상자를 계속 **빌리며** 새 0 표로 대체하지 않는다(독립 전송의 첫 BSS 0 은 이 함수가 대신한다).
- 연결 숫자 토큰은 호출자가 같은 실제 연결 저장소에서 해석한다: `hooks.node(token)` → 같은 실제 연결 객체(`PacketConnection`), `hooks.destroy(token)` = 004e03d0 고정 연결 소멸, `hooks.release(token)` = 004e4354 연결 또는 별도 송신버퍼 free, `hooks.assertion(line)` = 004e0620 줄 250 진단.
- `deferConnectionClose(connection, destroy, release)`(004df440): 진입 깊이를 먼저 보관 → `flags |= 0x40`(unsigned) → 보관한 깊이가 0 일 때만 소멸 → 반납. 소멸 뒤 깊이를 다시 읽지 않는다(수신 중이면 호출 쪽 분배기가 닫힘을 처리).
- `closePendingConnection(id, …)`(004df520): -31..-1 의 실제 칸만 종료하고, 소멸 callback 뒤의 **현재 개수에서 1 감소(unsigned 감김)** → 칸 0 → 개수 store. 빈 칸·범위 밖은 아무 일도 없다.
- `disconnectTransceiver(id, …)`(004e2940): 음수는 pending 위임. 22 이상은 줄 250 진단 뒤(반환하면) 부족한 실제 표를 거부한다. 양수는 동기화 토큰이 같으면 0 으로 풀고 연결을 종료하며 칸을 0 으로 만든 뒤, 별도 송신버퍼가 있으면 free 하고 0 으로 쓴다.
- `closeAllTransceivers`(004e29c0): 현재 0..21 을 한 번 순회하며 동기화·연결만 정리하고 **버퍼표는 보존**한다. 각 슬롯은 callback 뒤의 현재 값을 다시 읽는다(앞 칸을 채워도 같은 호출에서 되돌아가지 않음).
- `endPlayerConnections(server, …)`(004e2a10): 진입 `server` 가 비영일 때만 1..8 연결을 종료한다.
- `closeAllPendingConnections`(004df4a0/004e2a00): 0..31(0 칸도 포함)을 종료하고 칸만 0 으로 만들며 **개수·동기화·양수 표는 쓰지 않는다**.
- 선택한 장치는 **도달한 위치에서만** 요구한다. 빠졌으면 그때까지의 쓰기(flags·동기화 해제·소멸 효과)를 남긴 채 오류로 멈춘다. 빈 표·범위 밖 음수는 장치를 전혀 요구하지 않는다. 호스트 표 물리 부족(`checked`)은 해당 슬롯을 읽는 위치에서만 거부한다.

## 2. 기준선 뒤 변경 판정

`git log 1f00a25a..HEAD -- cpppj/src/o/RawConnectionCleanup.{cpp,h}` 가 비어 있다(시험·fixture 도 변경 없음) → 3.6절 ① (옮길 변경 없음). CD(10.72)는 옮기지 않았다. 게임 모드/Player/로그인 성공은 만들지 않는다(실제 Registry 가 소유 토큰과 수명을 담당, 다음 단위).

## 3. 검사 (`tests/raw-connection-cleanup.test.mjs` 6건)

- **원본 독립 관측 `connection-cleanup-x86.tsv` 580행 전체 재생**(originals 10.78 PE 여섯 몸체 × 두 FPU 환경 × 소멸/free/진단 중단 × 호출 도중 변경): 22 연결·22 버퍼·32 pending 표, 동기화/개수/server, 노드 flags·sender·received·heartbeat·깊이를 입력 892바이트 위에 되써 기대 출력과 비교하고 효과열(`D`/`F`/`A`)과 중단 여부(268행)를 대조한다.
- 손 시험(cpppj `ConnectionCleanupTests.cpp` 이식): 늦게 드러난 누락 장치의 prefix 보존(flags·동기화 해제·소멸 완료), 빈 표·범위 밖 음수의 무장치, `disconnectTransceiver(22)` 의 줄 250 진단 뒤 RangeError, 지연 닫힘의 깊이 보관·flags unsigned, pending 개별/전체의 개수 규칙, 표 칸 수와 초기값.
- 변형 확인 38개 중 **2개만 남았고 모두 동등**하다: pending 개수 읽기와 칸 0 쓰기의 순서(서로 독립 필드), `checked` 의 음수 검사(이 모듈은 음수 index 를 만들지 않음). 버퍼 표 칸 수 변형은 표 크기 시험을 추가해 잡았다.

## 4. 남은 경계

- 실제 소켓·Registry 수명(토큰 → 연결 객체 해석, 소유 추적)은 다음 단위 RawConnectionRegistry 이다. 3.1절 4-C 는 이 문서와 Registration 문서 기준 4/30 이다.
