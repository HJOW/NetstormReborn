# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**제품 1367 / 11.01 · 3차 정리 2026-11-24(HJOW-Athlon) · M8 관문1327 · 마지막 단위1367(2026-11-26) · 다음1368(LAN 시계/상태17·18) · 마지막 디컴파일 HJOW-Athlon · 일본어43/43·128주제·최종 관문1362 완료, CD/QuickJS/보간 보류 유지**

현재 상태·남은 작업·유효한 규칙만 유지한다. 2026-11-24 정리 직전 원문(0.1의 1328~1339 단위별 경과·추가 요구 세부·D3 경과)은 [1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339), 2차 정리 직전 원문(D8·WB E1288 점검 세부, 1절 단위별 검사 교정 행 포함)은 [2차 정리 전 보관본](LEFT_JOBS.old.md#archive-20261123-second-through-1327), 1314~1327의 단위별 상태 설명과 끝난 세부는 [1327 시점 전체 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327), 그 이전 경과와 만료된 지시는 [1310 시점 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310), 개별 구현·검증은 아래 근거 문서를 따른다. [dotnetpj](LEFT_JOBS.dotnetpj.md)·[webpj](LEFT_JOBS.webpj.md)는 각 인계 문서가 기준이다.

<a id="now"></a>

## 0. 현재 작업판

### 0.1 다음 행동

**현재 위치(2026-11-26).** 기존 자동 진행 범위는 M8 관문1327에서 마감했다. 2026-11-23 goal로 받은 추가 요구1328~1330과 후속 검사 교정1331·소규모 배포 확인1332를 끝냈고, 후속 사용자 답변(2026-11-23)에 따라 **D3 일본어 본문 번역을1362최종관문까지완료했다**(UI1333·본문43/43·help128주제·배포확인). 사용자 webpj 후속 지시로 끼운1339(원본 주문 생성/운반물 비트 대조·NB18 교정, 같은 시전 골렘의 사제 포획/Drop 복구)도 완료했다. 1340~1341에서 사제훈련bc4~9와 그림21개를 확인했고 1342~1343에서 캠페인1 여섯 미션을 추가했다(공식30/43). 1344~1345 캠페인2 다섯 미션·데모로36/43까지 완료했고 일본어 데모의 기본 AI 이름 필드 초과도 수정했다. 2026-11-26 사용자 goal에 따라 활성 cpppj 잔여를 계속 진행하며, TCP/IP 로컬 상대 봇 개발·실행 검증은 옛 MP 실행 제외보다 우선한다. CD10.72·QuickJS 추가 연동·프레임 보간과 명시 사용자 확인 검사는 보류한다. 단위별 결과는 [1절](#done)과 각 근거 문서에 있다.

1. **1362 일본어 최종 관문 완료.** 공식43본문·help128주제·언어52/채팅6검사·1480항목ZIP·압축해제GUI2회와원천불변확인까지마쳤다. 도움말1363도완료했으며다음1364는TCP/IP상대봇이다. [근거](docs/exe/cpp-japanese-final-gate-1362-20261126.md).
2. **1363 도움말 플래그 완료.** 원본00464ed0/00464f80의LOOK1000·HELP200·빈값/첫NUL해제를공통setter에반영했다. 메뉴169/언어52검사와영어세좌표·한국어·일본어예산About/생산창About를확인했다. 오래된Battle저장검사는실제파일/CoreData/기존섹션보존검사로교정했다. Release EXE는`154ea7f8…9f7e405`, 기존ZIP1362는이수정전이다. [근거](docs/exe/cpp-help-flags-1363-20261126.md).
   - [x] 원본10.78과같은도움말플래그쓰기·빈문자열해제·문자열소유권과실제숫자칸/생산창우클릭을1363에서검증했다. 마지막디컴파일호스트HJOW-Athlon.
3. **다음1368 — LAN 시계/상태17·18.**1367에서일반전투의객체Chaff/Client공유후처리3모드를맞춰Fort범위오류를해결했다. 고유등록ID의실제두참가자9차가로그인14/PlayerKnown·Ready·시작창소멸·짧은대기/두종료0을통과했다(같은Player명단/ID/SP6500/7660객체). 별도8차의간헐`Archer 현재 clock/경과 오류`를시계동기화/delta와대조한다. 후속17/18은원문4b7cd5/4b7cf3의accepted/denied로그·즉시14/출처0+next3이며아직미구현이다. 생산건설/개별객체·SP변경동기화/긴대기·재접속·최종ZIP도남았다. [1367근거](docs/exe/cpp-lan-fort-modes-1367-20261126.md)·[봇안내](docs/testing/cpp-lan-opponent-guide.md). 새MP오류/확정수정은사용자지시에따라[DN후속](docs/dotnet-cpp-lan-followup-20261126.md)과DN인계에도계속반영한다. CD/선택보류·명시사용자확인검사는유지한다.

| 구분 | 상태와 다음 행동 |
| --- | --- |
| 최신 기준 | 제품**1367**(Release 성공; 단위1328~1339는 관문 밖 집중 검사·실제 GUI로 확인)·**최신 M8 관문1327**(제품1326)·추가 배포 확인1332. 완료 묶음과 근거는 [1절](#done), 관문 수치는 [3절](#final-gate), 검사 기준선은 [4.2절](#baselines)이다. |
| 사용자 확인·결정 | [0.3](#user-checks)의 플레이·MP·물리 환경 확인은 AI 실행 범위 밖이다. [0.2](#decisions) D1~D7은 결정 전까지 권고 범위를 유지한다. |
| **CD10.72 복원 C3~C5** | **사용자 보류(2026-11-14). 재개 지시 전 진행하지 않는다.** 메뉴·CDDir 저장 후 재실행은1309에서 확인했다. 1310의 Start/AI 설정은 순수 몸체만 검증했고 실제 Session/Guide/SP/AI 생성자에는 미연결이다. 첫 튜토리얼 차단은 `RawMenuHtml.inc:3`의 PATCH 전용 HTML 검사다. [보류 인계·재개 지점](docs/exe/cpp-cd1072-deferred-20261114.md). |
| AI 자동 플레이 정체 | 1326으로 자동 범위 완료(초기화/프레임 미연결 아님). 1-3 후반 재시도 대기·1-4 돈 정체 미재현·3-1 옛 오류의 판정은 [1326](docs/exe/cpp-autoplay-ai-diagnostics-1326-20261122.md)을 따른다. 임의 전략 변경 없이 사용자 자연 플레이/동일성 확인을 유지한다. |
| 테스트 자동 플레이 | **플레이 결과 재현이 필요한 AI는 먼저 [AI 에이전트용 사용 안내](docs/testing/cpp-autoplay-test-guide.md)를 읽고 [실행 도구](tools/cpp_campaign_autoplay.py)를 쓴다.** 명령줄 전용 `--test-autoplay`·`--test-ally-ai`·`--test-enemy-ai`·`--test-start-sp`·`--test-enemy-start-sp`·`--test-follow-battle`이며 허용 범위는 [5절](#rules-verify)이다. 1313 일괄 실행 결과는 [결과 7절](docs/exe/cpp-campaign-test-play-20261114.md#7-캠페인-전체-자동-플레이-실행-1313-2026-11-14-후속). |
| DN/WB 후속 | 최초 실행은 DN만 남음([0.6](#first-run)), 한국어 글꼴 화면 적용은 DN만 남음([0.7](#korean-font-change)), 아이콘은 DN 창 표시·WB 탭 시각 확인만 남음([0.8](#exe-icon)). 추가 요구1328~1330은 WB가 E1289~E1291로 적용했고(버전 창 일본어 줄은 웹 13차로 미룸) DN은 해당 소비자를 연결할 때 따른다. 1335의 언어 무관 마나 그림 교정은 WB 재반영 판정·DN PORT_MAP 재대조 대상이다. 각 인계 문서에서 진행하며 CP 단위에서는 DN/WB 코드·인계를 수정하지 않는다. |
| 선택/추가 제보 | 비행체 표시 보간·QuickJS 추가 연동은 D2·D7 보류를 유지한다. D3 일본어는1362최종관문까지완료했다([2절](#lang)). 새 재현 자료가 오면 해당 NB 행을 다시 연다. |
| 한국어 글자 선명도 | **사용자 지시(2026-11-23): 문서에만 반영하고 지금 작업하지 않는다.** cpppj 후속 세 항목(전체화면 비정수 확대의 1px 획 불균일·고해상도 글자 층·webpj와의 배율/굵기/기준선 차이)은 [0.7](#korean-font-change) 체크에 완료 기준과 함께 있다. 재개는 [D8](#decisions) 결정 뒤 새 단위로 한다. |

<a id="user-requests-20261123"></a>

**사용자 추가 요구(2026-11-23 goal, 단위1328~1330 — cpppj 완료).** 세 프로젝트 공통 요구는 cpppj가 규칙을 먼저 정하고 webpj·dotnetpj가 따른다([WB](LEFT_JOBS.webpj.md#next)·[DN](LEFT_JOBS.dotnetpj.md#now)). 요구 원문과 단위별 세부는 [1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339)에 있다.

- [x] **방향키 카메라 이동(1328).** 게임 중 ↑↓←→로 카메라를 32논리픽셀/누름·Windows 반복으로 옮긴다(원본에 없는 추가 요구, 가장자리·Alt·가운데 버튼·미니맵·화면 위치 키와 별도). TextEntry·채팅 커서 이동(`TextEntryGump.cpp:194~199`)과 조각을 든 상태의 좌우 회전(`RawCampaignCursor.cpp:179~180`, 0x250000/0x270000)이 우선이다. DN/WB가 따를 계약: [근거](docs/exe/cpp-arrow-camera-1328-20261123.md).
- [x] **ESC 게임 메뉴 한국어 점검(1329).** Game·View·Options·Players·About와 하위를 실제 GUI로 점검해 누락23키·프레임 속도 번역 경계를 고쳤다. webpj에서 제보된 Players 글자 깨짐은 cpppj에서 재현되지 않았다. [근거](docs/exe/cpp-esc-menu-korean-1329-20261123.md).
- [x] **정보(About) ▸ 버전 창 Copyright(1330).** 원본 Tell `[About]`의 원작/10.78 Ticonderoga 표기를 보존하고 `11.01 … HJOW`와 `Copyright 2026 HJOW`를 세 언어 창에 추가했다. [근거](docs/exe/cpp-about-copyright-1330-20261123.md).

<a id="decisions"></a>

### 0.2 사용자 결정/정보 대기

별도 지시 전에는 아래 권고 범위를 유지한다.

| ID | 항목 | 현행 기준 / 필요한 결정 |
| --- | --- | --- |
| D1 | CD10.72 범위·CD 정책 | 현재 **보류**. 재개 시 메뉴+튜토리얼 최소 범위부터 권고. 로컬 `CDDir`+표시 파일로 확인하며 CD 삽입 대기·온라인 검증은 복구하지 않는다. |
| D2 | 비행체 표시 보간 | Frame Rate 60/75/120 상한과 양보 대기는 완료. 비행체는 원문50Hz 갱신이며 보간은 선택 사항으로 미착수다. |
| D3 | 일본어 본문·다른 언어 | **1362 AI 작업 완료.** UI536키·공식43본문·help128주제·그림/용어·언어52/채팅6검사·1480항목ZIP/실제GUI를확인했다. 사람문장/라이선스·물리입력검수는0.3에남긴다. 독일어등다른언어미노출은유지한다. [최종근거](docs/exe/cpp-japanese-final-gate-1362-20261126.md). |
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
| MP·LAN | 실제 LAN/Join/Test Battle·debugBurst·Outpost·희생 안내 UI/패킷, 메타 맵 커서 깊이0 반례, 로비 섬 사제·거주 구역 표시. **1364에서TCP대기실명단/두종료0을확인했다. 1365안내연결뒤State9/10/중앙섬/Fort전송을확인했고1367Fort모드교정뒤두참가자시작/대기/종료0을확인했다. 간헐시계/17·18/생산건설동기화는계속한다.** 2026-11-26사용자goal의TCP/IP상대봇검증이AI작업범위다. |
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
| 3차 LAN·Outpost | 구현·정적 대조 마감 | 상대봇/대기실1364완료, 안내1365완료, Source/수신1366완료, 두참가자시작1367완료, 시계/17·18·생산건설/동기화잔여, 실물 LAN은 사용자 확인 |
| 4차 MCP·와이드 | 구현 마감, 최대1920×1080 | 구체적 누락 제보 시 재개 |
| 5차 사용자 홈·맵 목록 | 구현·읽기 전용 설치 확인 | 실제 Program Files/UAC |
| 6차 한국어 | K1~K5·갈무리11 전환/상수 분리1322 완료 | 사람 검수·물리 입력·다른 DPI, 전체화면 글자 선명도는 D8 |
| 7차 오류·상세 로그 | 완료 | 던진 위치 스택 없음·ARM64 레지스터 미구현 |
| 8차 AA·FSR1.0 | 구현 마감, 기본OFF | 실물4K·성능·물리 출력 |
| 9차 시나리오 편집 | S1~S4 완료 | 없음 |
| 10차 알려진 버그 | 34건 판정 마감, NB18 운반물 방어 교정1339 | D4/D5·사용자 확인 |
| 11차 난이도 | 구현 완료, 1312 동맹 AI 판정 보정 | 실제 전투 체감 |
| 12차 스커미시 | UI·MP 규칙 월드 완료 | D6·사용자 확인 |
| 13차 기타 언어 | 일본어 UI·본문43/43·help128주제·최종관문1362완료 | 사람문장/라이선스검수·물리입력 |
| 14차 QuickJS | v0.18.0 정적 탑재·자체 검사·ARM64 교차 빌드 완료 | D7·ARM64 실기 |
| 15차 설정 초기화 | **1325 완료** | 없음. 집중6/6·실제GUI8경로, 초기화 후 정상 종료·최초 상태 재실행 및 맵/로그 보존 |
| 후순위 | 60/120FPS 상한·대기 완료 | 표시 보간은D2, CD복원은사용자 보류 |

### 0.5 기준값

| 항목 | 현재 기준 |
| --- | --- |
| 제품 | **1367 / 11.01**. M8 관문1327(제품1326) 뒤에 방향키1328·한국어 ESC 보완1329·버전 창 저작권1330, 일본어 UI 보완1333과 본문18개(1334~1338, 안내 마나 그림 보완1335 포함), 원본 주문/운반물 교정1339(Release 성공, GUI/전체/ZIP 미실행)를 더했다. 1340~1341에서 사제훈련bc4~9 본문을 추가했다(실제GUI6회·그림21개·일본어9/9·24/43 구조 감사). 1342~1343에서 캠페인1 여섯 미션과 원본 영어 독립 감사30파일을 확인했다(실제GUI6회·30/43, 1-5 @20 편지 자연 발생). 1344 캠페인2 첫3개(실제GUI3회)로33/43이다. 1345 캠페인2 후반/데모 번역과 기본 AI 이름 경계 교정·Release·관련57/57·수정 후 데모3회도 완료했다(36/43). 1346 캠페인3 3-1~3-3 번역·실제GUI3회·39/43 독립 감사도 완료했다. 1347 캠페인3 3-4~3-5 번역·실제GUI2회·41/43 독립 감사도 완료했다. 1348 Tell 첫 구간은 부분 번역으로 별도 집계했다(전체 완료41/43 유지·9/9·GUI3회). 1349 Tell 두 번째 구간도 부분 번역으로 완료했다(고유125줄·그림2개·9/9·GUI2회, 전체41/43 유지). 1350 Tell 크레딧138줄·9/9·실제19페이지 버튼 전환도 완료했다(전체41/43 유지). 1351 Tell 신기술/팁86줄·9/9·실제40팁 버튼/순번과 동적 그림 확인도 완료했다(전체41/43 유지). 1352 Tell 마지막196줄·8창 표시와 전체42파일 독립 감사를 완료했다(42/43·help만 미완료). 1353 help 첫16고유 주제·전체128주제 파서·일본어10/10·대표 그림/스크롤/내부 링크도 확인했다. 1362일본어43/43·128주제·용어/배포최종관문도완료했다(언어52/채팅6·ZIP1480·GUI2회). 1363도움말플래그·메뉴169/언어52·실제GUI4회도완료했다. 1364옵션수명/스타일·상대봇·메뉴170/170·TCP대기실도완료했다. 1365일반LAN안내두콜백·State9/10/중앙섬/Fort전송·기존1-1진입도확인했다. 1366Source반납/수신6콜백/채널해제·집중37/37도통과했다. 1367일반Fort모드교정·실제두참가자시작/짧은대기/두종료0도확인했다. 단위별 검사 수치는 [1절](#done) 근거 문서를 따른다. |
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
| LAN Fort 모드1367 | 객체Chaff·공유special/fort정리, 실제두참가자14/Ready/시작창소멸·같은명단/월드7660객체/두종료0. 간헐시계·17/18·동기화는잔여. [근거](docs/exe/cpp-lan-fort-modes-1367-20261126.md). |
| LAN Source/수신1366 | 인계하지않은Fort두끝반납·28수신필드/누락6연결·444ff0채널해제·연결30/채팅7통과. 실제Fort전송진행·객체/상태17은1367잔여. [근거](docs/exe/cpp-lan-source-owner-1366-20261126.md). |
| LAN 로그인 안내1365 | Client장기State7/즉시8 progress·공통72h중앙섬안내연결, 실제State9/10/중앙섬/Fort전송·기존1-1종료0. 완전로그인미완료·1366계속. [근거](docs/exe/cpp-lan-login-ui-1365-20261126.md). |
| TCP/IP 상대 봇/대기실1364 | 원문옵션Push/Move·스타일교정·메뉴170/170·실제두명명단/정상종료. 전투State7/8의안내미연결은1365잔여. [근거](docs/exe/cpp-lan-opponent-lobby-1364-20261126.md). |
| 도움말 플래그1363 | LOOK1000/HELP200·빈값/첫NUL·소유권, 메뉴169/언어52·실제GUI4회·보호3212불변. 기존Battle저장검사교정. [근거](docs/exe/cpp-help-flags-1363-20261126.md). |
| 일본어 최종관문1362 | 공식43/43·128주제·용어99곳·언어52/채팅6·ZIP1480항목/실제GUI2회·전체정적자산·보호불변. [근거](docs/exe/cpp-japanese-final-gate-1362-20261126.md). |
| 일본어 도움말 전체1361 | 마지막141줄·공식43/43·128주제/138앵커·한/영/전체 감사·10/10·최장 주제 끝/링크·37그림 참조·실제10주제·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1361-20261126.md). |
| 일본어 도움말1360 | 113~128앵커·118/128고유 주제·125줄·한/영 감사·10/10·자원2/상태2/아웃포스트 그림·실제10주제·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1360-20261126.md). |
| 일본어 도움말1359 | 97~112앵커·108/128고유 주제·168줄·한/영 감사·10/10·10타입/마나1그림·실제12주제·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1359-20261126.md). |
| 일본어 도움말1358 | 81~96앵커·95/128고유 주제·89줄·한/영 감사·10/10·15타입/마나2그림·실제16주제·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1358-20261126.md). |
| 일본어 도움말1357 | 65~80앵커·80/128고유 주제·73줄·한/영 감사·10/10·13타입 그림·실제16주제/워크샾 끝·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1357-20261126.md). |
| 일본어 도움말1356 | 49~64앵커·64/128고유 주제·52줄·한/영 감사·10/10·주문13/에너지14그림·실제16주제·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1356-20261126.md). |
| 일본어 도움말1355 | 33~48앵커·48/128고유 주제·91줄·신전3별칭·한/영 감사·10/10·주문18아이콘·GUI3회·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1355-20261126.md). |
| 일본어 도움말1354 | 17~32앵커·31/128고유 주제·332줄·한/영 감사·10/10·마나4/건물10그림·GUI2회·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1354-20261126.md). |
| 일본어 도움말1353 | 첫16고유 주제·전체128주제 실제 파서·10/10·마나4/채팅5/SP6그림·내부 링크·최종GUI3회·보호3,212불변. [근거](docs/exe/cpp-japanese-help-1353-20261126.md). |
| 일본어 Tell 전체1352 | 마지막196표시 줄·8창 표시·한/영/스타일 감사·9/9·42/43·보호3,212불변. [근거](docs/exe/cpp-japanese-tell-1352-20261126.md). |
| 일본어 Tell 신기술/팁1351 | 고유86표시 줄·팁40개/이전/다음/마지막0 초기화·동적 썬 캐논 그림·한/영/스타일 감사·9/9·최종GUI2회·보호3,212불변. [근거](docs/exe/cpp-japanese-tell-1351-20261126.md). |
| 일본어 Tell 크레딧1350 | 고유138표시 줄·고유명 보존·한/영/스타일 감사·9/9·메뉴3항목/실제19페이지 버튼 전환·최초 검사 버튼 가정 교정 후 종료0·보호3,212불변. [근거](docs/exe/cpp-japanese-tell-1350-20261126.md). |
| 일본어 Tell 두 번째 구간1349 | About~UCampaign 고유125표시 줄·부분Tell/전체41유지·한/영/스타일 감사·9/9·GUI2회·메뉴 버튼과 그림2개·보호3,212불변. [근거](docs/exe/cpp-japanese-tell-1349-20261126.md). |
| 일본어 Tell 첫 구간1348 | COMMENTS~WELCOME2 번역·전체41/43/존재42/부분Tell 구별·한/영/스타일 감사·9/9·GUI3회·시작 안내 버튼/경고2줄·보호3,212파일 불변. [근거](docs/exe/cpp-japanese-tell-1348-20261126.md). |
| 일본어 캠페인3 3-4~3-5 1347 | 공식41/43·한/영 독립 구조 감사, 실제GUI2회 종료0·일본어 다음/시작·기존 진행값 보존·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-campaign-1347-20261126.md). |
| 일본어 캠페인3 3-1~3-3 1346 | 공식39/43·한/영 독립 구조 감사, 실제GUI3회 종료0·일본어 다음/시작·기존 진행값 보존·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-campaign-1346-20261126.md). |
| 일본어 캠페인2 후반/데모·AI 기본 이름1345 | 공식36/43·한/영 감사, 확장 이름31BYTE 제한/기본 번역16BYTE·Release·관련57/57. 최초 데모1실패 뒤 교정·수정 후 데모3회 종료0/자연 timeout·일반2회·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-campaign-1345-20261126.md). |
| 일본어 캠페인2 2-1~2-3 1344 | 공식33/43·한/영 구조 감사, 실제GUI3회 종료0·일본어 다음/시작·진행값 보존·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-campaign-1344-20261126.md). |
| 일본어 캠페인1 1-4~1-6 1343 | 공식30/43·한/영 독립 구조 감사, 실제GUI3회 종료0·1-5 @20 편지 자연 발생/후속10본문 표시·진행0·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-campaign-1343-20261126.md). |
| 일본어 캠페인1 1-1~1-3 1342 | 공식27/43·한국어/원본 영어 독립 구조 감사 차이0, 실제GUI3회 종료0·일본어 시작 버튼/비정지·기존 진행값 보존·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-campaign-1342-20261126.md). |
| 일본어 사제훈련bc7~9 1341 | 공식24/43·구조 감사, 실제GUI3회 종료0·그림9개/6구간·@1 선택 안내 자연 발생·진행 완료값0·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-training-1341-20261126.md). |
| 일본어 사제훈련bc4~6 1340 | 공식21/43·명령/AI/SP 보존 감사, 실제GUI3회 종료0·그림12개/3구간·일본어9/9·보호/설치3,212파일 불변. [근거](docs/exe/cpp-japanese-training-1340-20261126.md). |
| 원본 주문/포획 교정1339 | 단독 주문 whitelist 제거·NB18 실제 운반물0x20 방어/주문1 허용. Cast/Drop3·실제 같은 시전 골렘 포획1·KnownBug8 통과, Release 성공·GUI/전체/ZIP 미실행. [원문·검증](docs/exe/cpp-spell-capture-1339-20261124.md). |
| 일본어 번역1333~1338(진행) | UI 누락23키(1333, 한/일 각536키)·공식 번들 연결/config/캠페인 목록9파일·무공백 줄바꿈(1334)·튜토리얼6개와 안내 그림39개/17구간(1335~1337)·사제훈련bc1~3(1338)으로 공식 본문18/43. 1335의 마나 그림 번호 교정(8~11·17→4~7·9)과 CD 원본 프레임 보충은 제품 HTML 연결 전체에 적용되는 언어 무관 표시 변경이다(영어·한국어·일본어 확인). 남은 단위는 [2절](#lang). [번역판](docs/exe/cpp-japanese-translation-20261123.md)·[1334](docs/exe/cpp-japanese-bodies-1334-20261123.md)·[1335](docs/exe/cpp-japanese-tutorials-1335-20261123.md)·[1336](docs/exe/cpp-japanese-tutorials-1336-20261123.md)·[1337](docs/exe/cpp-japanese-tutorials-1337-20261123.md)·[1338](docs/exe/cpp-japanese-training-1338-20261124.md). |
| 추가 요구1328~1330·후속1331~1332 | 방향키 카메라·한국어 ESC 메뉴·버전 창 Copyright([0.1 요구 목록](#user-requests-20261123)), 포획 안내 검사 장치 교정(메뉴 확장256 중 예외2 → 관련3/3), ZIP1,437항목 CRC·압축해제본 GUI2경로·원본/사본3,212지문 불변. [1331](docs/exe/cpp-carrier-notice-tests-1331-20261123.md)·[1332](docs/exe/cpp-followups-gate-1332-20261123.md). |
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

판정34건: 수정10·기반영1·미재현4·해당없음1·D4 정보 필요12·MP 미수정6이다. [NB01~34 현황표](docs/exe/cpp-known-bugs-status-20261111.md)가 근거·재현 조건의 기준이다. NB04/NB29 공식은D5, 새로운 제보는 해당 행을 다시 연다. 사용자 지시로1339에서 NB18의 잘못된 주문1 검사를 실제 운반물0x20으로 교정했고 원본과 같은 주문 보유 골렘 포획/Drop을 확인했다. [1339](docs/exe/cpp-spell-capture-1339-20261124.md)

<a id="lang"></a>

## 2. 기타 언어 — 일본어 본문 번역(D3) 진행

일본어 UI(509키+보완23키)는 완료했고 공식43본문 전체와help128고유 주제/138앵커를1361까지 번역했다. help부분 상태해제와최종용어/배포관문1362도완료했다. 새 본문은 `assets/lang/japanese/d/`(UTF-8 BOM)에 두며 없는 파일은 영어 대체를 유지한다. 단위 기준은 [번역판](docs/exe/cpp-japanese-translation-20261123.md)이며 사람 문장 검수는0.3에 남긴다.

Tell·help의 세부 단위(2026-11-24 정리에서 번역판의 "캠페인별 또는4파일 이하" 기준으로 나눔). **각 단위 공통 완료 기준:** 표시 문장·버튼 라벨을 번역하고 키·치환·명령·AI/SP·수치를 보존 감사하며, 관련 집중 검사와 실제 진입/표시 GUI 종료0, 안내 그림(유닛·건물·간헐천·아이콘)의 전체 그림·라벨·겹침·잘림·누락을 확인한다. 1-1~1-5 외 특정 적 공격·포획/희생·승리는 실행하지 않는다.

- [x] **1340 사제훈련bc4~6 완료**(`bc4bow_rush76`·`bc5walkers76`·`bc6mows76`, →21/43).
- [x] **1341 사제훈련bc7~9 완료**(`bc7sun_bars76`·`bc8fury76`·`bc9ssun_bars76`, →24/43).
- [x] **1342 캠페인1 1-1~1-3 완료**(`thewarbegins`·`masterofwhirligigs`·`savetheisland`, →27/43).
- [x] **1343 캠페인1 1-4~1-6 완료**(`fragilefortune`·`thunderingpower`·`dissolvedalliance`, →30/43).
- [x] **1344 캠페인2 2-1~2-3 완료**(`thenoose`·`rainvsrain`·`runforit`, →33/43).
- [x] **1345 캠페인2 2-4~2-5·데모 완료**(`guardmyback`·`surrounded`·`demostormrages`, →36/43). 데모는 실제 진입과 자동 안내만 확인한다.
- [x] **1346 캠페인3 3-1~3-3 완료**(`breakingthrough`·`totherescue`·`vicious`, →39/43).
- [x] **1347 캠페인3 3-4~3-5 완료**(`enemyterritory`·`finalconfrontation`, →41/43).
- [x] **Tell 공통 창 완료1352**(`tell`, →42/43). 섹션 이름·명령·매크로·기술/진행 키를 보존하고 비활성 온라인/관리 경로는 번역만 한다. `translation-progress.json`에 부분 번역을 기록하여 미완료 영어 구간을 전체 완료로 세지 않는다.
  - [x] **1348 COMMENTS~WELCOME2.** 한/영·스타일 감사·9/9·실제GUI3회, 시작 안내 버튼과 경고2줄 표시 완료.
  - [x] **1349 About~UCampaign 완료.** Credits 직전까지, 버전/소개·데모 목록·미션 이탈/편집/캠페인 공통 창. 명령·치환·그림 감사와 대표 GUI 표시.
  - [x] **1350 Credits~Credits13 완료.** 고유명/원저작권을 보존하며 역할·제목·버튼 번역, 11.01 추가 크레딧과 실제 표시 확인.
  - [x] **1351 NewTech~TipList 완료.** 신기술/사제·연결 경고·팁40개·데모 안내. Warning/치환/내부 명령 보존 감사와 대표 표시.
  - [x] **1352 AllianceRequested~END 완료.** 동맹·희생/전투·DirectDraw·서버/종료 창196줄·8창 표시·9/9. Tell 전체 감사/부분 상태 해제(42/43).
- [x] **도움말128주제 완료1361**(`help`, →43/43). 원본 앵커 순서로16주제 이하 단위(8단위 안팎)로 나눈다. 링크/그림/스타일/치환을 보존하고 128주제 파서·표시 부호화·대표 실제 도움말 화면을 확인한다.
  - [x] **1353 1~16앵커 완료.** sacrificeOutline~bridgeType,16/128고유 주제·203줄·10/10·대표 그림/스크롤/내부 링크.
  - [x] **1354 17~32앵커 완료.** teleportViewHelp~rainVortexType,332줄·31/128고유 주제·10/10·마나4/건물10그림.rain별칭의 공통 본문은 다음 단위.
  - [x] **1355 33~48앵커 완료.** windVortexType~bombIIITwisterType,91줄·48/128고유 주제·10/10·18주문 아이콘. rain별칭도 완료.
  - [x] **1356 49~64앵커 완료.** bombIManoType~influenceHelp,52줄·64/128고유 주제·10/10·주문13/에너지14그림.
  - [x] **1357 65~80앵커 완료.** factoryHelp~sunCannonType,73줄·80/128고유 주제·10/10·13타입 그림.
  - [x] **1358 81~96앵커 완료.** sunBalloonType~thunderWalkerType,89줄·95/128고유 주제·10/10·15타입/마나2그림. 벌프 공통 본문은 다음 단위.
  - [x] **1359 97~112앵커 완료.** bulfType~windVortexType 중복 구간,168줄·108/128고유 주제·10/10·10타입/마나1그림.
  - [x] **1360 113~128앵커 완료.** 중복thunderVortexType~reliabilityHelp,125줄·118/128고유 주제·10/10·4본문 그림/아웃포스트 타입.
  - [x] **1361 129~138앵커 완료.** battlemasterHelp~battleOptionsHelp,141줄·128/128고유 주제·10/10·37그림 참조·help 전체 감사/부분 상태 해제.
- [x] **1362 일본어 최종 관문 완료.** 용어 대조: 본문マンオウォー/UIマンオブウォー·고갈된 간헐천 일본어 표기·Air Ship의エアシップ/UI飛行船·Outpost의アウトポスト/UI前哨基地. 1353의SP그림 수5→6도교정했다. 공식43/43·언어52/채팅6·ZIP1480/실제GUI2회완료. 공식43파일·도움말128주제·메뉴/번역표·명령/수치/치환 보존 감사, 관련 집중 검사, 실제 미션/메뉴/도움말·배포본을 확인한다. 자연 완주/MP/물리 IME는 제외한다.

<a id="final-gate"></a>

## 3. M8 최종 관문 — 1327 완료

제품1326의 [최종 보고](docs/exe/cpp-m8-final-gate-1327-20261122.md)·[기계 판독 결과](cpppj/m8-final-gate-1327-20261122.json)가 현재 기준이다. Release 종료0·ZIP1,437항목 전수·일반31/데모3 전체진입·3언어Credits/스커미시4경로·QuickJS6/6·아이콘 원본4payload 일치·외부 정적 감사·배포1,437/보호2,808파일 불변을 확인했다. 최신 선정752는751통과·NB08의 옛 경로 기대1실패 뒤 해당1건만 교정해 통과했으며 전체를 반복하지 않았다.

ZIP: `extracted/cpp-m8-1327/NetstormReborn-11.01-cpppj.zip`(205,883,413바이트), SHA-256 `b8d7f37d9b649db46760ae02cea4140e36bc1954d6aa5b166433a85abaeb5a05`.

기존1327 관문은 유지하며, 추가 요청의1332 소규모 배포 확인도 완료했다([근거](docs/exe/cpp-followups-gate-1332-20261123.md)). 사용자 후속 지시로 일본어 번역(D3)을 진행한다. 사용자 직접 확인·CD/선택 보류·전체 솔루션/원본 전체 동일성 미판정은 유지한다.

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
- 2026-11-26 사용자 goal로 **TCP/IP 로컬 상대 봇 개발과 MP 실행 검증을 재개한다**. 기존 디컴파일을 기준으로 구현하며, 실제 LAN·물리 환경의 사용자 확인은 유지한다. 옛 MP 단위/통신 실행 제외는 이 범위에 적용하지 않는다. 공중3종(훨리기그·맨오워·더스트 데빌) 전용 생성/생산/행동 관측도 실행 제외다.
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
9. 현재 문서의 지시가 옛 보고서의 "다음"보다 우선한다. 기존 자동 진행 범위는1327에서 마감했고 2026-11-23 추가 요구1328~1330도 완료했다. 2026-11-26 goal로 활성 cpppj 잔여 전체를 진행한다. 순서는 D3 일본어 번역([2절](#lang)) → 도움말 setter → TCP/IP 상대 봇·MP 검증이다. CD 재개와 선택 추가 작업은 별도 사용자 지시를 기다린다.
10. **LEFT_JOBS 정리 요청 시**(AGENTS.md `1fc63cb1a`): 정리 직전 원문 전체를 [LEFT_JOBS.old.md](LEFT_JOBS.old.md) 끝에 앵커·SHA-256과 함께 보관한 뒤 만료·상세 불필요 내용은 요약만 남기고, 완료됐는데 미표시인 항목을 점검하며, 너무 큰 작업 단위는 완료 기준이 있는 세부 단위로 나눈다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 7. 근거·보관

[1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339)에는 2026-11-24 정리 직전 문서 전체(0.1의 1328~1339 단위별 경과 문단, 추가 요구 세 항목의 원 설명, D3 행·0.5 제품 행의 경과, 7절의 2026-11-23 두 정리 기록)를 원문 바이트로 보존했다. [2차 정리 전 보관본](LEFT_JOBS.old.md#archive-20261123-second-through-1327)에는 2차 정리 직전 문서 전체(0.7의 WB E1288 점검 세부 문장, 1절의 검사 교정 단위별 6행·M8 1311 행)를 원문 바이트로 보존했다. [1327 전체 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327)에는 2026-11-23 정리 직전 문서 전체를 원문 바이트로 보존했다. 0.1의 1314~1327 단위별 상태 행, 0.6~0.8의 완료 세부(최초 실행 관찰·글꼴 로딩/검증 체크·아이콘 리소스 표), 3.1 이전 관문1311 표, 머리의 2026-11-17 원격 병합·2026-11-19 아이콘 요구 안내는 이쪽에서 찾는다. 단위1302~1310의 경과·끝난 작업 상세·AGENTS 변경 이력표·낡은 다음 지시는 [1310 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310)에 있다. 기존 보관본은 수정하지 않았다.

**2026-11-23 정리 두 번의 기록**(WB 최초 실행·갈무리11·favicon 상태 바로잡기, 1절 검사 교정 행 합치기, D8 완료 기준 추가)은 [1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339) 7절에 원문이 있다.

**2026-11-24 정리 기록(AGENTS 정리 규칙).** ① 이관·요약: 0.1의 1328~1339 단위별 경과 문단을 현재 위치+다음 행동 목록으로 바꾸고, 추가 요구 세 항목·D3 행·0.5 제품 행·3.1 이전 관문 문장을 요약으로 줄였다(원문은 1339 보관본). ② 바로잡은 것: 머리(1339)와 0.5 제품 행(1338)의 불일치, D3 행의 "다음1339부터"(1339는 주문/포획 교정에 쓰여 번역 다음 단위는1340), 6절 9번의 "추가 요구1328~1330을 진행한다"(완료), `#decisions` 앵커와 0.2 제목 사이에 끼어 있던 웹 E1324 C++ 후보(0.1 체크 항목으로 옮김). 1절에 1328~1332·1333~1338 묶음 행을 더하고 1335가 언어 무관 표시 변경임을 적었다(웹 정리에서 발견). ③ 나눈 것: D3 남은25파일을 2절의 11개 세부 단위로 나눴다(공통 완료 기준 포함). 완료 미표시는 없었다: DN 최초 실행·갈무리11 화면·창 아이콘은 DN 코드에 아직 없고(`welcomeGiven`·`KoreanFontFamily`·창 아이콘 설정 없음), WB 탭 아이콘은 사용자 눈 확인, C++ `SetNodeHelp` 플래그 쓰기는 미착수다. `cpppj/src` 마지막 변경은 `13e74579b`(1339), 마지막 디컴파일 HJOW-Athlon, AGENTS.md 마지막 변경은 `1fc63cb1a` 그대로다. [번역판](docs/exe/cpp-japanese-translation-20261123.md)의 "다음1339:bc4~6"도 1340으로 고쳤다.

주요 기준: [SOURCE_MAP](cpppj/SOURCE_MAP.md)·[디컴파일 신뢰도](docs/exe/decompile-reliability.md)·[제품](docs/exe/cpp-product-1101-20261101.md)·[미션 대응](docs/gameplay/demo-campaign-map.md)·[MCP/session dump](docs/exe/cpp-session-state-dump-20261109.md). DN/WB 요청·상태는 각 인계 문서를 따른다. AGENTS.md는 수정하지 않으며 현행1~15차·DN/WB4~15차 범위는 각 프로젝트에서 유지한다.
