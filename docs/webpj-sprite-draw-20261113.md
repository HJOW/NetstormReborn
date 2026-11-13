# webpj SpriteDraw — 일반 Squid 객체 Draw (WB04-E388, 2026-11-13, 5-A 열일곱 번째 단위)

5-A 타입별 Draw 중 client **SpriteDraw**(cpppj `client/SpriteDraw.cpp/.h`, 기준선 468줄)를 `render/sprite-draw.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 일반 Squid 객체 Draw(10.78 `00498220`)로, 공통 표시 목록의 kind 0 항목과 kind 1 묶음 원소가 모두 같은 Draw 를 쓴다(`bindSpriteDraw`). 기존 `world-sprites.js`(월드 그림 일부, E-WB03)와 별개의 raw 풀 기반 Draw 이며 PORT_MAP 상태는 `partial → port` 이다.

## 1. 규칙

- **`SpriteDraw.draw(entry)`:** 항목 토큰(`tokenBase + SID × 50`)을 같은 raw 풀 SID 로 되돌리고(0·stride 배수 아님·용량 밖·TYPE 표 밖은 거부), 그림이 로드되지 않은 타입(`shape.loaded` 거짓, 그림자 alias 가 바꾼 타입 포함)이면 아무것도 하지 않는다. raw 의 부호 있는 화면 WORD(+22/+24)와 월드 투영(`x×16+0.5`, `y×11+0.5` x87 절삭)이 다르면 원본 assert 에 해당하는 계약 오류를 낸다. 화면 기준점은 화면 위치에서 TYPE 기준점을 뺀 값. **가상 Draw 가 슬롯을 바꿀 수 있어 raw 는 풀의 살아 있는 보기(`pool.physical`)로 다시 읽는다.**
  - **그림자 레이어(tail 비트 4):** abstract(extra 0x10)면 끝. 패치판은 진단 전역(공유면 정확히 0, 독립이면 alias 고정 안 함)일 때 그림자 alias 타입을 현재 타입으로 기록한다. 프레임이 논리 프레임 수 이상이면 한 번 빼고, 특수 모드이거나 죽은 객체(extra 1)면 끝, 아니면 `프레임 + 논리 프레임 수` 를 그림자 모드로 낸다.
  - **본체:** extra 비트 2 없음 — 죽음→죽음 그림, abstract→로컬 소유자일 때 abstract 그림, 특수 모드이고 필터(`filteredFlags & flags2`·넓은 소유자/필터 제외 타입)에 걸리면 abstract 그림, 아니면 일반 그림(소유자 변환표가 있고 소유자가 0 이 아니면 소유자 색 문맥 뒤 소유자 모드, 아니면 원색). extra 비트 2 있음 — 가상 프레임 조회(−1 이면 기본 프레임) 뒤 현재 필터로 같은 갈래를 고르고 가상 Draw 를 호출한다.
  - **abstract 가 아니면:** 표식(배너 옵션 && TYPE flags1 0x400 && 표식 표시 && 소유자 && 0단계 표면 SID 의 TYPE 이 제외 타입이 아닐 때 `소유자 팔레트 − 1` 프레임)과 상태 표시(extra 0x20, 발자국 절반만큼 이동).
  - **선택(extra 비트 4, 선택이 꺼져 있지 않을 때):** 선택 헤더 두 겹 선(소유자 팔레트→선택 색)과 막대들 — HP(`flags2 0x400000` 이면 extra 비트 0, 아니면 flags1 0x10)·건설(그림자 alias 타입이고 건설 중)·자원(`용량×10`)·타이머 셋·지속(실제 Kernel 순서의 상태 프로세스, 슬롯 0 제외, 첫 SID 일치에서 멈춤, flag 1 이 2 보다 우선)·비용. HP 는 E377 `readSpriteHitPointBar` 를 쓴다. 막대는 배경 fill + 본체 fill, 행마다 3픽셀씩 위로 쌓는다.
- **`SpritePixelTarget`:** 규칙 조회/가상 Draw 훅은 보존하고 픽셀 출력만 연결 — 원색은 그대로, 소유자는 소스 색 치환(현재 색 표), 죽음·그림자·밝게(160%)·abstract 는 고정 대상 변환표(translated 커널), `scaled` 요청은 회전 없는 VFX 배율 출력(E378). 선택 선은 양 끝 포함 직교 선분(대각선 거부), fill 은 끝 좌표 제외, 빈/역전 사각형은 아무것도 바꾸지 않는다.
- **`makeSpriteAssetViews`·`makeSpriteAssetShapes`·`makeSpriteSurfaceLookup`:** 실제 자산의 SHP 추가 헤더/프레임 수와 `.type` 의 기본 프레임·casttime·praytime·effecttime(전체 타입 번호 188 유지), 그림자 alias 의 SHP 블록, 표식용 +0.9999 float 상수 CRT 절삭의 0단계 표면 SID 조회(경계 밖 0).

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 10.78 전용(CD `00455f60` 경로의 BYTE 프레임/WORD HP·독립 체력 막대는 옮기지 않음). 공유 전역(진단 005318ec·배너 5319c0·소유자 표 557940)은 `{ value }` 상자/배열이며 독립 입력의 지역 초기값(`shadowAliasPinned`·`markerEnabled`)은 공유가 없을 때만 쓴다.
- `SpriteDraw_ObjectCollectionPixels`(실제 raw 해시 수집→정렬→분배→픽셀)는 `ObjectCollection`(client, todo)과 함께 그 단위에서 건다.

## 3. 검사 (`tests/sprite-draw.test.mjs` 21건)

- **원본 독립 관측(10.78):** `spritedraw-x86.tsv` originals **463행**(입력 50값 → VFX/가상 처리기/게임 조회만 경계로 원본이 낸 호출열 `M`·`S`·`L`·`R`·`F`·`V`, 선택된 SID, 그림자 alias 쓰기, raw 보존)이 **첫 실행에서 전부 일치**했다. original1037·originalCD 행(각 463)은 읽지 않는다.
- cpppj 시험 이식 10건: `CommonGroupDispatch`·`PixelModesTransparencyClip`·`PixelLinesAndBars`·`ContractsAndEarlyReturns`·`CurrentRawAfterVirtualDraw`·`SurfaceLookupBoundaries`·`ShadowAliasCurrentShape`·`CurrentFilterAfterVirtualFrame`·`SharedDiagnosticControlsCurrentShadowAlias`·`SharedOwnerTablesAndBannerAfterVirtualFrame`.
- 웹 계약(변형 확인에서 발견한 공백을 독립 입력으로 보강): 상태 기본값·모드 번호·생성 경계, 토큰/SID/타입/alias 오류 문구, x·y 따로 어긋나는 월드/화면 불일치와 카메라 보정, 그림자 레이어 프레임 보정과 extra 비트 마스크, 넓은 소유자의 WORD 폭, 소유자 표 길이와 공유 표(길이 1·0x80000000), 표식의 flags1 비트·팔레트 경계·제외 타입·죽은 객체의 표식, 선택 프레임 검사의 각 조건(물리 프레임 수·flags1 0x440000 배수·쓰레기 HP 표식)과 소유자 팔레트/색 경계, HP 막대 갈래·분모 1·1/4 HP, 건설/자원/타이머 셋/지속/비용 막대의 경계·입력 폭·형식 오류, 상태 프로세스 호출 조건과 규칙, 표면 조회의 칸 별칭 거부, 배율/모드별 색 표 요청, 표면·clip 검증 각 조건과 clip 원점, 바인딩 블록 경계, 실제 자산 view(188 타입 번호·casttime 등).

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 370개를 적용했다. 1차 생존 128건 → 보강 시험 둘 뒤 48건 → 셋째 뒤 결과는 LEFT_JOBS 0.1 의 한 줄 요약(기준 시험 통과를 먼저 확인한 뒤에만 믿음).
- 남은 동등 변형: ① `readField` 의 기본 폭·`(readField(…,22,2)<<16)>>16`·`readField(raw,26,4)|0`·`readField(raw,12,2)` 류 폭 +1(뒤에서 비트 연산으로 잘림), ② `truncate` 의 `±2^63` 범위·`asIntN` 폭(2^56 이상 float 의 하위 32비트는 0), ③ 선택 프레임 검사의 `(frame|0) < 0` 와 `frameCount <= 0`(앞 조건과 겹치는 중복 방어), ④ `0xdddddddd | 0`·`0xcdcdcdcd | 0` 의 `| 1`(최하위 비트가 이미 1), ⑤ 타이머 루프의 `<`→`<=`(미정의 칸은 타입 0 과만 일치), ⑥ 상태 프로세스 `flags & 2` 의 마스크 확장(flag 1 이 앞 분기), ⑦ 일반 요청의 기본 배율 65536→65537(작은 그림에서 같은 결과).
- 다음 5-A 항목: `RawCursorPreviews`(185, 커서 전역 K 영역 선행)·`ObjectCollection`, 입자 본체(`RawCampaignParticles`·`ParticlePaletteMemory`), 렌더러 프레임 본체의 `Renderer`·`ClientRendererFrame`·`ScreenPresentation`·`Screen`.
