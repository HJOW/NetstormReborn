# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 [E1166 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)과 그 아래 날짜별 보관본을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → 3.1(단계별 항목) → 0.5(단위 착수·종료 절차) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 칸으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-22, 마지막 단위 WB04-E1185)

| 항목 | 현행 상태 |
| --- | --- |
| 실행·플레이 | 브라우저에서 ‘클릭하여 시작’, 메인 메뉴·캠페인/데모 목록·도움말·옵션·Credits와 개발용 지형 미리보기가 동작한다. **미션:** 캠페인 목록 31개(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)와 데모 3개가 시작되고, 장시간 진행과 임의 조작(몽키) 시험에서 오류·먹통이 없다. Campaign ▸ Skirmish는 무작위 섬·간헐천 월드에서 AI와 진행된다. Tutorial2는 시작부터 완료 창·Tutorial3 시작까지 자연 진행으로 확인했고, 승패 안내 뒤 흐름(Continue·Replay Mission·Leave/Next Mission·진행 저장)도 확인했다. **조작:** 사제 선택·이동, 가장자리·Alt·가운데 버튼 스크롤, 미니맵 클릭·끌기, 생산 격자 클릭·단축키(D 포함)·조각 회전(C·회전 방향 옵션)·다리 배치, 사제·워크샾·제단·신전·배터리·발전기·운반체 우클릭 메뉴와 회수, 객체 명령(Detonate·Meltdown·신전 명령), 화면 위치 키(F4/H·F5·TAB·U·Shift+숫자 저장/숫자 복귀)가 동작한다. **게임 메뉴:** ESC 또는 화면 우측 상단 버튼(웹 전용)으로 Game·View·Options·Players·About 메뉴막대를 열고, Leave/Restart Mission(ABORT 확인 창), F2·F6·F7·F8·F9·Shift-F3·T, Pause(Shift-F9·Pause 키·메뉴), Shift-F7 화면 GIF 저장, Players 메뉴의 송금(Send X 숫자 입력 창 포함)·동맹이 실행된다. **HUD 틀:** 실제 Uber 루트가 세로·미니맵·채팅 구분선, 좌측 사이드바(SP·생산 덱)와 하단 미니맵 칸을 원본 위치로 도킹하고 지도 영역이 `{85,0,W,H}`다(4:3 하단 85×82픽셀이 원본 스크린샷과 일치, 16:9·16:10·1920×1080과 창 크기 변경 확인). **채팅 창:** F3로 채팅 영역이 보이고 로그 줄이 색과 함께 그려지며(E1169), Enter로 입력줄을 열어 문자를 치고 제출하면 로컬 서버를 거쳐 `You> 본문` 줄이 로그에 나오며 `.clear` 같은 점 명령이 실행되고 빈 Enter로 닫는다(E1170·E1172). **미완성:** 채팅 이름 완성(Tab)·`.flash` 섬광 효과·미션 알림이 채팅 로그로 가는 원천 확인(지식 NSInfo 패널은 멀티/메타 준비 창이라 웹 범위 밖, E1171), 나머지 객체 메뉴(Factory 파생·특수 대상·채팅 분기), 다른 미션의 목표 수행과 자연 완주(Players 하위의 Ignore chat·Whisper To·정보 보기는 단독 미션에서 나오지 않아 웹 범위 밖, E1167). |
| 원본과의 동일성 | 영어 4:3 메뉴 M1 8장면은 cpppj 오라클과 알려진 구름 배경 등 차이 영역 밖에서 픽셀 차이 0이다. 월드·UI 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다(최근 원본 공통 메뉴 788행·Factory 메뉴 39행·Uber 루트 160행·장면 음악 3,113행 등). 좌측 HUD 틀의 하단(미니맵 구분선·세로 구분선 아랫부분·미니맵 칸) 85×82픽셀은 원본 스크린샷과 완전히 일치한다. 메뉴 비활성 색·안내 창 위치·지식 카드 이름 줄 나눔·Wind Noise 무동작은 원본 기준으로 맞췄다. Tutorial1 시작 직후 Session 덤프는 시간 의존 값을 빼고 cpppj 기준선과 구조가 같다(E1129). 고정 시각 L2·L3·L4 화면 비교와 자연 완주는 미검증이다. |
| 현재 작업 | **게임 플레이 중 UI 구현 단계.** 4-D 후반(GameWorld·Client 서비스)과 5-A·5-B·6·8단계 항목을 미션 앱 프레임 위에서 병행한다. Renderer·Gump 트리·입력 큐·생산 패널·객체 메뉴·도움말·음향이 실제 미션 화면에 연결돼 있고, 최근 단위(E1139~E1185)는 원본 도움말 조작키·우클릭 메뉴·승패 흐름의 실사용 점검과 앱에 연결되지 않은 모듈 분류(E1150·E1163)로 빈칸을 메웠다. 순서는 아래 [다음 작업](#next)을 따른다. |
| 전체 회귀 | **Node 26.13.0:** E1182 전체 6,719건 중 6,717건 통과·2건 실패(E1179 가 `translations/xlat.korean` 을 고치고 `node webpj/tools/prepare-assets.js` 로 복사본·매니페스트를 갱신하지 않아 `assets.test`·`korean.test` 실패, E1183 에서 갱신해 두 파일 27건 통과, 전체 재실행은 다음 회귀 때; 로그 `extracted/webpj-e1182-node26.log`, 614초). 직전 전체 통과: E1178 6,710건(로그 `extracted/webpj-e1178-node26.log`, 554초; E1174·E1170도 6,710건 통과). **Node 22.23.3:** E1178 전체 6,710건 모두 통과(로그 `extracted/webpj-e1178-node22.log`, 624초; E1170도 6,710건 통과). 이전: v26 E1166 6,702건·v22 E1161 6,702건. **e2e:** E1147 전체 `npx playwright test` 100건 통과·8건 건너뜀(옵트인 장시간, 로그 `extracted/webpj-e1147-e2e-all.log`, 39분). 이후 단위는 새 e2e와 관련 e2e를 단위마다 돌렸다. **HJOW-Athlon 주기(0.5절 5):** v26 3/4(E1183~E1185; E1182 회귀의 2건 실패는 E1183 에서 고침), v22 7/8(E1179~E1185) — **4단위 뒤 Node v26·v22 전체 회귀**를 순차로 돈다. E1127 첫 실행의 `external-access` 1건 실패(시험 대역 누락)는 v26 재실행과 이후 v22 전체 회귀(E1130·E1138·E1145·E1153·E1161)에서 해소됐다. E1127~E1161의 회귀 이력은 [E1166 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)에 있다. |
| 브라우저 검사 | 미션 화면은 단위마다 Chromium e2e로 확인한다(최근 전체 실행 E1147, 16:9·16:10 건설·메뉴 E1143~E1144, 한국어 Players 메뉴 E1166). 메뉴는 2026-11-17 Chromium 2/2(갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체), 최초 실행·진행 잠금/체크·저장 재실행은 E616 Chromium 2/2. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md) |

<a id="next"></a>

#### 다음 작업

1. **게임 메뉴 나머지 — 완료(E1167~E1168).** Players 하위 항목 중 `Ignore chat`·`Whisper To`·`View Player Info`는 단독 미션에서 나오지 않아 웹 범위 밖이고([E1167](docs/webpj-player-menu-reachability-20261122.md)), **Send X 숫자 입력 창**은 모달·글자 입력창·키보드 경로와 함께 앱 미션에 연결했다([E1168](docs/webpj-send-money-dialog-20261122.md), `e2e/send-money-dialog.spec.js`). 송금·모두 보내기·동맹은 [E1126](docs/webpj-player-menu-actions-20261120.md), 원본 도움말 조작키는 모두 연결·확인했다([조작키 대조](docs/webpj-alt-scroll-keys-20261121.md) · [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md)). 채팅 창 내용물(F3)은 3번에서 잇는다.
2. **다른 미션의 목표 수행.** Tutorial2([E1086](docs/webpj-tutorial2-complete-20261120.md))처럼 Tutorial1·3~6과 세 캠페인 묶음 미션의 단계별 목표를 수행해 승리·완료 창까지 이어 보고, 남는 객체 메뉴(Factory 파생·특수 대상·채팅 분기, Archer 실제 우클릭·운반체 Carrying/Drop)와 입력 공백을 메운다. 승패 안내 뒤 흐름은 [E1152](docs/webpj-mission-outcome-flow-20261121.md)·[E1161](docs/webpj-idle-outcome-20261121.md)에서 확인했다. 앱에 연결되지 않은 나머지 모듈은 [E1163 분류표](docs/webpj-pause-key-unwired-modules-20261121.md)에 이어 사용자 기능과 직결된 것부터 확인한다(3.8절 입력·장치). 자연 완주는 1절 사용자 확인 범위다.
3. **Uber HUD 나머지.** 이식된 `ui/chat-window.js`(`ChatWindow`)의 호스트 훅 약 50개를 실제 부품에 묶어(Gump 글자 입력창·모달·키보드→입력창 경로는 E1168 `ui/client-money-dialog.js`에서 미션 앱에 처음 연결했으므로 같은 조립을 참고한다) Uber 루트의 `createChat`에 연결하고, HUD의 `chatVisible`/`chatInput` stub을 실제 창 표시 비트로 바꿔 채팅 표시 전환·수신·제출 경로를 잇는다(권장 순서는 [F3 채팅 설계 메모](docs/webpj-chat-toggle-f3-20261121.md), 5-B·5-C·3.8절 채팅 허브). 지식(NSInfo) 패널은 멀티/메타 준비 창이고 단독 미션 부착 경로가 화면 배치 없이 끝나며 원본 단독 미션 화면에도 없어 **웹 범위 밖으로 판정했다**([E1171](docs/webpj-nsinfo-scope-20261122.md)). 미니맵 클릭·끌기(E1128)·ESC 게임 메뉴 상단 바(E1084)·좌측 도킹(E1082)은 끝났다.
   - **채팅 창 조립 완료 범위(E1169~E1172, [조립 문서](docs/webpj-chat-window-assembly-20261122.md)).** `ClientChatWindow`(`ui/client-chat-window.js`)가 root·입력줄·로그·도우미·배경을 만들고 공유 로그 버퍼(`0x548758`)를 빌리며 HUD에 연결돼 F3로 보이고 로그가 그려지며, Enter로 입력줄이 열려 문자 키가 입력줄로만 가고 빈 Enter로 닫힌다(E1169~E1170). E1172에서 제출이 원본 파이프라인(`bindClientChatRecipients`·`bindClientChatMessage`·`bindClientChatSubmit`)을 타고 로컬 서버(9)를 거쳐 41h 수신(`dispatchChatPacket`)으로 돌아와 `You> 본문` 로그가 되며 점 명령(`.clear`·서식·issue 명령)이 실행된다. 원본에 기본값이 없는 열 개 표시 옵션은 **켜짐(1)** 으로 정했다(Netstorm.exe 0x42ed30 역분석: 값이 없으면 초기화하지 않은 스택 잔여값을 읽는다). **남은 일:** ① 없음(Edit Back/Front Format 입력 창은 E1179에서 연결: 원문 0x4d4580/0x4d48f0·확정 0x4c9600/0x4c9680, 송금 숫자 창과 같은 기반, 확정이 `<구독자>chatFormat`/`chatFormat2` 설정을 쓰고 chat manager 를 다시 초기화, 입력창 아래 가로 슬라이더의 원본 화면 대조는 미확인, [E1179](docs/webpj-chat-window-assembly-20261122.md#edit-backfront-format-입력-창e1179)). 붙여넣기(Ctrl+V)는 E1175에서, 도우미 메뉴가 올리는 삽입·설정 대입 명령은 E1177에서, 입력줄 복사·잘라내기와 도우미 `Copy`/`Paste` 클립보드 장치는 E1178에서 연결했다(원본 편집기 키 그대로 Ctrl+Insert 복사·Shift+Insert 붙여넣기·Shift+Delete 잘라내기이며 Ctrl+C/X 는 원본에 없다, [E1178](docs/webpj-chat-window-assembly-20261122.md#복사잘라내기클립보드e1178))(처리하지 못한 도우미 명령이 입력 큐를 3초 막고 오류로 버려지던 문제 포함). `.flash`는 E1163 판정(번개 섬광은 원본 창 모드에서 무동작)대로 소리 요청만 하고 오류 없이 처리됨을 e2e로 확인했다. Tab 이름 완성은 동맹 AI(채팅 기록 채널 참가자)로 확인했다. `EVERYONE:` 채널 접두 제출·서식 점 명령·도우미 메뉴 열기·닫기는 E1173 e2e로 확인했다. ② 로그 원천 — cpppj에서 채팅 패킷을 만드는 곳은 사용자 `say`(`SendChatPacket`)뿐이고 로그 줄은 수신 패킷과 `PrintChat` 오류 안내로만 들어오므로, 단독 미션에는 입력 외의 자동 로그 원천이 없다(`Our Priest has been Immobilized` 같은 알림은 오른쪽 위 경고 안내(`MessageGump`) 계열이며 채팅 로그가 아니다). 이 항목은 확인으로 닫았다.
4. **나머지 번역.** 게임 안 메뉴·객체 우클릭·지식 창의 번역 누락은 없다(E1166 수집, 소스 기준 누락은 개발자 치트 `CHEAT Set Owner`뿐). 번역 호출 자체가 없는 생산 창 도움말·미니맵 도움말·채팅 채널 이름의 고정 영어와 한국어 미션의 실제 화면 확인이 남았다(8단계·3.7절 6차). [게임 안 문구](docs/webpj-korean-ingame-xlat-20261120.md) · [한국어 이름](docs/webpj-korean-player-names-20261121.md)
5. **4-D 완료 기준 L2.** Tutorial1 Load/Start의 보고서·덤프·첫 프레임 논리 화면 대조(4D-10b)는 계속 4-D의 완료 기준이다. 시작 직후 덤프의 구조 일치는 [E1129](docs/webpj-l2-tutorial1-20261121.md)에서 확인했고 고정 시각 모드·L3·첫 프레임 화면 대조가 남았다.
6. **12차 스커미시 나머지.** 설정·화면·시작·월드 생성·금지 유닛과 난이도별 건설 속도는 E1101~E1107·E1149에서 연결·확인했다(3.7절 12차). AI의 생산·공격 단계(수 분 이상)·개별 난이도/동맹의 N프레임 동작, 실제 전투로 사제가 죽을 때의 승패 안내, 비미션 너겟 보충(4D-1c)을 확인한다(자연 완주는 1절 사용자 확인 범위). [게임 진행](docs/webpj-skirmish-play-20261120.md) · [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)

- **로컬 산출물.** `webpj/test-results-e1072-rerun`~`webpj/test-results-e1081`의 10개 폴더는 빈 폴더이거나 Git 제외 `*.zip` trace만 들어 있어 Git 상태에 나타나지 않는다. 다시 볼 필요가 없으면 삭제해도 된다(2026-11-21 정리 때도 남아 있음). 루트에 있던 E633~E640 Node 전체 회귀·대응표 로그 34개(`extracted-webpj-e6NN-*.log`, 경로 구분자가 빠져 루트에 생긴 Git 제외 파일)는 2026-11-21 `extracted/webpj-e633-e640-root-logs/`로 파일 이름 그대로 옮겼다. 해당 단위 문서(E633~E640)가 적은 로그 위치는 이 폴더로 읽는다.
- **미션 식별.** 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이고, 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이다(E753 재대조). [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md)

<a id="e951-e1081"></a>

#### E951~E1166 진행 요약

단위별 완료 문단·검증 수·회귀 이력은 [E1081 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081)의 0.1절(E951~E1081), [E1166 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)의 0.1·3.1·3.7절(E1082~E1166)과 각 단위 문서에 있다.

| 주제 | 단위 | 연결한 것 | 남은 것 | 대표 문서 |
| --- | --- | --- | --- | --- |
| 미션 로딩·Start | E951~E958, E968, E970, E974~E976 | 로그인 HUD·진행 창, 최초 Fort 요청 자동 처리, Ready→Start→첫 안내와 시계 정지/재개, 공유 Kernel 프레임 | 원본 Gump serial | [로딩](docs/webpj-app-mission-loading-20261119.md) · [Start](docs/webpj-app-tutorial-start-frame-20261120.md) |
| 음향·음악 | E957, E959~E966, E997, E1059, E1061, E1119~E1120, E1153 | 효과음·음악 적재와 Web Audio 재생, 위치 음향·발소리, 장면 음악(미션 중 순환)·결과 음악·희생 의식 곡, Wind Noise 판정 | 대기실 곡·승리/패배 곡 요청, 실제 청취 | [앱 음향](docs/webpj-app-campaign-audio-20261119.md) · [장면 음악 구동기](docs/webpj-scene-music-driver-20261120.md) |
| AI·채팅·Tell·승패 | E967, E969, E971~E973, E1058 | AI 이름·난이도·채팅 기록, 미션 Tell 조회, Gump 팝업 차단·닫기, 승패 안내, NSInfo·도움말 팝업 저장소 | 채팅 창 공장, 실제 State 정리·World 교체 | [AI](docs/webpj-app-campaign-ai-20261120.md) · [승패](docs/webpj-app-mission-outcome-20261120.md) |
| Renderer·화면 | E977~E989, E1081~E1082 | 지면·객체·입자·커서·Gump 수집과 출력, Screen DIB·구름 합성·Present, Gump 이동 dirty, 도킹된 지도 영역 | 지식·채팅 HUD 공장, Kencloud·구름 차이 | [출력](docs/webpj-campaign-renderer-drawing-20261120.md) · [Present](docs/webpj-app-renderer-present-20261120.md) |
| 미션 진입 | E985~E986, E1010~E1016, E1092 | Tutorial1~6·세 캠페인 묶음 진입·브리핑, 세 데모 시작·45초 Auto-Demo | 미션별 목표 수행·자연 완주 | [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md) · [데모](docs/webpj-demo-start-20261120.md) |
| 입력·카메라 | E990~E1001, E1009, E1084 | 선택·이동·포인터 폴링, 가장자리 스크롤·카메라 키(F4·P/R·F5·N), 조각 회전, ESC 메뉴 토글 | 여러 유닛 순환(Ctrl-F5·N은 사제 1명 상태로 확인) | [포인터](docs/webpj-app-mission-pointer-frame-20261120.md) · [사제 키](docs/webpj-browser-priest-shortcuts-20261120.md) |
| 생산·건설 | E1002~E1008, E1019~E1022, E1026~E1036, E1047, E1074, E1076~E1079, E1083, E1086 | 생산 패널·단축키·다리 배치, 회수·비용 표시, Factory 생산·업그레이드, Tutorial2 발사기 배치·회수 | 연속 배치·자연 완주 | [생산 패널](docs/webpj-client-production-panel-20261120.md) · [워크샾 생산](docs/webpj-browser-tutorial2-factory-20261120.md) |
| HUD 틀 | E1082, E1128, E1169~E1170, E1172 | Uber 루트 HUD 모드 3 도킹·화면 크기 변경 재배치, 미니맵 Gump와 클릭·끌기 e2e, 채팅 창(F3·로그·입력줄)·제출·수신 | 채팅 이름 완성·섬광(지식 NSInfo 공장은 웹 범위 밖, E1171) | [좌측 HUD 틀](docs/webpj-uber-hud-left-dock-20261120.md) |
| 게임 메뉴 | E1084~E1085, E1087~E1090, E1093~E1097, E1100, E1167~E1168 | ESC 메뉴막대·우측 상단 버튼, Leave/Restart, F6·F7·F9·T, Pause, Frame Rate·Language·Difficulty, Shift-F7 GIF, Players 하위 항목 실측(채팅·정보 항목은 단독 미션 도달 불가), Send X 숫자 입력 창(모달·글자 입력창) | 채팅 창 내용물 | [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md) · [Send X 창](docs/webpj-send-money-dialog-20261122.md) |
| 스커미시(12차) | E1101~E1107 | 설정 화면·저장, 시작 섬·섬 생성 부모, 세션 월드 생성·앱 시작, AI 진행·금지 유닛 | AI 생산·공격, 실제 전투 승패·너겟 | [시작과 월드 생성](docs/webpj-skirmish-start-20261120.md) · [게임 진행](docs/webpj-skirmish-play-20261120.md) |
| Gump·메뉴 UI | E1017~E1018, E1023~E1025, E1037~E1045, E1064~E1072, E1075, E1086 | Form 통지, 글자·Slider 출력, 중첩 메뉴, 사제·Player·공통 객체·운반체 메뉴, 신전 건설·완공 | 파생 +ac 객체 메뉴, 채팅 분기 | [Client 메뉴](docs/webpj-client-menu-gump-20261120.md) · [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) |
| 도움말 | E1046, E1048~E1055 | 생산 Help, TYPE 정보 창, 요구 에너지·도움말 그림 | 타입별 원본 화면 비교 | [앱 TYPE](docs/webpj-app-help-type-info-20261120.md) |
| 한국어 | E1056~E1057, E1062~E1063, E1073, E1080, E1108~E1118, E1140~E1142, E1166 | 모든 미션 대본, 게임 안 메뉴·객체 메뉴·지식 문구, 지식 카드 줄 나눔, Player 이름 디코딩 | 생산 창·미니맵 도움말·채팅 채널 이름(번역 호출 없음), 실제 화면 확인 | [캠페인 미션](docs/webpj-korean-campaign-missions-20261120.md) · [한국어 이름](docs/webpj-korean-player-names-20261121.md) |
| 조작키 | E1145~E1146, E1148, E1151, E1154~E1155, E1159, E1162~E1163 | Alt·가운데 버튼 스크롤, H, C·`rotmode`, TAB·U·Shift+숫자/숫자, D·F 키 점검, F3 구분선, Pause 키 | 채팅 창 내용물 | [Alt 스크롤·조작키](docs/webpj-alt-scroll-keys-20261121.md) · [화면 위치 키](docs/webpj-camera-keys-20261121.md) |
| 객체 메뉴·명령 | E1126~E1127, E1135~E1138, E1150 | Players 송금·동맹, 파생 객체·제단·운반체·Archer 메뉴, 메뉴 닫기·비활성 색, 객체 명령·정체 안전장치 | Factory 파생·특수 대상·채팅 분기, Archer·운반체 항목 확인 | [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [객체 명령](docs/webpj-object-commands-20261121.md) |
| 장시간·실사용 점검 | E1121~E1125, E1128, E1132~E1134, E1139, E1143~E1144, E1147, E1158, E1160, E1165 | 전 미션 시작·장시간 진행, 몽키, 시계 정지 누수·죽은 Gump 정리 수정, 실사용·넓은 화면 e2e, 성능 측정 | 자연 완주 | [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md) |
| 승패·L2·미션 흐름 | E1129~E1131, E1152, E1157, E1161 | 시작 덤프 구조 대조, 안내 창 배치, 승패 뒤 흐름·진행 저장, Priest Training 시작 연결 | 고정 시각 L2·L3, 실제 사제 처치 패배 | [L2 대조](docs/webpj-l2-tutorial1-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) |
| 분석·판정 | E1149, E1153, E1156, E1163 | 난이도별 건설 속도, Wind Noise, 빨리 감기 한계, 미연결 모듈 68개 분류 | 쉬움 채취 절반 실관찰, 미판정 모듈 | [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md) · [미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md) |
| 회귀 교정 | E1060, E1078 | 관측 자료 누락·메뉴 전용 번역 검사 범위, 외부 접속 감사의 비공개 필드 오탐 | — | [회귀 교정](docs/webpj-regression-translation-fixtures-20261120.md) · [접속 감사](docs/webpj-external-audit-private-field-20261120.md) |

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분은 중복 가산하지 않는다. 남은 단위와 줄 수는 `PORT_MAP.tsv`의 partial·todo 기준이며 partial에는 파일 전체 줄 수를 센다. E951~E1166은 대부분 상위 항목 안의 부분 진행이라 체크 수가 바뀌지 않았고, 2026-11-21 정리에서 8단계 **미션 수명**을 확인 범위가 채워진 것으로 판정해 8단계가 1/9가 됐다. 남은 양은 E1176 뒤 133단위·31,456줄이다(E1168에서 `SaveFortNameDialog`·`RawMenuSaveFort`를 편집 전용 제외에서 이식 대상 partial 두 단위로 옮겨 135단위·31,510줄에서 137단위·31,863줄로 늘었다가, E1176에서 NSInfo 메타 네 단위 407줄을 멀티 전용 제외로 옮겨 줄었다). `node webpj/tools/port-map.mjs`와 `python -X utf8 tools/port_inventory.py` 재집계로 확인했다. E1082~E1104의 단위별 증감(`UberHud.inc` todo→partial, `FrameRateOption`·`HelpLinkPolicy`·`Sound`·`SoundDevice`·`GifScreenshot`·`SkirmishSetup`·`SkirmishStartIslands`·섬 생성 부모 셋 →port)은 [E1166 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)에 있다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 |
| --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료. 4-C 통합 잔여는 4D-3 | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **56/85, L2 미완료** | 73 / 17,583 |
| 5 | 5-A 렌더러·Draw | 44/46 | 3 / 533 |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 9/13 | 7 / 1,186 |
| 5 | 5-C 화면 허브·미션 진입 | 1/6 → M2 | 10 / 6,290 |
| 6 | 입력·게임 메뉴·경제·건설 | 1/8 → M3 | 5 / 766 |
| 7 | 전투·의식·AI | 1/6 | 2 / 54 |
| 8 | 미션 수명·대본·오디오·도움말 | 1/9 → M4 | 24 / 4,499 |
| 9 | 표시 품질·브라우저/OS·배포 | E945 하위 경로·설정 복원 검증, M5 미완료 | 1 / 41 |
| 10 | AGENTS 후순위 웹 요구 | 일부 선행, 3.7절 참조 | 3 / 285 |
| **합계** | | | **128 / 31,237** |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,020단위는 **port 887(87.0%)·partial 100·todo 33**이다(2026-11-22 재집계, E1168에서 편집 전용 제외 두 단위를 송금 숫자 창 이식 대상 partial 로 옮기고 E1176에서 NSInfo 메타 네 단위를 멀티 전용 제외로 옮김). 제외 140단위를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session`과 오라클 대조를 뜻하는 `oracle`은 0이다. E1098~E1104의 재판정 내역(RawSurface 29행·Dialog 2행 todo→partial 등)은 [E1166 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)에 있다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 567 / 574 | 6 | 1 |
| `client/` 세션·UI·표시 | 315 / 445 | 66 | 64 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 128단위 중 기존 기준선은 68단위/26,834줄, 재기준선 추가분은 60단위/4,403줄이다. E854에서 `UberReconnect`의 단독 감시를, E884에서 단독 미션의 Fort 이름 선택 두 단위를 이식 대상으로 재분류했다. 재기준선 분류·영역별 상세는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)과 `python -X utf8 tools/port_inventory.py --families wb`에서 확인한다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 837항목을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

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

- [ ] **favicon 탭 시각 확인.** E894에서 공통 ICO를 복사하고 상대 경로 링크를 추가했다. 원본 두 이미지의 바이트 대조와 Chromium의 HTTP 200·해독을 확인했다. 2026-11-20 Chrome의 localhost NetStorm 탭은 접근성 트리에서 확인했으나 Windows 창 캡처가 반복 시간 초과하여 탭 그림의 시각 확인이 남았다. [작업·검증](docs/webpj-favicon-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

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
| E951~E1166 | 미션 앱 프레임의 로딩·Start·음향·Renderer/Gump 출력, 브라우저 입력·카메라와 원본 도움말 조작키, 31개 미션·데모·스커미시 진입과 장시간 진행, 생산 패널·건설, 객체 메뉴·명령, 게임 메뉴막대, 승패 뒤 흐름, 도움말, 한국어 대본·게임 안 문구, HUD 도킹. 주제별 표는 [0.1절 요약](#e951-e1081). |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 4-D 전체가 선행 대기면 가능한 5-A·5-B를 병행한다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다.

#### 4-D Session 조립·Tutorial1 Load/Start (56/85)

**완료 기준은 L2다.** Tutorial1 Load/Start의 보고서·Session 덤프·첫 프레임 논리 화면을 cpppj 오라클과 대조한다. typed `Load()`와 실제 Fort Login/Ready, 일부 Start는 Node 검사에서 확인했다. Client·Native 장치와 브라우저 앱 수명은 이어서 연결한다. 기준선 구간 지도와 E633~E801의 완료 체크 및 단위 링크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)에, E802~E1166의 항목별 완료 나열 원문은 [E1166 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)에 있다.

아래 cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다. E946에서 4D-1b를 완료해 56개다. 항목마다 남은 일을 먼저 적고 완료분은 단위 번호와 대표 근거만 둔다.

**4D-1 Session 외곽·snapshot.** 저장 writer, 현재 Core, 부모 callback, 생성자/Publish와 외곽 반납 앞단, 4D-1b 전체 snapshot(Battle 재적재·기술 획득 Fort snapshot, `loaded` 이전 Editor 수명은 Edit 제외, E946)은 완료했다. [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) · [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md)

- [ ] **4D-1c 외곽 callback** — 기술 획득, 로컬·세계 간헐천, 단일 미션 너겟 관문은 실자료로 확인했다. `PlaceBattleNuggets`의 비미션 정상 생성은 12차 실제 스커미시 Session에서 검증한다(5761..5772).

**4D-2 typed Load 완료.** 건설·이동·AI·의식·파괴/주문·운반 투하/기도·배치/커서를 Session에 설치했다. 캠페인 1-1의 저장 객체 적재와 `loaded` 관문을 확인했다. [E697](docs/webpj-session-load-20261118.md)

**4D-3 Login·로컬 연결·4-C 통합 시험.** 내부 Login → Load·명령 등록 → Ready → Start 순서를 실제 Session에서 확인했다.

- **E950 선행:** 지연 요청의 Poll 전 수명과 실제 Guide→State7→Ready를 검사했다. E968에서 기존 World가 없는 경우 앱 프레임 자동 호출을 연결했다. 기존 World 교체는 남았다. [E968](docs/webpj-app-fort-request-frame-20261120.md) [요청 프레임](docs/webpj-campaign-request-frame-20261119.md)

- [ ] **4D-3a RawCampaignSessionLogin**
  - 남음: 기존 World 교체, 로그인 완료 후 세계 화면 설치·분리 타이머(4D-6), 고정 수신기(4D-8), 나머지 31h 명령(4D-3e), `BeginGeneral`·`InitializeGeneral`·`LocalFort`의 필요 범위(스커미시에서 판단).
  - 완료: Begin/Pump·Ready 왕복과 행 base 뒤 32커밋 판정(E765), 진행 안내·분리 타이머(E851~E852), Fort 이름 선택·원본 관측 876행(E884~E892), 앱 Session의 Snapshot·Notify·State(E948), Guide Init·앱 프레임 실행(E886·E890), 최초 지연 요청 소비·Tutorial Start(E968·E975), 출하 Fort 32종 로그인(일반 31·데모 1, E752), 데모 모드 Start(E1092·E1125). [E765](docs/webpj-session-login-commit-audit-20261118.md) · [E948](docs/webpj-app-fort-name-session-20261119.md) · [출하 Fort](docs/webpj-session-login-shipped-forts-20261118.md)
- [ ] **4D-3b 로컬 공개 API·ClientCampaignTransport**
  - 남음: 실제 Client의 장기 소유·Native 전송 수명(4146..4200, 4292..4317).
  - 완료: 시작/Poll/Pump·상태8·전송 기록·로컬/mutable 송신·Tell과 typed 기본 수신기(경로/압축 관찰·기술·Chat·연결 종료, E772), 내부 전송 등록 세대의 Session 대여·수신 중 State 정리 차단(E893). [E772](docs/webpj-session-state-connect-record-20261118.md) · [E893](docs/webpj-session-internal-callback-20261119.md)
- [ ] **4D-3c 전송 앞단**
  - 남음: `RawClockSynchronization`의 월드 교체를 포함한 외곽 수명과 Native baseline, 소켓 지연 FIFO의 웹 제외 경계, `ClientPacketSources`.
  - 완료: 공통 `RawPacketFrame`의 로컬 전송 연결(E820), 앱의 시간 표본·Session Pump·공유 GameClock·State 정리(E947). [E820](docs/webpj-packet-frame-20261119.md) · [E947](docs/webpj-app-clock-synchronization-20261119.md)
- [ ] **4D-3d Squid 수신**
  - 남음: ReceiveSquid·Dais·Gun·Aviary·Flyer의 미연결/Native 경계(5148..5261). 실제 원격 로그인은 웹 범위 밖이다.
  - 완료: `RawPeerWorldSync` 본체와 World·Login 서버 콜백(E822~E823), 공통 객체·종속 부착·변경/삭제·검증·공유 Regular·압축의 typed 수신. [E823](docs/webpj-peer-world-session-20261119.md) · [압축](docs/webpj-compressed-squid-receive-20261118.md)
- [ ] **4D-3e Login 콜백·명령 등록**
  - 남음: 나머지 실제 Client 장치·단독 도달과 앱의 큐 소비.
  - 완료: Carrier·건설·연구 typed 등록(E750), RegisterCarrierCommands 5262..5436의 Native 분기·미연결 명령과 31h 개발자 패널 명령의 입력 고리 예약(E766). [E750](docs/webpj-session-login-ready-20261118.md) · [E766](docs/webpj-developer-devices-20261118.md)
- [ ] **4D-3f 통합 시험**
  - 남음: 실제 Client/앱 프레임과 유닛 생산(입력/생산 장치 연결과 함께).
  - 완료: 같은 Session의 Login/옵션/사제 자연 도착, 일반·데모 Start와 AI 동맹, 세 운반 유닛의 선택→호버→클릭→전송/Kernel 자연 도착(E826), 1-1 워크샾 Kernel 자연 완공·연구 58h·생산 기술 동기화·플레어 수명·중복 연구(E827~E830), GameWorld 프레임의 세 유닛 자연 도착(E836). [E826](docs/webpj-world-carrier-input-20261119.md) · [E827](docs/webpj-workshop-kernel-completion-20261119.md) · [E836](docs/webpj-game-world-frame-20261119.md)

**4D-4 프레임·카메라·Tutorial 게이트**

- [ ] **4D-4a 카메라·프레임**
  - 남음: 실제 Client·전체 Renderer 연결과 프레임 순서 대조(3982..3999, 4523..4551, 4564..4587).
  - 완료: Session PrepareFrame/RunFrame, 공유 카메라 요청·적용, 초기/Home/사제 시점, 입자 뷰·커서 경계, GameWorld 프레임·단일 Kernel 준비/후처리(E775·E777). [E775](docs/webpj-session-camera-request-20261118.md) · [E777](docs/webpj-session-particle-view-20261118.md)
- [ ] **4D-4b Tutorial 게이트·표시 장치**
  - 남음: 실제 UI·앱, 다리 수명 숫자·Renderer, 승패 판정 소비 경계(3659..3667, 6033..6075, 6115..6125).
  - 완료: Tutorial 1~6 판정·공통 게이트, 신전의 현재 슬롯 재조회(E781~E782). [E781](docs/webpj-session-temple-draw-20261118.md) · [E782](docs/webpj-session-tutorial-gate-20261118.md)

**4D-5 승패·종료·State 반납**

- [ ] **4D-5a 종료 수명**
  - 남음: 실제 Client/앱 State 반납, Native 건설·지속 실패와 승패 수명(4000..4057, 4588, 4816..4833).
  - 완료: 승패 조회 경계, AI·Player/Deck·Process 자식 반납 증빙, 일반 종료 순서·오류 진단(E786·E789). [E786](docs/webpj-session-state-withdraw-20261118.md) · [E789](docs/webpj-session-process-shutdown-error-20261118.md)
- [ ] **4D-5b Client State 정리**
  - 남음: ClientStateWorldGlobals의 실제 팝업·음향 소유자와 ClientStateWorldCleanup·ClientStateStorageCleanup·ClientStateRuntimeRetirement·ClientUserSelectionCleanup의 실제 Client/State 연결, 실제 Client 전송·RawMenuControls·ClientAudio와 승패 후 수명.
  - 완료: Fort 적재 Session(E818)과 내부 로그인·AI 미션 시작 뒤(E819)의 정리 검사. [E818](docs/webpj-client-state-loaded-cleanup-20261119.md) · [E819](docs/webpj-client-state-started-cleanup-20261119.md)
- [ ] **4D-5c 이탈·전투 판정**
  - 남음: 실제 앱 루프·스커미시 결과·저장 장치와 4209..4279의 단독 도달.
  - 완료: RawBattleCompletion 팀·기권·미접속 관문과 후반 Frame의 기준선 C++ 합성 상태 4,096건 대조, 실제 Session의 Player·동맹·연결·사제 종료 조회, Client 장기 소유자·입력 고리, 무승부 거절의 Session 6F 처리·방송, 전투 복귀 State6의 결과·배경·음악(E842~E847). [E842](docs/webpj-battle-completion-gates-20261119.md) · [E844](docs/webpj-battle-completion-frame-20261119.md) · [E847](docs/webpj-battle-return-state-20261119.md)

**4D-6 GameWorld·Client 서비스**

- **E951 선행:** State7·13 HUD 모드를 실제 앱 Gump 입력에 연결했다. E952에서 메뉴 진행 창을 추가했으며 원본 serial과 전체 HUD는 별도다. [로그인 HUD](docs/webpj-app-login-hud-mode-20261119.md)

- [ ] **4D-6a GameWorld** — 지연 Session 소유부터 앱 프레임 결선까지 진행 중이다.
  - 남음: 기존 World 교체, 로그인 완료 후 화면 설치·분리 타이머·진행 안내·종료·전체 삭제, 다른 UI 색상·날씨. 입력은 6단계에서 잇는다.
  - 완료: 지연 Session 소유·Login·자산·팔레트·선택 효과(E790~E793), GameWorld 프레임·단일 Kernel과 State7 부모·State8 내부 로그인, 로그인 뒤 연결 감시·타이머 프레임(E855), State Poll·패킷 Pump(E895), 카메라·팔레트·색·Screen DIB·구름·전투 팔레트(E898~E907), Gump 루트·트리·Uber 루트(E908~E913), Renderer 결선(5-A, E977~E989). [E790](docs/webpj-campaign-game-world-front-20261118.md) · [E895](docs/webpj-app-connection-frame-20261119.md) · [E908](docs/webpj-app-gump-root-20261119.md)
- [ ] **4D-6b Client 서비스** — CampaignClientServices와 앱 Client 장치를 실제 Session에 연결하고 있다.
  - 남음: 원본 Gump serial·취소 명령, 나머지 실제 앱 메뉴·Gump 소유자·프레임 타이머·입력 장치·Poll, 나머지 미션의 입력 큐 소비, 스커미시 준비와 건설 기록 장치.
  - 완료: Start 효과·AI·State7/8·개발자 입력(E837~E841), 로그인 진행·사용자 입력·커서·표시 장치(E851·E856~E858), 신전 소유권·PostPop 생산 갱신(E875), 번역·설정·Login 공통 장치(E896~E897), 미션 장치·화면 크기·즉시 카메라(E952~E956), 효과음(E965~E966), AI·채팅 창·Start·Tell·팝업·승패·Kernel 프레임(E967~E976), Tutorial 메뉴 시작과 선택·이동·포인터(E985~E998), 소리·안내 서비스 여섯 개(E1122). [E837](docs/webpj-campaign-client-start-effects-20261119.md) · [E967](docs/webpj-app-campaign-ai-20261120.md) · [E1122](docs/webpj-mission-services-long-run-20261120.md)
- [ ] **4D-6c Session 공개 조회·장치**
  - 남음: 나머지 조회/장치(4590..4619, 4643..4741, 6029..6032, 6076..6079, 6100..6114, 6316..6399).
  - 완료: Start의 조회·기술·사제, 건설 운반, 생산 소유자, 커서·에너지·배치, 운반체·Dude 이동 공개와 부모·통지 조회·인구/경로·속도별 시간표·도착 삭제(E848~E850). [E848](docs/webpj-session-public-movement-20261119.md) · [E850](docs/webpj-session-dude-path-20261119.md)

**4D-7 ClientMain·Client 전역**

- **E949 선행:** Client 효과음 이름표를 실제 Session에 공유했다. 재생 장치와 위치 음향은 8단계 E957~E966에서 연결했다. [이름표 수명](docs/webpj-app-sound-list-20261119.md)

- [ ] **4D-7a ClientMain 단독 조립**
  - 남음: 장기 Session과 나머지 Client 전역, RawSurfaceStorage 장치(기준선 ClientMain 442..533), ConfigInterface의 미션 연결. Native는 4D-8과 함께 적용한다.
  - 완료: 공통 난수·SP·Session 대여(E878), Player·Core·Template·Subscriber 장기 소유자(E879), 장기 Storage·Kernel(E880), 미션 자료 준비(E881), Guide 생성·교체·반납과 제한 표(E883). [E879](docs/webpj-app-shared-players-20261119.md) · [E880](docs/webpj-app-world-storage-20261119.md) · [E883](docs/webpj-app-campaign-guide-20261119.md)
- [ ] **4D-7b 메인 루프 단독 단계**
  - 남음: 실제 앱 미션·Kernel 프레임, 전투 자원 프레임·재접속 저장의 단독 도달, ClientDiagnostics 잔여와 고리·진단의 실제 Client 수명, 나머지 오류/예외 로깅.
  - 완료: 로그인 타이머·연결 활동 패킷·연결 감시 뒤 타이머 순서(E852~E854), 임시 문자열 고리·정수 서식·trace/console/Chat 경계(E859~E861), Kernel 소진 진단(E864), 공유 프레임 시계·보정과 앱 프레임·탭 숨김 정지(E876~E877). [E852](docs/webpj-session-login-frame-timer-20261119.md) · [E861](docs/webpj-client-diagnostics-20261119.md) · [E877](docs/webpj-app-frame-clock-20261119.md)
- [ ] **4D-7c 메타 단독 앞단** — ClientMetaPacketReceivers·ClientChallengeGlobals·NativeMetaConnectedState, MetaMenuType/NotifyPlayerResults(4620..4642)의 도달을 판정한다. ClientMetaSpriteRefresh·NativeMetaSpriteRefresh는 5-C와 확인한다.
- [ ] **4D-7d Client 렌더러 프레임 전역** — 채팅·HTML·점 명령 전역 이식과 ClientRendererFrameGlobals의 앱 소유·화면 크기 변경 수명(E977)은 마쳤다. 수집/출력 사용은 5-A ClientRendererFrame의 ClientRendererFrameGlobals 확인과 함께 마무리한다. [E977](docs/webpj-app-renderer-frame-owner-20261120.md) · [E496](docs/webpj-renderer-cloud-queue-20261115.md)

**4D-8 Native(10.78 Client) 분기.** 실제 cpppj 오라클은 시작 때 RawSurfaceStorage에 Native 장치를 결합한다. typed Session과 같은 상태를 비교하려면 실제 Client 소유자 아래 Native 경계를 함께 맞춰야 한다.

- [ ] **4D-8a RawSurface 삭제·수명·공통 11단위** — AssetDeletion·AssetNotice·Attack·Content·Decoration·Deletion·Depend·FrameRegular·Lifetime·PlayerRegular·RegistryDeletion.
- [ ] **4D-8b RawSurface 자원·생산·사제 9단위** — Carrier·DudeRegular·Priest·PriestDeletion·FactoryRegular·Mana·ResidenceRegular·Reward·Gump.
- [ ] **4D-8c RawSurface 전투·의식 9단위** — Balloon·Bomb·Damageable·Fence·Flare·Ice·DaisDeletion·Ritual·Vortex. 소비자 연결은 7단계와 연계한다.
- [ ] **4D-8d Native 건설·Vortex** — ClientConstructionGlobals·Regions·RandomPlacement·Placement·Devices·Gem, AssetDeletionVtables와 RawCampaignVortex. 입력·건설 나머지는 6단계다.
- [ ] **4D-8e Native 고정 수신기** — NativeLogin/Gameplay/Carrier/ConstructionPacketReceivers·ClientChatPacketReceiver·FortCoreDataLoad와 Session의 고정 수신(4063..4099, 5437..5582). canonical 기준선·대여를 함께 확인한다(3.6절).
- [ ] **4D-8f 공통 후처리·Fort 가상** — SquidPostPop의 실제 World 등록·기본 장치·자원/AI 통지와 NativeFortSaveVirtual.
- [ ] **4D-8g Native 분기 재대조** — 대응표와 단위 문서에서 남긴 `UsesNativeTransmissionDevices()` 분기, 장기 Storage owner, 보상/삭제 공유 장부, ProcessHostStorage·Kernel procSumm을 함께 확인한다.

**4D-9 웹 session-dump·오라클 시나리오.** 4D-9b 오라클 시나리오는 완료했다. E868에서 `tools/cpp-oracle.mjs`의 `session-dump` 단계·고정 프레임 인자와 결과 수집을, E869에서 고정 시각 Tutorial1의 실제 Load/Start 덤프·화면·보고서 수집을 연결했다. 웹 비교는 4D-10에 남는다. [E868](docs/webpj-cpp-oracle-session-dump-20261119.md) · [E869](docs/webpj-tutorial1-cpp-oracle-20261119.md)

- [ ] **4D-9a SessionStateDump**
  - 남음: 실제 Client 수명과 원본 덤프 호출 시점(Load/Start)의 연결.
  - 완료: 판번호 1 순수 TSV 서식과 분산 SP 복호(E865), 실제 Session의 시계·난수·Kernel/Regular·풀·Player/동맹·SP·Graph·TotalMade 수집(E866~E867), 앱의 검사 전용 `sessionDump()`(E1129). [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md) · [E865](docs/webpj-session-dump-format-20261119.md) · [E867](docs/webpj-session-dump-world-20261119.md)

**4D-10 재대조와 L2**

- [ ] **4D-10a 실제 소유자 재대조**
  - 남음: 남은 typed 선행 바인딩과 ClientGameplayGlobals, GameAux·TypeNames·RawBombBlastBindings를 실제 Client/Session 및 Native 장기 경로로 확인한다. Archer·Motion/Path·Cursor·SoundProcess 소비자 검증은 6~8단계와 연계한다.
  - 완료: CampaignGuideRelease의 실제 Client 금지값 참조와 Session 보유·무World 해제(E882). [E882](docs/webpj-client-guide-release-20261119.md)
- [ ] **4D-10b L2 판정** — 같은 시점·난수에서 Tutorial1 Load/Start의 보고서·덤프·첫 프레임 논리 화면을 맞추고 실제 결선 단위를 대응표에서 `session`으로 올린다.
  - 남음: 고정 시각 실행 모드로 행 전체 일치, `later.tsv` L3·첫 프레임 논리 화면(도움말 창 가운데 배치 확인 포함), Player 기록 앞 4바이트·Player 1 +0x68 원인 규명.
  - 완료: 판번호 1 덤프 행별 비교와 SP 소유자 0 제외(E870), 원본 테마 기본값으로 안쪽 613개 일치·경계 45개는 원소 후보 구간 내(E873~E874), 실제 앱 `sessionDump()`로 시작 직후 덤프가 시간 의존 값(시계·난수·정기 시각·풀 해시·애니메이션 프레임·앰비언트 바람 객체)을 빼고 같음(E1129, `e2e/l2-tutorial1.spec.js`), 안내·진행 창 지도 영역 배치(E1130~E1131). [E870](docs/webpj-session-dump-compare-20261119.md) · [E874](docs/webpj-tutorial1-border-candidates-20261119.md) · [E1129](docs/webpj-l2-tutorial1-20261121.md) · [E1130](docs/webpj-tell-window-map-view-20261121.md)


#### 5-A 렌더러·Draw·시각 시스템 (44/46)

- [ ] **ClientRendererFrame** — 미션 화면 출력의 원본 순서를 앱에 연결하고 있다.
  - 완료: 지형 수집 범위·행 한도 90행(E978), 지면·객체·입자·커서 수집과 원본 정렬·출력(E979~E980), Screen DIB·구름·Renderer 합성(E981), 표시 준비·Present·미션 화면과 임시 Tell 출력(E982~E984), Gump kind 2 수집·출력과 월드 부착 투영(E988~E989), Gump 이동 dirty(E1081), 도킹된 지도 영역(E1082).
  - 남음: 채팅 이름 완성·섬광(지식 HUD 공장은 웹 범위 밖), RawMenuControls(5-C)·GameWorld(4D-6a) 선행, ClientRendererFrameGlobals·GumpVisual 확인, Kencloud 메뉴 연결과 구름 배경의 알려진 차이 해소.
  - 근거: [E978](docs/webpj-renderer-collection-geometry-20261120.md) · [E981](docs/webpj-campaign-renderer-drawing-20261120.md) · [E984](docs/webpj-app-mission-display-frame-20261120.md) · [E989](docs/webpj-app-gump-world-projection-20261120.md) · [E1081](docs/webpj-gump-redraw-cost-trails-20261120.md) · [화면 문서](docs/webpj-screen-20261113.md)
- [ ] **E197·E198·E201 재대조** — 웹 조립으로만 확인한 시각 시스템을 실제 Session의 cpppj 시험과 대조한다. 7단계와 중복되는 시험은 한 번만 한다.

#### 5-B 카메라·HUD·미니맵 (9/13)

- [ ] **UberHud** — E1082에서 Uber 루트 교체 설치·HudDock 모드 3 도킹·화면 크기 변경 재배치·월드 해제 정리를 앱에 연결했고([좌측 HUD 틀](docs/webpj-uber-hud-left-dock-20261120.md)) E1169~E1170에서 채팅 창 공장(`createChat`)을 연결했다. 지식(NSInfo) 공장(`createKnowledge`)과 HudNsInfoMeta·NativePlayerInfoInput·NativeNsInfoInput·HudPlayerButtonDraw는 멀티/메타 준비 창이라 웹 범위 밖으로 판정했다([E1171](docs/webpj-nsinfo-scope-20261122.md)). 남음: 채팅 이름 완성·섬광·로그 원천.
- [ ] **RawMinimapControls** — E1082에서 Gump 루트·월드 Pop/Unpop dirty·카메라 입력에 연결하고 Tutorial1 화면의 미니맵 칸을 원본 스크린샷과 대조했다(85×82픽셀 일치). 클릭·끌기·F5 복귀는 E1128에서 e2e(`e2e/minimap.spec.js`)로 고정했다([몽키·미니맵](docs/webpj-monkey-minimap-20261121.md)). 원본 fixture 전체 대조가 남았다.
- [ ] **FloatingSpGump·RawProductionControls** — 실제 표시 소비자를 연결하고 InventoryItemGump 분리 뒤 잔여를 확인한다. E1032~E1035에서 생산 패널을 조립·앱 설치하고 E1074에서 건설 비용 표시 Gump를 커서·게임 시계에 연결했다. E1082에서 생산 패널을 Uber 루트의 직계 자식(폭 80·높이 H−80)으로 도킹해 좌측 사이드바 위치를 원본과 맞췄다.
- [ ] **Cursor·RawCursor 표시** — 선행 커서 부품을 실 화면에 연결한다. 실제 카메라 투영·가장자리 스크롤·마나 볼트 정기 표시를 함께 대조한다.

#### 5-C 게임 화면 허브·미션 진입 (1/6)

- [ ] **Uber 허브** — 실제 Client 등록표의 serial·현재 HUD 판정(E914), 공통 구분선 생성·입력·부착(E916)과 프레임 방송 분배(E917), UberRootGump 전체 생성·교체의 원본 160행 대조(E912), `ClientUberHud`로 구분선·미니맵·생산 패널을 소유하는 Uber 루트의 앱 설치(E1082)를 마쳤다. 채팅 창(E1169~E1170)까지 연결해 HUD·덱·미니맵·채팅을 소유한다(지식 NSInfo 공장은 웹 범위 밖, E1171). 남음: 채팅 이름 완성·섬광·로그 원천. Meta Sprite 두 단위는 4D-7c와 확인한다. [E912](docs/webpj-uber-root-construct-20261119.md) · [E914](docs/webpj-uber-root-lookup-20261119.md) · [E916](docs/webpj-app-hud-divider-20261119.md) · [E917](docs/webpj-app-gump-broadcast-20261119.md)
- [ ] **메뉴·버튼 컨트롤** — RawMenuControls·RawButtonControls·RawMenuHtml·RawMenuLoading·RawTextEntryControls·RawMenuChat. RawMenuControls는 5-A의 선행이다.
- [x] **메인 메뉴·타이틀 상태** — 완료(E1184): NativeIntroState·TitleBitmapLifetime·NativeMainMenuState에 이어 시작 title 그림 세 장(title0·title6·title7, 3초씩, 입력하면 건너뜀)을 클릭하여 시작 뒤에 보이게 연결했다([문서](docs/webpj-title-screens-20261122.md)).
- [ ] **미션 진입 명령** — MissionBegin·StartDemo·환영 창 명령을 실제 Fort 요청·State 전환·Session 생성에 연결한다. 캠페인 목록 31개 미션의 시작(E985·E1010~E1016·E1157)과 Demo 목록·Auto-Demo의 StartDemo(E1092)는 앱에서 연결했다. 환영 창 명령의 미션 연결과 원본 Gump serial·취소 명령을 확인해 체크한다. [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md) · [데모](docs/webpj-demo-start-20261120.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)
- [ ] **허브 fixture** — 3.8절의 채팅·HTML 항목을 연결한다.
- [ ] **M2 / L4a** — 4-D·5-A·5-B 조립 뒤 Tutorial1 진입 직후와 N프레임 뒤의 논리 화면 차이 0을 확인한다. 영어 4:3부터 16:9/16:10과 한국어로 넓힌다.

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (1/8)

- [ ] **입력 본체** — UserInput의 상위 메뉴·브라우저 이벤트와 특수 진단의 실제 로그 장치를 연결한다. 폴링·우클릭·단축키·실제 후보 선택 및 일반·이동 모드 좌클릭을 확인했다. NB20의 실제 조각 취득→P 취소는 Session에서 검증했다. [E802](docs/webpj-campaign-user-input-movement-click-20261118.md) · [E803](docs/webpj-campaign-user-input-primary-click-20261118.md) · [E804](docs/webpj-nb20-real-cursor-cancel-20261118.md) GameWorldInput 코드 이식(선택/강조·미니맵 이동과 객체 메뉴·이동 커서, 대응표 `GameWorldInput.inc` `port`)은 완료했다. [E795](docs/webpj-campaign-game-world-input-20261118.md) · [E796](docs/webpj-campaign-game-world-menu-cursor-20261118.md)
- [ ] **게임 메뉴** — E1084에서 RawGameMenuBar·RawGameMenus(웹 범위)를 실제 Client 메뉴 작성기에 연결해 ESC 가로 메뉴막대와 우측 상단 토글 버튼, Game·View·Options·Players·About 하위와 연결된 명령을 완성했다([게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md)). 이후 Restart/Leave(E1085)·F9 플레이어 목록(E1087)·F6 지식 창(E1088)·Pause(E1089·E1163)·F7 표식(E1090)·송금·동맹(E1126)·F3 구분선 전환(E1159)·한국어 Player 이름(E1166)을 연결했다. Player 메뉴의 Send X 숫자 입력 창(E1168, [송금 창](docs/webpj-send-money-dialog-20261122.md))과 Ignore chat·Whisper To·정보 보기의 웹 범위 판정(단독 미션 도달 불가, E1167)을 마쳤다. 남음: 채팅 이름 완성·섬광·로그 원천(F3 창·입력줄·제출·수신은 E1169~E1172에서 연결).
- [ ] **객체 우클릭 메뉴** — CampaignObjectMenuClient의 건물·운반체·특수 대상 메뉴와 소유자 명령 연결.
  - 완료: 중첩 메뉴와 사제·Player·공통 객체·운반체 메뉴, 사제 우클릭 건설(E1064~E1072), 워크샾 우클릭 생산·업그레이드(E1076~E1079), 신전(AI 메뉴·지식)·배터리(Meltdown!)·발전기(Storm Power) 등 파생 본체 일곱 주소(E1127), UI 밖 좌·우 누름으로 팝업 닫기(E1135), 비활성 하위 행의 `›`와 메뉴 비활성·그림자 색 (47,24,17)(E1136), 제단 받침 우클릭의 같은 좌표 제단 조회·위임(`Altar Level I`, E1137), 운반체(Bulf·Walker, 열기 4268c0)·Archer 계열(열기 41e660) 우클릭 열기(E1138).
  - 남음: Factory 파생의 나머지, 특수 대상 메뉴와 채팅 분기, Archer 유닛 실제 우클릭과 운반체 Carrying/Drop 항목 확인.
  - 근거: [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) · [워크샾 생산](docs/webpj-browser-tutorial2-factory-20261120.md) · [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [메뉴 닫기·건설 미리보기](docs/webpj-context-menu-close-build-preview-20261121.md) · [메뉴 비활성 색](docs/webpj-menu-disabled-color-20261121.md) · [제단 메뉴](docs/webpj-altar-menu-delegate-20261121.md) · [운반체·Archer 메뉴](docs/webpj-carrier-archer-menu-open-20261121.md)
- [ ] **사제·운반·경로 바인딩** — 캠페인 Session 운반 입력과 일반 사제 이동은 E794에서, 실제 마우스의 사제 선택→이동 커서→땅 클릭 이동은 E1139 e2e로 확인했다. MetaMap·CampaignUserInputServices·앱과 저장 미션 thewarbegins/masterofwhirligigs의 전체 초기화·경로 루프 대조, 운반체의 줍기·내려놓기가 남았다. [E794](docs/webpj-session-carrier-input-20261118.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md)
- [ ] **건설·생산·경제** — 생산 유지부터 실제 화면 생산 UI까지 진행 중이다.
  - 완료: RawProductionMaintenance 원본 86행 대조와 Session 연결, ClientProductionSync·ProductionControlContext·ClientBridgeDecay(E831~E835), 앱 생산 유지·다리 공급·생산 단축키·다리 배치(E1002~E1008), 생산 격자·항목·예산·패널 조립과 앱 설치(E1019~E1035), 회수 명령(E1047), 비용 표시(E1074), Factory 메뉴와 생산·업그레이드 명령(E1076~E1079), HUD 다리 조각 집기→미리보기→놓기·사제 건설 위치 지정(신전·워크샾·제단)·제단 회수의 실제 마우스 e2e(E1139).
  - 남음: 연속 배치·자연 완주, HUD의 주기 호출과 실제 Client 메인 루프, 4-D에 남은 건설 장치의 입력 소비자, 실제 wire 배치/확정, SP·덱/생산, 다리 붕괴, RawBuildReplay·ClientBuildReplay의 FILE/최신 rec·Host·F7/Ctrl+F6.
  - 근거: [E831](docs/webpj-production-maintenance-20261119.md) · [E835](docs/webpj-client-bridge-decay-20261119.md) · [E1003](docs/webpj-app-production-frame-20261120.md) · [E1008](docs/webpj-browser-bridge-placement-20261120.md) · [E1035](docs/webpj-app-production-panel-20261120.md) · [E1079](docs/webpj-browser-tutorial2-factory-20261120.md) · [건설 기록 재생](docs/webpj-build-replay-runtime-20261116.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md)
- [ ] **공장·지식 메뉴와 기술** — ThemeTechnologyCommand에서 연구·업그레이드·지식 메뉴와 기술 명령을 연결한다. E1076~E1079에서 Factory 생산·지식 항목과 생산·업그레이드 명령을 앱에 연결했다. 연구 명령(0x03000080)도 앱 입력 고리가 소비하며(`netstorm.js` 업그레이드와 같은 분기) 영어 Tutorial2 e2e 가 Factory 지식 행 선택 → 생산 목록에 기술이 추가되는 것까지 확인한다(2026-11-21 점검). 제단 업그레이드(`Upgrade Altar to Level %d`)·`Get 5,000` 행이 붙는 HTML 기술 목록(TechnologyListMenu 004cb690)은 전역 tell 의 `SacrificeMenu`에만 쓰이고 그 메뉴는 비미션 전용이라 **웹 범위 밖으로 닫았다**([E1180](docs/webpj-ritual-reward-scope-20261122.md)). 남음: 기술 명령(ThemeTechnologyCommand) 나머지. [Factory Session](docs/webpj-session-factory-menu-20261120.md) · [Factory Client 메뉴](docs/webpj-client-factory-menu-20261120.md)
- [ ] **M3 / L3** — Tutorial1 이동·수확·건설·생산의 입력 단계표로 cpppj 상태·화면을 대조한다. Tutorial1·2와 캠페인 1-1·1-2의 브라우저 검증까지 진행한다.

#### 7 전투·의식·AI (1/6)

- [ ] **전투 바인딩** — Hit·GunQueries·Archer·Motion·BombBlast·Damageable 표와 실제 저장 미션 Session/Kernel을 대조한다. Archer의 초기 배치·비행 및 factory/clock loop는 1절의 검증 범위를 적용한다.
- [ ] **파괴·주문 효과** — DetonateInput 소비자와 RawObjectConversion의 기폭 입력 fixture를 확인한다. 주문과 Meltdown typed Load는 완료했다.
- [x] **의식** — 완료(E1180): 의식 typed Load와 단독 미션의 희생 완료 안내는 연결돼 있고, RitualChoiceInput·보상 선택 UI(`SacrificeMenu`·`*TECHLIST*` 기술 목록·`UpgradeAltar`/`GetNewTech`/`RequestStormpower`)는 비미션(전투·메타) 경로 전용이라 **웹 범위 밖으로 판정**했다. [근거](docs/webpj-ritual-reward-scope-20261122.md)
- [ ] **AI 잔여·전투 이탈** — AiTypeRequiredEnergy에 실제 타입 자산을 연결한다. RawBattleLeave 부품은 이식했다.
- [ ] **AI 세션 결선** — Start의 일반/데모 AI·동맹과 typed 프레임은 확인했다. 현재 Player·SP·명령·난수 및 장기 RawSurface 소비자를 연결해 AI 미션의 N프레임 L3을 비교한다.
- [ ] **전용 시험이 없었던 시스템** — E197·E198·E201 등을 실제 cpppj Session 시험과 대조한다(5-A와 연계).

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (1/9)

- [ ] **대본·대화창** — DialogScript·DialogBodyLayout, 현재 미션의 안내·채팅·대화 소비자. Tutorial1~6과 캠페인·데모·훈련 미션 한국어 대본(E1056·E1062·E1108~E1116)과 Tell 그림(E1063)은 연결했다.
- [ ] **소리 본체** — Sound·SoundDevice·ClientAudio를 Web Audio로 적응한다. E957~E966에서 효과음 공개 자산·선별 적재·원본 위치 음향·SoundPlayer·감쇠 파일 선택·Web Audio 장치·사전 해독·앱 설정/Session 재생·재생 수를, E968에서 최초 Fort 요청의 음향 자동 준비를, E997에서 경로 발소리를 연결했다. 실제 청취를 확인한다(Wind Noise 는 E1153 분석으로 원본 기본 설치에서 무동작임을 확정해 값 저장만 유지한다).
- [ ] **음악** — 완료: 공개 자산(E957)·선별 적재(E959), 장면 음악 선택·기한·날씨(E1059, 독립 3,113행 대조), 승리·패배 음악 1회 재생(E1061), 미션 State7·프레임·떠나기의 장면 음악(원소 곡 무작위 순환·다음 곡 미리 받기·해독 버퍼 놓기, 미션 중 메뉴 곡이 계속 나던 결함 수정, E1119), 희생 의식의 `sacrifice.mus` 요청(`services.requestMusic`, E1120). 남음: 대기실 곡, 원소 팔레트(번개 섬광은 E1163에서 원본 창 모드와 같은 무동작으로 판정), 승리·패배 곡의 요청 연결, 게임 난수 공유, 실제 청취 판정(사용자 확인 범위). [장면 음악 구동기](docs/webpj-scene-music-driver-20261120.md) · [장면 음악](docs/webpj-scene-music-20261120.md) · [결과 음악](docs/webpj-result-music-playback-20261120.md)
- [ ] **동영상** — 인트로 제공 범위를 판정하고 RawMovieControls·MoviePlayback·MoviePcmOutput·MovieLifecycle·SmackerDecoder를 브라우저 디코더에 맞춘다.
- [ ] **도움말·Tell** — QuickHelpHtml/Control/Window/Request, HtmlConditionEffect·HtmlLinkText·TellFileMenus·HelpLinkPolicy의 남은 계약(RawNsInfoContext 는 NSInfo 범위 밖 판정으로 E1182 에서 제외로 옮겼다)(ExternalHelpClient 는 WB03-AD 의 새 탭/창 열기 구현과 시험으로 E1181 에서 대응표를 닫았다). E1046·E1048~E1055에서 생산 메뉴 Help·TYPE 정보 본문·요구 에너지 그림·도움말 그림을 연결했다.
- [ ] **글자·설정·그림** — BitmapFont·KoreanResources·ClientOptionsDisk·GifImage의 웹 적응·남은 부분과 1322 글꼴 변경 영향. GifScreenshot(Shift-F7 화면 저장)은 E1100에서 브라우저 내려받기로 완료했다. [화면 저장](docs/webpj-screenshot-gif-20261120.md)
- [x] **미션 수명** — 튜토리얼 완료→다음 튜토리얼과 완료 기록 저장(E1086·E1091), 정지(Pause, E1089·E1163), Retry(Replay Mission)·Next Mission·Abort(Leave/Restart Mission·ABORT 확인 창)·메뉴 복귀(E1085·E1152·E1161)와 진행 키 저장·다음 미션 해제(E1152, 승패 조건은 판정기 입력 변경으로 만듦), 데모 360초 반복(E1125)·Auto-Demo(E1092)를 확인했다. 실제 자연 승패 뒤 저장→새로고침→다음 미션 해제는 L5 사용자 확인 범위다(5절). [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Restart/Leave](docs/webpj-mission-leave-restart-20261120.md) · [영어 Tutorial2·완료 기록](docs/webpj-english-tutorial2-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md)
- [ ] **적응 fixture** — 3.8절의 오디오·글꼴·렌더러·음악 관측을 판정한다.
- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.
  - 완료: 캠페인 목록 31개 미션의 연속 시작(E1121, Priest Training 9개는 E1157에서 실제 연결하고 Auto-Demo가 대신 돈 e2e 거짓 통과를 정정), 캠페인 16개 장시간 진행과 AI 시점 미연결 서비스·좌표 오류 네 가지 수정·소리/안내 서비스 여섯 개 연결(E1122), Early Missions·Priest Training 15개 100초 진행(E1123)과 Priest Training 9개 150초씩 26분(E1158), 스커미시 AI 7명 300초(E1124), 데모 420초 진행과 360초 재시작(E1125), 후반 4개·중반 13개 장시간 진행으로 시계 정지 누수 수정(E1132~E1133), 몽키 시험(E1128·E1134·E1165, 죽은 Gump 정리 수정 포함), 승패 안내 뒤 흐름과 진행 저장·다음 미션 해제(E1152·E1161).
  - 남음: 실제 자연 승패에 따른 진행 저장·자연 완주, 한국어 전체 확인.
  - 근거: [모든 미션 시작](docs/webpj-all-missions-start-20261120.md) · [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md) · [시계 정지 누수](docs/webpj-mission-leave-pause-20261121.md) · [Gump 정리](docs/webpj-gump-cleanup-frame-20261121.md) · [몽키·미니맵](docs/webpj-monkey-minimap-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다. E945에서 Windows의 Chromium·Chrome·Edge·Firefox로 하위 경로의 정적 모듈·자산 로드와 같은 URL 새로고침 뒤 음량 설정 복원을 확인했다. HTTP 캐시, GUI Linux와 실제 미션 진행 저장은 별도 검증한다. [E945](docs/webpj-static-deployment-20261119.md)

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 기준선 cpppj의 `session-dump`·`--fixed-frame-ms` 수집 시나리오(E868~E869)와 웹 덤프 형식·수집(E865~E867)은 갖췄고, 웹 덤프와의 행별 비교·L2 판정은 4D-10b에 남았다. L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 제품은 1323(실행 파일 아이콘, 웹은 favicon으로 별도 반영)이며 웹 비교 기준선은 제품 1313이다. 1314 최초 실행과 1320 미션 전환을 웹에 반영했다(E616·E620). 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322(갈무리11) 소스 영향 판정은 3.6절에 있다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·특정 적 공격·승리·전체 화면/음향은 1절 범위에 따라 확인한다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- 웹 링크 브라우저 검증은 E942에서 마쳤다. Chromium에서 외부 출처 탭의 `opener`와 `referrer`가 비어 있고, 브라우저가 팝업을 차단해도 현재 문서가 유지된다. [E942](docs/webpj-external-access-20261105.md#71-브라우저-검증)
- 와이드 전투 시야와 카메라 재배치를 연결할 때 16:9의 논리 크기 전환도 검증한다. WebMCP 대상 브라우저·조작/캡처/녹화 범위는 착수 시 확정한다.

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다. 2026-11-21 정리 때 다시 돌린 `node webpj/tools/port-map.mjs` 보고도 5행(대응표 밖 새 파일 3개 별도)이다. 1322(`635906357`)의 BitmapFont·ClientMain·KoreanResources·UberGump 4행과 새 EmbeddedBitmapFont·KoreanFontSettings 파일의 웹 영향을 함께 판정한다. SoundMusicSelection 행의 base는 기준선보다 앞선 `50ca1acf`였으나 그 뒤 두 커밋(`225f2322c`·`07885f694`)이 `SoundMusicSelection.h`의 주석·멤버 설명만 바꾼 것을 확인해(웹 영향 없음) 2026-11-22에 base를 `225553df`로 바로잡았다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | Client 진행 창/분리 Poll, GameClock의 실제 Client 바인딩·RawClockSynchronization 외곽, ClientConnectionGlobals의 PacketSources·PacketDelays, RawPacketCopies 현재 State/Zone의 복제·인수, RawMonitorTime의 실제 Config/Chat 부모, RawIslandRuntime의 실제 Client 및 NoWorld 소비자 결선. 아비어리 ReadProcess·WritePacket 및 공용 Form writer의 실제 Storage/World 경계 확인. |
| 4D-8 | PacketRegistrationTable·RawPacketDispatch의 canonical 승격/등록/스냅샷, RawConnectionActivity·RawMissionLogin·CampaignTransportServices·ClientCampaignTransport의 고정 수신기 대여. 장기 RawSurfaceWorld/Storage·ProcessHostStorage, Native mutable 송신·기본 Form writer, 공통 Form/Process·자산 삭제/후처리·보상과 Native 조회기. SidPool 소진 회수·진단과 GameAuxState의 새 전역 칸은 소비자와 연결한다. |
| 5-A~5-C | 프레임 카메라·Renderer, UberGump/메뉴 허브·‘Creating Islands.’ 로그인 진행 안내와 메타 Sprite 소비 경계. |
| 6~7 | 건설 ActualConfirm/ActualPlace, F7/Ctrl+F6 건설 기록·자원 갱신, 동맹 협상 입력/단독 도달, RawPathRuntime/RawPathData 관계, RawGunHit Rain override. NB20 실제 취소는 E804에서 검증했다. |
| 8~10 | TellSection 대체 파일·한국어/일본어 자원 우선순위·입력, Frame Rate·Difficulty·NB10, 스커미시 일반 시작·중앙섬·자기 Fort의 실제 소비 경계. |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했다. Native 대여는 4D-8에 남는다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | E943에서 16:9 논리 화면을 최대 1920×1080까지 지원하고 창 크기 변경과 입력 좌표를 확인했다. 전투 카메라·HUD와 WebMCP 조작·화면 캡처·소리 포함 녹화가 남았다. |
| 5 | localStorage 설정·진행 저장과 오류 대응을 구현했다. 실제 미션에서 진행값이 발생하는 8단계와 15차 초기화에서 저장 수명을 확인한다. |
| 6 | 메뉴·Tell·도움말과 갈무리11, 튜토리얼·캠페인·데모·훈련 미션 대본 전부(E1108~E1116)와 게임 안 메뉴·객체 메뉴 문구(E1118·E1140~E1141·E1166)까지 구현했다. 번역 호출이 없는 생산 창·미니맵 도움말·채팅 채널 이름과 사람 검수·실제 화면 확인이 남았다. |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. 실제 미션 수명에서의 오류 진단이 남았다. [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | 10.78 알려진 버그 중 웹 적용분을 cpppj 판정과 대조한다. NB04/29 간헐천 계산·callback, NB18 요청/수신 관문과 NB20 실제 Session 선택/취소는 선행했다. 실제 스커미시와 브라우저 조작 검증은 남았다. |
| 11 | 쉬움/보통/어려움(기본 보통). 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반. E554·E555 계산과 AiCreate 연결, E1096 Options·ESC 메뉴 Difficulty 선택과 저장을 마쳤다. E1149에서 실제 스커미시로 난이도별 AI 다리·섬 건설 속도(쉬움 ≪ 보통 < 어려움)를 확인하고 옵트인 e2e로 고정했다([난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)). 쉬움의 채취 유닛 절반(최소 1)은 실제 진행에서 편차가 커 구분되지 않아(E1156, AI가 프레임 구동이라 빨리 감기 부적합, [빨리 감기](docs/webpj-fast-forward-clock-20261121.md)) 설정 단위 Node 시험과 AI 생성 연결을 근거로 둔다. |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천. Outpost 목록 수명과 분류, 순수 규칙(E1101)·Campaign 항목·설정 화면·저장(E1102)·시작 섬 fort 묶음(E1103)·섬 생성 부모 셋(E1104)·세션 월드 생성과 시작·항목 기본값 켜기(E1105)·어려움 AI 5분 진행(E1106)·금지 유닛의 미션 허용 표 반영(E1107)을 마쳤다. AI 생산·공격 단계, 실제 전투 승패 안내·너겟 보충 확인, 비미션 너겟 정상 생성과 간헐천의 실제 Session 검증이 남았다. [규칙](docs/webpj-skirmish-setup-20261120.md) · [UI](docs/webpj-skirmish-ui-20261120.md) · [fort 묶음](docs/webpj-skirmish-fort-bundle-20261120.md) · [섬 생성 부모](docs/webpj-island-generation-parents-20261120.md) · [시작](docs/webpj-skirmish-start-20261120.md) · [진행](docs/webpj-skirmish-play-20261120.md) |
| 13 | 일본어 등 추가 언어. WOFF2가 없는 글꼴은 Google Fonts CDN을 사용하고 외부 접속 문서와 소스 감사를 함께 갱신한다. 일본어 Patch Credits에도 카피라이트를 넣는다. |
| 14 | 웹 자체 JS 사용으로 QuickJS 탑재 불필요. |
| 15 | Reset All은 E863에서 구현·검증했다. Options 최하단에서 확인 후 현재 게임 소유 설정·진행 키만 지우고 앱 정리 후 다시 로딩한다. 취소·확정·오류·최초 실행과 다른 앱 저장값 보존을 검사했고 Chromium 실제 메뉴에서도 확인했다. 영구 저장 키가 추가되면 초기화 목록도 갱신한다. [E863](docs/webpj-reset-all-20261119.md) |

- **12차 Outpost 잔여.** 배치/거리 후보 목록의 실제 소유 관계 확인, 지역 소유 통지·Factory/연구/운반·AI·전투·Client 연결, 건설·금지 목록·삭제를 확인한다. 일반 캠페인/데모에서의 제외도 확인한다. [Outpost 부품](docs/webpj-raw-outpost-lifecycle-20261115.md) · [이관 전 범위](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)

### 3.8 원본 관측 연결 이월

- [x] **채팅 허브** — `chat-callbacks`·`chat-input-recipients`를 실제 Client/RawMenuControls·RawChatRuntime과 연결했다(E1169~E1175, [채팅 조립](docs/webpj-chat-window-assembly-20261122.md)).
- [ ] **점 명령/HTML** — `html-input-dialog` 생성/공장 관측을 SaveFortNameDialog 소비자와 확인한다.
- [ ] **입력·장치** — 원본 관측 장치의 앱 입력 루프 연결을 확인하고 있다.
  - 완료: `saved-camera-key`·`construction-history-key`(TAB·U) 분배기 연결과 저장 칸·이력의 최초 무효 초기화(E1154), D 키(E1155, Tutorial2 발사기로 종류 확인 E1162)와 F1·F2·Shift-F3·F8·Ctrl-F5 확인(E1155), F3 구분선 전환(E1159), `detonate-input`·`vortex-command-input`·`scenario-object-input`의 세션 `commandObject`(신전·Meltdown·RawObjectConversion·RawObjectCommand) 연결(E1150), `rotation-key-input`(C 키)과 `rotmode` 설정 동기화·저장(E1151), `pause-key-input`(E1163). 미연결 모듈 68개 중 `move-dialog`(웹 범위 밖)·`hud-ns-info`(멀티/메타)·`raw-theme-technology`(개발자 도구) 등은 E1163에서 분류했다.
  - 남음: 채팅 이름 완성·섬광·로그 원천(배경·기록·입력줄·제출·수신은 E1169~E1172), E1163에서 대체 경로를 판정하지 못한 미연결 모듈(`raw-priest-placement-*`·`client-production-sync`·`production-color-maps`·`raw-battle-resource-frame`·`storm-power-refresh` 등), `audio-reinterpret`·`font-face-enum`·`font-face-lookup`의 웹 적응 여부와 비교할 관측 판정.
  - 근거: [화면 위치 키](docs/webpj-camera-keys-20261121.md) · [D 키·단축키](docs/webpj-production-d-key-20261121.md) · [객체 명령](docs/webpj-object-commands-20261121.md) · [회전 키](docs/webpj-rotation-key-20261121.md) · [Pause 키·미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md)
- [ ] **렌더러·음악** — `renderer-projection`·`scenemusic`의 소비자·적응 경계를 확인한다.

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

**2026-11-21.** E1166 직후 문서 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)에 이관했다. 0.1절 전체 회귀 이력(E1127~E1161)과 다음 작업의 단위별 연쇄, 3.7절 11차 행에 쌓였던 E1150~E1166 단위 문단, 4-D 전 항목·M4·객체 우클릭 메뉴·Uber 허브·3.8절 입력·장치의 단위별 긴 나열은 남음/완료 하위 목록(근거 링크 2~4개)으로, 0.1절 주제별 표는 짧은 구절과 대표 문서 2개 이내로 줄이고 조작키·객체 메뉴·장시간 점검·승패·분석 행을 더했다. 완료 체크 `[x]` 8개(0.7 실제 미션 화면·4D-1b·출하 Fort 로그인·4D-9b·GameWorldInput·E945·웹 링크 검증·15차 Reset All)는 부모 문장·표에 한 줄로 합쳤다. 8단계 **미션 수명**은 E1085·E1086·E1089·E1091·E1125·E1152·E1161로 확인 범위(튜토리얼·정지·Retry/Next/Abort·메뉴 복귀·진행 저장)가 모두 채워져 `[x]`로 바꾸고 8단계를 1/9로 올렸다. 완료됐는데 남은 것으로 적혀 있던 항목(미니맵 클릭·끌기 실사용 E1128, ESC 메뉴 상단 바 E1084, 메뉴의 정지·지식·플레이어 목록 E1087~E1089, Shift+숫자·D·C·U·Tab·Ctrl-F5 키 E1151~E1162, 데모 모드 Start E1092·E1125, 번개 섬광·`move-dialog`·`hud-ns-info`·`raw-theme-technology` 판정 E1163, E1127 v22 재실행 E1130)을 정정했고, 정리 기록의 끊긴 앵커 링크(`#left-sidebar`)를 없앴다. 대응표(135단위·31,510줄, port 887·partial 97·todo 38)·변경 보고 5행·fixture manifest 837항목·cpppj 최신 제품 1323은 재집계로 같음을 확인했다. 미션 수명 밖의 단위 체크 상태와 단계 체크 수는 바꾸지 않았다. 루트에 있던 E633~E640 webpj 로그 34개는 `extracted/webpj-e633-e640-root-logs/`로 옮겼다(0.1절 로컬 산출물).

현재 AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음)와 `11b16b77`(webpj favicon, [0.6절](#favicon))까지 반영했다. 2026-11-21 정리 때 이후 AGENTS.md 변경이 없음을 확인했다. 이전 이관 기록은 보관본의 날짜별 원문에서 확인한다.
