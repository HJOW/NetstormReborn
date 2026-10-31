# dotnetpj MissionScript 이식 — 2026-10-31

2026-10-31, HJOW-Athlon. 사용자 요청("dotnetpj 다음 작업 진행해 줘", cpppj 쪽 파일 수정 금지)으로
0.64의 다음 순서(미션 객체 단위)를 재개했다. 시작 HEAD는 `8db90513`이고 작업 트리는 깨끗했다(병렬 CP 세션의 변경은 건드리지 않았다).
현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이다. cpppj `client/Mission.h/.cpp`·`MissionFortSource.h`와
`ConfigInterfaceTests.cpp`의 `MissionScript_ResolvesScriptFortAndType`을 읽고 미션 객체를 이식했다.
게임/창 실행과 디컴파일은 하지 않았다.

## 1. 이식 범위

| C# 파일 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `Netstorm.Client/Mission/MissionScript.cs` (신규) | `client/Mission.h/.cpp` 중 미션 개체 부분 | `Get`·`Name`·`ScriptPath`·`Loaded`·`FortName`·`FortPath`·`MissionType`·`Section`·`SectionNames`·`Set`·`FortFileReader`·`FileSource`·`Dispose`(등록 해제) |
| `Netstorm.Client/Mission/MissionFortSource.cs` (신규) | `client/MissionFortSource.h` | `MissionFortSourceImage`·`MissionFortFileReader` |

- `ReadFortImage`·`FortReader` 바이트 읽기 본체와 `CreateGuideMissionScript`·`ReleaseGuideMissionScript`(Guide 모드 전역)는 미션 실행 조립(R4·R6)에서 잇는다. `FortFileReader`는 경로 매핑 갈고리까지만 연결했다.
- `Config.Registry` 공유 등록을 그대로 쓴다. 미션 객체가 살아 있는 동안 `{mission.??}` 조회가 스크립트 값을 읽고, 버리면(`Dispose`) 등록이 풀린다(원본 소멸자와 같다).

## 2. fixture 연결

- 새 fixture 연결 없음. cpp 수동 검사(`MissionScript_ResolvesScriptFortAndType`)를 계약 검사 1종으로 옮겼다.
- `PORT_MAP.tsv`에 `Mission.h/.cpp`→`MissionScript.cs` 2행과 `MissionFortSource.h`→`MissionFortSource.cs` 1행을 `port`로 추가했다.
- `--ports` 기준 바뀐 쌍은 45개(`L1` 26·`port` 19)이며 이번 단위와 무관하다.

## 3. 검증

- 새 검사 `MissionScriptTests.MissionScript_ResolvesScriptFortAndType` 1종이 최초 실행부터 통과했다.
  (미션 개체 2종+없음 1종, `fortSpec`·`missionType`·추가 설정·`tellAtList`·전역/미션 분리·등록 수명).
  대화창 분리(`ParseDialogPage`) 부분은 포함하지 않았다.
- 변이 1건(`Get`의 `mission.` 접두 제거)에 계약 검사 실패를 확인하고 원복했다.
- 전체 **2,016개(Assets 364 + Core 1040 + Client 612) 통과**, xUnit 실패/건너뜀 0, 전체 Release 경고/오류 0이다.

## 4. 남은 것

- `MissionScript`의 Fort 바이트 읽기·Guide 생성/반납은 미션 실행 조립(R4·R6)에서 잇는다.
- `ConfigOverwriteAndQueueLifetime`의 대화창 분리(`ParseDialogPage`) 부분은 대화창 조립 단위에서 잇는다.
- 목록 owner·메시지/대화창·상위 메뉴/전체 자산/호스트 조립과 R3 전체 완료는 남는다. 전체 목표를 완료로 세지 않는다.
- cpppj·원본·CD·AGENTS.md·공통 LEFT_JOBS.md·webpj 변경은 없다.
