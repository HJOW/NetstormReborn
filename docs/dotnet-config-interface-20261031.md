# dotnetpj ConfigInterface 이식 — 2026-10-31

2026-10-31, HJOW-Athlon. 사용자 요청("AGENTS.md, LEFT_JOBS.dotnetpj.md 를 읽고 dotnetpj 다음 작업 진행해 줘" — cpppj 쪽 파일 수정 금지 준수)으로
0.63의 다음 순서(전체 ConfigInterface 조립)를 재개했다. 시작 HEAD는 `760e5ec3`이고 작업 트리는 깨끗했다.
현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. cpppj `o/ConfigInterface.h/.cpp`와 `ConfigInterfaceTests.cpp`를 읽고
설정 인터페이스 전체를 이식했다. 게임/창 실행과 디컴파일은 하지 않았다.

## 1. 이식 범위

| C# 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Original/ConfigInterface.cs` (신규) | `o/ConfigInterface.h/.cpp` 전체 | Startup 순서·Load 인자 처리, GetInt·PathSpec·ReadInt/ReadString·IntEquals, Set/SetInt와 onChanged, SubstituteLocal, LoadLanguage, SaveText/SaveFile와 변경 표시 보존, 파일 공급·Config 갈고리 연결 |
| `Assets/OriginalText.cs` (Encode 추가) | `o/OriginalText.h`의 EncodeOriginalText | 바이트 단위 Windows-1252 인코딩. Decode 쪽과 같은 표를 공유하며 표현 불가 문자·잘못된 서로게이트는 예외 |
| `Scripting/ScriptCommand.cs` (overload 추가) | `ScriptCommand.h`의 ConfigInterface 직접 연결 overload | `ApplyScriptConfig(source, config, noKeyword)`. 기존 쓰기-주입 overload는 그대로 둔다 |

- `ConfigParseLong`·`ConfigListItem`은 기존 `Assets/ConfigNumbers`를 그대로 쓴다(중복 이식 없음).
- `EncodeConfigFile`은 ConfigInterface의 정적 메서드로 두었다(헤더의 자유 함수 자리).
- `LoadArguments`의 `@파일` 읽기는 `Func<string, byte[]?>` 하나로 받고 없으면 건너뛴다(원본의 빈 FileReader 분기).
- `SaveFile`은 쓰기 실패·인코딩 실패 모두에서 변경 표시를 복원한 뒤 예외를 다시 던진다(원본 catch-rethrow와 동일).

## 2. fixture 연결

- 새 fixture 연결 없음. `config-x86.tsv`(2440행)는 R3k-10의 객체 층 대조용으로 이미 연결돼 있고 이번 단위는
  cpp 수동 검사(`ConfigInterfaceTests` 12종 중 이식 가능한 부분)를 계약 검사로 옮겼다.
- `PORT_MAP.tsv`에 `ConfigInterface.h/.cpp`→`ConfigInterface.cs` 2행과 `o/OriginalText.h`→`Assets/OriginalText.cs` 1행을
  `port`로 추가했다. `--ports` 기준 바뀐 쌍은 45개(`L1` 26·`port` 19)이며 이번 단위와 무관하다.
- `ScriptCommand.cpp/.h`→`ScriptCommand.cs` 2행의 기준 커밋을 HEAD로 올렸다(overload 추가 후 R3k-9 L1·신규 검사로 재대조).

## 3. 검증

- 새 검사 `ConfigInterfaceTests` 10종(시작 순서·타입 접근·미션 조회·저장 왕복·인코딩·파일 쓰기 2·인자/파일 읽기·정적 소유·한정 쓰기·임시 범위)이
  최초 실행부터 통과했다. `ParseLong/ListItem` 1종은 기존 `ConfigNumbersTests`가, `MissionScript` 1종은 미션 객체 단위가 맡는다.
- 새 검사 `ScriptCommandConfigTests` 1종(Config 덮어쓰기·큐 수명)이 통과했다. 대화창 분리(`ParseDialogPage`) 부분은 대화창 단위로 미뤘다.
- 변이 1건(`Set`의 변경 알림 제거)에 계약 검사 2종의 실패를 확인하고 원복했다.
- 전체 **2,015개(Assets 364 + Core 1040 + Client 611) 통과**, xUnit 실패/건너뜀 0, Release 경고/오류 0이다.
- `HtmlTextParserTests`·`HtmlTextParserNativeTests`에 있던 `ConfigText` 이름 충돌 해소용 using 별칭은 R3k-10에서 이미 두었고 이번 단위에서 손대지 않았다.

## 4. 남은 것

- `MissionScript_ResolvesScriptFortAndType` 상당(미션 객체·`fortSpec`·`missionType`·`tellAtList`)은 미션 객체 단위에서 잇는다.
- `ConfigOverwriteAndQueueLifetime`의 대화창 분리(`ParseDialogPage`) 부분은 대화창 조립 단위에서 잇는다.
- 목록 owner·메시지/대화창·상위 메뉴/전체 자산/호스트 조립과 R3 전체 완료는 남는다. 전체 목표를 완료로 세지 않는다.
- cpppj·원본·CD·AGENTS.md·공통 LEFT_JOBS.md·webpj 변경은 없다.
