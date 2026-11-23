# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 [E1237 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)과 그 아래 날짜별 보관본을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → 3.1(단계별 항목) → 0.5(단위 착수·종료 절차) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 칸으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-23 정리, 마지막 단위 WB04-E1283)

| 항목 | 현행 상태 |
| --- | --- |
| 실행·플레이 | 브라우저에서 ‘클릭하여 시작’, 시작 title 그림, 메인 메뉴·캠페인/데모 목록·도움말·옵션·Credits와 개발용 지형 미리보기가 동작한다. **미션:** 캠페인 목록 31개(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)와 데모 3개가 시작되고, 장시간 진행과 임의 조작(몽키) 시험에서 오류·먹통이 없다. Campaign ▸ Skirmish는 무작위 섬·간헐천 월드에서 AI와 진행된다. Tutorial2는 시작부터 완료 창·Tutorial3 시작까지 자연 진행으로 확인했고, 승패 안내 뒤 흐름(Continue·Replay Mission·Leave/Next Mission·진행 저장)도 확인했다. 미션 프레임에서 다리 붕괴 검사·전투 자원(간헐천 보충·스펠 소비·SP 주기 지급)·제단 결과 알림·주변음/공유 바람이 원본 주 루프 순서로 돈다(E1230~E1234). **조작:** 사제 선택·이동, 가장자리·Alt·가운데 버튼 스크롤, 미니맵 클릭·끌기, 생산 격자 클릭·단축키(D 포함)·조각 회전(C·회전 방향 옵션)·다리 배치, 사제·워크샾·제단·신전·배터리·발전기·운반체 우클릭 메뉴와 회수·기도·Cast/Drop, 객체 명령(Detonate·Meltdown·신전 명령), 화면 위치 키(F4/H·F5·TAB·U·Shift+숫자 저장/숫자 복귀), 미니맵·스톰 파워 표시·생산 창의 우클릭 빠른 도움말(E1236)이 동작한다. **게임 메뉴:** ESC 또는 화면 우측 상단 버튼(웹 전용)으로 Game·View·Options·Players·About 메뉴막대를 열고, Leave/Restart Mission(ABORT 확인 창), F1·F2·F6·F7·F8·F9·Shift-F3·T, Pause(Shift-F9·Pause 키·메뉴), Shift-F7 화면 GIF 저장, Players 메뉴의 송금(Send X 숫자 입력 창 포함)·동맹이 실행된다. 브리핑·완료 창의 Review Knowledge·GetTechnology 단추도 동작한다(E1227). **HUD 틀:** 실제 Uber 루트가 세로·미니맵·채팅 구분선, 좌측 사이드바(SP·생산 덱)와 하단 미니맵 칸을 원본 위치로 도킹하고 지도 영역이 `{85,0,W,H}`다(4:3 하단 85×82픽셀이 원본 스크린샷과 일치, 16:9·16:10·1920×1080과 창 크기 변경 확인). **채팅 창:** F3로 채팅 영역이 보이고 로그 줄이 색과 함께 그려지며, Enter로 입력줄을 열어 제출하면 로컬 서버를 거쳐 `You> 본문` 줄이 로그에 나오고 점 명령·Tab 이름 완성(동맹 AI)·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창이 동작한다(E1169~E1179). **미완성:** 읽기 전용 감사가 남긴 차이([다음 작업 1](#next); 승리/패배 곡은 E1238에서 단독 미션 도달 불가로 닫음), 나머지 객체 메뉴(Factory 파생·특수 대상·채팅 분기)와 운반체 줍기·내려놓기, 다른 미션의 목표 수행과 자연 완주. 지식 NSInfo 패널·Players 하위의 Ignore chat·Whisper To·정보 보기·의식 보상 선택 UI·인트로 영상은 웹 범위 밖이다(E1167·E1171·E1180·E1202). |
| 원본과의 동일성 | 영어 4:3 메뉴 M1 8장면은 cpppj 오라클과 알려진 구름 배경 등 차이 영역 밖에서 픽셀 차이 0이다. 월드·UI 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다(최근 원본 공통 메뉴 788행·Factory 메뉴 39행·Uber 루트 160행·장면 음악 3,113행 등). 좌측 HUD 틀의 하단(미니맵 구분선·세로 구분선 아랫부분·미니맵 칸) 85×82픽셀은 원본 스크린샷과 완전히 일치한다. 메뉴 비활성 색·안내 창 위치·지식 카드 이름 줄 나눔·Wind Noise 무동작은 원본 기준으로 맞췄다. Tutorial1 시작 직후 Session 덤프는 시간 의존 값을 빼고 cpppj 기준선과 구조가 같다(E1129). 고정 시각 L2·L3·L4 화면 비교와 자연 완주는 미검증이다. |
| 현재 작업 | **게임 플레이 중 UI 구현 단계.** 4-D 후반(GameWorld·Client 서비스)과 5-A·5-B·6·8단계 항목을 미션 앱 프레임 위에서 병행한다. Renderer·Gump 트리·입력 큐·생산 패널·객체 메뉴·도움말·음향·채팅 창이 실제 미션 화면에 연결돼 있다. 최근 단위(E1167~E1237)는 웹 범위 판정, Native 대응 행 대조로 partial 행 닫기, 읽기 전용 감사(E1223~E1231)가 찾은 원본과 다른 동작의 수정으로 빈칸을 메웠다. 순서는 아래 [다음 작업](#next)을 따른다. |
| 전체 회귀 | **Node 26.13.0:** E1282 기준 전체 6,801건 중 6,799건 통과(로그 `webpj/extracted/webpj-e1282-node26.log`, 615초). 실패 2건(`campaign-request-frame`·`raw-campaign-session-load`)은 월드 진입 정지(E1281)를 가정하지 않은 옛 시험 가정이었고 정지를 푸는 단계를 더해 고친 뒤 통과했다. 직전 E1278 실행은 6,801건 모두 통과(로그 `extracted/webpj-e1278-node26.log`, 551초; 직전 E1274 실행은 회귀 도중 추가한 새 시험이 manifest 에 등록되기 전에 fixture 등록 검사가 돈 1건만 실패했고 E1275 에서 등록한 뒤 통과했다). **Node 22.23.3:** E1276 기준 전체 6,801건 모두 통과(로그 `extracted/webpj-e1276-node22.log`, 597초). **e2e:** E1147 전체 `npx playwright test` 100건 통과·8건 건너뜀(옵트인 장시간, 로그 `extracted/webpj-e1147-e2e-all.log`, 39분). 이후 단위는 새 e2e와 관련 e2e를 단위마다 돌렸다. **HJOW-Athlon 주기(0.5절 5):** v26 1/4(E1282에서 실행, 이후 E1283)·v22 7/8(E1276에서 실행, 이후 E1277~E1283) — v26은 E1286 완료 뒤, v22는 E1284 완료 뒤이며 겹치면 순차로 돈다. E1127~E1236의 회귀 이력(실패와 교정 포함)은 [E1166 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)과 [E1237 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에 있다. |
| 브라우저 검사 | 미션 화면은 단위마다 Chromium e2e로 확인한다(최근 전체 실행 E1147, 16:9·16:10 건설·메뉴 E1143~E1144, 한국어 Players 메뉴 E1166). 메뉴는 2026-11-17 Chromium 2/2(갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체), 최초 실행·진행 잠금/체크·저장 재실행은 E616 Chromium 2/2. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md) |

<a id="next"></a>

#### 다음 작업

1. **읽기 전용 감사(E1228·E1230·E1231)가 남긴 차이(우선순위순).** 근거는 PORT_MAP 해당 행의 E1228·E1230·E1231 노트에 있다.
   - ① (E1238 닫음) 승리/패배 곡(fanfare/defeat)은 단독 미션 복귀가 결과 DWORD 를 0 으로 쓰고 State 3 이 곡을 요청하지 않아 원본도 틀지 않는다. 전투 종료 프레임은 `!missionPost` 관문이라 멀티플레이 전용이다. [판정](docs/webpj-result-music-scope-20261123.md)
   - ② (E1240 닫음) 월드 없는 메인 메뉴 Help의 TYPE 정보 — `ReadAssetQuickHelpType`를 `readAssetQuickHelpType`로 이식하고 `_helpTypeSource`가 월드 유무로 출처를 고른다. 미션 중 링크 이동·Back의 TYPE 정보 패널·유닛 그림은 E1229, 첫 주제 Back의 직전 팝업(F6 지식 창 등) 재열기는 E1237에서 해결했다. [근거](docs/webpj-main-menu-help-type-20261123.md)
   - ③ (E1239 닫음) "Sound On"을 끄면 원문은 장치를 닫아 음악도 무음이고 켜면 현재 곡을 처음부터 재생한다 — `AudioSystem.setSoundEnabled`가 같게 동작한다(원본 실기 청취 미확인). [근거](docs/webpj-sound-off-music-thunder-20261123.md)
   - ④ (E1239 닫음) 천둥 효과음은 이제 `SoundPlayer.playByName`을 거쳐 게임 메뉴 음소거·동시 재생 한도가 적용된다.
   - ⑤ (E1260 닫음) 한국어 큰 슬롯(제목)이 `라`→`다`, `잔`→`산` 처럼 읽히던 문제를 고쳤다(E1259). 원인은 비정수 배율(19·45px)만이 아니라 **글자 윗부분이 줄 맨 위 밖으로 나가 잘리는 것**(기준선이 원본 Arial 값)이어서 크기를 원문 정수 배율(슬롯 3 22px·슬롯 4 44px)로 하고 큰 슬롯 기준선을 글자 윗부분이 안 잘리게(20·41줄) 올렸다. Credits 목록 창 제목(`NetStorm 크레딧`)·좁은 캠페인 목록 소개 문구가 창 폭 규칙 때문에 잘리던 문제는 E1260 에서 고쳤다(`TellWindow.fit()` 이 맞춘 본문 폭이 가장 긴 낱말보다 좁으면 넓힘, 영어 창 크기 불변). **굵기(원문 Bold 고정)는 사용자 결정(2026-11-23 "지금은 그대로")으로 지금의 보통체를 유지한다.** 알려진 남은 문제: 슬롯 1·6 의 작은 한글 윗부분 1줄 잘림. [근거](docs/webpj-korean-title-legibility-20261123.md). 같은 행의 굵은 글꼴이 갖지 않은 문자(호환 자모 42자·한자·가나 등 8,271자)를 일반 갈무리11로 대체하는 규칙은 E1244에서 반영했다(보조 FontFace, [근거](docs/webpj-galmuri-bold-fallback-20261123.md)).
   - ⑥ (E1252 닫음) Tell 모달 스택·제목 사건 4개·메뉴 request/closed 사건은 도달성 판정으로 닫았다: 결과 안내가 프레임 관문(dialogsClear)을 거쳐 열린 창이 있으면 미뤄지므로 두 창이 동시에 필요한 순간이 없고(e2e: ABORT 확인 창이 열린 채 실패 판정이 나도 덮어쓰지 않고 닫은 뒤 이어서 열림), 나머지 사건은 웹에서 도달하지 않거나 영향이 없다. [근거](docs/webpj-menu-controls-close-20261123.md). `ownerMode.battle`이 이탈 뒤 true로 남는 것은 미션 밖 소비자가 없어 영향을 못 찾았다.
   - ⑧ (E1251 닫음, 사용자 결정 "바꿔 주세요") Reset All 확인을 브라우저 `confirm()` 에서 cpppj 1325 와 같은 게임 안 Tell 확인 창(`Patch1101.ResetAll`, 확정 `Patch1101.ResetAllConfirmed` 는 창이 열린 직후 한 번만)으로 바꿨다. 일본어 본문(13차)과 게임 안 ESC 메뉴 행은 남김. [근거](docs/webpj-reset-all-tell-20261123.md)
   - ⑨ (E1246 발견, E1247 판정) 스톰 파워 표시(왼쪽 위 예산 칸, 노드 (7,2) 66×21)의 **실제 마우스 우클릭**이 노드에 닿지 않는다(`input().route(0x20000, 40, 12)` 가 0). cpppj 대조 결과 원본 판정 논리와 같아 **웹 결함이 아닐 가능성이 높다**: 우클릭은 `baseFilter` 가 HELP 플래그(0x200)가 있는 노드의 경계 안에서만 전달하는데, 예산 텍스트 노드는 `TextGump::Construct`(flags=VISIBLE|VOID+DrawClip)로 HELP 플래그가 없고 cpppj 의 `SetHelpText`/`SetNodeHelp` 도 플래그를 바꾸지 않는다(HELP 가 켜지는 곳은 생산 루트 `flags|=0xe00` 과 격자·배경). 루트 핸들러는 `0x02000008` 등만 처리해 우클릭에는 0 을 돌려준다. 남은 것: 원본 실행 파일로 실제 우클릭이 팝업을 여는지 확인(AGENTS.md 실행 규칙 적용) — 열면 웹이 막는 지점을 찾고, 안 열면 닫는다. [근거](docs/webpj-korean-production-chat-xlat-20261123.md#budget-rightclick)
   - ⑦ (E1245 닫음) Priest Training 7 완료 안내의 "Training Menu" 단추가 원본 오타대로 `MissionBegin,BC1Menu`(76 접미 없음)라 웹에서 아무 일도 없던 것을 사용자 결정으로 `bc1menu76` 에 연결했다(`core/mission-name-alias.js`, 원본은 10.72판 loose `BC1Menu.english` 를 읽고 그 사슬(`BC2Bridging` 등)은 웹에 없다). [근거](docs/webpj-priest-training-menu-alias-20261123.md). ESC로 Tell 창을 닫는지는 E1264 에서 고쳤다(아래 ⑩). Early Mission 3 단계 A·6 단계 B의 너겟 납품(`0x020000b4`) 진행은 자연 완주로 확인하지 못했다.
   - ⑩ (E1263 점검이 발견, E1264 닫음) 미션 중·메인 메뉴에서 열린 Tell 창(브리핑·로딩 제외)을 ESC 가 닫지 않던 것을 고쳤다: `_openTell` 이 창의 Briefing/Dialog 단계를 기억하고(`_tellDialog`·`_tellBriefing`, 프레임 관문이 여는 안내 창만 Briefing, `Tell` 명령이 단계를 다음 창에 이어 줌), `_onKeyDown` 맨 앞의 `_escapeCloseTell()` 이 Dialog 단계의 Tell 창을 `Tell,Blank` 와 같은 길로 닫는다. 새 e2e `e2e/tell-escape.spec.js` 3건과 변형 확인, [근거](docs/webpj-tell-escape-20261123.md). 발견 당시의 설명: 미션 중 열린 Tell 창(브리핑·로딩 제외)을 ESC 가 닫지 않는다. 원문 `UberGump.cpp:821` 은 브리핑이 아닌 모든 Tell 을 열 때 `phase=Dialog` 로 두고 `:1810~1823` 은 Briefing/LoadingMission 이면 무시, Dialog 면 `Post{"Tell","Blank"}` 로 창을 닫는다. 웹은 `netstorm.js` 의 ESC 분기(2073행 등)가 모두 `!this._mainMenu?.dialog` 보호를 받고 3639행은 메인 메뉴 팝업만 닫아 미션 중 열린 Tell(게임 메뉴 About·F1·확인 창 등)에서 ESC 사건이 소비만 된다. 수정 위치는 입력 루프의 `cursorCancel` 분기이며 실제 e2e 로 확인한다(코드 대조만 한 판정, 확신 중간). [점검 기록](docs/webpj-left-jobs-audit-20261123.md)
   - 해결한 것(E1229~E1237: 도움말 TYPE·다리 붕괴·통지 명령 입력 막힘과 MissionAbort 확인 창·전투 자원·Player 결과 알림·주변음/바람·F1·HUD 우클릭 도움말·Back)은 0.1 요약표의 '읽기 전용 감사·수정' 행과 [RawSurface 감사 문서](docs/webpj-native-surface-audit-20261122.md)에 있다. 남은 확인: 주변음 실제 청취, 채팅 창 "Chat Window" 실제 우클릭.
2. **다른 미션의 목표 수행.** Tutorial2([E1086](docs/webpj-tutorial2-complete-20261120.md))처럼 Tutorial1·3~6과 세 캠페인 묶음 미션의 단계별 목표를 수행해 승리·완료 창까지 이어 보고, 남는 객체 메뉴(Factory 파생·특수 대상·채팅 분기, Archer 실제 우클릭·운반체 Carrying/Drop)와 입력 공백을 메운다. 승패 안내 뒤 흐름은 [E1152](docs/webpj-mission-outcome-flow-20261121.md)·[E1161](docs/webpj-idle-outcome-20261121.md)에서 확인했다. 앱에 연결되지 않은 나머지 모듈은 [E1163 분류표](docs/webpj-pause-key-unwired-modules-20261121.md)에 이어 사용자 기능과 직결된 것부터 확인한다(3.8절 입력·장치). 자연 완주는 1절 사용자 확인 범위다.
3. **나머지 번역.** 게임 안 메뉴·객체 우클릭·지식 창의 번역 누락은 없다(E1166 수집, 소스 기준 누락은 개발자 치트 `CHEAT Set Owner`뿐). 생산 창·스톰 파워 도움말과 채팅 채널 이름은 번역 호출이 이미 있었고 번역표 항목만 없어 영어로 남았던 것이라 항목을 더해 닫았다(E1246, 미니맵 도움말 `Sky Overview` 는 원본이 번역 호출 없이 리터럴을 넣어 영어 유지가 원본과 같음). 번역 호출이 배열·상수에 들어 있어 놓쳤던 미션 화면 문구 15개(커서 배치 경고 5·생산 힌트 2·객체 정보 2·Players 메뉴 Send 5·도움말 없음 안내 1)는 번역표에 더하고 도움말 없음 안내는 원본처럼 번역 장치를 거치게 고쳤다(E1249, 나머지 후보의 판정은 [근거](docs/webpj-korean-gameplay-xlat-20261123.md)). 채팅 도우미(Chat Helper) 메뉴의 고정 라벨 59개도 원본은 번역 호출로 받는데 두 한국어 표에 항목이 없어 영어였으므로 더했다(E1250, 범위 라벨 `Colors 0 to 21` 등은 이어 붙인 문자열이라 영어로 남는 원본 동작, [근거](docs/webpj-korean-chat-helper-xlat-20261123.md)). 한국어 미션의 실제 화면 확인(대본 본문 줄 나눔 등)이 남았다(8단계·3.7절 6차). [게임 안 문구](docs/webpj-korean-ingame-xlat-20261120.md) · [생산·채팅 항목](docs/webpj-korean-production-chat-xlat-20261123.md) · [미션 화면 15개](docs/webpj-korean-gameplay-xlat-20261123.md) · [채팅 도우미 59개](docs/webpj-korean-chat-helper-xlat-20261123.md) · [한국어 이름](docs/webpj-korean-player-names-20261121.md)
4. **4-D 완료 기준 L2.** Tutorial1 Load/Start의 덤프·첫 프레임 논리 화면 대조(4D-10b)는 E1283 에서 닫았다(영어 4:3·16:9, 90·130프레임, [난수·시각 맞춤](docs/webpj-state-install-rng-20261123.md)). 남은 4-D 체크는 4D-4b(Tutorial1·3~6 자연 진행, 사용자 범위)이다.
5. **12차 스커미시 나머지.** 설정·화면·시작·월드 생성·금지 유닛과 난이도별 건설 속도는 E1101~E1107·E1149에서 연결·확인했다(3.7절 12차). E1262 에서 실제 시간 관찰(옵트인 `e2e/skirmish-watch.spec.js`)로 AI 생산(다리·섬·사제 건물·공장·워커·배터리·비행체·아처)과 AI 공격으로 플레이어 사제가 죽는 것(어려움 4명 혼전, 약 18분), 패배 안내(`Defeat`·`Leave` → Abort 확인)를 확인했고 오류는 0건이었다([관찰](docs/webpj-skirmish-watch-20261123.md)). 남은 것: 승리 경로(적 전멸)와 개별 난이도/동맹의 N프레임 동작의 정량 비교, AI 가 다리를 멈추는 시점(관찰에서 소유자별 360~600초)의 원본 대조(자연 완주는 1절 사용자 확인 범위). 비미션 너겟 보충(4D-1c)은 스커미시가 미션 모드라 도달하지 않음을 E1270 에서 확인했다. [게임 진행](docs/webpj-skirmish-play-20261120.md) · [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)
6. **PORT_MAP partial 12행의 낡은 메모 정리와 작은 코드 잔여.** (E1263 점검, [점검 기록](docs/webpj-left-jobs-audit-20261123.md)) 메모가 E981·E1224·E1241 이후 해결분을 반영하지 않은 행: ClientRendererFrame("자동 프레임과 실제 HUD는 남음", 실제 남은 것은 메인 메뉴 Kencloud 연결), RawButtonControls(E1236~E1253 미반영), UberGump(E1245 BC1Menu·E1251 Reset All Tell 미반영), RawMenuChat, ClientCampaignTransport("InstallNative…" 문구는 E1193~E1194 로 낡음; 실제 남은 것은 앱에 Client 기본 수신기 표 미설치), ClientState 네 행·ClientUserSelectionCleanup(E1268 에서 port 로 올림), CampaignObjectMenuClient("다른 객체 메뉴는 남음"은 낡음 — 남은 것은 Archer 우클릭·운반체 Carrying/Drop 실제 확인뿐이라 확인 뒤 port 로 올림), UberReconnect·ClientMain(하트비트 문구 불일치: 단독은 하트비트 외 관측 효과 없음으로 확정). 작은 코드 잔여: ① (E1264 해소) 미션 중 Tell 을 ESC 가 닫지 않음(위 ⑩), ② (E1269 해소) `raw-priest-placement-*` 미연결의 도달성(3.8절 입력·장치), ③ (E1274 해소) Client 기본 수신기 표의 앱 설치 판정(4D-3b), ④ (E1267 해소) Session 큐 명령 코드와 입력 루프 허용 목록의 전수 대조(4D-3e), ⑤ (E1265 해소) 채팅 허브 fixture 두 TSV(`chat-callbacks-x86.tsv`·`chat-input-recipients-x86.tsv`)의 manifest 소비 시험(5-C 허브 fixture), ⑥ E197·E198·E201 재대조(5-A), ⑦ (E1266 해소) 프레임 순서 차이 두 건(4D-4a), ⑧ 채팅 로그 미션 간 초기화를 적응으로 인정할지(사용자 결정).

- **로컬 산출물.** `webpj/test-results-e1072-rerun`~`webpj/test-results-e1081`의 10개 폴더는 빈 폴더이거나 Git 제외 `*.zip` trace만 들어 있어 Git 상태에 나타나지 않는다(2026-11-23 정리 때도 남아 있음). 다시 볼 필요가 없으면 삭제해도 된다. E633~E640 단위 문서가 적은 루트 로그는 2026-11-21에 `extracted/webpj-e633-e640-root-logs/`로 옮겼으므로 그 폴더에서 읽는다.
- **미션 식별.** 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이고, 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이다(E753 재대조). [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md)

<a id="e951-e1081"></a>

#### E951~E1237 진행 요약

단위별 완료 문단·검증 수·회귀 이력은 [E1081 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081)의 0.1절(E951~E1081), [E1166 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)의 0.1·3.1·3.7절(E1082~E1166), [E1237 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)의 0.1·3.1절(E1167~E1237)과 각 단위 문서에 있다.

| 주제 | 단위 | 연결한 것 | 남은 것 | 대표 문서 |
| --- | --- | --- | --- | --- |
| 미션 로딩·Start | E951~E958, E968, E970, E974~E976 | 로그인 HUD·진행 창, 최초 Fort 요청 자동 처리, Ready→Start→첫 안내와 시계 정지/재개, 공유 Kernel 프레임 | 원본 Gump serial | [로딩](docs/webpj-app-mission-loading-20261119.md) · [Start](docs/webpj-app-tutorial-start-frame-20261120.md) |
| 음향·음악 | E957, E959~E966, E997, E1059, E1061, E1119~E1120, E1153 | 효과음·음악 적재와 Web Audio 재생, 위치 음향·발소리, 장면 음악(미션 중 순환)·결과 음악·희생 의식 곡, Wind Noise 판정 | 대기실 곡, 실제 청취(승리/패배 곡 요청은 단독 미션 도달 불가, E1238) | [앱 음향](docs/webpj-app-campaign-audio-20261119.md) · [장면 음악 구동기](docs/webpj-scene-music-driver-20261120.md) |
| AI·채팅·Tell·승패 | E967, E969, E971~E973, E1058 | AI 이름·난이도·채팅 기록, 미션 Tell 조회, Gump 팝업 차단·닫기, 승패 안내, NSInfo·도움말 팝업 저장소 | 채팅 창 공장, 실제 State 정리·World 교체 | [AI](docs/webpj-app-campaign-ai-20261120.md) · [승패](docs/webpj-app-mission-outcome-20261120.md) |
| Renderer·화면 | E977~E989, E1081~E1082 | 지면·객체·입자·커서·Gump 수집과 출력, Screen DIB·구름 합성·Present, Gump 이동 dirty, 도킹된 지도 영역 | 지식·채팅 HUD 공장, Kencloud·구름 차이 | [출력](docs/webpj-campaign-renderer-drawing-20261120.md) · [Present](docs/webpj-app-renderer-present-20261120.md) |
| 미션 진입 | E985~E986, E1010~E1016, E1092 | Tutorial1~6·세 캠페인 묶음 진입·브리핑, 세 데모 시작·45초 Auto-Demo | 미션별 목표 수행·자연 완주 | [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md) · [데모](docs/webpj-demo-start-20261120.md) |
| 입력·카메라 | E990~E1001, E1009, E1084 | 선택·이동·포인터 폴링, 가장자리 스크롤·카메라 키(F4·P/R·F5·N), 조각 회전, ESC 메뉴 토글 | 여러 유닛 순환(Ctrl-F5·N은 사제 1명 상태로 확인) | [포인터](docs/webpj-app-mission-pointer-frame-20261120.md) · [사제 키](docs/webpj-browser-priest-shortcuts-20261120.md) |
| 생산·건설 | E1002~E1008, E1019~E1022, E1026~E1036, E1047, E1074, E1076~E1079, E1083, E1086 | 생산 패널·단축키·다리 배치, 회수·비용 표시, Factory 생산·업그레이드, Tutorial2 발사기 배치·회수 | 연속 배치·자연 완주 | [생산 패널](docs/webpj-client-production-panel-20261120.md) · [워크샾 생산](docs/webpj-browser-tutorial2-factory-20261120.md) |
| HUD 틀 | E1082, E1128, E1236 | Uber 루트 HUD 모드 3 도킹·화면 크기 변경 재배치, 미니맵 Gump와 클릭·끌기 e2e, HUD 우클릭 빠른 도움말 | InstallHud 재구성·배경 GIF 등 표시 차이(E1224 감사; 지식 NSInfo 공장은 웹 범위 밖, E1171) | [좌측 HUD 틀](docs/webpj-uber-hud-left-dock-20261120.md) |
| 채팅 창 | E1169~E1179 | F3 표시·로그·입력줄·제출·수신·점 명령, Tab 이름 완성·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창 | Format 입력창 아래 가로 슬라이더의 원본 화면 대조 | [채팅 조립](docs/webpj-chat-window-assembly-20261122.md) |
| 게임 메뉴 | E1084~E1085, E1087~E1090, E1093~E1097, E1100, E1167~E1168, E1235 | ESC 메뉴막대·우측 상단 버튼, Leave/Restart, F6·F7·F9·T, Pause, Frame Rate·Language·Difficulty, Shift-F7 GIF, Players 하위 항목 실측(채팅·정보 항목은 단독 미션 도달 불가), Send X 숫자 입력 창(모달·글자 입력창), F1과 Tell/브리핑 창 | Tell 모달 스택·ESC Tell 닫기 확인 | [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md) · [Send X 창](docs/webpj-send-money-dialog-20261122.md) |
| 스커미시(12차) | E1101~E1107 | 설정 화면·저장, 시작 섬·섬 생성 부모, 세션 월드 생성·앱 시작, AI 진행·금지 유닛 | AI 생산·공격, 실제 전투 승패·너겟 | [시작과 월드 생성](docs/webpj-skirmish-start-20261120.md) · [게임 진행](docs/webpj-skirmish-play-20261120.md) |
| Gump·메뉴 UI | E1017~E1018, E1023~E1025, E1037~E1045, E1064~E1072, E1075, E1086 | Form 통지, 글자·Slider 출력, 중첩 메뉴, 사제·Player·공통 객체·운반체 메뉴, 신전 건설·완공 | 파생 +ac 객체 메뉴, 채팅 분기 | [Client 메뉴](docs/webpj-client-menu-gump-20261120.md) · [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) |
| 도움말 | E1046, E1048~E1055, E1229, E1240 | 생산 Help, TYPE 정보 창, 요구 에너지·도움말 그림, 링크 이동·월드 없는 메인 메뉴 Help 의 TYPE 정보 | 타입별 원본 화면 비교 | [앱 TYPE](docs/webpj-app-help-type-info-20261120.md) |
| 한국어 | E1056~E1057, E1062~E1063, E1073, E1080, E1108~E1118, E1140~E1142, E1166, E1244, E1246, E1249, E1250 | 모든 미션 대본, 게임 안 메뉴·객체 메뉴·지식 문구, 지식 카드 줄 나눔, Player 이름 디코딩, 굵은 글꼴 부족 문자의 일반 글리프 대체, 생산 창·스톰 파워 도움말·채팅 채널 이름 항목, 커서 경고·생산 힌트·객체 정보·Send 줄·도움말 없음 안내 번역, 채팅 도우미 메뉴 라벨 | 실제 화면 확인(미션 본문 줄 나눔 등), 사람 검수 | [캠페인 미션](docs/webpj-korean-campaign-missions-20261120.md) · [한국어 이름](docs/webpj-korean-player-names-20261121.md) |
| 조작키 | E1145~E1146, E1148, E1151, E1154~E1155, E1159, E1162~E1163 | Alt·가운데 버튼 스크롤, H, C·`rotmode`, TAB·U·Shift+숫자/숫자, D·F 키 점검, F3 구분선, Pause 키 | — | [Alt 스크롤·조작키](docs/webpj-alt-scroll-keys-20261121.md) · [화면 위치 키](docs/webpj-camera-keys-20261121.md) |
| 객체 메뉴·명령 | E1126~E1127, E1135~E1138, E1150, E1225 | Players 송금·동맹, 파생 객체·제단·운반체·Archer 메뉴, 메뉴 닫기·비활성 색, 객체 명령·정체 안전장치, 사제 기도·Cast/Drop·Factory 순환 명령 소비 | Factory 파생·특수 대상·채팅 분기, Archer·운반체 항목 확인 | [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [객체 명령](docs/webpj-object-commands-20261121.md) |
| 장시간·실사용 점검 | E1121~E1125, E1128, E1132~E1134, E1139, E1143~E1144, E1147, E1158, E1160, E1165 | 전 미션 시작·장시간 진행, 몽키, 시계 정지 누수·죽은 Gump 정리 수정, 실사용·넓은 화면 e2e, 성능 측정 | 자연 완주 | [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md) |
| 승패·L2·미션 흐름 | E1129~E1131, E1152, E1157, E1161 | 시작 덤프 구조 대조, 안내 창 배치, 승패 뒤 흐름·진행 저장, Priest Training 시작 연결 | 고정 시각 L2·L3, 실제 사제 처치 패배 | [L2 대조](docs/webpj-l2-tutorial1-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) |
| 분석·판정 | E1149, E1153, E1156, E1163, E1241 | 난이도별 건설 속도, Wind Noise, 빨리 감기 한계, 미연결 모듈 68개 분류, cpppj 1322·1325·1326 변경 아홉 행 판정(②는 파생 피해 진단 문구 하나) | 쉬움 채취 절반 실관찰, 미판정 모듈 | [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md) · [미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md) · [1322·1325·1326 판정](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md) |
| 회귀 교정 | E1060, E1078 | 관측 자료 누락·메뉴 전용 번역 검사 범위, 외부 접속 감사의 비공개 필드 오탐 | — | [회귀 교정](docs/webpj-regression-translation-fixtures-20261120.md) · [접속 감사](docs/webpj-external-audit-private-field-20261120.md) |
| 웹 범위 판정 | E1167, E1171, E1176, E1180, E1182, E1187~E1188, E1192, E1202~E1204 | Players 하위 3항목·지식 NSInfo·의식 보상 선택 UI·HTML `$Input=`·패킷 원천/지연·메타 앞단·Sprite 갱신·FortCoreDataLoad를 웹 범위 밖으로, 인트로 영상은 사용자 결정으로 웹 제외 | — | [NSInfo](docs/webpj-nsinfo-scope-20261122.md) · [동영상 결정](docs/webpj-intro-movie-decision-20261122.md) |
| 대본·기술·타이틀 | E1184, E1189~E1190, E1197, E1199~E1200, E1227 | 시작 title 그림, Fury War 기술 지급, 대본 82개 명령 점검(`ViewSpot`·`Salvage`), 도움말·Tell 남은 두 행, Detonate·GetTechnology 원본 관측, Review Knowledge·GetTechnology 단추 | — | [대본 명령](docs/webpj-script-commands-audit-20261122.md) · [Fury War](docs/webpj-fury-war-technology-20261122.md) |
| Native 대응 대조 | E1193~E1195, E1205~E1221 | Native 고정 수신기 네 묶음·채팅 수신기, 건설 여섯 행·AssetDeletionVtables, RawSurface 훅 대조와 표시 후 계속 진단·17종 가상 삭제·unit-lost 억제 | RawSurfaceDeletion·Lifetime(DestroyPatch 진단 열두 지점) | [수신기](docs/webpj-native-receivers-audit-20261122.md) · [RawSurface](docs/webpj-native-surface-audit-20261122.md) |
| 읽기 전용 감사·수정 | E1223~E1262 | Client 상태·입력(제단 희생 메뉴 훅)·UI 컨트롤(와이드 메인 메뉴 버튼) 감사, 객체 메뉴 명령 소비, 아이스 반사 0점 기본값, 도움말·음향·설정 partial 15행 닫기와 Tell Warning, 다리 붕괴·전투 자원·Player 결과 알림·주변음/바람 프레임, F1·HUD 우클릭 도움말·Back, 승리/패배 곡 범위 판정(E1238)·소리 끄기 음악 무음·천둥 효과음 경로(E1239)·월드 없는 Help TYPE 정보(E1240)·건설 기록 재생 개발자 전용 판정(E1242)·Uber 루트 배경 자식 판정(E1243)·Priest Training 7 Training Menu 원본 오타 별칭(E1245)·생산 창·채팅 채널 번역 항목(E1246)·예산 칸 우클릭이 원본 판정 논리와 같음 판정(E1247) | [다음 작업 1](#next)의 남은 차이 | [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md) |

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분은 중복 가산하지 않는다. 남은 단위와 줄 수는 `PORT_MAP.tsv`의 partial·todo 기준이며 partial에는 파일 전체 줄 수를 센다. 2026-11-23 정리 때 `node webpj/tools/port-map.mjs`와 `python -X utf8 tools/port_inventory.py --tsv`로 다시 세어 **partial 34·todo 0, 34단위 / 20,876줄**이었고 E1240 에서 QuickHelpHtml·QuickHelpRequest 두 행을 닫아 **partial 32·todo 0, 32단위 / 20,635줄**이었고 E1242 에서 RawBuildReplay·ClientBuildReplay 두 행을 개발자 전용으로 닫아 **partial 30·todo 0, 30단위 / 20,436줄**이었고 E1243 에서 RawUberRootControls 행을 닫아 **partial 29·todo 0, 29단위 / 20,275줄**이었고 E1248 에서 RawMinimapControls 행을 닫아 **partial 28·todo 0, 28단위 / 20,094줄**이었고 E1252 에서 RawMenuControls 행을 닫아 **partial 27·todo 0, 27단위 / 18,489줄**이었고 E1255 에서 UberHud 행을 닫아 **partial 26·todo 0, 26단위 / 18,333줄**이었고 E1256 에서 CampaignCursorClientEffects 행을 닫아 **partial 25·todo 0, 25단위 / 18,259줄**이었고 E1257 에서 RawProductionControls 행을 닫아 **partial 24·todo 0, 24단위 / 17,866줄**이었고 E1258 에서 SoundSceneMusic 행을 닫아 **partial 23·todo 0, 23단위 / 17,571줄**이었고 E1261 에서 BitmapFont·ClientAudio 두 행을 닫아 **partial 21·todo 0, 21단위 / 16,571줄**이었고 E1263 에서 GameWorld 행을 닫아(남은 갭 문장이 E1230~E1234 로 모두 해소) **partial 20·todo 0, 20단위 / 15,414줄**이었고 E1266 에서 ClientMain 행을 닫아(순서 차이 두 건을 원문 순서로 맞춤) **partial 19·todo 0, 19단위 / 13,276줄**이었고 E1268 에서 ClientState 계열 다섯 행을 닫아 **partial 14·todo 0, 14단위 / 12,849줄**이었고 E1273 에서 RawSurfaceDeletion·Lifetime 두 행을 닫아 **partial 12·todo 0, 12단위 / 12,748줄**이다. 단계 배정은 각 행이 남은 일로 걸린 3.1절 항목 기준이며 아래 둘째 표에 행 이름을 적었다. 같은 정리에서 4D-8b~4D-8f를 '남은 행 없음'(해당 대응표 행이 모두 port 또는 제외)으로 판정해 4-D가 61/85가 됐고 E1263 점검에서 15개를 더 닫아 76/85가 됐다. 이전 단계별 분배(51 / 24,263)와 E1082~E1230의 증감 경과는 [E1237 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에 있다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 |
| --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료. 4-C 통합 잔여는 4D-3 | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **84/85, L2 닫음(Tutorial1 4:3·16:9)** | 6 / 8,402 |
| 5 | 5-A 렌더러·Draw | 44/46 | 1 / 394 |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 12/13 | 0 / 0 |
| 5 | 5-C 화면 허브·미션 진입 | 4/6 → M2 | 3 / 3,562 |
| 6 | 입력·게임 메뉴·경제·건설 | 4/8 → M3 | 1 / 229 |
| 7 | 전투·의식·AI | 3/6 | 1 / 161 |
| 8 | 미션 수명·대본·오디오·도움말 | 8/9 → M4 | 0 / 0 |
| 9 | 표시 품질·브라우저/OS·배포 | E945 하위 경로·설정 복원 검증, M5 미완료 | 0 / 0 |
| 10 | AGENTS 후순위 웹 요구 | 일부 선행, 3.7절 참조 | 0 / 0 |
| **합계** | | | **12 / 12,748** |

남은 partial 행의 단계 배정(줄 수 큰 순, 대응표 메모와 3.1절 항목 기준):

| 단계 | 대응표 partial 행(줄 수) |
| --- | --- |
| 4-D | RawCampaignSession 7,130 · RawCampaignSessionLogin 389 · ClientCampaignTransport 378 · CampaignClientServices 373 · RawClockSynchronization 99 · UberReconnect 33 |
| 5-A | ClientRendererFrame 394 |
| 5-B | (남은 행 없음) |
| 5-C | UberGump 2,703 · RawButtonControls 811 · RawMenuChat 48 |
| 6 | CampaignObjectMenuClient 229 |
| 7 | RawCampaignRitual 161 |
| 8 | (남은 행 없음) |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,007단위는 **port 이상 995(98.8%; `port` 263·`session` 732)·partial 12·todo 0**이다(2026-11-23 재집계, E1273 반영, E1283 에서 session 승격). 제외 153단위(멀티 73·Edit 42·호스트 37·구판 1)를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session` 732행은 E1283 에서 `e2e/session-coverage.spec.js`(고정 시각 Tutorial1 4:3 130프레임 V8 함수 커버리지)와 `tools/port-map-session.mjs` 로 올렸다: **행의 웹 모듈이 모두 그 실행에서 함수를 하나 이상 실행했을 때만** 올리는 보수적 판정이며, 남은 `port` 263행(커버리지에 없음 62·함수 미실행 201)은 다른 시나리오(전투·AI·의식·편집 아닌 UI 등)에서 쓰이는 단위다. 오라클 대조를 뜻하는 `oracle`은 0이다(승격 규칙은 이후 시나리오가 늘 때 같은 도구로 다시 돌린다; [근거](docs/webpj-port-map-session-20261123.md)). E1098~E1237의 재판정 내역은 대응표 메모와 [E1166 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)·[E1237 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에 있다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 572 / 573 | 1 | 0 |
| `client/` 세션·UI·표시 | 414 / 433 | 19 | 0 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 12단위 중 기존 기준선은 8단위/12,515줄, 재기준선 추가분은 4단위/233줄이다(메모 머리 태그 `[rebaseline-port]`·`[rebaseline-adapt]` 기준). 재기준선 분류·영역별 상세는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)과 `python -X utf8 tools/port_inventory.py --families wb`에서 확인한다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 840항목을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

### 0.5 단위 착수와 종료

1. AGENTS.md와 공통·dotnetpj·웹 인계를 읽고 Git 상태·원격 변경을 확인한다. 같은 작업 트리의 다른 세션 변경을 고려해 파일명을 지정해 stage한다.
2. 대응표 검사와 기준선 뒤 변경을 확인한다. 3.1절의 단계 순서와 선행 조건에 맞춰 기준선 cpppj 코드, 기존 시험, fixture, 단위 문서를 읽는다. 큰 단위는 공개 API 경계로 나눈다.
3. 원본/클론·오라클 수집·별도 창 실행에는 AGENTS.md의 확인·예외 규칙을 적용한다. 웹 이식에는 디컴파일 소스 분석이 필요 없다.
4. 단위 문서에 근거 커밋·fixture/오라클·검사·실패와 교정·미연결 경계를 기록한다. 대응표의 web·base(8자리)·status·메모와 해당 manifest를 갱신한다. 현행 인계에는 현재 값·다음 작업·검증과 해당 체크만 갱신하고 완료 항목 상세는 정리 때 이관한다.
5. 각 작업 단위마다 해당 변경의 집중 검사와 연관 검사를 수행한다. HJOW-Athlon에서는 완료된 작업 단위 4회마다 Node v26 전체 회귀를, 8회마다 Node v22 전체 회귀를 수행한다. 두 버전의 전체 회귀가 함께 필요한 경우 순차 실행하며 동시에 실행하지 않는다. 다른 PC에서는 사용 가능한 최신 Node 버전으로 완료된 작업 단위 4회마다 전체 회귀를 수행한다. 필요하면 변경의 위험이나 통합 범위에 맞춰 추가 검사를 수행한다. 신호 NaN 비트 대조 시험은 맨 위에 `import './nan-exact.mjs'`를 둔다. (2026-11-19 사용자 지시)
6. 완료 후 당일 최대 번호를 확인하고 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. 기존 커밋의 Codex 공동 작성자 표기와 AGENTS.md의 push 금지 규칙을 따른다.

### 0.6 제품 공통 요구

11.01 표기·Credits 세 번째 항목과 HJOW/AI 문구·`Copyright 2026 HJOW`, 시작 클릭 화면, 공개 루트 내 실행 자산, 외부 접속 문서화·유효성 검사 비활성화는 반영했다. Credits AI 문구는 사용자가 다시 지시할 때 수정한다. **15차 Reset All은 E863에서 구현·검증했다.** [제품 11.01](docs/webpj-product-1101-20261105.md) · [외부 접속](docs/webpj-external-access-20261105.md) · [초기화](docs/webpj-reset-all-20261119.md)

<a id="favicon"></a>

- [ ] **favicon 탭 시각 확인.** E894에서 공통 ICO를 복사하고 상대 경로 링크를 추가했다. 원본 두 이미지의 바이트 대조와 Chromium의 HTTP 200·해독을 확인했다. 2026-11-20 Chrome의 localhost NetStorm 탭은 접근성 트리에서 확인했으나 Windows 창 캡처가 반복 시간 초과하여 탭 그림의 시각 확인이 남았다(E1263 점검: 탭 아이콘은 페이지 화면 캡처에 나오지 않는 브라우저 창 부분이라 자동 확인이 안 되고, 사용자 눈 확인 범위이다). [작업·검증](docs/webpj-favicon-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

<a id="korean-font-change"></a>

### 0.7 한국어 갈무리11

E896에서 실제 미션 설치의 You→당신 번역과 Player 표시 바이트를 연결했다. [설치](docs/webpj-app-install-display-20261119.md) 제공된 일반·굵은 WOFF2와 라이선스를 공개 데이터에 포함했고 로딩·Canvas·자료 준비에서 같은 글꼴 설정을 공유한다. 11픽셀 작은 글씨와 영어 대체를 Chromium 메뉴·도움말에서 확인했다. [구현·검증](docs/webpj-korean-galmuri11-20261117.md) 실제 미션 화면에서도 Tutorial1 한국어 브리핑 두 페이지의 갈무리11·혼합 제목·줄바꿈과 당시 미번역이던 Tutorial2의 영어 대체를 Chromium으로 확인했다. [한국어 미션](docs/webpj-korean-tutorial1-20261120.md)

## 1. 범위와 유지되는 결정

- **범위.** 일반 31경로와 데모 3경로를 대상으로 한다. 단독 미션의 같은 프로세스 안 내부 전송·패킷·Login·Zone·서버/클라이언트는 포함한다. LAN TCP·멀티플레이 UI·재접속/메타, 공식 검증 서버, Edit·커스텀 맵·User Made Campaigns, 설치/CD·Quit, 옵션 해상도/전체화면은 웹 범위에서 제외한다. 스커미시의 MP 시작 규칙과 Outpost는 12차에 포함한다.
- **이식.** cpppj의 파일과 API 경계, 부작용과 호출 순서, 정수 폭과 저장 시 정밀도, 난수 소비를 보존한다. 단독에서 도달하는지 확인해 웹 제외 분기에는 명시 오류를 유지한다. 현재 단계가 직접 요구하는 부품만 앞당긴다.
- **웹 구조.** `netstorm.js`가 앱 수명을 맡는다. `index.html`은 최소 변경하고 스타일은 JS에서 관리한다. 기존 CommonJS 서버와 `www/` ES 모듈 경계를 유지한다. 코어를 DOM·Canvas·Web Audio에서 분리하고 8비트 논리 화면과 최종 표시를 구분한다.
- **시간·입력·수명.** 시작 입력에서 오디오를 활성화한다. 탭이 숨으면 진행을 멈추고 복귀 시 시계를 다시 잡는다. `requestAnimationFrame`은 표시 기회다. 4:3부터 확인하고 16:9·16:10, 입력 역변환·DPI를 비교한다. 가장자리 스크롤은 활성 미션과 게임 영역 안의 포인터에서 동작한다.
- **자산·설정·언어.** `assets/game-data`·`fonts`·`webpj/translations`에서 `webpj/www/data`를 준비하며 느슨한 파일→TAFF 조회 순서를 유지한다. 버전 있는 `netstorm.settings`·`netstorm.progress` JSON은 localStorage에 두며 오류 시에도 계속할 수 있게 한다. 한국어 누락 파일·섹션·주제는 영어로 대체하고 미션 문구는 cpppj 확정본을 기본으로 삼는다. 사용자 검수와 언어별 글꼴은 3.5·3.7절을 따른다.
- **외부 링크.** Tell/도움말에서 치환된 `http:`·`https:`·`mailto:` 링크는 새 탭/창으로 열고 다른 주소의 제외 사유는 console에 기록한다. 개발 서버는 모든 인터페이스에서 수신한다. [외부 접속 7절](docs/webpj-external-access-20261105.md)
- **사용자 확인 범위.** 자연 완주·전체 외관/음향 동일성·실제 포획/희생·특정 적 공격·승리 재현은 사용자 확인 또는 별도 요청 대상으로 둔다. 공중 3종(훨리기그·맨오워·더스트 데빌)은 정적 이식 범위다.
- **보호·분담.** AGENTS.md·원본/CD·기존 fixture·공통 인계·dotnetpj 파일을 보존한다. cpppj 동시 작업은 사용자의 목표 지시를 기준으로 하며 별도 언급이 없으면 필요한 cpppj 수정이 가능하다. README 공통 상태는 일괄 갱신 때 반영한다. 새 문서와 코드는 UTF-8, 새 코드의 한국어 주석은 설명·사용법을 먼저 쓰고 추가·수정 시점과 이유를 뒤에 쓴다.

## 2. 완료 범위 요약

| 범위 | 확인한 결과 |
| --- | --- |
| WB00~WB04-D5c | 시작·메뉴·설정/진행·도움말·언어/제품 표기, 자산/TAFF/Fort 조회, SID·난수·Kernel·Form/Process 공통층. [기반과 단위 링크](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E1~E328 | 월드 규칙 부품, 자료·타입 표, Fort·지형·섬과 단독 미션 전송 앞단. [순서 3 및 4-A~4-C](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E329~E632 | 이동·비행·전투·의식 선행 부품, 렌더러·Gump·카메라/HUD, 제품 1313으로 재기준선, typed Session 생성자·공유 snapshot writer 선행. 최초 실행(E616)과 내부 미션 전환 flush/종료 수신(E620) 반영. [선행 부품과 재기준선](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E633~E702 | 외곽 생성자·snapshot/콜백과 typed `Load()` 완료분. 실제 1-1 Fort 적재, Battle snapshot 재적재, 기술·로컬/세계 간헐천 및 단일 미션 너겟 관문 확인. [typed Load](docs/webpj-session-load-20261118.md) · [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) |
| E703~E774 | 실제 typed Login/Ready·Start, 출하 스크립트 32종의 Fort 로그인, 같은 Session의 사제·골렘·벌룬 이동과 워크샾 배치·격리 완공. [Login](docs/webpj-session-login-ready-20261118.md) · [Start](docs/webpj-session-mission-start-20261118.md) · [워크샾](docs/webpj-workshop-isolated-completion-20261118.md) |
| E775~E801 | Session 카메라·입자·튜토리얼 게이트, AI/Player/Process 반납 경계, 지연 GameWorld·팔레트·자산 보기·선택. World 입력 부품과 UserInput 폴링·단축키·우클릭까지 연결. [공통 게이트](docs/webpj-session-tutorial-gate-20261118.md) · [지연 GameWorld](docs/webpj-campaign-game-world-front-20261118.md) · [입력](docs/webpj-campaign-selection-keys-20261118.md) |
| E802~E950 | 입력 클릭·NB20 취소, GameWorld 프레임·세 운반 유닛 도착, 워크샾 Kernel 완공·연구 동기화, 전투 완료 관문, Client 진단·프레임 시계, Session 덤프 형식과 오라클 시나리오(4D-9b), 앱 Guide·State7·Gump 루트·팔레트·Screen·입력 큐·TextEntry/IME·갈무리11 편집, Reset All(E863), 16:9 최대 1920×1080(E943), 하위 경로 배포(E945). [E1081 보관본 0.1·3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081) |
| E951~E1237 | 미션 앱 프레임의 로딩·Start·음향·Renderer/Gump 출력, 브라우저 입력·카메라와 원본 도움말 조작키, 31개 미션·데모·스커미시 진입과 장시간 진행, 생산 패널·건설, 객체 메뉴·명령, 게임 메뉴막대, 승패 뒤 흐름, 도움말, 한국어 대본·게임 안 문구, HUD 도킹·채팅 창, 웹 범위 판정과 Native 대응 대조, 감사로 찾은 프레임·도움말 차이 수정. 주제별 표는 [0.1절 요약](#e951-e1081). |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 4-D 전체가 선행 대기면 가능한 5-A·5-B를 병행한다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다.

#### 4-D Session 조립·Tutorial1 Load/Start (84/85)

**완료 기준은 L2다.** Tutorial1 Load/Start의 보고서·Session 덤프·첫 프레임 논리 화면을 cpppj 오라클과 대조한다. typed `Load()`와 실제 Fort Login/Ready, 일부 Start는 Node 검사에서 확인했다. Client·Native 장치와 브라우저 앱 수명은 이어서 연결한다. 기준선 구간 지도와 E633~E801의 완료 체크 및 단위 링크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)에, E802~E1166의 항목별 완료 나열 원문은 [E1166 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)에 있다.

아래 cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다. E946에서 4D-1b를 완료해 56개였고, 2026-11-23 정리에서 4D-8b~4D-8f를 완료로 판정해 61개였고, E1263 점검에서 15개(4D-3a·3d·3f·5a·5c·6a·6b·6c·7a·7b·7c·7d·8g·9a·10a)를 더 닫아 76개였고 E1266 에서 4D-4a 를 닫아 77개였고 E1267 에서 4D-3e 를 닫아 78개였고 E1268 에서 4D-3c·4D-5b 를 닫아 80개였고 E1270 에서 4D-1c 를 닫아 81개였고 E1273 에서 4D-8a 를 닫아 82개였고 E1274 에서 4D-3b 를 닫아 83개다. 닫은 항목의 남은 일은 "점검" 줄에 근거를 적었고 다른 열린 항목이 맡는 잔여는 그 항목으로 옮겼다([점검 기록](docs/webpj-left-jobs-audit-20261123.md)). 항목마다 남은 일을 먼저 적고 완료분은 단위 번호와 대표 근거만 둔다.

**4D-1 Session 외곽·snapshot.** 저장 writer, 현재 Core, 부모 callback, 생성자/Publish와 외곽 반납 앞단, 4D-1b 전체 snapshot(Battle 재적재·기술 획득 Fort snapshot, `loaded` 이전 Editor 수명은 Edit 제외, E946)은 완료했다. [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) · [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md)

- [x] **4D-1c 외곽 callback** — (E1270 닫음) 기술 획득, 로컬·세계 간헐천, 단일 미션 너겟 관문은 실자료로 확인했다. `PlaceBattleNuggets`·`ReplenishNuggets`(5761..5772)의 비미션 정상 생성은 웹에서 도달하지 않는다: 스커미시는 `missionPost` 가 참인 미션 모드의 전투 월드로 돌아(실제 Chromium 에서 확인: 월드에 너겟 객체가 없고 간헐천 27개가 무작위로 놓임, `e2e/skirmish-world-mode.spec.js`) 비미션 전투(멀티플레이)에서만 쓰이는 너겟 배치·보충 관문이 열리지 않는다. 원문의 단일 미션 관문(너겟 없음)과 같다. [근거](docs/webpj-skirmish-world-mode-20261123.md)

**4D-2 typed Load 완료.** 건설·이동·AI·의식·파괴/주문·운반 투하/기도·배치/커서를 Session에 설치했다. 캠페인 1-1의 저장 객체 적재와 `loaded` 관문을 확인했다. [E697](docs/webpj-session-load-20261118.md)

**4D-3 Login·로컬 연결·4-C 통합 시험.** 내부 Login → Load·명령 등록 → Ready → Start 순서를 실제 Session에서 확인했다.

- **E950 선행:** 지연 요청의 Poll 전 수명과 실제 Guide→State7→Ready를 검사했다. E968에서 기존 World가 없는 경우 앱 프레임 자동 호출을 연결했고, 기존 World 교체는 미션을 떠난 뒤 다시 시작하는 웹 구조로 대체했다(E1085, 4D-3a). [E968](docs/webpj-app-fort-request-frame-20261120.md) [요청 프레임](docs/webpj-campaign-request-frame-20261119.md)

- [x] **4D-3a RawCampaignSessionLogin** — (E1263 닫음)
  - 점검: 기존 World 교체는 미션을 먼저 떠난 뒤 Guide 를 다시 시작하는 웹 구조로 대체돼 있고(`netstorm.js` `_leaveCampaignMission`→`_restartCampaignMission`, E1085, e2e `game-menu-bar`·`mission-leave-pause`), 세계 화면·분리 타이머는 E851~E852·E981~E989·E1130, 고정 수신기는 E1193~E1194 에서 끝났다. 31h 개발자 명령은 수신기·해석기가 등록돼 있고 송신원이 개발 콘솔뿐이라 도달하지 않으며(E1242), `BeginGeneral`·`InitializeGeneral`·`LocalFort` 는 일반(LAN) 설치 모드 `CampaignInstallMode::GeneralBattle`(`RawCampaignSessionLogin.inc:325`)과 `!missionPost`(:246) 가지라 웹 Session(`objectState.mission=true`)에서 도달하지 않는다. 29분 스커미시에서도 웹의 해당 throw 에 걸리지 않았다(E1262).
  - 완료: Begin/Pump·Ready 왕복과 행 base 뒤 32커밋 판정(E765), 진행 안내·분리 타이머(E851~E852), Fort 이름 선택·원본 관측 876행(E884~E892), 앱 Session의 Snapshot·Notify·State(E948), Guide Init·앱 프레임 실행(E886·E890), 최초 지연 요청 소비·Tutorial Start(E968·E975), 출하 Fort 32종 로그인(일반 31·데모 1, E752), 데모 모드 Start(E1092·E1125). [E765](docs/webpj-session-login-commit-audit-20261118.md) · [E948](docs/webpj-app-fort-name-session-20261119.md) · [출하 Fort](docs/webpj-session-login-shipped-forts-20261118.md)
- [x] **4D-3b 로컬 공개 API·ClientCampaignTransport** — (E1274 닫음)
  - 점검: Client 기본 수신기 표(`makeClientCampaignTransport`·`installClientPacketDefaults`, 18행)는 서버→Client 패킷의 검증·진단 장치인데 웹 단독 미션의 내부 전송에는 그 종류가 도착하지 않는다 — 실제 Chromium 에서 스커미시 40초 동안 분배기에 도착한 패킷은 종류 1 두 개뿐이고 분배기는 수신기 없는 종류를 오류 없이 무시한다. 앱에 설치하지 않아도 같으며 e2e `internal-transport-packets.spec.js` 가 이 성질을 고정한다([근거](docs/webpj-client-default-receivers-scope-20261123.md)). Native 전송 수명은 웹의 `usesNativeTransmissionDevices` 가 항상 false 라 도달하지 않는다.
  - 완료: 시작/Poll/Pump·상태8·전송 기록·로컬/mutable 송신·Tell과 typed 기본 수신기(경로/압축 관찰·기술·Chat·연결 종료, E772), 내부 전송 등록 세대의 Session 대여·수신 중 State 정리 차단(E893). [E772](docs/webpj-session-state-connect-record-20261118.md) · [E893](docs/webpj-session-internal-callback-20261119.md)
- [x] **4D-3c 전송 앞단** — (E1268 닫음)
  - 점검: 미션 이탈 때 시간 동기화 세 DWORD(`gameTicks` 등) 초기화(`resetClientClockSynchronization`)를 E1268 에서 `_releaseCampaignWorld` 에 옮겼다(미션 중 `replyMode` 가 1 로 남던 것을 처음 상태로, e2e `mission-leave-state-reset.spec.js`, [근거](docs/webpj-mission-leave-state-reset-20261123.md)). 소켓 지연 FIFO(MP 전용, E1187)·`ClientPacketSources`(웹 대체, E1187)·Native baseline(canonical 수신기는 Native 전용, E1223)은 닫혔거나 도달하지 않는다.
  - 완료: 공통 `RawPacketFrame`의 로컬 전송 연결(E820), 앱의 시간 표본·Session Pump·공유 GameClock·State 정리(E947). [E820](docs/webpj-packet-frame-20261119.md) · [E947](docs/webpj-app-clock-synchronization-20261119.md)
- [x] **4D-3d Squid 수신** — (E1263 닫음)
  - 점검: ReceiveSquid(0x2c)·Dais(0x6a)·Gun(0x3a)·Flyer(0x39·0x86)·Aviary 수신이 모두 이식돼 Session 분배기에 등록돼 있다(`raw-campaign-session.js` 804~838행; 시험 `raw-campaign-session-load.test.mjs` Dais·`raw-gun-process`·`aviary-process-data`). Native 경계와 실제 원격 로그인은 웹 범위 밖이다.
  - 완료: `RawPeerWorldSync` 본체와 World·Login 서버 콜백(E822~E823), 공통 객체·종속 부착·변경/삭제·검증·공유 Regular·압축의 typed 수신. [E823](docs/webpj-peer-world-session-20261119.md) · [압축](docs/webpj-compressed-squid-receive-20261118.md)
- [x] **4D-3e Login 콜백·명령 등록** — (E1267 닫음)
  - 점검: 입력 큐에 명령을 올리는 발행처를 `www` 전체에서 모두 세어 입력 루프 허용 목록과 대조했다([근거](docs/webpj-input-queue-codes-audit-20261123.md)). 목적지 선택(0x02000044)·수입 알림(0x020000b4)·미션 첫 단계/재생성/전투 떠나기·Pause·ESC(`isGameMenuCommand`)·생산 단축키(0x020000f4·0x02000107)·대화상자 송금/동맹/회수/지식·Help Back 은 모두 허용 목록에 들고, 대본 버튼 명령은 E1190·E1231 이, 개발자 큐(31h, E1242)와 전투 종료 메뉴(`missionPost`, E1238)는 도달하지 않음을 확인했다. 새로 막히는 코드는 없었다. 등록(5262..5436의 패킷 종류)은 모두 끝났다.
  - 완료: Carrier·건설·연구 typed 등록(E750), RegisterCarrierCommands 5262..5436의 Native 분기·미연결 명령과 31h 개발자 패널 명령의 입력 고리 예약(E766). [E750](docs/webpj-session-login-ready-20261118.md) · [E766](docs/webpj-developer-devices-20261118.md)
- [x] **4D-3f 통합 시험** — (E1263 닫음)
  - 점검: 실제 앱 프레임은 E836 이후 e2e 가, 유닛 생산은 `e2e/english-tutorial2.spec.js` 가 워크샾 생산 뒤 Archer 를 클릭 배치해 `object.type===71` 개수가 늘어남을 확인한다(E1079·E1086).
  - 완료: 같은 Session의 Login/옵션/사제 자연 도착, 일반·데모 Start와 AI 동맹, 세 운반 유닛의 선택→호버→클릭→전송/Kernel 자연 도착(E826), 1-1 워크샾 Kernel 자연 완공·연구 58h·생산 기술 동기화·플레어 수명·중복 연구(E827~E830), GameWorld 프레임의 세 유닛 자연 도착(E836). [E826](docs/webpj-world-carrier-input-20261119.md) · [E827](docs/webpj-workshop-kernel-completion-20261119.md) · [E836](docs/webpj-game-world-frame-20261119.md)

**4D-4 프레임·카메라·Tutorial 게이트**

- [x] **4D-4a 카메라·프레임** — (E1266 닫음)
  - 점검: 프레임 순서 차이 두 건을 원문 주 루프(`ClientMain.cpp:596~632`) 순서로 맞췄다 — 주변음·바람 → 다리 붕괴(`world.runBridgeDecay`) → 장면 음악(`_sceneMusic.frame`; Game 난수열이 주변음·바람 뒤) → 미니맵 갱신(`hud.refresh`) → Gump 정리·Idle → Kernel([근거](docs/webpj-frame-order-20261123.md), `tests/raw-campaign-session-load.test.mjs`, e2e 11건). 실제 Client·전체 Renderer 연결과 나머지 프레임 순서는 E977~E989·E1230~E1234 에서 끝났다. 대응표 ClientMain 행도 port 로 올렸다.
  - 완료: Session PrepareFrame/RunFrame, 공유 카메라 요청·적용, 초기/Home/사제 시점, 입자 뷰·커서 경계, GameWorld 프레임·단일 Kernel 준비/후처리(E775·E777). [E775](docs/webpj-session-camera-request-20261118.md) · [E777](docs/webpj-session-particle-view-20261118.md)
- [ ] **4D-4b Tutorial 게이트·표시 장치**
  - 남음(E1263 점검): Tutorial1·3~6 의 실제 앱 단계 진행(자연 진행은 Tutorial2 만 e2e 로 확인, E1086; [다음 작업 2](#next)), 스커미시 승리 실전(1절 사용자 확인 범위). 다리 수명 숫자·Renderer(E1230)와 승패 판정 소비(E1152·E1161·E1262)는 끝났다.
  - 완료: Tutorial 1~6 판정·공통 게이트, 신전의 현재 슬롯 재조회(E781~E782). [E781](docs/webpj-session-temple-draw-20261118.md) · [E782](docs/webpj-session-tutorial-gate-20261118.md)

**4D-5 승패·종료·State 반납**

- [x] **4D-5a 종료 수명** — (E1263 닫음)
  - 점검: 실제 앱의 State 반납은 `Session.dispose`(`raw-campaign-session.js`)가 State 자식 증빙·Player 링크·AI·Process 를 반납하며 Leave/Restart(`game-menu-bar.spec.js`)·`mission-leave-pause.spec.js`·31개 미션 시작·장시간 시험이 그 경로를 밟는다. Native 건설·지속 실패는 Native 저장소가 없어 도달하지 않고 승패 수명은 E1152·E1161·E1262 가 확인했다. State 정리 부모 연결은 4D-5b 가 맡는다.
  - 완료: 승패 조회 경계, AI·Player/Deck·Process 자식 반납 증빙, 일반 종료 순서·오류 진단(E786·E789). [E786](docs/webpj-session-state-withdraw-20261118.md) · [E789](docs/webpj-session-process-shutdown-error-20261118.md)
- [x] **4D-5b Client State 정리** — (E1268 닫음)
  - 점검: 앱은 State 정리 부모(`cleanupClientStateWorld`)를 직접 부르지 않고(앱에 없는 장치 `interpretStateOptions`·`writeSongEndBits`·`killGumps`/`replaceUber`/`leaveMetamap`/`refreshSp`/`retireRuntime` 이 먼저 필요, E1223) `_leaveCampaignMission`→`_releaseCampaignWorld` 가 이탈 때 원문만 하는 효과를 하나씩 옮긴다([근거](docs/webpj-mission-leave-state-reset-20261123.md)): 건물 숨김 복구(E1230)·정지 깊이 반복 해제(E1132)·`ownerMode.battle` 잔존(영향 없음, 다음 작업 1 ⑥)·장면 음악 난수(E1258), E1268 에서 채팅 채널 정리(`ChatChannels.resetOccupied`)와 시간 동기화 초기화를 옮겼고(e2e `mission-leave-state-reset.spec.js`, 변형 확인) `MapRevision+1`(다음 미션 렌더러 초기화가 올림)·이탈 시점의 선택 해제 통지(미션 UI·Session 반납)는 영향 없음으로 판정했다. RawMenuControls(E1252)·ClientAudio(`5cb9366ec`)·승패 후 수명(E1152·E1161·E1262)은 끝났고 대응표 ClientState 계열 다섯 행을 port 로 올렸다.
  - 완료: Fort 적재 Session(E818)과 내부 로그인·AI 미션 시작 뒤(E819)의 정리 검사. [E818](docs/webpj-client-state-loaded-cleanup-20261119.md) · [E819](docs/webpj-client-state-started-cleanup-20261119.md)
- [x] **4D-5c 이탈·전투 판정** — (E1263 닫음)
  - 점검: 전투 종료 프레임은 원문 관문 `!battle || missionPost || !serverConnection || phase!=14`(`RawBattleCompletion.cpp:65~96`)에 막혀 단독 미션에서 도달하지 않고(E1238), 4209..4279 의 함수 중 FinalizeBattleLeave(cpppj 전체에 호출자 없음)·RejectDraw·RequestForcedAlliance·PollReconnectSave·TakeServerForState·SaveServerFailure·BroadcastConnectionPlayer 는 전투·재접속·LAN 전용이고 BattleGate·BattleFinished(`netstorm.js:1404`·`raw-campaign-session.js:984`)와 RunConnectionLoginTimer(`login-front.js` `pump()`)는 앱에 연결돼 있다. 재접속 저장(`!state.dat`)은 `RawSavedStateWrite` 가 `exclude-mp` 이고 missionPost 에서 자연 반환한다. 스커미시 패배 안내는 실제 시간 관찰(E1262)로, 캠페인 진행 저장은 E1152 로 확인했고 스커미시 적 전멸(승리) 관찰만 [다음 작업 5](#next) 에 남는다.
  - 완료: RawBattleCompletion 팀·기권·미접속 관문과 후반 Frame의 기준선 C++ 합성 상태 4,096건 대조, 실제 Session의 Player·동맹·연결·사제 종료 조회, Client 장기 소유자·입력 고리, 무승부 거절의 Session 6F 처리·방송, 전투 복귀 State6의 결과·배경·음악(E842~E847). [E842](docs/webpj-battle-completion-gates-20261119.md) · [E844](docs/webpj-battle-completion-frame-20261119.md) · [E847](docs/webpj-battle-return-state-20261119.md)

**4D-6 GameWorld·Client 서비스**

- **E951 선행:** State7·13 HUD 모드를 실제 앱 Gump 입력에 연결했다. E952에서 메뉴 진행 창을 추가했으며 원본 serial과 전체 HUD는 별도다. [로그인 HUD](docs/webpj-app-login-hud-mode-20261119.md)

- [x] **4D-6a GameWorld** — (E1263 닫음) 지연 Session 소유부터 앱 프레임 결선까지 끝났다.
  - 점검: 기존 World 교체는 4D-3a(미션을 떠난 뒤 다시 시작), 로그인 뒤 화면 설치·분리 타이머·진행 안내는 E851~E852·E908~E913·E977~E989·E1131 에서 끝났고 입력은 6단계에서 연결했다. "다른 UI 색상·날씨"는 날씨 색(`tint`)이 구름 비트(0x08)가 없는 화면 모드에서만 쓰이는데 웹 화면 모드가 `fallbackWindowed=0x02|0x08`(`render/screen.js:35`)이라 도달하지 않는다. 종료·전체 삭제(이탈) 때 원문만 하는 효과의 확인은 4D-5b 가 맡는다. 대응표 GameWorld 행은 남은 갭 문장이 E1230~E1234 로 모두 해소돼 port 로 올렸다.
  - 완료: 지연 Session 소유·Login·자산·팔레트·선택 효과(E790~E793), GameWorld 프레임·단일 Kernel과 State7 부모·State8 내부 로그인, 로그인 뒤 연결 감시·타이머 프레임(E855), State Poll·패킷 Pump(E895), 카메라·팔레트·색·Screen DIB·구름·전투 팔레트(E898~E907), Gump 루트·트리·Uber 루트(E908~E913), Renderer 결선(5-A, E977~E989). [E790](docs/webpj-campaign-game-world-front-20261118.md) · [E895](docs/webpj-app-connection-frame-20261119.md) · [E908](docs/webpj-app-gump-root-20261119.md)
- [x] **4D-6b Client 서비스** — (E1263 닫음) CampaignClientServices와 앱 Client 장치를 실제 Session에 연결했다.
  - 점검: 원본 Gump serial 은 웹이 TellWindow 어댑터를 써서 내부 serial 재사용만 없고 동작은 같다(E1252 판정, [근거](docs/webpj-app-mission-loading-20261119.md)). 취소 명령은 로딩 창 Cancel=`LeaveBattle`→`_leaveCampaignMission`(`mission-loading-window.test.mjs`; 로딩 중 실제 클릭 e2e 는 없음). 메뉴·Gump 소유자·프레임 타이머·입력 장치·Poll 은 E895·E908~E913·E919~E934·E967~E998·E1084~E1100, 나머지 미션의 입력 큐 소비는 E1225·E1231(대본 50개 전수, 허용 목록 보강; 3초 정체 폴백은 남음), 스커미시 준비는 E1101~E1107 이고 건설 기록 장치는 개발자 전용으로 웹 범위 밖이다(E1242). Early Mission 3 단계 A·6 단계 B 의 너겟 납품(`0x020000b4`) 자연 완주는 1절 사용자 확인 범위이다. 대응표 CampaignClientServices 행은 ricochet 값의 비결정성(E1226, 원문과 동등 재현 불가)이 있어 partial 로 둔다.
  - 완료: Start 효과·AI·State7/8·개발자 입력(E837~E841), 로그인 진행·사용자 입력·커서·표시 장치(E851·E856~E858), 신전 소유권·PostPop 생산 갱신(E875), 번역·설정·Login 공통 장치(E896~E897), 미션 장치·화면 크기·즉시 카메라(E952~E956), 효과음(E965~E966), AI·채팅 창·Start·Tell·팝업·승패·Kernel 프레임(E967~E976), Tutorial 메뉴 시작과 선택·이동·포인터(E985~E998), 소리·안내 서비스 여섯 개(E1122). [E837](docs/webpj-campaign-client-start-effects-20261119.md) · [E967](docs/webpj-app-campaign-ai-20261120.md) · [E1122](docs/webpj-mission-services-long-run-20261120.md)
- [x] **4D-6c Session 공개 조회·장치** — (E1263 닫음)
  - 점검(줄 단위 대응은 확인하지 못해 확신 낮음): 기준선 줄 구간으로만 적혀 낡은 목록이었다. 4590..4619 는 건설 기록 재생(개발자 전용, E1242)·Fort 구독자 변경(31h 개발자 명령·멀티 UI)·단순 조회, 4643..4741 의 Native 훅 11종은 [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md)(E1205~E1221)가 대조했고 송금·SP 는 E1166~E1168, 6029..6032 는 Fort 청크 저장(`raw-campaign-callbacks.js`, E829), 6076..6079 는 `raw-tutorial-later.js`, 6100..6114·6316..6399(기술 지우기·커서 마나/범위/에너지 바인딩·건설 이동·지면 충격)는 `raw-cursor-mana.js`·`raw-cursor-range.js`·`raw-campaign-session-cursor.js`·`raw-campaign-session.js` 에 같은 이름으로 연결돼 있고 건설 배치 e2e 가 쓴다. 웹은 C++ 의 Session 멤버를 모듈로 직접 조립하므로 같은 이름의 메서드를 따로 두지 않는다.
  - 완료: Start의 조회·기술·사제, 건설 운반, 생산 소유자, 커서·에너지·배치, 운반체·Dude 이동 공개와 부모·통지 조회·인구/경로·속도별 시간표·도착 삭제(E848~E850). [E848](docs/webpj-session-public-movement-20261119.md) · [E850](docs/webpj-session-dude-path-20261119.md)

**4D-7 ClientMain·Client 전역**

- **E949 선행:** Client 효과음 이름표를 실제 Session에 공유했다. 재생 장치와 위치 음향은 8단계 E957~E966에서 연결했다. [이름표 수명](docs/webpj-app-sound-list-20261119.md)

- [x] **4D-7a ClientMain 단독 조립** — (E1263 닫음)
  - 점검: 장기 Session·나머지 Client 전역은 E878~E883(ClientGameplayGlobals·RawSurfaceStorage 행 port), ConfigInterface 는 E896~E897, Native 는 4D-8b~4D-8f 에서 끝났다. RawSurfaceStorage 장치(ClientMain 442..533)에서 남은 것은 `EnablePatchDestroyDiagnostics` 에 해당하는 DestroyPatch 진단 열두 지점과 `rememberPlacement` 미연결이며 4D-8a 가 맡는다.
  - 완료: 공통 난수·SP·Session 대여(E878), Player·Core·Template·Subscriber 장기 소유자(E879), 장기 Storage·Kernel(E880), 미션 자료 준비(E881), Guide 생성·교체·반납과 제한 표(E883). [E879](docs/webpj-app-shared-players-20261119.md) · [E880](docs/webpj-app-world-storage-20261119.md) · [E883](docs/webpj-app-campaign-guide-20261119.md)
- [x] **4D-7b 메인 루프 단독 단계** — (E1263 닫음)
  - 점검: 앱 미션·Kernel 프레임은 E976·E1230~E1234 로 연결했고(전투 자원 프레임 `PollBattleResources` 는 E1232 에서 `Session.runBattleResources` 로 앱 Kernel 프레임 뒤에 연결), 재접속 저장은 단독 미션(`missionPost`)에서 원본도 no-op 이라 도달하지 않으며, ClientDiagnostics 는 행 port(E1223)·ClientStaticStrings 는 소비자 둘이 웹 제외라 앱이 만들지 않는다(E1228). 오류/예외 로깅은 3.7절 7차와 한 항목이며 프레임 예외 한 번에 렌더 루프가 멈추는 설계(`netstorm.js` `_startFrameLoop` 가 보고 후 rethrow)를 유지할지의 결정만 남는다. 대응표 UberReconnect 행(E1223 "단독에서 관측되는지 미확인")과 ClientMain 행(E1230 "영향 없음")의 문구가 달랐으나, 단독도 State 13 에서 `request(14)` 로 phase 14 에 닿고(`raw-local-mission-state.js:279`) `raw-connection-health.js` 의 서버 분기가 활동 프레임이 있는 다른 Player 에게만 안내를 내므로 단독에서는 하트비트 외 관측 효과가 없다고 확정한다(확신 중간).
  - 완료: 로그인 타이머·연결 활동 패킷·연결 감시 뒤 타이머 순서(E852~E854), 임시 문자열 고리·정수 서식·trace/console/Chat 경계(E859~E861), Kernel 소진 진단(E864), 공유 프레임 시계·보정과 앱 프레임·탭 숨김 정지(E876~E877). [E852](docs/webpj-session-login-frame-timer-20261119.md) · [E861](docs/webpj-client-diagnostics-20261119.md) · [E877](docs/webpj-app-frame-clock-20261119.md)
- [x] **4D-7c 메타 단독 앞단** — (E1263 닫음) ClientMetaPacketReceivers·ClientChallengeGlobals·NativeMetaConnectedState는 메타(챌린지) 전용이라 **멀티/메타 제외로 판정**했다([E1188](docs/webpj-meta-front-scope-20261122.md)). ClientMetaSpriteRefresh·NativeMetaSpriteRefresh는 단독 미션에 Sprite 객체가 없어 제외로 판정했다([E1203](docs/webpj-meta-sprite-scope-20261122.md)). NotifyPlayerResults는 E1233에서 프레임 끝(known·battle)에 연결해 단독 미션의 제단 등급·`AltarUpgrade` 안내로 소비됨을 확인했다. MetaMenuType(`RawCampaignSession.cpp` 4620..4642)은 유일한 소비자가 `UberMeta.inc:11`(`exclude-mp`)이라 단독 미션에서 도달하지 않는다(E1263 점검, cpppj 전체 grep).
- [x] **4D-7d Client 렌더러 프레임 전역** — (E1263 닫음) 채팅·HTML·점 명령 전역 이식과 ClientRendererFrameGlobals의 앱 소유·화면 크기 변경 수명(E977)에 이어 수집/출력의 앱 사용(`netstorm.js` 의 scan·cloudQueue·cloudDirtyCount·hideGroups, E979~E989)도 확인했다(행 port, E1223). [E977](docs/webpj-app-renderer-frame-owner-20261120.md) · [E496](docs/webpj-renderer-cloud-queue-20261115.md)

**4D-8 Native(10.78 Client) 분기.** 실제 cpppj 오라클은 시작 때 RawSurfaceStorage에 Native 장치를 결합한다. typed Session과 같은 상태를 비교하려면 실제 Client 소유자 아래 Native 경계를 함께 맞춰야 한다. **완료(2026-11-23 정리에서 체크, 해당 대응표 행이 모두 port 또는 제외):** 4D-8b 자원·생산·사제(Carrier E1220·Mana E1218·Reward E1219, DudeRegular·ResidenceRegular·Priest·PriestDeletion·FactoryRegular·Gump E1211·E1215; SP 표시는 HUD 예산 글자가 매 프레임 따라감), 4D-8c 전투·의식(Vortex E1219 섬 번호 편향 snap, Balloon·Bomb·Fence·Flare·Ice E1213, Damageable·DaisDeletion·Ritual E1215 — 소비자 연결은 7단계), 4D-8d Native 건설·Vortex(ClientConstruction 여섯 행 E1205~E1208, AssetDeletionVtables E1209·E1221, RawCampaignVortex 대응표 port — 입력·건설 나머지는 6단계), 4D-8e 고정 수신기(NativeLogin/Gameplay/Carrier/Construction 네 묶음 E1193~E1194, ClientChatPacketReceiver E1195, FortCoreDataLoad 제외 E1204 — canonical 기준선·대여 확인은 3.6절 4D-8 행), 4D-8f 공통 후처리·Fort 가상(NativeFortSaveVirtual E1210, SquidPostPop 대응표 port). [Native 수신기](docs/webpj-native-receivers-audit-20261122.md) · [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md) · [Fort CoreData](docs/webpj-fort-core-data-gate-20261122.md)

- [x] **4D-8a RawSurface 삭제·수명·공통** — (E1273 닫음, 의도된 차이 유지) 남았던 두 행(RawSurfaceDeletion·RawSurfaceLifetime)을 판정해 port 로 올렸다: ① Native DestroyPatch `AssertPatch` 열두 지점과 0x771 은 원문이 진단을 표시하고 계속하지만 웹은 보고 후 중단한다 — 불변식이 깨진 손상 상태에서만 갈리고 정상 경로 e2e 에서 나지 않으며 안전한 쪽을 유지한다(원문 순서로 바꾸려면 `destroy` 의 throw 열두 곳을 `hooks.assertion` 호출 뒤 계속으로 바꾼다, **사용자 결정으로 열어 둠**). ② `rememberPlacement` 는 cpppj 도 같은 logic_error 로 막아 복원이 없다. ③ BridgeRegular 상수 스냅샷(boss 항상 true·82/155)은 단독 플레이에서 같은 값이다. [근거](docs/webpj-destroy-patch-decision-20261123.md) · 차이 목록 [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md). AssetDeletion·AssetNotice·Content·Decoration·FrameRegular·RegistryDeletion·PlayerRegular 는 E1211·E1215 에서, Attack 은 E1220·Depend 는 E1217 에서 닫았고 unit-lost 억제 연결은 E1216 이다.
- [x] **4D-8g Native 분기 재대조** — (E1263 닫음) 대응표와 단위 문서에서 남긴 `UsesNativeTransmissionDevices()` 분기, 장기 Storage owner, 보상/삭제 공유 장부, ProcessHostStorage·Kernel procSumm을 함께 확인했다: 이 분기를 쓰는 cpppj 파일의 행(RawSurface*·Native*PacketReceivers·ClientConstruction*·RawProcessHostStorage·RawProcessAttach/Removal·Kernel·AssetDeletionVtables·RawCampaignVortex)은 모두 `port`, Native Meta* 는 `exclude-mp`, procSumm 은 `RawProcessSummary.cpp` 가 `exclude-host`(Kernel 의 "Out of processes" 개발자 덤프)이고 장기 Storage 는 앱이 쓴다(`netstorm.js` `_surfaceStorage`). 남은 partial 은 RawSurfaceDeletion·Lifetime 뿐이며 4D-8a 가 맡는다.

**4D-9 웹 session-dump·오라클 시나리오.** 4D-9b 오라클 시나리오는 완료했다. E868에서 `tools/cpp-oracle.mjs`의 `session-dump` 단계·고정 프레임 인자와 결과 수집을, E869에서 고정 시각 Tutorial1의 실제 Load/Start 덤프·화면·보고서 수집을 연결했다. 웹 비교는 4D-10에 남는다. [E868](docs/webpj-cpp-oracle-session-dump-20261119.md) · [E869](docs/webpj-tutorial1-cpp-oracle-20261119.md)

- [x] **4D-9a SessionStateDump** — (E1263 닫음)
  - 점검: 앱의 `Netstorm.sessionDump()`(E1129)가 실제 Client 수명의 Session 을 덤프하고, 원본 호출 시점(cpppj ui-script `session-dump`, 프레임 90 load·130 later)의 오라클 수집은 E868·E869 에서 끝났다. 시점을 맞춘 행별 비교는 고정 시각 모드와 함께 4D-10b 가 맡는다.
  - 완료: 판번호 1 순수 TSV 서식과 분산 SP 복호(E865), 실제 Session의 시계·난수·Kernel/Regular·풀·Player/동맹·SP·Graph·TotalMade 수집(E866~E867), 앱의 검사 전용 `sessionDump()`(E1129). [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md) · [E865](docs/webpj-session-dump-format-20261119.md) · [E867](docs/webpj-session-dump-world-20261119.md)

**4D-10 재대조와 L2**

- [x] **4D-10a 실제 소유자 재대조** — (E1263 닫음)
  - 점검: ClientGameplayGlobals 는 앱이 만들고(`netstorm.js`), GameAux 는 `raw-campaign-session.js`, TypeNames 는 `raw-fort-type-names-load.js`→`raw-campaign-session-streams.js`, RawBombBlastBindings 는 `raw-campaign-session-devastation.js`·`-particles.js` 로 실제 Session 에 연결돼 있고 대응표 행이 모두 `port` 이다. Archer·Motion/Path·Cursor·SoundProcess 소비자 검증은 6~8단계의 전투 바인딩·사제·운반·경로 바인딩·Cursor·RawCursor 표시 항목이 맡는다.
  - 완료: CampaignGuideRelease의 실제 Client 금지값 참조와 Session 보유·무World 해제(E882). [E882](docs/webpj-client-guide-release-20261119.md)
- [x] **4D-10b L2 판정** — (E1283 닫음) 같은 시점·난수에서 Tutorial1 Load/Start의 덤프·첫 프레임 논리 화면이 cpppj 오라클과 같다(영어 4:3·16:9, 90·130프레임: 화면 0픽셀, 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다름 — `player` 의 +1·+2 바이트는 주소 잔재, Player 1 +0x68·+0x69 는 Deck 할당 토큰(cpppj 는 프레임 90 전에 HUD 생산 동기화가 할당·웹은 첫 사용 때 할당하는 표현·시점 차이로 화면·동작과 무관)). 실제 결선 단위는 대응표에서 `session` 으로 올렸다(`tools/port-map-session.mjs`, 아래).
  - (남았던 것, E1281~E1283 에서 모두 닫음) 고정 시각 실행 모드로 행 전체 일치, `later.tsv` L3·첫 프레임 논리 화면(도움말 창 가운데 배치 확인 포함), Player 기록 앞 4바이트·Player 1 +0x68 원인 규명. (E1263 점검 당시 웹 화면 대 `load.png` 비교는 없었다(`l2-tutorial1.spec.js` 는 첫 안내 창 x 위치만 단언; E1278 에서 `fixed-time.spec.js` 에 추가), Player 1 +0x68·+0x69 는 기준선 0x02c0·웹 0 의 동적 Deck 필드로 원인 미분류.) **E1271 진행:** 앱에 검사 전용 고정 시각 모드(`?fixed=25`, `stepFixedFrames`)를 더해 같은 입력의 두 실행이 문자 단위로 같은 덤프를 내고(`e2e/fixed-time.spec.js`), 시작 그림 구간을 cpppj 처럼 360프레임·9초로 맞춰 행 수(1,964/1,964)와 `frame` 행(게임 시각 9.2)이 기준선과 같아졌다. E1272 에서 시작 그림 구간의 주 루프 갱신(난수 소비)을 건너뛰게 해 행 전체 일치는 1,787/1,964이고, 남은 차이는 월드 생성 순서(cpppj 는 월드 연결 프레임 f4 에 첫 안내 창·정지, 웹은 F4 연결·F8 창; 웹이 한 프레임 이름)와 생성 중 Game 난수 소비 총량(웹이 약 175회 적음)·초기 시계 0.125가 원인 후보다([난수 소비 대조](docs/webpj-fixed-time-mode-20261123.md)). cpppj 프레임 1~16 타임라인을 직접 수집해 근거를 적었다([근거](docs/webpj-fixed-time-mode-20261123.md)). **E1278 진행:** 웹 90프레임 논리 화면을 `load.png` 와 대조하는 시험을 `e2e/fixed-time.spec.js` 에 더했다 — 다른 픽셀 7,195개(0.91%)뿐이고 안내 창·HUD 대부분은 같으며 차이는 섬 아랫면·미니맵 조각·건물 하나에 모여 있다(난수 소비 순서 차이의 시각적 결과로 보임, 시험이 상한 7,195로 고정; [근거](docs/webpj-fixed-time-mode-20261123.md)). **E1280 진행:** cpppj 에 임시 난수 추적(호출 스택 RVA + 링크 맵)을 넣어 Game 난수 소비 순서 전체를 얻었다. 웹에 없던 것은 State 설치의 SP 정리(인트로 1,280·메인 메뉴 1,280·미션 로드 1,280회)·메인 메뉴 설치의 구름 구축(801회)·주변음 기한 재설정이었다. `netstorm.js` 에 `_installIntroStateEffects`·`_installMainMenuStateEffects`·State7 본문의 정리(`_clearPlayerSpForState`)를 더하자 **90프레임 웹 난수열이 cpppj 와 6,851회 모두 같고, 논리 화면은 `load.png` 와 다른 픽셀이 0개이며, Session 덤프는 `random` 행을 포함해 시각·주소 의존 21행(`frame`·`regular`·`object:15000`·`slot:15/15000`·`pool`·`player`·`sp 1`)만 다르다**([근거](docs/webpj-state-install-rng-20261123.md)). 남은 것은 게임 시각 정지 1프레임(0.025초)·프로세스 기한 3프레임 차이, 미션 이탈 뒤 State 3 재설치(정리·팔레트·주변음 기한)의 연결, `later.tsv`(130프레임) 대조였다. **E1281 진행:** 시각 차이의 원인은 (1) cpppj 고정 실행의 시계 출발 0.1초(첫 프레임 0.125)와 (2) 웹 앱에 없던 **월드 진입의 Guide 미션 정지**(원본 `LoadMissionWorldBody` 의 `PauseMissionWorldEntry`; 이전에는 첫 안내 창이 뜰 때까지 4프레임 동안 게임이 흘렀음)였다. 둘을 맞추자 웹 타임라인이 cpppj 와 프레임 단위로 같고(n=4 에서 9.2 정지, 관문 `lockFrames` 10→0 이 n=5~9), **90·130프레임 Session 덤프(`load.tsv`·`later.tsv`)가 기준선과 `sp 1`(소유자 0 미초기화 난수)·`player:0~8`(+1·+2 바이트 주소 잔재, Player 1 은 +0x68·+0x69 동적 Deck 필드)만 다르다**([근거](docs/webpj-state-install-rng-20261123.md)). Tutorial1 의 L2·L3 는 이 두 가지 외에 닫혔다. **E1282:** 같은 기준선 실행 파일로 1366×768(16:9)을 다시 수집해 대조하니 웹 16:9 도 90프레임 화면 0픽셀·덤프 `sp`/`player` 행만 다름이다(`tests/fixtures/cpppj/candidate-225553df/tutorial1/oracle-1366x768/`, `e2e/fixed-time.spec.js`). 월드 진입 정지로 Guide 미션의 첫 창 순서가 원본과 같아졌다(Fury War: 시작 안내 창 → 1초 뒤 원소 선택 창; 전체 e2e 162개 통과·1개 실패는 이 순서를 따르도록 시험을 고쳐 통과).
  - 완료: 판번호 1 덤프 행별 비교와 SP 소유자 0 제외(E870), 원본 테마 기본값으로 안쪽 613개 일치·경계 45개는 원소 후보 구간 내(E873~E874), 실제 앱 `sessionDump()`로 시작 직후 덤프가 시간 의존 값(시계·난수·정기 시각·풀 해시·애니메이션 프레임·앰비언트 바람 객체)을 빼고 같음(E1129, `e2e/l2-tutorial1.spec.js`), 안내·진행 창 지도 영역 배치(E1130~E1131). [E870](docs/webpj-session-dump-compare-20261119.md) · [E874](docs/webpj-tutorial1-border-candidates-20261119.md) · [E1129](docs/webpj-l2-tutorial1-20261121.md) · [E1130](docs/webpj-tell-window-map-view-20261121.md)


#### 5-A 렌더러·Draw·시각 시스템 (44/46)

- [ ] **ClientRendererFrame** — 미션 화면 출력의 원본 순서는 앱에 연결했고 메인 메뉴 구름 배경만 남았다.
  - 완료: 지형 수집 범위·행 한도 90행(E978), 지면·객체·입자·커서 수집과 원본 정렬·출력(E979~E980), Screen DIB·구름·Renderer 합성(E981), 표시 준비·Present·미션 화면과 임시 Tell 출력(E982~E984), Gump kind 2 수집·출력과 월드 부착 투영(E988~E989), Gump 이동 dirty(E1081), 도킹된 지도 영역(E1082).
  - 남음(E1263 점검): 메인 메뉴 구름 배경을 정적 GIF 타일(`ui/main-menu.js:202` 의 `surface.tile(clouds)`)에서 Renderer 의 Kencloud 출력(`ClientKencloudGlobals.draw`, 원문 `UberGump.cpp:2059`)으로 바꾸고 M1 구름 차이 37,054픽셀(4:3, `tests/fixtures/cpppj/m1-scenarios.json`)을 없앤다(미션 쪽 구름은 이미 Kencloud). GameWorld(4D-6a)·RawMenuControls(E1252)·ClientRendererFrameGlobals(앱 소유·사용, E1223)·GumpVisual(E1214)은 끝났고, 날씨 색(`tint`)은 웹 화면 모드에서 도달하지 않으며 진단·Meta·Rig 표시(`special` 개발자 관문·Meta 수신기 전용)는 웹 범위 밖이다. 채팅 창 출력은 E1169~E1179에서 연결했다(지식 HUD 공장은 웹 범위 밖).
  - 근거: [E978](docs/webpj-renderer-collection-geometry-20261120.md) · [E981](docs/webpj-campaign-renderer-drawing-20261120.md) · [E984](docs/webpj-app-mission-display-frame-20261120.md) · [E989](docs/webpj-app-gump-world-projection-20261120.md) · [E1081](docs/webpj-gump-redraw-cost-trails-20261120.md) · [화면 문서](docs/webpj-screen-20261113.md)
- [ ] **E197·E198·E201 재대조** — 웹 조립으로만 확인한 시각 시스템을 실제 Session의 cpppj 시험과 대조한다. 7단계와 중복되는 시험은 한 번만 한다. **E1275 진행:** cpppj 가 읽는 파편 캡처 원본 관측 세 개 `temple-capture-x86.tsv`(신전 181행)·`generator-capture-x86.tsv`(제네레이터 180행)·`blocker-capture-x86.tsv`(타워 120행)를 manifest 에 올리고 웹 시험 `temple-shrapnel-native-fixtures.test.mjs` 가 읽게 했다 — 481행의 크기·기준점·전체 픽셀(2,684,495+1,844,370+1,317,660)이 모두 일치한다(E197 Shrapnel, 변형 확인, [근거](docs/webpj-temple-shrapnel-capture-20261123.md)). cpppj 시험이 참조하지만 웹 manifest 에 없는 fixture 는 158개이며(편집기·파일·CD·시작·음악 장치 등 웹 제외가 다수) **E1276:** 같은 계열 `wind-aviary-capture`(54행)·`plain-workshop-capture`(2,115행)·`altar-capture`(1,824행, 약 95MB)도 올려 모두 일치함을 확인했다(`temple-shrapnel-native-fixtures.test.mjs`·`altar-capture-native-fixtures.test.mjs`). 남은 후보는 `GameWorld` 스프라이트 렌더링 대조인 `golem-capture`(67행)·`support-color-capture`(199행)다. 남음: E198(EvilRay)·E201(Burning)의 `DamageableBurningSessionTests` 등 cpppj Session 시험 대조. (E1263 점검: 7단계 "전용 시험이 없었던 시스템"과 같은 일이다. Shrapnel·EvilRay·Burning 표시 시스템을 `DamageableBurningSessionTests`·`CampaignTempleShrapnel_Native181CapturePixels`·`CampaignSession_ActualDropIslandSupportAndParticles` 와 대조한다는 기록이 `docs/webpj-burning-visual-20261109.md:73`·`docs/webpj-session-particles-20261116.md:35~37` 에만 있고 대조한 시험이 없다. Session 은 E790 이후 앱에 설치돼 선행 조건이 풀렸고 원본 실행 없이 읽기·Node 시험만으로 가능하다.)

#### 5-B 카메라·HUD·미니맵 (12/13)

- [x] **UberHud** — (E1255 닫음) E1082에서 Uber 루트 교체 설치·HudDock 모드 3 도킹·화면 크기 변경 재배치·월드 해제 정리를 앱에 연결했고([좌측 HUD 틀](docs/webpj-uber-hud-left-dock-20261120.md)) E1169~E1179에서 채팅 창 공장(`createChat`)과 내용물(로그·입력줄·제출·수신·Tab 이름 완성·`.flash`)을 마쳤다([채팅 조립](docs/webpj-chat-window-assembly-20261122.md)). 지식(NSInfo) 공장(`createKnowledge`)과 HudNsInfoMeta·NativePlayerInfoInput·NativeNsInfoInput·HudPlayerButtonDraw는 멀티/메타 준비 창이라 웹 범위 밖이다([E1171](docs/webpj-nsinfo-scope-20261122.md)). E1224 감사의 표시 차이 여섯 가지는 도달성으로 판정해 닫았다: InstallHud 재설치(Reflow·SetPalette 같은 값 재적용이라 같은 결과), 배경 GIF·ChangeFortPalette(battlePal 은 HUD 생성 전 종료, 나머지 웹 범위 밖), SubmitMonitorChat(숨은 설정 키 monitor/alert 기본 0·대본·자산에 없음), 서버 뉴스·파일 로그 웹 제외 ([근거](docs/webpj-uber-hud-close-20261123.md)).
- [x] **RawMinimapControls** — (E1248 닫음) E1082에서 Gump 루트·월드 Pop/Unpop dirty·카메라 입력에 연결하고 Tutorial1 화면의 미니맵 칸을 원본 스크린샷과 대조했다(85×82픽셀 일치). 클릭·끌기·F5 복귀는 E1128에서 e2e(`e2e/minimap.spec.js`)로 고정했고([몽키·미니맵](docs/webpj-monkey-minimap-20261121.md)) geometry·blit 은 원본 x86 fixture 시험이 대조하며 우클릭 "Sky Overview" 도움말은 E1236에서 연결했다. 남은 항목이던 ToggleMinimap(명령 0x02000037)은 기본 입력에 키도 대본 사용도 없어 단독 미션에서 도달하지 않는다고 판정했고 SetPalette 를 설치 때 한 번만 호출하는 차이는 HUD 생성 전에 battlePal 전환이 끝나 무해하다. [근거](docs/webpj-minimap-controls-close-20261123.md)
- [x] **FloatingSpGump·RawProductionControls** — (E1257 닫음) E1032~E1035에서 생산 패널을 조립·앱 설치하고 E1074에서 건설 비용 표시 Gump를 커서·게임 시계에 연결했다. E1082에서 생산 패널을 Uber 루트의 직계 자식(폭 80·높이 H−80)으로 도킹해 좌측 사이드바 위치를 원본과 맞췄고 E1236에서 예산·생산 창 우클릭 도움말을 연결했다. FloatingSpGump·InventoryItemGump 행은 대응표 port다. E1224 감사의 남은 표시 차이는 E1257 에서 닫았다: 루트 idle 순서는 같은 프레임 안의 순서뿐이고, UpdateContents 뒤 화면 갱신은 새 다리 조각 도착 때 격자가 바뀌는 픽셀 e2e 로 확인했고, 격자 부모 재바인딩은 단독 미션에서 기술 링크가 불변이라 도달하지 않으며, HUD 모드 이벤트 0x0200003d 는 원본도 설치·HudDock 두 곳뿐이다([근거](docs/webpj-production-controls-close-20261123.md)).
- [ ] **Cursor·RawCursor 표시** — 선행 커서 부품을 실 화면에 연결한다. 실제 카메라 투영·가장자리 스크롤·마나 볼트 정기 표시를 함께 대조한다. Cursor 행은 대응표 port이고, CampaignCursorClientEffects 행은 E1256 에서 닫았다(closePopups 뒤 화면 갱신은 픽셀 e2e, 한국어 번역 폭은 같은 FontSet.encode 시험, 개발자 키 전용 drop.record 는 도달 불가, [근거](docs/webpj-cursor-client-effects-close-20261123.md)). 이 체크는 연결(Cursor 계열 행 전부 port)과 가장자리 스크롤(E990~E1001)·마나 볼트 Session 연결(`raw-campaign-session-arrivals.js`)을 확인했고, 카메라 투영·커서·마나 볼트 정기 표시의 실제 화면 대조(미션 화면 픽셀 대조 자료가 아직 없음)가 남아 열어 둔다 — 5-C 의 M2/L4a 와 같은 일이다(E1263 점검).

#### 5-C 게임 화면 허브·미션 진입 (4/6)

- [x] **Uber 허브** — 실제 Client 등록표의 serial·현재 HUD 판정(E914), 공통 구분선 생성·입력·부착(E916)과 프레임 방송 분배(E917), UberRootGump 전체 생성·교체의 원본 160행 대조(E912), `ClientUberHud`로 구분선·미니맵·생산 패널을 소유하는 Uber 루트의 앱 설치(E1082)를 마쳤다. 채팅 창(E1169~E1179)까지 연결해 HUD·덱·미니맵·채팅을 소유한다(지식 NSInfo 공장은 웹 범위 밖, E1171). Meta Sprite 두 단위는 E1203에서 제외로 판정했다. E1224 감사가 남긴 0xb0 배경 GIF 메시지 자식과 NSInfo 자식은 E1243에서 웹 범위 밖(웹 메인 메뉴가 배경을 직접 그리고 미션 HUD title=0)으로 닫았다([판정](docs/webpj-uber-root-background-scope-20261123.md)). [E912](docs/webpj-uber-root-construct-20261119.md) · [E914](docs/webpj-uber-root-lookup-20261119.md) · [E916](docs/webpj-app-hud-divider-20261119.md) · [E917](docs/webpj-app-gump-broadcast-20261119.md)
- [ ] **메뉴·버튼 컨트롤** — 남은 행: RawButtonControls·RawMenuChat(E1224 감사의 표시 차이 중 우클릭 도움말은 E1253 에서 연결, 남은 것(E1263 점검): ① 채팅 로그를 미션 끝에 초기화(원본은 Client 수명 동안 유지, 웹은 문서화된 적응 E1170) — 고칠지 적응으로 인정할지 **사용자 결정 필요**, ② `storm-power-refresh`(원문의 Push/Attach 재부착)를 앱이 쓰지 않음(`refreshSp` 는 `invalidateAll+menu.refresh` 로 대체, 예산 칸과 겹치는 Gump 가 없어 눈에 보이는 차이는 없을 가능성, 확신 낮음). 알림 serial 0 고정·도우미 점멸 없음은 cpppj 에서 채팅 패킷 발생원이 사용자 say 뿐이라 도달하지 않는다(커밋 `f66401a24`), [근거](docs/webpj-chat-helper-help-20261123.md)). RawMenuHtml은 E1230에서, RawMenuControls는 E1252에서(Tell 모달 스택·제목 사건·request/closed 사건 도달성 판정, [근거](docs/webpj-menu-controls-close-20261123.md)) 닫았고 RawMenuLoading·RawTextEntryControls는 대응표 port다.
- [x] **미션 진입 명령** — (E1263 닫음) MissionBegin·StartDemo·환영 창 명령을 실제 Fort 요청·State 전환·Session 생성에 연결한다. 캠페인 목록 31개 미션의 시작(E985·E1010~E1016·E1157)과 Demo 목록·Auto-Demo의 StartDemo(E1092)는 앱에서 연결했다. 환영 창 명령은 `[WELCOME]`→`Tell Welcome2`→`Play Early Missions`=`MissionBegin,Tutorial1` 으로 `_onDialogCommand`→`_beginCampaignFromDialog` 에 이어지고(`tests/startup.test.mjs`·`e2e/first-run.spec.js`; "Play Early Missions" 를 실제로 눌러 Tutorial1 이 시작되는 e2e 는 없고 같은 경로의 다른 Tell 항목 e2e 만 있음), 취소 명령은 로딩 창 Cancel=`LeaveBattle`→`_leaveCampaignMission`(E1085·E1231), 원본 Gump serial 은 웹 TellWindow 어댑터라 재사용이 없고 관찰 동작은 E1252 판정으로 같다. [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md) · [데모](docs/webpj-demo-start-20261120.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)
- [x] **허브 fixture** — (E1265 닫음) 3.8절의 채팅·HTML 항목을 연결했다. HTML 은 E1192 에서 웹 범위 밖으로 닫았고 채팅 허브 연결은 E1169~E1179 에서 끝났다. 남았던 원본 관측 두 개는 manifest 에 올려 `tests/chat-hub-native-fixtures.test.mjs` 가 읽는다: `chat-input-recipients` 720행은 모두 일치(변형 두 가지로 확인)하고, `chat-callbacks` 29행은 반환 0 대조와 웹 적응 판정(로그 파일 쓰기·quit 큐 예약은 브라우저에 대응이 없어 제외)으로 닫았다. [근거](docs/webpj-chat-hub-fixtures-20261123.md)
- [ ] **M2 / L4a** — 4-D·5-A·5-B 조립 뒤 Tutorial1 진입 직후와 N프레임 뒤의 논리 화면 차이 0을 확인한다. 영어 4:3부터 16:9/16:10과 한국어로 넓힌다. (E1263 점검: 선행 조립은 대체로 충족했다. E1271 에서 고정 시각 모드를 구현했고 **E1278 에서 웹 90프레임 화면 대 `candidate-225553df/tutorial1/oracle/tutorial1/load.png`(1024×768) 대조를 시작해 다른 픽셀 7,195개까지 줄였고, E1280 에서 State 설치의 난수 소비를 맞춰 영어 4:3 첫 안내 창 화면의 차이를 0으로 만들었다(`e2e/fixed-time.spec.js` 가 0을 고정).** `later.tsv`(130프레임)는 덤프를 E1281 에서 맞췄고 **16:9(1366×768)는 E1282 에서 같은 방식으로 0픽셀임을 확인했다.** 남은 것은 130프레임 논리 화면 대조(`later` 시나리오는 화면 캡처 없이 보고서만 있음)와 16:10(cpppj 는 1280×800, 웹은 높이 768 기준이라 크기가 달라 직접 대조 불가)·한국어로 넓히는 일이다.)

완료한 체크(머리글 수에 포함): **메인 메뉴·타이틀 상태**(E1184, NativeIntroState·TitleBitmapLifetime·NativeMainMenuState와 클릭하여 시작 뒤의 title 그림 세 장, [문서](docs/webpj-title-screens-20261122.md)).

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (4/8)

- [x] **입력 본체**(UserInput 본체 순서·관문·명령 값은 E1223 읽기 전용 감사로 닫음) — (E1263 닫음) UserInput의 상위 메뉴(E1084 ESC 메뉴막대)·브라우저 이벤트(E919~E934, `e2e/browser-input-poll.spec.js`)와 특수 진단의 실제 로그 장치(`netstorm.js` `traceObject`→`traceClientObject`·`writeDiagnosticLog`, `campaign-game-world.js` `traceSpecialPointer`; 원문도 `player.special` 개발자 플래그일 때만 부름)를 연결했다. 편집 우클릭 메뉴·모드 1/6 좌클릭은 대응표 UserInput 행이 웹 제외로 기록했다. 폴링·우클릭·단축키·실제 후보 선택 및 일반·이동 모드 좌클릭을 확인했다. NB20의 실제 조각 취득→P 취소는 Session에서 검증했다. [E802](docs/webpj-campaign-user-input-movement-click-20261118.md) · [E803](docs/webpj-campaign-user-input-primary-click-20261118.md) · [E804](docs/webpj-nb20-real-cursor-cancel-20261118.md) GameWorldInput 코드 이식(선택/강조·미니맵 이동과 객체 메뉴·이동 커서, 대응표 `GameWorldInput.inc` `port`)은 완료했다. [E795](docs/webpj-campaign-game-world-input-20261118.md) · [E796](docs/webpj-campaign-game-world-menu-cursor-20261118.md)
- [x] **게임 메뉴** — (E1264 닫음) E1084에서 RawGameMenuBar·RawGameMenus(웹 범위)를 실제 Client 메뉴 작성기에 연결해 ESC 가로 메뉴막대와 우측 상단 토글 버튼, Game·View·Options·Players·About 하위와 연결된 명령을 완성했다([게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md)). 이후 Restart/Leave(E1085)·F9 플레이어 목록(E1087)·F6 지식 창(E1088)·Pause(E1089·E1163)·F7 표식(E1090)·송금·동맹(E1126)·F3 구분선 전환(E1159)·한국어 Player 이름(E1166)·Send X 숫자 입력 창(E1168, [송금 창](docs/webpj-send-money-dialog-20261122.md))·F3 채팅 창 내용물(E1169~E1179)·F1과 Tell/브리핑 창(E1235)을 연결했고, Ignore chat·Whisper To·정보 보기는 단독 미션 도달 불가로 판정했다(E1167). ESC 로 Tell 창 닫기는 E1264 에서 고쳤다(원문 `UberGump.cpp:821`·`:1810~1823` 의 Dialog 단계 규칙, [근거](docs/webpj-tell-escape-20261123.md)). Tell 모달 스택은 E1252 에서 도달성 판정으로 닫았다([다음 작업 1](#next) ⑥).
- [ ] **객체 우클릭 메뉴** — CampaignObjectMenuClient의 건물·운반체·특수 대상 메뉴와 소유자 명령 연결.
  - 완료: 중첩 메뉴와 사제·Player·공통 객체·운반체 메뉴, 사제 우클릭 건설(E1064~E1072), 워크샾 우클릭 생산·업그레이드(E1076~E1079), 신전(AI 메뉴·지식)·배터리(Meltdown!)·발전기(Storm Power) 등 파생 본체 일곱 주소(E1127), UI 밖 좌·우 누름으로 팝업 닫기(E1135), 비활성 하위 행의 `›`와 메뉴 비활성·그림자 색 (47,24,17)(E1136), 제단 받침 우클릭의 같은 좌표 제단 조회·위임(`Altar Level I`, E1137), 운반체(Bulf·Walker, 열기 4268c0)·Archer 계열(열기 41e660) 우클릭 열기(E1138).
  - 남음(E1263 점검): 폭탄·Turn To Obelisk! 같은 특수 행을 가진 Archer 계열의 실제 Chromium 우클릭(완성된 발사기=TYPE 71 의 우클릭→공통 메뉴 Salvage 는 `english-tutorial2.spec.js` 가 이미 확인한다, E1190·E1263 재점검)과 내용물을 실은 운반체의 Carrying/Drop 실제 동작 확인(열기·작성은 Node 시험 `carrier-menu.test.mjs` 로 확인했고 Drop 명령 소비 e2e 는 사제 대상이다). Factory 파생은 VT `0x509148`(앱 연결)과 `0x509210`(멀티 Outpost 래퍼) 둘뿐이고, 특수 대상 메뉴는 Meta·편집·AI 디버그 전용, 채팅 분기(Ignore chat·Whisper To·정보 보기)는 단독 미션에서 만들어지지 않아 도달하지 않는다(E1167).
  - 근거: [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) · [워크샾 생산](docs/webpj-browser-tutorial2-factory-20261120.md) · [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [메뉴 닫기·건설 미리보기](docs/webpj-context-menu-close-build-preview-20261121.md) · [메뉴 비활성 색](docs/webpj-menu-disabled-color-20261121.md) · [제단 메뉴](docs/webpj-altar-menu-delegate-20261121.md) · [운반체·Archer 메뉴](docs/webpj-carrier-archer-menu-open-20261121.md)
- [ ] **사제·운반·경로 바인딩** — 남음(E1263 점검): 운반체 줍기·내려놓기의 실제 앱 조작 확인(본체·Session 은 `raw-campaign-session-motion-runtime`·`-carrier-drop` Node 시험), cpppj `CampaignMotionTests.cpp:120~126` 저장 미션 경로 시험의 L3 대조. MetaMap 은 도달하지 않고(E1188), 저장 미션 두 개의 전체 초기화는 31개 시작 e2e·`raw-campaign-session-load.test.mjs`(출하 Fort 32종)·`tutorial-menu-start.spec.js`(MasterOfWhirligigs 실제 진행)로, 경로 루프는 `mission-priest-move.spec.js`(E1139)·`raw-campaign-motion-bindings.test.mjs` 로 확인했다. 완료: 캠페인 Session 운반 입력과 일반 사제 이동(E794), 실제 마우스의 사제 선택→이동 커서→땅 클릭 이동(E1139 e2e), 단독 미션 제단 희생 메뉴 훅(CampaignUserInputServices, E1223), 객체 메뉴의 사제 기도·Cast/Drop·Factory 순환 명령 소비(E1225, netstorm.js 분배·Session `commandPriest`/`commandCarrierDrop`, 시험은 raw-campaign-session-load.test.mjs·e2e/mission-object-commands.spec.js), 브리핑·완료 창의 `Review Knowledge`(ShowTechnology, 원본 스크립트 313개)·`GetTechnology`(101개) 단추와 전투 모드 기술 획득의 Fort snapshot 두 장치(`fortMission`·`fortBadges`, E1227). [E794](docs/webpj-session-carrier-input-20261118.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md)
- [ ] **건설·생산·경제** — 생산 유지부터 실제 화면 생산 UI까지 진행 중이다.
  - 완료: RawProductionMaintenance 원본 86행 대조와 Session 연결, ClientProductionSync·ProductionControlContext·ClientBridgeDecay(E831~E835), 앱 생산 유지·다리 공급·생산 단축키·다리 배치(E1002~E1008), 생산 격자·항목·예산·패널 조립과 앱 설치(E1019~E1035), 회수 명령(E1047), 비용 표시(E1074), Factory 메뉴와 생산·업그레이드 명령(E1076~E1079), HUD 다리 조각 집기→미리보기→놓기·사제 건설 위치 지정(신전·워크샾·제단)·제단 회수의 실제 마우스 e2e(E1139), 다리 붕괴 검사의 앱 Kernel 프레임 연결(E1230).
  - 남음(E1263 점검): 다리 조각 연속 배치의 실제 확인(발사기 연속 배치는 Tutorial2 e2e 로 확인했고 다리 조각은 한 조각 e2e `mission-build-placement.spec.js` 뿐), Tutorial2 외 미션의 자연 완주(1절 사용자 확인 범위). HUD의 주기 호출·실제 Client 메인 루프(E1230~E1234)·건설 장치의 입력 소비자(ClientConstruction* 행 port, E1205)·실제 wire 배치/확정(E1139 e2e)·SP·덱/생산(E1019~E1035)은 끝났다. RawBuildReplay·ClientBuildReplay의 FILE/최신 rec·Host·Ctrl+F7 은 개발자 전용이라 E1242에서 웹 범위 밖으로 닫았다([근거](docs/webpj-build-replay-scope-20261123.md)).
  - 근거: [E831](docs/webpj-production-maintenance-20261119.md) · [E835](docs/webpj-client-bridge-decay-20261119.md) · [E1003](docs/webpj-app-production-frame-20261120.md) · [E1008](docs/webpj-browser-bridge-placement-20261120.md) · [E1035](docs/webpj-app-production-panel-20261120.md) · [E1079](docs/webpj-browser-tutorial2-factory-20261120.md) · [건설 기록 재생](docs/webpj-build-replay-runtime-20261116.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md)
- [ ] **M3 / L3** — Tutorial1 이동·수확·건설·생산의 입력 단계표로 cpppj 상태·화면을 대조한다. Tutorial1·2와 캠페인 1-1·1-2의 브라우저 검증은 끝났다(`l2-tutorial1`·`english-tutorial2`·`korean-mission`·`tutorial-menu-start`·`mission-priest-move` e2e, E1263 점검). 남음: 고정 시각 모드와 `later.tsv` 로 L3 단계표 대조(4D-10b 와 같은 선행; cpppj 오라클 수집은 창을 띄우므로 AGENTS 실행 확인 규칙을 적용한다, 3.3절).

완료한 체크: **공장·지식 메뉴와 기술**(E1076~E1079 Factory 생산·지식·업그레이드, 연구 명령 0x03000080 소비, E1189 `<$CheatAllTechnology,원소>`로 Fury War 기술 지급; 제단 업그레이드·`Get 5,000` 행의 HTML 기술 목록은 비미션 전용이라 웹 범위 밖, [E1180](docs/webpj-ritual-reward-scope-20261122.md) · [E1189](docs/webpj-fury-war-technology-20261122.md)).

#### 7 전투·의식·AI (3/6)

- [ ] **전투 바인딩** — Hit·GunQueries·Archer·Motion·BombBlast·Damageable 표와 실제 저장 미션 Session/Kernel을 대조한다. Archer의 초기 배치·비행 및 factory/clock loop는 1절의 검증 범위를 적용한다. (E1263 점검: 여섯 표 행이 모두 port 이고 Session 에 결선돼 있으며(`raw-campaign-session-{hits,guns,gun-views,combat,flyers,damageable}.js`와 각 Node 시험) 실제 Chromium 에서 AI 혼전이 약 18분 오류 없이 돈다(E1262). 남음: 전투 장면의 cpppj N프레임 대조 — M3/L3·AI 세션 결선과 같은 일이다.)
- [ ] **AI 세션 결선** — Start의 일반/데모 AI·동맹과 typed 프레임은 확인했다. 현재 Player·SP·명령·난수 및 장기 RawSurface 소비자도 연결했다(`raw-campaign-session-ai*.js`·`netstorm-campaign-ai.test.mjs`, E1122·E1124·E1198·E1206~E1216). 남음(E1263 점검): AI 미션의 cpppj N프레임 L3 비교와 DestroyPatch 진단 12지점을 표시 후 계속으로 바꾸는 일(4D-8a).
- [ ] **전용 시험이 없었던 시스템** — E197·E198·E201 등을 실제 cpppj Session 시험과 대조한다(5-A와 연계). 5-A "E197·E198·E201 재대조"와 같은 일이라 한 번만 한다(E1263 점검).

완료한 체크: **파괴·주문 효과**(E1199, DetonateInput 원본 관측 768행·RawObjectConversion 128+36행), **의식**(E1180, 보상 선택 UI는 비미션 전용이라 웹 범위 밖), **AI 잔여·전투 이탈**(E1198, AiTypeRequiredEnergy 실제 Session 연결·RawBattleLeave 부품 이식). [의식 판정](docs/webpj-ritual-reward-scope-20261122.md)

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (8/9)

- [x] **대본·대화창** — (E1263 닫음) DialogScript·DialogBodyLayout·ScriptCommand·DialogCommandLifetime 행이 모두 port 이고 현재 미션의 안내·채팅·대화 소비자는 E1231 감사가 동등을 확인했다(`bc7sun_bars76` 의 `BC1Menu` 는 E1245). 대본 82개의 스크립트 명령을 모두 대조해 `ViewSpot`(Run For It 시작 카메라)·`Salvage`(회수 경고 창)·`CheatAllTechnology`(Fury War)를 연결했고 나머지 미연결 명령은 개발·멀티플레이·플랫폼 전용이다([E1190 점검](docs/webpj-script-commands-audit-20261122.md)). Tutorial1~6과 캠페인·데모·훈련 미션 한국어 대본(E1056·E1062·E1108~E1116)과 Tell 그림(E1063)은 연결했다.
- [x] **소리 본체** — (E1261 닫음) Sound·SoundDevice·ClientAudio를 Web Audio로 적응한다. E957~E966에서 효과음 공개 자산·선별 적재·원본 위치 음향·SoundPlayer·감쇠 파일 선택·Web Audio 장치·사전 해독·앱 설정/Session 재생·재생 수를, E968에서 최초 Fort 요청의 음향 자동 준비를, E997에서 경로 발소리를 연결했다. E1239에서 Sound On 끄기의 음악 무음·켤 때 현재 곡 재시작과 천둥 효과음의 SoundPlayer 경로를 원본과 맞췄다. 실제 청취 확인은 사용자 확인 범위이다(Wind Noise 는 E1153 분석으로 원본 기본 설치에서 무동작임을 확정해 값 저장만 유지한다).
- [x] **음악** — (E1258 닫음) 완료: 공개 자산(E957)·선별 적재(E959), 장면 음악 선택·기한·날씨(E1059, 독립 3,113행 대조), 승리·패배 음악 1회 재생(E1061), 미션 State7·프레임·떠나기의 장면 음악(원소 곡 무작위 순환·다음 곡 미리 받기·해독 버퍼 놓기, 미션 중 메뉴 곡이 계속 나던 결함 수정, E1119), 희생 의식의 `sacrifice.mus` 요청(`services.requestMusic`, E1120). E1238에서 승리·패배 곡의 요청 연결을 단독 미션 도달 불가(원본도 틀지 않음)로 닫았다([판정](docs/webpj-result-music-scope-20261123.md)). E1258 에서 State7 첫 곡 선택이 원본처럼 공유 게임 난수를 한 번 소비하게 고치고(e2e 로 선형 합동 한 단계 확인) 소리·음악을 끈 동안의 곡 기한을 원문처럼 180초로 맞췄으며, 나머지(대기실 곡=멀티 전용, 원소 팔레트=숨은 설정 키, 번개 섬광=창 모드 no-op, 희생곡 유지=도달 불가, 희생곡 무음=비동기 해독 적응)는 판정으로 닫았다([근거](docs/webpj-scene-music-close-20261123.md)). 실제 청취 판정은 사용자 확인 범위이다. [장면 음악 구동기](docs/webpj-scene-music-driver-20261120.md) · [장면 음악](docs/webpj-scene-music-20261120.md) · [결과 음악](docs/webpj-result-music-playback-20261120.md)
- [x] **글자·설정·그림** — (E1261 닫음) BitmapFont 행은 E1261 에서 닫았다(한국어 큰 슬롯 크기·기준선 E1259, 굵기는 사용자 결정으로 보통체 유지, 확장 글자 폭 동일 확인, 일본어 대표 전각 폭은 13차, [근거](docs/webpj-korean-title-legibility-20261123.md)). KoreanResources·ClientOptionsDisk·GifImage는 대응표 port이고 GifScreenshot(Shift-F7 화면 저장)은 E1100에서 브라우저 내려받기로 완료했다. [화면 저장](docs/webpj-screenshot-gif-20261120.md)
- [x] **적응 fixture** — (E1263 닫음) 3.8절의 오디오·글꼴·렌더러·음악 관측을 판정했다: `renderer-projection`(`renderer-collection-geometry.test.mjs` 가 90행을 비트 단위로 대조)·`scenemusic-x86`(`scene-music-native-fixtures.test.mjs` 3,113행)는 소비 시험이 있고, `audio-reinterpret`(Web Audio 적응, ClientAudio 행은 adapt-sound 로 port 종결 E1261)·`font-face-enum`·`font-face-lookup`(GDI 글꼴 열거는 브라우저에 대응이 없고 FontFaceEncoding 은 exclude-host)은 웹 적응·제외로 판정했다.
- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.
  - 완료: 캠페인 목록 31개 미션의 연속 시작(E1121, Priest Training 9개는 E1157에서 실제 연결하고 Auto-Demo가 대신 돈 e2e 거짓 통과를 정정), 캠페인 16개 장시간 진행과 AI 시점 미연결 서비스·좌표 오류 네 가지 수정·소리/안내 서비스 여섯 개 연결(E1122), Early Missions·Priest Training 15개 100초 진행(E1123)과 Priest Training 9개 150초씩 26분(E1158), 스커미시 AI 7명 300초(E1124), 데모 420초 진행과 360초 재시작(E1125), 후반 4개·중반 13개 장시간 진행으로 시계 정지 누수 수정(E1132~E1133), 몽키 시험(E1128·E1134·E1165, 죽은 Gump 정리 수정 포함), 승패 안내 뒤 흐름과 진행 저장·다음 미션 해제(E1152·E1161).
  - 남음: 실제 자연 승패에 따른 진행 저장·자연 완주, 한국어 전체 확인. (E1263 점검: 둘 다 사용자 확인 범위(L5·3.7절 6차)라 코드로 더 줄일 일은 없다.)
  - 근거: [모든 미션 시작](docs/webpj-all-missions-start-20261120.md) · [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md) · [시계 정지 누수](docs/webpj-mission-leave-pause-20261121.md) · [Gump 정리](docs/webpj-gump-cleanup-frame-20261121.md) · [몽키·미니맵](docs/webpj-monkey-minimap-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)

완료한 체크: **도움말·Tell**(QuickHelpWindow·TellFileMenus E1197, ExternalHelpClient E1181, HtmlLinkText E1212, QuickHelpControl E1237, QuickHelpHtml·QuickHelpRequest E1240까지 해당 대응표 행이 모두 port, HtmlConditionEffect·HelpLinkPolicy 대응표 port, RawNsInfoContext 는 NSInfo 범위 밖 제외 E1182; 생산 메뉴 Help·TYPE 정보 본문·요구 에너지 그림·도움말 그림 E1046·E1048~E1055, 링크 이동·Back 의 TYPE 조회 E1229, HUD 우클릭 빠른 도움말 E1236, 월드 없는 메인 메뉴 Help 의 TYPE 정보 E1240; [근거](docs/webpj-main-menu-help-type-20261123.md) · [E1197](docs/webpj-help-tell-leftovers-20261122.md)), **동영상**(E1202 사용자 결정, CD 전용 인트로 영상을 웹 빌드에서 제외하고 다섯 단위를 이식 대상에서 뺌, [결정 기록](docs/webpj-intro-movie-decision-20261122.md)), **미션 수명**(튜토리얼 완료·정지·Retry/Next/Abort·메뉴 복귀·진행 저장·데모 반복, E1085~E1161; 자연 승패 뒤 저장→새로고침→해제는 L5 사용자 확인, [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md)).

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다. E945에서 Windows의 Chromium·Chrome·Edge·Firefox로 하위 경로의 정적 모듈·자산 로드와 같은 URL 새로고침 뒤 음량 설정 복원을 확인했다. HTTP 캐시, GUI Linux와 실제 미션 진행 저장은 별도 검증한다. [E945](docs/webpj-static-deployment-20261119.md)

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 기준선 cpppj의 `session-dump`·`--fixed-frame-ms` 수집 시나리오(E868~E869)와 웹 덤프 형식·수집(E865~E867)은 갖췄고, 웹 덤프와의 행별 비교·L2 판정은 4D-10b에 남았다. L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 제품은 1326(1327 M8 관문)이며 웹 비교 기준선은 제품 1313이다. 1314 최초 실행과 1320 미션 전환을 웹에 반영했다(E616·E620). 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목(1316~1319·1321·1324)은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 소스 영향 판정은 3.6절에 있고, 1323 아이콘은 favicon(0.6절)으로 따로 반영했다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·특정 적 공격·승리·전체 화면/음향은 1절 범위에 따라 확인한다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- 웹 링크 브라우저 검증은 E942에서 마쳤다. Chromium에서 외부 출처 탭의 `opener`와 `referrer`가 비어 있고, 브라우저가 팝업을 차단해도 현재 문서가 유지된다. [E942](docs/webpj-external-access-20261105.md#71-브라우저-검증)
- 와이드 전투 시야와 카메라 재배치를 연결할 때 16:9의 논리 크기 전환도 검증한다. WebMCP 대상 브라우저·조작/캡처/녹화 범위는 착수 시 확정한다.

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다. 2026-11-23 정리 때 `node webpj/tools/port-map.mjs`는 9행(대응표 밖 새 파일 5개 별도)을 보고했고 E1241에서 모두 판정해 base를 `63590635`·`710b9647`·`baf141eb`로 올렸다(보고 0행). 1322(`635906357`)의 BitmapFont·ClientMain·KoreanResources·UberGump 4행과 1325(`710b9647b`, Reset All)의 ClientMain·ProductPresentation·RawGameMenus·UberGump 4행, 1326(`baf141ebe`, 테스트 AI 진단)의 RawCampaignSession·SessionStateDump·o/RawDamageableHit 3행 가운데 ② 재반영은 RawDamageableHit 의 파생 피해 진단 문구 하나이고(`core/raw-damageable-hit.js`), 나머지는 ① 웹 영향 없음이며 ③은 BitmapFont(8단계, 다음 작업 1 ⑤)와 Reset All 게임 안 확인 창(다음 작업 1 ⑧, 결정 대기) 둘이다. 새 파일 EmbeddedBitmapFont·KoreanFontSettings·ResetAllSettings 다섯 개는 모두 ①이며 재기준선 때 등록한다. [판정표](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md) SoundMusicSelection 행의 base는 기준선보다 앞선 `50ca1acf`였으나 그 뒤 두 커밋(`225f2322c`·`07885f694`)이 `SoundMusicSelection.h`의 주석·멤버 설명만 바꾼 것을 확인해(웹 영향 없음) 2026-11-22에 base를 `225553df`로 바로잡았다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | (E1263 점검) 미션 이탈 때 State 정리 효과(ChatChannels.resetOccupied·시간 동기화 초기화는 E1268 에서 옮기고 MapRevision·선택 해제 통지는 영향 없음으로 판정, 4D-5b), Client 기본 수신기 표의 앱 설치(4D-3b), 분리 Poll·`pollClientConnectionHealth` 미호출(단독은 하트비트 외 관측 효과 없음으로 확정, 4D-7b), RawClockSynchronization 의 CRT 동률 RTT 동등성(원본 필요). Client 진행 창·GameClock 바인딩·RawIslandRuntime 소비자는 끝났고 PacketDelays(실소켓)·RawPacketCopies(LAN)·RawMonitorTime(옵션 UI 가 없는 숨은 키 `monitor`·`alert`)은 도달하지 않는다. 아비어리·공용 Form writer 경계는 E594·E1210 으로 대부분 닫혔고 확인하지 못한 칸이 남았다(확신 낮음). |
| 4D-8 | (E1263 점검) RawSurfaceDeletion·Lifetime 의 DestroyPatch 진단 12지점·`rememberPlacement`(4D-8a)와 ClientCampaignTransport 의 기본 수신기 앱 설치(4D-3b). 나머지(canonical 등록·고정 수신기 대여 E1193~E1194, 장기 Storage·ProcessHostStorage·Native 송신/Form writer·자산 삭제/후처리·보상·Native 조회기 E1205~E1221)는 port 로 끝났다. SidPool 소진 회수·진단과 GameAuxState 의 새 전역 칸은 코드와 행이 있으나 소비 여부를 직접 확인하지 못했다. |
| 5-A~5-C | (E1263 점검) 메인 메뉴 Kencloud 구름 배경, UberGump/메뉴 허브의 ESC Tell 닫기(E1264 에서 해소), ‘Creating Islands.’ 로그인 진행 안내(앱이 `creatingIslands` 훅을 소비하지 않음; 원문은 `RawMissionServerPackets.cpp:60-63` 의 `createArchipelago`(Zone 0x100 비트·이름 `$`) 경로에서만 부르므로 단독·스커미시가 이 경로를 타는지 판정이 남음, 확신 낮음). 메타 Sprite 소비 경계는 제외로 판정했다(E1203). |
| 6~7 | (E1263 점검) 건설 ActualConfirm/ActualPlace(E1139 e2e)·동맹 협상 입력(`registerAllianceNegotiationCommands`, `player-menu-actions.spec.js`)·RawGunHit Rain override(`raw-gun-hit.js`)는 끝났고 F7/Ctrl+F6 건설 기록은 개발자 전용으로 제외했다(E1242). RawPathRuntime/RawPathData 는 두 행 port 이고 사제·운반·경로 바인딩 항목이 이어받는다. NB20 실제 취소는 E804에서 검증했다. |
| 8~10 | (E1263 점검) TellSection(행 port, E461·E971)·Frame Rate·Difficulty·NB10(`frame-rate.spec.js`·`difficulty.spec.js`, E1096·E1149)·스커미시 일반 시작·중앙섬·자기 Fort(E1103~E1107·E1262)는 끝났다. 한국어/일본어 자원 우선순위·입력은 13차(일본어)와 한국어 사람 검수가 남았다. |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했다. Native 대여는 4D-8에 남는다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | E943에서 16:9 논리 화면을 최대 1920×1080까지 지원하고 창 크기 변경과 입력 좌표를 확인했다. 전투 카메라·HUD와 WebMCP 조작·화면 캡처·소리 포함 녹화가 남았다. |
| 5 | localStorage 설정·진행 저장과 오류 대응을 구현했다. 실제 미션에서 진행값이 발생하는 8단계와 15차 초기화에서 저장 수명을 확인한다. |
| 6 | 메뉴·Tell·도움말과 갈무리11, 튜토리얼·캠페인·데모·훈련 미션 대본 전부(E1108~E1116)와 게임 안 메뉴·객체 메뉴 문구(E1118·E1140~E1141·E1166)까지 구현했다. 생산 창·스톰 파워 도움말과 채팅 채널 이름 항목도 더했다(E1246, 미니맵 도움말은 원본이 번역하지 않음). 사람 검수·실제 화면 확인이 남았다. |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. 실제 미션 수명에서의 오류 진단이 남았다(프레임 예외 한 번에 렌더 루프가 멈추는 설계를 유지할지의 결정 포함, 4D-7b). [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | 10.78 알려진 버그 중 웹 적용분을 cpppj 판정과 대조한다. NB04/29 간헐천 계산·callback, NB18 요청/수신 관문과 NB20 실제 Session 선택/취소는 선행했다. 실제 스커미시와 브라우저 조작 검증은 남았다. |
| 11 | 쉬움/보통/어려움(기본 보통). 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반. E554·E555 계산과 AiCreate 연결, E1096 Options·ESC 메뉴 Difficulty 선택과 저장을 마쳤다. E1149에서 실제 스커미시로 난이도별 AI 다리·섬 건설 속도(쉬움 ≪ 보통 < 어려움)를 확인하고 옵트인 e2e로 고정했다([난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)). 쉬움의 채취 유닛 절반(최소 1)은 실제 진행에서 편차가 커 구분되지 않아(E1156, AI가 프레임 구동이라 빨리 감기 부적합, [빨리 감기](docs/webpj-fast-forward-clock-20261121.md)) 설정 단위 Node 시험과 AI 생성 연결을 근거로 둔다. |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천. Outpost 목록 수명과 분류, 순수 규칙(E1101)·Campaign 항목·설정 화면·저장(E1102)·시작 섬 fort 묶음(E1103)·섬 생성 부모 셋(E1104)·세션 월드 생성과 시작·항목 기본값 켜기(E1105)·어려움 AI 5분 진행(E1106)·금지 유닛의 미션 허용 표 반영(E1107)을 마쳤다. E1262 에서 AI 생산·공격 단계와 사제 사망 패배 안내를 실제 시간 관찰로 확인했다([관찰](docs/webpj-skirmish-watch-20261123.md)). 승리 경로(적 전멸)와 AI 다리 멈춤 시점의 원본 대조가 남았다. 비미션 너겟 배치·보충은 스커미시가 미션 모드(`missionPost` 참)로 돌아 도달하지 않고(E1270, `e2e/skirmish-world-mode.spec.js`), 간헐천은 실제 Session 에서 무작위로 놓임을 확인했다. [규칙](docs/webpj-skirmish-setup-20261120.md) · [UI](docs/webpj-skirmish-ui-20261120.md) · [fort 묶음](docs/webpj-skirmish-fort-bundle-20261120.md) · [섬 생성 부모](docs/webpj-island-generation-parents-20261120.md) · [시작](docs/webpj-skirmish-start-20261120.md) · [진행](docs/webpj-skirmish-play-20261120.md) |
| 13 | 일본어 등 추가 언어. WOFF2가 없는 글꼴은 Google Fonts CDN을 사용하고 외부 접속 문서와 소스 감사를 함께 갱신한다. 일본어 Patch Credits에도 카피라이트를 넣는다. |
| 14 | 웹 자체 JS 사용으로 QuickJS 탑재 불필요. |
| 15 | Reset All은 E863에서 구현·검증했고 E1251에서 확인을 브라우저 `confirm()` 대신 게임 안 Tell 확인 창(옵션 최하단 → `Patch1101.ResetAll` → 확정 단추 `Patch1101.ResetAllConfirmed`)으로 바꿨다. 확정 뒤 현재 게임 소유 설정·진행 키만 지우고 앱 정리 후 다시 로딩한다. 취소·확정·가드·오류·최초 실행과 다른 앱 저장값 보존을 검사했고 Chromium 실제 메뉴(영어·한국어)에서도 확인했다. 영구 저장 키가 추가되면 초기화 목록도 갱신한다. 일본어 본문은 13차에서 더한다. [E863](docs/webpj-reset-all-20261119.md) · [E1251](docs/webpj-reset-all-tell-20261123.md) |

- **12차 Outpost 잔여.** 배치/거리 후보 목록의 실제 소유 관계 확인, 지역 소유 통지·Factory/연구/운반·AI·전투·Client 연결, 건설·금지 목록·삭제를 확인한다. 일반 캠페인/데모에서의 제외도 확인한다. [Outpost 부품](docs/webpj-raw-outpost-lifecycle-20261115.md) · [이관 전 범위](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)

### 3.8 원본 관측 연결 이월

- [x] **입력·장치** — (E1269 닫음) — 원본 관측 장치의 앱 입력 루프 연결을 확인하고 있다.
  - 완료: `saved-camera-key`·`construction-history-key`(TAB·U) 분배기 연결과 저장 칸·이력의 최초 무효 초기화(E1154), D 키(E1155, Tutorial2 발사기로 종류 확인 E1162)와 F1·F2·Shift-F3·F8·Ctrl-F5 확인(E1155), F3 구분선 전환(E1159), `detonate-input`·`vortex-command-input`·`scenario-object-input`의 세션 `commandObject`(신전·Meltdown·RawObjectConversion·RawObjectCommand) 연결(E1150), `rotation-key-input`(C 키)과 `rotmode` 설정 동기화·저장(E1151), `pause-key-input`(E1163). 미연결 모듈 68개 중 `move-dialog`(웹 범위 밖)·`hud-ns-info`(멀티/메타)·`raw-theme-technology`(개발자 도구) 등은 E1163에서 분류했다.
  - 점검: `raw-priest-placement-*` 6개(사제 배치 검사 MayPlace 체인)는 건물이 파괴될 때 갇힌 사제를 풀어 주는 경로(`releaseContained` → `RawPriestSpawn` 의 점유된 후보 칸 판정)에서 앱이 한 번도 연결하지 않아 "사제 월드 배치 검사 몸체 미연결"로 던졌고, E1269 에서 `bindGameplayPlacement` 와 함께 Session 에 연결했다([근거](docs/webpj-priest-placement-session-20261123.md), `raw-campaign-session-gameplay-placement.test.mjs`, 변형 확인). 나머지 E1163 미판정 모듈은 판정을 마쳤다: `raw-battle-resource-frame`(E1232 앱 연결), `storm-power-refresh`(HUD 예산 글자가 매 프레임 SP 를 읽음, E1219)·`production-color-maps`(`client-production-assembly.js`)·`client-production-sync`(웹 생산 패널의 자체 UpdateContents; 확신 낮음), `audio-reinterpret`(E1239·E1261)·`font-face-enum`·`font-face-lookup`(GDI 글꼴 열거, 웹 제외). 다른 모듈이 import 하지 않는 모듈은 E1163 때 67~68개에서 줄었다(E1263 점검 56개, `raw-priest-placement-*` 연결로 더 줄어듦). 채팅 창 내용물(배경·기록·입력줄·제출·수신·이름 완성·섬광)은 E1169~E1179에서 마쳤다.
  - 근거: [화면 위치 키](docs/webpj-camera-keys-20261121.md) · [D 키·단축키](docs/webpj-production-d-key-20261121.md) · [객체 명령](docs/webpj-object-commands-20261121.md) · [회전 키](docs/webpj-rotation-key-20261121.md) · [Pause 키·미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md)
- [x] **렌더러·음악** — (E1263 닫음) `renderer-projection`·`scenemusic` 은 cpppj fixture 이름(`renderer-projection-x86.tsv`·`scenemusic-x86.tsv`)이며 `renderer-collection-geometry.test.mjs`(90행; 앱 호출처 `campaign-render-collection.js`)·`scene-music-native-fixtures.test.mjs`(3,113행; `netstorm.js`→`audio/scene-music-driver.js`→`scene-music.js`)가 소비한다.

완료한 체크: **채팅 허브**(E1169~E1175, `chat-callbacks`·`chat-input-recipients`를 실제 Client/RawMenuControls·RawChatRuntime과 연결, [채팅 조립](docs/webpj-chat-window-assembly-20261122.md)), **점 명령/HTML**(E1192, HTML `$Input=`은 전역 tell의 길드·관리자 섹션 전용이라 SaveFortNameDialog·RawMenuSaveFort의 남은 가지를 웹 범위 밖으로 판정, [근거](docs/webpj-html-input-scope-20261122.md)).

[원본 fixture 감사](docs/webpj-fixture-audit-20261115.md)

## 4. 남은 차이와 검증 한계

| 영역 | 후속 확인 |
| --- | --- |
| 지도·지형 | 중립색·가운데 카메라·고정 구름은 개발 미리보기 정책이다. 실제 지도에서 owner·원소 투표/themeMode·도장/flood·카메라 제한/스크롤·공유 난수·색 번호·팔레트/ColorSort와 `useBanners`를 확인한다. 절벽 외 0x10/0x18 지형 선택 및 회전2/3의 실제 저장 사례도 남아 있다. |
| edgeFarm·보상/SP | edgeFarm의 spots/finder·MayPlace·Retype를 실 월드와 맞추고, 보상/SP의 재시드·실제 지갑·알림/소리·보상/후처리 장부 공유는 4D-8·6단계에 연결한다. [지형 추가 관측](docs/webpj-terrain-extras-20261102.md) · [보상과 Content 수명](docs/webpj-content-lifetime-20261107.md) |
| 메뉴·입력 | 구름 비교 차이는 4:3의 37,054픽셀이다. Kencloud 연결 뒤 M1 시나리오를 갱신한다. Help/Options 위치·와이드 배치, Overlay 드래그/캡처·미션 커서는 미확인이다. Edge Scroll(가장자리 스크롤)과 Auto-Demo(메인 메뉴 45초 무활동)의 실제 동작은 E990~E1001·E1092에서 확인했다([가장자리](docs/webpj-app-edge-scroll-frame-20261120.md) · [데모](docs/webpj-demo-start-20261120.md)). |
| 도움말·Tell | `<info>` 머리글/능력치, `#`/`&` 맞춤·색 치환, mana.8~11의 실제 원본 F1 모습, `<background>`/nsisles 사용, `{@미션.키}`·스크립트 도움말 Back 및 필요 시 두 열 메뉴를 확인한다. 도움말 그림은 레이어0/그림자 없음 경계를 유지한다. |
| 소리·환경 | 실제 청취·위치 음향, 다양한 브라우저·GUI Linux·고DPI/모니터 이동은 미확인이다. |
| Credits | 영어 AI 문구 `OpenAI Codex, ChatGPT and Anthropic Claude Code`와 한국어 문구는 사용자 수정 지시까지 유지한다. 수정 시 `www/assets/product.js`와 `tests/product.test.mjs`를 함께 갱신한다. |

## 5. 검증과 완료 판정

| 단계 | 판정 |
| --- | --- |
| L1 모듈 | 원본 10.78 x86 fixture와 cpppj 검사로 반환값·슬롯·정수/float/double 비트·부작용·난수 순서를 대조한다. 체크섬 비교와 바이트 비교를 구분한다. |
| L2 시작 상태 | 같은 Fort·언어·옵션·시드 및 Load/Start 시점의 Session 덤프와 논리 화면을 비교한다. |
| L3 입력 진행 | 같은 프레임·입력 단계표의 상태·보고서·화면을 비교한다. 표시 주사율·비동기 자산 로딩과 게임 시계/난수를 분리한다. |
| L4 화면·소리 | L4a는 논리 화면과 cpppj snapshot, L4b는 소리 시점·곡 선택과 MCP 녹화를 비교한다. 전체 외관과 청취는 사용자 확인 범위다. |
| L5 자연 플레이 | 사용자가 자연 승패 → 새 완료값 저장 → 새로고침/재실행 → 다음 미션 해제를 확인한다. |

실제 브라우저·OS·논리/물리 해상도·DPI·음향/저장 환경과 검사 범위를 기록한다. 원본 fixture를 덮어쓰지 않고 미연결 장치, 웹 적응과 누락 fixture를 단위 문서에 남긴다.

## 6. 작업 명령과 참고

```powershell
npm --prefix webpj start
npm --prefix webpj test
node webpj/tools/port-map.mjs
node webpj/tools/fixture-manifest.mjs
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
node webpj/tools/screen-compare.mjs --diff <폴더>
npm --prefix webpj run check-assets
npm --prefix webpj run test:e2e
```

- 개발 서버는 `http://localhost:9803/`이다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 규칙 없는 개발용 미리보기다.
- 필요한 항목만 `node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>`로 갱신한다. 자산 변경 시 `npm --prefix webpj run prepare-assets`를 쓴다.
- Node v22는 HJOW-Athlon의 webpj 폴더에서 `& "C:\Program Files\nodejs22\node.exe" --test "tests/*.test.mjs"`로 순차 실행한다.
- [이식 대응표 사용법](docs/webpj-port-map-20261106.md) · [실행 자산](docs/runtime-resources.md) · [형식 목록](docs/formats/README.md) · [공유 명칭](assets/unit-names.korean.json) · [cpppj 소스 지도](cpppj/SOURCE_MAP.md) · [외부 접속 정책](docs/webpj-external-access-20261105.md)

## 7. 정리 기록

**2026-11-23.** E1237 직후 문서 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에 이관했다. 0.1절 전체 회귀 이력(E1170~E1234)은 최신 v26·v22 두 기록으로, 끝난 다음 작업 두 개(게임 메뉴 나머지·Uber HUD 채팅 창)와 E1228·E1230·E1231 감사 세 항목의 해결 연쇄는 다음 작업 1 한 항목으로 줄였다. 완료 체크 `[x]` 아홉 개(5-C 메인 메뉴·6 공장·지식·7 파괴·의식·AI 잔여·8 동영상·미션 수명·3.8 채팅 허브·점 명령/HTML)는 각 단계 끝 한 줄로 합쳤다. 4D-8b~4D-8f는 항목 자신이 '남은 행 없음'이고 해당 대응표 행이 모두 port 또는 제외라 `[x]`로 바꿔 4-D 머리 수를 56/85에서 61/85로 올렸다(4D-8 머리 문단에 한 줄로 합침). 완료됐는데 남은 것으로 적혀 있던 항목(채팅 이름 완성·`.flash` 섬광 — E1173 확인, 채팅 로그 원천 — E1172 확인으로 종결, 다리 붕괴 — E1230, 전투 자원 프레임 — E1232, NotifyPlayerResults 단독 도달 — E1233, Meta Sprite — E1203 제외)을 정정하고, 5-B·5-C·8단계 항목의 남은 행을 대응표 partial 행과 E1223·E1224 감사 노트 기준으로 다시 적었다. 대응표(34단위·20,876줄, port 973·partial 34·todo 0, 제외 153)·단계별 분배·cpppj 변경 보고 9행과 새 파일 5개·fixture manifest 840항목·cpppj 최신 제품 1326을 재집계로 갱신했다. 2026-11-21 이관 기록 원문은 이번 보관본의 7절에 있다.

현재 AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음)와 `11b16b77`(webpj favicon, [0.6절](#favicon))까지 반영했다. 2026-11-23 정리 때 이후 AGENTS.md 변경이 없음을 확인했다. 이전 이관 기록은 보관본의 날짜별 원문에서 확인한다.
