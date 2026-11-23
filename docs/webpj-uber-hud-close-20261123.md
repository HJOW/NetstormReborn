# UberHud partial → port 판정 (WB04-E1255)

대응표 `cpppj/src/client/UberHud.inc` 행(156줄)에 남아 있던 차이(E1224 감사의 "표시만" 차이 여섯 가지)를 cpppj 호출처와 도달성으로 하나씩 판정해 행을 `partial` 에서 `port` 로 닫았다. 코드 변경은 없다(partial 27→26단위, 18,489→18,333줄, port 981).

| 남은 차이 | 원본 | 판정 | 근거 |
| --- | --- | --- | --- |
| `InstallHud` 를 미션당 한 번만 설치(원문은 UI 재구성마다 Reflow·팔레트 재적용) | `UberHud.inc:113~` `InstallHud()` 는 루트 교체(없을 때만) → `Reflow()` → 생산·미니맵 `SetPalette` → 첫 월드 대여만 mode 3 도킹 → 지도 영역 바인딩. 호출처는 `UberGump.cpp:2173`(미션 월드 준비 때)과 편집기 한 곳뿐 | **같은 결과** | 두 번째 이후 호출이 바꾸는 것은 `Reflow`(웹은 화면 크기 전환에서 `finishResize`·도킹이 같은 일: E1224 가 순서 대조)와 `SetPalette`(팔레트가 미션 중 바뀌지 않아 같은 값 재적용)뿐이다. mode 3 도킹은 첫 대여만이라 웹의 한 번 설치와 같다 |
| 지식(NsInfo) 공장 없음 | `UberHud.inc:40` `createKnowledge` → `ConstructNsInfo`(멀티·메타 Player 메뉴 바인딩) | **웹 범위 밖** | NsInfo 는 멀티플레이어/메타 준비 창이다(E1171 판정, E1230 의 "NsInfo·Meta*(멀티 메타맵) 웹 제외"). 단독 미션의 F6 지식 창은 별도(`ClientKnowledgeMenu`)로 구현·확인됨 |
| 배경 GIF(메시지 자식 0xb0) 없음 | `visitMessage`·`testMessage`·`messageEvent`·`paletteProbe` 가 메시지 배경 GIF 와 그 팔레트 파일(`HtmlBackgroundName.col`)을 다룸 | **웹 범위 밖** | E1243 이 같은 근거로 RawUberRootControls 행을 닫았다(웹 미션 HUD 는 title 을 0 으로 읽고 메인 메뉴 배경은 `main-menu.js` 가 직접 그려 M1 8장면 픽셀 차이 0) |
| `ChangeFortPalette` 경로 없음 | 호출처는 ① 위의 `visitMessage`(배경 GIF 경로), ② `CampaignClientServices.cpp:152` 의 `login.battlePalette`(미션 로그인 때 `battlePal` 설정), ③ 메타 연결·편집기 | **영향 없음** | ①은 위 판정, ②는 E1224 가 확인한 대로 HUD 생성 전에 끝나 웹은 `SetPalette` 를 설치 때 한 번만 호출해도 같다, ③은 웹 범위 밖 |
| `SubmitMonitorChat`(monitor 옵션 1) 호출 없음 | `ClientMain.cpp:1617~1624` `InterpretMonitorOptions`/`UpdateMonitorTime` 이 설정 키 `monitor`·`alert` 가 비영일 때 180초마다 `이름: !시각` 줄을 채팅으로 제출 | **도달하지 않음** | 두 키는 옵션 메뉴 항목이 없는 **숨은 설정 파일 키**(기본 0)이고, 추출한 전체 영어 대본·번역·설정 자산 어디에도 `monitor=` 설정 명령이 없다. 웹에는 `options.cfg` 가 없고 설정은 고정 키의 localStorage 라 이 키를 켤 방법이 없다. 시간 알림 로직 자체(`core/raw-monitor-time.js`, 대응표 RawMonitorTime port)는 이식돼 있어 키가 생기면 `UberGump.SubmitMonitorChat` 대응(`submitTemporaryChatFormat`)만 연결하면 된다 |
| 서버 뉴스·파일 로그 웹 제외 | `hooks.news`·`hooks.log`(`UberHud.inc:150~`) | **웹 범위 밖** | 서버 뉴스는 종료된 서버 기능(AGENTS.md 의 외부 접속 문서화 대상), 파일 로그는 웹에서 콘솔(7차: 브라우저 console 출력)로 대체 |

## 시험

코드 변경이 없어 새 시험은 없다. 대응표·manifest 시험으로 행 형식과 상태 규칙을 확인했다.

## 남은 일

- 원본 실행으로 `monitor` 키가 정말 어떤 UI 에도 없는지는 확인하지 않았다(소스·자산 검색 판정).
- 5-B 의 나머지 행(RawProductionControls·CampaignCursorClientEffects)은 그대로 남아 있다.
