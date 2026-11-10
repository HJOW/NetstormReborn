# L1~L3 일본어 지원 1차: 조사·구현·검수 (제품 1288, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음).

## 1. L1 — 언어 자원 조사와 범위 명시

| 언어 | 원문 10.78 자료 | 클론 현재 |
| --- | --- | --- |
| English(번호 0/1) | 전체 | 전체 |
| German(3) | `d\*.german` 85개와 xlat 774키(원문 TAFF) | 원문 경로 그대로(메뉴에는 노출하지 않음) |
| French(2)·Spanish(4)·Portuguese(6) | 이름만 예약, 자료 없음 | 없음(미지원) |
| **Japanese(5)** | 이름만 예약, 자료 없음 | **UI 번역표 509키(`assets/lang/japanese/xlat.japanese`), 외부 글꼴 Noto Sans JP, Options→Language 메뉴 `日本語`** |
| Korean(7, 클론 추가) | — | UI 번역표 509키 + 공식 본문 43개 + 도움말(6차 한국어) |

**일본어 미번역 범위(영어 대체로 표시):** 미션/튜토리얼/캠페인 본문 43개(`*.japanese` 없음 → 영어 본문 대체), 도움말 128주제, Tell 창(Credits 목록·Help 등)과 `config.*` 본문, 맵/캠페인 이름, 사용자 맵 제목, 유닛·건물 이름의 도움말 설명 문단. 영어 `xlat` 키가 없는 문구도 영어로 나온다. 번역표는 한국어 표와 같은 509키이며 서식 코드(`~색`, `%s/%d`, `{Focus}`)와 줄바꿈 위치가 영어 키와 일치하도록 병합 때 자동 검증했다(예외는 한국어와 같은 `~E~S` 키 1건).

## 2. L2 — 구현

- **표시 부호화(`o/OriginalText`):** 한글용 4바이트 확장 시퀀스에 히라가나·가타카나(`0x3040~0x30ff`)와 반각 가타카나(`0xff61~0xff9f`) 정적 구간을 한국어 인덱스 **뒤에** 추가했다(기존 한글 인덱스 불변). CJK 통합 한자 약 2만 자는 정적 구간에 담을 수 없어 **처음 쓰인 순서대로 남은 3,946개 인덱스에 동적 배정**한다(표시 바이트는 저장하지 않는 화면 경계라 같은 프로세스 안의 표만 맞으면 된다. 용량이 소진되면 한자는 확장되지 않는다).
- **언어 선택:** `kJapaneseLanguageNumber=5`, `IsExtendedLanguageNumber`로 한국어·일본어를 같은 번역표·확장 글리프 경로에 태웠다. 옵션 메뉴 Language 하위에 `日本語`가 추가됐다.
- **글꼴(exe 내장 없음):** 언어 변경/시작 때 `ApplyExtendedFontForLanguage`가 일본어는 `fonts\NotoSansJP*.ttf`(`Noto Sans JP`), 그 밖은 D2Coding을 `AddFontResourceExW(FR_PRIVATE)`로 등록한다. 전각 폭은 일본어에서 히라가나 `あ`를 대표 글자로 잰다.
- **번역표/본문 탐색:** `ReadLanguageTranslationFile`(한국어·일본어 공통), `.japanese` 본문이 없으면 `.english`로 대체(`EnglishFallbackName`).
- **11.01 패치 크레딧:** 일본어 모드에서도 세 번째 항목과 본문이 일본어다(`ProductTellBody`의 인자를 원문 언어 번호로 일반화).
- **배포:** `package-cpppj.ps1`이 `fonts\NotoSansJP-*.ttf`, `fonts\NotoSansJP-License.txt`, `lang\japanese\xlat.japanese`를 필수로 동봉한다. `NotoSansJP-License.txt`는 SIL OFL 1.1 본문(기존 `NotoSans-License.txt`와 같음)에 Noto Sans JP의 저작권 줄(Adobe, Google Fonts 배포본 표기)을 붙인 것이며 **원본 OFL.txt와 대조해 확정해야 한다(사용자 확인)**.

## 3. L3 — 검수

- 단위: `netstorm_korean_text_tests` 39건 통과(새 `JapaneseTextTests` 5건: 가나 왕복·한자 동적 배정·언어 번호/글꼴 탐색·번역표 509키 일치/왕복·크레딧). 6차 한국어 시험 2건은 `0x4e2d`(한자)가 이제 확장 대상이므로 `0x3400`(CJK 확장 A)으로 기준을 옮겼다.
- 실제 화면(개발 빌드와 **배포 ZIP을 푼 exe 모두**): 일본어 메인 메뉴 8버튼(`キャンペーン/マルチプレイヤー/デモ/ヘルプ/編集/クレジット/オプション/終了`)이 외부 Noto Sans JP로 표시되고, Options→Language→`日本語` 선택이 즉시 반영된다. 종료 코드 0·오류 로그 없음.
- 배포 ZIP 1433항목을 `tools/verify_cpp_package.py`로 전수 대조(일치, 누락/금지 0, exe 글꼴 시그니처 0).

## 4. 한계와 사용자 확인

- 물리 일본어 IME 조합·OS 클립보드, 일본어 번역 품질 검수(번역은 AI가 작성), 미션/도움말 본문 번역은 후속이다.
- Noto Sans JP 가변 글꼴의 기본 인스턴스가 Thin(wght 100)이라 GDI가 어떤 굵기로 렌더링하는지는 화면 크기별 확인이 더 필요하다(메인 메뉴·옵션에서는 읽을 수 있는 굵기로 보였다).
- 새 일본어 한자는 프로세스 수명 동안 최대 3,946자까지 배정한다(기본 UI는 수백 자 미만).
