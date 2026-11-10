# webpj 원문 인코딩 계층 — WB04-E269 (2026-11-10, 4-A OriginalText)

3.1절 4-A 의 **OriginalText** 를 마감했다. cpppj `OriginalText.cpp` 는 (1) Windows-1252 ↔ UTF-8 변환(원본 파일 경계), (2) 6차 한국어의 4바이트 확장 시퀀스(`DecodeDisplayText`/`EncodeDisplayText` — 한글 음절·호환 자모·CJK 기호·전각 ASCII 를 화면용 비트맵 글꼴 바이트로), (3) `AsciiLower` 를 가진다. 웹 `assets/text-encoding.js` 는 문자열 중심으로 WB03/WB02-D 에서 (1)의 읽기와 글꼴 라벨 인코딩만 가지고 있었다. 기준선 판(`git show 1f00a25a:cpppj/src/o/OriginalText.cpp`)과 대조해 남은 것을 옮기거나 판정했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 대조 결과

| cpppj | 웹 | 처리 |
| --- | --- | --- |
| `DecodeOriginalText`: BOM 이면 뒤를 UTF-8 로, 아니면 바이트마다 Windows-1252 | `decodeOriginalText` | **의도된 차이**: 웹은 BOM 이 없어도 올바른 UTF-8 이면 UTF-8, 아니면 Windows-1252(원본 파일은 1252, 클론이 새로 만드는 한국어 파일은 UTF-8 이라는 `docs/formats/vfs.md` 4절 규칙). 시험으로 고정 |
| 0x80~0x9f 표(정의되지 않은 0x81·0x8d·0x8f·0x90·0x9d 는 같은 값의 C1 제어 문자) | `decodeWindows1252` | 256바이트 전부 WHATWG windows-1252(Node `TextDecoder`)와 같음 — 시험으로 확인 |
| `EncodeOriginalText`: 엄격한 1252 인코딩(표 밖 문자·잘못된 UTF-8 거부, BOM 없음, C1 제어 문자 5개 허용) | 없었음(`encodeWindows1252` 는 글꼴 라벨용 `?` 대체) | **`encodeOriginalText` 추가**(표 밖 문자·짝 없는 서로게이트 거부, 256바이트 왕복) |
| `DecodeDisplayText`/`EncodeDisplayText`·`IsExtendedTextDigit`·`DecodeExtendedSequence`·`EncodeExtendedCodePoint`(4바이트 확장 시퀀스) | — | **웹 제외(adapt)**: 이 시퀀스는 비트맵 글꼴(`.chfnt`)이 한글을 못 그리는 cpppj 의 6차 우회이고, 웹은 D2Coding woff2 + UTF-16 코드 단위(`encodeUtf16Units`)로 한국어를 그린다 |
| `AsciiLower` | `asciiLower`(config.js) | 이미 같음 |

## 2. 검사 (`tests/original-text.test.mjs` 3건)

- 256바이트 해독이 표준 windows-1252 와 같고 정의되지 않은 5바이트가 같은 값의 제어 문자임.
- 엄격한 인코딩이 256바이트를 모두 왕복하고(C1 제어 문자 5개 포함, 처음에는 이 5개를 거부해 cpppj 의 `std::find` 대응과 달랐다 — 모듈을 고쳤다), 한글·`U+2028`·짝 없는 서로게이트·BMP 밖 문자를 거부하며 BOM 을 만들지 않는다. `encodeWindows1252` 가 `?` 로 대체하는 것과 구별한다.
- 파일 바이트 해독: BOM 있는 UTF-8·BOM 없는 올바른 UTF-8·그 밖은 Windows-1252·ASCII·빈 입력·큰 입력.
- 변형 확인: C1 되돌림·대체 문자 변형은 탐지했다. "짝 없는 서로게이트 거부" 줄을 지워도 시험이 통과하는 것은 동등 변형이다(서로게이트 코드 포인트가 표에도 없어 아래 일반 거부 분기가 같은 `RangeError` 를 낸다 — 앞의 거부는 의도를 드러내는 중복 방어).

## 3. 대응표·남은 경계

- `PORT_MAP.tsv` `OriginalText.cpp`: partial → **port**(웹 `text-encoding.js`; 확장 시퀀스는 웹 제외 adapt, 해독 규칙은 의도된 차이로 위 표에 기록).
- 다음은 4-A **Xlat**(+ client `XlatFileRead`)이다.
