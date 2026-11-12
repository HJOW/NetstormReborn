# webpj 미션 스크립트 — Mission (MissionScript) (WB04-E342, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획의 client **Mission**(기준선 `.h` 59 + `.cpp` 91 = 150줄, 기준선 뒤 +12줄)을 `core/mission-script.js` 로 옮겼다. 원본 `FUN_00482fb0` 의 `mission` 설정 객체 만들기·추가 설정과 스크립트 읽기·`mission.loadFort`/`missionType` 조회·`fortSpec` 경로 계산을 담는다(그 뒤의 미션 종류 표 검색과 미션 객체 생성은 원본 단위가 아니다). 이 단위를 위해 웹 `Config` 에 원본 몸체 네 개를 더하고(`isValid`·`appendSection`·`appendArguments`·`sectionNames`) `GameData` 에 설정 인터페이스 어댑터 세 개를 더했다(PORT_MAP `Config.cpp`/`ConfigInterface.cpp` 노트).

## 1. 규칙

- **MissionScript(configuration, name, extraSettings = '', source = null):** ① 등록부에 `mission` 이름의 설정 객체를 올린다 → ② 추가 설정(명시 source 가 있으면 `appendArguments(extra, source)`, 없으면 `configuration.appendConfigArguments`) → ③ `missionSpec` 경로의 스크립트 읽기(명시 source 면 그 읽기만, 아니면 `configuration.loadFile`) → ④ 읽혔으면 `fileName` 쓰기. **명시 source 는 추가 설정 파일·언어 스크립트·Fort 에 같은 수명으로 쓰이고 전역 reader 를 바꾸거나 우회하지 않는다**(부재도 전역 자산으로 대체하지 않는다). 생성 도중 예외가 나면 올린 객체를 등록부에서 내린 뒤 다시 던진다(원본 C++ 소멸자 대응).
- **조회:** `get(key)` 는 접두어 `mission.` 를 붙인 조회이며 스크립트를 못 읽었으면 `null`. `fortName()` = `mission.loadFort`(없거나 빈 값이면 미션 이름), `fortPath()` = `fortSpec` 경로, `missionType()`(없으면 빈 문자열), `section(name)`(치환 전 브리핑 본문), `sectionNames(prefix)`(Start 의 tellAtList), `set(key, value)`(객체 접두어 없이 현재 mission 객체에 기록, 이전에 있던 키면 true).
- **Fort 읽기:** `readFortImage(선택한 이름)` 은 현재 `fortSpec` 으로 해석한 경로를 이 미션의 source 에서 읽는다(`loadFort` 를 다시 선택하지 않음). 부재는 `Game file not found: <경로>`, 읽기 예외는 그대로, 빈 파일은 정상 값이다. `fortReader()`/`fortFileSelector(fileName)`/`fortFileReader(fileName)` 은 source 의 소유 복사를 닫아 두므로 스크립트와 설정이 사라진 뒤에도 같은 파일을 읽는다. 선택기는 I/O 없이 그 순간의 `fortSpec` 경로(와 호출자가 준 이름 변환)를 고정한 `{ fileName, read }`(E337 `createMissionFortFileSelection`)만 돌려주고, 선택 뒤 `fortSpec` 이 바뀌어도 보관한 read 는 앞선 경로를 읽는다. `fileSource()` 는 명시 source 만 돌려준다(기본 자산 reader 는 null).
- **기준선 뒤 +12줄(`1d5b1f43`, 3.6절 ② 반영):** 표시 제목 `title` 만 현재 언어의 스크립트가 처음 읽은 경로와 다르면 그 파일(익명 임시 Config)의 `title` 을 먼저 읽고, `section(name)` 도 현재 언어 파일의 섹션을 먼저 찾는다. 없으면(파일·키·섹션 부재) 처음 읽은 스크립트로 돌아간다. AI/SP 같은 실행 키는 처음 스크립트를 유지해 표시 언어가 바뀌어도 현재 월드의 실행 대본을 교체하지 않는다. 임시 객체는 조회마다 등록부에서 내린다(빈 제목/빈 섹션도 있는 값으로 취급).
- **안내 미션 수명:** `createGuideMissionScript(configuration, name, modeBox, source)` 는 미션을 만든 뒤(생성 예외는 모드·`global.inMission` 을 건드리지 않음) 모드 상자를 true, `global.inMission` 을 "1" 로 쓴다. `releaseGuideMissionScript(missionBox, modeBox, configuration)` 는 현재 미션이 있을 때만 스크립트를 **먼저** 내리고(등록부 반납) 모드 상자를 false, `global.inMission` 을 "0" 으로 쓴다(그 통지 시점에는 객체가 내려갔고 모드도 꺼져 있음). 전체 State 정리나 Editor 접두에서 원문에 없는 모드 초기화는 추가하지 않는다.

## 2. Config·GameData 확장 (이 단위의 선행)

- `Config.isValid()`(버퍼가 만들어졌는지), `appendSection(name)`(`[이름]` 줄, '[' 로 시작하면 거부), `appendArguments(text, readFile, loadFile)`(원본 00440410: `;`→CRLF, `@낱말`(공백·탭·`;` 에서 끝남)은 `loadFile` 이 있으면 그것만, 없으면 `readFile` 바이트를 `loadBytes`; 글 끝에서 끝나는 낱말도 읽고 거기서 멈춤; 빈 파일은 빈 줄 하나), `sectionNames(prefix)`(원본 00440640: `[접두어…` 섹션 이름을 `이름;` 로 이어 붙임, 본문이 비어 버퍼 끝인 마지막 섹션은 찾지 못한 것으로 다룸).
- `GameData.reader()`(언어 대체 없는 파일 읽기, Fort 등 비언어 파일용), `loadFile(config, path)`(경로 파일을 config 에 덧붙임, 기존 웹 언어 대체 적응 유지, `_loadMission` 이 이것을 쓰도록 정리), `appendConfigArguments(config, text)`.

## 3. 웹 범위

- cpppj 시험은 설정 인터페이스를 `ConfigInterface config(reader)` 로 직접 만든다. 웹은 실제 `GameData`(보호 아카이브+setup.cfg)를 만들고 시험 안에서 `configuration.assign` 으로 경로 지정값만 바꾸며, 전역 reader 우회 시험은 `data.files` 를 던지는 읽기로 바꿔 확인한다.
- **제외/적응:** `EditorFiles`·`EditorCatalogWorkspace`(Editor 작업 폴더, 편집기 웹 제외)를 쓰는 `MissionFileSource` 시험은 같은 계약(작업 namespace 의 삭제가 원본 fallback 으로 바뀌지 않음·해제 통지 순서·사용자 시나리오 별칭)을 메모리 source 로 확인했다. 같은 파일의 Player 머리글 부분(`MissionPlayers.Load`)은 E321 시험이 이미 맡는다.
- PORT_MAP: `Mission.cpp` port(base `1d5b1f43`). 앱 수명 연결(Init 이 MissionScript 를 만들고 State 가 보관)은 4-D C·D·E 조립 때 한다(PORT_MAP session 0).
- MissionTechnology 보호 미션 11개 시험(`MissionTechnology_ProtectedElevenAndInvalidStorage`)은 E338 이 이 단위로 미룬 것이며, 이 단위에서 `tests/mission-technology.test.mjs` 에 이식했다(실제 아카이브의 Tutorial1~6·TheWarBegins·MasterOfWhirligigs·DemoStormRages·GuardMyBack·DissolvedAlliance 스크립트와 보호 타입 표, **첫 실행 일치**).

## 4. 검사 (`tests/mission-script.test.mjs` 13건 + `tests/mission-technology.test.mjs` 1건 추가)

- cpppj 이식: `MissionFortSelection_SeparatesChoiceFromReadAndFreezesSelectedPath`·`ReadOwnsSourceAfterScriptAndConfigRetirement`·`DefersMissingEmptyAndThrowWithoutFallback`, `MissionFileSource` 5건의 계약(위 제외/적응 참고).
- 웹 계약: 기본 reader 경로(실제 Tutorial1 스크립트·Fort 이미지, 읽지 못한 미션의 조회), 빈 `loadFort`, `sectionNames`/`set`, 생성 예외 시 등록부 반납과 모드 유지, `dispose` 멱등, 현재 언어 title/Section 우선 규칙의 모든 분기(+12줄), Config 확장 네 개의 원본 규칙, GameData 어댑터 세 개.
- 첫 실행에서 일치하지 않은 것은 시험 기대값 두 곳이었다(`@end` 빈 파일의 빈 줄 하나가 인라인 글보다 먼저 들어간다는 점, 본문이 비어 버퍼 끝인 `[END]` 는 `sectionNames('')` 에 안 나옴). 모듈은 그대로이다.

## 5. 변형 확인과 남은 경계

- 한 문자열 치환 변형 49개(mission-script.js 34 + Config 확장 15)를 돌려 첫 실행에서 5개가 남았다. 둘은 시험 보강으로 잡았다: 기본 경로의 추가 설정이 `appendConfigArguments` 로 들어가는지(기본 경로에서 `extraKey=7` 조회), 파일 낱말만 있는 `appendArguments('@one')` 이 빈 줄을 더하지 않는지. **남은 동등 변형은 3개이며 모두 같은 결과이다.**
  1. `#localizedDisplayScript` 의 `path === ''` 검사 — 경로가 빈 문자열이면 어떤 source 도 파일을 주지 못해 임시 객체가 읽히지 않고(버퍼 없음) 곧 null 이 되므로 같다.
  2. `display.isValid()` 검사 — 버퍼 없는 임시 객체의 `get`/`section` 은 null 이고 `finally` 에서 내리므로 결과가 같다.
  3. `set` 이 `mission.` 접두어를 붙여 쓰는 변형 — `Config.set` 이 접두어가 맞는 등록 객체(`mission`)로 키를 보내므로 같은 줄을 쓴다(접두어 경로와 직접 경로가 같은 대상).
- 남은 경계: 실제 `Client` 의 미션 수명(Init 이 만들고 State 가 보관)·`Guide` 교체 시 `fileSource()` 전달·Editor 작업 폴더(웹 제외), 표시 언어 전환(13차 이후)의 실제 전환 경로.
