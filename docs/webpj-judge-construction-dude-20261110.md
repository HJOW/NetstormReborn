# webpj 3-E 판정-건설·운반·Dude — WB04-E262 (2026-11-10)

3.1절 3-E의 네 번째 항목 **판정-건설·운반·Dude** 15단위를 판정했다. 방법과 ①/②/③ 기준은 [판정-공통층 문서](webpj-judge-common-20261110.md)와 같다: 이식 원본은 기준선 `1f00a25a` 판이고, `git diff --ignore-cr-at-eol 1f00a25a HEAD -- <단위 파일>`(15단위 `.cpp/.h` 약 480줄 전부; 일부 `.h` 는 줄바꿈 차이로 diff 가 부풀어 `--ignore-cr-at-eol` 로 읽음)과 새 API 의 소비자를 읽어 단독 미션의 웹 결과가 바뀌는지만 판정했다. 판정 기준 커밋은 착수 때 `HEAD` `f77c378c`(20261110-116 WB)이며 행 base 는 단위 파일을 마지막으로 바꾼 커밋이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 판정 표

| 단위 | 판정 base(기준선 뒤 커밋) | 기준선 뒤 변경 | 판정 | 처리·시점 |
| --- | --- | --- | --- | --- |
| RawBuildingSpace | `be567d46` (1) | `BindCurrentIslandType`: 현재 isle TYPE 를 `HasIsland` 비교 위치에서 읽음 | ③ | **4-D**. 독립 typed 는 `state.islandType` 으로 동일 |
| RawCanonPlacementCollision | `cc562ccf` (2) | `sharedIgnoredGenus`(005325b4)·`sharedServer`(00540bc0)를 후보별로 읽는 getter | ③ | **4-D**·6(건설 충돌). 독립 typed 입력은 기존 값 |
| RawConstructionClear | `5f4b9d30` (1) | `sharedBridgeType`·`patternTypesRead`(후보 프레임 비교·decoder 생성 위치에서 현재 TYPE) | ③ | **4-D** |
| RawConstructionConfirm | `cc562ccf` (1) | 실제 wire 경로 `ActualConfirm`(444590: 512BYTE 초기 헤더 lazy wire→Create→WORD store→20개 진단 복귀→mutable 통지 송신+직접 수신→기록기 첫 SID)·`RecorderEnabled` 공유 | ③ | **4-C**(패킷 writer `RawConstructionPacket`·`RawFormPacket`)·**6**(건설). typed `Confirm` 은 `broadcast`/`handle` 미연결 때만 새 오류이며 결과 동일 |
| RawConstructionPlace | `ed29febb` (2) | 실제 wire 배치 `ActualPlace`(442c80: 비용→조각→후행 진단, 현재 SID reader)·`BindCurrentPlayers`·shared getters(로컬 플레이어·PlayerKnown·다리/noIsland TYPE·다리 수명·패턴 TYPE) | ③ | **4-C**·**6**. 독립 typed 입력은 기존 값 |
| RawConstructionProcess | `5f4b9d30` (3) | `quickBuild`/`quickVortex` 가 `RawCheckingFlag`(현재 55a49c/98 DWORD 비영 뷰)로 바뀜·`currentProcessType`/`sharedPushGenus` | ① + ③ | 웹 truthiness 판정과 같다(①). 005410e8/005325b4 대여는 **4-D** |
| RawConstructionRequest | `5f4b9d30` (1) | `sharedEditor`/`LocalPlayer`/`DisplaceGenus`/`IslandType`/`NoIslandType`·`patternTypesRead` 를 소비 위치마다 읽음, island TYPE 범위 밖은 명시 오류 | ③ | **4-D**. typed 결과 동일 |
| RawCarrierFalling | `d3767242` (1) | `NeedsEscapeIn`: Player/Session 없이 현재 장기 표에서 4225b0 실행(미리보기 타이머 소비자) | ③ | **4-D**. typed `NeedsEscape` 는 같은 몸체를 호출해 동일 |
| RawCarrierPreDestroy | `e3996287` (1) | `EnableNativeBody`(`Validate` 선검사 생략) | ③ | **4-D**. 독립 typed 계약 유지 |
| RawCarrierRegular | `d3767242` (2) | 현재 TYPE 의 기본 프레임(+118)·시전 시간(+1b8) getter 를 받는 Native 생성자(`details` 사본 없음) | ③ | **4-D**. typed 는 `details` 길이=TYPE 길이를 강제해 범위 검사 결과 동일 |
| RawCarrierTransfer | `079f170e` (1) | `sharedEmptyType`(00541344)을 donor 비교마다 읽는 getter | ③ | **4-D**. 독립 typed 는 기존 187 |
| RawDudeBehavior | `b1bf342d` (2) | 목록 제거를 `RemoveStateSidList` 로 공유·`PostDestroyNative`·`state.native` | ① + ③ | typed 경로는 `items()` 선검사가 count>용량을 먼저 거부하므로 웹 `removeAll`(미리검사+압축)과 **결과 동일(웹 변경 없음)**. Native 본체는 **4-D** |
| RawDudeNotice | `b1bf342d` (1) | `sharedPathType`(005410d0)·`NearestResidenceCurrent`(Native 48fd30: 현재 signed count/실제 목록) | ③ | **4-D**. typed `NearestResidence` 불변 |
| RawDudeRegular | `b1bf342d` (1) | `state.native` 분기(TYPE70 선검사·사전 목록 조회 생략, 난수 getter, 공급원 순환 IDIV 범위, WORD 쓰기를 `setWord` 훅으로)·`sharedPollEvent`/`sharedPathType` | ③ | **4-D**. typed 경로는 `Stage` 의 TYPE 범위 검사가 한 줄 늘었을 뿐 결과 동일 |
| RawDudeTask | `b1bf342d` (1) | Native `Set`(현재 FALL 진단→Regular 조회/생성→`ResetTask`→`setWord`) | ③ | **4-D**. typed 경로 불변 |

합계: ② 재반영 0·① 단독 0(RawConstructionProcess·RawDudeBehavior 는 ①+③)·③ 15 — 15단위가 모두 판정됐다. 이 묶음도 웹 코드와 시험을 바꾸지 않는다(문서·대응표만).

## 2. 확인한 점

- `RawDudeBehavior::PostDestroy` 의 typed 경로가 E259 의 `RemoveStateSidList` 를 쓰도록 바뀌었지만, 같은 함수에 남은 `population.Items()` 선검사가 count>용량을 먼저 거부하고 정상 count 의 압축은 같아서(원본 fixture 통과) 웹의 `squid-postpop-state.js` `removeAll` 과 결과가 같다. 그래서 E260 의 `RawPriestPreDestroy` 처럼 코드를 바꾸지 않았다. 음수 count 는 `PostDestroyNative`(Native)에서만 읽기 없이 지나가며, 그 분기는 4-D 몫이다.
- 건설 실제 wire(`ActualConfirm`/`ActualPlace`)는 단독 미션에서도 실제 건설이 지나는 길이지만, 웹은 패킷 writer(`RawConstructionPacket`·`RawFormPacket` — 4-C 목록)가 먼저 있어야 같은 wire 를 만들 수 있어 4-C 에 걸었다. 독립 typed `Confirm`/`Place` 의 결과는 변하지 않는다.

## 3. 대응표·문서 반영

- `PORT_MAP.tsv` 15행의 base 와 메모를 갱신했고 `node webpj/tools/port-map.mjs` 오류 0, 15단위의 `cpppj 변경` 보고가 사라진다.
- LEFT_JOBS.webpj.md 3.6절 표에 ③ 묶음 2행(4-D 건설/운반/Dude Native 공유 getter·본체, 4-C·6 건설 실제 wire 경로)을 더했다. [대응표 문서](webpj-port-map-20261106.md) 9절 표에 이 묶음을 더했다.

## 4. 남은 경계와 다음 시작점

- 다음은 3-E 다섯 번째 항목 **판정-전투**(18단위)이다.
