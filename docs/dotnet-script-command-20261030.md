# dotnetpj 스크립트 명령·Config 적용 이식 — 2026-10-30

2026-10-30, HJOW-Athlon. 사용자 요청("AGENTS.md, LEFT_JOBS.dotnetpj.md 를 읽고 dotnetpj 다음 작업 진행해 줘")으로
0.61의 일시 정지를 풀고 R3k 다음 순서(명령/ConfigInterface 조립)를 재개했다. 시작 HEAD는 `d2300fea`이고 작업 트리는
깨끗했다(병렬 CP·WB 세션의 cpppj·webpj·LEFT_JOBS.md 변경은 건드리지 않았다). 현재/마지막 전체 정밀 디컴파일 PC는
HJOW-Athlon이다. cpppj `ScriptCommand.h/.cpp`와 `ScriptCommandTests.cpp`를 읽고 명령 큐·Config 적용을 이식했다.
게임/창 실행과 디컴파일은 하지 않았다.

## 1. 이식 범위

| C# 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Scripting/ScriptCommand.cs` (신규) | `ScriptCommand.h/.cpp`의 `QueueScriptCommand`·`ApplyScriptConfig` | `#,;>`·`;>` 구분 토큰, 숫자 인자 atol·문자열 고리, `0xffffffff` 지연 거부, `=`·개행 Config 쓰기 |
| `Scripting/ScriptStringRing.cs` (기존) | 같은 파일의 고리·후크 | 0.61에서 옮긴 80칸 고리와 `ScriptCommandHooks`를 그대로 쓴다. 이번 단위에서 고치지 않았다 |

- `QueueScriptCommand`는 `InputQueue.PushCommand`·`ScriptStringRing.Store`·`ConfigNumbers.ParseLong`·`RawTextToken.Read`를
  그대로 연결한다. 표 조회 0의 즉시 진단과 `0xffffffff`의 인자 처리 뒤 지연 거부를 구분한다.
- `ApplyScriptConfig`는 쓰기 주입(`Func<string,string,bool>`) overload만 옮겼다. `ConfigInterface` 직접 연결 overload는
  전체 ConfigInterface 이식(다음 단위) 때 잇는다. `ScriptCommandTests`의 두 번째 검사(`ConfigOverwriteAndQueueLifetime`)도
  `ConfigInterface`·`ParseDialogPage`가 필요해 같은 때로 미룬다.
- `AsciiLower`는 검사 하네스의 표 조회에만 쓰는 helper라 제품 코드에 두지 않고 검사 파일의 private 함수로 두었다.

## 2. fixture 연결

| fixture | 행 | 근거 |
| --- | --- | --- |
| `script-command-x86.tsv` | 262 | Token·Config·Command·Ring 네 연산의 원본 재생. `cpp-fixture-manifest.tsv`에 `Client`로 등록했고 reflected-commit은 유지한다 |

- `PORT_MAP.tsv`에 `ScriptCommand.cpp/.h`→`ScriptCommand.cs` 2행을 `L1`로 추가했다. 같은 cpp 파일의 기존
  `ScriptStringRing.cs` 2행(port)은 그대로 둔다. `--ports` 기준 바뀐 쌍은 40개(`L1` 26·`port` 14)이며 이번 단위와 무관하다.
- `config-x86.tsv`(2443행)는 `ConfigInterface` 전체 대조용이라 이번 단위에서 연결하지 않는다.

## 3. 검증

- 새 검사 `X86ScriptCommandTests.ScriptCommandsMatchNative` 1개가 262행 전부(토큰 본문·다음 위치, 설정 쓰기·없음 알림·반환,
  큐 코드·인자·고리 위치·진단 수, 고리 저장·해석·위치·80칸 무효화)를 통과했다. 기대값을 C# 결과로 바꾸지 않았다.
- 변이 1건(`0xffffffff`→`0xfffffffe`)에 L1 기대값 실패를 확인하고 원복했다. 숫자 판별 하한(`'0'`→`'1'`) 변이는 걸리지 않았다.
  fixture에 `0`으로 시작하는 인자 행이 없어 판별력 근거로 쓰지 않는다.
- 전체 **2,003개(Assets 363 + Core 1030 + Client 610) 통과**, xUnit 실패/건너뜀 0, Release 경고/오류 0이다.
- Git 제외 로그: `extracted/dotnet-script-command-*.log`에 빌드·집중·전체 검사 로그를 남겼다. 생성 로그는 삭제했다.

## 4. 남은 것

- `ApplyScriptConfig`의 `ConfigInterface` overload와 `ScriptCommand_ConfigOverwriteAndQueueLifetime` 상당 검사.
  전체 `ConfigInterface`(등록부·`Set`·`GetInt`·`onChanged`·`SaveText`) 이식과 함께 다룬다.
- `ParseDialogPage` 명령 추출·HTML 합성(`$Config`·`$Warning`·`$Button`)은 대화창 조립 단위에서 잇는다.
- R3의 실제 메뉴 상위 소유/분배와 R4의 전체 미션/월드/HUD/Edit 조립은 그대로 남는다. 전체 목표를 완료로 세지 않는다.
- cpppj·원본·CD·AGENTS.md·공통 LEFT_JOBS.md·webpj 변경은 없다.
