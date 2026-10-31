# webpj Tell 대화상자 복원: Credits·Demo·Campaign 목록 (WB03-D, 2026-10-28)

2026-10-28, **HJOW-Athlon**. 사용자 요청("AGENTS.md·LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행, 이 PC 의 디컴파일 결과물 참고, 다른 AI 가 cpppj 를 동시 작업 중이니 cpppj 파일은 수정하지 말 것")에 따라 메인 메뉴 버튼이 여는 **Tell 대화상자(원본 HtmlWindowGump)** 를 복원했다. 중간에 사용자가 "적당한 수준에서 마무리하고 문서·커밋 후 중단"을 요청해 아래 '남은 일'을 두고 닫았다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md 는 수정하지 않았다. cpppj 소스와 문서는 읽기만 했고 원본 게임·analyzeManager 는 실행하지 않았다(브라우저 검사는 Edge 헤드리스, 창 없음).

## 1. 근거와 디컴파일 호스트

현재 PC 는 **HJOW-Athlon** 이고 공통 인계의 마지막 전체 정밀 디컴파일도 **HJOW-Athlon / 2026-10-23** 이라 일치한다(재디컴파일 불필요, `extracted/refined/originals/Netstorm.c` 를 읽음). 직전 인계가 요구한 "PC 불일치 시 재디컴파일 확인"은 이번에 해당하지 않았다.

| 항목 | 근거 |
|---|---|
| 본문 준비 절차(설정 치환 → 첫 변환 → 지시문 추출 → LF→`<br>` 두 번째 변환 → 끝 LF) | 원본 `004cf270`(Netstorm.c 163793행), cpppj `HtmlWindowFactory.cpp`·`HtmlTextParser.cpp`·`HtmlWindowMetadata.cpp` |
| 전체 배치 Fit(버튼 폭·본문 폭을 20씩 넓히는 반복·패널 크기·가운데 배치) | 원본 `004c7d40`(158999행), cpppj `HtmlWindowFit.cpp` |
| 장식 패널(질감 프레임 0 + 모서리 I00~I03, 명암 테두리) | 원본 `00465d10`(82909행: 배경 생성자에 `DAT_005ca920`·`DAT_005ca924`, 후자는 158449행에서 0), cpppj `GumpDecoration.cpp` |
| 본문 글자 Gump | 원본 `004c7aa0`(158845행)가 `004bc060`(149693행)에 (폭 0, 높이 0, **글꼴 0, 색 `DAT_005bec2c`=기본 흰색**)을 넘김 |
| 스타일 글자·줄 나눔·크기 맞춤 | cpppj `StyleText.cpp`·`StyleTextBuffer.cpp`·`TextGump.cpp`(원본 `004bb430`·`004b9a50`·`004b9fb0`·`004b9f20`) |
| 메뉴 목록(여백·구분선·강조·눌린 상자) | cpppj `MenuGumpLayout.cpp`·`MenuGumpDraw.cpp`·`MenuGumpInput.cpp`·`MenuGumpControls.cpp`, 원본 메뉴 배경 생성(`FUN_004655d0(…, 0x3e00000, 0x201, …)`, HTML 메뉴는 추가 플래그 `0x40000`=눌림) |
| `@` 파일 목록 | cpppj `TellFileMenus.cpp`, [Tell @/# 파일 목록](exe/cpp-tell-file-menus-client-20261025.md) |
| 시각 기준 | `screenShots/mainMenu - Credits.png`·`- Demo.png`·`- Campaign.png` |

## 2. cpppj 와 달리 원본 기준으로 고친 곳(cpppj 는 수정하지 않았다)

1. **본문 기본 글꼴·색:** cpppj `HtmlBodyGump`/`RawMenuControls` 는 본문을 슬롯 5·설명 색(219,213,203)으로 만들지만, 원본 `004c7aa0` 는 **슬롯 0(Arial 14 굵기 700)·흰색**을 넘긴다. Demo·Campaign 스크린샷의 본문(굵은 흰 글자, 그림자 없음)과 일치하는 쪽은 원본이다. 이 불일치는 [공통 인계](../LEFT_JOBS.md)에 알릴 사항이다(cpppj 수정은 이번 범위 밖).
2. **폭만 되돌린 본문의 재배치:** Fit 의 긴 본문 분기가 마지막에 폭만 `previousWidth` 로 되돌리고 `FitHeight` 를 다시 부르지 않으므로, 그림 직전(원본은 부착 때 `layoutWidth` 비교로 dirty → `EnsureLayout`)에 현재 폭으로 줄을 다시 나눠야 한다. 이를 빼면 Demo 본문이 폭 129 로 잘린 채 폭 189 로 나뉜 줄이 그려진다.
3. **메인 메뉴 버튼 숨김:** 대화상자가 열려 있는 동안 스크린샷 어디에도 메인 메뉴 돌 버튼이 없다. `MainMenuScreen.dialog` 가 있으면 버튼을 그리지 않고 입력을 대화상자가 독점한다.
4. **`<background="nsstart">`:** 이름만 전역에 저장하고 배경 그림은 바꾸지 않는다. Credits·Demo·Campaign 스크린샷 모두 타이틀 배경이 그대로다(원본은 해당 GIF 가 없으면 배경을 만들지 않는 분기).

## 3. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/font-set.js` | 글꼴 슬롯 0~6 × 스타일 5종 → `.chfnt` 경로·가용 조합·측정·기준선 메트릭, 글꼴 18개 목록 |
| `www/ui/style-text.js` | StyleText 조각 체인: 짧은 명령 표, 대괄호 명령(`T`·`C`·`B`·`E`·`I`·`S`), 색 번호 표, 체인 측정·그리기 |
| `www/ui/scroll-text.js` | 다중 줄 본문: 입력 줄(128 한도)·표시 줄 나눔(엄격 `<` 폭, 단어 경계, 긴 단어 단독 줄)·FitContent/FitHeight·그리기 |
| `www/ui/html-text.js` | HTML 유사 본문 → 스타일 문자열, 지시문 9종 추출(우선순위·제거 범위·OkButton), 두 번 변환 준비 절차 |
| `www/ui/menu-list.js` | 세로 메뉴: 눌린 질감 상자·행 높이(+2)·좌우 여백 15·구분선(어두운 선+밝은 선)·호버 강조·체크 표시·누르는 즉시 실행 |
| `www/ui/tell-lists.js` | `$Menu=@패턴` 파일 목록(설정 객체 등록·이름순·`--` 구분선·`{cur.file}` 치환) |
| `www/ui/tell-window.js` | `TellWindow`: Fit 이식·장식 패널·본문/목록/버튼 배치·입력·Timeout·`close()`/OnExit, `createTellWindow` |
| `www/ui/main-menu.js` | `setDialog`·`update`, 대화상자 모달 입력·버튼 숨김 |
| `www/netstorm.js` | `_openTell`·`_onDialogCommand`: Tell→다음 창, `DoNothing`→닫기, 그 밖은 닫고 안내. 프레임마다 `update`(Timeout) |
| `www/assets/menu-assets.js` | 자원에 `fonts`·`colors`·`panel`(프레임 0·모서리 54~57)·`marker`(J02=60) 추가, 목록 효과음 상수 |
| `tools/prepare-assets.js` | 글꼴 18개와 `openSubGump.wav` 를 `menu` 묶음에 추가(변환 없는 복사, 묶음 약 7.0MB → 약 7.5MB) |

fortGump 프레임: A00=0(패널), A01=1(목록·버튼), I00~I03=54~57(모서리), J02=60(체크). 모서리는 패널 네 꼭짓점에 각 프레임의 기준점 오프셋을 더해 그린다.

## 4. 검증 (HJOW-Athlon, Node v24.21.0, Edge 헤드리스, 2026-10-28)

`npm --prefix webpj test` **34개 통과**(기존 25 + 신규 `tests/dialog.test.mjs` 9). 신규 검사: 글꼴 경로 규칙과 준비 도구 목록 일치·HTML 변환·지시문 추출·StyleText 해석·줄 나눔 경계(폭과 정확히 같으면 접힘)·세 대화상자 좌표·`@` 목록·입력/Timeout/닫기·**원본 스크린샷 대조**.

- **크기·위치:** Credits 패널 **234×159**, Demo 높이 232, Campaign 높이 301 이 [메인 메뉴 문서](screens/main-menu.md)의 대략 범위와 같다. 세 스크린샷 모두 목록 영역의 정렬 탐색에서 최적 원점이 이웃 원점보다 5배 이상 좋아(대부분 10배 이상) 목록·본문·버튼 위치가 한 픽셀도 어긋나지 않는다. 목록·버튼 영역 평균 채널 차이는 Credits 1.4/0.8, Demo 1.8/1.7(팔레트 표시 오차 ±3 수준).
- **캡처 노이즈:** 스크린샷의 패널 위쪽 질감 일부는 팔레트 밖 색이 섞인 노이즈라(Credits 위쪽·왼쪽 모서리 평균 차이 약 6.5, 같은 패널 아래쪽 모서리는 1.8) 패널 질감 전체의 픽셀 일치로 세지 않는다. 본문 전체 평균 차이 약 3.3~4.8 도 같은 이유다. 패널 폭은 Demo 249, Campaign 238(문서 대략값은 248, 236 ±5).
- **변이 검사:** 패딩 65→64, 폭 증가 단위 20→10, 허용 폭 차이 50→70, 목록 여백 15→14, 그림자 오프셋 1→0, 줄 나눔 `>=`→`>` 가 모두 검사 실패를 일으킴을 확인했다(허용 폭 차이 50→40 은 Demo 결과가 같아 무영향이었다).
- **실제 브라우저(Edge 헤드리스 CDP, 1024×768):** 클릭 시작 → Credits 버튼 → 대화상자(패널 395,304~629,463, 행 2) → 두 번째 행 → `CreditsNew`(More/Back/Cancel) → Cancel → 닫힘 → Campaign(행 8, 패널 393,233~631,534) → Early Missions 목록(Back) → Demo(행 3). 콘솔에는 `Tell,Blank` 의 `TELL cannot find "Blank".` 경고(원본과 같은 동작)만 남고 오류는 없었다. 실제 스피커 출력·Chrome/Firefox·16:9 화면 클릭은 확인하지 않았다.

## 5. 남은 일(다음 단위)

1. **Early Missions 목록(`offical1.overview`)·`$Checked` 행:** 체크 표시·잠금(`{DoneTutorial1}` 등 진행 값)·`MissionBegin` 명령은 형식만 옮겼고 스크린샷(`mainMenu - Campaign - Early Missions.png` 등)과 아직 대조하지 않았다. 두 열 메뉴(`menuColumns>1`)와 `%{userSpec}` 인라인 목록(User Made Campaigns)은 미구현(경고만). *(2026-10-30 갱신: `%{userSpec}`/User Made Campaigns 는 웹 판본이 제공하지 않기로 결정해 구현 대상이 아니다. Campaign 목록에서 노출하지 않는다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 맨 위 '사용자 결정' 절. 2026-10-30 WB03-I 에서 코드에 반영해 Campaign 대화상자는 6행이다 — [캠페인 하위 목록 기록](webpj-campaign-lists-20261029.md) §7.)*
2. **Credits 긴 쪽·스크롤바:** 본문이 화면보다 길면 `useSlider` 만 켜고 경고한다(스크롤바 그림·입력 없음). 도움말(Help)에 필요하다. *(2026-10-31 갱신: WB03-K 에서 본문 스크롤바를 연결했다 — [Tell 링크 기록](webpj-tell-links-20261031.md).)*
3. **링크(`<a>`)·조건(`<?`)·그림(`<!`·`~[I...]`)·`$Input`·`$Zsend`·기술 목록:** 훅 자리만 있다. *(2026-10-30·31 갱신: 조건·그림은 WB03-G·J 에서, 링크는 2026-10-31 WB03-K 에서 연결했다. `$Input`·`$Zsend`·기술 목록(`*TechList*` 등)은 아직 없다.)*
4. **Tell 지역 인자 `Name(arg1,arg2)`**(2026-10-30 WB03-J 에서 완료 — [시작 창 기록](webpj-startup-dialogs-20261030.md)), 현재 미션 스크립트 조회, 한국어(`.korean` 자료·D2Coding), `<background>` 그림 교체(`nsisles` 자료는 원본에 없음).
5. MenuGump 펼침 메뉴(Help·Options), 시작 팁 창, 설정 저장.
6. `StartDemo`·`MissionBegin` 명령은 대화상자를 닫고 콘솔 안내만 한다(미션 단계 구현 때 연결).
