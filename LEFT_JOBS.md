# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**제품 1387 / 11.01 · 6차 정리 2026-11-29(HJOW-Athlon, 문서 정리1385) · M8 관문1327 · 일본어 최종 관문1362 · 최신 ZIP1378(제품1377) · 1381 뒤 일시 중지(1382~1384·1386~1387은 사용자 개별 요청만 처리) · 새 ID 중도 참가 추가 안 함 · 마지막 디컴파일 HJOW-Athlon · CD/QuickJS 추가 연동/보간 보류 유지**

현재 상태·남은 작업·유효한 규칙만 유지한다. 정리 직전 원문 전체는 [1384 보관본](LEFT_JOBS.old.md#archive-20261129-through-1384)에, 1371~1380의 상세 경과는 [1380 보관본](LEFT_JOBS.old.md#archive-20261127-through-1380)에 보존했다. 이전 이력은 [7절](#refs), 단위별 구현·검증은 아래 근거 문서를 따른다. [dotnetpj](LEFT_JOBS.dotnetpj.md)·[webpj](LEFT_JOBS.webpj.md)는 각 인계 문서가 기준이다.

<a id="now"></a>

## 0. 현재 작업판

### 0.1 현재 위치와 재개 기준

**현재 위치(2026-11-29).** 1381 뒤 사용자 요청으로 cpppj 목표를 일시 중지했고, 2026-11-28 사용자 제보·추가 요구 세 단위(1382~1384)와 2026-11-29 DN 차이 판정 요청(1386)과 스커미시 선택 아이콘 요청(1387)만 처리했다. 진행 중인 목표는 없으며 재개 지시 전 제품 작업을 잇지 않는다. 사용자 확인·미결 자료·선택 보류는 완료로 바꾸지 않는다. 작업·마지막 디컴파일 PC는 모두 **HJOW-Athlon**(재디컴파일 없음)이다.

| 구분 | 현재 기준 / 다음 행동 |
| --- | --- |
| 제품·배포 | 제품1387/11.01(EXE 해시는 0.5). 최신 ZIP1378은 제품1377의 배포본이며 1382~1384·1386~1387을 포함하지 않는다. 새 ZIP은 아래 재개 후보다. 기존 전수/보호 확인1380은 당시 범위로 유지한다. |
| 사용자 요청1382~1384(2026-11-28) | **1382** 정지한 건설 커서의 에너지·공간 재판정(`RawCampaignCursor::Update`). **1383** Tutorial6 `global.crossbowCost`에 실제 windArcher 비용 공개, 미션 중 Difficulty 변경 차단(메인 Options는 유지). **1384** AGENTS `33aae129b` 반영: 기본 최대1280×720·800·960·1024, FSR은1920·2560·3840 폭 네 화면비를1280 입력에서 확대, 옛1366/1600·미지원FSR 저장값은 할당 전 복구. 근거·검증은 [1절](#done) 행. |
| 사용자 요청1386(2026-11-29) | DN 1.116 차이 판정. 생산 격자 부모가 삭제된 뒤 패널을 닫으면 cpppj도 이미 반납된 Form 번호를 다시 지웠다(번호 재사용 시 `GumpForm 삭제 타입 오류`). 부모 소실 `bind(token,0)` 경계에서만 `0x40`·Form 번호를 지우도록 보완했다(원본과의 의도적 차이). 근거·검증은 [1절](#done) 행. |
| 튜토리얼 안내 | 1371~1372 완료: 초기6미션/40단계/39키·영어/한국어/일본어·배경 없는 우측 하단 글자·실제 진행 상태 읽기 전용·집중22/22·GUI19경로 종료0. [요구/범위](#tutorial-next-condition) |
| LAN | 상대 봇1364~1370, 골렘1373·추가 건설1374·기존 ID 복구1375를 완료했다.1380 최신 제품 실제 TCP 생산/건설/복구에서 공유7,305/로컬51·SP 일치·세 종료0. **D9 사용자 결정: 원본대로 유지하며 전투 중 새 ID 참가를 추가하지 않는다.** [봇 안내](docs/testing/cpp-lan-opponent-guide.md) |
| 사용자 요청1387(2026-11-29) | 스커미시의 AI 수·난이도·적/동맹·금지 유닛 선택을 Options와 같은 선택 아이콘으로 통일했다. 실제 메타12화면·집중9/9·제품 UI25/25·실제GUI1회 종료0·보호2,809파일 불변이다. [1387](docs/exe/cpp-skirmish-selection-icon-1387-20261129.md) |
| 기존 관문 | 최신 M8 관문1327(제품1326)·일본어 최종 관문1362는 당시 범위의 완료 기록으로 유지한다. 이후 집중/실행 검증을 전수 관문 재실행으로 세지 않는다. [3절](#final-gate)·[검사 기준선](#baselines) |
| DN/WB 후속 | **WB:** 튜토리얼 안내 E1386~E1389([WB 검증](docs/webpj-tutorial-objective-verification-20261127.md)), 정지 커서 E1486, 가격/난이도 E1487, 1280 해상도 E1488 완료. **DN:** 가격/난이도·정지 커서는 1.215 완료, 해상도는 1.216 계획·1.229 부분 반영(DN-RES-1~4 남음), 튜토리얼 안내 DN-TUT-1~4는 미구현이다. **1386 생산 격자:** DN 1.116은 통지 중 Form 반납·Object 0으로 다르게 막았고, WB `client-gump-root.js`의 `bindObject(token,0)`도 같은 원본 무동작이다. 후속은 [DN 0.1 6번](LEFT_JOBS.dotnetpj.md#now) DN-GRID-1~2와 [WB 다음 작업 2](LEFT_JOBS.webpj.md#next) WB-GRID-1~2다([1386](docs/exe/cpp-production-grid-parent-loss-1386-20261129.md)). [공통 적용 계약](docs/tutorial-objectives-porting-20261127.md)·[DN 작업](LEFT_JOBS.dotnetpj.md#tutorial-objective-port)·[WB 작업](LEFT_JOBS.webpj.md#tutorial-objective-port). 최초 실행·글꼴·아이콘의 DN 잔여는 [0.6](#first-run)~[0.8](#exe-icon), 다른 후속은 각 인계에서 관리한다. |
| 보류·사용자 확인 | CD10.72·QuickJS 추가 연동·비행체 보간·D8 한국어 선명도는 보류다. 자연 완주·실물 LAN·물리 입력/다른 모니터·DPI/Windows11·ARM·탐색기 아이콘은 [0.3](#user-checks), 필요한 결정/자료는 [0.2](#decisions)를 따른다. |
| 재개 시 | 새 지시의 범위를 확인하고 해당 보류/제보 항목을 새 단위로 연다. 자동 플레이는 먼저 [AI 검사 안내](docs/testing/cpp-autoplay-test-guide.md)를 읽고 [도구](tools/cpp_campaign_autoplay.py)를 사용한다. MP 계약은 [DN 후속](docs/dotnet-cpp-lan-followup-20261126.md)과 공유하되 CP 검증을 DN 검증으로 세지 않는다. |

**재개 후보(지시가 있을 때만 연다).**

- [ ] **제품1387 배포 ZIP.** 1382~1384·1386~1387을 포함한 ZIP을 배포 스크립트로 다시 만든다. 완료 기준: ZIP 항목 전수와 EXE 해시가 0.5 제품 값과 일치하고, 압축 해제본 실제 GUI 종료0·배포/보호 파일 불변을 1378과 같은 방식으로 확인한다([1378](docs/exe/cpp-final-package-1378-20261127.md)).

<a id="user-requests-20261123"></a>

**사용자 추가 요구(2026-11-23 goal, 단위1328~1330 — cpppj 완료).** 세 프로젝트 공통 요구는 cpppj가 규칙을 먼저 정하고 webpj·dotnetpj가 따른다([WB](LEFT_JOBS.webpj.md#next)·[DN](LEFT_JOBS.dotnetpj.md#now)). 요구 원문과 단위별 세부는 [1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339)에 있다.

- [x] **방향키 카메라 이동(1328).** 게임 중 ↑↓←→로 카메라를 32논리픽셀/누름·Windows 반복으로 옮긴다(원본에 없는 추가 요구, 가장자리·Alt·가운데 버튼·미니맵·화면 위치 키와 별도). TextEntry·채팅 커서 이동(`TextEntryGump.cpp:194~199`)과 조각을 든 상태의 좌우 회전(`RawCampaignCursor.cpp:179~180`, 0x250000/0x270000)이 우선이다. DN/WB가 따를 계약: [근거](docs/exe/cpp-arrow-camera-1328-20261123.md).
- [x] **ESC 게임 메뉴 한국어 점검(1329).** Game·View·Options·Players·About와 하위를 실제 GUI로 점검해 누락23키·프레임 속도 번역 경계를 고쳤다. webpj에서 제보된 Players 글자 깨짐은 cpppj에서 재현되지 않았다. [근거](docs/exe/cpp-esc-menu-korean-1329-20261123.md).
- [x] **정보(About) ▸ 버전 창 Copyright(1330).** 원본 Tell `[About]`의 원작/10.78 Ticonderoga 표기를 보존하고 `11.01 … HJOW`와 `Copyright 2026 HJOW`를 세 언어 창에 추가했다. [근거](docs/exe/cpp-about-copyright-1330-20261123.md).

<a id="tutorial-next-condition"></a>

**사용자 추가 요구(2026-11-27, 1371~1372 완료).** 원본에 없던 튜토리얼 진행 조건 안내를 우선 구현했다. 공식 초기 미션6개의40단계·영어/한국어/일본어·배경 없는 우측 하단 글자·원본 진행 상태 읽기 전용 결선을 완료했다. Release 성공·집중22/22·실제GUI19경로/모두 종료0·보호2,810파일 불변이다. [조건표](docs/exe/cpp-tutorial-objectives-1371-20261127.md)·[구현/검증/화면](docs/exe/cpp-tutorial-objective-display-1372-20261127.md). 포획/희생·특정 적 공격/승리의 튜토리얼 후반 자연 전환은 사용자 확인으로 남긴다.

- **적용 범위.** 초기 미션(튜토리얼)의 미션 진행 중에만 표시한다. 일반 캠페인·스커미시·멀티플레이·편집 모드로 확대하지 않는다.
- **표시 내용.** 현재 단계에서 다음 알림창 또는 미션 종료창이 뜨기 위해 충족해야 하는 조건을 문구로 계속 보여 준다. 단순한 최종 승리 목표가 아니라 현재 기다리는 다음 단계의 실제 조건을 안내하고, 단계가 바뀌면 문구도 갱신한다.
- **표시 형태.** 화면 우측 하단에 글자만 표시한다. 글자 뒤에는 게임 화면이 그대로 보여야 하며, 별도 배경·패널·불투명 또는 반투명 상자를 넣지 않는다. 기존 HUD와 조작을 가리지 않도록 배치한다.

- [x] **TUT-1~5 cpppj 완료.** 조건 조사·39키 문구·우측 하단 표시·진행/수명 연동·허용 범위 검증을1371~1372에서 마쳤다. 세부 완료 기준과 경과는 [1380 보관본](LEFT_JOBS.old.md#archive-20261127-through-1380)과 위 단위 문서에 있다.
- **DN/WB 필수 후속(2026-11-27).** 같은40단계 안내를 두 프로젝트에도 적용한다. WB는 E1386~E1389에서 완료했고 DN-TUT-1~4는 미구현이다. [공통 계약](docs/tutorial-objectives-porting-20261127.md)·[DN-TUT-1~4](LEFT_JOBS.dotnetpj.md#tutorial-objective-port)·[WB-TUT-1~4](LEFT_JOBS.webpj.md#tutorial-objective-port).

<a id="decisions"></a>

### 0.2 사용자 결정/정보 대기

별도 지시 전에는 아래 권고 범위를 유지한다.

| ID | 항목 | 현행 기준 / 필요한 결정 |
| --- | --- | --- |
| D1 | CD10.72 범위·CD 정책 | 현재 **보류**. 재개 시 메뉴+튜토리얼 최소 범위부터 권고. 로컬 `CDDir`+표시 파일로 확인하며 CD 삽입 대기·온라인 검증은 복구하지 않는다. |
| D2 | 비행체 표시 보간 | Frame Rate 60/75/120 상한과 양보 대기는 완료. 비행체는 원문50Hz 갱신이며 보간은 선택 사항으로 미착수다. |
| D3 | 일본어 본문·다른 언어 | **1362 AI 작업 완료.** UI536키·공식43본문·help128주제·그림/용어·언어52/채팅6검사·1,480항목 ZIP/실제 GUI를 확인했다. 사람 문장/라이선스·물리 입력 검수는 0.3에 남긴다. 독일어 등 다른 언어 미노출은 유지한다. [최종 근거](docs/exe/cpp-japanese-final-gate-1362-20261126.md). |
| D4 | 알려진 버그12건 | NB28 설계 기준, NB01/06/13/14/19/21/25/26/27/32/33 재현 조건·올바른 동작이 필요하다. [건별 필요한 자료](docs/exe/cpp-known-bugs-status-20261111.md). |
| D5 | NB04·NB29 공식 | 현행 Even 구역 순서 섞기·6명 이후 인당 간헐천6개 증가 공식을 유지한다. 변경 목표가 주어질 때만 조정한다. |
| D6 | 스커미시 시작 섬 | 현행22개 모양 후보·사람 남쪽(소유자1)을 유지한다. 위치/모양 확장은 선택 사항이다. |
| D7 | QuickJS 추가 연동 | 14차 **엔진 탑재만 완료**. 게임 상태·파일·네트워크를 노출하지 않는다. 진단 JS/API/모드/미션 대본으로 넓힐 때 별도 범위를 정한다. |
| D8 | 한국어 글자 선명도(2026-11-23 기록, 작업 보류) | ① 전체화면 확대 방식: 현행 픽셀 중심 최근접 비정수 확대(`ScreenPresentation.cpp` `ScaleScreenPresentation`)를 유지할지, 정수 최근접 뒤 부드러운 축소(webpj `sharp`)나 8차 안티앨리어싱·FSR 경로와 묶을지. ② '고해상도 글자 층'(아래 0.7) 채택 여부와 글꼴(갈무리11 정수배 또는 Noto Sans KR 등 외곽선 글꼴; AGENTS.md 지정은 현재 갈무리11). ③ 한국어 큰 슬롯 배율·굵기를 webpj(제목 2배·보통체)와 맞출지. 결정 전에는 현행 유지. |
| D9 | 진행 중 LAN의 새 ID 참가 | **1380 사용자 결정 완료(2026-11-27): 원본대로 유지, 새 ID 중도 참가 기능은 추가하지 않는다.** 기존 ID의 저장 복구는1375 완료이며 현재 작업 범위를 확장하지 않는다. |

<a id="user-checks"></a>

### 0.3 사용자 확인 대기 — AI 실행 제외

| 영역 | 확인할 것 |
| --- | --- |
| 플레이·동일성 | 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리, 전체 화면/음향 동일성. |
| MP·LAN | 실제 LAN/Join/Test Battle·debugBurst·Outpost·희생 안내 UI/패킷, 메타 맵 커서 깊이0 반례, 로비 섬 사제·거주 구역 표시. TCP/IP 상대 봇으로 AI가 확인한 범위(대기실·두 참가자 시작·시계·재접속·서버 인수·Wind 신전 배치/SP/복구 저장·골렘 생산·추가 건설·기존 참가자 복구, 1364~1375, 최신 제품 재확인1380)는 [봇 안내](docs/testing/cpp-lan-opponent-guide.md)에 있다. 새 ID 추가는 D9에서 하지 않기로 확정했다. 실물 LAN·물리 환경 확인은 사용자 몫이다. |
| 한국어 | 물리 IME·클립보드·본문 문장 교정. 좁은 창의 영문/한글 혼합 H2 제목 잘림은 근본 원인 미확정이다. |
| 일본어 | 물리 IME·클립보드·번역/라이선스 저작권 줄·화면 크기별 굵기. 가변 Noto Sans JP 기본 인스턴스는 Thin이다. |
| 알려진 버그 | NB03/05/09/10/11/18/19/22/23의 실제 조작·장시간·MP·교전 확인. 건별 조건은 [현황표](docs/exe/cpp-known-bugs-status-20261111.md)를 따른다. |
| 난이도·스커미시 | AI 속도 체감·게임 중 변경 반영, 금지 유닛의 AI 적용·동맹/전투/승패·유닛 이름·MP 규칙 섬 배치 체감. |
| 화면·설치 | 60/120FPS 소리 동기, 실물4K/FSR(1384의1920·2560·3840 폭 네 화면비 출력 포함)·활성 전체화면·다른 모니터/DPI·앱 전환·청취, 실제 Program Files/UAC·CD·Windows11/ARM. |
| 초기 미션 진행 안내 | 1372의40단계 안내 구현·초기6미션 진입·실제 F4 단계 변경/F8/미션 전환·이탈은 확인했다. 튜토리얼 후반 포획/희생·특정 적 공격·승리의 자연 진행은 사용자 직접 확인이다. |
| 미션 흐름·관전 | 실제 승리 창의 Next Missions 버튼 클릭(1320), 테스트 자동 플레이 관전 카메라의 희생 의식 중 제단 이동·전체 자연 플레이/원본 전략 동일성(1312·1326). |
| 아이콘 | cpppj 배포 ZIP 압축 해제본의 탐색기 아이콘 표시([0.8](#exe-icon)). |

<a id="stages"></a>

### 0.4 AGENTS 단계별 상태

10.78의 구현 마감과 전체 플레이·물리 환경 확인을 구별한다.

| 단계 | 상태 | 잔여 |
| --- | --- | --- |
| 1차 10.78 복원 | 비MP 활성 범위 마감, 최초 상태 교정/검증1314 완료 | 자연 완주·전체 동일성은 사용자 확인. 최신 전체34진입은1327 완료, 검사 기준선은4.2절 |
| 2차 Windows10/11 | 호환 구현 마감 | 다른 모니터/DPI·Windows11/ARM 실기 |
| 3차 LAN·Outpost | 구현·정적 대조 마감, TCP 상대 봇 실행 검증1364~1370·1373~1375·1380(기존 참가자 복구까지 완료) | ZIP1378 완료·D9 원본 범위 확정([0.1](#now)), 실물 LAN은 사용자 확인 |
| 4차 MCP·와이드 | 구현 마감, 1384에서 최대1280×720·800·960·1024로 변경(AGENTS `33aae129b`) | 구체적 누락 제보 시 재개 |
| 5차 사용자 홈·맵 목록 | 구현·읽기 전용 설치 확인 | 실제 Program Files/UAC |
| 6차 한국어 | K1~K5·갈무리11 전환/상수 분리1322 완료 | 사람 검수·물리 입력·다른 DPI, 전체화면 글자 선명도는 D8 |
| 7차 오류·상세 로그 | 완료·1377 표시/Regular 문맥 보완18/18 | 던진 위치 스택 없음·ARM64 레지스터 미구현 |
| 8차 AA·FSR1.0 | 구현 마감, 기본OFF, 1384에서 FSR 출력1920·2560·3840 폭(입력1280) | 실물4K·성능·물리 출력 |
| 9차 시나리오 편집 | S1~S4 완료 | 없음 |
| 10차 알려진 버그 | 34건 판정 마감, NB18 운반물 방어 교정1339 | D4/D5·사용자 확인 |
| 11차 난이도 | 구현 완료, 1312 동맹 AI 판정 보정·1383 메인 Options에서만 변경 | 실제 전투 체감 |
| 12차 스커미시 | UI·MP 규칙 월드 완료 | D6·사용자 확인 |
| 13차 기타 언어 | 일본어 UI·본문43/43·help128주제·최종관문1362완료 | 사람문장/라이선스검수·물리입력 |
| 14차 QuickJS | v0.18.0 정적 탑재·자체 검사·ARM64 교차 빌드 완료 | D7·ARM64 실기 |
| 15차 설정 초기화 | **1325 완료** | 없음. 집중6/6·실제GUI8경로, 초기화 후 정상 종료·최초 상태 재실행 및 맵/로그 보존 |
| 후순위 | 60/120FPS 상한·대기 완료 | 표시 보간은D2, CD복원은사용자 보류 |

### 0.5 기준값

| 항목 | 현재 기준 |
| --- | --- |
| 제품 | **1387 / 11.01**(Release 성공, EXE SHA-256 `a6ec6cb839b0f6dbbd14b9932639b6550dcdb42fbcf3d9e5c78b66cdda734b65`). 1382~1384와 1386 생산 격자·1387 스커미시 선택 아이콘 보완을 포함한다. [1387](docs/exe/cpp-skirmish-selection-icon-1387-20261129.md). 이전 제품1384 EXE는 `a9be978e…`(webpj 1280 오라클 메타와 같은 값)다. M8/일본어 전수 관문은 당시 범위로 유지한다. [1386](docs/exe/cpp-production-grid-parent-loss-1386-20261129.md)·[1384](docs/resolution-policy-1280-20261128.md) |
| 실행·플레이 | 10.78 메뉴·미션·스커미시 플레이 가능. CD는 메뉴만 확인했고 튜토리얼/플레이 미완료다. 전체 원본 동일성은 미확정이다. |
| ZIP·전체 검사 | **1327 M8 통과**(제품1326). ZIP1,437항목·배포38고유경로(일반31+데모3+언어Credits3+스커미시1)·QuickJS6/6·배포1,437/보호2,808불변. 최신 전체34진입은1327이며 전체 솔루션 전수 통과로 확대하지 않는다. 최신 선정752는751통과·NB08 검사1실패 뒤 해당1건 교정 통과. [1327](docs/exe/cpp-m8-final-gate-1327-20261122.md). 그 뒤 배포 확인은1332(ZIP1,437)·일본어 최종 관문1362(ZIP1,480항목, [3절](#final-gate))·**최신 ZIP1378**(제품1377/1,480항목·실제 GUI2회 종료0·배포/보호 불변, 1380에서 ZIP/EXE 해시 재대조, [1378](docs/exe/cpp-final-package-1378-20261127.md)·[1380](docs/exe/cpp-original-lan-scope-1380-20261127.md))이다. 제품1382~1387의 ZIP은 아직 없다([0.1](#now) 재개 후보). |
| 디컴파일 | Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon(192.168.0.94)**. 이후 원문 변경·재디컴파일 없음. [기록](docs/exe/cpp-decompilation-20261103.md). |
| 보호 | AGENTS.md·originals·originalCD·기존 fixture·README 개요를 보존한다. 최근 CD 사본 실행 전후 originalCD 전체 해시는 불변이었다. |

<a id="first-run"></a>

### 0.6 최초 실행 상태 — cpppj1314 완료, DN 적용 대기

사용자 커밋 `4040c1029`로 `originals/d/options.cfg`·`originals/fullscreenStateFile.dat`가 제거됐다. **원래 처음 설치 시 없는 파일이므로 복원하지 않는다.** 기존 스크린샷의 완료 기록·옛 옵션을 기본값으로 삼지 않는다. 기준은 [원본 분석](docs/exe/original-first-run-20261116.md), 완료 근거는 [cpppj1314 구현·제품 화면·검증](docs/exe/cpp-first-run-1314-20261116.md)과 [analyzeManager1317](docs/exe/analyze-manager-first-run-1317-20261117.md)이다.

- **기준 동작.** Welcome(OK) → Welcome2(Play Early Missions / Main Menu) → 메뉴. Welcome2가 열릴 때 `welcomeGiven=1`을 저장하며 최초에는 Tips로 대체하지 않는다. Struggle For Freedom은 1번만 선택 가능하고 모두 체크가 없다. 자기 `Done*`는 체크, 직전 `Done*`는 해금이다(1번은 상수1). Options는 소리·음악·바람·가장자리 스크롤·Auto-Demo·시작 팁 ON, 스피커 교환 OFF, **효과음5·음악5**, 800×600·전체화면 요청이다. 원본 캡처용1024×768을 기본값으로 가져오지 않으며 기존 사용자 설정·진행(예: 음량3·2)은 초기화하지 않는다.
- [x] **cpppj1314·analyzeManager1317.** 빈 사용자 루트/무옵션 사본의 시작 안내·잠금·옵션과 저장/재실행·전체화면 재실행을 집중48건·제품GUI13회로 확인했다. analyzeManager는 없는 `options.cfg`의 관찰 사본에만 네 화면/경로 키를 만든다(단위114통과).
- [x] **WB 적용.** webpj E616에서 반영하고 Chromium 2/2로 확인했다. [webpj 최초 실행](docs/webpj-first-run-20261116.md)
- [ ] **DN 적용.** 빈 사용자 홈에서 같은 시작창·잠금/체크·Options·저장 수명을 반영한다. [DN 인계 0.2](LEFT_JOBS.dotnetpj.md#first-run)

<a id="korean-font-change"></a>

### 0.7 한국어 글꼴 — 갈무리11: cpppj1322·webpj 적용 완료, DN 화면 적용 대기

작은 게임 글씨에서 D2Coding이 깨져 보인다는 제보로 세 프로젝트 모두 갈무리11을 사용한다. 이 사용자 지시가 AGENTS.md의 이전 D2Coding 지정과 기존 유지/업그레이드 계획보다 우선했으며(현재 AGENTS.md도 갈무리11 지정) AGENTS.md는 수정하지 않는다.

| 대상 | 제공 파일과 상태 |
| --- | --- |
| cpppj | `fonts/Galmuri11Bitmap-Regular-2.40.4.ttf`·`fonts/Galmuri11Bitmap-Bold-2.40.4.ttf`(내부 family `Galmuri11Bitmap`)를 실행 파일 옆 `fonts/`에서 읽는다. GDI가 TTF를 등록하지 못해 `EmbeddedBitmapFont`로 외부 EBDT/EBLC를 직접 읽고, 굵은 파일에 없는 글자는 같은 일반 파일로 대체한다. 글꼴명·파일명은 `cpppj/assets/korean-font.json` → CMake 생성 상수(`kKoreanFontFamily` 등)로 파일 탐색·FontStore·배포 스크립트·ZIP 검사가 공유한다. **1322 완료**(관련47/47·실제GUI5회, 파일 바이트 내장0). [근거/화면](docs/exe/cpp-korean-galmuri-1322-20261117.md) |
| dotnetpj | 같은 두 TTF. 0.807에서 배포·ZIP 배치와 D2Coding 배포 의존 제거를 마쳤다. `KoreanFontFamily` 상수·8비트 출력 적용·작은 글씨 확인이 남았다. [DN 인계](LEFT_JOBS.dotnetpj.md#korean-font-change) |
| webpj | `fonts/Galmuri11.woff2`·`fonts/Galmuri11-Bold.woff2`를 `webpj/www/data/fonts/`로 복사해 FontFace에 등록한다. 굵기는 사용자 결정으로 보통체를 유지한다. E1288에서 12px 설계 격자·4배 그리기 칸 평균으로 Canvas 번짐을 없애고 cpppj처럼 숫자·영문·공백을 원본 `.chfnt` 글리프로 그린다. [WB 인계](LEFT_JOBS.webpj.md#korean-font-change) · [E1288](docs/webpj-korean-glyph-grid-20261123.md) |
| 라이선스 | `fonts/Galmuri11-LICENSE.txt`를 각 배포판에 글꼴과 함께 포함한다. |

**WB E1288 점검의 cpppj 해당 여부(2026-11-23, 정적 대조·기존 화면 캡처, cpppj 실행 없음).** 사용자 지시로 기록만 한다([D8](#decisions)). webpj에서 고친 세 결함(11px 축소로 획 빠짐·SP 숫자 확대·Tutorial3 D1 그림 줄바꿈)은 cpppj에 없다: strike 비트맵을 정수배 복제하고(`EmbeddedBitmapFont`) Windows-1252 글자는 `EncodeBytes`→`.chfnt`로 그린다. [근거](docs/webpj-korean-glyph-grid-20261123.md)

- [ ] **전체화면 1px 획 불균일(D8 ①).** 1024×768을 1080p 등으로 픽셀 중심 최근접 비정수 확대(1.40625배, `ScreenPresentation.cpp:50` `ScaleScreenPresentation`)해 갈무리11의 1px 획·간격이 1~2px로 들쑥날쑥하다. 창 모드 1:1은 해당 없음. 완료 기준: D8에서 정한 방식으로 1080p 전체화면의 획 굵기가 고르고 기존 화면 대조 시험이 통과한다.
- [ ] **고해상도 글자 층(D8 ②, 제안).** `GumpTextDraw` 글리프 출력 훅에서 위치·코드·슬롯·색·잘림 영역과 그리기 전후 표면 픽셀을 기록하고, 표시 단계(`StretchDIBits`/`ScaleScreenPresentation`)에서 가려지지 않은 글자만 배경을 되돌린 뒤 GDI/DirectWrite로 출력 해상도 글자를 덧그린다. 논리 화면은 바뀌지 않아 대조 시험에 영향이 없고 8차 AA 옵션과 묶는 것이 자연스럽다. 완료 기준: 옵션 ON에서 한국어가 출력 해상도로 그려지고 OFF는 현행 화면과 같다.
- [ ] **webpj와의 차이(D8 ③).** 큰 슬롯 배율(cpppj strike ppem16 → 슬롯3 1배·슬롯4 3배, webpj 2배·4배)·굵기(cpppj 굵기700 슬롯은 굵은 갈무리, webpj 보통체)·한글 기준선(cpppj 칸 기준12줄이라 슬롯0에서 원본 기준선보다 1줄 아래, webpj 잉크 기준11줄). 완료 기준: 결정된 쪽으로 맞추거나 차이 유지 결정을 기록한다.

<a id="exe-icon"></a>

### 0.8 실행 파일 아이콘·webpj favicon — cpppj1323·DN0.807·WB E894/E1446 적용, cpppj 탐색기·DN 창 확인 남음

사용자 커밋 `d047bf9a3`(cpppj·dotnetpj `Netstorm.exe`)과 `11b16b778`(webpj favicon)의 요구다. 기준은 원본 `originals/Netstorm.exe`(SHA-256 `a305414cfe0b9449…`)의 RT_GROUP_ICON **102(0x66)** 32×32·4bpp(탐색기 파일 아이콘)와 **144(0x90)** 16×16·4bpp, 언어1033이다. 원본 `R.exe`에는 아이콘이 없다. 두 이미지를 공통 [assets/Netstorm.ico](assets/Netstorm.ico)에 담았다. [추출 도구·SHA-256](docs/exe/cpp-exe-icon-1323-20261119.md)

| 대상 | 상태 |
| --- | --- |
| cpppj | `cpppj/src/app/ProductVersion.rc`에 102·144/1033을 지정하고, 창 아이콘은 `ClientMain.cpp`에서 게임 폴더의 원본 `Netstorm.exe`를 자료로 열어 같은 ID를 읽는다. Release PE 바이트 대조·Windows 추출 API·배포 `-DryRun` 통과(1323). |
| dotnetpj | 0.807에서 `Netstorm.Game`/`Netstorm.exe`의 `ApplicationIcon`으로 공통 ICO를 지정하고 EXE·ZIP의 두 원시 아이콘 바이트 일치를 확인했다. DesktopGL 창·작업 표시줄 확인이 남았다. [DN 인계](LEFT_JOBS.dotnetpj.md#exe-icon) |
| webpj | E894에서 `webpj/www/favicon.ico`와 상대 경로 링크를 넣고 바이트 대조·Chromium HTTP 200·해독을 확인했다. **E1446(2026-11-27)에서 실제 Chrome 탭의 favicon 표시를 직접 시각 확인해 완료했다.** [WB favicon](docs/webpj-favicon-20261119.md)·[WB 인계](LEFT_JOBS.webpj.md#favicon) |

- [ ] **사용자 확인.** cpppj 배포 ZIP 압축 해제본의 탐색기 아이콘 표시를 확인한다(0.3).

<a id="done"></a>

## 1. 완료 묶음과 근거

이미 마감한 구현을 다시 만들지 않는다. 단위별 완료 행·주소·상세 수치는 [1380 정리 전 보관본](LEFT_JOBS.old.md#archive-20261127-through-1380)과 각 근거 문서를 따른다.

| 묶음 | 완료 범위·근거 |
| --- | --- |
| 10.78 비MP·호환 | UI·Edit·미션·수명/파일 구현 마감. [미션 범위](docs/exe/cpp-mission-state-scope-20261106.md)·[M6 종결](docs/exe/cpp-m6n-reachability-closure-20261111.md) |
| 사용자 데이터·한국어·편집 | 최초 실행1314·한국어 정보1315·갈무리11 1322·아이콘1323·Reset All1325, K1~K5·S1~S4 완료. [최초 실행](docs/exe/cpp-first-run-1314-20261116.md)·[갈무리11](docs/exe/cpp-korean-galmuri-1322-20261117.md)·[읽기 전용 설치/시나리오](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) |
| M8·미션 흐름·검사 | 전환1320·검사 계약1304~1324·AI 진단1326·관문1327 완료. 최신 검사 범위는 [4.2](#baselines), 관문은 [3절](#final-gate) |
| 추가 요구1328~1330·주문1339 | 방향키 카메라·한국어 ESC 메뉴·버전 Copyright와 주문/골렘 포획 교정. [추가 요구](#user-requests-20261123)·[1339](docs/exe/cpp-spell-capture-1339-20261124.md) |
| 일본어1333~1362 | UI·공식43본문·help128주제·최종 관문 완료. 튜토리얼 안내 뒤 번역표는 한/일 각575키. [2절](#lang)·[최종 관문](docs/exe/cpp-japanese-final-gate-1362-20261126.md) |
| LAN1363~1375·1380 | 공통 도움말·옵션/로그인·Fort/수신·시계·인수·배치 저장·생산/건설·기존 ID 복구.1380 최신 제품 재확인/원본 범위 확정. [봇/단위별 근거](docs/testing/cpp-lan-opponent-guide.md)·[1380](docs/exe/cpp-original-lan-scope-1380-20261127.md) |
| 튜토리얼1371~1372 | 공식6미션/40단계 안내·집중22/22·GUI19경로 종료0. [조건](docs/exe/cpp-tutorial-objectives-1371-20261127.md)·[구현/화면](docs/exe/cpp-tutorial-objective-display-1372-20261127.md) |
| 로그/최신 ZIP1377~1378 | 표시 상세/Regular 문맥18/18·정상 GUI, ZIP1,480항목/GUI2회 종료0·배포/보호 불변. [1377](docs/exe/cpp-error-context-1377-20261127.md)·[1378](docs/exe/cpp-final-package-1378-20261127.md) |
| 사용자 요청1382~1384 | 정지 커서 에너지·공간 재판정(커서 집중21/21), Tutorial6 가격 변수(현 자료550)·메인 메뉴 전용 난이도(실제 대본 치환1/1), 1280 폭/화면비별 FSR(정책3/3·GPU6/6), 모두 Release 빌드 종료0. [1382](docs/exe/cpp-stationary-placement-1382-20261128.md)·[1383](docs/game-user-followups-20261128.md)·[1384](docs/resolution-policy-1280-20261128.md) |
| 사용자 요청1386 | 생산 격자 부모 소실 뒤 반납된 Form 번호의 재삭제 방지(새 검사1/1, 관련13개 수정 전후13/13, 다른 집중24/24, Release 빌드 오류0). [1386](docs/exe/cpp-production-grid-parent-loss-1386-20261129.md) |
| 사용자 요청1387 | 스커미시 선택을 기존 Options 아이콘으로 통일(수/AI별 난이도·동맹/금지), 메타12화면·집중9/9·UI25/25·GUI종료0·보호2,809파일 불변. [1387](docs/exe/cpp-skirmish-selection-icon-1387-20261129.md) |
| 선택 기능 | 난이도·스커미시·AA/FSR·Frame Rate·QuickJS 엔진 구현 완료. CD10.72는 메뉴까지 확인 후 보류. [단계별 잔여](#stages) |

<a id="bugs"></a>

### 1.1 알려진 버그

판정34건: 수정10·기반영1·미재현4·해당없음1·D4 정보 필요12·MP 미수정6이다. [NB01~34 현황표](docs/exe/cpp-known-bugs-status-20261111.md)가 근거·재현 조건의 기준이다. NB04/NB29 공식은D5, 새로운 제보는 해당 행을 다시 연다. 사용자 지시로1339에서 NB18의 잘못된 주문1 검사를 실제 운반물0x20으로 교정했고 원본과 같은 주문 보유 골렘 포획/Drop을 확인했다. [1339](docs/exe/cpp-spell-capture-1339-20261124.md)

<a id="lang"></a>

## 2. 기타 언어 — 일본어 본문 번역(D3) 완료

일본어 UI(509키+보완23키)·공식43본문·도움말128고유 주제/138앵커를1333~1361에 번역하고 최종 관문1362(용어 대조·언어52/채팅6검사·ZIP1,480항목·압축 해제본 실제GUI2회)를 통과했다. 새 본문은 `assets/lang/japanese/d/`(UTF-8 BOM)에 두며 없는 파일은 영어 대체를 유지한다. 단위 기준은 [번역판](docs/exe/cpp-japanese-translation-20261123.md), 묶음 요약은 [1절](#done)이다. 1340~1362 세부 단위 체크와 단위 공통 완료 기준은 [1370 보관본](LEFT_JOBS.old.md#archive-20261126-through-1370) 2절에 있다.

- 남은 것: 사람 문장·라이선스 저작권 줄 검수와 물리 IME([0.3](#user-checks)), 독일어 등 다른 언어 미노출 유지([D3](#decisions)). webpj·dotnetpj의 13차 반영은 각 인계 문서를 따른다.

<a id="final-gate"></a>

## 3. M8 최종 관문 — 1327 완료

제품1326의 [최종 보고](docs/exe/cpp-m8-final-gate-1327-20261122.md)·[기계 판독 결과](cpppj/m8-final-gate-1327-20261122.json)가 현재 기준이다. Release 종료0·ZIP1,437항목 전수·일반31/데모3 전체진입·3언어Credits/스커미시4경로·QuickJS6/6·아이콘 원본4payload 일치·외부 정적 감사·배포1,437/보호2,808파일 불변을 확인했다. 최신 선정752는751통과·NB08의 옛 경로 기대1실패 뒤 해당1건만 교정해 통과했으며 전체를 반복하지 않았다.

ZIP: `extracted/cpp-m8-1327/NetstormReborn-11.01-cpppj.zip`(205,883,413바이트), SHA-256 `b8d7f37d9b649db46760ae02cea4140e36bc1954d6aa5b166433a85abaeb5a05`.

기존1327 관문은 유지한다. 그 뒤 추가 요청의1332 소규모 배포 확인([근거](docs/exe/cpp-followups-gate-1332-20261123.md))과 일본어 최종 관문1362([근거](docs/exe/cpp-japanese-final-gate-1362-20261126.md), ZIP `extracted/cpp-japanese-final-1362/NetstormReborn-11.01-cpppj.zip` 206,341,786바이트, SHA-256 `f11339669585ceb515fc0a5742127944224e6e0e76ff46f092efeebcdba65288`)를 완료했다. 현재 최신 배포본은 1377 로그 보완까지 포함한 ZIP1378이다([근거](docs/exe/cpp-final-package-1378-20261127.md), 206,351,884바이트/SHA-256 `d92b32495a068a0a0a428c0fbecafc091fec36cda688557a8c871b54c129645d`). 그 사이의 ZIP1376(1363~1375 반영)은 1378로 대체됐다([근거](docs/exe/cpp-final-package-1376-20261127.md)). 사용자 직접 확인·CD/선택 보류·전체 솔루션/원본 전체 동일성 미판정은 유지한다.

### 3.1 이전 관문1311 — 제품1310 기준

1311(제품1310)의 결과 표와 ZIP 해시(`extracted/cpp-m8-1310/NetstormReborn-11.01-cpppj.zip`)는 [최종 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md)·[기계 판독 결과](cpppj/m8-final-gate-20261114.json)에, 더 이전 결과는 [1294~1297](docs/exe/cpp-m8-final-gate-1294-20261111.md)에 있다.

<a id="contracts"></a>

## 4. 유지할 계약과 검사 기준선

### 4.1 구현 계약

- 장기 Client의 Pool/TYPE/SP/HP/Canon/Chunks·목록·Player/시계/단일 난수를 공유한다. 임시 World/Player·가짜 ACK·빈 성공 callback으로 활성 호출자를 대신하지 않는다.
- weak 대여→실제 Runtime pin→함수 사본→호출을 유지한다. 본체를 backing보다 먼저 반납하며 메뉴/Session/Host 정리 뒤 장치를 끊는다. NoWorld 허용은 메타 TYPE108/109/161뿐이다. 나머지 도달 불가 경계의 명시 오류를 유지한다.
- LAN/R은 현재 TCP/UDP·등록표·Config/주소/시계 소유자를 유지한다. 3BYTE/47BYTE 존은 compact 호환 범위다. 폐기7f/80·온라인 검증·PIRATE quota·원문에 없는 관리 포트를 복구하지 않는다. IPX/SPX는 명시 미지원이다.
- 스커미시는 생성 동안만 MP 시작 규칙을 적용하고 예외에도 모드를 복원한다. 5명 이상은100개 섬 기록 제한 때문에 군도 밀도5~8%를 쓴다. 일반 미션에는 적용하지 않는다.
- QuickJS 상류 파일·provenance·`-text`를 보존한다. 엔진 include는 ScriptEngine.cpp에만 두며 quickjs-libc와 D7 미정 API를 추가하지 않는다. 메모리 한도는 할당자 callback 예산이다.
- 세부 가상표/필드/호출 순서 계약은 [1310 보관본4.1](LEFT_JOBS.old.md#archive-20261114-through-1310)과 [M6 종결](docs/exe/cpp-m6n-reachability-closure-20261111.md)을 따른다.

<a id="baselines"></a>

### 4.2 알려진 실패와 기준선 — 최신 전수 통과로 바꾸지 않는다

| 대상 | 마지막 확인 기준 |
| --- | --- |
| mission_entry(1320) | **전체119/119 통과**(1316 Template owner 교정115건 + 1320 flush·종료 수신/호출 수명4건). [근거](docs/exe/cpp-mission-transition-1320-20261117.md). |
| pop_context(1321) | **선정325: 324통과·1실패 → 일반 Sprite `0050cc08` 지원 기대값 교정 후 통과.** fixture988파일 불변. 전체1175건/옛 실패24건 재집계는 아니다. [근거](docs/exe/cpp-pop-test-baseline-1321-20261117.md). |
| world_generation(1319) | **제네레이터1/1·SharedKernel4/4 통과**(그림자 대상색 인자·외부 Client 프로세스 정리 가정 교정). AiCarrierActions 옛 실패는 제외 범위. 전체 통과로 확대하지 않는다. [근거](docs/exe/cpp-world-test-contracts-1319-20261117.md). |
| guide_fort_request(1324) | 선정89: 최초87통과·2실패 → 표본 복사/State6 오류 문자열 계약 반영 뒤 89/89(1327 재확인). |
| file_directory(1318) | **전체18/18 통과**(실제 존재조회37칸 소비 반영). [근거](docs/exe/cpp-startup-name-test-1318-20261117.md). |
| menu_session·main_popup·file(1324·1331) | 최초 menu4·file5 실패 → 교정 후 최신 menu194/194·popup79/79·file390 중389(NB08 1실패 → 교정 후 통과, 1327). 과거 전체 실패수와 구별한다. 1331 메뉴 확장256 중254통과·포획 안내2예외 → 검사 장치 교정 후 관련3/3 통과(전체 재실행 아님). [1331](docs/exe/cpp-carrier-notice-tests-1331-20261123.md). |
| 실제 미션 하네스 | 옛 AGENTS 해시 때문에 passed:false 기록이 있다. 실행/종료 결과와 보호 해시를 따로 확인한다. |

변경 전과 같은 실패인지 실제 기준선으로 비교한다. 과거 세부 로그·수치는 보관본4.2에 있다.

<a id="rules-verify"></a>

## 5. 검증 제외·남은 한계

- 사제 포획/희생·특정 적 공격·시나리오 승리·전체 자연 완주·사용자 직접 확인 검사는 건너뛴다. 단순 비MP 진입/대기/생산/건설 검사는 수행한다.
- 예외(2026-11-21 사용자 요청, 1312 방식): **캠페인1-1~1-5·스커미시에 한해서** 테스트 전용 자동 플레이(`--test-*`, [AI 사용 안내](docs/testing/cpp-autoplay-test-guide.md))로 아군 AI가 캠페인/스커미시를 진행·승리하는지 확인하는 실행은 허용된다. 이는 사람 플레이·원본 동일성 검증을 대신하지 않으며 결과는 "테스트 자동 플레이"로 구분해 기록한다.
- 2026-11-26 사용자 goal로 TCP/IP 로컬 상대 봇 개발과 MP 실행 검증을 재개해 1364~1380에서 마쳤다(D9로 범위 확정). 기존 디컴파일을 기준으로 구현하며 실제 LAN·물리 환경의 사용자 확인은 유지한다. 옛 MP 단위/통신 실행 제외는 이 범위에 적용하지 않는다. 공중3종(훨리기그·맨오워·더스트 데빌) 전용 생성/생산/행동 관측도 실행 제외다.
- 스펠 도움말 그림·연녹색 직사각형 추가 분석은 보류다. 물리 입력/캡처는 이전 환경에서 반복 timeout이었으므로 환경 변화 전 재시도하지 않는다.
- 일반 Battle/자연 완주 잔여를 임의 demo=1·상태 주입·전략/승리 단언 약화로 우회하지 않는다. TEST02 자동 공략 지원은 별도 요청 전 하지 않는다.
- 설정/경로의 EncodeOriginalText와 화면용 EncodeDisplayText/DecodeDisplayText를 구별한다. [부호화 경계](docs/exe/cpp-font-face-encoding-20261103.md)를 따른다.
- 원문32비트 ABI·미작성값·x87 전 입력·물리 I/O 장애·ARM 실기·전체 화면/청취는 검증 한계다. 활성 호출자 미연결을 이 한계로 숨기지 않는다. 약200자를 넘는 설치 경로에는 원문256BYTE 경로 작성기 한계가 있다.

<a id="rules-ops"></a>

## 6. 착수·검증·커밋 규칙

1. AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md와 git status/log를 확인한다. 마지막 디컴파일 호스트가 다르면 cpppj/원문 분석 전에 `tools/ghidra/refine_all.ps1`, 필요 시 `refine_root_server.ps1`을 다시 수행하고 호스트를 기록한다.
2. 단위마다 현행 상태/다음 번호·근거·인계를 갱신하고 **자기 파일만** `YYYYMMDD-NN CP 요약`+Co-Authored-By로 로컬 커밋한다. 실패해도 작업은 계속한다. DN/WB 파일·stage와 저장소/전역 설정을 건드리지 않고 **push하지 않는다**.
3. HJOW-Athlon은 VS18/MSBuild18, HJOW-X3D는 VS17/v143이다. 아래4병렬 명령의 실제 종료0을 확인한다. **빌드 중 제품 소스를 수정하지 않는다.** 큰 헤더 변경은 긴 재컴파일을 일으킨다.
4. 최초 실패/교정 원시·기대값을 보존한다. 관련 집중 검사 뒤 다음 단위로 가며 전체 검사/ZIP은 관문에서만 반복한다. 준비본·정적 대조·컴파일·단위 실행·GUI·실물 검증을 구별한다.
5. AGENTS.md·originals/originalCD 기존 파일·기존 fixture·README.md **개요**를 보호한다. 실행 전후 해시/허용 설정 복원·내가 시작한 프로세스만 종료를 유지한다. 현재 HJOW-Athlon은 게임/별도 창 실행 확인의 호스트 예외다.
6. 제품은 Netstorm.exe·11.01·Patch Credits(HJOW/AI/카피라이트)를 유지한다. 무인자 GUI/--help/--run/분석 CLI/--quickjs-selftest, EXE 옆 game/netstorm.tarc 우선·원본 자동 탐색 금지, 배포 스크립트의 무인자 경로 입력/빈 값 취소·에셋/글꼴/라이선스 동봉을 보존한다.
7. 외부 접속은 [정책](docs/exe/cpp-external-services-20261101.md)·[목록](cpppj/external-endpoints-20261101.json)에 기록하며 폐기 클라이언트 검증은 비활성화한다.
8. 응답·새 주석은 한국어, 새 코드/문서는 UTF-8이다. 네임스페이스·상수·함수·구조체·클래스·멤버·반복문 주석, AGENTS 번역명·외부 fonts 규칙을 따른다. **LEFT_JOBS.md는 CRLF**로 저장한다. 원시/fixture/`-text` 소스의 기존 바이트·혼합 줄바꿈은 국소 수정으로 보존한다. 코드 줄 중간의 `//`가 뒤 코드를 삼키지 않게 한다.
9. 현행 지시가 옛 보고서의 "다음"보다 우선한다. 최신 ZIP1378·검증1380까지 마쳤고 D9는 원본 범위 유지로 확정됐다. 1381 뒤 목표를 일시 중지했으며 1382~1384·1386~1387은 사용자 요청 범위만 처리했다. DN/WB 후속은 각 인계의 미완료 작업으로 유지하고 해당 프로젝트 재개 지시에 따른다. CD·선택 작업·0.1 재개 후보는 별도 재개 지시 전 진행하지 않는다.
10. **LEFT_JOBS 정리 요청 시**(AGENTS.md `1fc63cb1a`): 정리 직전 원문 전체를 [LEFT_JOBS.old.md](LEFT_JOBS.old.md) 끝에 앵커·SHA-256과 함께 보관한 뒤 만료·상세 불필요 내용은 요약만 남기고, 완료됐는데 미표시인 항목을 점검하며, 너무 큰 작업 단위는 완료 기준이 있는 세부 단위로 나눈다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 7. 근거·보관

**2026-11-29 정리(문서 정리1385, 6차).** [1384 전문](LEFT_JOBS.old.md#archive-20261129-through-1384)을 보관하고 ① 0.1의 1382~1384·중지 인계 문단, 0.3 MP 행, 0.5/3절 ZIP 경과, 7절 옛 정리 기록을 요약했다. ② 바로잡음: 0.4 4차 최대1920×1080(→1280 폭·AGENTS `33aae129b`), 0.5 제품1383(→1384), 0.8 webpj 탭 시각 확인(→E1446 완료), 5절·6절 9번의 낡은 진행 문장, DN/WB 후속 행. DN 최초 실행 저장·`KoreanFontFamily`·창 아이콘·튜토리얼 안내는 `git grep -i`로 미완료를 재확인했다. ③ 미배포 상태를 0.1 재개 후보(제품1384 ZIP)로 나눴다. `cpppj/src` 마지막 변경 `3f12a2bd4`.

**보관본(최신순).** 모두 정리 직전 문서 전체를 원문 바이트로 보존했으며 기존 보관본은 수정하지 않았다.

- [1384 정리 전 보관본](LEFT_JOBS.old.md#archive-20261129-through-1384): 0.1의 1382~1384 단위 문단·중지 인계 문단, 0.3 MP·LAN 행의 단위별 경과, 0.5 ZIP 행과 3절의 ZIP1376 해시, 0.7 webpj 행 세부, 7절의 1381·2026-11-26 정리 기록.
- [1380 정리 전 보관본](LEFT_JOBS.old.md#archive-20261127-through-1380): 1371~1380의 중복 경과·완료 세부(TUT-1~5 세부 완료 기준 포함)와 D9 응답 대기 기록. 당시 정리 근거는 [1381 문서](docs/handoff-cleanup-1381-20261127.md).
- [1370 정리 전 보관본](LEFT_JOBS.old.md#archive-20261126-through-1370): 1절의 1340~1370 단위별 완료 행(일본어 22행·LAN 7행), 2절 일본어 세부 단위 체크(1340~1362)와 단위 공통 완료 기준, 0.1의 1362·1363 완료 항목과 1340~1345 경과 문단, 0.3 MP·LAN 행과 0.4 3차 행의 단위별 경과, 0.5 제품 행의 1340~1367 경과, 7절의 2026-11-24 정리 기록.
- [1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339): 0.1의 1328~1339 단위별 경과 문단, 추가 요구 세 항목의 원 설명, D3 행·0.5 제품 행의 경과, 7절의 2026-11-23 두 정리 기록(WB 최초 실행·갈무리11·favicon 상태 바로잡기, 1절 검사 교정 행 합치기, D8 완료 기준 추가).
- [2차 정리 전 보관본](LEFT_JOBS.old.md#archive-20261123-second-through-1327): 0.7의 WB E1288 점검 세부 문장, 1절의 검사 교정 단위별 6행·M8 1311 행.
- [1327 전체 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327): 0.1의 1314~1327 단위별 상태 행, 0.6~0.8의 완료 세부(최초 실행 관찰·글꼴 로딩/검증 체크·아이콘 리소스 표), 3.1 이전 관문1311 표, 머리의 2026-11-17 원격 병합·2026-11-19 아이콘 요구 안내.
- [1310 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310): 단위1302~1310의 경과·끝난 작업 상세·AGENTS 변경 이력표·낡은 다음 지시.

주요 기준: [SOURCE_MAP](cpppj/SOURCE_MAP.md)·[디컴파일 신뢰도](docs/exe/decompile-reliability.md)·[제품](docs/exe/cpp-product-1101-20261101.md)·[미션 대응](docs/gameplay/demo-campaign-map.md)·[MCP/session dump](docs/exe/cpp-session-state-dump-20261109.md). DN/WB 요청·상태는 각 인계 문서를 따른다. AGENTS.md는 수정하지 않으며 현행1~15차·DN/WB4~15차 범위는 각 프로젝트에서 유지한다.
