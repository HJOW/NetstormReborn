# webpj 로컬 간헐천 생성 이식(RawLocalGeyser) — WB04-E256

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 여덟째(마지막) 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawLocalGeyser`(Early1·전투 시작의 Player별 간헐천 배치: 원본 00486360 전체 생성·00486130 요청·00486030/00485890 후보·004857d0 Player 제외 영역)를 옮겼다. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +25/−9`·`.h +25/−0`·`LocalGeyserTests.cpp +20/−1`(+74 줄), 커밋은 `ed29febb`(20261109-40, 건설13참조와 현재 Player base 선택 레코드 수명 연결)·`3102fadd`(-44, Player 미할당 단계 건설 지연 구성과 실제 선택 경계 복원)·`5f4b9d30`(-53, 현재 건설 전역과 공유 마스크 및 효과 뒤 조회 순서 연결)다. 기존 fixture(`local-geyser-x86.tsv` 221행·`geyser-placement-x86.tsv` 183행)는 기준선 뒤 바뀐 적 없고 10.78 typed 결과는 기준선과 같다.

- **② 재반영:** 모두 단독 결과를 바꾸지 않는 장기 owner 진입이다.
  - 공유 전역 getter: `sharedFort`·`sharedEmptyGeyserType`·`sharedNoIslandType`·`sharedIsleType`·`sharedFirstQuantity`·`sharedLastQuantity`(기준선의 `sharedBoss`·`sharedGeyserType`과 함께). 읽기 접근자(`readXxx`)는 소비 위치마다 현재 값을 다시 읽는다 — 후보 constructor(빈 간헐천 TYPE)·실제 교체 조회·받침 확정(noIsland)·지면 프레임 문자 비교(isle)·Player 차례의 수량 조회.
  - 현재 Player base: `playerBoundsRecord(owner, record)`(선택 레코드를 경계 member 에 전달), `bindCurrentPlayers(getter)`, 그리고 Player 9기록을 생성 인자로 받지 않는 `RawLocalGeyser.deferred(getter, …)`. 매 Player 차례(`generate`)·제외 순회(`playerExclusions`)가 반복마다 현재 base 를 다시 고르고, 한 차례 안에서는 앞서 고른 같은 레코드(`subarray`)를 경계 호출까지 유지한다. 구성 때는 Player 를 조회하지 않는다.
- 행 base 는 판정한 `5f4b9d30`으로 올렸다. 호출자(client Session 의 간헐천 생성)와 훅의 실제 몸체 연결은 4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-local-geyser.js` — `LocalGeyserState`·`RawLocalGeyser`(`generate`·`populate`·`tryCandidate`·`playerExclusions`·`bindResourcePool`·`readResourcePool`·`bindCurrentPlayers`·`deferred`).

- 후보(`tryCandidate`): 높이 난수 뒤 폭 난수(한계가 0 이하이면 시드를 소비하지 않음, 폭/높이가 정수로 0 이면 난수 없이 사각형 모서리)로 점을 구해 float 로 저장하고 0.5 를 더해 바닥으로 반올림한다 → 제외 사각형(네 변 포함) → 3×3 첫 칸(반지름 2)이 사각형 안인지 → 3×3 칸을 x 바깥/y 안쪽으로 훑으며 칸마다 spot 을 먼저 읽고 기존 빈 간헐천(reuse TYPE, 표면/다리 부류면 hash0 포함)이 있으면 그 슬롯 좌표로 즉시 성공, 아니면 spot 하위 비트(0x3d)·땅/허공 허용·한 종류 지형·지면 프레임 문자 무늬 "FBICANGDH"(대소문자 무시) 검사이다. 첫 거부 칸에서 즉시 끝난다.
- 요청(`populate`): boss/요새 관문 → 수량 음수 진단(`negativeQuantity`) 뒤 0 → 최근 목록 비움 → (요청 시) Player 제외 영역 캡처 → 수량이 0 이 될 때까지 후보 100회(실패하면 요청 전체 종료) → 같은 자리 빈 간헐천 삭제(0x400000) → spot bit1 이 없으면 받침(noIsland) 확정 → 현재 TYPE factory(0 이면 오류) → Pop → 최근 5목록 추가 → 자원 pool(float 저장 뒤 비용 절삭, 0 미만은 0)이다.
- 전체 생성(`generate`): 시드 0xfed(설정 predictableGeysers 가 0 이면 현재 TimeSeed), Player 1~8 순서, 각 차례에서 현재 flags(0x22)·지역(127 아님)을 다시 읽는다.
- 자원 pool: `bindResourcePool({ value })`로 실제 단일 저장소(`GameAuxState.resourcePool` 등)의 상자를 빌린다. 값 복사나 동기화를 하지 않으며 삭제/Pop 효과 뒤에도 같은 상자의 현재 값을 다시 읽는다. 연결 전 독립 입력은 `state.resourcePool`이다.
- C++ 출력 참조는 JS 에서 `point = { x, y }` 객체, 최근 목록은 `SquidPostPopList`(`readCount`·`storage`·`capacity`·`setCount`로 연결된 장기 목록도 지원)이다.

## 3. 검증 (`tests/local-geyser.test.mjs`, 26건)

| fixture (10.78 사용 행) | 대조 내용 |
| --- | --- |
| `local-geyser-x86.tsv` 221행 | 후보(T)·요청(P)·전체 생성(G)의 사용 여부·마지막 좌표 비트·로컬 난수 상태·최근 목록 개수와 전체 칸(count 밖 꼬리)·자원 pool·수량 전역·생성 횟수·외부 효과 순서. 독립 값 상태, 공유 자원 상자, 장기 owner(`GameAuxState`)의 자원+반대 fallback 현재 TYPE, 생성 뒤 현재 Player base 교체, deferred 구성으로 다섯 번 재생 |
| `geyser-placement-x86.tsv` 183행 | 일반 요청의 허용 지형/허공·균일 지형·제외 영역(최대 8)과 현재 Player 재조회 후 결과·좌표·난수·최근 목록·자원·수량·효과 순서와 Player 9기록 전체 바이트 |

- cpppj 개별 시험(`LocalGeyserTests` 전부)을 옮겼다: 생성 누락 거부, 공유 boss 가 수량 0 의 최근 목록 비움을 정함, 일반 제외 요청의 선택 요구, `generate`가 Player 차례마다 base 를 다시 고르고 경계 부모를 권한 관문 앞에서 부름, 제외 순회가 선택 레코드를 유지.
- 경계 계약(변형 시험에서 살아남은 입력): 상태 읽기 접근자 8종의 부호 규칙(u32 부호 없음·수량 signed), 생성자 누락 거부 전수(판본·타입/프레임 표 각각과 둘 다 짧음·spot·Player·최근 목록·필수 하위 여덟), 현재 Player 연결 중복/누락/크기, 자원 상자 다른 주소 거부, 폭/높이 0·무한대·NaN 사각형의 난수 비소비와 한계 1 의 시드 소비·난수 순서, float 저장 뒤 반올림(15.4999990463+1)과 음수 .5/.75, 제외 사각형 네 변 포함, 3×3 첫 칸 경계(사각형 왼쪽/위와 같아도 허용), spot 0x3d 비트와 땅/허공 허용·종류 혼합·한 종류 지형, 무늬를 보드 모서리에서도 읽고 소문자를 대문자로 바꿈·공유 isle TYPE, 지면 TYPE/프레임 범위 오류, spot 음수/정확히 표 길이 거부, 기존 빈 간헐천의 x/y 구별 재사용·TYPE 정확 일치·표면 부류 hash0 조회·현재 빈 TYPE 범위, 받침 확정의 spot bit1 규칙과 공유 TYPE, 비용 TYPE 범위, 최근 개수 음수 오류, 공유 수량/설정 값 규칙, 지역 DWORD signed 전달과 float 저장한 확장 경계.

최초 실패와 교정:

- 모듈은 처음부터 fixture 두 종(221·183행)과 cpppj 계약 전부를 통과했다(첫 실행 11/11).
- 변형 134개: 처음 64개만 탐지(70개 생존)해 경계 계약 15건을 추가했다. 그 과정에서 확인한 것: (a) 재사용 좌표 읽기 변형은 fixture 의 빈 간헐천 좌표가 x=y=255 라 구별하지 못했다(x≠y 좌표 추가), (b) `find` 가 TYPE 을 비교하지 않는 변형은 검사 TYPE 에 발자국이 없어 finder 가 후보로 돌려주지 않아 살아남았다(발자국 추가), (c) 정확히 표 길이(65536)를 읽는 변형은 다음 칸이 어차피 범위 오류를 내 가려졌다(마지막 칸만 표 길이가 되는 사각형으로 분리).
- 남은 14개는 동등 변형이다: 후보 spot 보정 0.9999(float 상수 0.99989998 과 다른 구간에 float 가 없음), 좌표 절삭의 2^63 경계(하위 32비트가 같은 0), 한계 0 의 난수(폭/높이 0 을 먼저 걸러 도달 불가), 3×3 첫 칸의 오른쪽/아래 경계(첫 칸이 항상 점보다 작아 같을 수 없음)와 첫 칸 float 저장(정수 좌표라 항등), 루프 순서 표기(무동작 변형), 정수 범위 검사(그 전에 spot 범위 오류가 먼저 남), 종류 마스크 0x3f/0x0f(0x3d 가 나머지 비트를 먼저 거부), 프레임 번호 부호(어느 쪽이든 같은 범위 오류), 소문자 변환 끝 글자 z(무늬에 없는 글자), 무늬 NUL 종료(최대 9칸이라 완성 무늬 뒤에 올 수 없음), 최근 목록 용량 비교(개수가 5 미만이어야 들어가 같을 수 없음), 자원 절삭의 바닥/0 쪽 절삭(0 미만을 0 으로 자르므로 같음).

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 훅(설정·벽시각·Player 경계 레코드·삭제·건설 확정·factory·Pop·지역 존재)의 실제 몸체와 공유 전역 getter·자원 상자·현재 Player base 조회는 4-D 에서 잇는다.
- 이 단위로 `RawLocalGeyser`는 PORT_MAP `port`(base `5f4b9d30`)이고 **3-C 8/8 완료**다. 다음 단계는 3-D(`RawGraph`·`SquidPostPop`)다.
