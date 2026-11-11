# webpj 프레임 후보 선택(RawFrameChoose)과 10차 NB23 반영 — WB04-E294 (2026-11-11, 4-B)

3.1절 4-B 의 **RawFrameChoose**(partial 45줄)를 마쳤다. 원본 `0049aa90` 의 번호 wildcard(-1) 후보 선택 `ChooseTerrainFrame` 은 이미 `assets/terrain-tiles.js` 의 `chooseTerrainFrame` + `FrameCodeTable.candidates`(WB03-X)에 있었고, 남은 일은 **기준선 뒤 cpppj 변경 `bda77381e`(20261111-169, 10차 NB23)의 3.6절 판정**과 단위 계약 시험이었다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 3.6절 판정: ② 반영 (NB23)

- 변경: `RawFrameChoose.cpp` 의 원소 분위 범위(`high <= low`) 처리 한 줄. 기준선은 `out_of_range("지역 프레임 quartile 난수 범위 오류")` 였고, 교정 뒤에는 theme -1 과 같이 **전체 후보에서 `Next(후보 수)` 로 한 번 고른다**.
- 원인(cpppj 주석): 후보가 8개 미만인 타일 코드에 theme(0..3)이 있으면 분위 범위가 비어 원문은 0 으로 나누는 난수 범위를 썼다. 스커미시의 MP 규칙 월드 생성(무작위 중앙섬/군도)에서 약 5% 로 드문 섬 생성 종료를 일으켰다.
- 판정 이유: 기준선에서 성공하던 경로(후보가 8개 이상이라 분위 범위가 있는 경우, 후보가 없어 -1 인 경우)는 바뀌지 않고 **기준선에서 예외로 끝나던 경로만** 결과가 생기므로 이전 대조 결과를 깰 수 없다. 웹의 12차(스커미시)가 무작위 섬을 만들 때 필수이고, 실제 Fort 465개의 칸은 이 경로에 닿지 않는다. 그래서 순서 10까지 미루지 않고 이 단위에서 반영한다(대응표 base 를 `bda77381` 로 올림).
- 반영: `chooseTerrainFrame` 의 `themeRange` 호출을 분위 계산으로 풀어 `high <= low` 이면 `candidates[random.next(candidates.length)]`. 타입 모델의 `themeRange`(빈 범위 오류, 후보 수 0~7 표)는 그대로 둔다(미리보기 분석 도구의 문서화된 모델).
- 같이 고친 기존 시험 2건(`tests/terrain-tiles.test.mjs`): `chooseTerrainFrame: 난수 소비와 후보 선택`(빈 범위 오류 → 후보 수 한계로 한 번 난수), `buildTerrainFrames: 후보가 4분할되지 않는 칸은 …`(첫 도장의 (J,J) 통로 칸이 오류 대신 그 후보 안에서 고름).

## 2. 검사 (`tests/frame-choose.test.mjs` 5건 + 기존 terrain-tiles/terrain-fringe)

- cpppj `KnownBug_NB23_SparseTileCodeFallsBackToAllCandidates` 와 같은 입력(`B/A` 5개·`C/A` 16개)으로 모든 원소·40회 반복의 범위를 대조.
- 난수 대역으로 소비 횟수와 한계를 고정: 후보 1..7개는 모든 원소에서 `next(후보 수)` 한 번, 후보 8개는 원소 `t` 가 `next(1)`·`2t + 1`번째, theme -1 은 후보 수 한계.
- 후보 검색 단계(정확히 → 정규화 → (방향,'A') → 방향만)와 후보 없음(-1, 난수 소비 없음), 후보 최대 256개(분위도 256개 기준), 원소 범위의 난수 초과는 후보 범위 오류.
- 원본 Fort 465개로 후보 단계/분위를 대조하는 기존 시험은 그대로 통과한다(기준선 경로 불변 확인).
- 변형 확인 17개 중 처음에 4개가 살아남았다. 정규화/(방향,'A') 단계는 시험을 보강해 잡았고(`('A','F')`→`('B','F')` 한 개 후보, 방향만 후보가 앞서는 표), 남은 둘은 **동등 변형**이다: `index < 0` 하한은 `low >= 1` 이라 닿지 않는 방어, 정확히 일치 단계를 `> 1` 로 바꿔도 같은 후보가 정규화 단계에서 다시 선택되어 고르는 결과가 같다(단계 라벨만 `terrainFrameCandidates` 시험이 본다).

## 3. 남은 경계

- C++ 의 `FrameCode` 구조체 인자는 웹에서 방향·변형 글자 두 인자로 대신한다(번호·플래그는 선택에 쓰이지 않음). 후속 RawTerrainStamp 는 이 함수를 `theme` 0(패치)과 -1(fringe) 로 부른다.
- 3.1절 4-B 는 25/31 이다. 다음은 **RawTerrainStamp**(partial 241줄)이다.
