# webpj 일반 도움말(F1) 창 복원 (WB03-G, 2026-10-30)

2026-10-30, **HJOW-X3D**. 사용자 요청("AGENTS.md·LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")에 따라 인수인계의 첫 번째 '다음' 항목인 **General Help - F1 도움말 창**(본문 스크롤바·링크·조건·그림)을 복원했다. 메인 메뉴 Help 펼침 메뉴의 `General Help - F1` 행과 F1 키가 이 창을 연다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(cpppj 는 읽기만 함). 원본 게임·analyzeManager 는 실행하지 않았고 브라우저 확인은 Edge 헤드리스(창 없음)다. **디컴파일 소스는 읽지 않았다.** 마지막 전체 디컴파일 호스트는 HJOW-Athlon 이고 현재 PC 는 HJOW-X3D 라 AGENTS.md 규칙상 디컴파일 분석이 필요하면 재디컴파일이 선행돼야 하지만, 이번 단위의 근거는 cpppj 소스와 원본 스크린샷뿐이라 해당 사항이 없었다(WB03-E·F 와 같은 방식).

## 1. 근거

| 항목 | 근거 |
|---|---|
| 창 구조(450×350, 본문 배경·안쪽 단색 칸·버튼 배치)·열리는 위치(화면 폭 가운데, y 40) | cpppj `QuickHelpWindow::Construct/Attach`, `QuickHelpControl::OpenQuickHelpWindow` |
| 방문 기록·Back·OK·창 밖 누름·키 스크롤 | cpppj `QuickHelpControl::DispatchQuickHelpWindow`, `QuickHelpHistory` |
| 주제 본문 선택(`<a name="…">` 앞부분 일치, `~5~w`, 앵커 닫힘에서 중지)·링크 표·조건·그림 변환 | cpppj `QuickHelpHtml`, `QuickHelpWindow::Show`, `HtmlLinkText`, `HtmlConditionEffect`, `HtmlPictureText`, `RawMenuControls::RenderHelp` |
| 스크롤바(값·손잡이 계산·화살표·끌기)·본문 스크롤(SetScroll·링크 판정) | cpppj `ScrollGump`·`ScrollGumpWidgets`·`ScrollingTextGumpWidgets`·`TextGump`(`ScrollingTextGump`), `ButtonGump`(반복 0.4/0.05초) |
| 대각선 선 그리기 | cpppj `VFXLine` |
| 도움말 파일 | 설정 키 `Quick Help Spec` = `{DataDir}\{Quick Help Filename}` = `\d\help.<언어>`(TAFF 안, 영어 120,888바이트·138앵커) |
| 시각 대조 | `screenShots/help - NetStorm Instructions.png` |

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/ui/help-window.js` | `HelpWindow`(창 배치·그리기·입력·방문 기록·`cmd:`/바깥 주소 링크), `createHelpWindow`(설정 키로 도움말 파일 읽기, 없으면 null) |
| `www/ui/help-topic.js` | `renderHelpTopic`: 도움말 파일 → 주제 구간 StyleText(`Inside`·`BeforeClose` 훅, 링크 표, 조건, 그림, 설정 치환) |
| `www/ui/slider.js` | `VerticalSlider`: 위·아래 화살표(돌 버튼+삼각형)·눌린 질감 홈·튀어나온 손잡이, 값 규칙(`SetRange/SetPage/SetPosition`), 화살표 단계·반복, 홈 쪽 이동, 손잡이 끌기 |
| `www/render/vfx-line.js` | 원본 VFX 선(수평·수직 양 끝 포함, 대각선은 0.32 분수 위상) |
| `www/ui/scroll-text.js`(확장) | `scroll`·`setScroll`(스크롤바 동기화)·`scrollBy`·`handleStep`·`hitTest`·스크롤된 그리기·`attachSlider`/`placeSlider`·dirty 줄 나눔, 그림 조각 배치 |
| `www/ui/html-text.js`(확장) | `LinkTable`(링크 목적지 → `~[T번호]`), `htmlPictureText`, `applyHtmlCondition`(`<?식>`: 불리언·`??`·`g/ge/l/le`·`!`·숨김 중첩) |
| `www/ui/style-text.js`(확장) | 그림 조각(`createPictureFragment`, 측정·기준선·그리기), `~[I이름.프레임]` 해석 훅 |
| `www/assets/menu-assets.js`(확장) | `resources.resolvePicture`(fortGump 숫자 프레임) |
| `www/netstorm.js` | `F1Help` 팝업 명령·F1 키·PageUp/PageDown/위·아래 화살표·휠 연결, `_openHelp` |

## 3. 동작 규칙

- **배치:** 창 450×350, x = (화면 폭 − 450)×0.5(float32), y 40(1024×768 에서 (287,40)~(737,390), 16:9 1366 폭에서는 x 458). 본문 배경은 (17,17)에서 폭 420·높이 293(눌린 질감 테두리, 질감 프레임 0)이고 그 안쪽 (2,2)에 폭·높이 −4 의 단색 칸(RGB 55,51,54)이 있다. 본문은 그 칸(416×289)을 채우고 오른쪽 9픽셀이 스크롤바다. Back·OK 는 폭 38 로 같고 간격 10, 창 아래에서 10 띄운다(OK 글자 폭 + 12 의 1.2배를 정수로 절삭해 통일).
- **본문:** 글꼴 슬롯 5 · 설명 색(219,213,203). 제목 `<h2>` 는 슬롯 3 굵은 글자 + 그림자, 링크는 파란색(`~l`) + 그림자다. `F1Help` 는 처음 보이는 주제이고 `<?{global.inMission}>` 안의 F8 안내는 미션 밖에서 숨는다(웹에는 아직 미션 없음).
- **링크:** 누르는 즉시(효과음 없음) 따라간다. `#앵커`·앵커 이름은 도움말 주제(방문 기록에 쌓음, 마지막과 대소문자만 다르면 쌓지 않음, 스크롤은 맨 위로), `cmd:이름,인자` 는 호스트 명령(`Tell,TechSupport` → Tell 대화상자), `http://` 등 바깥 주소는 열지 않고 알린다(과거 도메인).
- **Back/OK/닫기:** Back 은 이전 주제로(없으면 창 닫기), OK·창 밖 누름은 닫기다. 돌 버튼은 누르면 효과음과 눌린 모양, 안쪽에서 뗄 때만 실행한다.
- **스크롤:** 링크가 아닌 글자·빈 곳을 누른 채 끌면 끄는 반대 방향으로 스크롤된다. 스크롤바: 화살표는 안쪽에서 뗄 때 5픽셀, 누르고 0.4초 뒤부터 0.05초마다 반복한다. 손잡이 위·아래 홈은 한 쪽(본문 높이)씩, 손잡이는 끌어서 위치를 옮긴다. 키: 위·아래 화살표 10, PageUp·PageDown 한 쪽. 위치는 (전체 높이 − 보이는 높이) 이하·0 이상으로 제한된다.
- **방문 가능 주제:** 도움말 파일의 128개 주제(앵커 첫 이름)가 모두 오류 없이 열린다. 본문 줄 수는 가장 긴 `statsHelp` 가 100줄이라 원본 128칸 한도에 걸리는 주제는 없다.

## 4. 검증 (HJOW-X3D, Node v24.21.0, Edge 헤드리스)

`npm --prefix webpj test` **62개 통과**(이번 신규 12: `tests/help.test.mjs`).

- **스크린샷 대조(원점 (4,34), 다음 후보와 큰 차이):** 창 영역 평균 채널 차이 **0.29**, 본문 0.12, 스크롤바 열 1.59, 버튼 0.27, 본문 밝은 글자 마스크 6,297/6,297(일치율 1.000), 화살표 삼각형 위·아래 각 15/15 픽셀. 이웃 위치(±1~2픽셀)로 옮기면 본문 9.3~13.7, 스크롤바 15.8~50.4 로 크게 나빠진다. 즉 창 질감·테두리·본문 글자·스크롤바 손잡이 위치까지 사실상 픽셀 일치다(차이가 큰 점은 캡처 노이즈 2픽셀뿐).
- **값·입력:** 스크롤바 초기값·범위·쪽·위치 제한·손잡이 사각형, 화살표 단계/바깥에서 떼면 불발/0.4초 뒤 반복, 홈 쪽 이동(위·아래), 손잡이 끌기(범위×(이동량+손잡이 위쪽)÷(높이−폭×2)), 본문 스크롤 제한·링크 판정·스크롤된 그리기, 링크 표·조건 연산자·숨김 중첩, 주제 본문(F1Help 12개 링크·다음 주제와 섞이지 않음·조건·앞부분 일치·없는 주제), 창 입력(링크 이동·Back·OK·창 밖·`cmd:`/바깥 주소·끌기·화살표·쪽·키·휠), 메인 메뉴 `dialog` 자리 연결, 본문 그림(해석·줄 배치·그리기·새 줄).
- **변이 검사 17종**(스크롤바 손잡이 위쪽 −1, 화살표 색, 본문 테두리 눌림, 끌기 방향, 방문 기록 대소문자, 첫 주제 Back, 스크롤 무시, 조건 반전, 앵커 대소문자, 창 밖 누름, 반복 지연, 쪽 이동 방향, 그림 y 기준점·줄 바꿈·폭·기준선·해석기 대소문자): 첫 시험에서 **쪽 이동 방향 반전을 놓쳐** 손잡이 위쪽 홈 검사를 보강했고 이후 17종 모두 걸렸다.
- **실제 브라우저(1024×768 및 1920×1080 = 논리 1366×768) 각 16/16:** Help 펼침 메뉴 → General Help 행으로 열림·링크 클릭 주제 이동·휠·PageUp·아래 화살표·본문 끌기·Back 복귀·첫 주제 Back 닫기·F1 키로 열기·OK 닫기·창 밖 누름·`cmd:Tell,TechSupport` 링크가 Tell 대화상자를 엶. 콘솔 오류 0(favicon 404 제외).

## 5. cpppj 와 다르게 정한 점·알려진 차이·미확인

- **화살표 삼각형 색:** cpppj 는 슬라이더 프레임 인자(1)를 화살표 색으로 쓰는 것으로 읽지만 이 팔레트의 1번은 어두운 빨강(99,38,22)이고 원본 스크린샷의 두 삼각형은 순수 검정(0,0,0)이라 **검정(색 번호 0)** 을 쓴다(`SLIDER_ARROW_COLOR`). cpppj 쪽 해석 확인이 필요하다(공통 인계에 알릴 사항).
- **웹 적응:** 마우스 휠로 스크롤(한 칸 45픽셀 = 세 줄, 원본에 없음), 바깥 주소는 열지 않음, 도움말 파일이 없으면 열지 않음.
- **원본에서 알 수 없어 만들지 않은 것:** Esc 로 도움말 닫기(원본 동작 근거 없음), 본문 위 커서 모양(끌기 가능 커서 15·링크 커서 16: 커서 리소스가 아직 없음), 도움말을 Tell 대화상자의 링크에서 열었을 때 Back 이 이전 Tell 을 다시 여는 동작(cpppj 의 `popup.command` 재등록). *(2026-10-31 정정: 이런 동작은 원본에 없다. `popup.command` 는 미션 스크립트가 연 도움말과 지식 창에서만 표시되며 Tell 링크로 연 도움말의 Back 은 도움말만 닫는다. 링크 분배 규칙(`cmd:`·주제·바깥 주소)은 `ui/help-link.js` 로 옮겨 Tell 창과 함께 쓴다 — [Tell 링크 기록](webpj-tell-links-20261031.md) 4절.)*
- **타입 정보 머리글(`<info>`)·타입 그림:** 타입 표(.type)가 없어 `<info>`(능력치 머리글·그림)는 건너뛰고 타입 이름 주제(`priestType` 등)는 본문만 나온다. 그림은 fortGump 숫자 프레임(저장 에너지 아이콘, `moneyGumpHelp`·`salvageHelp`·`priestType` 등)만 지원하며 `mana`·`bomb*`·`windVortex.*`·`fortgump.L1` 같은 이름·글자 프레임·`*` 프레임은 그리지 않고 알린다(주제 41개에 해당). **아이콘의 기준선 정렬은 아이콘이 나온 원본 스크린샷이 없어 대조하지 못했다**(원본 기준점 규칙으로만 그림).
- **미션 안:** `global.inMission` 은 항상 0 이라 F8 안내가 나오지 않는다. 미션 안 도움말(우클릭 도움말·정지 소유권)은 미션 구현 때 연결한다.
- 확인하지 않은 것: 실제 소리 출력·Chrome/Firefox·터치·16:9 이외 와이드 화면의 원본 위치.
