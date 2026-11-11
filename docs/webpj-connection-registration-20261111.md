# webpj 연결 등록(RawConnectionRegistration) — WB04-E307 (2026-11-11, 4-C)

3.1절 4-C 의 **RawConnectionRegistration**(`o/`, 기준선 45줄 = `.cpp` 27 + `.h` 18, 기준선 뒤 변경 없음)을 `core/raw-connection-registration.js` 로 옮겼다. 실제 연결 등록·재배치와 pending 제거의 원문 store 순서(004e2870·004df4f0·004e27b0)이다. 앞 항목 [RawConnectionCleanup](webpj-connection-cleanup-20261111.md)과 같은 장기 표 묶음(`createConnectionCleanupGlobals`)을 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `registerTransceiver(id, token, globals, hooks)`(004e2870): 대상 칸이 차 있으면 줄 `0xc2` 진단 → 새 연결 store → 같은 노드의 `sender = id`(unsigned). 표 밖 id 는 칸을 읽는 위치에서 RangeError.
- `removePendingConnection(id, globals)`(004df4f0): `pending[-id] = 0` → **현재 개수** 를 unsigned 로 1 감소. 입력 검증을 앞당기지 않으므로 pending 표 밖 id 는 개수를 건드리기 전에 거부한다.
- `assignPendingConnection(pending, target, server, globals, hooks)`(004e27b0), 순서 고정: 서버면 대상 1..8(아니면 줄 `0xb0` 진단), 클라이언트면 대상 9(아니면 `0xb3`) → 대상 칸이 차 있으면 `0xb6` → 현재 pending 토큰을 양수 표에 store → `sender = target` → `hooks.buffered(현재 양수 칸 토큰)` → `pending >= 0` 이면 `0xbb` 진단 → pending 제거(callback 이 바꾼 **현재 개수**에서 감소) → 대상 반환.
- 진단 장치가 반환하면 원문처럼 계속 진행한다. 선택한 장치(`node`/`buffered`/`assertion`)는 **도달한 위치에서만** 요구하고, 빠졌으면 앞선 store(양수 칸·sender)를 되돌리지 않은 채 오류로 멈춘다. 대상 칸이 찬 상태에서 pending 이 표 밖이면 줄 `0xb6` 진단이 표 거부보다 먼저 불린다.

## 2. 기준선 뒤 변경 판정

`git log 1f00a25a..HEAD -- cpppj/src/o/RawConnectionRegistration.{cpp,h}` 가 비어 있다(시험·fixture 도 변경 없음) → 3.6절 ① . CD(10.72)는 옮기지 않았다. Registry 가 이 함수들을 부르는 방식(`Register`/`Assign`)은 다음 단위이다.

## 3. 검사 (`tests/raw-connection-registration.test.mjs` 5건)

- **원본 독립 관측 `connection-registration-x86.tsv` 288행 전체 재생**(등록·pending 제거·재배치 세 연산 × 두 FPU 환경 × 진단/buffered 중단 × 호출 도중 변경): 22 연결·32 pending 표, 개수, 노드 flags(buffered 가 켠 비트 2 포함)·sender 를 입력 800바이트에 되써 기대 출력과 비교하고 효과열(`A`/`B`)과 중단 여부(60행)를 대조한다. 호스트 표현 필드(received·heartbeat·깊이·delayed)는 변하지 않음을 함께 본다.
- 손 시험(cpppj `ConnectionRegistrationTests.cpp` 이식): 차 있는 칸의 `0xc2` 진단과 장치 누락, sender unsigned, pending 제거의 감김과 표 밖 거부, 재배치 순서(buffered 시점에 양수 칸·sender 는 이미 쓰였고 pending/개수는 아직 남음), callback 이 바꾼 현재 개수에서 감소, `0xb0`/`0xb3`/`0xb6`/`0xbb` 진단 위치, 중단 시 앞선 store 보존, 표 밖 대상.
- 변형 확인 32개 중 **2개 동등**(`sender` 의 `>>> 0` 은 표 범위를 통과한 0..21 에서 부호 차이가 없음, 등록·재배치 모두) 외 모두 잡았다. 진단 순서 변형은 위 `0xb6` 시험으로 잡았다.

## 4. 남은 경계

- 3.1절 4-C 는 4/30(Cleanup·Registration 완료)이다. 다음: RawConnectionRegistry(연결 토큰 소유·고정표·수명, 위 두 부모를 호출).
