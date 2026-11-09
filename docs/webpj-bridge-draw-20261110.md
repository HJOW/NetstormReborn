# webpj 다리 캡처 가상 그림 몸체 이식(RawBridgeDraw) — WB04-E245

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 셋째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawBridgeDraw.h`(헤더 전용 38줄; 원본 4222f0 다리 기본 프레임 getter와 4223c0 추가 그림 가상 몸체)를 옮기고, 이것을 E199 `RawShapeToBuffer`에 연결해 다리 캡처 1,200행을 원본 관측으로 대조했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 `RawBridgeDraw.h`에 없다(저장소 루트 `git diff --numstat 1f00a25a HEAD`가 빈 결과).

## 1. 옮긴 것

`www/core/raw-bridge-draw.js` — `createBridgeDrawState()`, `RawBridgeDraw(pool, state)`, `BRIDGE_DRAW_VTABLE`

- `createBridgeDrawState()`: 실제 5453ac의 기본 0에 해당하는 현재 수명 숫자 표시 전역 `{ lifeNumbers: 0 }`. 호출자가 값을 바꾸면 다음 `additional`이 그 값을 읽는다.
- `mainFrame(sid)`: 슬롯 +0의 가상 표 기록값이 다리 클래스(`0x005034c8`)가 아니면 오류, 맞으면 프레임 DWORD(+36)를 signed int32로 돌려준다(`RawShapeToBuffer`가 `extra & 2`일 때 기본 그림으로 쓰며 -1은 "그리지 않음"). 현재 프레임을 캐시하지 않고 호출 때마다 풀 슬롯을 다시 읽는다.
- `additional()`: `lifeNumbers`가 0이면 source/x/y를 읽지 않고 정상 반환(원본 4223c0). 비0이면 미복원 개발용 숫자를 빈 성공으로 넘기지 않고 오류로 멈춘다.
- 생성은 10.78 풀(`pool.edition === 'patch1078'`)만 받는다.

## 2. 검증 (`tests/bridge-draw.test.mjs`, 2건)

cpppj `BridgeCollapseTests.cpp`의 `BridgeCollapse_NativeWrapperAndAllOwnerFrames`를 같은 입력·순서로 옮겼다. 원본 fixture는 `cpppj/tests/fixtures`를 직접 읽는다.

- `bridge-collapse-x86.tsv`: wrapper 3행(셋째 가상 픽셀 생성기 종류 2, `ShrapnelKind.Variant2` 확인)과 보호 60프레임 × 소유자 0/9색 캡처 600행(extra 0). `bridge-extra-capture-x86.tsv`: 실제 extra 2의 600행(기본 프레임은 `mainFrame`, 추가 그림은 `additional`). 합쳐 캡처 1,200행·wrapper 3행.
- 각 행은 raw 슬롯 50(다리 vtable·TYPE 82·소유자·프레임·extra)과 표준 소유자 색표(`buildStandardOwnerColorMaps(82, 82)`)만 설치하고, 캡처 메타데이터 16바이트(폭·높이·기준점 X/Y)와 전체 픽셀 hex를 대조한다. 소유자 색표 소비자는 소유자가 0이 아닐 때 정확히 한 번 불린다.
- TYPE 기준점은 원문 입력대로 0,0이다(웹 TYPE 레코드는 기준점 0이라 cpppj 시험 중 `실제 TYPE 기준점` 출력은 옮기지 않았다).
- 계약 검사: 다른 가상 표 거부, 프레임 DWORD의 signed 해석(0xffffffff → -1, 0x80000000 → -2^31), 현재 프레임 비캐시, 수명 숫자 비0 거부, 10.78이 아닌 풀(없는 풀) 거부.
- cpppj `CampaignSessionTests`가 같은 `RawBridgeDraw`를 실제 다리 파괴 캡처에 쓰는 부분은 client 세션 단위(4-D)에서 대조한다.

최초 실패와 교정: 처음부터 통과했다. 변형 8개(프레임 오프셋 40·unsigned 해석·가상 표 검사 제거·가상 표 상수 오타·수명 숫자 무시·판본 검사 제거·프레임 캐시·프레임 값 바꿈)를 넣어 확인했고 7개는 바로 탐지했다. 살아남은 하나는 가상 표 상수 오타였다. 처음 검사가 설치용 값을 모듈에서 import해 같은 값으로 짝을 맞췄기 때문이어서, 설치를 원문 리터럴 `0x5034c8`로 바꿔 닫았다.

## 3. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 World의 다리 캡처 소비자(파편 `RawShrapnelFrame`·위치 음향)와 렌더러 연결은 4-D·5-A에서 잇는다.
- 이 단위로 `RawBridgeDraw`는 PORT_MAP `port`다. 3-B의 다음 항목은 `Squid`(저장 필드·공간 단계의 나머지)다.
