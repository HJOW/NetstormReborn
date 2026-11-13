# webpj GumpCollection — UI(Gump) 트리 위치·경계·표시 수집기 (WB04-E395, 2026-11-13, 5-A 스물세 번째 단위)

`ClientRendererFrame` 의 선행 부품인 client **GumpCollection**(cpppj `client/GumpCollection.cpp/.h`, 기준선 197줄, 원본 Gump.cpp 의 화면 위치·경계·트리 수집, 상태 todo)을 `render/gump-collection.js` 로 옮겨 port 로 올렸다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). CD 경로(월드 부착 부모의 정수 위치를 먼저 float 로 저장)는 웹이 10.78 만 지원해 옮기지 않았다. 노드 저장소와 월드→화면 투영은 호출자가 훅(`node(token)`·`project(object)`)으로 공급한다(UI 트리 본체 `Gump`/`GumpTree`/`UberGump` 은 이후 단위).

## 1. 규칙

- **노드:** `{ flags, next, parent, child, object, depth, width, height, x, y, cached: { x, y } }`. flags 비트는 visible 1·화면 밖 자식 통과 0x20·월드 부착 0x2000·삭제 대기 0x10000. 링크/깊이는 부호 없는 DWORD, 크기·위치는 float32 값, `cached` 는 일반 위치 조회만 갱신하고 월드 부착 자신은 보존한다. 상태 `{ root, overrideHead }` 는 호출 때마다 읽는다.
- **`position(token)`:** 월드 부착 노드는 투영점 + CRT 절삭한 로컬 위치(DWORD 덧셈). 일반 노드는 부모 체인을 따라 **각 단계마다 float32 로 저장하며** `부모 x + 자식 x` 를 더하고(월드 부착 부모는 투영점을 더함, 부모 자신의 캐시는 쓰지 않음) 마지막에 CRT 64비트 절삭의 하위 DWORD(signed; 비유한/±2^63 밖은 0)로 `cached` 를 갱신한다. 월드 부착은 반드시 루트 직계 자식이어야 하고 투영 훅이 필요하다.
- **`bounds(token)`:** 정수 위치에 배율 적용 크기를 더해 절삭(합산 직후 float 로 다시 반올림하지 않음). **`screenToLocal(token, point)`:** 004650a0 — 각 뺄셈 뒤 float32 저장, 루트 자신의 위치 생략, 월드 부착 부모를 만난 뒤에는 일반 부모의 로컬 위치를 쓰지 않고 위치 캐시는 갱신하지 않는다. **`flags(token)`:** 현재 raw 플래그.
- **`collectNode(token, view, output, limit)`:** 깊이 우선. 현재 노드의 경계가 view 와 **엄격히 교차**(맞닿음 제외, 역전 사각형 거부, 노드 내부의 영크기 허용, view 가 한 점이어도 노드 안이면 교차)하면 16바이트 항목(x·y=노드 위치 float, token, depth WORD, kind 2, tail 0)을 쓰고 남은 개수를 줄인다. 교차하지 않아도 0x20(통과) 플래그면 자식으로 내려간다. 자식은 visible 만 검사하고 dead 필터는 없다. 남은 개수가 1 미만이면 0. 출력 칸이 없는데 쓰려 하면 거부한다. 경로/형제 체인이 1000개를 넘거나 순환이면 거부한다.
- **`collect(view, output, limit)`:** `root.child`(또는 `overrideHead`)부터 `next` 순서로, 다음 링크는 재귀 전에 저장하고 visible 이며 dead 가 아닌 최상위만 수집한다. limit 은 **각 최상위에 같은 값**으로 전달해 전체 반환 수가 limit 을 넘을 수 있다. `bindGumpCollection(hooks, collector)` 는 공통 수집 훅의 `ui` 를 잇는다.

## 2. 검사 (`tests/gump-collection.test.mjs` 10건 + `object-collection.test.mjs` 1건)

- **원본 독립 관측(첫 실행 일치):** `gumpcollection-x86.tsv` 의 **originals 389행**(Position/Bounds/Collect/CollectNode: 루트·override·대상·한도·화면 사각형, 128바이트 노드 원본, 월드 투영 응답(정수점과 호출 때 바꿀 next) → 반환 좌표/경계/개수, 출력 32칸 전체, 전 노드 캐시, 투영 호출 순서, 출력 앞뒤 0xa5 보호 항목)을 재생한다(manifest `gumpcollection`; CD·추가판 행은 10.78 전용이라 제외).
- **cpppj 시험 이식:** `PerRootLimitAndSafeBuffer`, `RejectsDamagedLinksAndMissingProjection`, `CommonSubmissionIntegration`(실제 트리 수집→병합→정렬→kind 2 분배와 UI 전용 dirty) — 그리고 **E389 가 미룬 `ObjectCollection_CommonSubmissionIntegration`**(UI·지면 묶음·일반 객체 본체/그림자 세 수집기가 한 공통 목록에서 병합·정렬되어 `ui:2`, `6:0:4`(그림자), `6:0:0`, `5:1:0`(지면 내부 kind 1) 순으로 분배되고 묶음 숨김·UI 전용 dirty 가 동작)을 `object-collection.test.mjs` 에 이식했다.
- **직접 계산 보강(fixture 가 다루지 않는 경로 6건):** 위치(float32 누적·음수 절삭·비유한/±2^63 밖·부모 캐시 불변·반환 사본)·월드 부착(DWORD 감김·루트 직계 요구)·`screenToLocal`(루트 생략·float32 저장·월드 부착 이후 일반 부모 생략·손상 링크)·수집 규칙(override·dead 최상위 제외·통과·깊이 부호·한도·출력 부족)·경계 절삭 범위·교차 판정/1000개 한도.

## 3. 변형 확인

`gump-collection.js`(25~200행) 연산자·상수 변형 96회: 1차 생존 12건 → 시험 보강(범위 밖 합의 비영 하위 DWORD·영크기 view·1000개 경계) 후 **92/96 잡음, 생존 4건 모두 동등/비관측**이다: ① 절삭 범위 경계(`>=`→`>`, `<`→`<=`: 정확히 ±2^63 의 하위 DWORD 가 0 이라 같은 결과), ② 출력 칸 검사 `>=`→`>`(경계 항목 쓰기가 `DataView` 범위 오류로 어차피 거부), ③ `setUint32` 의 `>>> 0`→`>> 0`(같은 비트).

## 4. 남은 경계

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 실제 UI 트리 본체와 월드 부착 투영(`UberGump*`·`GumpTree`·`Gump`·카메라 투영), 그리고 `ClientRendererFrame.Collect` 의 수집기 묶음은 해당 단위에서 한다.
- `ClientRendererFrame` 선행은 이제 `ClientKencloudGlobals`·`RawCampaignCursorDisplay`·`RawCampaignFactoryDisplay`·`RawMenuControls`·`GameWorld`·`ClientGameplayGlobals` 이다(`TileGroupCollection`·`GumpCollection` 은 끝남).
