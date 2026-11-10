# webpj 번역표 마감 — WB04-E270 (2026-11-10, 4-A Xlat)

3.1절 4-A 의 **Xlat**(+ client `XlatFileRead`)을 마감했다. 10.78 일곱 파일 버퍼와 언어별 번역 장부(`RawXlat`)·시작 순서(`initializeRawXlat`)는 WB04-E242 에서 옮겨 `xlat-runtime-x86.tsv`/`xlat-assets-x86.tsv` 로 대조했고, 웹 메뉴가 쓰는 UTF-8 용어표 계층(`assets/xlat.js`, WB03-N)은 cpppj `Xlat.cpp` 의 `XlatTable` 규칙을 따랐지만 시험이 한국어 시험(`korean.test`)의 간접 사용뿐이었다. 기준선 판(`git show 1f00a25a:cpppj/src/o/Xlat.cpp`·`client/XlatFileRead.cpp`)과 대조했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 대조 결과

| cpppj | 웹 | 처리 |
| --- | --- | --- |
| `XlatTable` 생성(CR 제거 → 줄 첫 글자만 `*`/`-`/`=` 구분, 원문·번역문 끝 줄바꿈 하나만 뗌, 끝나지 않은 마지막 블록 버림, 중복 원문은 덮어씀) | `XlatTable` 생성자 | 이미 같음 — 시험 5건으로 고정 |
| `Translate(original, languageNumber)`: 대소문자 구분 완전 일치, 없으면 원문, **언어 0·1 은 표를 보지 않고 원문**(004de9a0 우회) | `translate(original)` 에 언어 인자가 없었음 | **`translate(original, languageNumber)` 로 확장**(0·1 우회, 생략하면 우회 없음) |
| `TranslateView`(3-E ①) | — | JS 문자열에는 해당 없음(C++ 포인터 정체성용) |
| `Size()` | `size`·`has` | 이미 같음 |
| `RawXlat`·`InitializeNativeXlat`(german(3) → english(1) → portuguese(6) 순서, 부재 로그, 다음 파일 계속) | `core/raw-xlat.js` `RawXlat`·`initializeRawXlat` (E242) | 이미 같음 |
| `XlatFileRead.cpp` `ReadXlatFile`/`LoadNativeXlatFile`(CRT FILE 열기·길이·읽기·정규화·닫기 순서, 기존 노드 재사용 거부, 키 XOR 복호 후 파싱) | `initializeRawXlat` 의 `hooks.exists/read` 로 파일 읽기를 대체 | **웹 adapt**: 브라우저는 FILE 을 열지 않고 호출자가 바이트를 준다. FILE/BaseFile 필드·정리 순서 효과열(`F:… P;O;T;S:…`)과 부분 실패 후 정리는 CRT 수명 문제라 옮기지 않는다. 읽은 바이트의 정규화·병합·언어별 조회 결과는 E242 시험(253행 fixture 의 번역 칸)이 이미 같은 `RawXlat.load` 로 대조한다 |

## 2. 검사 (`tests/xlat-table.test.mjs` 5건)

CR 과 줄 첫 글자 규칙(공백·줄 중간 구분은 무시), 끝나지 않은 블록 버림·LF 없는 마지막 `=` 줄 처리·잘못된 순서 구분 무시, 줄바꿈 하나만 뗌·여러 줄 원문/번역문·중복 덮어쓰기와 고유 원문 수, 대소문자 구분·원문 폴백·언어 0·1 우회·언어 2 이상의 표 사용, 블록 안 `*` 줄 무시와 소문자 키. 모두 첫 실행에 통과했다. 변형 9개(CR 제거·줄 첫 글자·구분 상태 0 관문·번역/원문 끝 trim·언어 0/1 우회·중복 덮어쓰기·대소문자 무시)를 sed 로 넣어 보니 3개(CR 제거는 JSON 이스케이프로 앵커 불일치, 구분 상태 0 관문·대소문자 무시는 시험 입력 부족)가 처음에 확인되지 않았고, 블록 안 `*` 줄과 소문자 키 시험을 더하고 앵커를 고쳐 9개 모두 탐지했다.

## 3. 대응표·남은 경계

- `PORT_MAP.tsv` `Xlat.cpp`: partial → **port**, `XlatFileRead.cpp`: todo → **port**(adapt-file, 읽기는 `initializeRawXlat` hooks 로 대체). `RawXlat.cpp` 는 E242 의 port 그대로.
- 다음은 4-A **BaseFile 계열**(adapt 8단위)이다.
