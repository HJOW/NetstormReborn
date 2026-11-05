# webpj 이식 추적 목록(fixture manifest) — WB02-F

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 세 번째 선행 보완이다. 기존 webpj 검사가 쓰는 **원본 대조 자료(fixture)·원본 스크린샷·교차 확인 소스**를 한 목록에 모으고, 각 자료의 소비 검사·웹 모듈·판본·정규화 SHA-256/줄 수·cpppj 근거 커밋·실제 반영 범위를 기록했다. 목록 형식을 확정해 다음 구현 단위(WB04-B의 `hash-x86.tsv`·`rawfinder-x86.tsv` 등)부터 같은 방식으로 등록한다. dotnetpj 동기화 도구는 웹 검증으로 쓰지 않는다.

## 1. 파일

| 파일 | 역할 |
| --- | --- |
| `webpj/tests/fixtures/manifest.json` (신규) | 이식 추적 목록. 형식 `netstorm-webpj-fixture-manifest` v1, 35항목 |
| `webpj/tools/fixture-manifest.mjs` (신규) | 계산 칸 채우기(`--write`)·검사(기본: 해시 비교 + cpppj 변경 보고). 로컬 파일과 git 기록만 읽는다 |
| `webpj/tests/fixture-manifest.test.mjs` (신규) | 형식·참조 파일·해시·적용 범위 검사 3개(git 미사용) |

## 2. 형식

사람이 관리하는 칸과 도구가 계산하는 칸을 나눈다. 검토 커밋(단위 문서의 "검토 트리")과 실제 반영 커밋(`webCommit`)은 따로 둔다.

| 칸 | 관리 | 의미 |
| --- | --- | --- |
| `id`·`path` | 사람 | 고유 이름, 저장소 상대 경로 |
| `kind` | 사람 | `original-x86`(cpppj가 원본10.78 함수를 제한 x86으로 격리 실행한 관측) / `independent`(Python 등 독립 구현 기대값) / `screenshot`(원본 캡처) / `source`(cpppj·dotnetpj 교차 확인 소스, 해시 미기록) / `original-binary`(보호 원본 실행 파일의 정적 자료를 파일 읽기로만 대조, 2026-11-05 WB04-C2 추가) |
| `edition`·`required` | 사람 | 원본 판본(현재 모두 10.78), 없으면 실패할 필수 자료인지(webpj 기대값만 필수, cpppj fixture·스크린샷은 없으면 건너뜀) |
| `producer`·`consumers`·`modules`·`unit`·`doc` | 사람 | 생성 방법, 소비 검사, 이식한 웹 모듈, 단위 번호, 단위 문서 |
| `cpppjSources`·`scope` | 사람 | 웹 모듈이 옮긴 cpppj 복원 소스, 실제 반영 범위와 미연결 경계 |
| `text`·`sha256`·`bytes`·`lines` | 도구 | 텍스트(앞 8000바이트에 NUL 없음)는 **CRLF→LF 정규화** 뒤 SHA-256/크기, 비어 있지 않은 줄 수. 이진 파일은 바이트 그대로이며 줄 수 null |
| `fixtureCommit` | 도구 | 자료 파일을 마지막으로 바꾼 커밋 |
| `webCommit` | 도구 | 웹 모듈을 마지막으로 바꾼 커밋(실제 반영 커밋) |
| `cpppjBasis` | 도구 | `webCommit` 시점에 각 cpppj 근거 소스를 마지막으로 바꾼 커밋(웹이 옮길 때 볼 수 있던 판) |

정규화 이유: 저장소는 `core.autocrlf=true`이고 cpppj fixture 일부는 `eol=lf`, webpj 기대값은 작업 사본이 CRLF다. 체크아웃 방식과 무관하게 같은 값을 얻도록 텍스트만 정규화한다.

## 3. 현재 목록 (35항목)

종류별: original-x86 12, independent 7, screenshot 13, source 3. 스크린샷 13장(메인 메뉴·Credits·Demo·Campaign 6장·Options 2장·Help 메뉴·도움말 창)은 `screenShots/`의 원본 캡처이며 표에서 생략했다.

| id | 종류 | 자료 | 줄 | 단위 | 자료 커밋 | 웹 반영 | cpppj 근거(basis) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| taff-entries | independent | taff-entries.tsv | 249 | WB02-D | 3d038e51 | 3d038e51 | - (docs/formats/taff.md) |
| config | original-x86 | config-x86.tsv | 2443 | WB02-D | 0ded2f07 | 5e40032b | Config.cpp 0ded2f07, ConfigInterface.cpp 2ede4027 |
| help-links | original-x86 | help-links-x86.tsv | 92 | WB03-K | 8f85465d | 8db90513 | HelpLinkPolicy.cpp 8f85465d |
| script-command | original-x86 | script-command-x86.tsv | 263 | WB03-J | 7f1c4740 | 404c43ec | ScriptCommand.cpp 2054e074 |
| tell-parameters | original-x86 | tell-parameters-x86.tsv | 1071 | WB03-J | 5adcf532 | 404c43ec | TellParameters.cpp 5adcf532, TellSection.cpp 6dd8d803 |
| tell-tip | original-x86 | tell-tip-x86.tsv | 389 | WB03-J | b37bd01a | 404c43ec | TellTip.cpp b37bd01a |
| fort-reader | independent | fort-expected.tsv | 468 | WB03-U | 725dabf9 | 725dabf9 | Template.cpp 03f9dcdf, RiftTypeTable.cpp 700475b5 |
| territory | independent | territory-expected.tsv | 2993 | WB03-V | 2f7c2d51 | 2f7c2d51 | CanonDecoder.cpp 5bdb5bed, ChunkMap.cpp da6c61ae, Islandbuilder.cpp bfb780c4 |
| terrain-mask | independent | terrain-expected.tsv | 468 | WB03-W | 473432cd | 473432cd | TerrainBuilder.cpp 225f2322, RawTerrainContext.cpp 552656c0 |
| terrain-tiles | independent | terrain-tiles-expected.tsv | 731 | WB03-X | 8085cd03 | 310c5ba6 | RawTerrainStamp.cpp b76adf95, RawFrameChoose.cpp 5c2d0a1e, RawRegionPaint.cpp bae8b702, RawGameRandomSeed.cpp a24db9dc |
| terrain-fringe | independent | terrain-fringe-expected.tsv | 314 | WB03-Y | 310c5ba6 | 310c5ba6 | RawTerrainStamp.cpp b76adf95, RawFrameChoose.cpp 5c2d0a1e, RawTerrainContext.cpp 552656c0 |
| terrain-stamp | original-x86 | terrainstamp-x86.tsv | 1561 | WB03-Y | b76adf95 | 310c5ba6 | RawTerrainStamp.cpp b76adf95 |
| island-supports | independent | island-supports-expected.tsv | 468 | WB03-Y | 310c5ba6 | 310c5ba6 | RawIslandPostPop.cpp deaf73f8 |
| owner-hsl-colors | original-x86 | owner-hsl-colors-x86.tsv | 16 | WB03-Y | c7e9e8b3 | 310c5ba6 | OwnerColorMaps.cpp 0dbfd0c8 |
| owner-capture | original-x86 | owner-capture-x86.tsv | 499 | WB03-Y | 7f192a4a | 310c5ba6 | OwnerColorTableState.cpp 0dbfd0c8 |
| draw-sort | original-x86 | drawsort-x86.tsv | 1156 | WB03-Z | 687dd2cf | 67aae862 | DrawSort.cpp 225f2322 |
| camera-view | original-x86 | camera-view-x86.tsv | 17 | WB03-Z | e210c755 | 67aae862 | RawCameraView.cpp e210c755 |
| fort-object-load | original-x86 | fortobjectload-x86.tsv | 1219 | WB03-AB | ada85427 | 699421d7 | RawFortObjectLoad.cpp 674e3202 |
| sid-pool | original-x86 | sid-x86.tsv | 4721 | WB04-A | ffdc5ddb | 0fd343a1 | SidPool.cpp 6035f810, SidPool.h b7065914 |
| src-scrolling-widgets | source | ScrollingTextGumpWidgets.cpp | - | WB03-L | 8661b66b | db9fd26b | 같은 파일 8661b66b |
| src-type-load-order | source | TypeLoadOrder.cpp | - | WB03-T | 128f8b95 | 725dabf9 | 같은 파일 128f8b95 |
| src-territory-patterns | source | TerritoryPatterns.json(dotnetpj) | - | WB03-V | d25ad4a1 | 2f7c2d51 | - (교차 확인용, 정답 근거 아님) |

`webCommit`은 모듈의 **마지막** 변경이다. 예: Credits/Demo/Campaign 스크린샷의 `webCommit`은 WB03-AC(`48339f60`)인데, 이는 `tell-window.js`에 제품 본문 적용 한 줄이 들어갔기 때문이며 원본 대조 배치는 `product:false`로 그대로 검증한다.

## 4. 웹 반영 뒤 cpppj 근거 소스가 바뀐 곳 (재검토 후보)

`node webpj/tools/fixture-manifest.mjs` 기본 검사가 보고한 7곳이다(작성 시 HEAD `f07c4a0b`). 실패가 아니라 재검토 알림이다. 이번 단위에서는 diff 크기와 커밋 제목으로 영향을 분류만 했고 이식은 하지 않았다.

**2026-11-05 사용자 결정:** cpppj는 진행 중이므로 webpj는 단위 착수 시점의 cpppj 현재 버전을 참고해 구현하고, 그 뒤의 cpppj 변경은 **별도 재반영 과정**으로 다시 반영한다. 아래 7곳은 그 재반영 목록의 첫 대상이며, 절차와 상태 표는 [LEFT_JOBS.webpj.md 3.5절](../LEFT_JOBS.webpj.md)에서 관리한다. 이 도구의 "cpppj 변경" 보고가 재반영 대상을 찾는 기본 수단이다.

| cpppj 소스 | basis 이후 | 내용과 웹 영향 판단 |
| --- | --- | --- |
| `o/Config.cpp` | 1커밋 `2ca39d0b` | `LoadBytes`를 `AppendDecodedBytes`로 나눈 구조 변경과 `@파일` 인자 적재 훅 추가. 서명/BOM/NUL 처리 의미는 같아 보임. 웹은 `@파일` 인자 경로를 쓰지 않음 → 영향 낮음 |
| `o/ConfigInterface.cpp` | 3커밋 `8d2f81a1`·`1ba89851`·`2ca39d0b` | 설정 Load·FileSpec 검색 수명·Startup 파일명 순서. 웹은 FileSpec/설치 경로가 없고 느슨한 파일→TAFF 조회만 씀 → 미션 적재(WB05) 때 현재 미션 Tell 읽기와 함께 재검토 |
| `client/TellSection.cpp` | 1커밋 `2ca39d0b` | fallback tell 적재가 `config.LoadFile`(현재 파일 해석)로 바뀜. 일반 미션 Tell 읽기 연결의 일부 → WB05 현재 미션 Tell 연결 때 반영 검토 |
| `o/RiftTypeTable.cpp` | 1커밋 `fbc27346` | 타입 레코드에 기본 외형 프레임(+128) 보관 추가. 표시 소비자용 → WB03-M2/WB05 Draw 연결 때 `type-flags.js`/`type-table.js` 반영 검토 |
| `o/CanonDecoder.cpp` | 1커밋 `167866d9` | 현재 프레임 진단과 커서 배치 소비자 연결. 웹 영역 해석 결과와 무관한 진단/소비자 쪽 → 영향 낮음(커서 배치 구현 때 확인) |
| `o/RawTerrainContext.cpp` | 2커밋 `3eec0830`·`1fe5fae7` | 판본·군도 모드 접근자 추가. 군도 경로는 웹 미구현 → 군도 지형 이식 때 반영 |
| `o/RawIslandPostPop.cpp` | 1커밋 `0c06fa66` | 섬/표면 현재 후처리와 진단 복귀(64+/29−). 실제 Pop/PostPop 경로 → **WB04-E에서 이식**(현재 `island-supports.js`는 정적 받침 계산만) |

## 5. 사용 규칙

1. 새 검사가 원본 대조 자료를 쓰면 같은 단위에서 목록에 항목을 추가하고 `node webpj/tools/fixture-manifest.mjs --write`로 계산 칸을 채운다. 적용 범위 검사가 등록 누락을 실패로 알린다.
2. 자료 해시가 바뀌어 검사가 실패하면(다른 세션의 cpppj fixture 재생성 등), 해당 웹 검사를 다시 돌려 대조 결과를 확인하고 단위 문서에 기록한 뒤 `--write`로 갱신한다. 해시만 갱신해 실패를 숨기지 않는다.
3. 웹 모듈을 고친 단위는 `--write`로 `webCommit`/`cpppjBasis`를 갱신한다. 모듈에 커밋하지 않은 변경(새 파일 포함)이 있으면 `webCommit`은 `worktree`로, `cpppjBasis`는 현재 HEAD 기준으로 기록된다. 다음 단위에서 `--write`하면 실제 커밋 해시로 바뀌며, 기본 검사가 "갱신 필요"로 알린다(2026-11-05 WB04-B 보완).
4. 기본 검사의 cpppj 변경 보고는 다음 이식 단위 착수 때 확인한다.

## 6. 검증

- 신규 `fixture-manifest.test.mjs` 3/3 통과. 음성 대조로 목록에서 `sid-pool`·`shot-demo`·`src-type-load-order`를 빼고 `camera-view` 줄 수를 바꾸면 적용 범위 검사가 3건 누락, 해시 검사가 1건 불일치를 보고함을 확인한 뒤 원래 목록으로 되돌렸다.
- 도구 기본 검사: 35항목, 해시 불일치0, 없는 파일0.
- 전체 `npm --prefix webpj test` 293/293 통과(Node v24.21.0, 약27.98초).
- 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다. cpppj fixture·소스는 읽기만 했다.
