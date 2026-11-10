# webpj 3-E 판정-섬·다리·자원 — WB04-E261 (2026-11-10)

3.1절 3-E의 세 번째 항목 **판정-섬·다리·자원** 6단위를 판정했다. 방법과 ①/②/③ 기준은 [판정-공통층 문서](webpj-judge-common-20261110.md)와 같다: 이식 원본은 기준선 `1f00a25a` 판이고, `git diff 1f00a25a HEAD -- <단위 파일>`(6단위 `.cpp/.h` 약 200줄 전부)과 새 API 의 소비자(`grep` 로 `cpppj/src`·`cpppj/tests` 전체)를 읽어 단독 미션의 웹 결과가 바뀌는지만 판정했다. 판정 기준 커밋은 착수 때 `HEAD` `627f394c`(20261110-115 WB)이며 행 base 는 단위 파일을 마지막으로 바꾼 커밋이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 판정 표

| 단위 | 판정 base(기준선 뒤 커밋) | 기준선 뒤 변경 | 판정 | 처리·시점 |
| --- | --- | --- | --- | --- |
| RawBridgeEvents | `a927575d` (1) | `BindCurrentState`: 권한·디버그 유지·다리 TYPE 의 실제 전역 주소를 소비 위치마다 읽음 | ③ | **4-D**(다리 Regular 의 장기 전역). 독립 typed 는 기존 state 값을 읽어 결과 동일 |
| RawIslandPostPop | `be567d46` (1) | `sharedLoadingDepth`: Client Template/Core 의 005c89b8 깊이를 앞선 효과 뒤 다음 관문에서 다시 읽음 | ③ | **4-B**(Fort/섬 적재 깊이 공유). 독립 3인자 계약은 기존 `loadingDepth` 로 동일. 웹 `raw-island-post-pop` 에 공유 깊이 연결은 아직 없다 |
| RawIslandRuntime | `f0d065dc` (3) | Pool/Player/효과 없이 쓰는 `RawIslandExists`/`Owner`/`Bounds`(소비자 `ClientConstructionGlobals`·`ClientConstructionRandomPlacement`, cpppj `IslandRuntimeTests`)·`BindCurrentPlayers`·`assertionAt`(원문 식/행 진단) | ③ | **4-D**(건설 무작위 배치의 NoWorld 섬 조회). 클래스 메서드는 결과 동일(부재 진단 뒤 재조회 순서·오류 범위 불변, `FirstRegion` 의 owner>8 거부는 물리 범위 밖 호스트 오류만 구체화) |
| RawGeyserLifetime | `241b013e` (1) | `EnableNativeBody`·`RemoveNative`: 현재 signed count/물리 DWORD 를 반복마다 읽는 목록 제거, 물리 슬롯 직접 쓰기 | ③ | **4-D**. typed `Remove`(unsigned count 반복) 불변 |
| RawNuggetRegular | `ed20738b` (1) | `hooks.aFrameCount`: 현재 TYPE 의 A 프레임 수를 선택 위치에서 읽는 getter | ③ | **4-D**(현재 TYPE 표). 독립 typed 188칸 span 은 같은 소비 지점으로 결과 동일 |
| RawNuggetReplenish | `e3996287` (1) | 모드 관문 뒤에만 장기 Bounds 를 조회하는 getter 생성자 | ③ | **4-D**. 독립 typed 생성자는 같은 Bounds 를 감싸 결과 동일 |

합계: ② 재반영 0·① 0(모두 Native 연결이면서 typed 결과 동일)·③ 6. 6단위가 모두 판정됐다. 이 묶음은 웹 코드와 시험을 바꾸지 않는다(문서·대응표만).

## 2. 확인한 점

- 6단위 모두 기준선 뒤 변경이 **새 선택 경로**(실제 Client 가 소유하는 전역·조회기·getter 를 소비 위치에서 읽음)이며, 독립 typed 입력의 호출 순서·오류 시점·반환 비트는 같다. 단독 미션의 결과가 바뀌는 지점은 4-D 에서 장기 owner 를 실제로 묶을 때 생긴다.
- `RawIslandRuntime` 의 세 자유 함수는 World 없는 호출자가 쓴다. 소비자(건설 무작위 배치)는 6단계 건설 연결에 속하지만, 같은 3204바이트 섬 저장소를 4-B 의 Fort/섬 적재가 채우므로 4-B 에서 저장소를 만들 때 함께 옮기고 cpppj `IslandRuntimeTests` 로 대조한다.
- `RawGeyserLifetime::RemoveNative` 와 E259 의 `removeStateSidList` 는 다른 목록 몸체다: 앞의 것은 현재 count/포인터를 반복마다 다시 읽는 간헐천 목록(`SquidPostPopList` 현재 binding)이고 뒤의 것은 값 목록 압축이다.

## 3. 대응표·문서 반영

- `PORT_MAP.tsv` 6행의 base 와 메모를 갱신했고 `node webpj/tools/port-map.mjs` 오류 0, 6단위의 `cpppj 변경` 보고가 사라진다.
- LEFT_JOBS.webpj.md 3.6절 표에 ③ 묶음 2행(4-D 다리/간헐천/너겟/섬 Native 전역·조회기, 4-B 섬 후처리 깊이 공유)을 더했다. [대응표 문서](webpj-port-map-20261106.md) 9절 표에 이 묶음을 더했다.

## 4. 남은 경계와 다음 시작점

- 다음은 3-E 네 번째 항목 **판정-건설·운반·Dude**(15단위)이다. `RawDudeBehavior` 는 E259 의 `removeStateSidList` 를 소비하는지 확인한다(cpppj 거주 구역 Dude 삭제의 `RemoveStateSidList` 호출, c9140743).
