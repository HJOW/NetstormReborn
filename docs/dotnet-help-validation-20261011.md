# dotnetpj 도움말 인계 검증과 중첩 스타일·스크롤 정리

2026-10-11, **HJOW-Athlon**. cpppj 작업을 마친 뒤 사용자 지시에 따라 AGENTS.md·두 인수인계 문서를 확인하고 dotnetpj를 이어서 진행했다. HJOW-X3D에서 컴파일까지만 확인했던 도움말을 실제 클론 창에서 검증하고 추가 결함을 수정했다. 앞 단계 cpppj 변경은 유지했다. 원본/복사본 게임 실행·새 디컴파일·원본 파일 수정·커밋/푸시 없음. 마지막 디컴파일 PC는 앞 단계의 **HJOW-Athlon, 2026-10-11** 그대로다.

## 구현과 재현

- `HelpDocument`가 `<b><c>Left Click</c></b>`의 색과 굵기를 각각 보존한다. 기존 해석은 색 태그가 굵은 태그를 덮고 안쪽 태그 종료 시 바깥 스타일까지 잃었다. `HelpTextRun.Bold`를 화면 조각에도 전달하여 단독 `<c>drag</c>`는 보통 글꼴, 중첩 `<b><c>`는 굵은 색 글꼴로 그린다. 중복 닫는 태그는 깊이를 음수로 만들지 않는다.
- 인라인 태그 사이 공백을 보존한다. `<b>one</b> <b>two</b>`처럼 공백만 있는 구간을 버려 단어가 붙던 결함을 해결했다. 줄/문단 앞의 들여쓰기는 빈 줄을 만들지 않고 원래 줄바꿈에 남긴다.
- `HelpWindow`의 줄 배치·화살표·휠이 같은 `BodyLineHeight`를 사용한다. 영어 원본 글꼴은 15, D2Coding은 원본 간격과 실제 글꼴 줄 높이 중 큰 값을 사용한다. 현재 한국어 모드에서도 실제 높이는 **15**였으며 중첩/잘림 문제가 없었다. 한국어 모드는 도움말 원문 영어를 D2Coding으로 표시하는 검사이며 **도움말 한글 번역 완료를 뜻하지 않는다**.
- 자동 입력에 `wheel`을 추가했다. 휠 값은 이동·클릭·버튼 떼기에도 유지하여 커서를 옮길 때 반대 스크롤이 발생하지 않게 한다. 도움말의 실제 높이·스크롤 상태는 자동 검사에서만 읽는다.
- `clone_help_options_smoke.ps1`의 High Priests 클릭을 **(340,222) → (340,204)**로 갱신했다. 현재 코드로 옛 좌표를 실행하면 `priestType` 대신 **unitHelp**가 열리는 것을 재현했다. 게임 빌드 전의 기존 실행 파일로 통과한 최초 결과는 최종 결과에 포함하지 않았다.

## 원본 캡처와 줄 위치

`screenShots/help - NetStorm Instructions.png`와 최종 영어 메뉴 F1 화면을 대조했다. 기존 캡처 정렬(창 바깥 2,33 + 도움말 추가 2,1)을 사용하여 원본 y에서 **34**를 뺀 위치와 비교했다. 제목 높이 **21**을 유지해도 첫 문단까지 35픽셀이었고 목록 줄 간격도 15였다.

| 글자 윗선 | 원본 캡처 y | 최종 클론 y | 정렬 후 차이 |
|---|---:|---:|---:|
| 제목 | 96 | 61 | -1 |
| 첫 문단 | 131 | 96 | -1 |
| GAME HELP | 159 | 125 | 0 |
| 내부 링크 8줄 | 173,188,203,218,233,248,263,278 | 140,155,170,185,200,215,230,245 | +1 |
| GAME SUPPORT | 307 | 274 | +1 |
| 지원 링크 3줄 | 321,336,351 | 289,304,319 | +2 |

이 결과는 **줄 위치의 제한된 대조**이며 전체 화면의 원본 픽셀 일치 판정이 아니다. 돌 질감/버튼 크기·지원 명령 링크의 회색 표시·일부 창 치수 등은 아직 차이가 있다. 미션 F1의 목표 다시 보기 문구와 info 주제의 본문 시작(패널 위 +152 = 화면 y192)도 확인했다.

## 검증 결과

Release 빌드 **경고 0·오류 0**, 단위 **968개 통과**(Assets 331 + Core 637), 실패/건너뜀 **0**. 기준선은 965개였으며 중첩 스타일·인라인 공백·실제 원문 스타일 회귀 **3개**를 추가했다. 도움말 전용 검사 8개도 통과했다. 기존 원본 x86 대조 누적 **49,007개**는 이번 작업에서 늘리지 않았다.

최종 GUI는 **11회·게임 내 단언 313개** 모두 통과했다. 별도로 스크롤 끝/픽셀/설정 저장/일시정지 타이머도 검사했다.

| 실제 클론 실행 | 횟수 | 게임 내 단언 |
|---|---:|---:|
| 새 도움말: 영어/한국어 × 메뉴/미션 | 4 | 220 (각 55) |
| 기존 도움말/옵션: Menu·Mission·Workshop | 3 | 49 (17·14·18) |
| 공통 UI·전체화면 저장/재실행·창 복귀 재실행 | 4 | 44 (33·4·4·3) |
| 합계 | **11** | **313** |

새 `tools/clone_help_layout_smoke.ps1`는 목차의 내부 링크 8개를 실제 좌표로 누르고 Back으로 복귀한다. 사제 info 주제에서는 화살표 15, 휠 45, 본문 드래그 60픽셀 이동과 휠 뒤 커서 이동의 스크롤 보존, 스크롤바 끝 도달, 맨 위 복원, Escape의 창 유지/OK 닫기를 검사한다. 마지막 바닥 스크롤은 영어 **164**, 한국어 **209**다. 외부 주소는 누르지 않는다.

네 조합 모두 스크롤 전후 고정 머리 **50,096화소**가 같았고, 맨 위로 복원한 본문 **62,216화소**도 같았다(차이 **0**). 이 비교는 클론의 같은 화면 영역끼리 비교한 결과다. 원본 화면과의 픽셀 대조가 아니다. 스모크 자체에서 화소 비교와 바닥 위치를 확인하여 다음 작업에서도 회귀를 검출한다.

전체화면은 **1920×1080** 실제 그래픽 장치의 왕복/저장/재실행과 **1280×720** 창 복귀 뒤 재실행을 확인했다. 모든 GUI 설정은 결과 폴더의 `NETSTORM_SETTINGS_DIR`에 격리했다.

```powershell
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build --no-restore
powershell -ExecutionPolicy Bypass -File tools/clone_help_layout_smoke.ps1 -OutputDirectory extracted/screens/dotnet-help-20261011/layout-pixels
powershell -ExecutionPolicy Bypass -File tools/clone_help_options_smoke.ps1 -Mode Menu -OutputDirectory extracted/screens/dotnet-help-20261011/options-menu
powershell -ExecutionPolicy Bypass -File tools/clone_help_options_smoke.ps1 -Mode Mission -OutputDirectory extracted/screens/dotnet-help-20261011/options-mission
powershell -ExecutionPolicy Bypass -File tools/clone_help_options_smoke.ps1 -Mode Workshop -OutputDirectory extracted/screens/dotnet-help-20261011/options-workshop
powershell -ExecutionPolicy Bypass -File tools/clone_ui_smoke.ps1 -OutputDirectory extracted/screens/dotnet-help-20261011/ui-final
```

로그/PNG는 `extracted/dotnet-help-20261011-*.log`·`extracted/screens/dotnet-help-20261011/`, 픽셀/집계는 `extracted/dotnet-help-20261011-{pixels,counts}.json`에 있다. 최초 실행/비교용 캡처와 최종 11회 집계는 구분했다.

## 다음 작업

이전 인계의 도움말 1~5 검증과 전체화면 보류 항목은 완료했다. 다음은 **9-1 A**의 남은 이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션 시간 모델을 계통별 원본 근거에 맞추는 일이다. 실제 Graph/표면 identity와 Failed/TryAgain 창 폭 규칙도 남았다. `options.cfg` 저장 위치/격리 방식/진행값 이전을 다루는 9-1 C의 기존 결정 항목은 유지했다.
