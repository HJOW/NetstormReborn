# Editor 작업본 루트의 자산 경로 정규화 — 465

- 작업 PC: HJOW-Athlon, 2026-10-26.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. 디컴파일 소스는 변경하지 않았다.
- 범위: 기존 맵 Edit 진입에 사용하는 별도 작업 디렉터리 계산을 보완한다. 게임 규칙이나 원본 파일 저장 경로는 바꾸지 않는다.

## 문제와 수정

464의 `EditorRuntime`은 `absolute(GameDirectory()).filename()`으로 자산 디렉터리 이름을 구했다. Windows 자산 경로 끝에 `/` 또는 `\`가 있으면 마지막 요소가 비어 `자산폴더/-editor`를 선택한다. 이 경로는 보호된 자산 디렉터리 안이어서 `EditorFiles` 생성 시 거부된다. `.`과 `..`를 포함한 표기도 서로 다른 작업본을 선택할 수 있었다.

`EditorWorkingRoot`가 기존 파일 장치와 같은 `weakly_canonical(absolute(...))` 경계를 사용하고 끝의 빈 요소를 제거한 뒤 실제 디렉터리의 형제 `<이름>-editor`를 계산한다. 파일시스템 루트처럼 이름 없는 자산 경로는 거부한다. 계산 자체로 파일이나 디렉터리를 만들지 않는다.

`UberEditor.inc`는 이 helper를 작업 루트와 원본/CD 보호 루트의 형제 기준에 함께 사용한다. 최종 경로의 보호 검사와 기존 작업본 보존은 기존 `EditorFiles` 부모가 그대로 수행한다.

## 검증

기존 검사 본문을 변경하지 않고 `EditorFilesTests.cpp`에 다음 두 사례를 추가했다.

- `EditorFiles_WorkingRootNormalizesDirectorySpellingsAndPreservesWork`: 일반 경로, 끝 `/`, Windows 끝 `\`, `.`, `d/..`, `d/../`가 같은 형제 작업 루트와 수정한 기존 작업본을 선택하는지 확인한다. 실제 자산 디렉터리로의 쓰기 거부와 그 안에 `-editor`가 생기지 않는지도 확인한다.
- `EditorFiles_WorkingRootStillRejectsProtectedAndNamelessRoots`: 정규화 뒤에도 상위 보호 루트/원본 하위 작업 경로를 거부하고, 파일시스템 루트를 이름 없는 작업본으로 변환하지 않으며 파일을 만들지 않는지 확인한다.

Release 증분 빌드1 종료0, `EditorFiles` 집중7개(신규2개 포함)·실패0을 확인했다. 결과는 `cpppj/editor-working-root-20261026.json`, 원시는 `extracted/cpp-editor-root-465-{build1,focused1}.log`다. 기존 검사 본문은 그대로 유지한다. 후행 구분자를 포함한 실제 Edit 진입은 후속 기능 묶음의 같은 최종 제품에서 검사하며 이번 단위에서 GUI/전체 CTest를 실행했다고 주장하지 않는다.

## 남은 범위

`d/!state.dat`가 있을 때의 실제 open/8Player 복구/전역 복구/재접속 질문, 캠페인 이탈 후 Native mission 모드의 수명은 별도 후속이다. 새 맵, 편집 도구, 저장, Test Battle, 언어별 시나리오, 전체 미션 자연 완주와 원본 동일성도 이번 수정의 완료 범위가 아니다.
