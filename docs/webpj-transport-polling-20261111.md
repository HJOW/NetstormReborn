# webpj 전송 조회 도구(RawTransportPolling) — WB04-E310 (2026-11-11, 4-C)

3.1절 4-C 의 **RawTransportPolling**(`o/`, 기준선 18줄, 헤더만, 기준선 뒤 변경 없음)을 `core/raw-transport-polling.js` 로 옮겼다. 여러 실제 전송 owner 를 원문 하나의 현재 ID 순회로 묶을 때 쓰는 작은 결과/조회 도구이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `transportPollResult(consumed = false, again = false)`: 한 번의 조회 결과이다. `consumed` 는 한 완성 프레임을 실제 입력에서 꺼냈는지, `again` 은 같은 현재 정상 ID 를 다시 조회해 읽어야 하는지이다(무효 프레임의 소비와 길이 0 sentinel 의 반복 중단을 구별). 값은 불리언으로 정규화한다.
- `currentTransportToken(registry, id)`: [연결 등록소](webpj-connection-registry-20261111.md)의 **현재** 표에서 원문 -31..-1 pending 과 0..21 정상 칸의 토큰만 읽는다. 음수는 pending(`-id` 칸), 0 이상은 정상 연결 칸이며 빈 칸은 0 이다. `-32` 이하·`22` 이상·비정수는 RangeError(`원문 현재 전송 ID 범위 오류`). 매 호출 현재 표 값을 읽고 이전 조회를 재사용하지 않는다. 실제 노드와 표는 등록소가 계속 소유한다.

## 2. 기준선 뒤 변경 판정

`git log 1f00a25a..HEAD -- cpppj/src/o/RawTransportPolling.h` 가 비어 있다 → 3.6절 ①. CD(10.72)는 옮기지 않았다. 이 도구를 쓰는 전송 owner(`LocalMissionTransport` 등)는 이후 단위에서 옮긴다.

## 3. 검사 (`tests/raw-transport-polling.test.mjs` 3건)

원본 관측 파일이 없는 단위 계약 검사이다(cpppj 는 이 헤더를 쓰는 전송 owner 시험 안에서 간접 확인): 결과 구조의 기본값·정규화, pending/정상 칸의 현재 토큰과 경계(-31, -1, 0, 21)·빈 칸·범위 밖 12종 거부, 등록소의 수락/재배치/forget 에 따라 매 호출 현재 값을 읽음. 변형 확인 10개를 모두 잡았다.

## 4. 남은 경계

- 3.1절 4-C 는 7/30 이다. 다음: RawBattleBroadcast(현재 Player/연결 표를 읽는 두 송신 분배).
