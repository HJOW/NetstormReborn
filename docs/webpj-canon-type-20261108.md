# webpj 타입별 Canon 디코더(CanonTypeDecoder)·패턴 없는 한 칸 경로 이식 — WB04-E136

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른여섯 번째 부분이다. 건설 정리·배치(`RawConstructionClear`/`Place`)와 사제 배치가 공통으로 쓰는 Canon 디코더의 타입 선택(원본 생성자 `00425c20`)을 옮긴 `o/CanonTypeDecoder`·`o/CanonTypePatterns.inc`와, 기존 웹 `CanonDecoder`(패턴 경로만 있던 `www/assets/territory.js`)에 없던 **패턴 없는 한 칸 경로**·다리 패턴 추첨(`004257c0`)이다. cpppj 기준선 `1f00a25a`, 기준선 뒤 세 파일(`CanonDecoder.cpp`·`CanonTypeDecoder.cpp`·`CanonTypePatterns.inc`) 모두 변경 없음.

## 1. 옮긴 것

`tools/make-canon-type-patterns.py` → `www/assets/canon-type-patterns.js`(생성, 직접 고치지 않음; `--check` 가능)

- 다리 패턴 26개(`0x52f998`, 첫 값 = 추첨 가중치, 합 287)·섬 2개(`0x531410`)·받침 1개(`0x5314a0`)·공유 전투 1개(`0x5314e8`)를 cpppj `CanonDecoderTables.inc`·`CanonTypePatterns.inc` 에서 읽어, 영역 패턴(`territory-patterns.js`)과 같은 모양 `{ first, width, height, sides, variations, labels }` 로 쓴다. 가중치 합이 287 이 아니면 생성이 실패한다. CD 다리 표(합 306)는 범위 밖이라 읽지 않는다.

`www/assets/territory.js`

- `CanonDecoder` 생성자에 패턴 없는 한 칸 경로를 추가했다(`pattern === null` + 마지막 인자 `{ defaultFrame, argument, explicitFrame }`): 회전 0·폭 높이 1, 명시 프레임이 아니면 기본 프레임을, 명시 프레임이면 argument 를 쓴다. 명시 프레임이 프레임 표 밖이면 원문 진단 `0 <= frame && frame < ::getType(type)->numFrames`(Canondecoder.cpp 0x305)를 부르고 복귀하면 그 값으로 계속한다(물리 표 밖 `side` 조회는 거부). 좌표가 유한하지 않으면 거부한다. 금 간 프레임 진단(0x325)은 패턴 경로와 같은 위치에서 같은 `checking` 으로 읽는다. 기존 패턴 경로의 동작은 그대로다.

`www/core/raw-canon-type.js`

- `decodeCanonType(frameCodes, defaultFrame, patternTypes, query, diagnostics)` (00425c20): 타입 전역 8개(puzzlePiece·bridge·island·noIsland·thunderCannon·rainCannon·windArcher·windBlocker)를 앞 네 개부터 차례로 비교해 영역 68·다리 26·섬 2·받침 1 패턴을, 뒤 네 개 중 하나와 같으면 공유 전투 패턴을 고르고 argument 를 패턴 번호로 쓴다(범위 밖 거부). 어느 것도 아니면 한 칸 경로이며 패턴 타입에서는 `explicitFrame` 을 무시한다(타입 선택이 앞선다). 반환은 이미 첫 유효 칸으로 진행한 `CanonDecoder` 이다.
- `canonSpecialPatterns(group)` — 섬(0)·받침(1)·공유 전투(2) 표. 다른 그룹은 거부.
- `selectBridgePattern(random)` (004257c0) — 가중치 합 287 에 대한 signed 나머지를 누적 가중치와 `<=` 로 비교한다(나머지 0·음수는 가중치 0 인 첫 항목).

## 2. 검증

`tests/canon-type.test.mjs`(6건):

1. `canontype-x86.tsv` 의 `originals` 2079행(`originalCD` 924·`original1037` 924행은 다른 판본이라 제외) — 타입 전역 9종×인자×방향(홀수·signed −1 포함)×프레임 표 3종(정상·역순·빈 표)×전역 별칭 입력에서 생성자 직후 발자국 범위, 남은 칸 전체 순서(프레임·x/y 비트·번호·방향 글자), 끝 상태(프레임·유효 여부·마지막 좌표 비트·번호·끝 범위)가 모두 같다.
2. 생성 표 검증: `.inc` 선언을 시험이 직접 읽어 생성된 JS 표와 첫 값·폭·높이·글자 세 줄을 대조하고 다리 가중치 합 287 을 확인한다(Python 이 없어도 시험이 표 불일치를 잡는다).
3. 특수 패턴 그룹 크기(2/1/1)와 첫 글자, 범위 밖 번호·잘못된 회전·비유한 좌표·잘못된 전역 개수 거부, 빈 프레임 표에서 유효 칸 없음, 패턴 타입의 `explicitFrame` 무시(섬 패턴 1번 → 프레임 168), 전역 선행 비교(별칭).
4. 다리 추첨: 가중치 0·30·10·4·50 의 누적 경계(0→0, 30→1, 31→2, 44→3, 45→4), 287 배수·음수·`-0x80000000` 이 첫 항목, 나머지 최댓값 286 이 25번.
5. `canon-decoder-checking-x86.tsv` 156행 전부 — 한 칸 경로의 명시/기본 프레임·checking·금 간 프레임 코드 비트·진단 복귀/중단·진단이 checking 을 끄는 경우에서 진단 사건(식·파일·줄 바이트)·결과·첫/마지막 공개 상태 4필드·마지막 checking 이 같다.
6. 같은 owner 의 현재 checking 을 다음 진행에서 읽고, 교체한 진단 콜백이 불리며, 진단 장치가 없으면 성공을 합성하지 않고 거부하고, 명시 프레임 진단이 복귀해도 물리 표 밖 방향 조회는 거부한다.

최초 실패와 교정: 새 경로와 모듈은 2079행·156행 fixture 를 첫 실행에 통과했다. 생성 도구는 `python3`(Windows 스토어 별칭) 대신 `python` 으로 실행했다.

## 3. 경계와 남은 것

- 10.72(CD) 다리 표·홀수 방향 거부·CD 명시 프레임 거부는 범위 밖이다. cpppj 시험의 CD/10.37 재생은 옮기지 않았다.
- `RawConstructionClear`/`RawConstructionPlace`·사제 배치·커서 미리보기 등 소비자는 이 디코더 위에서 이어 옮긴다. 다리 건설기가 쓰는 추첨 난수의 공급(`selectBridgePattern` 호출)은 해당 단위에서 잇는다.

## 4. 다음 시작점

`RawConstructionClear`(Canon 디코더·`RawSquidFinder` 위의 로컬 예측 조각 정리)·`RawPriestPlacementGeometry`→`RawConstructionPlace`, 이어서 `Confirm`·`Request`·`Local`·`Command`·`Data`·`Builder`, `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
