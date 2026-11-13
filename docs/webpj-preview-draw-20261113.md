# webpj PreviewDraw — 커서 미리보기 그리기 (WB04-E387, 2026-11-13, 5-A 열여섯 번째 단위)

5-A 타입별 Draw·미리보기 중 client **PreviewDraw**(cpppj `client/PreviewDraw.cpp/.h`, 기준선 363줄)를 `render/preview-draw.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 커서 미리보기 그리기(10.78 `00446270`)로, E386 `PreviewCollection` 이 공통 표시 목록에 넣은 kind 4 항목(본체 tail 0·보조 tail 2)을 실제로 그린다. **독립 상태 경로**(`PreviewDrawState` + 훅)와 픽셀 출력 경계(`PreviewPixelTarget`)·자산 바인딩(`makePreviewAssetShapes`, E385 `GameAssets` 재사용)을 옮겼고, 실제 커서 전역(`CursorPreviewSource`)에 묶는 `bindCursor`·비용 그림 토큰을 `CursorLifecycleState` 에서 읽는 `makeCursorPreviewAssetShapes`·SharedCursor 시험 3건은 커서 전역(K 영역) 단위가 옮겨진 뒤 잇는다 — PORT_MAP 상태는 `partial`.

## 1. 규칙

- **`PreviewDraw.draw(entry)`:** 항목의 대리 토큰(`tokenBase + SID × 50`)을 같은 raw 풀의 SID 로 되돌리고(기준 미만·0·stride 배수 아님·용량 밖·TYPE 표 밖은 거부) raw 의 부호 있는 화면 WORD(+22/+24)에서 카메라 원점을 빼 화면 위치를 정한다. 출력 경계가 바꾼 값은 다음 원본 접근에서 현재 참조로 다시 읽는다.
  - **본체(tail 비트 2 없음):** 윤곽(`부모 Gump 없음 && flags2 의 0x100004 비트 없음 && 윤곽 제외 타입이 아님`)이면 허용/거부 색 사각형. 배치 가능이면 소유자 변환표가 있고 로컬 플레이어가 양수일 때 소유자 색 문맥 뒤 현재 색 그림, 아니면 원색 그림(+ 덧그림 타입이면 같은 위치에 한 장 더). 불가능이면 삭제 모드(선택 타입 == 삭제 타입)에서 삭제 색 문맥 뒤 현재 색 그림, 그 밖에는 명시 불법 표 그림.
  - **보조(tail 비트 2):** 배치 불가이고 윤곽이며 격자 옵션 비트 0 또는 글자 표시가 켜져 있으면 footprint(상한 10)의 칸마다(행 먼저, 격자 `(열+1)*12+행+1`) 참인 칸의 잘린 사각형(체크 무늬, 불량 색; 앵커는 x87 절삭)을 낸다. 부모 Gump 가 없으면 방향 글자(그림자 + 본문, signed char 비교로 소문자 번호는 글자 표시가 꺼져 있으면 숨기되 기준은 한 칸씩 이동), 경고 문구 2~4(차단/동력 없음/다리 없음; 호출 플래그 0x20 이면 덮어쓰기가 없는 한 숨김), 호출 플래그 0x40 이면 비용 문구 5 와 비용 그림 한 장.
- **`PreviewPixelTarget`:** 현재 8비트 표면에서 SHP 투명/색 표/격자·윤곽을 합성한다 — 표면·clip(오른쪽/아래 제외)을 먼저 검증하고 표면 경계로 자른다. 색 문맥은 이후 현재 색 그림에만 쓰이고 명시 불법 표는 그 그림에만 적용하며(소유자 0), 특수 레코드 프레임은 그리지 않는다. 체크 무늬는 절대 좌표 위상 `(x ^ y) & 1`(오른쪽/아래 모서리 제외), 윤곽은 모서리 선 네 개(폭/높이 0 도 전달, 잘린 면에 새 선을 만들지 않음).
- **`bindPreviewDraw`·`makePreviewAssetShapes`:** 공통 분배 묶음의 `preview` 를 이 Draw 로 연결(항목 전체를 받아 tail 을 잃지 않음), 원본 타입 번호(70~185)를 `GameAssets` 의 SHP 블록으로 연결하고 비용 그림 타입만 별도 바인딩으로 돌려준다.

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 10.78 전용(CD `0043dbd0` 경로의 BYTE 프레임·스택 좌표는 옮기지 않음). 항목은 16바이트 `Uint8Array`.
- 남은 것(partial): `bindCursor`·`makeCursorPreviewAssetShapes`, 시험 `PreviewDraw_PatchNativeSharedCursor`(같은 477행을 실제 커서 참조에 공급)·`SharedCursorCurrentReadsAndContracts`·`CurrentCursorProtectedAssetBinding`.

## 3. 검사 (`tests/preview-draw.test.mjs` 14건)

- **원본 독립 관측(10.78):** `previewdraw-x86.tsv` originals **477행**(실제 미리보기 Draw 전체 — 수집/Draw 상태 40값·방향 글자 바이트 열·격자 144바이트·raw SID 5 의 타입/화면 WORD/프레임/좌표·tail 에서 원본이 낸 출력 명령열: 색 문맥 `M`·그림 `S`(모드 0 원색·3 불법 표·그 밖은 직전 색 문맥)·사각형 `R`(체크 무늬 여부)·문구 `T`)이 **첫 실행에서 전부 일치**했고 raw 객체는 Draw 전후 같다. 픽셀/글꼴/번역만 경계이며 original1037·originalCD 행(각 477)은 읽지 않는다.
- cpppj 시험 이식 5건: `CommonCollectionDispatch`(실제 수집→CRT 정렬→kind 4 분배→VFX 픽셀, 본체/보조 차례)·`PixelsColorsTransparencyClip`·`PixelsCheckerAndOutline`·`Contracts`·`CurrentStateAfterGrid`.
- 웹 계약(변형 확인에서 발견한 공백을 독립 입력으로 보강): 상태 기본값과 생성 경계, 토큰/SID/타입 오류 문구와 경계(마지막 유효 SID 23999·타입 73/74), 부호 있는 화면 WORD(0xfff0→−16·0x8000→−32768), 윤곽 제외 비트(flags2 의 낮은 비트만 있으면 윤곽), 프레임 4바이트, 소유자 표 길이·공유 표 우선(0x80000000 도 비영)·양수 플레이어 조건, 격자 footprint 10 허용/11 거부(144칸 전부), 호출 플래그 비트(0x20/0x40 외 낮은 비트 무영향), 격자 앵커 좌표가 비유한이면 거부, 방향 글자 경계(0x60 은 그림·0x61 부터 숨김·글자 표시 시 전부), 표면/clip 검증의 각 조건과 clip 자르기, 프레임 기준 상자(rect.left/top)를 그림 위치에 더함, 특수 레코드 무시, 현재 색 누락 거부, 자산 타입 경계(70/185 허용·69/186 거부), 윤곽 테두리 픽셀을 독립 기하 정의로 대조(12개 사각형)·체크 무늬 위상.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 229개와 손으로 쓴 구조 변형 60개를 적용했다. 1차 생존 87건 중 대부분이 위 보강 시험으로 해소됐다(결과 요약은 LEFT_JOBS 0.1 의 한 줄).
- 남은 동등 변형: ① `readField` 의 폭 +1(화면 WORD·프레임 DWORD 읽기가 한 바이트 더 읽어도 뒤에서 `<<16>>16`/`>>>0` 으로 잘림), ② 격자 좌표 `truncate` 의 `±2^63` 범위 검사·`asIntN` 폭(비유한은 `isFinite` 가 먼저 막고 2^56 이상 float 의 하위 32비트는 0), ③ `cellIndex >= grid.length` 의 `>=`→`>`(같은 오류 문구로 한 칸 뒤에서 거부), ④ 체크 무늬의 `visible` 빈 교차 조기 반환(루프가 어차피 비어 있음), ⑤ 필드 기본값 중 사용하지 않는 칸.
- 다음 5-A 항목: 타입별 Draw·미리보기의 남은 `SpriteDraw`(partial 470)·`RawCursorPreviews`(185), 입자 본체(`RawCampaignParticles`·`ParticlePaletteMemory`), 렌더러 프레임 본체의 `Renderer`·`ClientRendererFrame`·`ScreenPresentation`·`Screen`.
