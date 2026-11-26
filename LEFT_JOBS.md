# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**제품 1377 / 11.01 · 4차 정리 2026-11-26(HJOW-Athlon) · M8 관문1327 · 일본어 최종 관문1362 · 마지막 단위1380(2026-11-27, 원본 LAN 범위 확정) · 현재 허용 작업 마감 · 새 ID 중도 참가 추가 안 함 · 마지막 디컴파일 HJOW-Athlon · CD/QuickJS 추가 연동/보간 보류 유지**

현재 상태·남은 작업·유효한 규칙만 유지한다. 2026-11-26 정리 직전 원문(1340~1370 단위별 완료 행, 2절 일본어 세부 단위 체크, 0.5 제품 행과 0.3·0.4의 LAN 단위별 경과)은 [1370 정리 전 보관본](LEFT_JOBS.old.md#archive-20261126-through-1370)에 있고, 그 이전 보관본과 각 보관본에서 찾을 내용은 [7절](#refs)에 있다. 개별 구현·검증은 아래 근거 문서를 따른다. [dotnetpj](LEFT_JOBS.dotnetpj.md)·[webpj](LEFT_JOBS.webpj.md)는 각 인계 문서가 기준이다.

<a id="now"></a>

## 0. 현재 작업판

### 0.1 다음 행동

**현재 위치(2026-11-27).** 사용자 목표를 재개했으며 최신 지시로 [튜토리얼 안내](#tutorial-next-condition)를 LAN 후속보다 먼저 진행한다. 1371~1372에서 TUT-1~5 조사·40단계/3언어 안내·표시·진행 상태 결선과 허용 범위 검증을 완료했다. 1373에서 실제 TCP 골렘 생산·양쪽 객체/SP·복구 저장·두 정상 종료를 확인했다. 1374에서 썬 워크샾과 단일 다리·양쪽 객체/SP300·저장/두 정상 종료도 확인했다. 1375에서 기존 참가자 복구의 세 결함을 교정하고 실제 TCP·집중130/130·튜토리얼 회귀를 마쳤다. 1376에서 최신 ZIP1,480항목·압축 해제본 실제GUI2회/종료0·배포1,480/보호2,810 불변을 확인했다. 1377 완료 감사에서 실제1375 실패의 SID/TYPE/프레임 문맥이 오류 파일에 빠지는 누락을 교정하고 집중18/18·정상 한국어GUI/종료0·보호2,810 불변을 확인했다. 1378에서 최신 ZIP1,480항목/압축 해제본 GUI2회/종료0·배포/보호 불변도 확인했다. [1377 로그](docs/exe/cpp-error-context-1377-20261127.md)·[1378 ZIP](docs/exe/cpp-final-package-1378-20261127.md). 1380 사용자 지시로 D9를 원본 범위 유지로 확정했다. 새 ID 중도 참가는 추가하지 않고 기존 ID의 저장 복구만 유지한다. 최신 제품1377의 실제 TCP 생산/추가 건설/기존 ID 복구도 통과했다(공유7,305/로컬51·SP 일치·세 종료0). 현재 EXE/ZIP1378 해시와 보호2,810 불변을 대조해 현재 허용 작업을 마쳤다. [1380 마감 근거](docs/exe/cpp-original-lan-scope-1380-20261127.md). [1375 복구](docs/exe/cpp-lan-restart-1375-20261127.md)·[1376 ZIP](docs/exe/cpp-final-package-1376-20261127.md). [1374 근거](docs/exe/cpp-lan-extra-build-1374-20261127.md). [1373 근거](docs/exe/cpp-lan-production-1373-20261127.md). [1372 근거](docs/exe/cpp-tutorial-objective-display-1372-20261127.md).  기존 자동 진행 범위는 M8 관문1327에서 마감했다. 그 뒤 사용자 추가 요구1328~1330과 후속1331~1332, 원본 주문/포획 교정1339, D3 일본어 번역(UI1333·본문43/43·도움말128주제, 최종 관문1362), 도움말 플래그1363을 끝냈다. 2026-11-26 사용자 goal에 따라 TCP/IP 로컬 상대 봇과 MP 실행 검증을1364~1370에서 진행했고(옛 MP 실행 제외보다 우선), **1370 뒤 일시 중지는2026-11-27 재개 지시로 해제했다.** CD10.72·QuickJS 추가 연동·프레임 보간과 명시 사용자 확인 검사는 보류한다. 단위별 결과는 [1절](#done)과 각 근거 문서에 있다.

**1379~1380 범위 확인.**1379에서 최신 ZIP/현재 EXE 해시·GUI2회 종료0·로그18/18·완료 커밋과 작업 트리를 확인하고 D9 응답을 기다렸다.1380 사용자가 "원본대로 진행 (전투 중 참가 기능 추가하지 않음)"으로 결정해 대기 사유를 해소했다. 기존 ID의 재시작·저장 복구를 원본 LAN 범위로 유지한다. 최신 제품 실제 TCP와 보호 해시 대조까지 완료해 현재 허용 작업을 마감했다. DN 항목은 CP 범위 밖이고 D8·탐색기 아이콘 등 기존 사용자 보류/직접 확인은 유지한다.

1. **LAN 후속 결과와 범위 결정.** [1370 근거](docs/exe/cpp-lan-cursor-save-1370-20261126.md)의 "다음 재개" 범위를 완료 기준이 있는 세부 단위로 나눴다. 공통: [봇 안내](docs/testing/cpp-lan-opponent-guide.md)의 새 두 프로필·실시간 TCP로 실행하고 포획/희생·특정 적 공격/승리·실물 LAN·명시 사용자 확인은 실행하지 않는다. 게임 상태·ACK·비용·저장 성공을 주입하지 않으며([4.1](#contracts)), 새로 확정한 MP 사항은 [DN 후속](docs/dotnet-cpp-lan-followup-20261126.md)에 DN-MP 행으로 남긴다.
   - [x] **LAN 유닛 생산(1373).** 실제 두 참가자 월드에서 유닛 하나를 생산한다. 완료 기준: 1370처럼 양쪽 객체의 SID/자산/owner/좌표/HP와 SP 차감이 일치하고 두 게임 종료0, 관련 집중 검사 통과. **1373 완료: 골렘 SID15249/좌표116,206/HP50·SP1500→1100·저장owner1/SP1100·두 종료0·관련14/14.**
   - [x] **LAN 추가 건설(1374).** 1370의 Wind 신전 외 건물·다리를 더 배치한다. 완료 기준: 같은 객체/SP 대조, 호스트 복구 저장(`Saves/!state.dat`)의 owner/SP 갱신, 두 게임 종료0. **1374 완료: 썬 워크샾HP3000/abstract해제·신규 단일 bridge 전체 행/0비용·SP300/저장300·두 종료0·집중19/19.**
   - [x] **기존 참가자 재시작·복구(1375).** 원본 epoch 시계·State13 로컬 SP·Residence SavedGame 결선을 교정했다. 실제 새 프로세스의 owner2/ID 복원·공유 SID객체6,525/로컬 표면45·양쪽 SP300/6500·세 종료0·집중130/130·튜토리얼 한국어 실제 회귀/종료0·보호2,810 불변을 확인했다. [근거](docs/exe/cpp-lan-restart-1375-20261127.md).
   - [x] **오류 문맥의 사용자 파일 기록(1377).** 실제1375 실패에서 누락됐던 표시 상세/Regular SID·TYPE·frame·event를 공통 파일에 연결했다. 원래 예외·상세/풀 불변과 PATCH/CD 범위를 포함한18/18·정상GUI/종료0·보호2,810 불변을 확인했다. [근거/검사 사용법](docs/exe/cpp-error-context-1377-20261127.md).
   - [x] **로그 보완 반영 ZIP(1378).** 제품1377/커밋aed29e3d1의 최신 ZIP1,480항목·CRC/원천 해시/중복0·글꼴/라이선스·압축 해제본 한국어 전체화면/일본어 창 GUI2회/종료0·배포1,480/보호2,810 불변을 확인했다. [근거](docs/exe/cpp-final-package-1378-20261127.md).
   - [x] **D9 원본 LAN 범위 확정(1380).** 사용자 지시(2026-11-27): 원본대로 진행하며 전투 중 새 ID의 참가 기능은 추가하지 않는다.1375에서 구현한 기존 ID의 재시작·저장 복구를 유지한다.1380에서 최신 배포 제품과 같은 EXE로 실제 TCP 생산/추가 건설/복구·공유7,305/로컬51·SP/세 종료0·보호2,810 불변을 재확인했다. [마감 근거](docs/exe/cpp-original-lan-scope-1380-20261127.md).
   - [x] **최종 ZIP 갱신(1376).** 제품1375/커밋84182644a의 최신 ZIP1,480항목을 생성했다. CRC/원천 해시·중복0·글꼴/라이선스·설정 제외·압축 해제1,480파일/246,000,122바이트가 일치하고 한국어 전체화면/일본어 창 실제GUI2회/종료0·실행 뒤 배포/보호 불변을 확인했다. [ZIP 근거](docs/exe/cpp-final-package-1376-20261127.md).

| 구분 | 상태와 다음 행동 |
| --- | --- |
| 최신 기준 | 제품**1377**(Release 성공; 단위1328~1377는 관문 밖 집중 검사·실제 GUI/TCP 실행으로 확인)·LAN 생산/추가 건설/기존 참가자 복구 실제 검증1373~1375·최신 제품 재확인1380·**최신 M8 관문1327**(제품1326)·추가 배포 확인1332·일본어 최종 관문1362·**최신 ZIP1378**(제품1377, 1,480항목/실제GUI2회). 완료 묶음과 근거는 [1절](#done), 관문 수치는 [3절](#final-gate), 검사 기준선은 [4.2절](#baselines)이다. |
| 사용자 확인·결정 | [0.3](#user-checks)의 플레이·MP·물리 환경 확인은 AI 실행 범위 밖이다. [0.2](#decisions)의 미결 사항은 현행/보류 범위를 유지한다. D9는1380에서 원본 범위 유지로 확정했다. |
| **CD10.72 복원 C3~C5** | **사용자 보류(2026-11-14). 재개 지시 전 진행하지 않는다.** 메뉴·CDDir 저장 후 재실행은1309에서 확인했다. 1310의 Start/AI 설정은 순수 몸체만 검증했고 실제 Session/Guide/SP/AI 생성자에는 미연결이다. 첫 튜토리얼 차단은 `RawMenuHtml.inc:3`의 PATCH 전용 HTML 검사다. [보류 인계·재개 지점](docs/exe/cpp-cd1072-deferred-20261114.md). |
| AI 자동 플레이 정체 | 1326으로 자동 범위 완료(초기화/프레임 미연결 아님). 1-3 후반 재시도 대기·1-4 돈 정체 미재현·3-1 옛 오류의 판정은 [1326](docs/exe/cpp-autoplay-ai-diagnostics-1326-20261122.md)을 따른다. 임의 전략 변경 없이 사용자 자연 플레이/동일성 확인을 유지한다. |
| 테스트 자동 플레이 | **플레이 결과 재현이 필요한 AI는 먼저 [AI 에이전트용 사용 안내](docs/testing/cpp-autoplay-test-guide.md)를 읽고 [실행 도구](tools/cpp_campaign_autoplay.py)를 쓴다.** 명령줄 전용 `--test-autoplay`·`--test-ally-ai`·`--test-enemy-ai`·`--test-start-sp`·`--test-enemy-start-sp`·`--test-follow-battle`이며 허용 범위는 [5절](#rules-verify)이다. 1313 일괄 실행 결과는 [결과 7절](docs/exe/cpp-campaign-test-play-20261114.md#7-캠페인-전체-자동-플레이-실행-1313-2026-11-14-후속). |
| DN/WB 후속 | 최초 실행은 DN만 남음([0.6](#first-run)), 한국어 글꼴 화면 적용은 DN만 남음([0.7](#korean-font-change)), 아이콘은 DN 창 표시·WB 탭 시각 확인만 남음([0.8](#exe-icon)). 추가 요구1328~1330은 WB가 E1289~E1291로 적용했고(버전 창 일본어 줄은 웹 13차로 미룸) DN은 해당 소비자를 연결할 때 따른다. 언어 무관 변경인1335 마나 그림 교정·1345 기본 AI 이름31BYTE 제한·1363 도움말 플래그(`SetNodeHelp`의 LOOK1000/HELP200·빈값 해제)는 WB 재반영 판정·DN PORT_MAP 재대조 대상이다. 1363~1370의 MP 확정 사항은 [DN 후속](docs/dotnet-cpp-lan-followup-20261126.md) DN-MP-01~08에 있다(webpj는 멀티플레이 제외). 각 인계 문서에서 진행하며 CP 단위에서는 DN/WB 코드·인계를 수정하지 않는다. |
| 선택/추가 제보 | 비행체 표시 보간·QuickJS 추가 연동은 D2·D7 보류를 유지한다. D3 일본어는1362최종관문까지완료했다([2절](#lang)). 새 재현 자료가 오면 해당 NB 행을 다시 연다. |
| 한국어 글자 선명도 | **사용자 지시(2026-11-23): 문서에만 반영하고 지금 작업하지 않는다.** cpppj 후속 세 항목(전체화면 비정수 확대의 1px 획 불균일·고해상도 글자 층·webpj와의 배율/굵기/기준선 차이)은 [0.7](#korean-font-change) 체크에 완료 기준과 함께 있다. 재개는 [D8](#decisions) 결정 뒤 새 단위로 한다. |
| 튜토리얼 다음 단계 조건 표시 | **1371~1372 구현/허용 범위 검증 완료.** 초기 미션(튜토리얼)에서 다음 알림창 또는 미션 종료창의 발생 조건을 화면 우측 하단에 배경 없이 계속 표시한다. 조건 조사·문구·표시·단계 연동·검증의 세부 작업은 [요구와 완료 기준](#tutorial-next-condition)에 있다. |

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

후속 작업은 아래 순서로 나누고, 각 단계의 조사 결과와 구현 근거를 별도 단위 문서에 남긴다. TUT-1~5의 구현/허용 범위 검증을 완료했으며, 사용자 직접 확인 범위는 그대로 남긴다.

- [x] **TUT-1 대상·조건 조사(1371).** 초기 미션 목록과 각 미션의 단계별 알림창·종료창 발생 조건을 실제 진행 로직에 대응시킨다. 완료 기준: 대상 미션마다 단계 식별자·다음 창·충족 조건·단계 전환 시점을 표로 정리하고, 복합 조건과 마지막 종료 조건까지 누락 없이 확인한다.
- [x] **TUT-2 안내 문구·언어 자산(1372).** TUT-1의 조건을 플레이어가 수행할 행동으로 풀어 쓰고 기존 언어 선택 체계에 연결한다. 완료 기준: 모든 대상 단계에 영어·한국어·일본어 문구가 대응하며, 건물·유닛 이름은 AGENTS.md의 표기 규칙을 따른다.
- [x] **TUT-3 우측 하단 표시(1372).** 현재 단계 안내를 미션 화면에 계속 그리는 표시 경로를 추가한다. 완료 기준: 별도 배경 없이 게임 화면 위에 문구가 표시되고, 긴 문구의 줄바꿈·화면 여백·HUD 겹침을 처리하며 마우스 조작을 방해하지 않는다.
- [x] **TUT-4 진행 상태 연동·정리(1372).** 실제 튜토리얼 단계 전환에 맞춰 문구를 갱신하고, 미션 종료·중단·재시작·다른 미션 진입 시 표시 상태를 정리한다. 완료 기준: 다음 알림창과 종료창의 원래 조건·발생 순서는 보존되고, 이전 단계나 이전 미션의 안내가 남지 않으며 대상 밖에서는 표시되지 않는다.
- [x] **TUT-5 검증·인계(1372, 허용 범위 완료).** 단계별 조건/문구 대응과 상태 전환을 집중 검사하고 실제 화면을 확인한다. 완료 기준: 영어·한국어·일본어, 창/전체화면과 4:3·16:9·16:10에서 우측 하단 위치·줄바꿈·배경 없음·HUD/입력 보존을 확인하고, 기존 검증 제외·실행 확인 규칙에 따라 미확인 단계는 명시한다.

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
| D9 | 진행 중 LAN의 새 ID 참가 | **1380 사용자 결정 완료(2026-11-27): 원본대로 유지, 새 ID 중도 참가 기능은 추가하지 않는다.** 기존 ID의 저장 복구는1375 완료이며 현재 작업 범위를 확장하지 않는다. |

<a id="user-checks"></a>

### 0.3 사용자 확인 대기 — AI 실행 제외

| 영역 | 확인할 것 |
| --- | --- |
| 플레이·동일성 | 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리, 전체 화면/음향 동일성. |
| MP·LAN | 실제 LAN/Join/Test Battle·debugBurst·Outpost·희생 안내 UI/패킷, 메타 맵 커서 깊이0 반례, 로비 섬 사제·거주 구역 표시. **TCP/IP 상대 봇 실행(대기실·두 참가자 시작·시계·재접속17/18·서버 인수·Wind 신전 배치/SP/복구 저장)은1364~1370에서, 골렘 실제 생산·양쪽 객체/SP·복구 저장·두 종료0은1373에서 AI가 확인했다. 추가 건설은1374·기존 참가자 새 프로세스 복구는1375에서 확인했다. 새 ID 추가는D9에서 하지 않기로 확정했고 최신 ZIP1378과 현재 제품 실제 TCP 재확인1380도 완료했다([0.1](#now)).** 실물 LAN·물리 환경 확인은 사용자 몫이다. |
| 한국어 | 물리 IME·클립보드·본문 문장 교정. 좁은 창의 영문/한글 혼합 H2 제목 잘림은 근본 원인 미확정이다. |
| 일본어 | 물리 IME·클립보드·번역/라이선스 저작권 줄·화면 크기별 굵기. 가변 Noto Sans JP 기본 인스턴스는 Thin이다. |
| 알려진 버그 | NB03/05/09/10/11/18/19/22/23의 실제 조작·장시간·MP·교전 확인. 건별 조건은 [현황표](docs/exe/cpp-known-bugs-status-20261111.md)를 따른다. |
| 난이도·스커미시 | AI 속도 체감·게임 중 변경 반영, 금지 유닛의 AI 적용·동맹/전투/승패·유닛 이름·MP 규칙 섬 배치 체감. |
| 화면·설치 | 60/120FPS 소리 동기, 실물4K/FSR·활성 전체화면·다른 모니터/DPI·앱 전환·청취, 실제 Program Files/UAC·CD·Windows11/ARM. |
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
| 4차 MCP·와이드 | 구현 마감, 최대1920×1080 | 구체적 누락 제보 시 재개 |
| 5차 사용자 홈·맵 목록 | 구현·읽기 전용 설치 확인 | 실제 Program Files/UAC |
| 6차 한국어 | K1~K5·갈무리11 전환/상수 분리1322 완료 | 사람 검수·물리 입력·다른 DPI, 전체화면 글자 선명도는 D8 |
| 7차 오류·상세 로그 | 완료·1377 표시/Regular 문맥 보완18/18 | 던진 위치 스택 없음·ARM64 레지스터 미구현 |
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
| 제품 | **1377 / 11.01**(Release 성공, EXE SHA-256 `6851dfabaa7f6b8eb98eb4c8b312b9374d3287ae4ee845552d6fd9d78dc0846b`). 제품 소스 커밋aed29e3d1. M8 관문1327(제품1326) 뒤 추가 요구1328~1330·후속1331~1332·일본어1333~1362·도움말1363·LAN1364~1375·튜토리얼1371~1372·오류 문맥 로그1377을 반영했다. 현재 제품 실제 TCP 재확인은1380, 최신 ZIP은1378이다. 단위별 검사 수치는 [1절](#done) 근거를 따른다. |
| 실행·플레이 | 10.78 메뉴·미션·스커미시 플레이 가능. CD는 메뉴만 확인했고 튜토리얼/플레이 미완료다. 전체 원본 동일성은 미확정이다. |
| ZIP·전체 검사 | **1327 M8 통과**(제품1326). ZIP1,437항목·배포38고유경로(일반31+데모3+언어Credits3+스커미시1)·QuickJS6/6·배포1,437/보호2,808불변. 최신 전체34진입은1327이며 전체 솔루션 전수 통과로 확대하지 않는다. 최신 선정752는751통과·NB08 검사1실패 뒤 해당1건 교정 통과. [1327](docs/exe/cpp-m8-final-gate-1327-20261122.md). 그 뒤 배포 확인은1332(ZIP1,437)와 일본어 최종 관문1362(ZIP1,480항목·EXE는1345 판, [3절](#final-gate))이며, 최신 ZIP1378은 제품1377/1,480항목·실제 GUI2회 종료0·배포/보호 불변을 확인했고1380에서 ZIP/EXE 해시를 재대조했다([1378](docs/exe/cpp-final-package-1378-20261127.md)·[1380](docs/exe/cpp-original-lan-scope-1380-20261127.md)). |
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
| TCP/IP 상대 봇·LAN 실행1364~1370 | 새 두 프로필·실시간 TCP로 원문 경로를 단계별로 연결했다: 옵션 Push/Move·스타일 교정과 대기실 명단·정상 종료(1364) → 일반 로그인 안내 State7/즉시8·72h 중앙섬 안내(1365) → 인계 안 한 Fort Source 반납·누락 수신6콜백·`444ff0` 채널 해제(1366) → 일반 Fort 객체/공유 모드 교정·두 참가자 시작(1367) → 시계 보정의 음수 경과/월드 역행 보존·60초 이상 반복2회(1368) → 재접속 상태17/18·실제 호스트 종료 뒤 서버 인수(1369) → 배치 비용 차감 뒤 `004b54e0` 복구 저장·Wind 신전 SID/SP 동기화(1370). 단계별 첫 실패와 교정 뒤 결과는 각 근거에 있다. 사용자 요청으로1370 뒤 일시 중지. [1364](docs/exe/cpp-lan-opponent-lobby-1364-20261126.md)·[1365](docs/exe/cpp-lan-login-ui-1365-20261126.md)·[1366](docs/exe/cpp-lan-source-owner-1366-20261126.md)·[1367](docs/exe/cpp-lan-fort-modes-1367-20261126.md)·[1368](docs/exe/cpp-lan-clock-1368-20261126.md)·[1369](docs/exe/cpp-lan-reconnect-1369-20261126.md)·[1370](docs/exe/cpp-lan-cursor-save-1370-20261126.md)·[봇 안내](docs/testing/cpp-lan-opponent-guide.md)·[DN 후속](docs/dotnet-cpp-lan-followup-20261126.md). |
| 튜토리얼 안내1371~1372 | 공식 초기 미션6개의40단계/완료 조건 조사→3언어 투명 글자·상태/수명·입력 투과. 기존 fixture 보존·집중22/22·실제GUI19경로 종료0·보호2,810파일 불변. 포획/희생·특정 공격/승리 자연 전환은 사용자 확인. [조건표1371](docs/exe/cpp-tutorial-objectives-1371-20261127.md)·[구현/검증1372](docs/exe/cpp-tutorial-objective-display-1372-20261127.md). |
| LAN 골렘 생산1373 | 실제 두 참가자 TCP에서 사제 메뉴/아이콘 클릭·허용 배치로 Wind 신전과 골렘을 생성했다. 양쪽 SID/자산/owner/좌표/HP·SP400 차감·호스트 복구 저장·두 정상 종료 및 집중14/14·보호2810불변. 제품은1372와 같다. [근거](docs/exe/cpp-lan-production-1373-20261127.md). |
| LAN 추가 건설1374 | 실제 썬 워크샾 비용800·HP3000/abstract해제와 자연 공급 단일 다리0비용을 확인했다. 양쪽 워크샾/bridge 객체·SP300·복구 저장/두 종료0·관련19/19·보호2810불변. 첫 검사의 건설중 비교 실패는 원시 보존했다. 제품은1372와 같다. [근거](docs/exe/cpp-lan-extra-build-1374-20261127.md). |
| 도움말 플래그1363 | LOOK1000/HELP200·빈값/첫NUL·소유권, 메뉴169/언어52·실제GUI4회·보호3212불변. 기존Battle저장검사교정. [근거](docs/exe/cpp-help-flags-1363-20261126.md). |
| 일본어 번역1333~1362 | UI 누락23키(1333, 한/일 각536키)·공식 본문43/43(1334 번들/config/캠페인 목록9파일과 무공백 줄바꿈, 튜토리얼·사제훈련·캠페인1~3·데모1335~1347, Tell 공통 창1348~1352, 도움말128고유 주제/138앵커1353~1361)·최종 관문1362(용어 대조·언어52/채팅6검사·ZIP1,480항목·압축 해제본 실제GUI2회). 언어 무관 변경: 1335 마나 그림 번호 교정(8~11·17→4~7·9)과 CD 원본 프레임 보충(제품 HTML 연결 전체), 1345 Client 기본 AI 이름의 확장 표시 문자31BYTE 제한. 단위별 완료 행과 세부 체크는 [1370 보관본](LEFT_JOBS.old.md#archive-20261126-through-1370), 단위 문서는 `docs/exe/cpp-japanese-*-13NN-*.md`. [번역판](docs/exe/cpp-japanese-translation-20261123.md)·[최종 관문1362](docs/exe/cpp-japanese-final-gate-1362-20261126.md). |
| 원본 주문/포획 교정1339 | 단독 주문 whitelist 제거·NB18 실제 운반물0x20 방어/주문1 허용. Cast/Drop3·실제 같은 시전 골렘 포획1·KnownBug8 통과, Release 성공·GUI/전체/ZIP 미실행. [원문·검증](docs/exe/cpp-spell-capture-1339-20261124.md). |
| 추가 요구1328~1330·후속1331~1332 | 방향키 카메라·한국어 ESC 메뉴·버전 창 Copyright([0.1 요구 목록](#user-requests-20261123)), 포획 안내 검사 장치 교정(메뉴 확장256 중 예외2 → 관련3/3), ZIP1,437항목 CRC·압축해제본 GUI2경로·원본/사본3,212지문 불변. [1331](docs/exe/cpp-carrier-notice-tests-1331-20261123.md)·[1332](docs/exe/cpp-followups-gate-1332-20261123.md). |
| 최신 M8 관문1327 | 제품1326 Release·ZIP1,437전수·34/34진입+Credits/스커미시4경로·QuickJS6/6·정적 감사·배포/보호 불변. [보고](docs/exe/cpp-m8-final-gate-1327-20261122.md). 이전 관문: [1311](docs/exe/cpp-m8-final-gate-1310-20261114.md)·[요약JSON](cpppj/m8-final-gate-20261114.json)·[1294~1297](docs/exe/cpp-m8-final-gate-1294-20261111.md)([3.1절](#final-gate)). |
| LAN/배포1373~1376 | 실제 골렘·썬 워크샾/다리·기존 ID 새 프로세스 복구를 검증했다.1375는 epoch/SP/SavedGame 결선 교정·집중130/130·공유 객체6,525/로컬45·SP/세 종료0.1376 최신ZIP1,480항목/GUI2회/배포·보호 불변. [1375](docs/exe/cpp-lan-restart-1375-20261127.md)·[1376](docs/exe/cpp-final-package-1376-20261127.md). 새 ID 기능은D9 사용자 결정으로 추가하지 않는다. |
| 오류 문맥/최신 ZIP/범위 마감1377~1380 | 표시 상세와 Regular SID/TYPE/frame/event의 파일 기록18/18·정상 GUI, 최신 ZIP1,480항목·GUI2회 종료0을 확인했다.1380 사용자 결정으로 새 ID 중도 참가를 추가하지 않고 최신 제품 실제 TCP 생산/추가 건설/기존 ID 복구·공유7,305/로컬51·SP/세 종료0·보호2,810 불변을 재확인했다. [1377](docs/exe/cpp-error-context-1377-20261127.md)·[1378](docs/exe/cpp-final-package-1378-20261127.md)·[1380](docs/exe/cpp-original-lan-scope-1380-20261127.md). |
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

## 2. 기타 언어 — 일본어 본문 번역(D3) 완료

일본어 UI(509키+보완23키)·공식43본문·도움말128고유 주제/138앵커를1333~1361에 번역하고 최종 관문1362(용어 대조·언어52/채팅6검사·ZIP1,480항목·압축 해제본 실제GUI2회)를 통과했다. 새 본문은 `assets/lang/japanese/d/`(UTF-8 BOM)에 두며 없는 파일은 영어 대체를 유지한다. 단위 기준은 [번역판](docs/exe/cpp-japanese-translation-20261123.md), 묶음 요약은 [1절](#done)이다. 1340~1362 세부 단위 체크와 단위 공통 완료 기준은 [1370 보관본](LEFT_JOBS.old.md#archive-20261126-through-1370) 2절에 있다.

- 남은 것: 사람 문장·라이선스 저작권 줄 검수와 물리 IME([0.3](#user-checks)), 독일어 등 다른 언어 미노출 유지([D3](#decisions)). webpj·dotnetpj의 13차 반영은 각 인계 문서를 따른다.

<a id="final-gate"></a>

## 3. M8 최종 관문 — 1327 완료

제품1326의 [최종 보고](docs/exe/cpp-m8-final-gate-1327-20261122.md)·[기계 판독 결과](cpppj/m8-final-gate-1327-20261122.json)가 현재 기준이다. Release 종료0·ZIP1,437항목 전수·일반31/데모3 전체진입·3언어Credits/스커미시4경로·QuickJS6/6·아이콘 원본4payload 일치·외부 정적 감사·배포1,437/보호2,808파일 불변을 확인했다. 최신 선정752는751통과·NB08의 옛 경로 기대1실패 뒤 해당1건만 교정해 통과했으며 전체를 반복하지 않았다.

ZIP: `extracted/cpp-m8-1327/NetstormReborn-11.01-cpppj.zip`(205,883,413바이트), SHA-256 `b8d7f37d9b649db46760ae02cea4140e36bc1954d6aa5b166433a85abaeb5a05`.

기존1327 관문은 유지한다. 그 뒤 추가 요청의1332 소규모 배포 확인([근거](docs/exe/cpp-followups-gate-1332-20261123.md))과 일본어 최종 관문1362([근거](docs/exe/cpp-japanese-final-gate-1362-20261126.md), ZIP `extracted/cpp-japanese-final-1362/NetstormReborn-11.01-cpppj.zip` 206,341,786바이트, SHA-256 `f11339669585ceb515fc0a5742127944224e6e0e76ff46f092efeebcdba65288`)를 완료했다. 1363~1375를 반영한 최신 ZIP1376도 완료했다([근거](docs/exe/cpp-final-package-1376-20261127.md), 206,351,258바이트/SHA-256 `ccc1d7b0594027a559567597c1866e1b6af3e5210c2590cc5a6bb7a33cf03348`). 이후1377 로그 보완을 포함한 최신 ZIP1378도 통과했다([근거](docs/exe/cpp-final-package-1378-20261127.md), 206,351,884바이트/SHA-256 `d92b32495a068a0a0a428c0fbecafc091fec36cda688557a8c871b54c129645d`). 사용자 직접 확인·CD/선택 보류·전체 솔루션/원본 전체 동일성 미판정은 유지한다.

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
9. 현재 문서의 지시가 옛 보고서의 "다음"보다 우선한다. 기존 자동 진행 범위는1327에서 마감했고 추가 요구1328~1330·D3 일본어(1362)·도움말 setter(1363)도 완료했다. 2026-11-26 goal의 남은 순서는 [0.1](#now) 재개 순서(LAN 유닛 생산 → 추가 건설 → 기존 참가자 재시작·복구 → 최종 ZIP)이며2026-11-27 지시의 튜토리얼 안내는1372로 마쳤고1373의 골렘 실제 생산 검증도 완료해1374 추가 건설도 마쳤으며1375 기존 참가자 복구도 마쳐1376 최종 ZIP까지 마쳤다. 1377 오류 문맥 로그 보완·1378 최신 ZIP도 마쳤다. D9는1380 사용자 지시로 원본 범위 유지(새 ID 중도 참가 추가 안 함)로 확정됐으며 최신 제품 실제 TCP 재확인도 완료해 현재 허용 작업을 마쳤다. CD 재개와 선택 추가 작업은 별도 사용자 지시를 기다린다.
10. **LEFT_JOBS 정리 요청 시**(AGENTS.md `1fc63cb1a`): 정리 직전 원문 전체를 [LEFT_JOBS.old.md](LEFT_JOBS.old.md) 끝에 앵커·SHA-256과 함께 보관한 뒤 만료·상세 불필요 내용은 요약만 남기고, 완료됐는데 미표시인 항목을 점검하며, 너무 큰 작업 단위는 완료 기준이 있는 세부 단위로 나눈다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 7. 근거·보관

**보관본(최신순).** 모두 정리 직전 문서 전체를 원문 바이트로 보존했으며 기존 보관본은 수정하지 않았다.

- [1370 정리 전 보관본](LEFT_JOBS.old.md#archive-20261126-through-1370): 1절의 1340~1370 단위별 완료 행(일본어 22행·LAN 7행), 2절 일본어 세부 단위 체크(1340~1362)와 단위 공통 완료 기준, 0.1의 1362·1363 완료 항목과 1340~1345 경과 문단, 0.3 MP·LAN 행과 0.4 3차 행의 단위별 경과, 0.5 제품 행의 1340~1367 경과, 7절의 2026-11-24 정리 기록.
- [1339 정리 전 보관본](LEFT_JOBS.old.md#archive-20261124-through-1339): 0.1의 1328~1339 단위별 경과 문단, 추가 요구 세 항목의 원 설명, D3 행·0.5 제품 행의 경과, 7절의 2026-11-23 두 정리 기록(WB 최초 실행·갈무리11·favicon 상태 바로잡기, 1절 검사 교정 행 합치기, D8 완료 기준 추가).
- [2차 정리 전 보관본](LEFT_JOBS.old.md#archive-20261123-second-through-1327): 0.7의 WB E1288 점검 세부 문장, 1절의 검사 교정 단위별 6행·M8 1311 행.
- [1327 전체 보관본](LEFT_JOBS.old.md#archive-20261123-through-1327): 0.1의 1314~1327 단위별 상태 행, 0.6~0.8의 완료 세부(최초 실행 관찰·글꼴 로딩/검증 체크·아이콘 리소스 표), 3.1 이전 관문1311 표, 머리의 2026-11-17 원격 병합·2026-11-19 아이콘 요구 안내.
- [1310 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310): 단위1302~1310의 경과·끝난 작업 상세·AGENTS 변경 이력표·낡은 다음 지시.

**2026-11-26 정리 기록(AGENTS 정리 규칙, 4차).** ① 이관·요약: 1절의 일본어 단위별 22행(1340~1361)과 LAN 7행(1364~1370)을 묶음 행 둘로 합치고, 2절의 끝난 세부 체크·0.1의 완료 항목1362/1363·0.5 제품 행 경과·0.3/0.4의 LAN 단위 나열을 요약했다. ② 바로잡은 것: 0.5 제품 행이1367에서 멈춰 있던 것(→1370), 2절 제목 "진행"(→완료), 3절의 "일본어 번역(D3)을 진행한다"(1362 완료), 6절 9번의 옛 순서(D3 → 도움말 setter → 상대 봇), ZIP 기준에 1362 ZIP1,480항목 추가, 1345 기본 AI 이름 제한을 언어 무관 변경으로 DN/WB 후속에 추가. 완료 미표시는 없었다: DN은 1.054부터 `WelcomeGiven`을 읽어 Welcome/Tips로 나누지만 Welcome2의 `welcomeGiven=1` 저장·재실행 확인이 없고 `KoreanFontFamily`도 없다. ③ 나눈 것: 1370의 "다음 재개" 범위를 0.1의 네 세부 단위(유닛 생산·추가 건설·새 참가자 재접속·최종 ZIP)로 나눴다. `cpppj/src` 마지막 변경은 `ffd35d367`(1370), 마지막 디컴파일 HJOW-Athlon(1363~1370 재디컴파일 없음), AGENTS.md 마지막 변경은 `1fc63cb1a` 그대로다.

주요 기준: [SOURCE_MAP](cpppj/SOURCE_MAP.md)·[디컴파일 신뢰도](docs/exe/decompile-reliability.md)·[제품](docs/exe/cpp-product-1101-20261101.md)·[미션 대응](docs/gameplay/demo-campaign-map.md)·[MCP/session dump](docs/exe/cpp-session-state-dump-20261109.md). DN/WB 요청·상태는 각 인계 문서를 따른다. AGENTS.md는 수정하지 않으며 현행1~15차·DN/WB4~15차 범위는 각 프로젝트에서 유지한다.
