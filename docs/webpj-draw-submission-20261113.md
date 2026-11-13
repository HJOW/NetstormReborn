# webpj DrawSubmission·ConstructionVisualRenderer (WB04-E382, 2026-11-13, 5-A 열한 번째 단위)

5-A 의 렌더러 프레임 본체 앞단 두 client 단위 — **DrawSubmission**(cpppj `client/DrawSubmission.cpp/.h`, 기준선 143줄)과 선행 관계로 묶여 있던 **ConstructionVisualRenderer**(`client/ConstructionVisualRenderer.cpp/.h`, 57줄) — 를 `render/draw-submission.js`·`render/construction-visual-renderer.js` 로 옮겼다. 두 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 Renderer(10.78 `004994b0`)가 유효 dirty 사각형 안에서 부르는 표시 목록 병합과 kind 0~4 분배의 순수 로직이며, 좌표 환산·dirty 순회·배경·장치 잠금은 아직 없는 `Renderer` 본체(다음 항목)의 책임이다.

## 1. 규칙

- **항목 형식:** 16바이트(x float32 · y float32 · token DWORD · depth int16 · kind BYTE · tail BYTE)이고 목록은 `Uint8Array(16×n)`이다 — E131 `RawRenderCollection` 이 쓰는 같은 형식이다. 묶음 저장소의 개수는 원본 포인터에 해당하는 `{ value }` 상자라 그리기 중 바뀐 값을 매번 다시 읽는다.
- **`collectDrawEntries(request, work, hooks)`:** 작업 용량을 5,000 항목 이하로 제한하고 UI → 건설 표시 → 묶음 → 일반 자산/효과 → 미리보기 순으로 병합한다. 수집기는 남은 칸(부분 배열)과 한도만 받고 실제 기록 수를 돌려주며, 음수·남은 용량 초과·정수가 아닌 반환은 예외다. 비어 있는 수집기는 건너뛰고 연결된 수집기는 남은 용량 0 에서도 원본 순서로 부른다. 요청 `dirtyFlags` 의 비트 2 는 UI 만 수집한다. 묶음은 첫 항목의 x·y·depth 와 헤더 토큰으로 새 항목을 만들고 kind 를 1 로 두며 마지막 BYTE(tail)는 쓰지 않는다(보호 바이트 유지). 빈 묶음 저장소·개수 상자 누락·남은 용량 초과는 예외다.
- **`dispatchDrawEntries(entries, state, hooks)`:** kind 0 은 sprite(항목 전체 보기), 1 은 묶음 내부를 매 차례 **현재** 개수로 sprite 에(`hideGroups` 가 켜져 있으면 생략), 2·3 은 ui·construction(토큰), 4 는 preview(항목 전체), 알 수 없는 kind 는 그리기만 건너뛴다. `debugAfterEach` 이면 상위 항목마다 한 번(알 수 없는 kind 포함, 묶음 내부 항목마다가 아님) `afterEach` 를 부른다. 두 플래그는 항목마다 현재 값을 읽는다. 쓰는 종류의 필수 콜백이나 묶음 저장소가 없으면 `콜백 누락`/`개수 저장소 누락` 으로 거부한다(쓰지 않는 종류는 요구하지 않음).
- **`sortRenderEntries(entries)`:** 16바이트 목록을 `render/draw-sort.js`(원본 비교 00497900 + CRT qsort/shortsort 복원)로 제자리 정렬한다. 항목 전체 16바이트가 함께 움직여 kind·tail 이 보존되고, 유한 float32 좌표와 signed WORD 깊이가 아니면 교환 전에 거부한다.
- **`drawVisibleConstruction(system, view, work, sort = true)`:** 월드 영역의 가시 건설 표시를 수집·(선택) 정렬·제출하고 수집 수를 돌려준다(`sort=false` 는 정렬 전역이 꺼진 상태, 수집 순서 보존). **`drawVisibleScene(system, request, work, collection, state, dispatch, sort = true)`:** 실제 건설 수집/토큰 Draw 를 공통 목록에 연결해 외부 UI/묶음/자산/미리보기 수집기와 함께 정렬·분배한다. `system` 은 `ConstructionVisualSystem`(안의 공통 표시 저장소를 씀) 또는 `RenderProcessSystem`(건설/불꽃 꼬리를 포함한 원본 단일 목록) — C++ 오버로드 둘이며 `instanceof` 로 가른다. 호출자의 훅 묶음은 바뀌지 않고 `construction` 만 시스템으로 교체한 사본을 쓴다.

## 2. 웹 범위

- 10.78 전용: 판본 인자(정렬의 CD CRT, 묶음 CD 몸체)는 옮기지 않았다. 관측 fixture 의 `originalCD`·`original1037` 행은 읽지 않는다(각 322행).
- 소비자 연결은 `Renderer` 본체(다음 5-A 항목: `RawRendererPrelude`·`Renderer`·`ClientRendererFrame`·`DrawSubmission` 호출자)에서 한다. 이 단위는 앱 수명에 연결하지 않았다(`PORT_MAP` session 0).
- 장면 장치 `World`(E194 의 `construction-visual-system.test.mjs` 안)를 `tests/construction-visual-support.mjs` 로 분리해 두 시험이 공유한다(E194 시험 10건은 그대로 통과).

## 3. 검사

- **원본 독립 관측(10.78):** `drawsubmission-x86.tsv` originals **322행**(상위 Renderer 의 실제 병합/CRT 정렬/분배 — 네 수집기의 완성 입력 바이트와 묶음 저장소, 옵션 여섯, 수집기 호출 기록·병합 후/정렬 후 바이트·분배 기록·작업 배열 앞뒤 0xa5 보호 바이트)이 **첫 실행에서 전부 일치**했다. `tests/draw-submission.test.mjs` 8건.
- **cpppj 시험 이식:** `DrawSubmission_RejectsInvalidCountsAndGroupOverflow`(음수·초과·비정수 반환, 묶음 초과·빈 저장소·개수 상자 누락 거부)·`_DispatchContractsAndUnknownKinds`(쓰는 종류만 필수·알 수 없는 kind 의 debug 관찰·개수 초과)·`_CapacityAndEmptyBuffer`(5,000 초과 버퍼, 용량 0 에서도 수집기 호출). E194 가 미룬 건설 표시 쪽 5건(`tests/construction-visual-renderer.test.mjs` 8건): 가시 영역 제출(깊이 7/0/−1 정렬·카메라 Draw·용량 2 제한·정렬 끔)·작은 동률과 NaN 좌표 거부·9항목 완전 동률의 CRT 순서(`drawsort-x86.tsv` originals 한 행)·UI/묶음/자산/미리보기와의 혼합 병합(`U99,S77,S78,C2,S103,C1,P104`)·분배 중 건설 토큰 삭제.
- **웹 계약:** 묶음 헤더의 tail·보호 바이트 미기록, dirty 비트 조합(4·5 만 UI 전용), 정렬이 kind·tail 보존과 반환값, 항목 보기가 같은 저장소를 가리킴, 두 대상 종류가 같은 결과, 훅 묶음 불변, 32비트 signed 최대 작업 개수 경계(허용/거부).

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 44개와 손으로 쓴 구조 변형 66개를 적용했다(draw-submission 84, construction-visual-renderer 26). 1차에서 생존한 중요한 것 — 묶음이 남은 용량을 **정확히** 채우는 경우의 `>`/`>=`, 필수 콜백·개수 상자 누락의 **오류 문구**(TypeError 클래스만 보면 "is not a function" 에 가려짐), 정렬 반환값, 잘못된 대상의 오류 문구, 32비트 최대 개수 경계 — 은 시험을 더해 모두 잡았다.
- 남은 동등 변형: ① 묶음 복사의 위치 바이트 수 8→9 와 depth 복사 2→3 (뒤에서 token·kind 를 덮어씀), ② `append` 의 `rectangle === undefined` 추가 방어, ③ `drawVisibleConstruction` 요청 객체의 쓰이지 않는 칸(`screen` 네 값·`dirtyFlags` 0→1·`groupRowLimit`·`unclipped`) — 수집기를 하나(`construction`)만 연결하므로 관찰할 수 없다(cpppj 도 같은 값을 넘기지만 관측 불가), ④ `system.rendering ?? system` (잘못된 대상은 어느 쪽이든 TypeError 로 거부).
- 다음 5-A 항목: 렌더러 프레임 본체(`RawRendererPrelude`·`RawRendererClouds`·`Renderer`·`RendererDiagnostics`·`ClientRendererFrame`·`RawCameraPresent`·`ScreenPresentation`·`Screen`) — `DrawSubmission` 호출자가 여기 있다.
