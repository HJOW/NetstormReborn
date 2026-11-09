# webpj 다리 낙하 진단 부모의 실제 Host 연결(RawBridgeFallParent 마감) — WB04-E244

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 둘째 항목이다. [E15](webpj-small-parents-20261108.md)가 옮긴 `RunBridgeFallParent`(원문 00421f90)가 PORT_MAP에서 `partial`로 남긴 `ScheduleBridgeFallChecked`(cpppj 실제 Host 연결)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 `RawBridgeFallParent.cpp/.h`에 없다(저장소 루트 `git diff --numstat 1f00a25a HEAD`가 빈 결과).

## 1. 옮긴 것

`www/core/raw-bridge-fall-parent.js` — `scheduleBridgeFallChecked(host, pool, types, bridge, x, y, checking, assertion = null)` 추가(기존 `runBridgeFallParent`는 그대로).

- `host.pool`이 `pool`과 다르면 `TypeError`(cpppj `invalid_argument`)로 등록 전에 거부한다.
- genus 장치는 `pool.slot(bridge)[10]`(현재 TYPE 번호)로 `types[번호].flags2`를 읽고, 번호가 표 길이 이상이면 `RangeError`(원문 물리 범위 밖 읽기를 오류로 바꿈)다. `checking`이 0이면 `runBridgeFallParent`가 genus를 읽지 않으므로 표 밖 TYPE이어도 예약한다.
- 예약 장치는 기존 `scheduleBridgeFall`(`squid-process.js`, 00421530 좌표 포장과 Regular 생성)이다. 반환은 그 프로세스다.
- 진단 콜백이 있으면 복귀를 허용해 같은 SID와 원래 좌표로 예약까지 이어지고, 없으면 `원문 다리 낙하 진단 \Ns\O\Bridge.cpp:803: s.getGenus() & gBRIDGE` 오류로 등록 전에 멈춘다.
- cpppj의 CD 분기(`pool.Edition() != Patch1078`이면 진단 없이 직접 예약)는 10.78 전용 웹 풀에는 도달하지 않아 옮기지 않았다.

## 2. 검증 (`tests/bridge-fall-parent-host.test.mjs`, 2건)

- cpppj `BridgeFallParent_ActualHostChecksBeforeRegistrationAndKeepsChild`를 같은 입력·순서로 옮겼다: 첫 예약(`event`=0x2692·`payload`=5396·`time`=9·`count`=0·Kernel 1) → checking=1이면 같은 현재 TYPE을 다시 진단해 실패하고 Kernel 크기·풀 빈 슬롯 불변 → 다리 genus가 있으면 두 번째 예약 → 콜백 복귀 뒤 세 번째 예약(콜백 한 번, 조건/파일/줄 `s.getGenus() & gBRIDGE`·`\Ns\O\Bridge.cpp`·0x323).
- 연결 경계 계약: 풀 불일치 `TypeError`, TYPE 188(표 길이와 같음)·200 `RangeError`, checking 0의 표 밖 TYPE 예약, 콜백 없는 기본 진단의 원문 조건/파일/줄 메시지.
- 원본 10.78 관측 200행(작은 부모 `runBridgeFallParent`)은 E15 `tests/small-parents.test.mjs`가 그대로 대조한다. 이 단위에서 다시 하지 않는다.
- cpppj의 세 번째 시험 `BridgeFallParent_StorageViewRetainsDwordAcrossWorldUses`는 `client/RawSurfaceStorage`의 Checking 뷰(Acquire/반납 뒤 같은 owner를 읽는 공유 저장소) 수명 시험이라 이 부모의 입력이 아니라 client 저장소 단위(4-B 영역 B)에서 대조한다. 웹의 `checking`은 getter 함수로 받는다.

최초 실패와 교정: 2건 모두 첫 실행에 통과했다. 변형 9개(풀 검사 제거·flags1 읽기·콜백 무시·반환 null·슬롯 오프셋 11·좌표 뒤바꿈·메시지 줄 변경·checking 생략 등)를 넣어 확인했고, 8개는 바로 탐지했다. 살아남은 하나는 표 경계 비교 `>=`→`>`(번호 188에서 `undefined` 접근이 `TypeError`로 나오는 차이)였고, 경계 번호 188 사례를 추가해 닫았다.

## 3. 경계와 다음 시작점

- `scheduleBridgeFallChecked`는 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 호출자인 `RawBridgeEvents` 낙하 예약 소비와 실제 World 연결은 4-D에서 잇는다.
- 이 단위로 `RawBridgeFallParent`는 PORT_MAP `port`다. 3-B의 다음 항목은 `RawBridgeDraw`(38줄)다.
