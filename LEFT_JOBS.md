# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**제품 1313 / 11.01 · 정리 2026-11-14(HJOW-Athlon) · 마지막 단위 1313(캠페인 자동 플레이 일괄 실행·제단 총기 피격 연결, 사용자 지시) · 다음 번호 1314 · 마지막 디컴파일 HJOW-Athlon · 사용자 요청으로 목표 일시 중지**

현재 상태·남은 작업·유효한 규칙만 유지한다. 단위별 경과와 만료된 지시는 [1310 시점 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310), 개별 구현·검증은 아래 근거 문서를 따른다. [dotnetpj](LEFT_JOBS.dotnetpj.md)·[webpj](LEFT_JOBS.webpj.md)는 각 인계 문서가 기준이며 이 작업에서 수정하지 않는다.

<a id="now"></a>

## 0. 현재 작업판

### 0.1 다음 행동

| 구분 | 상태와 다음 행동 |
| --- | --- |
| **M8 최종 배포 관문** | **1311 완료.** 확정1310의 Release·ZIP1,436항목·배포본5경로·QuickJS6건·외부 접속 감사를 통과했다([3절](#final-gate)). 사용자 지시(2026-11-14)에 따라 문서·로컬 커밋 뒤 목표를 **paused**로 전환하며 재개 지시를 기다린다. |
| **테스트 자동 플레이 1312** | **완료(2026-11-14 사용자 요청).** 명령줄 전용 `--test-autoplay`·`--test-ally-ai`·`--test-enemy-ai`·`--test-start-sp`·`--test-enemy-start-sp`로 **캠페인·스커미시**에서 로컬 플레이어에도 원본 데모 분기와 같은 AI를 붙여 자동 플레이한다(무인자 일반 실행·options.cfg·메뉴에는 없음). 실제 1-1에서 아군 어려움·적 쉬움·SP 20000으로 **승리(`BadTeamDead`) 재현**, 스커미시 실행 확인. 사람이 지켜볼 수 있게 **관전 카메라**(`--test-follow-battle`, 프리셋 포함: 주요 전투 위치, 희생 의식 중에는 제단으로 이동)도 넣었다. 11차 동맹 AI 난이도 판정도 보정했다. **플레이 결과 재현이 필요한 AI는 먼저 [AI 에이전트용 사용 안내](docs/testing/cpp-autoplay-test-guide.md)를 읽고 [실행 도구](tools/cpp_campaign_autoplay.py)를 쓴다.** [구현 근거](docs/exe/cpp-campaign-test-play-20261114.md). **1313 일괄 실행(사용자 지시로 3-x·2-5 제외, 캠페인 1·2의 10개):** 승리5(1-1·1-2·1-5·2-2·2-3)·패배1(2-1)·미결4(1-3·1-4·1-6·2-4). 2-4는 **제단 총기 피격 미복원으로 비정상 종료 → 수정, 재실행 완주**. 남은 과제: 아군 AI 정체 원인(AI 본체 필드 관측 필요)·희생 의식 카메라 미관측·3-1 `다른 파생 실제 피해 미복원`(3-x 제외로 미수정). [결과 7절](docs/exe/cpp-campaign-test-play-20261114.md#7-캠페인-전체-자동-플레이-실행-1313-2026-11-14-후속). |
| **CD10.72 복원 C3~C5** | **사용자 보류(2026-11-14). 재개 지시 전 진행하지 않는다.** 메뉴·CDDir 저장 후 재실행은1309에서 확인했다. 1310의 Start/AI 설정은 순수 몸체만 검증했고 실제 Session/Guide/SP/AI 생성자에는 미연결이다. 첫 튜토리얼 차단은 `RawMenuHtml.inc:3`의 PATCH 전용 HTML 검사다. [보류 인계·재개 지점](docs/exe/cpp-cd1072-deferred-20261114.md). |
| 선택/추가 제보 | 비행체 표시 보간·번역·QuickJS 추가 연동은 D2·D3·D7 결정 뒤 진행한다. 새 재현 자료가 오면 해당 NB 행을 다시 연다. |

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

<a id="stages"></a>

### 0.4 AGENTS 단계별 상태

10.78의 구현 마감과 전체 플레이·물리 환경 확인을 구별한다.

| 단계 | 상태 | 잔여 |
| --- | --- | --- |
| 1차 10.78 복원 | 비MP 활성 범위 마감 | 자연 완주·전체 동일성은 사용자 확인 |
| 2차 Windows10/11 | 호환 구현 마감 | 다른 모니터/DPI·Windows11/ARM 실기 |
| 3차 LAN·Outpost | 구현·정적 대조 마감 | MP 실행은 사용자 확인 |
| 4차 MCP·와이드 | 구현 마감, 최대1920×1080 | 구체적 누락 제보 시 재개 |
| 5차 사용자 홈·맵 목록 | 구현·읽기 전용 설치 확인 | 실제 Program Files/UAC |
| 6차 한국어 | K1~K5 완료 | 사람 검수·물리 입력 |
| 7차 오류·상세 로그 | 완료 | 던진 위치 스택 없음·ARM64 레지스터 미구현 |
| 8차 AA·FSR1.0 | 구현 마감, 기본OFF | 실물4K·성능·물리 출력 |
| 9차 시나리오 편집 | S1~S4 완료 | 없음 |
| 10차 알려진 버그 | 34건 판정 마감 | D4/D5·사용자 확인 |
| 11차 난이도 | 구현 완료, 1312 동맹 AI 판정 보정 | 실제 전투 체감 |
| 12차 스커미시 | UI·MP 규칙 월드 완료 | D6·사용자 확인 |
| 13차 기타 언어 | 일본어 UI 완료 | D3·사람 검수 |
| 14차 QuickJS | v0.18.0 정적 탑재·자체 검사·ARM64 교차 빌드 완료 | D7·ARM64 실기 |
| 후순위 | 60/120FPS 상한·대기 완료 | 표시 보간은D2, CD복원은사용자 보류 |

### 0.5 기준값

| 항목 | 현재 기준 |
| --- | --- |
| 제품 | **1313 / 11.01**, 코드는 1312·1313 단위 커밋(이전 1310 `09f9f34d5`). 마지막 집중 검사는 `netstorm_campaign_test_play_tests`16건·`netstorm_ai_difficulty_tests`5건·`netstorm_cd_mission_start_tests`10건 통과, NetstormCpp·NetstormRoot Release 종료0. 단위별 결과는 근거 문서에 있다. |
| 실행·플레이 | 10.78 메뉴·미션·스커미시 플레이 가능. CD는 메뉴만 확인했고 튜토리얼/플레이 미완료다. 전체 원본 동일성은 미확정이다. |
| ZIP·전체 검사 | **1311 M8 통과**, ZIP1,436항목·배포본5경로·QuickJS6/6(제품1310 기준). **1312·1313에서 제품 코드가 바뀌었으므로 배포 전 새 ZIP·M8 관문이 필요하다.** 전체34경로 진입의 마지막 기록은 제품644, 최신 전체 솔루션 통과 기록은 없다. |
| 디컴파일 | Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon(192.168.0.94)**. 이후 원문 변경·재디컴파일 없음. [기록](docs/exe/cpp-decompilation-20261103.md). |
| 보호 | AGENTS.md·originals·originalCD·기존 fixture·README 개요를 보존한다. 최근 CD 사본 실행 전후 originalCD 전체 해시는 불변이었다. |

<a id="done"></a>

## 1. 완료 묶음과 근거

이미 마감한 구현을 다시 만들지 않는다. 상세 단위 이력·주소 목록은 [전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310)에 있다.

| 묶음 | 요약·근거 |
| --- | --- |
| 비MP 복원·호환 | [UI](docs/exe/cpp-ui-implementation-closure-20261105.md)·[Edit](docs/exe/cpp-editor-implementation-closure-20261106.md)·[미션](docs/exe/cpp-mission-state-scope-20261106.md)·[수명/FILE](docs/exe/cpp-internal-lifetime-closure-20261106.md). |
| LAN·Native·M7 | M6-a~n 정적 종결, 장기 소유자·희생 안내·오류 로그 연결. [M6 종결](docs/exe/cpp-m6n-reachability-closure-20261111.md)·[오류 로그](docs/exe/cpp-error-log-20261111.md). |
| 한국어·시나리오 편집 | K1~K5·S1~S4 완료. [한국어](docs/exe/cpp-korean-k4-k5-closure-20261111.md)·[읽기 전용 설치/시나리오](docs/exe/cpp-scenario-s4-readonly-home-20261111.md). |
| 언어·제품 표기 | 일본어 UI509키·외부 글꼴·영어 대체, 11.01 Patch Credits/HJOW·AI·Copyright2026 HJOW. [일본어](docs/exe/cpp-japanese-l1-l3-20261111.md)·[제품](docs/exe/cpp-product-1101-20261101.md). |
| 난이도·스커미시·프레임 | [난이도](docs/exe/cpp-difficulty-d-20261111.md)·[MP 규칙 스커미시](docs/exe/cpp-skirmish-sk-20261111.md)·[60/120FPS](docs/exe/cpp-post-plan-fps-cd1072-20261111.md). |
| QuickJS | 탑재·자원 한도·자체 검사·배포 라이선스. [1301](docs/exe/cpp-quickjs-engine-20261111.md). |
| 시험 교정1304~1305 | Pop/로컬 전송 accept·NB18 충돌 시험 교정. 실행 파일 불변. [근거](docs/exe/cpp-test-local-transport-accept-20261112.md). |
| M8 1311 | Release·ZIP 전수·배포본 실행·PE/소스 감사 완료. [최종 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md)·[요약JSON](cpppj/m8-final-gate-20261114.json). 사용자 요청으로 목표 일시 중지. |
| 테스트 자동 플레이1312 | 캠페인·스커미시 아군 AI 자동 플레이·아군/적 난이도·시작 SP·관전 카메라(명령줄 전용)와 실행 도구, 11차 동맹 AI 판정 보정(`o/AiAllianceSide.h`). 1-1 승리 재현·스커미시 확인. [AI 사용 안내](docs/testing/cpp-autoplay-test-guide.md)·[근거](docs/exe/cpp-campaign-test-play-20261114.md). |
| CD1306~1310 | 기본 해상도·메시지·CDDir·Player/메뉴·순수 Start/AI 설정. **후속 보류.** [메뉴/Player](docs/exe/cpp-cd1072-player-initialization-20261114.md)·[Start/AI](docs/exe/cpp-cd1072-mission-start-ai-settings-20261114.md)·[보류 인계](docs/exe/cpp-cd1072-deferred-20261114.md). |

<a id="bugs"></a>

### 1.1 알려진 버그

판정34건: 수정10·기반영1·미재현4·해당없음1·D4 정보 필요12·MP 미수정6이다. [NB01~34 현황표](docs/exe/cpp-known-bugs-status-20261111.md)가 근거·재현 조건의 기준이다. NB04/NB29 공식은D5, 새로운 제보는 해당 행을 다시 연다.

<a id="lang"></a>

## 2. 기타 언어

일본어 UI 이후 본문 번역·추가 언어 노출은 D3 대기다. 글꼴·물리 입력 검수는0.3에 남긴다.

<a id="final-gate"></a>

## 3. M8 최종 관문 — 1311 완료

확정1310의 [최종 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md)·[기계 판독 결과](cpppj/m8-final-gate-20261114.json)가 기준이다. 제품 코드는 바뀌지 않았다.

| 항목 | 결과 |
| --- | --- |
| 빌드·ZIP | Release NetstormCpp·NetstormRoot 종료0. ZIP1,436항목(원천1,429·생성4·런타임3) 대조, 누락/금지/실행 파일 내 글꼴0. QuickJS 라이선스·출처 포함. |
| 배포본 실행 | 영어·한국어·일본어 메뉴/11.01 Credits, 일반 미션 thewarbegins·AI3 스커미시 진입/대기 **5경로 종료0·오류 로그0**. QuickJS 자체 검사6/6. |
| 불변·외부 감사 | 실행 뒤 배포1,436파일·보호1,821파일 불변. 두 PE의 WinINet/WinHTTP/URLMon0, 폐기 검증 비활성·LAN 정책 유지, 새 네트워크/미연결 checking 소비자0. |
| 범위 | 전체34경로/전체 솔루션 검사는 재집계하지 않았다. 기존 실패·5절의 사용자 확인/실행 제외를 유지한다. CD 추가 실행 없음. |

ZIP: `extracted/cpp-m8-1310/NetstormReborn-11.01-cpppj.zip`(209,724,870바이트), SHA-256 `0a94b7f11a39bef8b1c702ddaa6a6ae70cc816224db16dea98ebbdf94485c66e`.

**현재 필수 AI 작업은 마쳤다. 사용자 지시대로 로컬 커밋 뒤 목표를 일시 중지하며, CD·선택 작업·추가 검사는 재개 지시 전 진행하지 않는다.** 향후 제품 코드가 바뀌면 새 ZIP과 관문이 필요하다. 과거 결과는 [1294~1297](docs/exe/cpp-m8-final-gate-1294-20261111.md)에 있다.

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

### 4.2 알려진 기존 실패 — 최신 전수 통과로 바꾸지 않는다

| 대상 | 마지막 확인 기준 |
| --- | --- |
| pop_context | 1304 당시24건. |
| world_generation | 1304~1305 당시 GeneratorBodyShadowAndPicking·AiCarrierActionsUseActualWorldContentsAndLifetime 실패 후 CampaignSharedKernel 구간 비정상 종료. SessionPickupWire10건은 별도 통과. |
| guide_fort_request / file_directory | 1278 당시4건 / 1285 당시3건. |
| menu_session·main_popup·file | 1277 기준선과 동일. menu_session6건은1268에서 직접 대조 없이 무관 진단으로 판단한 기록이다. |
| 실제 미션 하네스 | 옛 AGENTS 해시와 달라 passed:false가 나오는 기록이 있다. 미션 실행/종료 결과와 보호 해시 기준을 따로 확인한다. |

변경 전과 같은 실패인지 실제 기준선으로 비교한다. 과거 세부 로그·수치는 보관본4.2에 있다.

<a id="rules-verify"></a>

## 5. 검증 제외·남은 한계

- 사제 포획/희생·특정 적 공격·시나리오 승리·전체 자연 완주·사용자 직접 확인 검사는 건너뛴다. 단순 비MP 진입/대기/생산/건설 검사는 수행한다.
- 예외(1312 사용자 요청): 테스트 전용 자동 플레이(`--test-*`, [AI 사용 안내](docs/testing/cpp-autoplay-test-guide.md))로 아군 AI가 캠페인/스커미시를 진행·승리하는지 확인하는 실행은 허용된다. 이는 사람 플레이·원본 동일성 검증을 대신하지 않으며 결과는 "테스트 자동 플레이"로 구분해 기록한다.
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
9. 현재 문서의 지시가 옛 보고서의 "다음"보다 우선한다. CD 재개 및 목표 재개는 사용자 지시를 기다린다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 7. 근거·보관

[1310 전체 보관본](LEFT_JOBS.old.md#archive-20261114-through-1310)에는 정리 전 문서 전체를 원문 바이트로 보존했다. 단위1302~1310의 경과·끝난 작업 상세·AGENTS 변경 이력표·낡은 다음 지시는 이쪽으로 옮겼다. 기존 보관본은 수정하지 않았다.

주요 기준: [SOURCE_MAP](cpppj/SOURCE_MAP.md)·[디컴파일 신뢰도](docs/exe/decompile-reliability.md)·[제품](docs/exe/cpp-product-1101-20261101.md)·[미션 대응](docs/gameplay/demo-campaign-map.md)·[MCP/session dump](docs/exe/cpp-session-state-dump-20261109.md). DN/WB 요청·상태는 각 인계 문서를 따른다. AGENTS.md는 수정하지 않으며 현행1~14차·DN/WB4~14차 범위는 각 프로젝트에서 유지한다.
