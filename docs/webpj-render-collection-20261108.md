# webpj 표시 수집기(RawRenderCollection) 이식 — WB04-E131

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른한 번째 부분이다. 원본 `Particle.cpp` 의 collector — 등록된 표시 영역을 월드 좌표로 환산하고, 가시 영역과 교차하는 표시 객체를 16바이트 그리기 항목으로 모으는 `o/RawRenderCollection`(cpppj 기준선 `1f00a25a`)을 옮겼다. 선행은 [표시 목록 수명(E129)](webpj-construction-lifetime-20261108.md)의 `RawRenderRegistry`·`createRenderProcessFields` 와 기존 `www/render/draw-sort.js` 의 `compareDrawOrder` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 의 직접 나눗셈 환산과 x87 분기 차이를 따르는 교차 판정은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-render-collection.js`

- `RENDER_ENTRY_BYTES` (16) — 그리기 항목의 바이트 폭: x(4)·y(4)·token(4)·depth(int16)·kind(1)·tail(1). 출력은 항목 n 개의 `Uint8Array` 이다.
- `RawRenderCollection(pool, visualState, registry, lookup)` — `registry.pool` 과 같은 풀이고 `lookup` 이 함수일 때만 받는다. `lookup(token)` 은 토큰의 현재 등록 필드를 돌려주며 `null` 은 생성 중인 호스트 객체를 건너뛴다.
  - `bounds(fields)` (00487a40): 부모 head(WORD)가 0 이 아닌지 확인(부모 범위 밖·head 0 은 거부)한 뒤 배율이 유한 비영인지 확인하고 `1/배율`(double)을 먼저 float 로 반올림하지 않은 채 좌표에 곱해 float 로 저장한 월드 사각형을 돌려준다. 실제 필드와 계산 결과의 NaN/무한대는 교차 판정에 그대로 전달한다.
  - `RawRenderCollection.intersects(view, bounds)` (00487740): 두 사각형이 모두 정상(왼쪽≤오른쪽, 위≤아래)이고 네 변이 엄격히 겹칠 때만 참. 경계만 닿으면 제외하고 NaN 은 거짓이다. 점/선 사각형은 내부에서 허용한다.
  - `collect(view, output, limit)` (00487f10): `limit>0` 이 항목 수보다 크면 변경 전에 거부하고, 표시 목록 슬롯 0 번부터 순서대로 — 먼저 `count>=limit` 이면 종료(다음 슬롯의 부모를 조회하지 않음), 빈 칸/조회 null 은 건너뜀, 교차하지 않으면 건너뜀 — 항목을 쓴다. `x` 는 부모 raw 의 float 바이트를 그대로 복사하고 `y` 는 `float(부모 y) + 0.001`(원본 float32 `0x3a83126f`)을 float 로 저장(같은 부모의 자산보다 뒤에 오도록), `token`, `depth`(등록 zOrder 의 하위 WORD), `kind=3` 을 쓰고 `tail` 과 미사용 항목은 보존한다. `limit<=0` 이면 0 이다. 별도의 부모 free/dead/void/abstract 필터는 추가하지 않는다.

정렬(`draw-sort.js`)과 실제 그리기는 별도 단계이다.

## 2. 검증

`tests/render-collection.test.mjs`(2건):

1. `rendercollection-x86.tsv` 의 `originals` 1435행(Bounds 226·Intersect 225·Collect 948·Order 36; `originalCD`·`original1037` 은 다른 판본이라 제외) — 배율·등록 영역·가시 영역·슬롯 구성(빈 칸 포함)·제한 수·부모 raw(head·좌표·상태 byte)를 주었을 때 환산된 월드 사각형 16바이트(NaN 비트까지 그대로), 교차 판정(독립 helper 와 collector 내부 판정 모두), 반환 개수와 128바이트 출력(미사용 칸 `0xa5`와 항목 tail 보존 포함), 그리기 정렬 비교(`compareDrawOrder`) 결과가 모두 같다.
2. 계약: 다른 풀의 어댑터와 조회 함수 없음 거부, 배율 0/무한대/NaN 거부, 부모 번호 −1·풀 크기·head 0 거부, 출력 용량보다 큰 limit 거부(출력 보존), 출력을 다 채운 뒤 head 가 빈 후속 부모에 접근하지 않음, limit 0/−1 은 출력 없이 0, 조회가 null 인 슬롯은 건너뜀.

최초 실패와 교정: 새 모듈은 1435행 fixture 를 첫 실행에 통과했다. NaN 비트를 정규화해 비교하던 임시 검사 보조를 시험해 보고, 없어도 비트까지 같음을 확인해 제거했다(더 엄격한 비교).

## 3. 경계와 남은 것

- 실제 표시 객체의 그리기 항목 정렬·그림자료 연결·화면 출력과 `ConstructionVisualRenderer` 는 클라이언트 렌더러 쪽 후속 단위이다.
- `lookup` 의 실제 구현(호스트 객체 테이블)은 월드/클라이언트 연결 단계에서 만든다.

## 4. 다음 시작점

`RawConstructionProcess` 계열, Missile Host 팩토리(`RawGunProcess` 이후), `RawGunImpact`(SquidFrame/Host 장면 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체, 번개 수명/프레임/화면, `RawPathProcess` 계열(커널/Form 장면 선행), `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
