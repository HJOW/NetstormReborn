# webpj ESC 로 Tell 창 닫기 (WB04-E1264, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 찾은 차이를 고쳤다. 미션 중이든 메인 메뉴든 열린 Tell 창(브리핑이 아닌 것)을 ESC 가 닫지 않고 키 사건이 그냥 사라졌다.

## 원문 규칙 (cpppj `UberGump.cpp`)

- `TryTell`(821행)은 창을 열 때 단계를 정한다: 인자 `briefing` 이 참이면 `ClientPhase::Briefing`, 아니면 `ClientPhase::Dialog`. `briefing` 이 참인 호출은 미션 프레임 관문이 여는 안내 창(`frameEffects.tell`, 1164행)뿐이다. 미션 결과 안내(`missionOutcome.section`, 1094행)·메뉴·확인 창은 모두 Dialog 단계다.
- 단추 명령 `Tell,이름` 은 현재 단계를 다음 창에 그대로 넘긴다(2020행 `Tell(action.argument, phase==Briefing)`).
- ESC(1810~1823행): Briefing·LoadingMission 이면 아무것도 하지 않고, 펼침 메뉴가 열려 있으면 그것을 닫고, Dialog 이면 `Post{"Tell","Blank"}` 로 창을 닫는다. 미션 월드 선택 취소·게임 메뉴막대 전환은 단계가 Mission 일 때만이다.

## 웹의 이전 동작과 고친 곳

- 이전: `_onKeyDown` 이 ESC 를 `!this._mainMenu?.dialog` 일 때만 입력 큐에 넣고 입력 루프의 모든 ESC 분기도 같은 보호를 받아, Tell 창이 열린 동안 ESC 는 소비만 됐다(메인 메뉴의 Credits·미션 중 ABORT 확인 창 포함).
- `www/netstorm.js`:
  - `_openTell(name, { suppressWindow, briefing })` 에 `briefing` 을 더했다. 창이 실제로 열렸을 때 그 창 객체(`_tellDialog`)와 단계(`_tellBriefing`)를 기억한다.
  - 프레임 관문의 안내 창(`_prepareCampaignTutorial` 의 `tell`)은 `{ briefing: true }` 로 연다.
  - `_onDialogCommand` 는 닫기 전에 창의 단계를 읽어 `_runDialogCommand(command, briefing)` 로 넘기고, `Tell` 명령이 다음 창에 그 단계를 이어 준다(스커미시 설정 변경 재호출도 같다).
  - 새 `_escapeCloseTell()`: 열린 창이 `_openTell` 이 연 Tell 창이고 브리핑이 아니며 펼침 메뉴가 없으면 `Tell,Blank` 단추와 같은 `_onDialogCommand` 로 닫는다(창 종료 명령 OnExit·Reset All 확인 해제도 그대로 따른다). `_onKeyDown` 맨 앞(시작 그림 다음)에서 반복되지 않은 ESC 를 가로채며 월드 입력 큐로는 보내지 않는다.
- 대상이 아닌 창은 그대로다: 미션 로딩 창(Tell 이 아니다), F1 도움말·지식 창·Send X 입력 창의 기존 ESC 경로, 펼침 메뉴 닫기(메인 메뉴).

## 검증

- 새 e2e `e2e/tell-escape.spec.js` 3건 통과(Chromium 4:3): ① 메인 메뉴 Credits 창이 ESC 로 닫힌다 ② Death Bridge 시작 브리핑(`Technology available`/`Lets do this!`)은 `_tellBriefing=true` 이고 ESC 를 무시하며, 미션 중 ABORT 확인 창은 Dialog 단계로 ESC 에 닫히고 시계가 계속 흐르며 오류가 없다 ③ 실패 안내(`Training Menu`/`Restart`/`Leave Missions`)는 Dialog 단계라 ESC 로 닫히고, 이어지는 ESC 는 게임 메뉴막대를 연다.
- 변형 확인: ESC 가로채기를 꺼 보면 ①이 실패하고, 브리핑 구분(`_tellBriefing`)을 빼 보면 ②가 "ESC 뒤에도 브리핑"에서 실패한다. 원복 후 통과를 다시 확인했다.
- 관련 e2e 묶음(`mission-priest-training-flow`·`game-menu-bar`·`reset-all`·`first-run`·`mission-object-commands`) 24건 통과(로그 `extracted/webpj-e1264-e2e.log`), `netstorm.js` 를 쓰는 Node 시험 25개 파일 285건 통과.

## 남은 확인

- 원본 게임에서 실제로 ESC 가 Credits·결과 안내를 닫는지는 원본 실행으로 확인하지 못했다(cpppj 복원 코드 기준). 결과 안내를 ESC 로 닫으면 단추 선택 없이 미션 화면으로 돌아가며, 이 동작도 cpppj 와 같다.
