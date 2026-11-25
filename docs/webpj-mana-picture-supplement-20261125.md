# webpj 마나 아이콘 보충과 별칭(cpppj 1335 판정 ②) — WB04-E1336

- 작성 일시: 2026-11-25
- 작업 ID: `WB04-E1336`
- 근거 커밋: cpppj `87324232f`(`20261123-16 CP`, 제품 1335) — `HtmlTextParser.cpp`·`Markup.cpp`·`StyleTextMarkup.cpp`·`UberGump.cpp`·`TextPictureResources.h`·`ManaPictureData.inc`

## 1. 판정

| 변경 | 판정 | 처리 |
| --- | --- | --- |
| 공식 안내·도움말 HTML 의 마나 그림 `mana.8`·`9`·`10`·`11`·`17` 을 현행 `Imana.4`·`5`·`6`·`7`·`9` 로 교정(`BindHtmlPictureText`) | ② 웹 재반영 | `htmlPictureTextRepaired`(`ui/html-text-parser.js`)를 `bindHtmlPictureText`·`help-topic.js`·`tell-window.js`의 `picture` 훅에 연결 |
| 10.78 마나 블록 8프레임 → 원본 CD 25프레임 보충(UI 측정·출력 한정, 월드 TYPE·SHP 불변) | ② 웹 재반영 | `assets/text-picture-resources.js`의 `resolveTextPictureResource`를 `picture-resolver.js`(메뉴·Help·Tell), `markup-picture.js`(`MarkupAssets`), `style-text-markup.js`(높이·프레임 수)에 연결 |
| `KoreanResources.cpp`(5238ee83): `.japanese` 공식 본문 번들 읽기 | ③ 13차(일본어) | 한국어 경로 변경 없음 |
| `StyleTextBuffer.cpp`(5238ee83): 가나·한자 문자 단위 줄바꿈 | ③ 13차(일본어) | 일본어 조각에만 적용되고 한글·영문 경로는 기존과 같음 |

## 2. 구현

- `webpj/tools/make-mana-picture-data.mjs`: cpppj 의 `ManaPictureData.inc`(8,016바이트)를 `www/assets/mana-picture-data.js`(base64)로 변환한다. `--check`로 일치를 검사한다. `originalCD`는 읽지 않는다.
- `www/assets/text-picture-resources.js`: 별칭표 `repairManaPictureText`, 보충 `ShapeDatabase`(처음 쓸 때 한 번 생성), `resolveTextPictureResource(shapes, block, clusterCount)`. 마나 블록이 8클러스터·8프레임일 때만 보충본(블록 0, 25프레임)으로 바꾸고 다른 타입은 빌려 준 저장소를 그대로 쓴다. cpppj 와 같은 조건이다.
- 웹은 10.78 전용이므로 cpppj 의 `Patch1078` 분기는 항상 참이다.

## 3. 검증

- 새 시험 `tests/mana-supplement.test.mjs` 4건: 별칭 다섯 개와 불변 사례, 앞 8프레임의 사각형·해독 결과·Squid 헤더가 10.78 원본과 같음, `mana.8~24` 실제 그림·`mana.25` 빈 그림, `.inc`와 모듈 바이트 일치.
- 기존 `tests/picture.test.mjs`·`tests/help.test.mjs`·`tests/style-text-markup.test.mjs`의 기대값을 UI 보충 25프레임에 맞췄다(`mana.8`은 더 이상 빈 그림이 아니고 도움말 그림 377/377 모두 실제 그림).
- 원본 10.78 `htmlpicture-x86.tsv` 138행은 별칭 교정 전 helper를 그대로 대조하고, 실제 앱 `bindHtmlPictureText`는 `mana.17→mana.9` 별칭을 별도 확인한다.
- 집중 검사 42/42·추가 교정 검사 40/40 통과. `npm --prefix webpj run build`로 번들을 다시 만들었고 Node 26.13.0 전체 회귀 6,851/6,851 통과(376.5초, 실패·취소·미실행 0).
- `fixture-manifest`는 1,056항목이며 새 cpppj `ManaPictureData.inc` 교차 확인 소스와 E1335 Tutorial4 자료의 stale 해시 16항목을 동기화했다. `bundle-fresh`·`port-map`은 전체 회귀에서 통과했다.

## 4. 남은 것

- 실제 F1 도움말·미션 안내 화면의 마나 아이콘 육안 대조는 사용자 확인 범위다(원본 10.78 은 이 번호의 그림이 없어 원본 스크린샷 대조 불가).
- `EmbeddedBitmapFont`·`KoreanFontSettings.h`·`ResetAllSettings`·`TextPictureResources.h`·`ManaPictureData.inc` 는 대응표 밖 새 파일이며 재기준선 때 등록한다(웹 해당분은 위 구현과 E863·갈무리11 단위로 반영됨).
