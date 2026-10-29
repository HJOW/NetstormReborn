# webpj 메인 메뉴 펼침 메뉴 복원: Help·Options와 사용자 설정 (WB03-F, 2026-10-29)

2026-10-29, **HJOW-X3D**. 사용자 요청("webpj 다음 단계 진행", 팝업 위치 근거 부족에 대해 "일단 B 로 가자" = 웹 적응으로 만들고 차이를 문서화)에 따라 메인 메뉴의 **Help·Options 펼침 메뉴(원본 MenuGump 팝업)** 와 그 설정 저장을 복원했다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(cpppj 는 읽기만 함). 원본 게임·analyzeManager 는 실행하지 않았고 브라우저 검사는 Edge 헤드리스(창 없음)다. 디컴파일 소스는 읽지 않았고(사용자 지시, 마지막 디컴파일 호스트는 HJOW-Athlon 유지) 근거는 cpppj 소스와 원본 스크린샷이다.

## 1. 근거

| 항목 | 근거 |
|---|---|
| Options 항목·순서·체크 규칙 | cpppj `RawGameMenus::Options`, 스크린샷 `mainMenu - Options*.png` |
| Help 항목 | 스크린샷 `mainMenu - Help.png`(세 행이 17px 간격으로 붙음 = 구분선 없음), cpppj `RawGameMenuBar`(게임 안 About 하위에는 구분선이 있음) |
| 하위 메뉴(`>`)·튀어나온 모양 | cpppj `MenuGumpLayout::Reflow`(항목 폭 += 표시 폭), `MenuGumpDraw::Draw`(표시 ` >` 를 라벨 뒤에 그림), `MenuGumpWidgets::BeginVertical(0)`(추가 배경 플래그 0 = 눌림 없음) |
| 입력 규칙 | [원본 버튼 동작](videos/menu-buttons-20261003.md)(행은 누르는 즉시 실행·`openSubGump.wav`, 바깥 누름은 메뉴만 닫음), [옵션 메뉴 상태](screens/options-menu.md)(하위 메뉴는 클릭으로 열림, 값 변경 뒤 닫힘) |
| 기본값 | cpppj `ClientAudioOptions`(소리·음악 켜짐, 효과음 3, 음악 2), 스크린샷의 기본 체크 상태 |
| Esc 로 닫기 | cpppj `UberGump`(입력 `0x020000be` → 팝업 닫기) |

cpppj `UberGump::OpenMenu` 는 호스트 어댑터(Help 는 "About NetStorm" 한 줄, 팝업은 화면 가운데 근처 고정)라 **원본 메인 메뉴의 Help·Options 구성과 위치의 근거가 아니다.**

## 2. 팝업의 실제 위치 (이번에 새로 구한 측정값)

처음에는 위치 규칙을 알 수 없어 "누른 위치" 로 열기로(B안) 했으나, 스크린샷을 더 분석해 **원본 위치를 확정**했다. 방법: (1) 타이틀 그림 윗부분(팝업·버튼이 없는 영역)으로 스크린샷의 클라이언트 원점을 정하고(평균 차이 2.9~3.5, 다음 후보 11 이상), (2) 그 원점에서 팝업을 ±10 픽셀 훑어 가장 맞는 위치를 찾았다.

| 팝업 | 클라이언트 원점 | 원본 팝업 위치 | 평균 차이 | 이웃 위치(차순위) |
|---|---|---|---:|---:|
| Options | (6,34) | **(551,294)** | 2.46 | 20.7 |
| Help | (6,36) | **(630,311)** | 4.64 | 25.1 |
| Music Volume 자식 | (4,35) | 부모 (551,294) 의 자식 | 3.92 | 24.1 |

질감 위상까지 맞아서(이웃 위치는 20 이상) 위치는 확정이다. 이를 원본 8버튼 배치(1024×768: Help 버튼 (593,311), Options 버튼 (514,334), 폭 75)와 비교하면:

- **x = 버튼 가운데**: Help 593 + 37 = 630, Options 514 + 37 = 551 (폭 75 의 절반을 절삭).
- **y**: Help 는 버튼 위쪽 311 과 같다. Options 는 버튼 위쪽 334 보다 **40 위**(294)다. 이 −40 이 어떤 규칙(행 높이 17 의 배수가 아님, 메뉴 높이·화면 경계와의 관계도 확인 못 함)에서 나오는지는 **알 수 없다.** 두 번째 줄에 있는 팝업 버튼이 Options 하나뿐이라 측정값을 칸 정의(`MAIN_MENU_SLOTS` 의 `popupDy`: Help 0, Options −40)에 그대로 넣었다.

웹 5버튼 배치(폭 67, 1024 화면: Help (549,311), Options (514,334))에서는 같은 규칙으로 Help 팝업 (582,311), Options 팝업 (547,294)가 된다. 어느 위치든 목록이 화면 밖으로 나가면 안쪽으로 당긴다(원본 Attach 의 화면 안 보정과 같은 방식). **16:9 등 와이드 화면과 다른 버튼 배치에서 원본이 어디에 여는지는 자료가 없다.**

자식 목록: 부모 오른쪽 가장자리에 부모 행의 위쪽을 맞춘다(Music Volume 행은 부모 위쪽 + 135). 스크린샷과 일치한다.

## 3. 구현

| 파일 | 내용 |
|---|---|
| `www/platform/settings.js` | 설정 9개(sound·music·playWind·swapLeftRightSpeakers·soundVolume·musicVolume·edgeScroll·autoDemo·tellTips)의 기본값·검증·로컬 저장소 저장/복원(버전 1 JSON, 손상·다른 버전·차단된 저장소에서는 기본값으로 계속) |
| `www/ui/option-menus.js` | Options·Help 항목 정의(웹 항목 / `includeAll` 이면 원본 전체 항목) |
| `www/ui/popup-menu.js` | 펼침 메뉴 컨트롤러: 단계 쌓기·화면 안 보정·강조·하위 메뉴 열기·누르는 즉시 실행·바깥 누름 |
| `www/ui/menu-list.js` | `raised`(튀어나온 모양), 하위 메뉴 행(`addSubmenu`, ` >` 표시 폭·그리기) 추가 |
| `www/ui/main-menu.js` | 펼침 메뉴 보관·그리기·모달 입력·`popupAnchor`(버튼 기준점), 대화상자·크기 변경 시 닫기, Esc 용 `closePopup` |
| `www/audio/audio-system.js` | `setSoundEnabled`·`setMusicEnabled`(끄면 정지, 켜면 요청한 곡을 처음부터)·`setSoundVolumeStep`·`setMusicVolumeStep` |
| `www/netstorm.js` | 설정 생성(init)·펼침 메뉴 열기·명령 처리(Toggle·Volume·Tell)·설정 → 소리 반영·Esc·포인터 이탈 시 강조 해제 |

웹에서 뺀 항목(`includeAll` 로만 만든다): **Direct Draw / Full Screen·Resolution**(해상도·전체화면은 웹 제외, AGENTS.md), **Pass Server Diagnostic**(멀티플레이·방화벽 진단), **Pause - Shift-F9**(메인 메뉴에는 멈출 게임이 없다), **Technical Help**(외부 `help.exe` 실행). 웹 Options 는 토글 4개(Sound On·Play Music·Wind Noise·Speaker Swap L/R)·음량 하위 2개 | 토글 3개(Edge Scroll in Fullscreen·Auto-Demo·Tell Tips at Startup)이고 Help 는 General Help - F1·Version 이다. Version 은 `Tell,About` 로 "Version v10.78" 대화상자를 연다.

## 4. 설정이 하는 일 (현재)

| 설정 | 지금 하는 일 |
|---|---|
| Sound On | 효과음(button·openSubGump)을 낸다/내지 않는다(이미 재생 중인 짧은 소리는 끝까지 난다) |
| Play Music | 끄면 메뉴 음악이 멈추고, 켜면 처음부터 다시 반복 재생한다 |
| Sound Effect Volume / Music Volume | 1~5 단계가 바로 음량이 된다(1 = −40dB … 3 = −10dB, 4 = −5dB, 5 = 0dB) |
| Wind Noise·Speaker Swap L/R·Edge Scroll in Fullscreen·Auto-Demo·Tell Tips at Startup | **값만 저장한다.** 해당 기능(바람 소리·위치 소리·가장자리 스크롤·자동 데모·시작 팁 창)이 아직 없다 |

저장은 브라우저 로컬 저장소 `netstorm.settings`(버전 1)이고, 새로고침 뒤에도 유지된다(브라우저에서 확인). `?` 주소의 호스트 옵션 음량은 저장하지 않고 그 실행에서만 우선한다. 게임 데이터 설정 층(`GameData`)에는 **첫 적재 때의 값만** 들어가며 이후 변경은 그곳에 반영되지 않는다(설정 쓰기 층이 아직 없다. 미션 완료 기록 `Done*` 저장을 만들 때 함께 정한다).

## 5. 검증 (HJOW-X3D, Node v24.21.0, Edge 헤드리스)

`npm --prefix webpj test` **50개 통과**(이번 신규 13: `tests/popup.test.mjs`).

- **스크린샷 대조(원본 위치에 열어 비교):** 크기 Options 166×269 / Help 125×51 / Volume 목록 83×85(문서 대략값 165×268 / 122×51 과 ±5). 흰 글자 마스크 일치율 Options **0.999**·Help **0.999**·Volume **0.999**, 켜짐 표시(파란 점) Options 0.972(247/254)·Volume 목록 0.932(41/44). 평균 채널 차이 2.5 / 4.6 / 3.9(이웃 위치 28 이상). 하위 메뉴 행(Sound Effect Volume·Music Volume)은 행 단위로도 비교한다(` >` 표시 위치).
- **입력 규칙:** 올리기만 해서는 하위 메뉴가 열리지 않음·눌러서 열기·다른 하위 메뉴로 교체·일반 행 누르는 즉시 실행과 닫힘·구분선 무시·바깥 누름은 닫기만(입력 소비)·화면 크기 변경 시 닫힘. 메인 메뉴 연결(버튼 기준점·모달 입력·바깥 버튼은 눌리지 않음·대화상자와의 관계).
- **설정:** 기본값·검증(토글 0/1, 음량 1~5 정수)·저장/복원·손상·다른 버전·일부 잘못된 값·읽기/쓰기가 막힌 저장소·저장소 없음. **AudioSystem:** 소리·음악 켜기/끄기·재개·음량 배율.
- **변이 검사 6종**(모두 검사가 실패함을 확인하고 원복): ` >` 표시 제거, 눌린 모양으로 바꿈, 좌우 여백 15→14, 자식 y 를 행 위쪽이 아닌 부모 위쪽에 맞춤, 바깥 누름이 닫지 않음, 일반 행이 닫지 않고 실행. 첫 변이 시험에서 ` >` 제거와 바깥 누름 변이가 통과하거나 멈춘 것을 계기로 검사를 보강했다(하위 메뉴 행 단위 마스크 비교, 큰 객체를 `assert.equal` 에 넘기면 실패 때 차이 출력이 1분 넘게 걸려 불리언 비교로 교체).
- **실제 브라우저(1024×768, DPR 1) 17항목:** 저장소 비어 있음 → Options 열기(버튼 기준점 (547,294))·웹 항목만 표시·하위 메뉴 열기·Volume 5 선택(저장·증폭률 1·메뉴 닫힘)·Play Music 끄기(저장·음악 정지)·Esc 닫기·바깥 누름은 메뉴만 닫음·Help 항목·Version → About 대화상자·새로고침 뒤 설정 유지·음악이 꺼진 채 시작. 콘솔 기록 없음(오류 0).

## 6. 확인하지 않은 것·알려진 차이

- **실제 소리 출력**(스피커)·Chrome/Firefox·16:9 화면에서의 클릭·터치.
- 원본 문서 메모: 메인 메뉴 버튼은 "커서를 먼저 올린 뒤 클릭해야" 펼침 메뉴가 열렸다(원인 미구분). 웹은 올리기 없이 누르는 즉시 열린다.
- Options −40 의 규칙, 와이드 화면·다른 배치에서의 원본 위치.
- `General Help - F1` 은 도움말 창(스크롤바·링크 필요)이 없어 콘솔 안내만 했다(**2026-10-30 WB03-G 에서 도움말 창을 구현했다**: [webpj 도움말 창 기록](webpj-help-window-20261030.md)). 한국어 라벨·번역표는 아직 없다(영어 고정).
- 저장소가 차단된 실제 브라우저(개인 정보 보호 창)는 가짜 저장소로만 확인했다.
