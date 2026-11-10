# webpj 파일 계층(BaseFile 계열 adapt 8단위) — WB04-E271 (2026-11-10, 4-A)

3.1절 4-A 의 **BaseFile 계열**(BaseFile·RawBaseFileIO·RawBaseFileOpenPolicy·RawBaseFileReadPolicy·RawBaseFileWholeRead·FileSpecQuery·StartupArchiveFiles·RawConfigFileLoad)을 판정했다. 이 단위들은 `adapt-file` 분류, 곧 원본의 Win32/CRT 파일 계층이다. 웹은 파일 시스템 대신 공개 자료 폴더(`www/data`)와 TAFF 아카이브를 `GameFiles`(느슨한 파일 → 아카이브 순서 조회, WB02-D)로 읽으므로 **코드를 새로 옮기지 않고**, cpppj 가 정한 관측 가능한 계약(읽기 순서·이름 규칙·와일드카드·FileFinder 목록 순서·설정 파일 XOR 판정)을 웹이 지키는지 대조해 시험으로 고정하고, 옮기지 않는 플랫폼 경계를 기록했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 단위별 판정

| cpppj 단위 | 하는 일 | 웹 처리 |
| --- | --- | --- |
| `BaseFile`(`BaseFileSystem`) | 디스크 → TAFF → 보조(CD) 디스크 → 보조 TAFF 읽기 순서, 출처 0..5 `Probe`, 쓰기 금지 상태 `ReadOnly`, 아카이브 등록(`RegisterArchive(s)`), `Match`/`MatchDisk`/`MatchFinder` 와일드카드 목록 | `GameFiles`: **느슨한 파일 → 등록 순서의 아카이브**(원본 설치본은 `netstorm.tarc` 하나), 이름 대소문자 무시·`\`/`/`/`./` 표기 통일, `find`(= `MatchFinder`: 느슨한 파일 먼저, 아카이브 등록 순서, **첫 대소문자 무시 중복만 남기고 정렬하지 않음**). 보조/CD 디스크·`Probe` 출처 번호·`ReadOnly`·절대 경로 직접 읽기는 웹 제외(브라우저 자료는 상대 경로의 공개 파일뿐) |
| `RawBaseFileIO`·`RawBaseFileOpenPolicy`·`RawBaseFileReadPolicy`·`RawBaseFileWholeRead` | FILE 열기 모드/생성, 키(`key`/`keyLength`) 필드, 길이·읽기 정책, 전체 읽기의 최소 할당·조건부 해독·정리 순서(`basefile-*-x86.tsv`) | **웹 제외(adapt)**: 브라우저에는 FILE 포인터·CRT 상태 필드·부분 실패 후 정리가 없다. 읽기 결과 바이트는 `GameFiles.read` 가 한 번에 주고, XOR 키 해독은 `taff.js` `xorDecode`(아카이브 엔트리·설정 파일) 가 맡는다 |
| `FileSpecQuery` | `FindFirstFile/FindNextFile` 핸들로 패턴 파일을 하나씩 열거, 패턴이 없으면 `.\` 접두와 `/`→`\` 이름 한 개 | 열거는 `GameFiles.find(pattern)` 이 한 번에 목록으로 준다(`*` 0개 이상·`?` 한 글자, **폴더를 넘지 않음**). 핸들 수명과 `FindClose` 는 웹 제외 |
| `StartupArchiveFiles` | 시작 때 선택한 root 의 `*.tarc` 를 Win32 열거 순서로 모두 등록하고(기본 → CD 두 번째 목록) 보조 root 를 정함 | `GameData.build` 가 `netstorm.tarc` 를 아카이브로 등록한다(`addArchive`). CD/보조 root 와 열거 순서는 웹 제외 |
| `RawConfigFileLoad` | 설정 파일 전체 읽기 → **첫째·셋째 바이트가 0 이면 XOR 인코딩** 해독 → 끝 NUL 로 닫아 `Append` | `Config.loadBytes`: 같은 판정(`plain[0]===0 && plain[2]===0` → `xorDecode`), 첫 NUL 에서 끝나는 C 문자열, 서명 `mQdsT` 는 떼지 않고 붙임(`config-x86.tsv` 2,440행으로 대조) |

## 2. 검사 (`tests/game-files-contract.test.mjs` 3건)

- 와일드카드: `?` 한 글자(`offical?` 는 `Offical1` 만, `offical10` 제외), `*` 는 같은 폴더의 파일만(하위 폴더·잘못된 폴더 패턴은 빈 목록), 이름 비교 대소문자 무시.
- FileFinder 목록: 첫 대소문자 무시 중복만 남고(`a.english` 뒤의 `A.ENGLISH` 제외) 등록 순서를 유지하며 정렬하지 않는다. 읽기는 먼저 등록된 파일, 표기(`\D\A.english`·`./d/a.ENGLISH`)가 달라도 같은 파일이며 같은 바이트 배열을 복사 없이 돌려준다.
- 설정 파일 읽기: 평문 설정, **XOR 인코딩 파일**(서명 `mQdsT` 로 시작하는 평문의 첫째·셋째 글자가 키 `m`·`d` 와 같아 인코딩된 첫째·셋째 바이트가 0)의 해독, 첫 NUL 이후 무시. 모두 첫 실행(인코딩 시험은 입력을 서명으로 시작하게 고친 뒤)에 통과했다 — 처음 쓴 인코딩 시험은 본문이 서명으로 시작하지 않아 `Key2` 줄이 사라진 것으로 보였으나 모듈 결함이 아니라 시험 입력의 형식 오류였다.

## 3. 대응표·남은 경계

- `PORT_MAP.tsv` 8행: `BaseFile.cpp` partial → port, 나머지 일곱은 todo → port(분류 `adapt-file`, 메모에 위 웹 대응과 제외 경계를 적음).
- 실제 미션 자료의 조회 **순서**(Tell·`.fort`·타입 표)를 한 번에 확인하는 시험은 4-A 마지막 항목이다.
- 다음은 4-A **client 자료 읽기 계열**(TypeFileRead·ConfigFileRead·GifFileRead·PaletteFileRead·QuickHelpFileRead·DataManagerFileRead·FontCacheFile·LanguageOptions·ClientTemplateSectionNames·GameAssets·ClientFortGlobals 등)이다.
