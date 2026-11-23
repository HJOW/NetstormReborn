# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**제품 1326 / 11.01 · 2차 정리 2026-11-23(HJOW-Athlon) · 마지막 단위1327(M8 완료) · 다음 번호1328(추가 요청 시) · 마지막 디컴파일 HJOW-Athlon · 자동 진행 범위 마감, 사용자 확인·기존 CD/선택 보류 유지**

현재 상태·남은 작업·유효한 규칙만 유지한다. 2차 정리 직전 원문(D8·WB E1288 점검 세부, 1절 단위별 검사 교정 행 포함)은 [2차 정리 전 보관본](LEFT_JOBS.old.md#archive-20261123-second-through-1327), 1314~1327의 단위별 상태 설명과 끝난 세부는 [1327 시점 전체 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327), 그 이전 경과와 만료된 지시는 [1310 시점 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310), 개별 구현·검증은 아래 근거 문서를 따른다. [dotnetpj](LEFT_JOBS.dotnetpj.md)·[webpj](LEFT_JOBS.webpj.md)는 각 인계 문서가 기준이다.

<a id="now"></a>

## 0. 현재 작업판

### 0.1 다음 행동

**현재 자동 진행 작업 없음.** 2026-11-22의1324~1327로 자동 진행 범위를 마감했다. 추가 제보·사용자 선택/보류 해제가 오면 아래 해당 행부터 재개하고 새 단위 번호는1328을 쓴다. 기존 사용자 확인·MP 실행 제외·CD/선택 보류는 유지한다.

| 구분 | 상태와 다음 행동 |
| --- | --- |
| 최신 기준 | 제품1326 Release·**M8 관문1327 통과**. 완료 묶음과 근거는 [1절](#done), 관문 수치는 [3절](#final-gate), 검사 기준선은 [4.2절](#baselines)이다. |
| 사용자 확인·결정 | [0.3](#user-checks)의 플레이·MP·물리 환경 확인은 AI 실행 범위 밖이다. [0.2](#decisions) D1~D7은 결정 전까지 권고 범위를 유지한다. |
| **CD10.72 복원 C3~C5** | **사용자 보류(2026-11-14). 재개 지시 전 진행하지 않는다.** 메뉴·CDDir 저장 후 재실행은1309에서 확인했다. 1310의 Start/AI 설정은 순수 몸체만 검증했고 실제 Session/Guide/SP/AI 생성자에는 미연결이다. 첫 튜토리얼 차단은 `RawMenuHtml.inc:3`의 PATCH 전용 HTML 검사다. [보류 인계·재개 지점](docs/exe/cpp-cd1072-deferred-20261114.md). |
| AI 자동 플레이 정체 | 1326으로 자동 범위 완료(초기화/프레임 미연결 아님). 1-3 후반 재시도 대기·1-4 돈 정체 미재현·3-1 옛 오류의 판정은 [1326](docs/exe/cpp-autoplay-ai-diagnostics-1326-20261122.md)을 따른다. 임의 전략 변경 없이 사용자 자연 플레이/동일성 확인을 유지한다. |
| 테스트 자동 플레이 | **플레이 결과 재현이 필요한 AI는 먼저 [AI 에이전트용 사용 안내](docs/testing/cpp-autoplay-test-guide.md)를 읽고 [실행 도구](tools/cpp_campaign_autoplay.py)를 쓴다.** 명령줄 전용 `--test-autoplay`·`--test-ally-ai`·`--test-enemy-ai`·`--test-start-sp`·`--test-enemy-start-sp`·`--test-follow-battle`이며 허용 범위는 [5절](#rules-verify)이다. 1313 일괄 실행 결과는 [결과 7절](docs/exe/cpp-campaign-test-play-20261114.md#7-캠페인-전체-자동-플레이-실행-1313-2026-11-14-후속). |
| DN/WB 후속 | 최초 실행은 DN만 남음([0.6](#first-run)), 한국어 글꼴 화면 적용은 DN만 남음([0.7](#korean-font-change)), 아이콘은 DN 창 표시·WB 탭 시각 확인만 남음([0.8](#exe-icon)). 각 인계 문서에서 진행하며 CP 단위에서는 DN/WB 코드·인계를 수정하지 않는다. |
| 선택/추가 제보 | 비행체 표시 보간·번역·QuickJS 추가 연동은 D2·D3·D7 결정 뒤 진행한다. 새 재현 자료가 오면 해당 NB 행을 다시 연다. |
| 한국어 글자 선명도 | **사용자 지시(2026-11-23): 문서에만 반영하고 지금 작업하지 않는다.** WB E1288 점검에서 찾은 cpppj 후속(전체화면 최근접 비정수 확대로 1px 획 굵기 불균일, 고해상도 글자 층 제안, 큰 슬롯 배율·기준선 차이)은 [0.7](#korean-font-change) 하단에 적었다. 재개는 [D8](#decisions) 결정 뒤 새 단위(1328~)로 한다. |

<a id="decisions"></a>

### 0.2 사용자 결정/정보 대기

별도 지시 전에는 아래 권고 범위를 유지한다.

| ID | 항목 | 현행 기준 / 필요한 결정 |
| --- | --- | --- |
| D1 | CD10.72 범위·CD 정책 | 현재 **보류**. 재개 시 메뉴+튜토리얼 최소 범위부터 권고. 로컬 `CDDir`+표시 파일로 확인하며 CD 삽입 대기·온라인 검증은 복구하지 않는다. |
| D2 | 비행체 표시 보간 | Frame Rate 60/75/120 상한과 양보 대기는 완료. 비행체는 원문50Hz 갱신이며 보간은 선택 사항으로 미착수다. |
| D3 | 일본어 본문·다른 언어 | 일본어 UI는 완료. 미션/캠페인 본문43개·도움말128주제 등은 영어 대체다. 번역 범위·AI 번역 사용·검수 주체를 정한 뒤 진행한다. 독일어 등 추가 언어는 미노출이다. |
| D4 | 알려진 버그12건 | NB28 설계 기준, NB01/06/13/14/19/21/25/26/27/32/33 재현 조건·올바른 동작이 필요하다. [건별 필요한 자료](docs/exe/cpp-known-bugs-status-20261111.md). |
| D5 | NB04·NB29 공식 | 현행 Even 구역 순서 섞기·6명 이후 인당 간헐천6개 증가 공식을 유지한다. 변경 목표가 주어질 때만 조정한다. |
| D6 | 스커미시 시작 섬 | 현행22개 모양 후보·사람 남쪽(소유자1)을 유지한다. 위치/모양 확장은 선택 사항이다. |
| D7 | QuickJS 추가 연동 | 14차 **엔진 탑재만 완료**. 게임 상태·파일·네트워크를 노출하지 않는다. 진단 JS/API/모드/미션 대본으로 넓힐 때 별도 범위를 정한다. |
| D8 | 한국어 글자 선명도(2026-11-23 기록, 작업 보류) | ① 전체화면 확대 방식: 현행 픽셀 중심 최근접 비정수 확대(`ScreenPresentation.cpp` `ScaleScreenPresentation`)를 유지할지, 정수 최근접 뒤 부드러운 축소(webpj `sharp`)나 8차 안티앨리어싱·FSR 경로와 묶을지. ② '고해상도 글자 층'(아래 0.7) 채택 여부와 글꼴(갈무리11 정수배 또는 Noto Sans KR 등 외곽선 글꼴; AGENTS.md 지정은 현재 갈무리11). ③ 한국어 큰 슬롯 배율·굵기를 webpj(제목 2배·보통체)와 맞출지. 결정 전에는 현행 유지. |

<a id="user-checks"></a>

### 0.3 사용자 확인 대기 — AI 실행 제외

| 영역 | 확인할 것 |
| --- | --- |
| 플레이·동일성 | 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리, 전체 화면/음향 동일성. |
| MP·LAN | 실제 LAN/Join/Test Battle·debugBurst·Outpost·희생 안내 UI/패킷, 메타 맵 커서 깊이0 반례, 로비 섬 사제·거주 구역 표시. **MP 실행 검증은0**이다. |
| 한국어 | 물리 IME·클립보드·본문 문장 교정. 좁은 창의 영문/한글 혼합 H2 제목 잘림은 근본 원인 미확정이다. |
| 일본어 | 물리 IME·클립보드·번역/라이선스 저작권 줄·화면 크기별 굵기. 가변 Noto Sans JP 기본 인스턴스는 Thin이다. |
| 알려진 버그 | NB03/05/09/10/11/18/19/22/23의 실제 조작·장시간·MP·교전 확인. 건별 조건은 [현황표](docs/exe/cpp-known-bugs-status-20261111.md)를 따른다. |
| 난이도·스커미시 | AI 속도 체감·게임 중 변경 반영, 금지 유닛의 AI 적용·동맹/전투/승패·유닛 이름·MP 규칙 섬 배치 체감. |
| 화면·설치 | 60/120FPS 소리 동기, 실물4K/FSR·활성 전체화면·다른 모니터/DPI·앱 전환·청취, 실제 Program Files/UAC·CD·Windows11/ARM. |
| 미션 흐름·관전 | 실제 승리 창의 Next Missions 버튼 클릭(1320), 테스트 자동 플레이 관전 카메라의 희생 의식 중 제단 이동·전체 자연 플레이/원본 전략 동일성(1312·1326). |
| 아이콘 | cpppj 배포 ZIP 압축 해제본의 탐색기 아이콘 표시([0.8](#exe-icon)). |

<a id="stages"></a>

### 0.4 AGENTS 단계별 상태

10.78의 구현 마감과 전체 플레이·물리 환경 확인을 구별한다.

| 단계 | 상태 | 잔여 |
| --- | --- | --- |
| 1차 10.78 복원 | 비MP 활성 범위 마감, 최초 상태 교정/검증1314 완료 | 자연 완주·전체 동일성은 사용자 확인. 최신 전체34진입은1327 완료, 검사 기준선은4.2절 |
| 2차 Windows10/11 | 호환 구현 마감 | 다른 모니터/DPI·Windows11/ARM 실기 |
| 3차 LAN·Outpost | 구현·정적 대조 마감 | MP 실행은 사용자 확인 |
| 4차 MCP·와이드 | 구현 마감, 최대1920×1080 | 구체적 누락 제보 시 재개 |
| 5차 사용자 홈·맵 목록 | 구현·읽기 전용 설치 확인 | 실제 Program Files/UAC |
| 6차 한국어 | K1~K5·갈무리11 전환/상수 분리1322 완료 | 사람 검수·물리 입력·다른 DPI, 전체화면 글자 선명도는 D8 |
| 7차 오류·상세 로그 | 완료 | 던진 위치 스택 없음·ARM64 레지스터 미구현 |
| 8차 AA·FSR1.0 | 구현 마감, 기본OFF | 실물4K·성능·물리 출력 |
| 9차 시나리오 편집 | S1~S4 완료 | 없음 |
| 10차 알려진 버그 | 34건 판정 마감 | D4/D5·사용자 확인 |
| 11차 난이도 | 구현 완료, 1312 동맹 AI 판정 보정 | 실제 전투 체감 |
| 12차 스커미시 | UI·MP 규칙 월드 완료 | D6·사용자 확인 |
| 13차 기타 언어 | 일본어 UI 완료 | D3·사람 검수 |
| 14차 QuickJS | v0.18.0 정적 탑재·자체 검사·ARM64 교차 빌드 완료 | D7·ARM64 실기 |
| 15차 설정 초기화 | **1325 완료** | 없음. 집중6/6·실제GUI8경로, 초기화 후 정상 종료·최초 상태 재실행 및 맵/로그 보존 |
| 후순위 | 60/120FPS 상한·대기 완료 | 표시 보간은D2, CD복원은사용자 보류 |

### 0.5 기준값

| 항목 | 현재 기준 |
| --- | --- |
| 제품 | **1326 / 11.01**. Reset All1325와 테스트 선택 AI 본문/피해 문맥1326을 포함한다(1326 관련10/10·실제1-3/1-4 각600초 종료0, 1325 새6/6·GUI8경로). 이전 단위의 기준값(아이콘1323 리소스 대조·글꼴1322 관련47/47·미션 전환1320 119/119)은 [1절](#done) 근거 문서를 따른다. |
| 실행·플레이 | 10.78 메뉴·미션·스커미시 플레이 가능. CD는 메뉴만 확인했고 튜토리얼/플레이 미완료다. 전체 원본 동일성은 미확정이다. |
| ZIP·전체 검사 | **1327 M8 통과**(제품1326). ZIP1,437항목·배포38고유경로(일반31+데모3+언어Credits3+스커미시1)·QuickJS6/6·배포1,437/보호2,808불변. 최신 전체34진입은1327이며 전체 솔루션 전수 통과로 확대하지 않는다. 최신 선정752는751통과·NB08 검사1실패 뒤 해당1건 교정 통과. [1327](docs/exe/cpp-m8-final-gate-1327-20261122.md). |
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
| webpj | `fonts/Galmuri11.woff2`·`fonts/Galmuri11-Bold.woff2`를 `webpj/www/data/fonts/`로 복사해 FontFace에 등록한다. 메뉴·도움말과 Tutorial1 실제 미션 브리핑에서 확인했다. 굵기는 사용자 결정으로 보통체를 유지한다. **E1288:** WOFF2 설계 격자(12px)로 그리고 4배 그리기·칸 평균으로 Canvas 번짐을 없앴으며, cpppj처럼 숫자·영문·공백을 원본 `.chfnt` 글리프로 그린다(SP 숫자·Tutorial3 D1 그림 줄 해소). [WB 인계](LEFT_JOBS.webpj.md#korean-font-change) · [E1288](docs/webpj-korean-glyph-grid-20261123.md) |
| 라이선스 | `fonts/Galmuri11-LICENSE.txt`를 각 배포판에 글꼴과 함께 포함한다. |

**WB E1288 점검의 cpppj 해당 여부(2026-11-23, 정적 대조·기존 화면 캡처, cpppj 실행 없음).** 사용자 지시로 기록만 하고 작업하지 않는다([D8](#decisions)). webpj에서 고친 세 결함(11px 축소로 획 빠짐·SP 숫자 확대·Tutorial3 D1 그림 줄바꿈)은 cpppj에 없다: strike 비트맵을 픽셀 그대로 정수배 복제하고(`EmbeddedBitmapFont`, 한글 칸 전진 폭12·기준선 위12줄), Windows-1252 글자는 `EncodeBytes`→`.chfnt`로 그린다(`docs/screens/cpp-korean-galmuri-1322-20261117/briefing.png` 한글 11줄 온전). [근거](docs/webpj-korean-glyph-grid-20261123.md)

- [ ] **전체화면 1px 획 불균일(D8 ①).** 1024×768을 1080p 등으로 픽셀 중심 최근접 비정수 확대(1.40625배, `ScreenPresentation.cpp:50` `ScaleScreenPresentation`)해 갈무리11의 1px 획·간격이 1~2px로 들쑥날쑥하다. 창 모드 1:1은 해당 없음. 완료 기준: D8에서 정한 방식으로 1080p 전체화면의 획 굵기가 고르고 기존 화면 대조 시험이 통과한다.
- [ ] **고해상도 글자 층(D8 ②, 제안).** `GumpTextDraw` 글리프 출력 훅에서 위치·코드·슬롯·색·잘림 영역과 그리기 전후 표면 픽셀을 기록하고, 표시 단계(`StretchDIBits`/`ScaleScreenPresentation`)에서 가려지지 않은 글자만 배경을 되돌린 뒤 GDI/DirectWrite로 출력 해상도 글자를 덧그린다. 논리 화면은 바뀌지 않아 대조 시험에 영향이 없고 8차 AA 옵션과 묶는 것이 자연스럽다. 완료 기준: 옵션 ON에서 한국어가 출력 해상도로 그려지고 OFF는 현행 화면과 같다.
- [ ] **webpj와의 차이(D8 ③).** 큰 슬롯 배율(cpppj strike ppem16 → 슬롯3 1배·슬롯4 3배, webpj 2배·4배)·굵기(cpppj 굵기700 슬롯은 굵은 갈무리, webpj 보통체)·한글 기준선(cpppj 칸 기준12줄이라 슬롯0에서 원본 기준선보다 1줄 아래, webpj 잉크 기준11줄). 완료 기준: 결정된 쪽으로 맞추거나 차이 유지 결정을 기록한다.

<a id="exe-icon"></a>

### 0.8 실행 파일 아이콘·webpj favicon — cpppj1323·DN0.807·WB E894 적용, 시각 확인 남음

사용자 커밋 `d047bf9a3`(cpppj·dotnetpj `Netstorm.exe`)과 `11b16b778`(webpj favicon)의 요구다. 기준은 원본 `originals/Netstorm.exe`(SHA-256 `a305414cfe0b9449…`)의 RT_GROUP_ICON **102(0x66)** 32×32·4bpp(탐색기 파일 아이콘)와 **144(0x90)** 16×16·4bpp, 언어1033이다. 원본 `R.exe`에는 아이콘이 없다. 두 이미지를 공통 [assets/Netstorm.ico](assets/Netstorm.ico)에 담았다. [추출 도구·SHA-256](docs/exe/cpp-exe-icon-1323-20261119.md)

| 대상 | 상태 |
| --- | --- |
| cpppj | `cpppj/src/app/ProductVersion.rc`에 102·144/1033을 지정하고, 창 아이콘은 `ClientMain.cpp`에서 게임 폴더의 원본 `Netstorm.exe`를 자료로 열어 같은 ID를 읽는다. Release PE 바이트 대조·Windows 추출 API·배포 `-DryRun` 통과(1323). |
| dotnetpj | 0.807에서 `Netstorm.Game`/`Netstorm.exe`의 `ApplicationIcon`으로 공통 ICO를 지정하고 EXE·ZIP의 두 원시 아이콘 바이트 일치를 확인했다. DesktopGL 창·작업 표시줄 확인이 남았다. [DN 인계](LEFT_JOBS.dotnetpj.md#exe-icon) |
| webpj | E894에서 `webpj/www/favicon.ico`와 상대 경로 링크를 넣고 바이트 대조·Chromium HTTP 200·해독을 확인했다. 탭 그림의 시각 확인이 남았다. [WB 인계](LEFT_JOBS.webpj.md#favicon) |

- [ ] **사용자 확인.** cpppj 배포 ZIP 압축 해제본의 탐색기 아이콘 표시를 확인한다(0.3).

<a id="done"></a>

## 1. 완료 묶음과 근거

이미 마감한 구현을 다시 만들지 않는다. 1314~1327의 단위별 상태 설명은 [1327 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327), 그 이전 단위 이력·주소 목록은 [1310 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310)에 있다.

| 묶음 | 요약·근거 |
| --- | --- |
| 최신 M8 관문1327 | 제품1326 Release·ZIP1,437전수·34/34진입+Credits/스커미시4경로·QuickJS6/6·정적 감사·배포/보호 불변. [보고](docs/exe/cpp-m8-final-gate-1327-20261122.md). 이전 관문: [1311](docs/exe/cpp-m8-final-gate-1310-20261114.md)·[요약JSON](cpppj/m8-final-gate-20261114.json)·[1294~1297](docs/exe/cpp-m8-final-gate-1294-20261111.md)([3.1절](#final-gate)). |
| 검사 계약 교정 1304~1305·1316·1318·1319·1321·1324 | 제품·fixture 불변, 옛 기대값·검사 장치만 현행 계약으로 교정했다. 최신 통과 수치는 [4.2절](#baselines). 근거: [1324](docs/exe/cpp-remaining-test-contracts-1324-20261122.md)·[1321](docs/exe/cpp-pop-test-baseline-1321-20261117.md)([선정 목록/결과](cpppj/pop-test-baseline-1321-20261117.json))·[1319](docs/exe/cpp-world-test-contracts-1319-20261117.md)·[1318](docs/exe/cpp-startup-name-test-1318-20261117.md)·[1316](docs/exe/cpp-mission-parent-tests-1316-20261117.md)·[1304~1305](docs/exe/cpp-test-local-transport-accept-20261112.md). |
| AI 본문·피해 진단1326 | 일반 덤프 보존/테스트 선택 AI 관측, 미연결 피해 문맥, 관련10/10·1-3/1-4 각600초 종료0·진단0. 원문 기한 대기/미재현 한계는 [1326](docs/exe/cpp-autoplay-ai-diagnostics-1326-20261122.md). |
| 설정 초기화1325 | 실제 HTML 확인 제출 토큰만 소비, 취소/ESC/직접 Tell 보존, 사용자 설정/진행 초기화·정상 종료와 최초 상태 재시작. 집중6/6·고유GUI8경로. [근거](docs/exe/cpp-reset-all-1325-20261122.md). |
| 실행 파일 아이콘 1323 | 원본 32×32·16×16 리소스 탑재와 공통 ICO 저장. Release PE 리소스 바이트 대조 및 Windows API 로드 검증. [근거](docs/exe/cpp-exe-icon-1323-20261119.md). |
| 한국어 갈무리11 1322 | 외부 일반/굵은 비트맵 원형·같은 일반체 대체·공통 JSON/상수·배포 전환. 관련47/47·실제GUI5회 종료0·ZIP1,437항목/보호2,807파일 불변. [근거/화면](docs/exe/cpp-korean-galmuri-1322-20261117.md). |
| 미션 전환1320 | 이전 Source의 실제 flush·종료 수신 전달, 등록표 번호 재사용/호출 수명 교정. 새4/4·전체119/119, 실제 Replay2회·한국어/영어 양방향 전환·오류 로그0. 실제 승리/MP 실행은 제외. [근거](docs/exe/cpp-mission-transition-1320-20261117.md). |
| analyzeManager 최초 준비1317 | 옵션 파일이 없는 정상 입력 지원, 사본에만 관찰용 네 키 생성·원본/setup 보존. 단위114통과·MCP 기본 검사 통과. [근거](docs/exe/analyze-manager-first-run-1317-20261117.md). |
| 한국어 유닛 정보1315 | `Type` 내부 접미사 오역 교정, 실제 우클릭 정보/F1·영어 대조, 정보 앵커76개·관련40검사 통과. [원인·검증](docs/exe/cpp-korean-unit-help-1315-20261117.md). |
| 최초 실행1314 | 원본 PE 기본 음량5·5, Client/메뉴/보고서의 같은 설정 해석, 사용자 음량·안내/진행 저장 보존. 집중48건·제품GUI13회·데이터 불변 확인. 넓은 미션 진입 검사의 별도 실패는4.2절. [구현·검증·제품 화면](docs/exe/cpp-first-run-1314-20261116.md). |
| 비MP 복원·호환 | [UI](docs/exe/cpp-ui-implementation-closure-20261105.md)·[Edit](docs/exe/cpp-editor-implementation-closure-20261106.md)·[미션](docs/exe/cpp-mission-state-scope-20261106.md)·[수명/FILE](docs/exe/cpp-internal-lifetime-closure-20261106.md). |
| LAN·Native·M7 | M6-a~n 정적 종결, 장기 소유자·희생 안내·오류 로그 연결. [M6 종결](docs/exe/cpp-m6n-reachability-closure-20261111.md)·[오류 로그](docs/exe/cpp-error-log-20261111.md). |
| 한국어·시나리오 편집 | K1~K5·S1~S4 완료. [한국어](docs/exe/cpp-korean-k4-k5-closure-20261111.md)·[읽기 전용 설치/시나리오](docs/exe/cpp-scenario-s4-readonly-home-20261111.md). |
| 언어·제품 표기 | 일본어 UI509키·외부 글꼴·영어 대체, 11.01 Patch Credits/HJOW·AI·Copyright2026 HJOW. [일본어](docs/exe/cpp-japanese-l1-l3-20261111.md)·[제품](docs/exe/cpp-product-1101-20261101.md). |
| 난이도·스커미시·프레임 | [난이도](docs/exe/cpp-difficulty-d-20261111.md)·[MP 규칙 스커미시](docs/exe/cpp-skirmish-sk-20261111.md)·[60/120FPS](docs/exe/cpp-post-plan-fps-cd1072-20261111.md). |
| QuickJS | 탑재·자원 한도·자체 검사·배포 라이선스. [1301](docs/exe/cpp-quickjs-engine-20261111.md). |
| 테스트 자동 플레이1312 | 캠페인·스커미시 아군 AI 자동 플레이·아군/적 난이도·시작 SP·관전 카메라(명령줄 전용)와 실행 도구, 11차 동맹 AI 판정 보정(`o/AiAllianceSide.h`). 1-1 승리 재현·스커미시 확인. [AI 사용 안내](docs/testing/cpp-autoplay-test-guide.md)·[근거](docs/exe/cpp-campaign-test-play-20261114.md). |
| CD1306~1310 | 기본 해상도·메시지·CDDir·Player/메뉴·순수 Start/AI 설정. **후속 보류.** [메뉴/Player](docs/exe/cpp-cd1072-player-initialization-20261114.md)·[Start/AI](docs/exe/cpp-cd1072-mission-start-ai-settings-20261114.md)·[보류 인계](docs/exe/cpp-cd1072-deferred-20261114.md). |

<a id="bugs"></a>

### 1.1 알려진 버그

판정34건: 수정10·기반영1·미재현4·해당없음1·D4 정보 필요12·MP 미수정6이다. [NB01~34 현황표](docs/exe/cpp-known-bugs-status-20261111.md)가 근거·재현 조건의 기준이다. NB04/NB29 공식은D5, 새로운 제보는 해당 행을 다시 연다.

<a id="lang"></a>

## 2. 기타 언어

일본어 UI 이후 본문 번역·추가 언어 노출은 D3 대기다. 글꼴·물리 입력 검수는0.3에 남긴다.

<a id="final-gate"></a>

## 3. M8 최종 관문 — 1327 완료

제품1326의 [최종 보고](docs/exe/cpp-m8-final-gate-1327-20261122.md)·[기계 판독 결과](cpppj/m8-final-gate-1327-20261122.json)가 현재 기준이다. Release 종료0·ZIP1,437항목 전수·일반31/데모3 전체진입·3언어Credits/스커미시4경로·QuickJS6/6·아이콘 원본4payload 일치·외부 정적 감사·배포1,437/보호2,808파일 불변을 확인했다. 최신 선정752는751통과·NB08의 옛 경로 기대1실패 뒤 해당1건만 교정해 통과했으며 전체를 반복하지 않았다.

ZIP: `extracted/cpp-m8-1327/NetstormReborn-11.01-cpppj.zip`(205,883,413바이트), SHA-256 `b8d7f37d9b649db46760ae02cea4140e36bc1954d6aa5b166433a85abaeb5a05`.

현재 자동 진행 가능한 잔여는 없다. 사용자 직접 확인·CD/선택 보류·전체 솔루션/원본 전체 동일성 미판정은 유지한다.

### 3.1 이전 관문1311 — 제품1310 기준

제품1310의 Release·ZIP1,436항목·배포5경로 관문이며 제품 코드는 바뀌지 않았다. 결과 표와 ZIP 해시(`extracted/cpp-m8-1310/NetstormReborn-11.01-cpppj.zip`)는 [최종 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md)·[기계 판독 결과](cpppj/m8-final-gate-20261114.json)와 [1327 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327)의 3.1절에 있다. 그 뒤 요청된 1314~1326 작업과 최신 관문1327은 모두 완료했다. 더 이전 결과는 [1294~1297](docs/exe/cpp-m8-final-gate-1294-20261111.md)에 있다.

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
| menu_session·main_popup·file(1324) | 최초 menu4·file5 실패 → 교정 후 최신 menu194/194·popup79/79·file390 중389(NB08 1실패 → 교정 후 통과, 1327). 과거 전체 실패수와 구별한다. |
| 실제 미션 하네스 | 옛 AGENTS 해시 때문에 passed:false 기록이 있다. 실행/종료 결과와 보호 해시를 따로 확인한다. |

변경 전과 같은 실패인지 실제 기준선으로 비교한다. 과거 세부 로그·수치는 보관본4.2에 있다.

<a id="rules-verify"></a>

## 5. 검증 제외·남은 한계

- 사제 포획/희생·특정 적 공격·시나리오 승리·전체 자연 완주·사용자 직접 확인 검사는 건너뛴다. 단순 비MP 진입/대기/생산/건설 검사는 수행한다.
- 예외(2026-11-21 사용자 요청, 1312 방식): **캠페인1-1~1-5·스커미시에 한해서** 테스트 전용 자동 플레이(`--test-*`, [AI 사용 안내](docs/testing/cpp-autoplay-test-guide.md))로 아군 AI가 캠페인/스커미시를 진행·승리하는지 확인하는 실행은 허용된다. 이는 사람 플레이·원본 동일성 검증을 대신하지 않으며 결과는 "테스트 자동 플레이"로 구분해 기록한다.
- 새 MP 단위/패킷/게임/통신/기록파일 실행 검사는 하지 않는다. MP 구현·컴파일·정적 대조는 가능하다. 공중3종(훨리기그·맨오워·더스트 데빌) 전용 생성/생산/행동 관측도 실행 제외다.
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
9. 현재 문서의 지시가 옛 보고서의 "다음"보다 우선한다. 자동 진행 범위는2026-11-22의1327에서 마감했다. CD 재개와 선택 추가 작업은 별도 사용자 지시를 기다린다.
10. **LEFT_JOBS 정리 요청 시**(AGENTS.md `1fc63cb1a`): 정리 직전 원문 전체를 [LEFT_JOBS.old.md](LEFT_JOBS.old.md) 끝에 앵커·SHA-256과 함께 보관한 뒤 만료·상세 불필요 내용은 요약만 남기고, 완료됐는데 미표시인 항목을 점검하며, 너무 큰 작업 단위는 완료 기준이 있는 세부 단위로 나눈다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 7. 근거·보관

[2차 정리 전 보관본](LEFT_JOBS.old.md#archive-20261123-second-through-1327)에는 2차 정리 직전 문서 전체(0.7의 WB E1288 점검 세부 문장, 1절의 검사 교정 단위별 6행·M8 1311 행)를 원문 바이트로 보존했다. [1327 전체 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327)에는 2026-11-23 정리 직전 문서 전체를 원문 바이트로 보존했다. 0.1의 1314~1327 단위별 상태 행, 0.6~0.8의 완료 세부(최초 실행 관찰·글꼴 로딩/검증 체크·아이콘 리소스 표), 3.1 이전 관문1311 표, 머리의 2026-11-17 원격 병합·2026-11-19 아이콘 요구 안내는 이쪽에서 찾는다. 단위1302~1310의 경과·끝난 작업 상세·AGENTS 변경 이력표·낡은 다음 지시는 [1310 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310)에 있다. 기존 보관본은 수정하지 않았다.

**2026-11-23 정리 기록.** 바로잡은 것: 0.6의 WB 최초 실행 적용(E616 완료, DN만 남음), 0.7의 WB 갈무리11(적용·확인 완료, DN 화면 적용만 남음), 0.8의 webpj favicon(E894에서 `favicon.ico`·링크 추가, 탭 시각 확인만 남음). 흩어져 있던 1320 승리 창 버튼·1312 관전 카메라·탐색기 아이콘 확인은 0.3으로 모았다. cpppj 소스·제품·디컴파일 기준은 그대로다(마지막 `cpppj/src` 변경 `baf141ebe`=1326, AGENTS.md 마지막 변경은 당시 `11b16b778`).

**2026-11-23 2차 정리 기록(AGENTS.md `1fc63cb1a` 정리 규칙 적용).** 1절 검사 교정 6행·M8 1311 행을 합치고 0.7 E1288 점검·4.2 기준선 문장을 요약했다(원문은 2차 보관본). D8 세 항목에 완료 기준을 달았다. 완료 미표시는 없었다(DN 최초 실행·한국어 화면·창 아이콘, WB 탭 아이콘은 각 인계 기준으로 남음; `cpppj/src`는 1326 이후 변경 없음). AGENTS.md 마지막 변경은 `1fc63cb1a`(정리 규칙)이고 한국어 글꼴 지정은 갈무리11 그대로다(`96d81381c`의 Noto Sans KR은 참고 자료).

주요 기준: [SOURCE_MAP](cpppj/SOURCE_MAP.md)·[디컴파일 신뢰도](docs/exe/decompile-reliability.md)·[제품](docs/exe/cpp-product-1101-20261101.md)·[미션 대응](docs/gameplay/demo-campaign-map.md)·[MCP/session dump](docs/exe/cpp-session-state-dump-20261109.md). DN/WB 요청·상태는 각 인계 문서를 따른다. AGENTS.md는 수정하지 않으며 현행1~15차·DN/WB4~15차 범위는 각 프로젝트에서 유지한다.
