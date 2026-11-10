# webpj 설정 층 마감 — WB04-E268 (2026-11-10, 4-A Config·ConfigInterface)

3.1절 4-A 의 **Config·ConfigInterface** 를 마감했다. 웹은 `assets/config.js`(Config·ConfigRegistry·키 조회/원시 값/치환/섹션)와 `assets/game-data.js`(GameData = cpppj `ConfigInterface` 의 웹 대응)로 이 층을 WB02-D 에서 이식했고 `config-x86.tsv` 2,440행으로 대조하고 있었다. 기준선 판(`git show 1f00a25a:cpppj/src/o/ConfigInterface.cpp`)과 `ConfigInterfaceTests` 12건을 하나씩 대조해 남은 차이를 옮기고 나머지는 판정했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 대조 결과

| cpppj `ConfigInterface` | 웹 | 처리 |
| --- | --- | --- |
| `ConfigParseLong`·`ConfigListItem` | `configParseLong`(config.js)·`configListItem`(core/raw-mission-ai-settings.js) | 이미 같음 — 시험으로 재확인 |
| `GetInt`·`ReadInt`·`ReadString`·`IntEquals` | 없었음 | **`GameData.getInt`/`readInt`/`readString`/`intEquals` 추가**(키 없음·같은 값은 거짓, 문자열은 대소문자 구분, `IntEquals(…, 0)` 항상 거짓) |
| `ReadStringBytes`(3-E ③) | — | **① 웹 적응**: 고정 C 문자열 목적 버퍼의 첫 불일치 뒤 NUL 복사는 소비자(`ClientMain` 옵션 읽기·`ClientLocalStateGlobals`)가 바이트 버퍼일 때만 필요하다. 웹은 설정 값을 JS 문자열 저장소(`platform/settings`·`GameData`)에 두므로 `readString` 이 같은 결과(바뀌었는가·새 값)를 낸다. 3.6절 ③ 행 해소 |
| `Startup` 인자 순서·`Load`(첫 `@` 전환)·섹션 줄 | `GameData._loadConfiguration`(직접 조립) | 웹 adapt(직접 조립 + 브라우저 저장소). 결과(`[ARGS]`…`[END]` 본문과 우선순위, 버전 줄, 11.01 제품 표기)는 `data.test.mjs`·`startup.test.mjs` 가 실제 자료로 확인 |
| `{@미션.키}`(missionSpec 경로 스크립트를 임시 `local` 객체로 읽음) | `ConfigRegistry({ loadMission })`·`GameData._loadMission` | 이미 같음. **현재 미션** 연결은 4-D |
| `LoadLanguage`·`PathSpec`·`SubstituteLocal` | `GameData._loadLanguage`·`pathSpec` | 이미 같음 |
| `SaveText`·`SaveFile`(XOR "mydoghasfleas")·사용자 홈 우선 읽기 | `platform/settings`(브라우저 저장소, `options-x86.tsv` 저장 형식 시험은 WB03 쓰기/저장 단위) | 웹 적응 |
| `ConfigPathLoad`/`ConfigLiveRead`(Native FILE 수명·6개 후보 경로 탐색·작업 폴더·CD 찾기) | — | 웹 제외(브라우저 자료 조회로 대체, 호출자가 경로와 읽기를 준다) |

## 2. 검사 (`tests/config-interface.test.mjs` 3건)

- **`original-x86.tsv` Config 270행**(원본 기계어의 `FindKey`/`GetRaw`): 키 오프셋과 원시 값(`missing` 또는 `hex:`)이 전부 같다(웹 `findKey`/`getRaw`). 첫 실행에 통과했다.
- `ConfigParseLong`(공백·부호·멈춤·32비트 감김 4294967297 → 1)과 `ConfigListItem`(`a;b;;d` 의 0~4번째와 음수 인덱스).
- 타입 접근자의 원본 quirk: `ReadInt`(다르면 새 값, 같은 값/키 없음은 거짓)·`ReadString`(대소문자 구분)·`IntEquals`(`seven` ` 7x` 는 7, 0 비교는 항상 거짓)·`GetInt`(없으면 0). 변형 8개(`getInt` 기본값·`readInt` 같은 값/키 없음·`readString` 같은 값/대소문자·`intEquals` 변경 조건/시작 값)를 sed 로 넣어 보니 처음에 1개가 문자열 앵커 불일치(줄바꿈)로 실행되지 않았고, 한 줄 앵커로 다시 넣어 전부 탐지했다.

## 3. 대응표·남은 경계

- `PORT_MAP.tsv` `Config.cpp`: partial → **port**. `ConfigInterface.cpp`: partial 유지 — 남는 것은 `Startup`/`Load`/`SaveFile` 의 파일 장치(웹 adapt)와 현재 미션 연결(4-D)이다.
- manifest `originalx86` 항목의 scope 를 Config 270행까지 넓혔다(소비자·모듈 추가).
- 다음은 4-A **OriginalText**(원문 인코딩 계층의 남은 부분)이다.
