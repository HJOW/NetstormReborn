# webpj 원본 토큰 읽기·SNART 이름표·Template 구역 이름표·DataManager 초기화 이식 — WB04-E19

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열아홉 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawTextToken`, `o/RawSnart`, `o/RawTemplateSectionNames`, `o/RawDataManagerInitialize`(10.78)를 한 묶음으로 옮겼다. 앞의 둘이 뒤의 둘이 쓰는 도구이고 `DataManager` 초기화의 원본 관측이 네 모듈을 함께 검증한다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/raw-text-token.js` | 00487620 | `readRawTextToken(source, offset, delimiters)`: NUL은 입력/구분자의 끝, 앞 space/tab만 건너뜀, 입력 78 바이트 한도(따옴표도 한도 1을 소비하지만 출력에는 쓰지 않음), 인용 밖 구분자에서 멈추고 한도로 멈춰도 현재 연속 구분자를 건너뜀, 뒤 space/tab은 지우되 첫 출력 바이트는 남김. 반환 `{text, next}`. |
| `www/core/raw-snart.js` | 004a8800, 004a8850 | `selectSnartName(names, index, assertion)`: 같은 이름표를 제자리 변환(첫 NUL→0xff, 첫 0xff 전의 `/`→NUL, 나머지 원시 BYTE 보존)하고 index 번째 이름의 시작 뷰를 돌려줌. 첫 BYTE가 `/`가 아니면 원문 진단(Snart.cpp 0x46) 후 계속. `readSnartName`은 현재 첫 NUL까지 읽는 문자열. |
| `www/core/raw-template-section-names.js` | 004bccb0 | `getTemplateSectionNames(globals, hooks)`: 현재 토큰이 비0이면 하위 없이 반환, 아니면 512 BYTE 할당→토큰 공개→기본 135 문자+NUL→`Terr00..19`와 `/`를 현재 NUL 뒤에 매번 현재 표를 다시 조회해 붙임→마지막 `/`(총 277 BYTE, 표 끝 235 BYTE는 미정). 토큰 0이면 원문 004bcced의 NULL 복사로 거부. |
| `www/core/raw-data-manager-initialize.js` | 0044d870 | `initializeDataManager(manager, hooks)`: root(+4)가 비0이면 진단 0xc9 → 현재 이름표를 정확히 한 번 조회(표 포인터 고정, 이후 이름 내용은 Create 뒤 다시 읽음) → index 0부터 `selectSnartName`(진단 0x46)으로 이름을 선택해 빈 이름이면 반환, 아니면 `create(이름 시작 뷰)`. |

## 2. 검증 (`tests/data-manager-names.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| 토큰 128행(소비 포인터·토큰 바이트) + 한도/인용/NUL/뒤 공백 보존 경계 | `script-command-x86.tsv`의 `Token` 행(같은 파일의 Config 18·Command 31·Ring 85행은 각 해당 단위) |
| DataManager 초기화 64행(root 진단·이름표 1회 조회·이름별 Create·미래 이름 변경·repeat) — SNART와 Template 이름표를 함께 검증 | `data-manager-initialize-x86.tsv` |
| Template 구역 이름표 12행(최초 512 할당·재사용·NULL 공개 후 실패의 전체 manager/두 표/호출 순서) + 기존 표는 하위 없이 반환 | `template-section-names-x86.tsv` |
| SNART 제자리 변환·index 선택·진단 장치 누락 | (독립 입력) |

최초 실패와 교정: 구현은 처음부터 원본 관측 전부를 통과했다. 직접 쓴 토큰 경계 기대값 하나가 연속 구분자(`,`와 공백) 소비를 한 칸 잘못 세어 실패했고, 모듈이 아닌 기대값을 고쳤다(원본 128행은 처음부터 통과).

## 3. 경계

- `RawSnart`는 `DataManagerInitialize`·Template 이름표 관측을 통해 대조했고, 기존 `RawDataManagerImage`(파일 이미지 reader)가 같은 이름표를 읽는 경로는 해당 DataManager 단위에서 대조한다.
- cpppj의 `ClientTemplateSectionNames`·`CampaignTemplateStorage`·`SoloMapCatalog`와의 공유 이름표 수명 검사는 해당 Client 단위에서 한다.
- 실제 `Create`(0044d1b0)와 할당·서식 장치(004a89c0)는 호스트가 hooks로 공급한다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
