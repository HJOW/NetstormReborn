# dotnetpj 캠페인 진행 저장 검증·돌 버튼 모양·도움말 본문 원본화 (진행 기록)

> **후속 완료(2026-10-11, HJOW-Athlon):** 아래 미검증 도움말을 확인하고 중첩 색/굵기·인라인 공백을 수정했다. Release 경고/오류 0, 단위 968개·최종 GUI 11회/313개 게임 내 단언 통과. 도움말 1~5와 전체화면 재실행 보류도 해소했다. [후속 상세 기록](dotnet-help-validation-20261011.md). 아래는 HJOW-X3D에서 중단한 당시 상태다.

작업 PC: `HJOW-X3D`, 2026-10-11. 마지막 디컴파일 호스트는 기존 `HJOW-Athlon`이며 **이 PC에서는 디컴파일 산출물을 읽지 않았다**(`extracted/refined/` 없음). 원본 게임·복사본 실행, 새 디컴파일, cpppj/AGENTS.md/원본 폴더 변경, 커밋/푸시는 없다. 사용자 지시("구현 완료 후 문법 검사까지만, 테스트는 하지 않는다")에 따라 **마지막 구현은 컴파일(Release 경고 0·오류 0)만 확인했다.**

## 1. 캠페인 진행 저장 흐름 검증 (5-6 마무리) — 확인 완료

1-1 승리 창 → 설정 저장 → 재실행 → 1-2 해제를 실제 창 모드 클론으로 끝까지 확인했다. 오래 걸리는 실제 플레이를 건너뛰려고 **실제 승패 알림과 같은 경로**(`FortMapViewer.OpenTutorialTell`)로 섹션을 여는 검증 명령을 넣었다.

- UI 자동화 명령 `mission-section 섹션이름` ([UiAutomation.cs](../dotnetpj/src/Netstorm.Game/UiAutomation.cs) → [NetstormGame.cs](../dotnetpj/src/Netstorm.Game/NetstormGame.cs) → `FortMapViewer.OpenMissionSection`). 사용자 화면에는 영향이 없다.
- 새 스모크 [tools/clone_campaign_progress_smoke.ps1](../tools/clone_campaign_progress_smoke.ps1): 첫 실행(캠페인 → 1-2 잠김 확인 → 1-1 진입 → `Succeeded` 열기 → 종료) 뒤 `settings.json` 의 `ScriptValues` 에 `DoneTheWarBegins=1`(대소문자 무시) 저장을 확인하고, 같은 설정 폴더로 재실행해 1-2 가 열려 브리핑(`mission=masterofwhirligigs`)에 들어가는지 본다. 문자열을 ASCII 로 쓴 것은 BOM 없는 파일을 Windows PowerShell 5.1 이 ANSI 로 읽어 한글 문자열에서 구문 오류가 났기 때문이다.
- 결과: 위 흐름 **통과**. `Done{mission.fileName}` 키(`thewarbegins` 소문자 이름)와 메뉴의 `{DoneTheWarBegins}` 조회가 대소문자 무시로 맞았다. 재실행 뒤 1-1 줄에 완료 표시(파란 점), 2번 줄이 밝게 열린 것을 캡처로 확인했다.
- 1024×768 창에서 미션 목록 줄 위치(화면 중심 기준): 1-1 = +51, 1-2 = +69 (줄 높이 18 간격). 이전 스모크의 38·56 은 다른 해상도/목록 배치 값이다.

## 2. 돌 버튼 모양 원본화 — 구현·부분 확인

[`OriginalUiSkin.Button`](../dotnetpj/src/Netstorm.Game/OriginalUiSkin.cs) 를 원본 메인 메뉴 캡처(`screenShots/mainMenu.png`, 클라이언트 오프셋 (2,33))의 버튼 8개(75×19)에서 잰 값으로 바꿨다. 바탕을 검정 16%로 덮던 것과 1픽셀 테두리를 없앴다.

| 요소 | 잰 값 (RGB 평균, 캡처 손실 압축 ±10) |
|---|---|
| 바깥 윤곽 1픽셀 | (53, 51, 52). **네 모서리 픽셀은 비운다**(위·아래 가로줄 x 1..폭-2, 좌·우 세로줄 y 1..높이-2) |
| 밝은 선 | (198, 183, 162) = 기존 `MenuLight`. 위 2줄(안쪽 전체 폭), 왼쪽 2칸(위 2줄 아래 ~ 아래 1줄 위) |
| 어두운 선 | (82, 77, 69) = 기존 `FrameDark`(질감 ×0.61). 아래 1줄(안쪽 전체 폭), 오른쪽 2칸(위 2줄 아래 ~ 아래 1줄 위) |

- 새 `StoneButtonFrame`. 눌린 모양은 밝고 어두운 쪽을 맞바꾼다(눌린 원본 캡처는 없어 기존 기록 "테두리 명암 뒤집기"만 따름).
- 확인(이 지시 이전에 실행): 원본 대비 테두리 고리 평균 차이 **62.69 → 18.26**(남은 차이는 질감 위상·손실 압축). 클론 창 스모크 `clone_buttons_smoke` 통과(tips 14·menu 71 checks), `clone_campaign_smoke`, `clone_help_options_smoke` Menu·Mission 통과, 눌린 모양·승리 창 버튼 확대 캡처 육안 확인.
- 근거 없는 부분: 비활성 버튼(검정 45%)은 원본 캡처가 없어 그대로 뒀다. 메인 메뉴의 Multiplayer·Demo·Edit·Credits 는 원본에서 활성이지만 클론은 미구현이라 비활성 모양이다(결함 아님).

## 3. 도움말 본문 원본화 — 구현만, 실행·화면 확인 못 함 ⚠

원본 캡처 `screenShots/help - NetStorm Instructions.png`(오프셋 (2,33)+정렬 (2,1))와 클론 F1 창을 대조해 찾은 차이를 고쳤다. **빌드(경고 0·오류 0)만 했고 화면·테스트는 돌리지 않았다.**

| 항목 | 원본(잰 값) | 이전 클론 | 이번 변경 |
|---|---|---|---|
| 본문 줄 간격 | 15 (목록 8줄 모두) | 17 | `HelpWindow.LineHeight` 15 |
| 문단 간격 | 줄 간격 + 14 | + 8 | `ParagraphGap` 14 |
| 제목 → 첫 문단 | 35 (= 제목 줄 21 + 14) | 27 | 원본 글꼴 제목 줄 `HeadingRowHeight` 21 (**어림, 미확인**) |
| 첫 문단 → "GAME HELP" | 약 28 | 42 | 줄 끝 공백이 새 줄(빈 줄)을 만들던 **결함 수정** |
| 본문 글자색 | (217, 212, 203), **그림자 없음** | 흰색 + 검은 그림자 | `TextColor`, `OriginalUiSkin.PlainText` |
| 링크 글자색 | (154, 209, 205), **그림자 있음** | (176, 214, 214) | `LinkColor`, 링크·제목만 그림자 |
| `~l … ~.` ("blue") | 링크 색, 그림자 없음 | 색 없음 | 새 `TutorialTextStyle.LinkColor` |
| `<c>` ("drag") | 노란색 (242, 228, 152), 굵지 않음 | 굵은 흰색 | 새 `TutorialTextStyle.ColorText` → `ValueColor` |
| 본문 바탕 | (54, 51, 53) | (52, 49, 51) | 같게 |

- 해석: [`HelpDocument.Parse`](../dotnetpj/src/Netstorm.Assets/HelpDocument.cs) 가 `~l`/`~.` 로 구간을 나눠 `LinkColor` 조각을 만들고 `<c>` 를 `ColorText`, `<b>` 만 `Highlight` 로 둔다. **`Text.Contains("blue ~")` 같은 이전 가정이 깨져** 테스트 `Parse_PreservesLinksPicturesAndMissionCondition` 을 고치고 `Parse_SeparatesBoldColorAndLinkColorStyles` 를 추가했다(**돌리지 않음**).
- 원본 캡처 줄 윗선(스크린샷 좌표): 제목 96, 첫 문단 131, GAME HELP 159, 목록 173·188·203·218·233·248·263·278, GAME SUPPORT 307, 321·336·351.

## 4. 새로 확인했으나 고치지 않은 것

- **실패(Failed) 창 크기 불일치.** 원본 `The War Begins! - Lose 1.png` 의 창은 좌우 테두리 x=358~359·759~760 으로 **폭 약 403**, 본문 시작이 창 왼쪽 + 약 50 이고 본문 4줄 + "Your only chance…" 2줄이다. 클론은 제목 있는 창 기준폭 **351·여백 30** 이라 폭이 작고 줄이 5줄로 감긴다(승리 창은 이전 기록대로 375). **폭을 정하는 원본 규칙은 모른다**(창마다 값이 다름: 브리핑 351, 승리 375, 실패 ≈403, TryAgain 275). 디컴파일(`HJOW-Athlon`)이 필요하다.
- 9-1 A 나머지: 이번에 시작하지 않았다. 이동·전투·건설 등 틱 타이머 전체 이행은 원본의 경로 꼭짓점 시각 규칙(cpppj 에 Pathprocess 미복원)·타이머별 간격 근거가 필요해 이 PC(디컴파일 불가)에서는 근거 없이 진행하지 않았다. **`HJOW-Athlon` 에서 하는 것을 권한다.**
- 9-1 C(`options.cfg`): 저장 위치(게임 폴더 `d/options.cfg` 직접 쓰기 vs 클론 전용 복사본)와 스모크 격리(`NETSTORM_SETTINGS_DIR` 같은 경로 재지정)를 사용자가 정해야 한다. 정하지 않고 구현하지 않았다.

## 5. 검증 상태 요약

| 항목 | 상태 |
|---|---|
| 캠페인 진행 저장 스모크 | 통과(지시 이전) |
| 돌 버튼: buttons·campaign·help_options Menu/Mission 스모크 | 통과(도움말 변경 **이전** 코드) |
| 도움말 변경(3절) 이후 | **컴파일만**(Release 경고 0·오류 0). 단위 테스트·스모크 미실행 |
| `dotnet test` 전체 | 이번 세션 **미실행**(마지막 전체 통과 기록은 Athlon 의 964개) |

테스트 캡처 폴더(`extracted/screens/campaign-progress-20261011`, `button-shape-20261011`)는 정리했다.
