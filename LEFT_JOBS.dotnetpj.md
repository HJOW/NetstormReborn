# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 현행 상태와 남은 작업

**기준 단위 0.403 · 정리 2026-11-11(HJOW-Athlon) · 다음 단위 0.404 · cpppj 기준 제품 1301(14차 QuickJS-ng v0.18.0 엔진 탑재) · dotnetpj의 마지막 cpppj 영향 검토는 제품 1247(`4d4b3a928`) · 전체 목표 미완료 — 남은 일은 [0.4](#now), 사용자 결정(D1~D5, 2026-11-11 확정)은 [0.5](#decisions)**

이 문서는 dotnetpj의 **현재 상태·남은 작업·계속 유효한 결정과 규칙**만 둔다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 이번 정리 직전의 이 문서 전체(단위 기록 0.280~0.341, 0.9의 단위별 완료 체크 목록, 옛 4~8차 번호의 R8 표)는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341)에 바이트 그대로 보관했고, 그 이전 보관본 4개와 0.279 이하 단위 기록도 같은 파일에 있다. [이식 계획](docs/dotnet-cpp-port-plan-20261016.md)의 W 번호·fixture 목록은 근거 목록으로만 쓰고, 순서·구조·완료 기준은 이 문서를 따른다.

**읽는 순서:** 0.4(남은 작업) → 0.5(확정된 결정) → 1절(cpppj 마감분 반영과 4~14차 대응표) → 필요할 때 나머지 절.

<a id="dotnet-overview"></a>

## 0. 한눈에 보기

### 0.1 진행 상황 — 실행·플레이 가능 여부와 원본과의 동일성

AGENTS.md의 진행 요약 기준(실행·플레이 가능 여부, 원본과의 유사성)으로 평가한다. dotnetpj에는 실행 경로가 둘이다([2.1](#dotnet-current-plan-20261028) 결정 5).

| 경로 | 실행·플레이 가능 여부 | 원본과의 동일성 |
| --- | --- | --- |
| **기존 경로** `Netstorm.Game`<br>(`Netstorm.exe`, 현재 배포 대상) | **실행·플레이 가능.** 메인 메뉴와 캠페인 1-1·1-2를 근사 규칙으로 플레이할 수 있다. 한국어 선택·와이드 시야 확장·테두리 없는 전체화면이 있다. 새 기능을 넣지 않는 동결 상태다(실행 오류 수정만). | **낮다.** 녹화·팬게임 기반 근사(RGBA 합성, 자체 UI 배치, 근사 세션 `BattleSession`)라 원본 동일성의 기준이 될 수 없다. 새 경로가 같은 범위를 대체하면 삭제한다. |
| **새 경로** `Netstorm.Client` + `Netstorm.Client.Host`<br>(개발 호스트, 아직 배포 안 함) | **메뉴 일부만 실행.** 타이틀, 메인 메뉴 8버튼, Help·Options(해상도·음량)·Credits(11.01 포함), 캠페인 선택, Edit 맵 목록·새 맵 이름까지다. **미션 월드가 실제 호스트에 조립되지 않아 플레이할 수 없다.** | **부품은 높지만 전체는 미확정.** cpppj를 구조 그대로 직역했고 원본 x86 fixture 234개로 부품을 대조했다(PORT_MAP L1 333행). **cpppj와의 화면 캡처 비교(L4a)는 메인 메뉴 한 장면(1024×768, 3프레임)만 했고 차이 0이다(BMP 바이트 동일, 0.380·0.381).** 첫 비교의 38,793픽셀(4.93%) 차이는 버튼 높이(0.380)와 배경 구름 Kencloud 미이식(0.381)이었고 둘 다 닫았다. 하위 메뉴 L4a는 12장면(0.381~0.388)으로 늘었다. 세션 상태 비교(L2)는 cpppj 기준선 1경로(thewarbegins Load/Start 덤프, 0.390)까지 확보했고, 새 경로 world-1 비교는 아직 0건이다(L3도 0건). 부품 대조(L1) 통과를 화면·플레이 동일성으로 합산하지 않는다. |

**이식 범위(2026-11-11 집계):** PORT_MAP은 cpppj `src`의 2,303파일 중 584파일, 약 16.8만 줄 중 약 6.0만 줄(36%)을 가리킨다. 행이 있다고 파일 전체를 옮긴 것은 아니다(예: `RawCampaignSession`은 관문·소유 상태 위주로 일부만). PORT_MAP에 없는 나머지 약 10.7만 줄(1,719파일) 중 큰 묶음은 `client/Client*`(ClientMain 등) 6.6천·`RawCampaign*` 4.9천·`Native*`(LAN) 3.7천·`Uber*` 3.7천·`Sound*` 2.7천·`RawCursor*` 1.7천·`Game*` 1.7천·`Editor*` 1.1천·`Hud*` 1.0천 줄, `o/RawAi*` 4.1천·`RawConstruction*` 3.4천·`RawPriest*` 2.4천·`RawCarrier*` 2.2천·`RawGun*` 1.8천·`RawPath*` 1.7천·`RawMission*` 1.6천·`RawBomb*` 1.4천·`RawFence*` 1.1천 줄이다(R4 조립·R5 표시·R6 규칙 범위). 집계 방법은 [1.2](#resync-scope)와 같은 PORT_MAP `cpp` 열 대조다.

### 0.2 단계별 진행 (R0~R8)

| 단계 | 내용 | 이정표 | 상태 |
| --- | --- | --- | --- |
| R0 | 기준선·동기화(cpppj 오라클, PORT_MAP, `cpp_sync.py`) | — | ✅ 완료. 단 cpppj 제품 1248~1300의 영향 재검토가 필요하다(0.4 1-2) |
| R1 | 8비트 화면 호스트·입력 큐·오라클 도구(`--ui-script`·`snapshot`·`session-dump`·`--fixed-frame-ms`) | — | ✅ 완료 |
| R2 | 그리기 원시·팔레트·글꼴 | — | ✅ 본체 완료(실자산·세션 연결은 R4·R5) |
| R3 | Gump UI와 메인 메뉴 계열 | D1 메뉴 픽셀 일치 | 🔶 위젯·호스트 대부분 이식. 캡처 비교는 메인 메뉴(차이 0, 0.381)·Credits Tell(차이 0, 0.382)·Help 팝업(차이 0, 0.383)·Options 팝업(차이 0, 0.384)·Campaign 목록(차이 0, 0.385)·Demo 목록(차이 0, 0.386)·Edit 목록(차이 0, 0.386)·About 창(차이 0, 0.387)·Resolution 펼침(차이 0, 0.387)·Credits 항목(차이 0, 0.388)·Campaign 하위(차이 0, 0.388)·Difficulty 펼침(차이 0, 0.388) 12장면을 했다. 남은 메뉴 장면은 사유와 함께 R5·R7·R8로 미뤘다(Language 자식 CJK는 R8, Multiplayer 섬 화면은 R5·3차, Edit 이동행은 R7). |
| R4 | 월드 세션 조립 | M1 시작 상태 L2 | 🔶 월드 아래층(Template·Fort·지형·섬·Player·저장소·프로세스·월드 본체)과 F3 세션의 관문·소유 상태 대부분 완료. **실제 호스트·월드 조립은 착수(미션 Fort 적재 0.391, 세션 홀더+world-1 덤프 0.392·0.393, 미션 값 채우기 0.394~0.397·0.399·0.400, 호스트 미션 진입 0.398, 마스크 라이브 연결 0.401, Zone 설치 검증 0.402, 가입 원형 연결 0.403). 월드 본체 조립은 미착수** |
| R5 | 월드 표시·카메라·HUD | D2 진입 화면 픽셀 일치 | ⬜ 미착수 |
| R6 | 플레이 입력·규칙·소리·AI | M2·M3 | ⬜ 미착수 |
| R7 | 미션 전체·Edit·설정·배포 | M4 | ⬜ 미착수(Edit 목록·`options.cfg` 저장 일부는 R3에서 선행) |
| R8 | AGENTS **4~14차**, 60/120프레임, Linux, LAN·Outpost | — | ⬜ 미착수. 즉시 요구인 11.01 표기·Credits 3항목·외부 접속 문서화는 완료, **`Copyright 2026 HJOW` 줄은 0.342에서 반영**(0.4 1-3 완료). LAN은 `Netstorm.Net` 사전 구현 일부(검사 없음) |

### 0.3 숫자로 보는 현재

| 항목 | 값 |
| --- | --- |
| 마지막 구현 커밋 | 0.403(참가자 가입 원형 연결). 직전은 0.402(로컬 Zone 설치 검증) |
| 검사 | 마지막 전체 완주는 0.345: Assets 366·Client 1,021·Core 1,886(합계 3,273, 실패 0, 백그라운드 완주). 0.330~0.341의 12단위는 집중 검사만 했다(0.330~0.333은 지시대로 검사 없는 사전 구현, 빌드 경고·오류 0과 `CppPortMapTests`만 확인). 0.342는 `ProductPresentationTests` 6건, 0.344는 `StyleTextMarkupTests` 45건, 0.345는 `RawFrameChooseTests` 7건(구 코드 변이 5건 검출)만 별도 확인했다. 0.346은 `AddRandomParentTests` 3건 통과(실패 0)로 확인했다. 0.347은 `AiDifficultyTests`·`RawAiCreateTests` 합계 21건 통과(실패 0)와 NB10 제거 변이 검출로 확인했다. 0.348은 `HtmlWindowMetadataRepairTests`·`TellFileMenusNativeTests`·`HtmlWindowFactoryNativeTests` 합계 10건 통과(실패 0)와 NB08 무력화 변이 2건 검출로 확인했다. 0.349는 `InputQueueTests`·`MissionScriptDisplayTests`·`QuickHelpWindowTests`(자산 1건 제외) 합계 14건 통과(실패 0)와 표시 분리 무력화 변이 1건 검출로 확인했다. 0.350은 `ExtendedTextTests` 12건 통과(실패 0)와 구간 제거 변이 1건 검출로 확인했다. 0.351은 `DisplayTextInputTests`·`InputQueueTests` 합계 7건 통과(실패 0)와 검증 무력화 변이 1건 검출로 확인했다. 0.352는 `KoreanTextEntryTests`·`TextEntryStorageTests` 합계 13건 통과(실패 0)와 확장 무력화 변이 2건 검출로 확인했다. 0.353은 `HtmlExtendedContentTests`·파서·공장 기존 검사 합계 10건 통과(실패 0, 자산 1건 제외)와 확장 무력화 변이 1건 검출로 확인했다. 0.354는 `SaveFortNameDialogTests` 4건 통과(실패 0)와 상한 변이 2건 검출로 확인했다. 0.355는 Core 3건·Client 3건 통과(실패 0)와 Pop·Unpop 양쪽 변이 검출로 확인했다. 0.356은 기존 검사 Client 25건·Core 11건 통과(실패 0)로 확인했다(새 코드 없음). 0.357은 `TellFileMenusNativeTests` 5건 통과(실패 0, L1 포함)와 표시 구형 변이 1건 검출로 확인했다. 0.358은 `ProductPresentationTests` 9건 통과(실패 0)와 언어 번호 변이 3건 검출로 확인했다. 0.359는 `UserDataRootTests` 3건 통과(실패 0)와 폴더명 변이 1건 검출로 확인했다. 0.360은 `ErrorLogTests`·`UserDataRootTests` 합계 12건 통과(실패 0)와 경고 임계 변이 2건 검출로 확인했다. 0.361은 `HostErrorLogTests`·`ErrorLogTests`·`UserDataRootTests` 합계 14건 통과(실패 0, 2회 반복) 확인했다. 0.362는 기존 검사 35건 통과(실패 0)로 확인했다(새 코드 없음). 0.363은 `TellMapMenusTests`·`SoloMapCatalogTests`·`SoloMapCatalogStorageTests` 합계 17건 통과(실패 0, 자산 지정)와 이동행 변이 2건 검출로 확인했다. 0.364는 기존 검사 16건 통과(실패 0)로 확인했다(새 코드 없음). 0.365는 `SaveFortDialogScenarioTests` 8건 통과(실패 0)와 기존 SaveFort·이름 확인 10건 포함 합계 18건 통과(실패 0, `NETSTORM_DATA` 지정)로 확인했고, 확정 거짓 무력화 변이 1건 검출로 확인했다. 0.366은 `ConnectionActivityRegistrationTests` 9건 통과(실패 0)로 확인했고, Native 주소 무력화 변이 1건 검출로 확인했다. 0.367은 `TellMapMenusTests` 8건 통과(실패 0)로 확인했고, 번역 전달 무력화 변이 1건 검출로 확인했다. 0.368은 기존 검사 `X86RawPopTests`·`RawPopTests`·`X86RawUnpopTests` 13건 통과(실패 0)로 확인했다(새 코드 없음). 0.369는 전체 회귀 재완주: Assets 366·Client 1,075·Core 1,934(합계 3,375, 실패 0)로 0.3 기준선을 새로 적었다. 0.370은 `main.cpp` 구간 3커밋 전수 대조로 잔여가 14차 QuickJS뿐임을 확정하고 기존 검사 18건 통과(실패 0)로 확인했다(새 코드 없음). 0.371은 SaveFort 확인 18건 통과(실패 0, `NETSTORM_DATA` 지정)로 확인했다(새 코드 없음). 0.372는 `RawMenuControls.cpp` 구간 4커밋 전수 대조로 소유자 잔여를 확정하고 기존 검사 17건 통과(실패 0)로 확인했다(새 코드 없음). 0.373은 `SquidPostPop.cpp` 구간 2커밋 전수 대조로 분배 잔여를 확정하고 기존 검사 59건 통과(실패 0)로 확인했다(새 코드 없음). 0.374는 `BitmapFont.cpp` 구간 1줄 대조로 확장 경로 부재를 확정하고 기존 검사 15건 통과(실패 0)로 확인했다(새 코드 없음). 0.375는 지연 FIFO 구간 1커밋 전수 대조로 소켓 배선 잔여를 확정하고 기존 검사 2건 통과(실패 0)로 확인했다(새 코드 없음). 0.376은 표면 저장소·월드 구간 3커밋 전수 대조로 후처리 본문·소용돌이 배관 잔여를 확정하고 기존 검사 141건 통과(실패 0)로 확인했다(새 코드 없음). 0.377은 세션 구간 13커밋 전수 대조로 수신기·난이도·스커미시 잔여를 확정하고 기존 검사 93건 통과(실패 0)로 확인했다(새 코드 없음). 0.378은 호스트 루프 구간 11커밋 실변경 대조로 차수별 잔여를 확정하고 기존 검사 4건 통과(실패 0)로 확인했다(새 코드 없음). 0.379는 영향 재검토 범위 종결(신규 `src` 변경 없음·31항목 전수 처분)을 확인했다(새 코드 없음). 0.380은 `HostStartupSceneTests`·`GumpButtonTreeTests` 합계 28건 통과(실패 0, 기대값 1건 교정 포함)와 L4a 첫 비교(메인 메뉴 3프레임)로 확인했다. 0.381은 새 검사 14건(Core `RawKencloudTests`·`RawKencloudRenderTests` 4건 + Client `ClientKencloudGlobalsTests` 10건)과 기존 `HostStartupSceneTests`·`GumpButtonTreeTests`를 합친 집중 검사, 변이 5건 검출(처음 놓친 1건은 부분 표시 검사를 더해 닫았다), 그리고 L4a 메인 메뉴 차이 0으로 확인했다그리고 전체 회귀 재완주(Assets 366·Client 1,085·Core 1,938, 합계 3,389, 실패 0, 백그라운드 10분 안팎)로 확인했다. 전체 회귀 본체는 0.381 코드 기준이다(0.369는 3,375건). 단위별 집중 검사 내역은 [보관본 0.1](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341) |
| PORT_MAP | `PORT_MAP.tsv` 691줄(자료 686행: L1 337·port 349). 마지막 추가는 0.381 `RawKencloud.cpp/.h`·`RawKencloudRender.cpp/.h`(L1)·`ClientKencloudGlobals.cpp/.h`(port) 6행. 그 앞은 0.363 `RawSoloMapList.h`·`FortSelection.h`. 0.359 `UserDataRoot.cpp/.h` → `Host/UserDataRoot.cs`. 0.351 `DisplayTextInput.cpp/.h` → `Input/DisplayTextInput.cs`. 0.350 `OriginalText.h/.cpp`(확장 부분) → `ExtendedText.cs`. 0.344~0.349의 행별 `base` 갱신은 §8 단위 기록과 재검토 문서 4절에 있다. 0.352에서 `TextEntryBuffer.cpp/.h`·`TextEntryGump.cpp/.h` 4행의 `base`를 `84db5219f`로 올렸다(확장 경계 재대조 완료, L1 유지). 0.353에서 `HtmlTextParser.cpp/.h`·`HtmlWindowFactory.cpp/.h` 4행의 `base`를 `e99c641aa`로 올렸다(확장 본문 재대조 완료, L1 유지). 0.354에서 `SaveFortNameDialog.cpp`→`SaveFortNameDialog.cs` 1행의 `base`를 `9d35b9a34`로 올렸다(시나리오 입력 재대조 완료, port 유지). 0.355에서 `RawGameAuxState.h`·`EditorMapCatalogFiles.cpp/.h`→`EditorCatalogWorkspace.cs`·`SquidPop.cpp`→`OriginalPopDisplay.cs`·`SquidUnpop.cpp`→`OriginalUnpopDisplay.cs` 5행의 `base`를 `7bf9248b3`으로 올렸다(생성자 헤더·삭제 표식·표 지원 재대조 완료, L1/port 유지). 0.356에서 변경 영향이 없는 대응행 5행(`InputEvent.cpp`→`HostInputMapper.cs`·`Mission.cpp/.h`→`GuideMissionScriptLifetime.cs` 2행·`SaveFortNameDialog.cpp`→`SaveFortName.cs`·`SquidPop.cpp`→`RawPop.cs`)의 `base`를 `8f8ddf6c2`로 올렸다(기존 검사 36건 통과·코드 변경 없음). 0.357에서 `TellFileMenus.cpp/.h` 2행의 `base`를 `a951ca7ed`로 올렸다(언어 전환·표시 부호화 재대조 완료, L1 유지). 0.358에서 `ProductPresentation.cpp/.h` 2행의 `base`를 `89498ca11`로 올렸다(언어·스커미시 항목 재대조 완료, port 유지). 0.359에서 `UserDataRoot.cpp/.h` 2행을 새로 등록했다(3건 통과·port). 0.360에서 `ErrorLog.cpp/.h` 2행을 새로 등록했다(12건 통과·port). 0.362에서 변경 영향이 없는 대응행 4행(`EditorMapCatalogFiles.cpp`→`EditorCatalogLedger.cs`·`EditorWorkspacePaths.cs`·`RawMenuHtml.inc`→`GumpModal.cs`·`HtmlWindowTextParser.cs`)의 `base`를 `996012399`으로 올렸다(기존 검사 35건 통과·코드 변경 없음). 0.363에서 `TellMapMenus.cpp` 1행의 `base`를 `fdb9a2b9b`로 올렸다(스크롤 표시 재대조 완료, L1 유지). 0.364에서 `main.cpp`→`UiScript.cs` 1행의 `base`를 `03c31cde6`으로 올렸다(`text` 연산 재대조 완료). 0.365에서 `RawMenuSaveFort.inc`→`HostSaveFortDialog.cs` 1행의 `base`를 `68a9545e2`로 올렸다(시나리오 확정·취소·오류 문맥 재대조 완료, 부분·port 유지, `HostFortNameConfirmation.cs`행은 그대로). 0.366에서 `RawConnectionActivity.cpp`→`RawConnectionActivity.cs` 1행의 `base`를 `eeae45c69`로 올렸다(Native 대여 판정 재대조 완료, port 유지, `.h`행은 그대로). 0.367에서 `RawMenuHtml.inc`→`GumpModal.cs`·`HtmlWindowTextParser.cs` 2행의 `base`를 `350641c48`로 올렸다(맵 단어 번역 배선 재대조 완료, L1/port 유지). 0.371에서 `RawMenuSaveFort.inc`→`HostFortNameConfirmation.cs` 1행의 `base`를 `68a9545e2`로 올렸다(확인행 무영향 재대조 완료, port 유지) |
| fixture | 연결 236개(0.381에서 `kencloud-v2-x86.tsv` 246행·`kencloud-render-x86.tsv` 944행을 Core에 더했다). `cpp_sync.py`는 미반영분 때문에 0종료가 아니다. `reflected-commit`·PORT_MAP floor는 `f95a8db2` 그대로다(영향 재검토를 마친 행만 올린다) |
| cpppj 기준 | 현재 cpppj는 제품 1301 / 11.01(14차 QuickJS-ng v0.18.0 엔진 탑재, LEFT_JOBS.md 0.5). dotnetpj의 마지막 영향 검토는 0.281(제품 1247, `4d4b3a928`)이다. 그 뒤 `cpppj/src` 변경 중 PORT_MAP에 있는 파일 47개([1.2](#resync-scope)). 14차 엔진 본체(`ScriptEngine`·`third_party/quickjs-ng`)는 다음 재검토 패스에 포함한다 |
| 디컴파일 | 마지막 정밀 디컴파일은 HJOW-Athlon 2026-11-03(R 전용 11-07, 같은 호스트)이다. dotnetpj 이식 근거는 cpppj 소스·fixture·문서라 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 호스트를 확인한다 |

<a id="now"></a><a id="09-바로-다음-작업"></a>

### 0.4 남은 작업 — 순서대로

체크 대상은 AI 작업이다. 자연 완주·전체 동일성·Windows 11 실기는 AI 작업이 아니며 단계 전환을 막지 않는다([2.3](#stage-criteria)).

| 순서 | 작업 | 할 일과 완료 기준 | 시작 조건 |
| --- | --- | --- | --- |
| **1-1** | **전체 회귀 완주** | ✅ 0.381 재완주: Assets 366·Client 1,085·Core 1,938(합계 3,389, 실패 0). 0.369 때(3,375)보다 새 검사 14건만 늘었다. 0.382는 집중 검사만 했다(모달 신규 1건 포함 `HostStartupSceneTests`·`GumpButtonTreeTests`·`ProductPresentationTests` 38건 통과, 실패 0). 0.383은 기존 검사 1건(`HostHelpPopupOpensAboutFromActualMenuRow`)을 3행 구성으로 고쳤고 새로 추가한 검사는 없다. 집중 묶음(`HostStartupSceneTests`·`GumpButtonTreeTests`·`ProductPresentationTests`) 38건 통과(실패 0)다. 0.384도 집중 검사만 했다(같은 묶음+`HostOptions` 41건 통과, 실패 0. 하위 메뉴 펼침 구조로 같은 검사를 펼침 경로·자식 실명령·음량 흐름 단정으로 고쳤다). 0.385는 코드 변경이 없어 검사를 새로 돌리지 않았다(L4a만). 0.386은 집중 검사만 했다(같은 묶음+TellMapMenus·SoloMapCatalog 56건 통과, 실패 0. 솔루션 빌드 경고 0). 0.387도 집중 검사만 했다(해상도 자식 10행 단정으로 고친 같은 검사 1건 포함). 0.388은 코드 변경이 없어 검사를 새로 돌리지 않았다(L4a만). Language 자식 장면은 CJK 글리프 출력(R8 6차·13차, 결정 D4)이 필요해 비교 전에 제외하고 문서에만 남겼다. 0.389도 코드 변경이 없어 검사를 새로 돌리지 않았다(탐색 실행만). 0.390도 코드 변경이 없어 검사를 새로 돌리지 않았다(L2 탐색 실행만). 0.391은 집중 검사만 했다(MissionFortLoadTests 2건+HostStartupSceneTests 25건 합계 27건 통과, 실패 0. 솔루션 빌드 경고 0). 0.392도 집중 검사만 했다(SessionStateDumpTests 7건+MissionFortLoadTests 2건+HostStartupSceneTests 25건 합계 34건 통과, 실패 0. 솔루션 빌드 경고 0). 0.393도 집중 검사만 했다(SessionStateDumpTests 10건+MissionFortLoadTests 2건+HostStartupSceneTests 25건 합계 37건 통과, 실패 0. 솔루션 빌드 경고 0). 0.394도 집중 검사만 했다(MissionPlayersLoadTests 1건+MissionFortLoadTests 2건 합계 3건 통과, 실패 0. 솔루션 빌드 경고 0). 0.395도 집중 검사만 했다(MissionPlayersLoadTests 2건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 14건 통과, 실패 0. 솔루션 빌드 경고 0). 0.396도 집중 검사만 했다(MissionPlayersLoadTests 3건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 15건 통과, 실패 0. 솔루션 빌드 경고 0). 0.397도 집중 검사만 했다(MissionPlayersLoadTests 4건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 16건 통과, 실패 0. 솔루션 빌드 경고 0). 0.398도 집중 검사만 했다(HostMissionOptionsTests 1건+HostOptionsFileTests 3건+MissionPlayersLoadTests 4건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 20건 통과, 실패 0. 솔루션 빌드 경고 0). 0.399도 집중 검사만 했다(MissionPlayersLoadTests 7건(신규 3건)+MissionFortLoadTests 2건+SessionStateDumpTests 10건+HostMissionOptionsTests 1건+HostOptionsFileTests 3건 합계 23건 통과, 실패 0. 솔루션 빌드 경고 0). 0.400도 집중 검사만 했다(MissionPlayersLoadTests 8건(신규 1건)+MissionFortLoadTests 2건+SessionStateDumpTests 10건+HostMissionOptionsTests 1건+HostOptionsFileTests 3건 합계 24건 통과, 실패 0. 솔루션 빌드 경고 0). 0.401은 라이브 진입 검증과 집중 검사다(--mission thewarbegins --handle Youpson 260프레임 실행 종료 코드 0, world-1 덤프 281행의 마스크 9행이 기준선과 바이트 동일, 연결 전 덤프는 9행 모두 0, originals 1394파일 해시 차이 0. 같은 집중 24건 통과, 실패 0. 솔루션 빌드 경고 0). 0.402는 집중 검사만 했다(LocalMissionZoneInstallTests 신규 2건 통과, 실패 0. 솔루션 빌드 경고 0). 0.403도 집중 검사만 했다(MissionPlayersLoadTests 11건(신규 3건)+MissionFortLoadTests 2건+SessionStateDumpTests 10건+HostMissionOptionsTests 1건+HostOptionsFileTests 3건 합계 27건 통과, 실패 0. 솔루션 빌드 경고 0). ✅ 0.369 재완주. `dotnet test dotnetpj/Netstorm.sln -c Release` 완주: Assets 366·Client 1,075·Core 1,934(합계 3,375, 실패 0). 0.3 기준선을 위 값으로 새로 적었다(0.345 기준선 3,273 대체). 첫 전체 실행에서 `NETSTORM_DATA`를 `originals/`로 잘못 지정해 Assets 21건·Core 1건이 실패했으나, 클론 데이터(`assets/game-data/`)로 다시 실행해 0건임을 확정했다(코드 무관·환경 원인) | 바로 |
| **1-2** | **cpppj 영향 재검토(제품 1247→HEAD)** | ✅ 게임 실행 없는 범위 종결(0.379). 정적 분류 0.343 + 재대조·신규 이식 0.344~0.367 + 구간 대조 0.368·0.370~0.378로 31항목 전수 처분(완료 22·부분 1·미착수 8)했다. `922f8abcf..HEAD`에 `cpppj/src` 신규 변경이 없어 범위가 그대로다. 남은 9건(부분 1번 14차 QuickJS·미착수 8건)은 각 단계(R1·R3·R4·R6·10단계·R8)에서 해당 파일을 건드릴 때 HEAD 기준으로 처리한다 | 1-1 뒤(병행 가능) |
| **1-3** | **11.01 카피라이트** | ✅ 0.342 완료. 새 경로 `Netstorm.Client/ProductPresentation.cs`의 `PatchCreditsBody`와 기존 경로 `MainMenuView`의 11.01 Credits 상세에 `Copyright 2026 HJOW`를 넣었다. 기준은 cpppj `client/ProductPresentation.cpp`(제품 1291)다. 이번 목표 중 게임 실행 검사는 지시에 따라 건너뛰고 빌드 경고·오류 0과 `ProductPresentationTests` 6건 통과로 확인했다 | 바로 |
| **1-4** | **오라클 갱신** | ✅ 0.380 완료. 오라클은 `cpppj/build/bin/Release/Netstorm.exe`(2026-11-11 11:20:25 빌드, SHA-256 `09042F35637459793784D46D4489B524D8E412C52294F7640FFFFDE5B2A8E996`)이며 cpppj `src` 마지막 변경 `5b4b80874`(제품 1301, 11:20:54 커밋)와 같은 빌드다(소스 변경 29초 뒤 커밋, 이후 `cpppj/src` 변경 없음). 데이터 사본 `extracted/dotnet-oracle-data/`(`originals/` 1,394파일 + 클론 `cursors/`)와 `originals/` 해시 기준선 `extracted/dotnet-r0-baseline/originals-hash-before-0380.txt`를 이 PC에 새로 만들었다(Git 제외). 이 오라클이 낡은 것으로 보이면(cpppj `src`가 더 새로 바뀌었으면) PowerShell로 다시 빌드한다. 0.382 L4a는 CD 10.72 변경(`93a589636`·`b833ecb51`)만 포함한 11-12 재빌드 오라클로 실행했고 메뉴 경로에 영향이 없어 비교가 유효하다 | 완료 |
| **2** | **R4 마무리 → M1** | ① 실제 호스트·월드 조립 ② AI 나머지(11차 배율 필드 포함) ③ 미뤄 둔 전수 재생 ④ 34경로 Load/Start 상태를 cpppj `session-dump`와 대조(L2). 세부는 [2.4](#r4-detail). **진행(0.390):** thewarbegins L2 베이스라인 확보(cpp world-1 덤프 f090 슬롯 1941·객체 1881·f240 슬롯 1943, 11-10 수치와 동일. 새 경로는 world-0 상한과 --mission 미지원 확인). 다음 조립은 0.391+ 단위로 Fort 적재부터 잇는다. **진행(0.391):** 호스트 미션 Fort 적재 연결(thewarbegins 목록이 cpp inspect-fort와 일치). 다음은 세션 홀더+world 0→1 확장이다. **진행(0.392):** 세션 홀더(`HostSession`: Kernel·난수 공유+풀 소유)+world-1 덤프(머리·Kernel·pool·slot 행) 연결. 다음은 Player·동맹·SP·Graph·장부 행과 Start 순서다. **진행(0.393):** 세션 저장소 행(player·hidden·alliances·sp·graph·totals·totalmade) 덤프 연결. 다음은 Start 순서와 미션 시작 흐름 연결이다. **진행(0.394):** 미션 값 채우기 첫 조각(MissionPlayers::Load 이식, thewarbegins 1·2번·나머지 번호 검증). 다음은 SP 초기화·세션 바인딩·AI 설정·생성이다. **진행(0.395):** 시작 SP 지정(재시드 연결+활성 참가자 Set, 참가자 값 확정). 다음은 세션 바인딩(AI 설정·생성 포함)·Start 순서와 미션 시작 흐름 연결이다. **진행(0.395):** 시작 SP 지정(재시드 연결+활성 참가자 Set, 참가자 값 확정). **진행(0.396):** 인공지능 설정 바인딩(AI 소유자 600B 채우기, 생성 시 난이도 적용은 기존 RawAiCreate 경로). 다음은 AI 생성 본체 연결·Start 순서와 미션 시작 흐름 연결이다. **진행(0.397):** 참가자·동맹 채우기(동맹 81칸 바이트 동일, 이름 기록). **진행(0.398):** 호스트 --mission 진입+세션 조립 검증(실행 호스트 world-1 덤프의 동맹·숨김 바이트 동일, SP 참가자 확정). **진행(0.399):** 인공지능 생성 본체 연결(세션 버퍼 공유 참여 모듈+이식된 RawAiCreate로 2번 brain 생성·부착 검증, 라이브 시작 흐름 연결은 월드 효과와 함께 다음 단위). **진행(0.400):** 동맹 마스크 기록(9기록 0x54 DWORD를 자기 비트+대칭 판정으로 채워 기준선과 바이트 동일 확인, 라이브 시작 흐름 연결은 Start 순서와 함께 다음 단위). **진행(0.401):** 마스크 라이브 연결(시작 흐름에 StartPlayerMasks 연결, 실행 호스트 world-1 덤프 281행의 마스크 9행이 기준선과 바이트 동일, 연결 전 9행 모두 0, originals 1394파일 해시 차이 0). **진행(0.402):** Zone 설치 검증(이식된 LocalMissionZone.Install의 단일 바인딩을 검사로 확정: 구독자 1·flags 0x08·이름·현재 상태, 로그인·Ready·라이브 연결은 다음 단위). **진행(0.403):** 가입 원형 연결(로그인 Joined의 구독자·flags·접속 주소·전송 두 순회를 세션 원형으로 연결하고 검사로 확정, Ready·라이브 연결은 다음 단위). 다음은 Start 순서다. offset·풀·슬롯·이름 바이트15·핸들 출처·구독자·flags·brain 토큰은 월드·Start와 함께 잇는다 | 1-2 뒤 |
| **3** | **R3 마무리 → 이정표 D1** | 남은 위젯·모달·대화창·목록 소유자를 옮기고([2.5](#r3-r2-detail)), 같은 ui-script로 메뉴·하위 메뉴·Help·Credits 캡처를 cpppj와 비교해 차이 0. 메뉴 기본 실행 교체는 4단계에서 미션과 함께 한다(결정 D2). **진행(0.380~0.382):** 비교 도구 [`l4a_compare.ps1`](dotnetpj/tools/l4a_compare.ps1)(양쪽 실행+비교 일괄)와 첫 장면 [`l4a/menu-main.tsv`](dotnetpj/tools/l4a/menu-main.tsv)를 만들었다. 메인 메뉴 1024×768 3프레임은 **차이 0(BMP 바이트 동일)**이다. 첫 비교의 38,793픽셀 차이는 버튼 높이(75×19→75×20, 0.380)와 구름 Kencloud 미이식(0.381: `RawKencloud`·`RawKencloudRender`·`ClientKencloudGlobals` 이식과 메인 메뉴 진입의 팔레트 전환 연결)이었다. 방법·원인·구조는 [dotnet-l4a-menu-cloud-20261111.md](docs/dotnet-l4a-menu-cloud-20261111.md)다. **0.382**에서 Credits Tell 장면 [`l4a/menu-credits.tsv`](dotnetpj/tools/l4a/menu-credits.tsv)(버튼 클릭 후 3프레임)도 **차이 0**이 됐다. 드러난 2,929픽셀 차이는 모달 Tell이 열려도 메인 메뉴 돌 버튼·배경을 계속 그리던 결함이었고, `HostMainMenu.HideWithModal`(버튼 8개+배경 자식)과 `HostGumpDrawing`의 모달 게이트로 닫았다(팝업 메뉴는 모달이 아니라 제외). **0.383**에서 Help 팝업 장면 [`l4a/menu-help.tsv`](dotnetpj/tools/l4a/menu-help.tsv)(버튼 클릭 후 3프레임)도 **차이 0**이 됐다. 드러난 7,965픽셀 차이는 Help 팝업 내용·위치 미이식(옛 1행 About 자리표시·화면 중앙 부착)이었고, 실제 3행(`General Help - ~iF1~.` 0x17·`Technical Help` 0x49·`Version` 0x95→Tell About)·앵커 부착(버튼 `baseWidth*0.5+x`·`y`)·플래그(0x04004000)로 닫았다(`UberGump.cpp` 1211~1229행·`RawMenuControls.cpp` 1042~1056행). 깊이 조정·모달 지정은 R3 잔여로 미뤘다(모달을 붙이면 0.382 게이트도 함께 손봐야 한다). **0.384**에서 Options 팝업 장면 [`l4a/menu-options.tsv`](dotnetpj/tools/l4a/menu-options.tsv)(버튼 클릭 후 3프레임)도 **차이 0**이 됐다. 드러난 차이는 3단계였다: ① 42,880픽셀(내용·위치 미이식: 옛 12행 중앙 부착) → 실제 20칸(Language·Frame Rate·Difficulty·AA·FSR·구분선 3개)·앵커 부착(버튼 `x+37.5`·`y-40`)·플래그(0x04000000)로 12,250픽셀까지 줄었다(`RawGameMenus.cpp` 107~160행). ② 소리 6행의 사용·사용 안 함(음향 장치 준비 `0x5c7b14`) → `--mute-audio` 이식(`HostOptions`)과 장치 판정(`ClientHostGame`→`HostPopupMenu`) 전달, `l4a_compare.ps1`의 dotnet 실행에도 `--mute-audio` 추가. ③ 88픽셀(자식 표시 `>` 1px: 리터럴 측정·출력 대신 실제 자식+마커 경로) → 하위 메뉴 6종을 미리 만들어 부모 행에 붙이는 원문 구조(`AddSubmenu`+펼침)로 닫았다. 자식 행은 실제 명령을 carry 하며(해상도 0x542948·음량 0x542a8c/90·언어 0x542988), 음량 선택은 기존 동작을 유지한다. **0.385**에서 Campaign 목록 장면 [`l4a/menu-campaign.tsv`](dotnetpj/tools/l4a/menu-campaign.tsv)(버튼 클릭 후 3프레임)도 첫 실행에 **차이 0**이었다(코드 변경 없음. Early·Priest·자유투쟁·국가부흥·완전승리·사용자제작·Skirmish 7행+Back). **0.386**에서 Demo 목록 장면 [l4a/menu-demo.tsv](dotnetpj/tools/l4a/menu-demo.tsv)(버튼 클릭 후 3프레임)은 첫 실행에 **차이 0**이었다(코드 변경 없음). Edit 목록 장면 [l4a/menu-edit.tsv](dotnetpj/tools/l4a/menu-edit.tsv)은 처음 **79,819픽셀(10.1%) 다름** → HostEditorSelection에서 목록 한도를 int32 최대로 올려 **차이 0**에 닫았다. 드러난 차이는 목록 50개 상한 때문이었고, cpppj는 452개를 담고 50행 창+Next 50 > 이동행으로 보여준다(UberGump.cpp 379행의 5차 무제한). **0.387**에서 About 창 장면 [l4a/menu-about.tsv](dotnetpj/tools/l4a/menu-about.tsv)(Help 팝업 Version 행 클릭 후 3프레임)은 첫 실행에 **차이 0**이었다(코드 변경 없음). Resolution 펼침 장면 [l4a/menu-resolution.tsv](dotnetpj/tools/l4a/menu-resolution.tsv)(Options 팝업 Resolution 행 클릭 후 3프레임)은 처음 **7,702픽셀(0.98%) 다름**에서 자식 10행(기존 6행+1600x900·1920x1080·FSR 2560x1440·3840x2160)으로 **차이 0**에 닫았다(근거 cpppj ScreenResolutions.h의 원본 3+호환 5+FSR 2). FSR 행·장치 판정은 R8 8차에서 잇는다. **0.388**에서 Credits 항목 장면 [l4a/menu-credits-entry.tsv](dotnetpj/tools/l4a/menu-credits-entry.tsv)(목록 첫 행 클릭 후 3프레임), Campaign 하위 장면 [l4a/menu-campaign-early.tsv](dotnetpj/tools/l4a/menu-campaign-early.tsv)(Early Missions 행 클릭 후 3프레임), Difficulty 펼침 장면 [l4a/menu-difficulty.tsv](dotnetpj/tools/l4a/menu-difficulty.tsv)(Difficulty 행 클릭 후 3프레임)이 모두 첫 실행에 **차이 0**이었다(코드 변경 없음). Language 자식은 펼침까지는 맞았으나 한국어·일본어 행 글리프가 달라 비교에서 뺐다. 외부 글리프 출력은 R8에서 하며(R8 6차·13차, 결정 D4) 장면 TSV도 그때 다시 만든다. **0.389**에서 남은 메뉴 장면을 탐색해 적용 범위를 확정했다. Multiplayer 버튼은 cpppj가 멀티섬 설정 화면(State 6, 월드 표시+R5·3차)으로 가고 dn은 Not Implemented Yet 창을 내어 비교에서 뺐다(TSV 삭제, R5·10단계에서 잇는다). Edit Next 50 이동행 선택 소비는 R7 잔여(0.363 기록)라 장면을 R7로 미뤘다. Welcome 창 장면은 양쪽 모두 창이 안 떠서(메인 메뉴와 바이트 동일) 만들지 않았다. 이정표 D1의 AI 작업은 미룬 3건을 제외하고 끝났으므로 **다음 단위 0.390**부터 0.4 순서 2(R4 실제 호스트·월드 조립)로 간다 | 1-4 뒤. 2와 병행 가능 |
| 4 | R5 → 이정표 D2, **`Netstorm.exe` 전환** | 월드 표시·카메라·HUD. 34경로 진입 직후와 N프레임 뒤 캡처 차이 0. 이 시점에 메뉴·미션 기본 실행을 새 경로로 바꾸고 `Netstorm.exe` 이름과 배포 대상을 새 호스트로 넘긴다. 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 두고 배포에서 뺀다(결정 D2) | 2·3 뒤 |
| 5 | R6 → M2·M3 | 입력·선택·커서, 게임 메뉴·우클릭 메뉴, 규칙(사제·운반·건설·생산·전투·의식·미션)·AI·소리. 고정 입력열 L3 상태 해시와 대표 장면 캡처 일치. 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로(`NetstormLegacy`)와 그 검사를 삭제한다(결정 D2·결정 5) | 4 뒤 |
| 6 | R7 → M4 | 34경로 전체·데모 반복·진행 저장·Retry/Next/Abort, Edit 전체, `options.cfg`(9-1 C·D·F 판정), 배포 스크립트 갱신과 압축 해제 실행 확인 | 5 뒤 |
| 7 | R8 — AGENTS 4~14차 | [1.3](#dotnet-r8-requirements) 표의 차수별 할 일. 결정 D1에 따라 요구사항 코드 상당 부분이 2~6 단계에서 함께 들어오므로 R8은 요구사항 전용 모듈(MCP·FSR 출력기·오류 로그·스커미시 화면·시나리오 편집기·외부 글꼴 출력·14차 QuickJS 엔진 탑재) 이식과 요구사항 검증이 중심이다. 14차는 cpppj가 아직 구현하지 않았으므로 cpppj 구현 뒤 같은 계약으로 옮긴다 | 6 뒤 |
| 8 | 60/120프레임 | cpppj F1·F2(옵션 `Frame Rate` 60/75 원본/120·양보 대기). 비행체 표시 보간은 cpppj D2를 따른다 | 7 뒤 |
| 9 | Linux(DesktopGL) | 설정·진행 저장·커스텀 맵·로그는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`(결정 D5). Windows 전용 동작은 호스트 OS 분기에만 둔다 | 8 뒤 |
| 10 | TCP/IP 로컬 멀티플레이·Outpost | cpppj 3차(M1~M7)를 로컬 네트워크만으로 옮긴다. MP 실행 검사는 cpppj처럼 사용자 확인으로 둔다 | 최후순위 |
| — | 새 제보 처리 | 사용자가 오류·원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다 | 제보 시 |

**진행 원칙:**

- **단위 크기(권고, 원하지 않으면 알려 달라):** cpppj가 1~13차를 마감해 소스가 안정됐으므로, 지금까지의 "함수 하나 + 검사 몇 건" 단위 대신 **cpppj 파일(모듈) 묶음 단위**로 옮긴다. 검증의 중심은 L2(`session-dump`)·L4a(캡처) 비교로 옮기고, 원본 fixture가 있는 모듈은 계속 L1을 연결한다. 0.330~0.341처럼 실제 조립과 무관한 관문을 하나씩 늘리는 단위는 줄인다.
- 단위마다 이 문서의 0절(0.2·0.3·0.4)과 해당 상세 절을 갱신하고, 작은 단위 기록은 [8절](#dotnet-unit-records)에 남긴 뒤 로컬 커밋한다([6.2](#dotnet-ops)).
- 실제 실행 검사에서 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 검사는 건너뛴다. MP 실행 검사는 하지 않는다. 단순 미션 진입·대기·생산·건설로 끝나는 비MP 검사는 수행한다(LEFT_JOBS.md 5.1과 같음).

<a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a>

### 0.5 사용자 결정 (2026-11-11 확정)

이번 정리에서 사용자 확인을 기다리던 항목을 사용자가 2026-11-11에 모두 정했다. **현재 결정을 기다리는 항목은 없다.** 새 결정이 필요해지면 이 절에 표로 더한다. 아래 결정 D1~D5는 이정표 D1(메뉴 픽셀 일치)·D2(진입 화면 픽셀 일치)와 다르다.

| ID | 결정 | 내용과 적용 |
| --- | --- | --- |
| **D1** | **cpppj HEAD를 요구사항 코드까지 그대로 옮긴다**(권고안) | cpppj가 원본 복원 코드 위에 같은 파일로 얹은 요구사항 코드(Options의 Difficulty·Frame Rate·Language, Campaign의 Skirmish, Edit의 Scenario 패널, 모든 파일 경로의 사용자 홈 루트, 한글·일본어 표시 부호화, Credits 카피라이트 등)를 결정 2대로 함께 옮긴다. 각 단계의 완료 판정은 먼저 **기본 설정(영어·보통 난이도·Frame Rate 75 원본·4:3·창 모드)에서 cpppj와 같음**으로 하고, 요구사항 기능 자체의 검증·마감은 R8에서 한다. AGENTS의 "일단은 기존 게임과 완전히 동일하게 동작하는 것을 목표로 개발 후 요구사항을 반영"은 이 검증 순서로 지킨다. 저장 위치는 옛 계획의 "R7까지 게임 폴더 `d/`, R8 5차에서 사용자 홈"이 아니라 처음부터 cpppj와 같은 사용자 루트다(시험은 `NETSTORM_USER_DATA` 빈 루트) |
| **D2** | **새 경로가 `Netstorm.exe` 이름을 이어받는다. 작업이 끝나면 기존 경로는 제거한다** | 적용(AI 해석): 이름과 배포 대상은 새 경로가 메뉴와 미션을 모두 맡을 수 있게 되는 R5(이정표 D2)에서 한 번에 넘긴다. 그 전에 넘기면 배포판에서 미션을 할 수 없거나, 새 메뉴에서 기존 경로 미션으로 넘기는 임시 연결이 필요하다. 넘길 때 `Netstorm.Client.Host`의 어셈블리 이름을 `Netstorm`으로, 기존 `Netstorm.Game`은 `NetstormLegacy`로 바꾸고 배포에서 뺀다. 기존 경로와 그 근사 검사는 새 경로가 1-1·1-2를 같은 수준 이상으로 진행하면(R6, 결정 5) 삭제한다. 그때까지 기존 경로는 실행 오류 수정만 한다 |
| D3 | **dotnetpj 클론에도 4차 로컬 MCP를 붙인다**(권고안) | R8 4차에서 cpppj MCP(935~937)와 같은 도구 표면(stdio JSON-RPC로 게임 내 모든 조작·상태 조회·PNG 화면 캡처·소리 포함 녹화)을 만든다. 그 전까지 dotnetpj 개발의 분석은 cpppj MCP와 `--ui-script`·`session-dump`로 한다 |
| D4 | **한국어(6차)·일본어(13차) 화면을 R8보다 앞당기지 않는다**(권고안) | 결정 D1에 따라 언어 메뉴·표시 부호화 경계는 R3 이식에 함께 들어오지만 검증은 영어로만 한다. 외부 `fonts/`의 D2Coding·Noto Sans JP 글리프 출력과 번역 본문 적재·검증은 R8에서 한다 |
| D5 | **Linux 저장 경로는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`**(권고안) | Windows의 `%USERPROFILE%\Saved Games\Netstorm\`(`Saves\`·`CustomMaps\`·`logs\`)에 대응한다. 글자대로 바꾼 `$HOME/.netstorm/Saved Games/Netstorm/...`는 쓰지 않는다 |

**이번 정리로 닫힌 옛 확인 항목:**

- **옛 0.8 7번(알려진 버그 34건의 dotnetpj 반영) — AGENTS 변경으로 해소.** `8661e1060`·`60226ea93`이 dotnetpj 후순위 범위를 4~13차로 넓혀 10차(알려진 버그)가 dotnetpj 범위에 들어왔다. dotnetpj는 34건을 따로 조사하지 않고, cpppj HEAD의 수정 10건(NB02·NB04·NB08·NB09·NB10·NB18·NB20·NB23·NB24·NB29)을 해당 파일을 옮길 때 함께 옮긴 뒤 단위 기록에 "10차 NBxx 포함"으로 적는다. cpppj가 새 수정을 넣으면(LEFT_JOBS.md D4·D5) 같은 방식으로 따라간다.
- 옛 0.8 1번(기존 경로 처리)·3번(cpppj 미복원 경로)·4번(프로젝트 배치)은 결정 5·6·3으로 확정돼 [2.1](#dotnet-current-plan-20261028)에 남긴다.

<a id="user-checks"></a>

### 0.6 사용자 확인 대기 (AI는 실행하지 않음)

| 영역 | 확인할 것 |
| --- | --- |
| 플레이 동일성(L5) | 모든 시나리오(Early Missions·Priest Training·캠페인·데모)의 자연 완주와 전체 화면·음향 동일성. 단계에 이것만 남으면 다음 단계로 넘어간다(2026-10-31 사용자 결정) |
| 기존 경로 | 1-1·1-2 근사 플레이 중 실행 오류 제보(실행 오류 수정만 한다) |
| 환경 | Windows 11 실기는 단계 전환 조건이 아니다(Windows 10에서 오류 없이 구동되면 넘어간다). 다른 모니터/DPI·실물 4K FSR·Program Files/UAC 설치는 R8 이후 |
| MP·LAN | 실제 LAN 접속과 MP 경로 동작(cpppj와 같은 사용자 확인 범위) |

## 1. cpppj 마감분 반영 (2026-11-11)

### 1.1 cpppj 현황과 dotnetpj에 주는 의미

cpppj는 제품 1300에서 AGENTS 1~13차 구현을 모두 마쳤다([LEFT_JOBS.md 0.4](LEFT_JOBS.md#stages)). cpppj에 남은 AI 작업은 CD 10.72 복원 C3~C5, M8 최종 관문 재실행, 비행체 표시 보간(F3, 선택)이다. 2026-11-11 사용자 커밋 `85551c6c2`가 **14차(QuickJS 엔진 탑재)**를 새로 더했고 dotnetpj 범위를 4~14차로 넓혔다. 14차는 cpppj도 아직 착수하지 않았다.

- **이식 기준이 안정됐다.** 그동안 cpppj가 같은 파일을 계속 고쳐 dotnetpj는 "파일을 건드릴 때 늦게 재대조"했다(0.281). 이제 한 번 재검토(0.4 1-2)한 뒤 HEAD 기준 파일 묶음 단위 이식이 가능하다.
- **요구사항(4~13차)의 해석과 계약이 cpppj 구현으로 확정됐다(14차는 cpppj 구현 뒤 확정).** dotnetpj는 같은 계약을 옮긴다([1.3](#dotnet-r8-requirements)). 옛 문서가 미정으로 둔 해석(MCP 범위·와이드 방식·사용자 홈 범위·Test Battle `.english` 처리)은 cpppj 문서를 따른다.
- **오라클 장치가 갖춰졌다.** `snapshot`(L4a)·`session-dump`(L2)·`--fixed-frame-ms`(L3)·MCP. dotnetpj 쪽 같은 장치는 0.283·0.284에서 만들었다(world 0 덤프까지, 세션 조립에 따라 행을 늘린다).
- **dotnetpj 비대상:** CD 10.72 복원은 cpppj 전용이다(AGENTS가 cpppj에만 둠). 단 cpppj가 C3에서 `Patch1078` 게이트(241파일 306곳)를 판본 분기로 바꾸면, dotnetpj가 이미 옮긴 판본 분기(PATCH/CD) 파일의 재대조가 필요하다. 그 커밋 이후의 영향 검토에서 확인한다. cpppj M8의 ZIP·검증 도구(`tools/verify_cpp_package.py`)는 dotnetpj 배포 검증(R7)의 참고 자료다.
- **cpppj 사용자 결정 중 dotnetpj에 닿는 것:** cpppj D2(비행체 보간) → dotnetpj 60/120프레임 범위, cpppj D3(일본어 본문 번역·다른 언어) → dotnetpj 13차 범위, cpppj D4·D5(알려진 버그 재현 조건·공식) → 수정되면 HEAD로 따라간다, cpppj D6(스커미시 후속) → dotnetpj 12차 범위.

<a id="resync-scope"></a>

### 1.2 영향 재검토 범위 (0.4 1-2의 입력)

| 항목 | 값 |
| --- | --- |
| 지난 검토 | 0.281: `f95a8db2..`(당시 HEAD, 제품 1247) — 자료 629행 중 458행 영향(L1 247·port 211), R4 우선 65행 목록. 재대조는 파일을 건드릴 때 하는 방식으로 미뤘다 |
| 이번 구간 | `4d4b3a928..HEAD`(제품 1247→1300): cpppj 47커밋, `cpppj/src` 184파일 변경(신규 39), 그중 PORT_MAP `cpp` 열에 있는 파일 47개 |
| 주요 변경 묶음 | M6 LAN·Native 등록(1258~1279), M7 5건(helpWord 캐시·메타 맵 커서·희생 안내·오류 로그, 1268~1273), K 한국어(1270·1276·1281), S 시나리오 편집(1277·1282~1284), B 알려진 버그 수정 10건(1285~1294·1300), L 일본어 UI(1288), 11.01 카피라이트(1291), D 난이도(1292), SK 스커미시(1293·1300), F 프레임(1296~1298), M8 배포(1274·1280·1295~1297) |
| 집계 방법 | `git diff --name-only 4d4b3a928..HEAD -- cpppj/src`와 `dotnetpj/PORT_MAP.tsv`의 `cpp` 열 교집합. 행별 `base` 이후 변경은 `git diff --name-only <base>..HEAD -- <cpp 파일>`로 본다(`cpp_sync.py --ports`는 0.147 이후 시간 초과라 쓰지 않는다) |

<a id="dotnet-r8-requirements"></a>

### 1.3 R8 — AGENTS 4~14차 차수별 대응

AGENTS(`8661e1060`·`60226ea93`·`85551c6c2`)는 cpppj의 **4~14차**를 dotnetpj 후순위 요구로 둔다(webpj와 달리 제외 차수가 없다). 옛 문서의 4~8차 번호는 7차(오류 로그) 신설로 한 칸씩 밀렸다(옛 7차 AA·FSR → 8차, 옛 8차 시나리오 편집 → 9차, 옛 9차 알려진 버그 → 10차). 각 차수의 해석·세부 계약은 cpppj 구현을 기준으로 옮긴다(결정 2). 기존 경로에 이미 있는 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식으로 다시 만든다.

| 차수 | AGENTS 요구 | cpppj 구현·근거 | cpppj 주요 소스 | dotnetpj에서 할 일 | dotnetpj 현황 |
| --- | --- | --- | --- | --- | --- |
| **4차** | 로컬 MCP(게임 내 모든 조작·화면 캡처·소리 포함 녹화), 16:9 와이드 최대 1920×1080(해상도 지원만, 그래픽 크기 스케일 조정 없음) | 935~937 [MCP](docs/exe/cpp-mcp-server-20261106.md), [와이드](docs/exe/cpp-wide-resolution-20261106.md)·[와이드 덱 배경](docs/exe/cpp-wide-deck-background-20261106.md) | `app/McpServer`, `platform/Json`·`Png`·`AudioLoopback`·`FrameRecorder`, `client/ScreenPresentation`·`ScreenResolutions.h`, 도구 `tools/mcp_cpp_smoke.py`·`mcp_cpp_record_smoke.py` | MCP는 결정 D3대로 dotnetpj에도 붙인다. 와이드는 cpppj 논리 해상도 방식으로 만들고 기존 경로의 시야 확장(`WideScreenMode`)은 교체한다 | 기존 경로에 근사 와이드만 |
| **5차** | 설정·캠페인 진행을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`에, 커스텀 맵은 기존 폴더와 `...\CustomMaps\` 양쪽에서 읽고 Edit 신규 맵은 `CustomMaps\`에 저장, Edit 목록 스크롤·개수 무제한, 구동 중 자동 생성하는 **모든** 파일·디렉터리를 사용자 홈 아래로(읽기 전용 설치에서도 동작) | 932~934 [사용자 루트](docs/exe/cpp-user-data-root-20261105.md)(`CustomMaps\d\NAME.fort`·최초 Edit의 원본 맵 복사·한 화면 50행 스크롤), 1284 [읽기 전용 설치 확인](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) | `platform/UserDataRoot`·`PortableStartup` | 결정 D1에 따라 파일 장치 이식 때 함께 옮긴다(R7 이전부터 사용자 루트). 기존 경로는 R5에서 배포에서 빠지고 R6 뒤 삭제되므로 `settings.json`은 옮기지 않는다(결정 D2). Linux는 `$HOME/.netstorm/`(결정 D5) | 새 경로 파일 장치(0.126·0.127)는 게임 폴더 기준 |
| **6차** | 한국어(D2Coding, 한국어 명칭 규칙) | K1~K5 1270~1281 [K1](docs/exe/cpp-korean-k1-merge-20261111.md)·[화면·입력](docs/exe/cpp-korean-screens-input-verification-20261111.md)·[K4/K5](docs/exe/cpp-korean-k4-k5-closure-20261111.md)·[글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md) | `client/DisplayTextInput`, 표시 전용 `EncodeDisplayText`/`DecodeDisplayText`, `lang\korean` 번역표, 외부 `fonts\` 적재 | 8비트 화면에 D2Coding 글리프를 그린다. 원문 8비트 문자열과 화면 표시 부호화를 구분한다. cpppj 배포는 저장소의 최신 D2Coding(1.4.0)을 넣으므로, 이때 [5절](#dotnet-user-decisions) 7번의 1.4.0 교체 점검을 함께 한다 | 기존 경로만(D2Coding 1.3.2) |
| **7차** | 오류/예외 처리·상세 로그 공통 구조(`...\Netstorm\logs\yyyyMMddHHmmss.log`, 원인 추적에 충분한 정보) | 1269 [오류 로그](docs/exe/cpp-error-log-20261111.md) | `platform/ErrorLog` | .NET 처리되지 않은 예외(`AppDomain.UnhandledException` 등)와 명시 오류 경로를 같은 위치·이름 규칙으로 기록한다. Linux는 `$HOME/.netstorm/logs/`(결정 D5) | 없음 |
| **8차** | 안티앨리어싱·FSR 1.0 선택(기본 꺼짐), FSR 지원 환경이면 2560×1440·3840×2160 추가(자동 FSR, 실질 1920×1080 업스케일) | 939 [AA·FSR](docs/exe/cpp-aa-fsr-20261106.md) | `client/GpuPresenter`·`FsrConstants`(Direct3D 11 후처리 출력기) | MonoGame 최종 출력 단계의 셰이더로 같은 상수·조건을 옮긴다. FSR 3.1 안은 쓰지 않는다 | 없음 |
| **9차** | Edit에서 시나리오 자체 편집(편집 모드 전용 UI: 초기 자본·테크·적 구성·동맹 등), 새 맵의 `.english` 기본 내용 자동 생성, `.english`가 없을 때 Test Battle 종료 수정 | S1~S4 1277·1282~1284 [S1](docs/exe/cpp-scenario-s1-merge-20261111.md)·[S2](docs/exe/cpp-scenario-s2-test-battle-20261111.md)·[S3](docs/exe/cpp-scenario-s3-ui-20261111.md)·[S4](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) | `client/EditorScenarioController`·`EditorScenarioFiles`·`EditorScenarioModel`·`ScenarioDocument`·`UberScenario.inc` | R7 Edit 이식 뒤 | 없음(Edit 목록·이름 모달까지만) |
| **10차** | 10.78 알려진 버그 34건 확인·수정 | 판정 34/34, 수정 10건 [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)·[NB01~NB34 목록](docs/exe/cpp-known-bugs-plan-20261109.md) | 건별(판정표 참조) | 해당 파일을 옮길 때 함께 옮기고 단위 기록에 NB 번호를 적는다. 따로 조사하지 않는다(0.5 아래) | — |
| **11차** | Options 난이도(쉬움·보통·어려움, 기본 보통). 쉬움은 적 AI 다리 건설·유닛 생성 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 주기 ×½ | 1292 [난이도](docs/exe/cpp-difficulty-d-20261111.md)(AI 설정 +0x220/224/228 배율, AI 단위 값) | `o/AiDifficulty` | 세션 AI 설정 소유자(0.306)에 배율 필드를 HEAD대로 두고, 보통이 원본과 같은지 먼저 검증한다 | 없음 |
| **12차** | 스커미시(Campaign 메뉴 항목, 자체 UI: 적 AI 수·AI별 난이도/동맹·금지 유닛 최대 5개(Outpost 포함)·시작/취소, 섬 배치·간헐천은 MP처럼 랜덤) | 1293·1300 [스커미시](docs/exe/cpp-skirmish-sk-20261111.md)(미션 시작 전 `prepareWorld`가 군도 밀도→중앙섬→참가자별 전투섬 순서로 생성) | `client/SkirmishSetup`·`SkirmishMapList.inc`·`SkirmishStartIslands.inc` | 이미 옮긴 섬 생성(중앙·전투·군도 분리, 0.156)과 간헐천 생성을 MP 시작 순서로 묶는 경로를 옮긴다. 계약은 LEFT_JOBS.md 4.1 '스커미시 월드' | 없음 |
| **13차** | 기타 언어(일본어 — 추후 추가 예정) | 일본어 UI 1288 [일본어](docs/exe/cpp-japanese-l1-l3-20261111.md)(번역표 509키·외부 Noto Sans JP·`.japanese`가 없으면 영어 본문) | 번역표·표시 부호화·외부 `fonts\NotoSansJP-*` | 6차 다음. 본문 번역 범위는 cpppj D3을 따른다 | 없음 |
| **14차** | QuickJS 지원([quickjs-ng](https://github.com/quickjs-ng/quickjs)). 일단 엔진 탑재만 하고 어느 정도까지 연동할지는 추후 결정(AGENTS `85551c6c2`, 2026-11-11 추가) | **cpppj 미착수**([LEFT_JOBS.md](LEFT_JOBS.md) 0.4에 아직 없음) | — | cpppj가 탑재 방식·노출 API·연동 범위를 정한 뒤 같은 계약으로 옮긴다(결정 D1). AI 기본안: quickjs-ng 네이티브 라이브러리를 대상 플랫폼별(Windows x64 먼저, Linux는 9단계)로 빌드해 P/Invoke로 적재하고, 글꼴처럼 exe 밖 파일로 배포 ZIP에 라이선스(MIT)와 함께 넣는다. AGENTS가 QuickJS를 지정했으므로 Jint 같은 관리형 JS 엔진으로 대체하지 않는다. 엔진이 외부 접속을 하지 않음을 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)에 적는다 | 없음 |
| 후순위 | 60/120프레임 | F1·F2 1297·1298 [계획·결과](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)(옵션 `Frame Rate` 60/75 원본/120, 양보 대기로 CPU 점유 감소·프레임 시간 동일) | `client/FrameRateOption` | 0.4 8단계 | 기존 경로에 근사 `FramePacing`만 |
| 후순위 | Linux(DesktopGL) | — | — | 0.4 9단계, 경로는 결정 D5 | 없음 |
| 최후순위 | TCP/IP 로컬 멀티플레이·Outpost(3차) | M1~M7 [LEFT_JOBS.md 1절](LEFT_JOBS.md#done) | `client/Native*`, `src/zacket`, `o/RawPacket*`·`RawConnection*` 등 | 로컬 네트워크만(`LocalNetworkPolicy` 루프백·리슨) | 사전 구현(검사 없음): `Netstorm.Net` 0.152·0.287·0.307·0.321·0.324·0.330~0.333, 세션 에디터 전송 0.293 |

**단계와 무관한 AGENTS 공통 요구:** `Netstorm.exe` 이름(결정 D2: R5에서 새 호스트로 이전), 11.01 표기·Credits 3항목(완료 0.150), `Copyright 2026 HJOW`(0.4 1-3), 외부 접속 문서화·클라이언트 유효성 검사 비활성화(완료 0.151, 새 경로를 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md) 갱신), 글꼴 exe 내장 금지·exe 옆 `fonts/`(기존 경로 준수), 배포 스크립트 `package-dotnetpj.ps1`(인자 또는 입력받은 경로·빈 값이면 취소·실행 파일과 필요 에셋 전부 포함 — 현행 준수, 구성이 바뀔 때마다 압축 해제 실행 확인).

<a id="cpppj-requests"></a>

### 1.4 cpppj에 전달할 요청 (이식 계획 8절의 남은 것)

dotnetpj 작업에서는 cpppj·LEFT_JOBS.md를 수정하지 않는다. 아래 요청은 사용자에게 전달한다. 요청 1·2(`session-dump`·고정 시각 실행)는 cpppj `c91d9b84`와 dotnetpj 0.283·0.284로 해결됐다.

| 번호 | 요청 | 상태(2026-11-11) |
| --- | --- | --- |
| 3 | fixture TSV 첫 주석 줄에 열 이름 | 남음 |
| 4 | 모듈별 연결 상태표(독립 대조만 / 실제 Session 연결 / GUI 연결) | 남음. cpppj [SOURCE_MAP](cpppj/SOURCE_MAP.md)로 대신할 수 있는지 0.4 1-2에서 확인 |
| 5 | `cpppj/tests/GraphTests.cpp:105`가 `graph-x86.tsv` 객체 열 `v[11]`(범위 밖, 객체는 정수 11개)을 읽음 | 남음(HEAD에서도 같은 줄) |
| 6 | `world.ClearProcesses()` 뒤 `FindEvent(sid,1)`가 null 대신 미등록 진단(`PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular` 꼬리, 0.305) | 남음. dotnetpj는 원본 동작 그대로 둔다 |

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a>

## 2. 현행 계획 — cpppj를 오라클로 한 구조 보존 직역 (2026-10-28 수립, 2026-11-11 갱신)

**진단(2026-10-28):** 기존 구현은 화면이 RGBA 합성이고, UI가 원본 Gump 트리가 아닌 자체 배치이며, 규칙이 근사 세션(`BattleSession`)에 있었다. 옮겨 둔 `Original` 모듈은 세션에 연결되지 않아 플레이를 바꾸지 못했고 같은 입력에서 cpppj와 비교할 수단도 없었다. 그래서 아래 구조로 다시 세웠다.

### 2.1 구조 결정 (계속 유효)

- **결정 1 — 화면은 8비트 논리 화면으로 옮긴다.** cpppj의 `Screen`·`Renderer`·팔레트·셰이프·색표·Gump Draw를 C#으로 옮긴다. 그리기 대상은 팔레트 인덱스 버퍼(`byte[]`, 행 간격 포함)와 256색 팔레트다. MonoGame은 창·입력 장치·오디오 장치·**최종 출력**만 맡는다. 최종 출력은 인덱스를 RGBA로 바꿔 텍스처를 갱신하고, cpppj `ScaleScreenPresentation`과 같은 정수 픽셀 중심 최근접 확대와 검정 여백을 쓴다. 게임 화면 경로에서는 SpriteBatch 합성·실수 알파·`RenderTarget2D` 확대를 없앤다. 9-1 E(부분 다시 그리기)는 cpppj Renderer의 dirty 경로로 해소한다.
- **결정 2 — cpppj 구조를 보존해 직역한다.** cpppj의 파일·클래스·함수 경계와 이름, 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 그대로 옮긴다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 C#도 같은 배치를 쓴다. Win32·DirectDraw·GDI·WINMM·파일 장치처럼 cpppj에서도 대체 경계인 부분만 .NET/MonoGame으로 바꾸고 차이와 이유를 한국어 주석에 적는다. Windows 전용 동작(커서 제한 등)은 호스트의 OS 분기에만 두어 Linux 작업을 막지 않는다. 요구사항 코드가 섞인 파일도 그대로 옮긴다(결정 D1).
- **결정 3 — 프로젝트를 cpppj 디렉터리와 짝지어 배치한다.**

  | 프로젝트 | 대응하는 cpppj | 내용 |
  | --- | --- | --- |
  | `Netstorm.Core`(`Original/` 등) | `src/o` | 원본 월드 코어·규칙 모듈. 기존 경로용 근사 코드(`Simulation`·`Rules`·`Bridges` 등)도 아직 같은 프로젝트에 있다 |
  | `Netstorm.Client` | `src/client` 중 플랫폼 독립 부분 | 화면 버퍼·Gump·Renderer·세션·HUD·소리 논리·Chat·Mission. MonoGame에 의존하지 않는다 |
  | `Netstorm.Client.Host` | `src/app`·`src/platform`의 대체 부분 | 새 경로 MonoGame 호스트(개발용) |
  | `Netstorm.Game` | — | 기존 경로 MonoGame 게임(동결, 결정 5) |
  | `Netstorm.Assets` | `GameAssets`·`RiftTypeTable`·`Template`·`Config*` 등 | 자료 판독. cpppj HEAD와 재대조한다 |
  | `Netstorm.Net` | `src/zacket`, 연결 계열 | 멀티플레이. 최후순위(현재 사전 구현만) |

  대응표는 [PORT_MAP.md](dotnetpj/PORT_MAP.md)·`PORT_MAP.tsv`(cpppj 파일 → C# 파일 → 이식 기준 cpppj 커밋 → 상태)이며 [cpppj/SOURCE_MAP.md](cpppj/SOURCE_MAP.md)와 짝을 이룬다.
- **결정 4 — cpppj 실행 파일을 판정 기준(오라클)으로 쓴다.** cpppj는 원본 기계어와 대조된 결과이므로 dotnetpj는 같은 입력에서 cpppj와 같아야 한다. 화면은 cpppj `--run … --ui-script <steps.tsv>`의 `snapshot` 논리 캡처(팔레트 변환 BMP)를, 상태는 헤드리스 출력(`--dump-world`·`--dump-types`·`--inspect-*`)과 ui-script `session-dump`(L2)·`--fixed-frame-ms`(L3)를 기준으로 쓴다([형식](docs/exe/cpp-session-state-dump-20261109.md)). dotnetpj 호스트도 같은 문법의 실행기와 같은 형식의 캡처·덤프를 낸다. 비교 도구는 `dotnetpj/tools/`에 둔다.
- **결정 5 — 기존 경로는 동결한 뒤 교체·삭제한다.** 기존 `BattleSession`·`FortMapViewer`·`MainMenuView` 등에는 새 기능 추가와 부분 수선을 멈춘다(실행 오류 수정만 예외). 새 경로가 메뉴와 미션을 모두 맡게 되는 R5에서 기본 실행과 `Netstorm.exe` 이름·배포 대상을 새 경로로 넘기고, 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 둔다(결정 D2). 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로와 그 검사를 삭제한다. 근사를 기대하는 기존 검사는 기대값을 고치지 않고 대체되는 코드와 함께 은퇴시키며 은퇴 목록을 단위 기록에 남긴다. 원본 fixture를 쓰는 검사는 대상 모듈을 옮긴 뒤에도 유지한다.
- **결정 6 — cpppj가 미완·보류로 둔 범위는 추측으로 채우지 않는다.** cpppj가 명시 경계·미복원·보류로 둔 동작은 dotnetpj 새 경로에서도 같은 경계로 둔다. cpppj가 명시 오류로 멈추는 미연결 경로는 같은 지점의 명시 예외·진단으로 옮긴다(예: LEFT_JOBS.md 4.1의 NoWorld 경계). 옛 "미복원 동작은 근사를 유지한다"는 동결된 기존 경로에만 남는다.

<a id="04-기존-dotnetpj-소스의-재수정-판정"></a>

### 2.2 기존 dotnetpj 소스의 재수정 판정

"교체"는 cpppj 대응 모듈을 새로 옮긴 뒤 기존 파일을 지운다는 뜻이다. "재대조"는 cpppj HEAD와 같은 입력에서 결과·오류 처리를 비교해, 같으면 유지하고 다르면 cpppj 구조로 바꾼다는 뜻이다. 아직 은퇴시킨 기존 코드·검사는 0건이다.

| 기존 dotnetpj | 판정 | 기준이 되는 cpppj | 단계 |
| --- | --- | --- | --- |
| `Game/NetstormGame`·`DisplayManager`·`PerfMeter`·`FrameWallClock`, `Core/Display/ScreenLayout`·`WideScreenMode`·`FramePacing`·`DisplaySettings` | 교체. 호스트에는 창·입력·오디오 장치·출력만 남긴다. 와이드 시야 확장은 cpppj 논리 해상도 방식으로 바꾼다. 새 경로는 cpppj와 같은 사용자 루트의 `options.cfg`를 쓴다(결정 D1). 기존 경로의 `settings.json`은 기존 경로와 함께 삭제한다 | `ClientMain`, `Screen`, `ScreenPresentation`, `ScreenResolutions.h`, `ScreenPaletteApply`, `FrameRateOption` | R1(설정은 R7) |
| `Game/UiAutomation` | 교체 | `app/main.cpp`의 `--ui-script`·`snapshot` | R1 |
| `Core/Display/OriginalGameClock`·`FrameLoopClock`, `Simulation/BattleSessionFrameClock` | 유지 후 재대조(L1 검증 있음). `ClientMain` 루프에 맞춰 새 호스트에 다시 연결 | `ClientMain`, `o/GameClock`, [main-loop.md](docs/exe/main-loop.md) | R1 |
| `Game/SpriteAnimation`·`FortMapViewer.Sprites`, `Assets/ObjectColorRemap`·`IsleColorRemap`·`ProductionTintRemap`·`MapSpriteFrames`·`StructureAnimation`·`BitmapFontAtlas`, `Core/Display/DrawOrder`·`DisplayBounds` | 교체. `DrawOrder`·`DisplayBounds`의 L1 대조는 새 모듈의 검사로 흡수 | `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `*ColorMaps`, `Palette*`, `DrawSort`, `o/SquidDisplay` | R2·R5 |
| `Game/MainMenuView`·`OriginalUiSkin`·`HelpWindow`·`OriginalFonts`·`OriginalCursor`, `Core/Rules/ButtonGump`, `Assets/CampaignMenu`·`StartupTips`·`HelpDocument`·`HelpTopics`·`InlinePicture` | 교체 | `Gump*`, `ButtonGump`·`StoneButtonGump`, `MenuGump*`, `ScrollGump*`, `TextGump`, `HtmlWindow*`·`HtmlBodyGump`·`Markup`·`StyleText*`, `QuickHelp*`, `TellTip`, `UberGump`·`UberRootGump`, `NativeMainMenuState`, `BitmapFont`, `Cursor` | R3 |
| `Assets/BitmapFont`(`.chfnt`)·`Palette`·`GifImage`·`ShapeDatabase`·`CursorBitmap`·`TaffArchive`·`XlatTable`·`XorCipher`·`WaveFile`·`PatternDecoder`·`Config*`·`Type*`·`OriginalText`·`GameFileSystem`·`GameDataLocator`·`GameResources`·`GameLanguage`·`GameUnitNames`·`TrueTypeCollection` | 재대조. L1/정적 대조 기록은 있으나 기준 시점이 10-07~10-10이다 | `GameAssets`, `RiftTypeTable`, `Template`, `Config*`, `Xlat`, `GifImage`, `BitmapFont`, `RawImage` | R0·R2 |
| `Assets/FortFile`·`FortMap` / `FortTerrainPreview`·`FortTerrainFringe`·`FortEdgeFarmPreview`·`FortIslandSupports` | Fort 판독은 재대조(`FortFile`은 연결하지 않은 옛 판독기라 R4 Fort 적재 연결 때 은퇴), 지형 미리보기 4종은 교체 | `RawFort*Load`, `RawCampaignWorldLoad`, `TerrainBuilder`, `RawTerrain*`, `Islandbuilder`·`RawIsland*`, `RawFarmCluster` | R4 |
| `Assets/MissionScript`·`MissionConditions`·`TutorialDialogScript` | 교체 | `TellSection`·`TellParameters`·`DialogScript`·`Html*`·`ScriptCommand`·`RawMissionGuide` | R3·R6 |
| `Core/Simulation/BattleSession.*`(15파일)·`BattleSessionFactory`·`GameEntity`·`GameCommands`·`SessionEvents`·`PlayerState`·`UnitProduction`·`*Animation`·`CombatImpact`·`FlyerFlight`·`MovementRate`·`UnitHeading`·`ConstructionTimes`·`TutorialStages`·`TutorialGeysers`·`FixedTimestep` | 교체 후 삭제 | `RawCampaignSession*`, `RawCampaign*Bindings`, `RawSurfaceWorld`, `State`, `Mission*`, 대응 `Raw*` | R4·R6 |
| `Core/Rules/*`(25파일: `BattleMap`·`SpotMap`·`Footprint`·`IslandOwnership`·`Energy*`·`StormPower`·`ProductionDeck`·`KnowledgeCatalog`·`PlacementCursor`·`RangeIndicator`·`SelectionMarks`·`GameCursor`·`MiniMapLayout`·`PlayerColors`·`MissionStart` 등) | 교체 후 삭제. `SpotRules`의 L1 대조는 새 해시 모듈 검사로 흡수 | `SquidHash`·`SquidSpatial`, `CanonDecoder`·`RawCanonPlacement*`, `RawEnergySupply`, `SpStore`, `RawProductionDeck`, `RawPlayerTechnology`, `RawCursor*`, `MinimapGeometry`, `RawPlayerColor`, `RawMissionStart` 등 | R4~R6 |
| `Core/Bridges/*`(13파일) | L1 대조된 `BridgeDecayRules`·`SurfaceFinder`·`BridgeSurfaceRules`·`SurfaceGraph`는 `Original` 쪽 대응 모듈로 합친다. 나머지는 교체 후 삭제 | `Bridge`, `Graph`·`RawGraph`, `SquidFinder`, `RawBridge*`, `CanonDecoder` | R4 |
| `Simulation/MissionFrameGate`·`PlayerParticipation`·`MissionParticipation`·`TotalMade`·`TutorialOne`·`NetstormRandom`·`MsvcRandom`·`SidPool` | 유지 후 재대조(L1 검증 있음). 세션 조립 때 cpppj와 같은 소유 위치로 재배선. `SidPool`은 `RawSidPool`과 하나로 정리 | `RawMissionFrameGate`, `RawPlayerParticipation`, `RawTotalMade`, `RawTutorialOne`, `CrtRandom`, `SidPool` | R0·R4 |
| `Core/Original/*` | 유지. 영향 재검토(0.4 1-2) 뒤 세션 조립에 쓴다 | 대응 `o/*` | R0·R4 |
| `Core/Audio/*`, `Game/AudioPlayer`, `FortMapViewer.Audio` | 교체. 장치 계층만 MonoGame | `Sound`·`SoundProcess`·`SoundMusic*`·`SoundSceneMusic`·`SoundDevice`·`ClientAudio` | R6 |
| `Game/FortMapViewer*.cs`(26파일) | 교체 후 삭제 | `Renderer`·HUD·커서·선택·메뉴, `RawCampaignSession`의 입력 | R5·R6 |
| `Game/SpriteBrowser` | 개발 도구로 유지하되 게임 경로와 분리 | — | 필요 시 |
| `package-dotnetpj.ps1` | 유지. 프로젝트·자산 구성이 바뀔 때마다 갱신하고 압축 해제 실행을 확인한다. 글꼴은 exe 옆 `fonts/`에 별도 파일로 넣는다. R5 전환 때 배포 대상을 새 호스트(`Netstorm.exe`)로 바꾸고 `NetstormLegacy`는 넣지 않는다(결정 D2) | `cpppj/package-cpppj.ps1`(참고) | 매 전환 |

<a id="stage-criteria"></a>

### 2.3 단계별 범위와 완료 기준 (R3~R7)

각 단계 괄호의 W 번호는 [이식 계획](docs/dotnet-cpp-port-plan-20261016.md) 5절의 근거 목록이다. R0~R2는 완료했다([7절](#dotnet-done-index)).

**단계 전환 기준(2026-10-31 사용자 결정, cpppj와 같음):** 모든 시나리오의 자연 완주와 전체 동일성은 사용자가 직접 플레이하며 확인하고, 단계에 남은 일이 이것뿐이면 다음 단계로 넘어간다. 각 단계의 L1~L4 대조는 AI가 한다. Windows 10에서 오류 없이 구동되면 Windows 11 실기 검증 없이 넘어간다. 사용자 제보는 해당 항목을 AI 작업으로 올린다.

- **R3 — Gump UI와 메인 메뉴 계열(W15) → 이정표 D1: 메뉴 화면 픽셀 일치.** 옮길 것: Gump 프레임워크 전체, 버튼·돌 버튼·메뉴·스크롤·텍스트·텍스트 입력·HTML 창·메시지·이미지·배경 GIF 위젯, `UberGump`·`UberRootGump`·`RawUberRootControls`·`NativeMainMenuState`, 로딩 화면(`MissionLoadingMessage`·`RawMenuLoading`), Tell·Dialog·StyleText·Markup·QuickHelp, 기본 커서. 범위: 메인 메뉴 8버튼, Campaign/Demo/Help/Options(해상도·음량·Window Mode)/Credits/Tips/Quit, 캠페인 목록, 브리핑 창(결정 D1에 따라 cpppj HEAD의 Difficulty·Frame Rate·Language·Skirmish 항목도 함께 옮기되 기본 설정 화면으로 비교). **완료 기준:** 같은 ui-script로 각 화면·하위 메뉴·Help 쪽·Credits 쪽을 캡처해 cpppj와 차이 0. 메뉴 기본 실행 교체는 R5에서 미션과 함께 한다(결정 D2).
- **R4 — 월드 세션 조립(W02·W03 잔여, W04~W07) → M1 시작 상태 L2.** 옮길 것: 시간·Kernel·Process·Regular, SP 저장소·보상·난수의 실제 연결, Fort 적재(객체·월드·메타·돈·소유자 마감·TypeNames·Deck), 지형·섬·농지·영역, 다리 연결, Player·기술·색, 미션 Start·로그인·타이머·Guide, AI 생성과 초기 갱신, 생산 Deck. `RawSurfaceWorld`/`RawWorldCore`와 `RawCampaignSession`의 소유 구조를 그대로 옮기고 기존 `Original` 모듈을 그 안에 끼운다. **완료 기준:** 34경로(Early6·Training9·캠페인16·데모3) 각각의 Load/Start 뒤 상태가 cpppj와 같다. cpppj 쪽은 ui-script `session-dump`(판번호 1)로 얻고 dotnetpj 호스트도 같은 형식을 낸다. 양쪽 모두 같은 `--fixed-frame-ms`와 빈 사용자 루트를 쓴다([6.1](#dotnet-commands)).
- **R5 — 월드 표시·카메라·HUD(W14 나머지, W15의 HUD) → 이정표 D2: 진입 화면 픽셀 일치.** 월드 표시(`Renderer`·`DrawSort`·`DrawSubmission`·`RawRenderCollection`·`ObjectCollection`·`TileGroupCollection`·`PreviewCollection`·`PreviewDraw`·`SquidRenderer`, 지형 바인딩, 타입별 Draw, 시각 시스템 Burning·Construction·Dissolve·FieryTail·Lightning·Shrapnel·Smoke·RenderProcess, KenCloud, 파티클, 번개 화면, Fort 팔레트 전환), 카메라(`RawCameraView`·`RawCameraCommit`·`RawCameraPresent`·`CameraHome`·`UberGumpCamera`·`RawEdgeScroll`), HUD(`UberHud`·`HudDock`·`HudNsInfo`·채팅 표시, 미니맵, `FloatingSpGump`·`ProductionHintGump`, 덱 `InventoryGump`). **완료 기준:** 34경로의 진입 직후와 N프레임 뒤 캡처가 cpppj와 차이 0. 애니메이션 장면은 같은 `--set`(maxFPS·sleepPerLoop 등)과 `--fixed-frame-ms` 조건에서 비교하고, 맞출 수 없는 장면은 비교 범위를 기록한다. 이 시점에 메뉴·미션의 기본 실행과 `Netstorm.exe` 이름·배포 대상을 새 경로로 넘긴다(결정 D2).
- **R6 — 플레이 입력·규칙·소리(W08~W13, W16, W19의 AI) → M2·M3.** 입력·선택·커서(`UserInput`·`GameWorldInput`·`RawUserPick`·`RawObjectHit`·`RawUserSelection`·`RawUserHighlight`·`RawCampaignCursor*`·`RawCursor*`), 게임 메뉴(QWASZX `RawProductionControls`·`ProductionShortcuts`, ESC 가로·하위 메뉴, Player List·F9, 우클릭 메뉴 전체, Warning·MessageBoard), 규칙(사제·경로·운반·수확, 배치·건설·생산, 전투·방어·파생 건물, 의식·신전·제단, 미션 진행·안내·승패, 공통 전략 AI `RawAi*`), 소리(소리 Process·음악 선택·흐름·장면 음악, 장치는 MonoGame). `arc-spire`·`acid-fence`·`ice-tower` fixture는 해당 몸체 클래스를 옮길 때 연결한다. **완료 기준:** 고정 입력열의 L3 상태 해시(프레임별 `session-dump`)와 대표 장면 캡처가 일치하고, 1-1의 건설 파이프라인이 원본 경로로 동작한다(M2). 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다.
- **R7 — 미션 전체·Edit·설정·배포(W17·W18, 9-1 C·D·F) → M4.** 34경로 전체, 데모 3개의 반복, 진행 저장과 다음 미션 해제, Retry/Next/Abort. Edit(편집 메뉴·섬 추가·저장·Test Battle·SaveGoMain, `.english` 등 언어 파일; Edit 제외는 webpj에만 해당). `options.cfg`를 원본의 시점·형식으로 읽고 쓰되 전체화면 전환 뒤 재실행 오류는 허용하지 않는다. 9-1 D·F를 판정하고 Windows 10에서 확인한다. 저장 위치는 cpppj와 같은 사용자 루트다(결정 D1). 배포 스크립트를 갱신하고(`Netstorm.exe`, 글꼴은 exe 옆 `fonts/`) 압축 해제한 사본에서 실행되는지 확인한다. **완료 기준:** 34경로 짧은 진입과 핵심 장면이 cpppj와 일치한다. 자연 완주(L5)는 완료 기준에 넣지 않는다.
- **R8** — [1.3](#dotnet-r8-requirements) 표.

<a id="r4-detail"></a>

### 2.4 R4 진행 상세 (0.4 2단계)

**완료 묶음** (단위별 상세는 보관본의 단위 기록과 [7절](#dotnet-done-index) 색인):

- Template/DataManager와 저장소 TS0~TS6(0.128~0.136), Fort 타입 표·적재·저장 F1·F2a~F2c-12(0.137~0.155).
- 지형·섬 생성 1·2차와 Fort 사제 준비(0.156~0.160), 일반지형 요청형(0.174), F2c 정상 생성·도장 본체(0.285), 월드 도장 훅(0.291), 지형 패턴 장면값 재생(0.297).
- 다리·섬 연결 본체와 판본별 전수 재생(0.161·0.162), 섬 수명주기·후처리 전수 재생(0.289·0.290, `islandlifecycle-x86.tsv` 2055행·`islandpostpop-x86.tsv` 1988행).
- F2d Player Core·초기화·Chat·참여·CoreData·Player 월드, 캠페인 월드 전역·적재, 서피스 삭제 분배기, 저장 나머지(0.163~0.171). Player 월드 CD 분기·후처리 순서 계약·잔여 순서 계약·가상 분배·월드 직접 조립 검사(0.298·0.299·0.303~0.305), Money 월드 공유(0.300), 공통 몸체 실패 순서(0.301), Subscriber 현재 읽기(0.302).
- 서피스 월드 기반·프로세스·호스트(0.172~0.225), 월드 본체 조립(0.226~0.264: `RawSurfaceWorld` 본체·`RunFrame`·수명 분배기 등).
- F3 세션: 껍데기·Fort 씨앗 적재·프레임/보고서 위임·섬 개수(0.265~0.271), Stub 관문 8종(0.272~0.279)의 본체 연결(0.286·0.288·0.292·0.294~0.296). 세션 소유 상태: Player 상태·Battle(0.286), outposts/temples 목록(0.288), AI 설정(0.306), 미션 시작 설정(0.313), 전투 옵션 87바이트·숨은 플레이어 뷰(0.319·0.320), 현재 인지(0.322), TYPE 표(0.327), 동맹 표 81칸(0.341). AI 관문: 생성·준비·초기화·다리 갱신·자금·다리 경로 전진(0.308~0.312·0.314). 동맹: 해제·협상 수신·무승부 마감·요청 입력(0.315~0.318). 객체 소유자 지정과 세션-월드 연결 검사(0.323·0.328), 가변 로컬 패킷 송신(0.326).
- 채팅 형식·추적·본문 조합·패킷·수신·이름 자동완성(0.334~0.339), Player 식별자(0.340), 아카이브 열거 검사(0.325).
- 재개 준비: 전체 회귀 재기준선(0.280), cpppj 영향 확인(0.281), cpppj 재반영 2건(Demo `Demoville`·Help 스크롤 화살표 색, 0.282), 고정 시각 실행·`session-dump` world 0(0.283·0.284), 전체 회귀(0.329).
- 사전 구현(검사 없음, 3차 범위): LAN TCP/UDP 루프백(0.287), 세션 에디터 전송(0.293), 로컬 미션 Zone(0.307), 연결 활성(0.321), 전투 방송(0.324), 패킷 등록 표(0.330), 연결 등록·정리·종료(0.331~0.333).

**남은 항목:**

- [ ] **실제 호스트·월드 조립:** 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 실제 호스트에 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과에 연결한다(0.66·0.68·0.69·0.75~0.79). Tutorial Init의 실제 템플릿/옵션 소유자·요새 선택·상태 실행·실제 타입 조회·호스트 단계 연결(0.105~0.110), Start의 실제 Config·Player·SP·AI·Gump 효과(0.70), `PlayerParticipation`의 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance`(0.74), 동맹 표의 `ClientPlayerRecords` 공유(0.341), 세션 TYPE 표의 메뉴·월드 연결(`HostEditorSelection`의 188 상수 교체, 0.327)이 남았다. 세션 대리자(`Bind*`) 묶음은 이 조립에서 실제 몸체에 연결한다.
- [ ] **AI 나머지:** `CrankAiMoney`·`SetAiParameter`(players·aiMemory 필요), `ResetAiForState`·`ReturnAiForState`(participation 필요), 실제 AI 몸체(`RawAiCreate.Create`의 600바이트 검사, 준비 00416eb0, 258 완료 판정 등). 11차 난이도 배율 필드(`o/AiDifficulty`, AI 설정 +0x220/224/228)는 결정 D1에 따라 HEAD와 함께 옮기고 보통이 원본과 같은지 검증한다.
- [ ] **미뤄 둔 전수 재생:** `player-world-x86.tsv` 1680행(월드 스택 하네스 필요), 0.154에서 옮기지 않은 cpp 검사(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`의 전체 월드 장면 등), 0.134~0.139에서 F3로 미룬 검사(`CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests`·`TemplateDiagnosticClientTests` 일부·`SoloMapCatalogTests`의 Tell 메타 1종·`CampaignNewFortTests`·`CampaignNewFortConfigTests`). 가상 분배 진입(`PostPopVirtual`)의 나머지 파생 분기.
- [ ] **M1 판정:** 34경로의 Load/Start 뒤 상태를 cpppj `session-dump`와 대조한다(L2). 0.390에서 thewarbegins 1경로의 cpp 기준선을 확보했다(f090·f240 덤프, 아래 0.390 기록). 새 경로 world-1 덤프와 나머지 33경로는 0.391+ 단위에서 잇는다. 0.391에서 미션 Fort 적재(스크립트+원문 읽기+Template 풀기)를 호스트 장치로 연결했다.

<a id="r3-r2-detail"></a>

### 2.5 R3·R2 잔여와 9-1 A

- [ ] **R3 잔여 → 이정표 D1:** 위젯 본체와 실제 호스트의 표시·입력 경로는 대부분 옮겼으나 R3 완료로 세지 않는다.
  - 대화창 분리(`ParseDialogPage`)와 `ConfigOverwriteAndQueueLifetime`의 남은 부분, MessageGump·대화창.
  - 맵/파일/기술 목록 owner, 실제 상위 메뉴 소유·분배, `RawMenuControls`의 전체 모달·등록 소유자와 링크·명령 소비자, Guide 우선 연결 등 기타 상위 명령·실제 맵/기술 목록 조립, HTML 그림 자산과 다른 파생·월드 통합 출력.
  - 전체 자산 조립, `ReadAssetQuickHelpType`의 TYPE/AST·요구 에너지 후처리, 음향.
  - `RawTextEntryControls`의 Console·부분 생성 정리·시스템 클립보드, Save as·숫자 창·ChatWindow, `RawBackgroundGifControls`의 노드·창 소유자.
  - 호스트 프레임·입력 큐, report·kernel-tell: 메인 메뉴 라벨 좌표는 실제 버튼에 연결했고(0.85) 하위 화면 라벨과 실제 호스트 명령 소비가 남는다.
  - 이정표 D1 메뉴 캡처 비교(오라클은 0.4 1-4 완료). 메인 메뉴 한 장면은 차이 0(0.381), 하위 메뉴 장면이 남았다.
- [ ] **R2 잔여 세션 연결:** 실자산 어댑터, 수집·정렬·kind 분배, `SpriteDraw` 클래스·그림자 alias 공유, `owner-capture` C행 480개는 R4·R5에서 잇는다.
- [ ] **9-1 A 나머지([4절](#original-parity-plan)):** 정수 틱을 실제 시각·원본 타이머로 옮기는 일을 세션 교체와 함께 처리한다.

<a id="carried-limits"></a>

### 2.6 이월한 이식 한계 (이후 단위에서 해소됐다는 기록이 없는 것만)

- 0.80: 초기 구름 카메라 원점은 임시 범위이고 실제 카메라(R5)·시간별 이동·부분 dirty는 미연결이다. 512×512 구름 파일이 없거나 크기가 다를 때의 원본 생성 fallback은 이식하지 않아 명시 오류다.
- 0.81: 반복 `--set`은 C# 호스트가 병합하고 같은 이름은 마지막 값을 쓰지만 cpppj는 마지막 묶음만 보관한다. 오라클 연결 때 계약 차이를 정리한다.
- 0.86·0.87: 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악(R6 범위)과 옵션 변경 중 재생 관리가 남았다.
- 0.93: 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례를 검증하지 않았다.
- 0.126: cpppj와의 이관 장부 교차 읽기, `MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 세션 소유자는 미이식이다(F3 범위).
- 0.127: Windows 외의 `WriteAtomic`은 write-through·재시도가 없는 준비용 `File.Move`이며 durable 검증이 아니다. `ReadOnly`의 Windows 외 동작은 관측하지 않았다. 새 파일 장치의 이름 정규화(ASCII만 접음)와 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)의 차이는 고치지 않았다. cpppj `RelativePath`는 옮기지 않았다.
- 0.128: 기존 `Netstorm.Assets.FortFile`은 연결하지 않은 이전 독립 판독기로 남아 있으며 은퇴는 R4 Fort 적재 단위에서 한다.
- 0.132: `WriteStream`은 입력을 배열로 복사하므로 입력이 같은 섹션 버퍼의 구간이면 원본의 DWORD 순방향 복사와 다를 수 있다(현재 호출처에는 없음, 소비자를 옮길 때 확인).
- 0.134: `AllocateEmptyManager`의 할당 실패(NULL) 경로는 이식하지 않았다.
- 0.135: 짝이 맞지 않는 UTF-8(옛 ANSI) 이름은 manager에서 꺼낼 때 U+FFFD로 바뀌어 왕복하지 않는다. `Dump`의 파일명은 바이트 문자열 그대로 출력된다.
- 0.136: `HostFortNameConfirmation`의 `header`·`setName` 실제 연결(현재 Template 소유자·전체 snapshot)은 F3이다. `FortCurrentNameDevices`의 두 dirty DWORD 주소 비교는 생략했다(호출자가 `ClientFortGlobals.Catalog`에 연결해야 함). cpppj는 비ASCII 이름을 CP1252→UTF-8로 이중 인코딩하는 차이가 있다.
- 0.137·0.138: `ReadSections` 검사의 `Parse` 이식은 F3(세션 표 소유)이다. `from_chars` 지수 언더플로 거부는 MSVC 동작을 확인하지 못해 옮기지 않았다. `TypeRequiredEnergyTests`·`ArcherTypeDataTests`·`CanonTypeDecoder` 계열과 세션 통합 검사는 F2·F3에서 잇는다.
- 0.152·0.208·0.220·0.230·0.268: `LocalMissionTransport`·로컬 미션 패킷·Host 전송(`WriteFormPacket`)·월드 전송(`SynchronizePeer`·`TransmitDeletion`)·세션 연결 Stub은 검사 없는 사전 구현이거나 명시 오류다. Zone·로그인·분배 검사는 F3 세션 연결 때, TCP/IP 소켓과 송수신은 3차 로컬 구현 때 `LocalNetworkPolicy`의 루프백 전용으로 더한다.
- 0.153: `AddRandom_Native288`의 대본 GameMenu 후반부(`QueueScriptCommand`·`GameMenuScriptCode`)는 R6에서 옮긴다.
- 0.154: 판본 분기 잔여(원문 근거는 있으나 fixture에서 관측되지 않음) — `RawSquidFactory.Take`·`PostCreate`의 나머지 CD 분기, `RawPop`의 무효 void CD throw·겹침 CD 항상 쓰기, `RawUnpop`의 추가 비트 CD 위치, `RawPostPop`·`RawUnpop`의 표시 지원 판본 overload 연결. 탐색기 발자국 조건은 원문(`footX<0`) 대신 기존 C# 조건(`FootX<1`)을 유지했다(기존 직접 검사 `RawFinderTests.FootprintValidationRejectsBadFeet`와 충돌, 차이로 기록). `RawPathSearch` 전체·`RawCanonPlacementPipeline`·`MakeFortRandomPlacementHooks`는 R6이다.
- 0.159: 배치 지형의 파이프라인 어댑터·통합 검사는 R6 범위로 뺐다.
- 0.176: `RawSurfaceStorage.UpdateAmbientWind`의 F3 연결이 남았다.
- 0.182: `Kernel` 진단 5종은 Client·`SquidProcess`가 필요해 옮기지 않았다.
- 0.221~0.224: 공장 HP의 소비(`TypeHitPoints`)는 R6, Create/Take 부모 진단 전달은 월드·세션 조립 때 잇는다.
- 0.242: 월드 보고서의 효과·삭제 요청 계수기 증가 연결(수명주기 쪽)은 미연결이다.
- 0.305: 정리 뒤 정기 조회 꼬리는 오라클도 실패하므로 원본 동작 그대로 두었다([1.4](#cpppj-requests) 요청 6).
- 0.330~0.341의 C# 모양 차이: 문자열은 Latin1로 풀어 string으로 전달(ASCII 범위에서 원문과 동등), C++ 약한 참조·공유 포인터는 `WeakReference`·`IDisposable` pin으로, 세션의 cpp 공유 참조는 내부 소유 배열·대리자(`Bind*`) 주입으로 옮겼다. 실제 공유 바인딩은 실제 호스트 조립에서 잇는다.

## 3. 검증 단계와 계속 유효한 결정·제약

<a id="verify-levels"></a>

### 3.1 검증 단계 L1~L5와 기록 규칙

- **L1:** 원본 x86 fixture. **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다(cpppj `session-dump`). **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다(`--fixed-frame-ms` + 프레임별 `session-dump`). **L4a:** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행 — 사용자 확인 대상이라 AI 작업과 단계 완료 기준에 넣지 않는다.
- cpppj 캡처·덤프는 오라클 SHA·스크립트·`--set` 값·`--fixed-frame-ms` 값·프레임과 함께 기록한다. 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본과 빈 사용자 루트를 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처·덤프 수와 차이, 함께 옮긴 10차 NB 수정, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC, 그 밖의 PC는 실행 전 목적 설명과 명시적 확인). 이 계획은 원본 게임을 실행할 필요가 없다.

<a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a>

### 3.2 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(AGENTS 4~14차, 화면비·60/120프레임·다국어·재실행 오류 방지) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만든다. cpppj HEAD에 섞인 요구사항 코드도 함께 옮기고 기본 설정에서 먼저 검증한다(결정 D1).
- **AGENTS.md 요구의 반영 상태** — [9.1](#agents-reflection) 표.
- **글꼴·명칭·주석:** 영어 화면은 원본 `.chfnt`, 한국어는 D2Coding, 일본어·기타 언어는 Noto Sans·Noto Sans JP다([5절](#dotnet-user-decisions) 2·7번). 글꼴은 exe(어셈블리 리소스 포함)에 내장하지 않고 배포판 `Netstorm.exe` 옆 `fonts/`에서 별도 파일로 읽으며 라이선스 파일도 함께 배포한다. 현재 기존 경로는 D2Coding 1.3.2를 출력 폴더 `fonts/`로 복사한다(`CopyToOutputDirectory`). AGENTS.md의 한국어 명칭 규칙을 따른다. 기존 주석은 대상을 고칠 때 정비한다(5절 8번).
- **cpppj 공통 제약을 준용한다.** CHEAT 항목은 Edit 편집에서만 나타난다. 공격형 공중 3종(훨리기그·맨오워·더스트 데빌)은 코드를 옮기되 실제 생산·생성·우클릭 재현과 추가 실행 관측은 하지 않는다. 연녹색 직사각형 분석은 보류한다(원본에서도 관찰됨). 대응 `.english`가 없을 때 Test Battle이 끝나는 원본 동작과 새 맵 `.english` 자동 생성은 9차(cpppj S2) 계약을 따른다. 실제 실행 검사에서 사제 포획/희생·특정 적 공격·시나리오 승리를 재현해야 하는 검사는 건너뛴다([TODO.md](TODO.md)). MP 실행 검사는 하지 않는다.
- **cpppj의 미완·경계 범위는 cpppj를 따라간다.** cpppj가 지금도 명시 경계·보류로 둔 것(LEFT_JOBS.md 4.1 NoWorld·외부 기능, 5.1 보류 분석, 5.2 기존 한계)만 dotnetpj도 경계로 둔다. 옛 목록(View/zoom, 도킹, DD 조정·확인 UI 잔여, Chat GUI 드래그, 미니맵 내용물·grid·항목별 debug, Renderer 시차 경계)은 cpppj 929 UI 마감 이전 기준이므로 0.4 1-2에서 항목별로 cpppj HEAD 구현 여부를 확인해, 구현된 것은 이식 대상으로 바꾼다. 독립 미니맵, Credits 자동 전환, Chat 입력·전달, 마나 103·배율 Draw는 이미 이식 대상이다.
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 [1.4](#cpppj-requests)와 이식 계획 8절에 적어 사용자에게 전달한다(사용자가 두 문서를 함께 정리하라고 지시한 경우는 예외).
- **외부 접속:** 새 경로의 Help 링크(R3)·시작 화면의 검증 안내·`Netstorm.Net`(3차)을 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)의 소비자 목록을 갱신한다. 구 클라이언트 검증·master/root 인터넷 서버·ping/진단은 이식해도 연결하지 않고 성공을 합성하지 않는다. 사용자가 선택하는 도움말 링크 실행과 로컬 서버/LAN 접속(3차)은 별도 경계다(0.151).
- **보호·커밋:** 원본/CD 디렉터리·AGENTS.md·루트 README.md의 "개요"는 수정하지 않는다(README.en.md "Overview"는 번역으로만). push하지 않는다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

<a id="original-parity-plan"></a>

## 4. 9-1 결정 — 의도적 차이는 원본처럼 동작하도록 바꾼다 (2026-10-10 사용자 결정)

**결정:** dotnetpj가 일부러 다르게 두었던 차이(아래 A~F)는 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 요구사항·편의는 더 이상 차이를 그대로 두는 근거가 아니다. 목표 순서 ①에 속한다.

**항목마다 밟는 순서:** ① 그 차이 때문에 원본과 달라지는 동작을 구체적으로 적는다 → ② 원본 근거(cpppj의 x86 기대값·`--dump-*` 출력·분석 문서·녹화)와 같은 입력으로 비교한다 → ③ 차이가 없거나 플레이에서 구별할 수 없으면 구조를 유지하고 근거를 이 문서에 남긴다 → ④ 구별되면 먼저 지금 구조 안에서 고치고, 안 되면 구조를 원본 방식으로 바꾼다. **비교 근거 없이 "같다"고 적지 않는다.** 2026-10-28부터 MonoGame은 창·입력·오디오 장치·최종 출력 계층으로만 쓰고 게임 화면은 8비트 논리 화면에 그린다(결정 1). cpppj가 원본 배치를 쓰는 곳은 C#도 같은 배치를 쓴다(결정 2). CD 10.72·10.82 값을 쓰지 않는 것은 기준 판본(10.78)을 따르는 것이라 원본과의 차이가 아니다.

| # | 판정 시작 시 차이(원본) | 원본과 달라질 수 있는 동작 | 처리 시점·현황 |
| --- | --- | --- | --- |
| A | 24Hz 고정 틱 + 화면 보간(원본은 고정 틱 없이 프레임마다 한 번 돌고 "지금 + 간격" 타이머를 쓴다) | 사건이 틱 경계(41.7ms)에 맞춰 일어나고, 명령이 다음 틱 처음에 실행되며, 붕괴 스캔이 틱당 33개다(원본은 프레임당 11개) | **첫 단계 완료(2026-10-11):** 명령·프레임 붕괴 스캔·정지 시각 보존, 화면 루프·숫자 시계·세션 시계를 프레임 처리로 바꾸고 원본 10.78 Scan 1,948행·Init 4행과 일치시켰다([근거](docs/dotnet-frame-parity-20261011.md)). **나머지(R4 Kernel/Regular와 함께):** 이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션의 정수 틱을 실제 시각/원본 타이머로 이행, 과도 단계의 `FrameSeconds`·`Seconds`·8틱 상한·정지 중 보간 잔여 정리, 안내창 재개/브리핑 지연·FPS 표본/Capture·초 경계 초기화, 이벤트 Tick·RunTicks 기반 검사 이전. A 전체 완료로 처리하지 않는다 |
| B | 입력을 MonoGame으로 읽는다(원본은 사건 큐 40칸, 넘치면 새 사건을 버리고 한 바퀴에 한 번 처리) | 빠른 연속 클릭·끌기·키 반복의 처리 순서와 누락 | R1에서 입력 큐를 만들었고 전체 입력 처리(`client/UserInput`·`InputEvent`)는 R6에서 옮긴다 |
| C | 자체 `settings.json`을 쓰고 `options.cfg`는 읽기만(원본은 `d/options.cfg`를 시작·변경 시 저장하고 `fullscreenStateFile.dat`를 만들고 지운다) | 옵션 저장 시점과 위치 | R7. 새 경로는 0.114에서 변경 옵션의 `options.cfg` 저장을 선행했다. **전체화면 전환 뒤 재실행 오류는 클론에서 발생하지 않아야 한다**(AGENTS). 위치는 cpppj와 같은 사용자 루트(결정 D1) |
| D | 그리기 직전 제한의 대기를 Sleep/Yield로 양보(원본은 바쁜 대기) | 실제 간격·흔들림·초과 대기 | R7에서 판정. cpppj F2(1298)가 양보 대기로 바꾸고 **프레임 시간이 같음**을 확인했으므로([근거](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)) 판정 때 이 근거와 같은 방식의 측정을 쓴다. 근거 없이 같다고 적지 않는다 |
| E | 매 프레임 전체를 다시 그린다(원본은 변경 사각형 100개 표로 부분만) | 화면 결과가 같아야 한다 | R1·R5에서 cpppj Renderer의 dirty 경로로 처리 |
| F | 뮤텍스·레지스트리·CD 찾기·창 크기 고정이 없다 | 창 크기 조절·중복 실행처럼 사용자가 보는 동작 | R7에서 판정. 사용자가 보는 동작만 원본대로 맞춘다([cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md)) |

판정 근거 문서: A는 [main-loop.md](docs/exe/main-loop.md) 7절·[cpp-process-reconstruction.md](docs/exe/cpp-process-reconstruction.md)·`process-x86.tsv`·`bridgedecay-x86.tsv`의 Scan 행, B는 [cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) 6절, C는 [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md)와 options x86 기대값 16개, D는 main-loop.md 4·7절, E는 [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md). 옛 9절 표·옛 순서 제안은 [보관 파일](LEFT_JOBS.dotnetpj.old.md)에 있다.

<a id="dotnet-user-decisions"></a>

## 5. 사용자 결정·확인 기록 (옛 10절, 계속 유효한 것)

번호는 옛 10절 번호다. 해결·구현이 끝난 1·3·4번은 보관 파일에 있다. 2026-11-11에 확정한 D1~D5는 [0.5](#decisions)다.

2. **영어 UI 글꼴(2026-10-10 사용자 결정):** 판단 기준은 선택한 언어다. 영어이면 UI 글자를 원본 비트맵 글꼴(`.chfnt`)로 그리고 폭도 그 글꼴로 잰다. 한국어이면 D2Coding을 쓴다. 영어 화면에 원본 글꼴에 없는 글자(한글 등)가 섞이면 D2Coding으로 대신 그린다(에이전트 해석). 새 경로는 R2·R3의 `BitmapFont`·Gump 글리프로 영어를 그리고, 한국어는 R8(6차)에서 8비트 화면용으로 다시 만든다(결정 D4). 글꼴 파일은 exe 옆 `fonts/`에서 읽는다(AGENTS `75e4eed9`).
5. **"원본과 완전히 동일한 동작"과 의도적 차이(2026-10-10 사용자 결정):** 원본처럼 동작하도록 바꾼다. 구조를 둔 채 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다. 항목별 판정은 [4절](#original-parity-plan).
6. **이미 들어간 요구사항 기능(2026-10-10, 에이전트 해석):** 한국어 선택·와이드 화면 시야 확장·테두리 없는 전체화면 창은 되돌리지 않는다. 기본 실행(영어·4:3)에서 원본과 같은 동작을 먼저 맞추고, 새 경로로 옮길 때 cpppj 방식으로 다시 만든다.
7. **D2Coding 1.4.0 교체(2026-10-10 사용자 결정): 중요도가 낮다.** 당분간 1.3.2를 쓰고, 글꼴 코드를 고칠 일이 있거나 여유가 있을 때 한다. 교체 때 확인할 것: 코드 3곳의 파일명(`Netstorm.Game.csproj`·`NetstormGame.cs`·`TrueTypeCollectionTests.cs`), 배포 스크립트의 글꼴 경로와 고지문, TTC face 순서·글자 폭·줄 높이, `TrueTypeCollectionTests` 기대값, 한국어 화면 캡처, README 두 개의 파일명. 새 경로의 6차 작업은 글꼴 코드를 새로 만드는 일이라 이때 cpppj와 같은 최신 D2Coding으로 맞추는 것을 기본으로 한다([1.3](#dotnet-r8-requirements) 6차).
8. **기존 코드 주석 정비(2026-10-10 사용자 결정):** 주석이 설명하는 대상이 수정될 때 새 규칙(설명·사용법 먼저, 추가/수정 이력은 뒤)에 맞춘다. 고치지 않는 대상의 주석을 일괄 정리하지 않는다.
9. **cpppj 미복원 동작(2026-10-16, 에이전트 기본값):** 동결된 기존 경로에서만 근사를 유지하고 [core-rules.md](docs/core-rules.md)의 근사 목록에 "원본 근거 없음"으로 등록한다. 새 경로는 결정 6.
10. **원본 월드 코어 도입(2026-10-16, 에이전트 판단):** 5번 결정의 "구조 자체를 원본처럼 바꾼다"에 해당한다([이식 계획 4절](docs/dotnet-cpp-port-plan-20261016.md#4-구조-결정-c-원본-월드-코어-도입)). 원본 월드 코어는 R4에서 세션에 조립한다.
11. **원본 월드 코어의 저장 방식(2026-10-16, 에이전트 판단):** 코어 저장소는 슬롯 50바이트를 그대로 둔다(W02~W03 fixture가 슬롯 전체 바이트를 대조하고 파생 생성자 쓰기가 필드 이름 없는 오프셋이라 의미 모델로는 검증할 수 없다). 가상 표 주소는 클래스 식별 값으로만 쓴다. 결정 2로 범위를 넓혀, cpppj가 원본 배치를 쓰는 곳은 세션·화면 쪽도 같은 배치를 쓴다.
12. **Template 저장소 이식 방식(2026-11-01 사용자 결정):** 계획서 기본값 네 가지(`RawSectionBuffer`, `RiftTypeTable` 직역, `FortTemplate` 위치는 F1에서 확정, 저장소 cpppj 대조 대신 fort 433개 왕복 검사)로 진행했다([계획](docs/dotnet-template-storage-plan-20261101.md) 7절).

## 6. 검증 명령과 운영 규칙

<a id="dotnet-commands"></a>

### 6.1 검증 명령

```powershell
# 저장소 루트에서 기준선과 단계별 검증 (헤드리스)
dotnet build dotnetpj/Netstorm.sln -c Release
dotnet test dotnetpj/Netstorm.sln -c Release

# cpppj 콘솔 출력 대조 (헤드리스 명령만 사용. --run 등 게임/창 실행은 해당 PC의 규칙을 별도로 확인)
cpppj/build/bin/Release/Netstorm.exe --dump-types originals
cpppj/build/bin/Release/Netstorm.exe --dump-world originals thewarbegins
cpppj/build/bin/Release/Netstorm.exe --inspect-bridges originals

# cpppj 반영 범위 확인. 저장소 루트에서 실행. 마지막 반영 커밋은 cpp-fixture-manifest.tsv 의 reflected-commit
python dotnetpj/tools/cpp_sync.py                                    # 보고 (읽기 전용, 연결 fixture 가 어긋나면 종료 코드 1)
python dotnetpj/tools/cpp_sync.py --update --reflect HEAD            # HEAD까지 영향 검토·반영을 모두 마친 경우에만 갱신
python dotnetpj/tools/cpp_sync.py --update --add 이름:Core           # 새 fixture 를 연결할 때 목록에 등록 (Assets 도 가능)

# 영향 재검토(0.4 1-2): 지난 검토 이후 바뀐 cpppj 소스와 PORT_MAP 의 교집합
git diff --name-only 4d4b3a928..HEAD -- cpppj/src
```

```powershell
# cpppj 오라클 캡처. 창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다.
# 데이터는 originals/ 대신 extracted/ 아래 사본을 쓴다. steps.tsv 한 줄은 "프레임<TAB>동작<TAB>인자"다
# (click·click-if·right·key·esc·pointer·snapshot·session-dump 등. snapshot·session-dump 인자는 저장할 파일 경로).
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --window --no-audio --frames 120 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script steps.tsv --ui-report states.tsv            # 미션 직행은 --mission thewarbegins 등을 더한다
# L4a 일괄 도구(0.380): 같은 ui-script 로 cpppj 와 dotnetpj 호스트를 각각 돌려 캡처를 만들고 바로 비교한다. 종료 코드 0 일치·1 차이·2 오류.
#   steps 템플릿의 캡처 경로 자리에 {OUT} 을 쓴다(예: dotnetpj/tools/l4a/menu-main.tsv). 캡처는 extracted/dotnet-oracle-captures/<Name>/cpp, dn 에 남는다.
#   powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj/tools/l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-main.tsv -Name menu-main -Frames 130 [-Build] [-SkipCpp]
#   창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다. 실행 전후 originals/ 해시를 비교해 달라지면 실패로 끝낸다.
# 캡처 비교 도구: python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png]  /  --dir 폴더A 폴더B (종료 코드 0 일치·1 차이·2 입력 오류)
# dotnetpj 쪽의 같은 실행 인자(--frames/--ui-script/--ui-report/--set/--fixed-frame-ms)는 Netstorm.Client.Host 가 받는다.
# PowerShell 5.1의 Start-Process -PassThru 는 종료 코드를 비우므로 `& exe 인자; $LASTEXITCODE` 로 읽는다.

# cpppj 실제 Session 상태 덤프(L2)·고정 시각 실행(L3).
# 결정론 비교에는 가상 시계(--fixed-frame-ms)와 빈 사용자 루트를 쓴다. 기본 OS 시계 두 실행은 게임 시각부터 달라 비교할 수 없다.
# WelcomeGiven=1 이 없으면 환영 창 때문에 메뉴 클릭이 실패하고, --mission 뒤 'Play Mission' 은 늦게 나타나므로 click-if 를 여러 프레임에 둔다.
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --mission thewarbegins --fixed-frame-ms 25 --window --mute-audio --frames 250 `
  --set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0" --ui-script steps.tsv --ui-report report.tsv
# steps.tsv 예: "5<TAB>click-if<TAB>Play Mission", "90<TAB>session-dump<TAB><출력 경로>.tsv"
# 덤프 형식(판번호 1, ASCII·LF·탭, float/double 은 비트 정수): docs/exe/cpp-session-state-dump-20261109.md
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 AGENTS.md의 확인·예외 규칙을 따른다.
- 원본 x86 기대값 대조만 돌릴 때: `dotnet test -c Release --filter "FullyQualifiedName~X86"`. 새 기대값 파일은 `tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj`에 링크로 추가하고(사본 금지) `cpp_sync.py --update --add`로 SHA-256 목록에 등록한다(빠뜨리면 `EveryLinkedFixtureIsRegistered`가 실패). 한 fixture는 한 소비자 프로젝트에서 검사한다(`config-x86.tsv`는 Assets.Tests). 포트 맵의 `port` 행은 fixture 대조가 없는 이식이며 L1이 아니다.
- **변이 검사:** 대표 변이를 수동으로 넣고 제품·검사 빌드 뒤 `--no-build` 집중 실행으로 검출을 확인한 다음 원복하고 집중 검사를 다시 통과시킨다. 원복 뒤 증분 빌드가 옛 결과를 쓰지 않게 필요하면 `--no-incremental`로 다시 빌드한다. 사본(`extracted/dn-mutation-ts0`)을 쓸 때는 다중 행 패턴 들여쓰기가 틀리면 일치 0개로 조용히 건너뛰므로 개수 assert로 잡고 복사한 파일은 반드시 `touch`한다. 무한 루프로 걸리는 변이도 검출로 기록한다.
- **전체 검사 완주:** 단위마다 집중 검사로 넘어가더라도 20단위 또는 한 작업일마다 솔루션 전체 검사를 끝까지 돌려 0.3의 기준선을 새로 적는다.
- **오라클 자료:** 데이터 사본 `extracted/dotnet-oracle-data/`, 캡처 `extracted/dotnet-oracle-captures/`, 기준선 `extracted/dotnet-r0-baseline/`은 Git 제외이며 작업 PC에만 있다. 실행 전후 `originals/` 해시를 비교한다.
- **options.cfg 변경 판별:** 호스트나 원본 게임을 실행한 뒤 추적 파일 `assets/game-data/d/options.cfg`가 바뀌어 있으면 누구의 실행 때문인지 먼저 가린다. 내가 만든 변경일 때만 되돌린다.

<a id="dotnet-ops"></a>

### 6.2 착수·커밋 운영 규칙

1. **착수:** AGENTS.md·이 문서·LEFT_JOBS.md를 다시 읽고 `git status`/`git log`로 다른 세션 변경을 확인한다. dotnetpj는 디컴파일 산출물을 쓰지 않으므로 호스트 확인은 산출물을 직접 읽을 때만 한다.
2. **단위 종료:** 머리말의 기준 단위·다음 단위 번호, 0.2·0.3·0.4와 해당 상세 절을 갱신한다. 문서가 없는 작은 단위만 [8절](#dotnet-unit-records)에 짧게 적고, 긴 경과는 `docs/dotnet-*.md` 단위 문서에 둔다. 끝난 상세가 길어지면 보관 파일로 옮기고 [7절](#dotnet-done-index) 색인만 갱신한다.
3. **커밋:** 인계 작성 뒤 **이 단위의 파일만** 경로로 지정해 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다(끝에 Co-Authored-By 트레일러). 커밋 번호는 다른 PC·세션과 겹칠 수 있으므로 `git log`로 당일 마지막 번호를 확인해 이어 쓴다. 같은 작업 트리에서 다른 세션이 cpppj·`LEFT_JOBS.md`를 자주 고치므로 CP/WB 파일·stage를 건드리지 않는다. **push하지 않는다.**
4. **줄바꿈:** `dotnetpj`·`docs`와 `LEFT_JOBS.dotnetpj*.md`는 저장소에 LF로 저장된다(이 문서는 작업 트리도 LF, 보관 파일은 작업 트리 CRLF). 기본 `autocrlf`로 add하고 `-c core.autocrlf=false`를 쓰지 않는다. CRLF 작업 트리 파일은 Edit 도구 대신 정확히 1회 일치하는 바이트 치환으로 고친다.
5. **빌드 환경:** Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 실패하므로 PowerShell로 실행한다. `cpp_sync.py` 보고가 cp949 콘솔에서 깨지면 `PYTHONUTF8=1` 또는 `python -X utf8`. 오라클에는 게임 실행 파일만 필요하며 cpppj 전체 빌드는 검사 대상까지 포함해 오래 걸린다.
6. **주석:** 새 코드의 네임스페이스·상수·함수·구조체·클래스·멤버함수·멤버변수·반복문에 한국어 주석을 단다. 주 내용은 설명과 사용법이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C#에서는 `/// <summary>`에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고 원본 주소·근거 문서·이력은 summary 뒤쪽이나 `<remarks>`에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 원본 주소는 10.78 주소와 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.

## 7. 참고 문서와 완료 범위 색인

<a id="dotnet-done-index"></a>

### 7.1 완료 범위 색인

단위별 상세는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)와 아래 문서에 있다. 마지막 열은 현행 계획에서의 처리다([2.2](#04-기존-dotnetpj-소스의-재수정-판정)). 새 경로의 진행도는 Client 검사와 2.3의 실제 완료 기준을 함께 본다. Assets·Core의 기존 경로 검사 수를 새 경로 진행도로 세지 않는다.

| 기간·단위 | 완료 범위 | 근거 문서 | 처리 |
| --- | --- | --- | --- |
| 2026-09-27~10-05 | MonoGame 기반 초기 C# 구현(메인 메뉴·캠페인 1-1/1-2·다리·사제·전투·희생·덱 등 녹화·팬게임 기반), `dotnetpj/` 이동 | [보관 파일 14절](LEFT_JOBS.dotnetpj.old.md#legacy-dotnet-history) | 기존 경로(대부분 교체) |
| 2026-10-07~10-13 | 자료 계층 정정, 다리·표면·그래프·SID·점유·표시 경계 계산, `.chfnt`·커서 표·배경 GIF 색·미션 시작 값, 9-1 A 첫 단계·화면 루프·시계, 도움말·안내창·단계 프레임, 한국어 명칭 | [10-07](docs/dotnet-reconstruction-20261007.md)·[10-09](docs/dotnet-reconstruction-20261009.md)·[10-10](docs/dotnet-reconstruction-20261010.md)·[frame-parity](docs/dotnet-frame-parity-20261011.md)·[loop-parity](docs/dotnet-loop-parity-20261012.md)·[clock-kernel](docs/dotnet-clock-kernel-20261012.md)·[session-clock](docs/dotnet-session-clock-20261012.md)·[dialog-frames](docs/dotnet-dialog-frames-20261013.md)·[stage-frames](docs/dotnet-stage-frames-20261013.md) | 판독기·시계는 재대조, 계산은 `Original`로 흡수, 나머지 교체 |
| 2026-10-15~10-23 | 감사 D01~D16, W01 동기화 도구, W02·W03 원본 월드 코어(슬롯 풀·생성자·Pop/Unpop/PostPop·그래프·삭제·Damageable·Form·내용물 등), W05 선행(SP 저장소·보상), 배포 스크립트 | [감사](docs/dotnet-cpp-audit-20261015.md)·[이식 계획](docs/dotnet-cpp-port-plan-20261016.md)·[W02 첫 조각](docs/dotnet-original-core-20261016.md) 외 `docs/dotnet-original-core-*.md`·[SP 저장소·보상](docs/dotnet-original-core-spstore-reward-20261023.md) | 유지. R4 세션 조립 |
| R0~R2 (10-28~29, 0.13~0.28) | cpppj 오라클·재대조 영향표·PORT_MAP, `Netstorm.Client` 8비트 Screen·입력 큐·캡처·스크립트 도구·`cpp_frame_compare.py`·MonoGame 출력 호스트, R0-4 코어 보강, 팔레트·VFX·색 표·SpriteDraw | [영향표](docs/dotnet-cpp-resync-20261028.md)·[PORT_MAP.md](dotnetpj/PORT_MAP.md)·[R1 문서](docs/dotnet-client-screen-20261028.md)·`docs/dotnet-r04-*.md`·[SpriteDraw](docs/dotnet-spritedraw-20261029.md) | 새 경로 |
| R3a~R3k (10-29~31, 0.29~0.65) | Gump 기반·트리·수명·사건/입력·프레임·클립·배경·글꼴·버튼, StyleText·Slider·메뉴, 문자 입력·Console·Image·MetaImage, 배경 GIF·HTML·도움말·스크립트 명령·Config·MissionScript | `docs/dotnet-gump-*.md`·`docs/dotnet-style*.md`·`docs/dotnet-menu-*.md`·`docs/dotnet-text-entry-*.md`·`docs/dotnet-html-*.md`·`docs/dotnet-quick-help-*.md`·`docs/dotnet-config-*.md` 등 | 새 경로 |
| 0.66~0.127 (10-31~11-01) | 미션 조립 부품, 호스트 입력·타이틀·메인 메뉴, 공통 트리·HTML 창, 기존 경로 `Netstorm.exe` 이름·배포(0.96), 통합 표시·메뉴 입력, Tutorial Init·초기화, 호스트 Help·Options·옵션 저장, Edit 맵 목록·이름·작업본 | [보관 0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)·[보관 0.127~0.154](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) | 새 경로(L1 일부). 실제 조립은 R4 |
| 0.128~0.155 (11-01~07) | DataManager·Template 저장소 TS0~TS6, Fort 타입 표·적재·저장 F1~F2c-12, 11.01 표기·Credits(0.150), 외부 접속 문서화(0.151), 멀티플레이 사전 구현(0.152) | [보관 0.127~0.154](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)·[Template 저장소 계획](docs/dotnet-template-storage-plan-20261101.md)·[외부 접속](docs/dotnet-external-endpoints-20261103.md) | 대부분 L1, 세션 연결은 F3 |
| 0.155~0.279 (11-07~10) | 지형·섬 생성, 다리 연결, F2d Player·캠페인 월드, 서피스 월드 기반·프로세스·호스트, 월드 본체 조립, F3 세션 껍데기·Stub | [보관 0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) | 새 경로(L1·port) |
| 0.280~0.341 (11-10~11) | 재개 준비(재기준선·영향 확인·재반영 2건·`session-dump`·고정 시각), F2c 정상 생성, 섬 전수 재생, F3 본체·세션 소유 상태·AI 관문·동맹·채팅·Player 식별자, LAN 사전 구현, 전체 회귀(0.329) | [보관 0.280~0.341](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341), 요약 [2.4](#r4-detail) | 새 경로(port 위주) |

### 7.2 참고 문서

| 주제 | 문서 | cpppj 기준 구현 |
| --- | --- | --- |
| **cpppj 현행 상태·계약·사용자 결정** | [LEFT_JOBS.md](LEFT_JOBS.md)(0.4 단계별 현황·1절 완료 묶음·4.1 계약·5절 실행 제외) | — |
| cpppj 실제 Session 덤프·고정 시각 실행(L2·L3 오라클) | [cpp-session-state-dump-20261109.md](docs/exe/cpp-session-state-dump-20261109.md) | `client/SessionStateDump`, `client/ClientTimeSource`, `app/main.cpp` |
| cpppj의 다른 프로젝트 요청 처리·재반영 대상 | [cpp-webpj-requests-20261109.md](docs/exe/cpp-webpj-requests-20261109.md) | `client/UberGump.cpp`, `client/RawMenuControls` |
| **AGENTS 4~14차의 cpppj 구현**(14차는 cpppj 미착수) | [1.3](#dotnet-r8-requirements) 표의 근거 열 | 1.3 표의 소스 열 |
| cpppj 11.01 표기·Credits·카피라이트 / 외부 접속 목록 | [cpp-product-1101-20261101.md](docs/exe/cpp-product-1101-20261101.md), [cpp-external-services-20261101.md](docs/exe/cpp-external-services-20261101.md) | `client/ProductPresentation.cpp` |
| cpppj CD 10.72(dotnetpj 비대상, 게이트 영향만) | [cpp-cd1072-startup-and-gates-20261111.md](docs/exe/cpp-cd1072-startup-and-gates-20261111.md) | `Patch1078` 게이트 |
| **L4a 첫 비교(메인 메뉴)·구름 Kencloud 이식** | [dotnet-l4a-menu-cloud-20261111.md](docs/dotnet-l4a-menu-cloud-20261111.md) | `o/RawKencloud`·`RawKencloudRender`, `client/ClientKencloudGlobals`·`ClientMain.cpp` 팔레트 전환 |
| 8비트 화면 출력·전체화면 호환 | [cpp-modern-fullscreen-20261027.md](docs/exe/cpp-modern-fullscreen-20261027.md), [cpp-screen-resolution-menu-20261027.md](docs/exe/cpp-screen-resolution-menu-20261027.md) | `client/Screen.cpp`, `client/ScreenPresentation.cpp`, `client/ClientMain.cpp` |
| cpppj → dotnetpj 이식 계획(W01~W20, 8절 요청) | [dotnet-cpp-port-plan-20261016.md](docs/dotnet-cpp-port-plan-20261016.md) | `client/RawCampaignSession`, `client/RawSurfaceWorld` |
| 원본 월드 코어 W02 첫 조각 | [dotnet-original-core-20261016.md](docs/dotnet-original-core-20261016.md) | `o/SidPool.cpp`, `o/SquidFactory.cpp`, `o/TypeConstructors.inc`, `o/DerivedConstructors.inc` |
| Template 저장소 이식 계획(TS0~TS6, F1~F3) | [dotnet-template-storage-plan-20261101.md](docs/dotnet-template-storage-plan-20261101.md) | `client/CampaignTemplateStorage` |
| dotnetpj 외부 접속 문서 | [dotnet-external-endpoints-20261103.md](docs/dotnet-external-endpoints-20261103.md) | — |
| 반영 점검 D01~D16 | [dotnet-cpp-audit-20261015.md](docs/dotnet-cpp-audit-20261015.md) | — |
| 표시 이름·내부 타입명 대응 | [unit-source-name-map.md](docs/unit-source-name-map.md) | `o/RiftTypeTable.cpp` |
| 데모·캠페인 대응 | [demo-campaign-map.md](docs/gameplay/demo-campaign-map.md) | `o/RawMissionStart` |
| 설정 조회·치환·저장 | [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md), [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md) | `o/Config.cpp`, `o/ConfigInterface.cpp` |
| 타입 표·요새 파일 | [cpp-fort-reconstruction.md](docs/exe/cpp-fort-reconstruction.md), [cpp-assets-reconstruction.md](docs/exe/cpp-assets-reconstruction.md) | `o/RiftTypeTable.cpp`, `o/Template.cpp` |
| 다리 표·추첨·열린 방향·수명 | [cpp-bridge-reconstruction.md](docs/exe/cpp-bridge-reconstruction.md), [bridge-pieces.md](docs/exe/bridge-pieces.md) | `o/Bridge.cpp`, `o/CanonDecoder.cpp` |
| 표면 이웃·연결·붕괴 방문 / 그래프 | [cpp-surface-reconstruction.md](docs/exe/cpp-surface-reconstruction.md), [cpp-graph-reconstruction.md](docs/exe/cpp-graph-reconstruction.md) | `o/SquidFinder.cpp`, `o/Graph.cpp`, `o/RawGraph.cpp` |
| 점유 비트·해시 / SID / 첫 등록 | [cpp-hash-reconstruction.md](docs/exe/cpp-hash-reconstruction.md), [cpp-spatial-reconstruction.md](docs/exe/cpp-spatial-reconstruction.md), [cpp-sid-reconstruction.md](docs/exe/cpp-sid-reconstruction.md), [cpp-postpop-reconstruction.md](docs/exe/cpp-postpop-reconstruction.md) | `o/SquidHash.cpp`, `o/SquidSpatial.cpp`, `o/SidPool.cpp`, `o/SquidPostPop.cpp` |
| 그리기 순서·글꼴·커서 / 메뉴·브리핑 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), [cpp-display-reconstruction.md](docs/exe/cpp-display-reconstruction.md), [cpp-menu-reconstruction.md](docs/exe/cpp-menu-reconstruction.md) | `client/Renderer.cpp`, `o/SquidDisplay.cpp`, `client/UberGump.cpp`, `client/Gump.cpp` |
| 지형·시작 값·조작 / 메인 루프 | [cpp-world-reconstruction.md](docs/exe/cpp-world-reconstruction.md), [main-loop.md](docs/exe/main-loop.md) | `o/TerrainBuilder.cpp`, `client/ClientMain.cpp` |
| 판본 비교(10.37·10.62·10.82·V12) | [cpp-reference-versions.md](docs/exe/cpp-reference-versions.md) | — |
| C# 규칙 코어의 현재 범위와 근사 목록(기존 경로) | [core-rules.md](docs/core-rules.md) | — |
| cpppj 소스 맵·디컴파일 신뢰도 | [SOURCE_MAP](cpppj/SOURCE_MAP.md), [decompile-reliability.md](docs/exe/decompile-reliability.md) | — |

<a id="dotnet-unit-records"></a>

## 8. 단위 기록 (0.342부터)

문서가 없는 작은 단위만 아래에 최신순으로 짧게 적는다. 한 기록에는 범위, 구현(새 파일·수정 파일·옮긴 cpppj 파일과 기준 커밋), C#에서 달라진 모양, 검증(집중·전체·변이·L2/L4a 비교), PORT_MAP·fixture 변화, 함께 옮긴 10차 NB 수정, 보호·실행·외부 접속 관점, 남은 것을 적는다([3.1](#verify-levels)). 0.11~0.341은 보관 파일에 있다: [0.11~0.65](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073), [0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126), [0.127~0.153](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154), [0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279), [0.280~0.341](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341).

(다음 단위 0.404부터 이 절에 적는다)

#### 0.403 — 참가자 가입 원형 연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 열세 번째 단위. Start 순서의 둘째 조각(로그인 가입 쓰기)을 세션 원형으로 연결한다. Ready 0x40·라이브 연결은 다음 단위다.
- 구현: HostSession.JoinPlayer+PlayerJoinHooks(구독자 선기입·접속 주소(로컬 제외)·flags `(x&~0x100)|3`·접속/참여 표시 대상 전송 두 순회, 원본 `Joined` 순서 그대로). 도달하지 않은 효과는 null로 두고 도달 때만 요구한다.
- 검증: 1번 가입(구독자 1·flags 3·덤프 행 바이트·자기 전송 `[(1,1),(1,1)]`), 원격 2번(구독자 77·flags 3·접속 주소·자기 전송 `[(2,2),(2,2)]`), 주소 미연결(구독자만 쓰고 flags 전 실패)을 단정한다. 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 11건(신규 3건)+MissionFortLoadTests 2건+SessionStateDumpTests 10건+HostMissionOptionsTests 1건+HostOptionsFileTests 3건 합계 27건 통과(실패 0, NETSTORM_DATA 지정). `|3` 제거 변이에서 가입 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 없음(이식된 모듈의 호스트 연결이다). 1바이트 판독·리틀 엔디언 기록은 원본 폭 그대로 둔다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Start 순서와 미션 시작 흐름 연결(Ready·Zone·로그인의 라이브 연결·구독자·flags·brain 토큰·월드 본체 조립·offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.402 — 로컬 Zone 설치 검증 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 열두 번째 단위. Start 순서의 첫 조각(구독자·flags 0x08 바인딩)의 출처를 추적하고 이식된 설치 본체를 검사로 확정한다. 로그인·Ready·라이브 연결은 다음 단위다.
- 추적: thewarbegins 1번(sub 1·flags 0x4b)의 완전 사슬을 밝혔다. Zone 설치(`InstallLocalMissionZone`: 구독자 일치 기록에 flags 0x08, 전 클라이언트에 구독자·이름 기록, 기본 mySubscriber 1) → 로그인 수신(`CheckLogin`·`Joined`으로 flags 0x03, battleOwner에 Ready 0x40). 합계 0x08|0x03|0x40=0x4b다. dn `Netstorm.Net/LocalMissionZone.cs`는 완전 이식된 사전 구현(검사 없음)임을 확인했다.
- 구현: 새 검사 `Core.Tests/LocalMissionZoneInstallTests.cs`(신규 2건). 단일 바인딩(구독자 1·이름 Youpson)에서 1번 0x2c=1·flags 0x08·이름·현재 상태(TemplateOwner·BattleOwner·PlayerKnown 1)·전역·이름표·서브 효과를 단정하고, 2번 미접촉을 단정한다. 범위·소유자·진단 거부를 단정한다. 제품 코드는 바꾸지 않았다(변이 원복 확인).
- 검증: 깨끗한 빌드(경고 0·오류 0) 뒤 신규 2건 통과(실패 0). flags 0x08 제거 변이에서 바인딩 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 없음(이식된 모듈의 검사 연결이다). 검사 배치는 `Core.Tests`(기존 `Netstorm.Net` 참조 이용)이며 새 프로젝트를 만들지 않았다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Start 순서와 미션 시작 흐름 연결(Zone·로그인·Ready의 라이브 연결·구독자·flags·brain 토큰·월드 본체 조립·offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.401 — 동맹 마스크 라이브 연결+L2 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 열한 번째 단위. 0.400의 마스크 기록을 시작 흐름에 연결하고 실행 호스트 덤프를 기준선과 행별로 대조한다. 구독자·flags·brain·Start 순서는 다음 단위다.
- 구현: ClientHostGame.StartMission에 StartPlayerMasks 연결(StartPlayers 다음). 대본·타입 표를 목록용으로 한 번 더 여는 중복은 세션이 표를 소유하면 합친다.
- 검증(게임 실행 있음, 예외 PC): --mission thewarbegins --handle Youpson 260프레임 실행(종료 코드 0)으로 world-1 덤프 281행을 얻었다. 마스크 9행이 기준선 f090과 바이트 동일하다(0번 0x1·1번 0x2·2번 0x4·3~8번 1u<<번호). 연결 전 덤프는 9행 모두 0이라 연결이 바꾼 것임을 확인했다. originals 1394파일 해시가 기준선과 차이 0이다. 깨끗한 빌드(경고 0·오류 0) 뒤 같은 집중 24건 통과(실패 0, NETSTORM_DATA 지정).
- C# 모양 차이: 없음(이식된 모듈의 호스트 연결이다). 라이브 brain 생성 연결은 월드 효과와 함께 다음 단위다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Start 순서와 미션 시작 흐름 연결(라이브 생성 효과·구독자·flags·brain 토큰·월드 본체 조립·offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.400 — 동맹 마스크 기록 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 열 번째 단위. 미션 값 채우기의 여섯째 조각으로 참가자 기록의 동맹 마스크 DWORD를 채운다. Start 순서·라이브 시작 흐름 연결은 다음 단위다.
- 구현: HostSession.StartPlayerMasks(9기록 0x54 DWORD를 자기 비트+목록 대칭 판정으로 번호 순서대로 쓰기, 0번 중립은 판정 밖이라 자기 비트만). Chat 표시는 바꾸지 않는다.
- 검증: thewarbegins 9기록 마스크가 기준선 f090과 같다(0번 0x1·1번 0x2·2번 0x4·3~8번 1u<<번호, 덤프 player 행의 같은 바이트도 단정). 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 8건(신규 1건)+MissionFortLoadTests 2건+SessionStateDumpTests 10건+HostMissionOptionsTests 1건+HostOptionsFileTests 3건 합계 24건 통과(실패 0, NETSTORM_DATA 지정). 자기 비트 제거 변이에서 마스크 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 없음(이식된 모듈·버퍼의 호스트 연결이다). 마스크 오프셋 0x54·기록 폭 180은 RawPlayerParticipation과 같은 상수다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Start 순서와 미션 시작 흐름 연결(라이브 마스크·생성 효과·구독자·flags·brain 토큰·월드 본체 조립·offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.399 — 인공지능 생성 본체 연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 아홉 번째 단위. 미션 값 채우기의 다섯째 조각으로 인공지능 생성 본체를 연결한다. Start 순서·월드 본체 조립은 다음 단위다.
- 구현: HostSession.AiBrains 소유+CreateAiBrains(세션 버퍼 공유 참여 모듈+이식된 RawAiCreate로 활성·전산 소유자만 번호 순서대로 생성·부착, 설정 미보유는 명시 거부). 라이브 시작 흐름(ClientHostGame.StartMission) 연결은 월드 효과와 함께 다음 단위로 남겼다(스텁 토큰이 Player 덤프 행을 바꾸지 않게).
- 검증: thewarbegins 2번만 생성된다(brain≠0, Player 기록 부착·BrainFlag, 생성자 600B·0xb08·보통 설정 그대로·입력 불변, 색 효과·재호출 재사용 단정). NoTemple 분기(첫 지역·신전 조회+0x400000 제거, 생성자 미호출)와 생성자 미연결(실패·미부착)도 단정한다. 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 7건(신규 3건)+MissionFortLoadTests 2건+SessionStateDumpTests 10건+HostMissionOptionsTests 1건+HostOptionsFileTests 3건 합계 23건 통과(실패 0, NETSTORM_DATA 지정). 전산 조건 완화(1번 포함) 변이에서 생성 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 참여 모듈은 세션 버퍼를 공유하는 Simulation.PlayerParticipation이다(Original.RawPlayerParticipation이 아니라 RawAiCreate가 쓰는 쪽). 모호 참조는 전체 이름으로 가린다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Start 순서와 미션 시작 흐름 연결(라이브 생성 효과·월드 본체 조립·offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.398 — 호스트 미션 진입+L2 행별 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 여덟 번째 단위. 호스트 --mission 진입과 세션 데이터 조립을 연결하고 실행 호스트 world-1 덤프를 기준선과 행별로 대조한다. 월드 본체 조립·AI 생성 본체·브리핑 화면은 다음 단위다.
- 구현: HostOptions --mission·--handle 인자, ClientHostGame.StartMission(대본+타입 표+요새 적재+목록+SP+AI+참가자 묶음, 덤프에 세션 연결), Program 사용법 문구. 대본·타입 표를 목록용으로 한 번 더 여는 중복은 세션이 표를 소유하면 합친다.
- 검증(게임 실행 있음, 예외 PC): --mission thewarbegins --handle Youpson 260프레임 실행(종료 코드 0)으로 world-1 덤프 281행을 얻었다. 기준선 f090과 행별 대조: 동맹 81칸 바이트 동일, 숨김 18자리 동일, SP 참가자 확정(초기화 1·1번 1161527296·2~8번 0, offset 97·0번 0은 기록만), 이름 바이트(2번 바이트 동일·1번 바이트15 잔여). 풀·슬롯·graph·totals·totalmade·Kernel·시각 행은 월드 본체 미조립으로 다름을 확인하고 남겼다. HostMissionOptionsTests 1건 포함 집중 20건 통과(실패 0). 파서 변이(--mission 고정값)로 검출력을 확인했다.
- C# 모양 차이: --mission 직접 진입은 브리핑 화면을 거치지 않는다(M1 상태 대조용, 화면 흐름은 R5·R7에서 잇는다). 1번 핸들은 --handle 입력이며 출처(메뉴·설정)는 Start 흐름 단위에서 잇는다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: AI 생성 본체 연결·Start 순서(월드 본체 조립·offset 사슬·이름 쓰기 순서·핸들 출처 포함), 34경로 L2 대조(M1).

#### 0.397 — 참가자·동맹 채우기 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 일곱 번째 단위. 미션 값 채우기의 넷째 조각으로 참가자·동맹 채우기를 연결한다. AI 생성 본체·Start 순서는 다음 단위다.
- 구현: HostSession.StartPlayers(미션 목록+로컬 핸들). 동맹 81칸은 대각선 초기화+대칭 판정으로 채운다. 이름은 비어 있지 않을 때만 Latin1+NUL을 0xc에 쓴다(1번은 대본 이름이 아니라 실제 핸들). 숨김 9바이트는 0 그대로다.
- 검증: thewarbegins 동맹 81칸이 기준선과 바이트 동일하다(대각선 1). 대각선 0번을 빼는 변이에서 검사 1건 실패로 검출력을 확인했다. 숨김 18자리 0과 2번 이름(The Jailer NUL 바이트), 1번 이름 붙여쓰기를 단정한다. 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 4건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 16건 통과(실패 0, NETSTORM_DATA 지정).
- 잔여 2건(증거 포함): ① 1번 바이트 15 NUL. 기준선은 "You\0pson"인데 구현은 "Youpson"을 붙여 쓴다. 프레임·실행에 안정적이므로 로그인 쓰기 순서 문제다. Start 순서 단위에서 잇는다. ② 1번 핸들 출처. 대본 이름("Player")이 아니라 실제 핸들이 들어가며, 값은 오라클 관측("Youpson")을 썼다. 출처(메뉴·설정)는 Start 흐름 단위에서 잇는다.
- C# 모양 차이: 없음(이식된 모듈·버퍼의 호스트 연결이다). 기록 폭 180·이름 오프셋 0xc·폭 32는 RawPlayerParticipation과 같은 상수다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: AI 생성 본체 연결·Start 순서와 미션 시작 흐름 연결(offset 사슬·이름 쓰기 순서·핸들 출처 포함), 34경로 L2 대조(M1).

#### 0.396 — 인공지능 설정 바인딩 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 여섯 번째 단위. 미션 값 채우기의 셋째 조각으로 인공지능 설정 바인딩을 연결한다. AI 생성 본체 연결·Start 순서는 다음 단위다.
- 구현: HostSession.PrepareAiSettings(인공지능 소유자만 RawMissionAiSettings.Prepare로 600B 채우기, 대본 조회+atof 머리값+I등록 이름 진단 포함)+AiSettings 소유(난이도 배율은 생성 때 기존 RawAiCreate 경로로 적용하므로 원본 그대로 둔다). atof 머리값은 RawMissionPlayers와 같은 규칙의 지역 함수다.
- 검증: thewarbegins 2번 설정이 대본과 같다(이름 The Jailer·기술 원문·주기 6·6·3·시작금 0·부착 1·채취 1·능력 비트 0xF7DF). 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 3건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 15건 통과(실패 0, NETSTORM_DATA 지정). 인공지능 판별 완화(전산 조건 제거) 변이에서 설정 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 없음(이식된 모듈의 호스트 연결이다). 1번 사람·3~8번 비활성은 만들지 않는다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: AI 생성 본체 연결·Start 순서와 미션 시작 흐름 연결(offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.395 — 시작 SP 지정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 다섯 번째 단위. 미션 값 채우기의 둘째 조각으로 시작 SP 지정을 연결한다. 세션 바인딩(AI 설정, 생성)과 Start 순서는 다음 단위다.
- 구현: HostSession.BindSpReseed(재시드 공급 연결)+StartPlayerSp(활성 참가자를 이식된 SetPlayerSpAmount로 지정, 읽기는 로컬 활성·1번 owner·비편집 고정, 표시 갱신 없음). SP 저장소는 재시드 전달 람다로 만든다. 미연결 지정은 명시 거부다.
- 검증: thewarbegins 시작 SP가 기준선 sp 행의 참가자 값과 같다(초기화 1, 1번 1161527296, 0·2~8번 0). 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 2건+MissionFortLoadTests 2건+SessionStateDumpTests 10건 합계 14건 통과(실패 0, NETSTORM_DATA 지정). 지정 범위 축소(1번 제외) 변이에서 시작 SP 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- offset 잔여: 실제 사슬(세션 게임 난수+CRT 상자+모드 상자+고정 유닉스 초 946684800) 연결에도 offset 97이 나와 기준선 103과 다르다. 고정 시각 2회 실행에서 기준선이 103으로 안정적이고 소유자 0만 다른 것을 확인했으므로(0.390·0.395 탐색) offset은 결정적 비교 대상이다. 후보 유닉스 초 30개를 훑어도 103이 없어 입력 상자 상태(CRT·모드 소비 시점) 또는 Rotate·추적 offset 경로가 Start 순서와 함께 맞아야 한다. 참가자 값은 offset과 무관하게 정확하므로(지정·복호가 offset을 상쇄) 이번 단위에서 확정했다. 진단용 탐색 검사(SpOffsetProbeTests)는 삭제했다.
- C# 모양 차이: NetstormRandom과 GameRandom은 같은 식(0x10003·+3·상위 16비트)이라 SP 저장소 난수를 세션 난수와 같은 값으로 쓸 수 있다. 재시드 공급은 미션 시작 흐름에서 실제 사슬로 교체한다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: 세션 바인딩(AI 설정·생성 포함)·Start 순서와 미션 시작 흐름 연결(offset 사슬 포함), 34경로 L2 대조(M1).

#### 0.394 — 미션 플레이어 목록 적재 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 네 번째 단위. 미션 값 채우기의 첫 조각으로 플레이어 목록 적재를 이식한다. SP 초기화·세션 바인딩·AI 설정·생성은 다음 단위다.
- 구현: 새 파일 Netstorm.Core/Original/RawMissionPlayers.cs(cpppj o/Player.cpp의 MissionPlayers::Load·Allied 직역: techAllowed deny·allow·all 순서, 소유 번호 색 기본값, 구식 ai 설정의 플레이어 2 기본 입력, 이름·SP·기술·지식·능력·색·동맹, 저장 기술·덱 보존, 동맹 대칭화). atof 머리값·세미콜론 목록은 이식된 ConfigNumbers·RiftTypeTable.Find 규칙을 쓴다. PORT_MAP.tsv에 Player.cpp·Player.h 대응 2행(port)을 끝에 직접 더했다.
- 검증: thewarbegins 실자료 적재가 대본과 같다(1번 Player·3000·지식 4종·동맹 자기자신, 2번 The Jailer·0·지식 3종·능력 원문·동맹 자기자신, 3~8번 비활성·자기 동맹만, 저장 기술 26·덱 38, techAllowed 전부 참). 깨끗한 빌드(경고 0·오류 0) 뒤 MissionPlayersLoadTests 1건+MissionFortLoadTests 2건 합계 3건 통과(실패 0, NETSTORM_DATA 지정). ai 기본 입력 번호 변이(2→3)에서 목록 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 조회 함수는 Func<string, string?>이다. 저장 기술·덱은 요새 Template의 같은 객체를 둔다. 구 MissionScript 이름 겹침은 0.391 장치에서 완전 이름으로 피했다.
- PORT_MAP 691→693줄(자료 688행: L1 337·port 351). fixture 변화 없음(L1 아님). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: SP 초기화·세션 바인딩(AI 설정·생성 포함)·Start 순서와 미션 시작 흐름 연결, 34경로 L2 대조(M1).

#### 0.393 — 세션 저장소 덤프 행 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 세 번째 단위. world-1 덤프에 Player·동맹·SP·Graph·장부·TotalMade 행을 연결한다. 미션 값 채우기와 시작 흐름 연결은 다음 단위다.
- 구현: SessionStateDump에 SessionDumpPlayers·SessionDumpSp·SessionDumpGraphRow·SessionDumpTotals·SessionDumpTotalMade 기록, DecodeScrambledSp 복호(0040ec50), FormatWorld1 확장(행 순서·개수 검증·소문자 16진은 cpppj와 같다). HostSession에 Player 1620B·숨김 9B·동맹 81DWORD 버퍼(Start 순서에서 RawPlayerParticipation과 공유), RawSpStore 소유, RawGraphStorage·RawTotalMadeStorage 소유, 장부 3정수를 더하고 덤프 관측 5종을 달았다. HostSessionDump.Write는 홀더 관측을 그대로 넘긴다.
- 검증: 빈 세션 덤프에 player 9행(0 360자리)·hidden 18자리·alliances 0 81칸·sp 대시 9칸·graph 255행 0·totals 0·totalmade 없음이 들어간다. 0이 아닌 값(기록 바이트·숨김·동맹 양끝·장부 음수·TotalMade 3종)은 행으로 나오고, 초기화 SP는 소유자별 복호값으로 나온다. 깨끗한 빌드(경고 0·오류 0) 뒤 SessionStateDumpTests 10건+MissionFortLoadTests 2건+HostStartupSceneTests 25건 합계 37건 통과(실패 0, NETSTORM_DATA 지정). 동맹 오타(alliances 단수) 변이에서 0아닌 저장소 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 저장소 값은 덤프 때 복사한다. SP 난수는 홀더 소유 NetstormRandom이며 Start 순서에서 실제 난수에 연결한다. Graph ResetTable·SP 초기화·장부 집계는 월드·Start 연결 때 잇는다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Start 순서와 미션 시작 흐름 연결(미션 값 채우기·세션 바인딩·AI 설정·생성), 34경로 L2 대조(M1).

#### 0.392 — 세션 홀더+world-1 덤프 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 두 번째 단위. 미션 세션 홀더와 world 1 덤프(머리, 난수, Kernel, pool, slot 행)를 연결한다. Player 이후 행과 미션 시작 흐름 연결은 다음 단위다.
- 구현: 새 파일 Netstorm.Client/Host/HostSession.cs(Kernel·난수 공유+RawSidPool 소유, 미션 풀 32768칸, 덤프용 풀 관측+예약·사용 슬롯 목록). SessionStateDump에 SessionDumpPool·SessionDumpSlot 기록과 FormatWorld1 추가(머리 공용화, pool 행 순서·소문자 16진·해시 무패딩은 cpppj와 같다). HostSessionDump.BindSession 연결·해제. 미션 시작 흐름의 호스트 연결은 아직 없어 게임 고리에는 잇지 않았다.
- 검증: 빈 풀 덤프가 pool 행(32768·50·32763·15000·5·1·해시)과 예약 슬롯 0~4 행(0번 머리·1번 0·2~4번 free)과 바이트 동일 구조로 나온다. 클라이언트 할당 1건은 slot 행으로 나오고 반납하면 빠진다. 깨끗한 빌드(경고 0·오류 0) 뒤 SessionStateDumpTests 7건+MissionFortLoadTests 2건+HostStartupSceneTests 25건 합계 34건 통과(실패 0, NETSTORM_DATA 지정). 세션 무시(Bind 무력화) 변이에서 world-1 파일 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. world 0 기존 검사 5건도 그대로 통과해 머리 공용화가 출력을 바꾸지 않음을 확인했다.
- C# 모양 차이: 덤프용 풀 관측은 값 복사 기록이다(덤프 중 풀 변경과 섞이지 않는다). 풀 용량은 미션 고정 32768이며 미션별 가변은 월드 조립에서 잇는다.
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Player·동맹·SP·Graph·장부·TotalMade 행, Start 순서와 미션 시작 흐름 연결, 34경로 L2 대조(M1).

#### 0.391 — 호스트 미션 Fort 적재 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트, 월드 조립)의 첫 조립 단위. 미션 스크립트+요새 원문 읽기+Template 풀기를 호스트 장치로 연결한다. 세션·월드 본체 조립은 다음 단위다.
- 구현: 새 파일 Netstorm.Client/Host/MissionFortLoad.cs(미션 스크립트 열기+FortFileReader 원문 읽기+LoadTypeTable 10.78 타입 표+RiftTypeLoadOrder 순서+BOM 규칙+FortTemplate.Parse 묶음, 결과 레코드). 구 Assets.MissionScript·FortFile(교체·은퇴 대상, 2.2·2.6-0.128)은 쓰지 않고 이식된 MissionScript·FortTemplate.Parse 경로만 쓴다. 새 검사 Netstorm.Client.Tests/MissionFortLoadTests.cs 2건.
- 검증: thewarbegins 적재 목록이 cpp --inspect-fort와 행 단위로 같다(섹션 35·flag 0·trailing 56·subscriber 66888756 Player·storm power 100000·type names 171·technology 26·deck 38·Chaff 145=bridge 15·geyser 13·noIsland 117·Terr00 4종·Terr01 6종 23개·타입 표 188). 깨끗한 빌드(경고 0·오류 0) 뒤 MissionFortLoadTests 2건+HostStartupSceneTests 25건 합계 27건 통과(실패 0, NETSTORM_DATA 지정). 판독 약화(ReadSections만) 변이에서 목록 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- C# 모양 차이: 구 MissionScript 이름이 겹쳐(Assets 대 Client) MissionScript·FortChunk·FortObject 참조를 완전 이름으로 적었다.구 판독기 FortFile은 건드리지 않았다(은퇴는 Edit 경로 전환 때 2.2·2.6-0.128대로 한다).
- PORT_MAP, fixture 변화 없음(이식된 모듈의 호스트 연결이며 대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: 세션 홀더+world 0→1 확장(덤프 world·kernel·pool 행부터), slot·object·player·graph·sp·totalmade 행과 Start 순서, 34경로 L2 대조(M1).

#### 0.390 — L2 베이스라인 탐색 thewarbegins, 코드 변경 없음 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 마무리)의 첫 단위. M1 판정의 비교 기준(cpp world-1 덤프)과 새 경로의 현재 상한을 같은 조건에서 확정한다. Early Missions 첫 경로 thewarbegins 1경로만 본다(나머지 33경로는 0.391+).
- cpp 기준선(오라클 cpppj/build/bin/Release/Netstorm.exe, 11-12 빌드, 메뉴 경로와 같은 빌드): --run extracted/dotnet-oracle-data --mission thewarbegins --fixed-frame-ms 25 --window --mute-audio --frames 260 --set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0" + ui-script(click-if Play Mission ×6, f090·f240 session-dump). 종료 코드 0, 실행 전후 originals 해시 일치. f090 덤프 4174행(world 1, kernel 35, process 35, regular 25, slot 1941, object 1881, player 9, graph 255, totalmade 16), f240 덤프 4175행(slot 1943). 슬롯·객체 수가 11-10 관측(1941/1943·1881)과 같아 오라클 결정론이 유지됨을 확인했다. --dump-world 정적 기준선도 같은 자료에 저장했다(world 172·카메라·Player·object 행).
- 새 경로 상한(같은 자료·설정·고정 시각): 메뉴 world-0 덤프는 판번호 1 형식으로 나온다(frame·random·world 0·kernel·end). --mission/--run 인자는 사용법 오류(종료 코드 2)로 거부된다. 즉 world-1 덤프에 필요한 미션 진입·세션·월드 조립이 전혀 없어 M1 비교가 0건인 상태다.
- 다음 조립 순서(0.391+ 제안): ① 호스트 미션 Fort 적재(이식된 Fort reader로, 2.6-0.128의 FortFile 은퇴 포함) ② 세션 홀더+world 0→1 확장(덤프 world·kernel·pool 행부터) ③ slot·object·player·graph·sp·totalmade 행과 Start 순서 ④ 34경로 L2 대조. 산출물은 extracted/dotnet-l2-baseline/thewarbegins/(Git 제외)에 있다.
- 검증(게임 실행 있음, 예외 PC): 위 탐색 실행 4건(cpp 미션 1·dn world-0 1·dn --mission 거부 1·cpp --dump-world 1). 코드 변경이 없어 집중 검사는 다시 돌리지 않았다. 첫 실행에서 UI 스크립트 UTF-8 BOM 때문에 cpp가 invalid stoull로 실패한 것을 확인하고 BOM 없이 저장하는 방식(l4a_compare.ps1과 같은 UTF8Encoding false)으로 고쳐 재실행했다(도구·산출물 변경 없음).
- PORT_MAP, fixture, 검사, 코드 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: 0.4 순서 2(R4 실제 호스트, 월드 조립, M1).

#### 0.389 — L4a 메뉴 적용 범위 확정, 코드 변경 없음 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 남은 메뉴 장면 탐색. Multiplayer 버튼·Edit 이동행(Next 50)·Welcome 창을 l4a_compare.ps1 탐색 실행으로 확인했다(게임 실행 있음, 예외 PC).
- 결과: 3건 모두 비교 장면에서 뺐다. ① Multiplayer 버튼은 cpppj가 멀티섬 설정 화면(State 6)으로 가고(섬 월드 표시+R5, 멀티Setup+3차) dn은 Not Implemented Yet 창을 낸다. 캡처 91% 다름을 확인하고 TSV를 삭제했다. R5 진입 화면 비교(D2), 10단계(로컬 멀티)에서 잇는다. ② Edit Next 50 이동행 선택 소비는 R7 잔여다(0.363 기록: 이동행 선택 소비는 메뉴 소유자 R7에서 잇는다). Dispatch에 분기가 없어 누르면 명시 예외가 나며, 장면은 R7 Edit에서 잇는다. ③ Welcome 창은 양쪽 모두 창이 안 떠서 메인 메뉴와 바이트 동일이다. 장면을 만들지 않았다.
- 이정표 D1 정리: 메뉴 L4a 12장면이 차이 0이다(메인·Credits·Help·Options·Campaign·Demo·Edit·About·Resolution·Credits 항목·Campaign 하위·Difficulty). AI 작업으로 남은 메뉴 장면은 위 3건뿐이며 모두 단계 뒤로 미뤘다(Language 자식 CJK는 0.388에서 R8로). 따라서 다음 단위부터 0.4 순서 2(R4 실제 호스트, 월드 조립)로 간다. R3 잔여 코드(2.5: 대화창 분리·목록 소유자·모달 소유자 등)는 세션 조립과 함께 파일 묶음 단위로 처리한다.
- 검증(게임 실행 있음, 예외 PC): 탐색 실행 3건(Multiplayer 비교 차이 확인, Edit 이동행 미연결 확인, Welcome 무창 확인). 코드 변경이 없어 집중 검사는 다시 돌리지 않았다.
- PORT_MAP, fixture, 검사, 코드 변화 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치, 도구가 검증). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: 0.4 순서 2(R4 실제 호스트, 월드 조립, M1).

#### 0.388 — 2차 3장면 L4a 차이 0, 코드 변경 없음 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 2차 화면 L4a 확장 계속. 새 장면 l4a/menu-credits-entry.tsv(Credits 첫 행 클릭 후 3프레임), l4a/menu-campaign-early.tsv(Early Missions 행 클릭 후 3프레임), l4a/menu-difficulty.tsv(Difficulty 행 클릭 후 3프레임)를 l4a_compare.ps1로 cpppj와 비교했다(게임 실행 있음, 예외 PC). 2차 행 클릭은 화면 좌표(screen:x,y)를 쓴다.
- 결과: 3장면 모두 첫 실행에 3프레임 **차이 0(BMP 바이트 동일)**. 코드 변경이 없다. Credits 항목 창(10.72 상세+More·Back·Cancel), Campaign 하위(Early 브리핑+6임무+Back), Difficulty 자식 펼침이 모두 기존 이식(0.358 TellBody, 0.384 펼침 구조)으로 맞았다. 캡처 내용을 눈으로 확인했다.
- Language 자식은 펼침·위치·English 행까지 맞았으나 한국어, 일본어 행 글리프가 달라 비교에서 제외했다(TSV 삭제). 외부 글리프 출력과 번역 적재, 검증은 R8에서 하며(6차, 13차, 결정 D4) 장면도 그때 다시 만든다. 확장 CJK 측정 부재(R2 잔여, 0.374)의 화면 증거다.
- 검증(게임 실행 있음, 예외 PC): l4a_compare.ps1 3장면이 종료 코드 0이다. 코드 변경이 없어 집중 검사는 다시 돌리지 않았다.
- PORT_MAP, fixture, 검사 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치, 도구가 검증). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Multiplayer, Edit 이동행(Next 50) L4a 장면 추가와 차이 해소, 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트, 월드 조립).

#### 0.387 — About·Resolution 펼침 L4a 차이 0과 해상도 10행 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 2차 화면 L4a 확장. 새 장면 l4a/menu-about.tsv(Help 팝업 Version 행 클릭 후 3프레임)·l4a/menu-resolution.tsv(Options 팝업 Resolution 행 클릭 후 3프레임)을 l4a_compare.ps1로 cpppj와 비교했다(게임 실행 있음, 예외 PC). 2차 행 클릭은 화면 좌표(screen:x,y)를 쓴다. 라벨 조회기는 메인 버튼만 알기 때문이다.
- 결과: About은 첫 실행에 3프레임 모두 차이 0(코드 변경 없음). Resolution 펼침은 처음 7,702픽셀(0.98%) 다름에서 자식 10행으로 차이 0(BMP 바이트 동일). dn 자식은 6행(원본 3+호환 3)이었고, cpppj 자식은 10행(원본 3+호환 5+FSR 2)이다. 1600x900·1920x1080(4차 와이드)과 2560x1440·3840x2160(FSR)을 더했다(근거 cpppj ScreenResolutions.h). 자식 펼침·앵커·선택 강조는 0.384 구조 그대로 맞았다.
- 검증(게임 실행 있음, 예외 PC): 깨끗한 빌드(경고 0·오류 0) 뒤 해상도 자식 10행 단정으로 고친 HostHelpPopupOpensAboutFromActualMenuRow 1건 통과(실패 0). l4a_compare.ps1 2장면이 종료 코드 0이다. 기존 7장면(menu-main·menu-credits·menu-help·menu-options)도 재완주해 차이 0을 유지했다(About·Options·Help은 SkipCpp 재사용).
- C# 모양 차이: 없음(자식 행 4줄 추가). FSR 행의 장치 판정과 SETSIZE 선택 적용은 R8 8차, R7 설정에서 잇는다.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치, 도구가 검증). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Multiplayer, 나머지 2차 펼침(언어·프레임·난이도·음량 자식, Credits 항목, Campaign 하위, Edit 이동행) L4a 장면 추가와 차이 해소, 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트, 월드 조립).

#### 0.386 — Demo·Edit 목록 L4a 차이 0과 Edit 무제한 연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 하위 메뉴 L4a 확장 다섯/여섯 번째 장면. 새 장면 l4a/menu-demo.tsv·l4a/menu-edit.tsv(프레임 5에 click, 30·60·120에 snapshot)을 l4a_compare.ps1로 cpppj와 비교했다(게임 실행 있음, 예외 PC).
- 결과: Demo는 첫 실행에 3프레임 모두 차이 0(코드 변경 없음). Edit는 처음 79,819픽셀(10.1%) 다름에서 수정 뒤 3프레임 모두 차이 0(BMP 바이트 동일). 원인은 목록 50개 상한이었다. dn은 처음 50개만 담아 이동행 없이 보여줬고, cpppj는 452개를 담고 50행 창+Next 50 이동행으로 보여준다. HostEditorSelection 생성 뒤에 Catalog.State.Limit = int.MaxValue를 넣어 닫았다(5차 무제한, 근거 cpppj UberGump.cpp 379행). 창이 넓어지며 목록 위치, 선택 강조까지 함께 맞았다.
- 검증(게임 실행 있음, 예외 PC): 깨끗한 빌드(경고 0, 오류 0, 솔루션 전체) 뒤 HostStartupSceneTests, GumpButtonTreeTests, ProductPresentationTests, TellMapMenusTests, SoloMapCatalogTests, SoloMapCatalogStorageTests 56건 통과(실패 0). 진단용 임시 검사(Edit 목록 구조 덤프)로 오라클 자료에서 50개 상한, 이동행 부재를 확정하고 삭제했다. l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-edit.tsv -Name menu-edit -Frames 130 -Build -SkipCpp가 종료 코드 0이다. xUnit2031 경고(Where+Single)도 같은 단위에서 걷어냈다.
- C# 모양 차이: 없음(한도 상수 1줄). solo 목록도 같은 목록을 쓰므로 함께 무제한이다(cpppj와 같은 구조).
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(실행 전후 originals 해시 일치, 도구가 검증). 추적 assets/game-data/d/options.cfg 불변(git status 확인).
- 남은 것: Multiplayer, 2차 하위 화면 펼침(Credits 항목, About, Version, Campaign 하위, 해상도, 음량, 언어, 프레임, 난이도 자식, Edit 이동행) L4a 장면 추가와 차이 해소, 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트, 월드 조립).

#### 0.385 — Campaign 목록 L4a 차이 0, 코드 변경 없음 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 하위 메뉴 L4a 확장 네 번째 장면. 새 장면 [`l4a/menu-campaign.tsv`](dotnetpj/tools/l4a/menu-campaign.tsv)(프레임 5에 `click Campaign`, 30·60·120에 `snapshot`)을 `l4a_compare.ps1`로 cpppj와 비교했다(게임 실행 있음, 예외 PC).
- 결과: 첫 실행에 3프레임 모두 **차이 0(BMP 바이트 동일)**. 코드 변경이 없다. Campaign Tell(`UCampaign`+Skirmish 7행+Back)은 Credits와 같은 모달 Tell 경로라 0.382 게이트로 이미 맞았고, Skirmish 행 추가(0.358 `TellBody`)도 양쪽에 들어 있다. 캡처에 7행과 Back 버튼이 모두 보이는 것을 눈으로 확인했다.
- 검증(게임 실행 있음, 예외 PC): `l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-campaign.tsv -Name menu-campaign -Frames 130`이 종료 코드 0이다(양쪽 실행 포함). 코드 변경이 없어 집중 검사는 다시 돌리지 않았다.
- PORT_MAP·fixture·검사 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(실행 전후 `originals/` 해시 일치, 도구가 검증). 추적 `assets/game-data/d/options.cfg` 불변(`git status` 확인).
- 남은 것: Demo·Edit 하위 메뉴 L4a 장면 추가와 차이 해소 → 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트·월드 조립).

#### 0.384 — Options 실제 행·하위 메뉴·앵커와 L4a 차이 0 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 하위 메뉴 L4a 확장 세 번째 장면. 새 장면 [`l4a/menu-options.tsv`](dotnetpj/tools/l4a/menu-options.tsv)(프레임 5에 `click Options`, 30·60·120에 `snapshot`)을 `l4a_compare.ps1`로 cpppj와 비교했다(게임 실행 있음, 예외 PC).
- 결과: 처음 **42,880픽셀(5.45%) 다름** → 3단계로 0에 닫았다. ① 내용·위치(옛 12행 중앙 부착) → 실제 20칸(Resolution·Language·Frame Rate·Difficulty·음량 하위·AA·FSR·구분선 3개)·앵커 부착·플래그로 12,250픽셀. ② 소리 6행의 사용·사용 안 함(`0x5c7b14` 장치 준비, 오라클 `--mute-audio`로 거짓) → `HostOptions --mute-audio` 이식과 `ClientHostGame`→`HostPopupMenu` 판정 전달, 비교 도구의 dotnet 실행에도 `--mute-audio`를 넣어 88픽셀. ③ 자식 표시 `>` 1px(리터럴 `" >"` 측정·출력 대신 실제 자식+마커 경로) → 하위 메뉴 6종 사전 구축+`AddSubmenu` 펼침 구조로 **차이 0(BMP 바이트 동일)**.
- 구현: `HostPopupMenu.OpenOptions` 전면 교체(부모 20칸+자식 6열 사전 구축), 자식 행 실명령(해상도 0x542948+SETSIZE·음량 0x542a8c/90+번호·언어 0x542988+번호·프레임/난이도 0x542948+Patch1101 문자열), `HostMenuInput.Dispatch`에 음량 원시 분기(큐 마스크 뒤 `0x02002a8c`·`0x02002a90`, 기존 `Execute("Volume")`로 돌려 동작 유지), `HostOptions.MuteAudio`, `MenuGump.AddMenuFlags`, `HostMainMenu.ControlBounds`. 중앙 팝업 구방식(`Open("resolution")` 등 6분기)은 삭제하고 펼침 방식으로 일원화했다.
- 검증(게임 실행 있음, 예외 PC): 깨끗한 빌드(경고 0·오류 0) 뒤 `HostHelpPopupOpensAboutFromActualMenuRow`(펼침 경로·자식 실명령·음량 흐름으로 갱신) 포함 집중 41건 통과(실패 0). 자식 펼침 2-idle·마스크 값은 디버깅 과정에서 실행 출력으로 확정했다(펼침 뒤 1 idle만으로는 자식이 입력을 받지 않고, 원시 코드는 마스크 뒤 값으로 판별한다). `l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-options.tsv -Name menu-options -Frames 130 -Build -SkipCpp`가 종료 코드 0이다. 기존 3장면(menu-main·menu-credits·menu-help)도 `--mute-audio` 추가 뒤 재완주해 차이 0을 유지했다.
- C# 모양 차이: 부모 행은 기존 번호표·`Execute` 방식을 유지하고 자식 행만 실명령을 carry 한다. 언어·프레임·난이도·해상도 선택 적용, 펼침 자식의 깊이·모달, FSR 해상도 행, 후처리 장치 판정은 R8 해당 차수·R3 잔여로 미뤘다(기록에만 남긴다).
- PORT_MAP·fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(실행 전후 `originals/` 해시 일치, 도구가 검증). 추적 `assets/game-data/d/options.cfg` 불변(`git status` 확인).
- 남은 것: Campaign 하위 메뉴 L4a 장면 추가와 차이 해소 → 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트·월드 조립).

#### 0.383 — Help 팝업 실제 3행·앵커와 L4a 차이 0 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 하위 메뉴 L4a 확장 두 번째 장면. 새 장면 [`l4a/menu-help.tsv`](dotnetpj/tools/l4a/menu-help.tsv)(프레임 5에 `click Help`, 30·60·120에 `snapshot`)을 `l4a_compare.ps1`로 cpppj와 비교했다(게임 실행 있음, 예외 PC).
- 결과: 처음 **7,965픽셀(1.01%) 다름** → 수정 뒤 3프레임 모두 **차이 0(BMP 바이트 동일)**. 원인은 Help 팝업 미이식이었다. dn 자리표시는 1행(`About NetStorm`)을 화면 중앙에 붙였고, cpppj는 3행(`General Help - ~iF1~.`·`Technical Help`·`Version`)을 Help 버튼 앵커(`baseWidth*0.5+x`·`y`)에 붙인다. `HostPopupMenu.Open("help")`을 실제 구성으로 바꿨다: 3행의 실제 명령·인자(0x02000017 + `0x70000000|F1Help` 별칭·0x02000049·0x02000095 + `About` 별칭), 앵커 부착(`HostMainMenu.ControlBounds` + `Widgets.Attach` + `MenuGump.AddMenuFlags` 0x04004000). 영어 라벨은 번역 그대로 쓰며(D4) 한·일 번역은 R8 6차·13차에서 잇는다.
- 함께 한 것: `HostMenuInput.Dispatch`에 0x17·0x49 명시 예외(도움말 주제 창 R3 잔여·기술 도움말 외부 실행 경계). `TechnicalHelpAvailable`은 같은 설정 키(`techHelpExe`·`techHelpFile`)와 파일 장치 읽기 가능으로 판정한다(exact FileSpec 해석은 R3 잔여). 팝업의 깊이 조정·모달 지정(`RawMenuControls.cpp` 1051행)은 R3 잔여로 미뤘다(모달을 붙이면 0.382의 모달 게이트도 함께 손봐야 한다).
- 검증(게임 실행 있음, 예외 PC): 깨끗한 빌드(경고 0·오류 0) 뒤 기존 검사 `HostHelpPopupOpensAboutFromActualMenuRow`(3행·명령·Version→About 흐름으로 갱신) 포함 집중 38건 통과(실패 0). 행 구성을 옛 1행으로 되돌리는 변이는 위 갱신 검사가 실패하므로 검출력을 유지한다(별도 변이 실행 없이 구조로 확인). `l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-help.tsv -Name menu-help -Frames 130 -Build -SkipCpp`가 종료 코드 0이다(cpp 캡처는 0.383 실행에서 만든 것을 재사용).
- C# 모양 차이: `HostPopupMenu` 생성자에 공유 문자열 고리(`ScriptStringRing`)를 받는다(호출자 2곳 수정). 선택 흐름은 cpppj와 같이 실제 명령을 큐에 넣는다(SelectCommand 우회 없음).
- PORT_MAP·fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음(기술 도움말 실행은 연결하지 않았다).
- 보호: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(실행 전후 `originals/` 해시 일치, 도구가 검증). 추적 `assets/game-data/d/options.cfg` 불변(`git status` 확인).
- 남은 것: Options·Campaign 하위 메뉴 L4a 장면 추가와 차이 해소 → 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트·월드 조립).

#### 0.382 — Credits Tell 모달 출력과 L4a 차이 0 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 하위 메뉴 L4a 확장 첫 장면. 새 장면 [`l4a/menu-credits.tsv`](dotnetpj/tools/l4a/menu-credits.tsv)(프레임 5에 `click Credits`, 30·60·120에 `snapshot`)을 `l4a_compare.ps1`로 cpppj와 비교했다(게임 실행 있음, 예외 PC).
- 결과: 처음 **2,929픽셀(0.37%) 다름** → 수정 뒤 3프레임 모두 **차이 0(BMP 바이트 동일)**. 원인은 모달 Tell이 열려도 메인 메뉴 돌 버튼 8개와 각 배경 자식을 계속 그리던 결함이었다. cpppj는 Tell이 열리면 Dialog 단계로 바뀌어 버튼을 그리지 않고 Tell 창만 그린다(`UberGump.cpp` 2029행·2065행). 팝업 메뉴(Help·Options)는 모달이 아니라 버튼을 계속 그리므로(같은 2029행) 제외하지 않는다. 배경 자식은 별도 수집 항목이라 함께 건너뛰어야 했다(버튼만 숨긴 중간 실행에서 2,711픽셀로 줄었다).
- 구현: `HostMainMenu.HideWithModal`(버튼 8개+배경 자식 집합, 생성자에서 수집) 추가, `HostGumpDrawing.Draw`의 돌 버튼·배경 분기에 `CurrentDialog != 0` 게이트 추가. 새 검사 `HostStartupSceneTests.ModalTellHidesMainMenuStonesButKeepsDialogButton`(모달 중 버튼 밖 영역이 타이틀과 같고 Cancel 버튼은 그려짐).
- 검증(게임 실행 있음, 예외 PC): 깨끗한 빌드(경고 0·오류 0) 뒤 신규 포함 `HostStartupSceneTests`·`GumpButtonTreeTests`·`ProductPresentationTests` 38건 통과(실패 0). 무력화 변이(`HideWithModal` 항상 거짓)에서 신규 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. `l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-credits.tsv -Name menu-credits -Frames 130 -Build -SkipCpp`가 종료 코드 0이다(cpp 캡처는 CD 10.72 변경만 포함한 11-12 재빌드 오라클로 만든 것을 재사용, 메뉴 경로 무영향).
- C# 모양 차이: cpppj의 단계(phase) 기계를 모달 판별로 옮겼다. 실제 Dialog 단계의 HTML 배경 GIF 분기는 바꾸지 않았다(타이틀 기반이 이미 일치한다).
- PORT_MAP·fixture 변화 없음(신규 파일이 아닌 기존 호스트 장치 수정). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(실행 전후 `originals/` 해시 일치, 도구가 검증). 추적 `assets/game-data/d/options.cfg` 불변(`git status` 확인).
- 남은 것: Options·Help·Campaign 하위 메뉴 L4a 장면 추가와 차이 해소 → 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트·월드 조립).

#### 0.381 — Kencloud 구름 이식과 메인 메뉴 L4a 차이 0 (2026-11-11, HJOW-Athlon)

- 범위: 0.4 순서 3(R3 마무리)의 첫 비교(0.380)에서 남은 배경 구름 차이 37,054픽셀 닫기. 방법·원인·구조는 [dotnet-l4a-menu-cloud-20261111.md](docs/dotnet-l4a-menu-cloud-20261111.md)에 따로 적었다.
- 옮긴 cpppj 파일(기준 커밋 `3fc1de7db`, 원문 변경 없음): `o/RawKencloud.cpp/.h`(파일 부모 00470bb0·생성 00471040·행 이동 00470b20) → `Netstorm.Core/Original/RawKencloud.cs`(L1), `o/RawKencloudRender.cpp/.h`(갱신·dirty·행 조회·표시 두 부모) → `RawKencloudRender.cs`(L1), `client/ClientKencloudGlobals.cpp/.h`(메모리·파일·설정·화면 연결, 팔레트 전환) → `Netstorm.Client/Display/ClientKencloudGlobals.cs`(port). `HostStartupScene`은 512×512 타일 반복 대신 시작 팔레트(clear 1)→`ChangeFortPalette`(구름 생성+`cloudSpeed`번 이동+GamePalSpec 팔레트)→`MarkDirty`→`DrawKencloud`→`CleanDirty`→타이틀 순서로 바꿨고 생성자에 게임 난수 인자를 받는다(구름이 난수 801번을 소비). `ClientHostGame`은 시작 화면의 같은 `GameRandom`을 상태 덤프·세션에 쓴다.
- 검사: Core `RawKencloudTests`(`kencloud-v2-x86.tsv` 246행 전수: 정상 208·부분 중단 38, 같은 난수·할당 3개·인접 전역 8216바이트·파일 scratch 264바이트·호출 순서)·`RawKencloudRenderTests`(`kencloud-render-x86.tsv` 944행 전수 + 자연 생략·물리 접두) 4건, Client `ClientKencloudGlobalsTests` 10건(물리 전역·주소 수명·실제 파일·fallback 2·fury 0 접두·옵션·메뉴 팔레트 전환·타일·부분 표시·alternate 거부). fixture 2개를 `dotnetpj/tests/Netstorm.Core.Tests`에 링크하고 `cpp_sync.py --update --add`로 등록(연결 236개).
- 전체 회귀: `dotnet test dotnetpj/Netstorm.sln -c Release`(`NETSTORM_DATA`=`assets/game-data`) 재완주 — Assets 366·Core 1,938(+4)·Client 1,085(+10), 합계 3,389, 실패 0. 0.369 대비 증가분이 새 검사 14건과 같다.
- 변이: 행 이동 묶음 증가량·위상 마스크·부분 표시 플래그(Core 3건), 메뉴 팔레트 전환 제거·부분 표시 원본 행(Client 2건)을 검출했다. 부분 표시 원본 행 변이는 처음 검출되지 않았다(전체 경로에서는 원본 행이 0이라 구별되지 않음) → 부분 표시 검사 `PartialDrawBlitsOnlyDirtyRowsInEveryTile`을 더해 닫았다. 모두 원복 후 재통과.
- L4a: `l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-main.tsv -Name menu-main`(cpppj 오라클 재실행 포함)이 3프레임 모두 **차이 0**, 프레임 120의 두 BMP SHA-256 `E2E20AB6134A6A10…`로 같다(0.380의 38,793픽셀에서 0).
- C# 모양 차이: 전역 참조 묶음→같은 배열을 빌리는 `KencloudGlobals`(속성), 구름 이름 버퍼→`MemoryManager<byte>`, 미초기화 힙 복사→0 채움(교체 가능), `ReadBackgroundGifFile`→`GameFileSystem`+`DecodeBackgroundGif`(정상 경로 동일), 색 표 갱신·진단 로그 생략, `BindRenderer`는 값만(R5). 자세한 표는 설명 문서 4절.
- 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음(구름 파일은 로컬 자산).
- 보호: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(실행 전후 `originals/` 해시 일치, 추적 `assets/game-data/d/options.cfg` 불변).
- 도구 메모: `cpp_sync.py --ports-add`는 PORT_MAP.tsv 전체를 다시 정렬해 300줄 가까운 churn을 만든다. 되돌리고 6행을 끝에 직접 덧붙였다(이 도구는 앞으로 같은 방식으로 쓴다). `--ports` 보고는 시간 초과라 쓰지 않는다.
- 남은 것: 하위 메뉴(Credits·Options·Help·Campaign 등) L4a 장면 추가와 차이 해소 → 이정표 D1, 그 뒤 0.4 순서 2(R4 실제 호스트·월드 조립). R5에서 `Renderer`·카메라 연결 때 `RendererToken`·`WorldRedraw`·`PrepareRendererFrame`의 구름 갱신 호출을 옮긴다.

#### 0.380 — 오라클 갱신(1-4)과 첫 L4a 비교, 메인 메뉴 버튼 높이 교정 (2026-11-11, HJOW-Athlon)

- 범위: 0.4 순서 1-4(오라클 갱신) + 순서 3(R3 마무리)의 첫 캡처 비교. 지금까지 코드를 바꾸지 않은 대조 단위(0.368~0.379)와 달리, 실행 검사가 가능한 예외 PC(HJOW-Athlon, 192.168.0.94)에서 cpppj와 dotnetpj를 처음으로 같은 ui-script로 돌려 화면을 비교했다.
- 오라클: `cpppj/build/bin/Release/Netstorm.exe` SHA-256 `09042F35637459793784D46D4489B524D8E412C52294F7640FFFFDE5B2A8E996`(11:20:25 빌드, cpppj `src` 마지막 커밋 `5b4b80874` 11:20:54와 같은 빌드). 데이터 사본 `extracted/dotnet-oracle-data/`는 `originals/` 1,394파일 복사에 클론 `assets/game-data/cursors/`를 더한 것이다(dotnetpj 호스트가 `cursors/`를 읽는다. 없으면 `원본 커서 데이터가 없습니다: 8`로 끝난다). 실행 전후 `originals/` 해시 목록 일치.
- 첫 비교(1024×768, `--fixed-frame-ms 25`, 프레임 10·40·120의 메인 메뉴): **38,793픽셀(4.93%) 다름.** 세 프레임이 서로 바이트 동일해 메뉴는 이 구간에서 정지한다. 원인은 둘이었다.
  1. **버튼 높이(코드 결함, 수정함).** cpppj는 컨트롤 영역을 `{x, y, x+75, y+kButtonHeight+1}`로 만들고(`UberGump.cpp` 2019행) 돌 버튼을 그 크기(75×**20**)로 구성한다(`RawButtonControls.cpp` 294행). `HostMainMenu`는 75×19였다. 버튼 8개 모두 아래 테두리가 한 줄 위에 있었다(3행 폭 차이, 채널 최대 차이 145). 20으로 바꿔 이 차이는 0이 됐다.
  2. **배경 구름(미이식, 남음).** GIF 바깥 37,054픽셀(채널 최대 차이 17)은 모두 구름이다. cpppj는 팔레트 전환(`ClientMain.cpp` 1103행 `Transition`)에서 구름 파일을 읽어 `GameRandom`으로 씨앗 800개·`spacer` 1개를 뽑고(`BuildKencloud`) `cloudSpeed`(=2)번 `StepKencloud`를 돌린다. 한 번 이동마다 `phaseRows[phase]`로 시작하는 16행 간격 구간에서 989바이트(약 1.93행)를 왼쪽으로 1바이트 밀어(`memmove`) 행 띠 24개(3행씩)가 타일에서 1픽셀 어긋난다. dotnetpj `HostStartupScene.Draw`는 이 단계 없이 512×512 구름을 원점 0에서 반복해 깔 뿐이다. 행별 어긋남 검사로 확인했다(차이 행 중 24개는 정확히 −1픽셀 이동, 나머지는 띠 가장자리). 640×480 영역 안쪽 x=831 한 열에도 차이가 있었는데 `titleMenu.gif`가 639×480이라 그 열이 GIF 밖(구름)이기 때문이며 같은 원인이다. 이것은 다음 단위 0.381에서 Kencloud를 옮겨 닫는다.
- 구현: `dotnetpj/src/Netstorm.Client/Host/HostMainMenu.cs`(버튼 높이 19→20, 근거 주석), `dotnetpj/tests/Netstorm.Client.Tests/HostStartupSceneTests.cs`(`ScriptLocatorUsesLiveMenuButtons` 기대 좌표 (393,320)→(393,321): cpppj `ControlPoint`는 컨트롤 영역 `{356,311,431,331}`의 중심이다), 새 도구 `dotnetpj/tools/l4a_compare.ps1`(양쪽 실행·`originals/` 해시 검증·`cpp_frame_compare.py --dir` 비교, 종료 코드 0/1/2)와 첫 장면 `dotnetpj/tools/l4a/menu-main.tsv`.
- C# 모양 차이: 없음(상수 교정).
- 검증(게임 실행 있음, 예외 PC): `HostStartupSceneTests`·`GumpButtonTreeTests` 28건 통과(실패 0; 처음 실행에서 위 기대값 1건 실패 → 오라클 근거로 교정 후 통과). 도구 전체 실행 `l4a_compare.ps1 ... -Build`가 종료 코드 1과 같은 37,054픽셀 차이를 재현해 도구와 결과를 모두 확인했다. 전체 회귀는 하지 않았다(HostMainMenu 한 상수와 기대값 한 줄).
- PORT_MAP·fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(실행 전후 `originals/` 해시 일치). 호스트가 쓰는 `d/options.cfg`는 데이터 사본(`extracted/dotnet-oracle-data/d/`)에만 쓰였고 추적 파일 `assets/game-data/d/options.cfg`는 바뀌지 않았다(`git status` 확인).
- 남은 것: 0.381 Kencloud 이식(위 2번)으로 메인 메뉴 차이 0 달성 → Credits·Options·Help·Campaign 같은 하위 메뉴 장면 추가 → 0.4 순서 2(R4 실제 호스트·월드 조립).

#### 0.379 — 영향 재검토 범위 종결과 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 0.4 순서 1-2의 게임 실행 없는 범위 종결. 코드 변경은 없다.
- 확인: `922f8abcf..HEAD`에 `cpppj/src` 신규 변경이 없어(`git log` 빈 결과) 재검토 범위가 그대로임을 확인했다. 31항목 처분 현황: 완료 22(4·5·6·7·8·9·10·11·14·15·18·19·20·21·22·23·24·25·26·27·28·29)·부분 1(1번, 14차 QuickJS 잔여)·미착수 8(2 R2·R8·3 R1·12 F3·R4·13 R3·16·17 R4·R6·30 R4·31 10단계). 각 단위 기록(0.344~0.378)과 재검토 문서 1·4절이 항목별 근거를 갖고 있다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 범위 신선도 확인(`git log` 빈 결과). 새 검사는 돌리지 않았다(코드 변경 없음, 0.369 전체 기준선 유효).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 0.4 순서 1-4(오라클 갱신, 게임 실행 필요)·2~10단계(R4 조립부터는 실제 호스트·월드 연결과 L2·L4a 비교가 필요해 게임 실행 검사와 함께 진행).

#### 0.378 — 호스트 루프 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 3번(`ClientMain.cpp`, R1 호스트 루프) 구간(1247→HEAD) 실변경 대조. 코드 변경은 없다.
- 확인: 표면 통계 2966줄 중 줄바꿈 복원을 걷어내면 실변경 121+/13-(11커밋 중 96번은 순수 복원)이며 전부 처분했다. 소용돌이 장치(88)·지연 FIFO 3종(95)·`testBattleAddress`(108, R7)·`DebugBurst`(114, 10단계)·`helpWord`(120, R3)·ErrorLog·어설션 20줄(121, R8 7차)·K1 61줄(122, R3+R8 6차)·일본어 12줄(149, R8 13차)·F1 후처리(164)·F2 양보 대기(166, 0.4 8단계). C# 호스트 루프(`HostFrameDriver`·`HostOptions`·`ScreenCapture`)는 R1에서 HEAD 기준으로 통째로 옮긴다. 기존 검사 `InputQueueTests` 4건 통과(실패 0)로 인접 입력 경로 회귀 없음을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 4건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.377과 같음. 3번은 호스트 루프라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.377 — 세션 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 12번(`RawCampaignSession.cpp/.h`, F3 세션) 구간(1247→HEAD) 전수 대조. 코드 변경은 없다.
- 확인: 구간 내 변경은 13커밋(565+/133-)이며 전부 처분했다. Native 고정 수신기(97/98/101/103/105/108) → 3차·최후순위. Dude 후처리(112)·무기 도표(88)·M6-n Fort 채우기(133) → R4 월드 조립·AI 나머지. NB18(156, 운반체 사제 줍기 거부) → R4 AI 관문과 함께(10차). 11차 난이도 AI 설정(157) → R4 AI 나머지+R8 11차(`AiDifficulty` 기반 0.347 완료, 세션 배율 필드 후속). 12차 스커미시 설정 화면(158)·스커미시 월드 MP 시작·NB23 종료(169)·NB04/NB29(162) → R4 세션+R8 12차. C#은 관문·소유 상태 위주 일부만 옮겨져 있어 F3 실제 조립(0.4 2단계)에서 HEAD 기준으로 통째로 재대조한다. 기존 검사 `RawCampaignSessionTests` 93건 통과(실패 0, `NETSTORM_DATA` 지정)로 회귀 없음을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 93건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.376과 같음. 12번은 F3 조립이라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.376 — 표면 저장소·월드 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 16번(`RawSurfaceStorage.cpp`)·17번(`RawSurfaceWorld.cpp/.h`, R4 월드 본체 조립) 구간(1247→HEAD) 전수 대조. 코드 변경은 없다.
- 확인: 구간 내 변경은 3커밋이다. ① `c102cdbf3`(88) 무기 도표(신전 정기 가상표·상승 본문 소유자). ② `86f26ac1d`(112) Dude·세 주문 효과 최초 후처리 본문. ③ `14fd017c3`(119) 자산 공용 본문(Geyser·Generator·Gun·Arc·Acid)+보호막 최초 후처리+소용돌이 배관. 필요한 본문·배관(`DudePostPop`·`BombPostPop`·`ResourceAssetPostPop`·`ForcefieldPostPop`·`VortexRegular`·`VortexState`·`BindVortex*`·`DudeSpawnState`·`AssetRegularState`)이 C#에 전무함을 `grep`으로 확인했다. R4 월드 조립(`RawSurfaceWorld`·`RawWorldCore` 소유 구조)과 R6 규칙 몸체(`RawSurfaceVortex` 등) 때 함께 옮긴다. 기존 검사 `RawSurfaceStorageTests`·`RawSurfaceWorldTests` 141건 통과(실패 0, `NETSTORM_DATA` 지정)로 회귀 없음을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 141건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.375와 같음. 16·17번은 본문·배관이 R4·R6이라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.375 — 지연 FIFO 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 31번(`LocalTcp/UdpTransport.cpp` + 신규 `RawPacketDelay.cpp/.h`, 최후순위 3차) 구간(1247→HEAD) 전수 대조. 코드 변경은 없다.
- 확인: 구간 내 변경은 단일 커밋(`ec0b6e973`, 95)뿐이다. 신규 지연 FIFO(`RawPacketDelay.cpp/.h` 75+33) + `LocalTcp` 12줄(포함·`Adopt` 해제 갈고리 큐 초기화·`Fail` 지연 가드·`ReceiveOne` 준비·펌프) + `LocalUdp` 4줄(포함 등)이다. C# `LocalTcp/UdpTransport.cs`는 소켓·분배기 없는 루프백 사전 구현(0.287)이라 실제 Poll 배선을 받을 수 없고, 큐 본체(`PreparePacketDelay`·`Pump`)는 분배기(`RawPacketDispatch`)와 함께 10단계에서 옮긴다(`PacketConnection`의 `DelayQueue`·`Delayed` 자리는 이미 있다). 기존 검사 `CppPortMapTests` 2건 통과(실패 0)로 매핑 정합을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 2건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음(MP 실행 검사도 하지 않는다). 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.374와 같음. 31번은 소켓·분배기가 10단계라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.374 — BitmapFont.cpp 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 2번(`BitmapFont.cpp`, R2 잔여·R8 13차) 구간(1247→HEAD) 대조. 코드 변경은 없다.
- 확인: 구간 내 변경은 `ExtendedAdvance()` 1줄뿐이다(일본어 글꼴이면 히라가나 'あ'`0x3042` 전각 폭, 아니면 '가'`0xAC00`). C# 3종(`Assets/BitmapFont`·`Display/BitmapFont`·`Host/WindowsBitmapFont`)에는 확장 경로 자체가 없다(`ConfigureExtended`·`ExtendedGlyph`·`ExtendedAdvance`·`HasExtended` 전무를 `grep`으로 확인. `HasExtendedDisplayText`는 `DisplayTextInput` 판별기뿐). 1줄만 옮길 수 없으므로 확장 글리프 기반 전체(`ConfigureExtended`·`ExtendedGlyph`·`ExtendedAdvance`·`Measure` 분기·`FontStore` 배선)와 함께 R2 잔여 표시 연결·R8 13차에서 옮긴다. 선행 기반(`ExtendedText` 0.350·`DisplayTextInput` 0.351)은 완료돼 있다. 기존 검사 `BitmapFontTests` 15건(Assets 10·Client 5) 통과(실패 0, `NETSTORM_DATA` 지정)로 회귀 없음을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 15건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.373과 같음. 2번은 확장 기반이 R2·R8이라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.373 — SquidPostPop.cpp 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 30번(`SquidPostPop.cpp`, R4) 구간(1247→HEAD) 전수 대조. 코드 변경은 없다.
- 확인: 구간 내 변경은 2커밋이나 순수 효과는 조건 1줄뿐이다. ① `86f26ac1d`(112) Dude·세 주문 효과 최초 후처리 장기 본문 연결이 같은 조건줄을 고쳤다. ② `14fd017c3`(119) 같은 줄을 다시 고쳐 최종 Dude 가상표 4개(`0x502c38`·`0x502cf0`·`0x502db0`·`0x507770`)가 `aiNotice` 분배에 들어갔다. C# `RawPostPop.cs`에는 `DispatchPostPop`·`aiNotice`·파생 분배 자체가 없어(`HandlesDerived` 항상 거짓, `PostPopVirtual`은 Player만) 상수만 옮길 수 없으므로 R4 월드 조립(`PostPopVirtual`·`DispatchPostPop` 분배) 때 함께 옮긴다. Dude 가상표 4개는 C# 어디에도 없다. 기존 검사 PostPop 계열 59건(`X86RawPostPopTests` 등, 기존 L1 포함) 통과(실패 0)로 회귀 없음을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 59건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.372와 같음. 30번은 분배 장치가 R4라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.372 — RawMenuControls.cpp 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 13번(`RawMenuControls.cpp`, R3 잔여)의 구간(1247→HEAD) 전수 대조. 코드 변경은 없다.
- 확인: 구간 내 변경은 4커밋뿐이며 전부 처분했다. ① `c551d159d`(120) `HelpText` helpWord NULL 분기 → R3 잔여(도움말 소유자 층; C#은 `InvalidateCache`만 0.349). ② `1d5b1f437`(122) `DisplayTextInput.h` 포함(빌드 경계; C# 장치는 0.351 존재라 조치 없음) + `InvalidateHelpLanguage` → R3 잔여(같은 소유자 층). ③ `237c86314`(128) 주석-only → 조치 없음. ④ `68a9545e2`(132) Scenario Enter 삼키기 → 0.365(`IsScenarioEnter`, 15번 파일 경유) 완료, `ShowScenarioInputDialog` 소유자 포장 → R7(2.5). 대응행(`HtmlBodyTextParser.cs`의 `RenderEmbeddedTell` 부분)은 구간 변경이 없어 `base`를 유지한다. 기존 검사 `QuickHelpWindowTests`·`SaveFortDialogScenarioTests` 17건 통과(실패 0, `NETSTORM_DATA` 지정)로 회귀 없음을 확인했다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 17건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(0.371과 같음. 13번은 소유자 층이 R3 잔여라 미착수 유지), 0.4 순서 1-4(보류).

#### 0.371 — SaveFort 확인행 재대조 완료와 항목 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 15번(`RawMenuSaveFort.inc`, R3·R7) 확정. 코드 변경은 없다.
- 확인: 구간 내 변경은 단일 커밋(`68a9545e2`, 5+/2-, 3헝크)뿐이며 전부 처분했다. ① `editorAccept` 분기 → 0.365(`TryAcceptScenario`·`AcceptWithScenario`·`ReadEntryText`) 완료. ② `reportError` 포장 → 0.365(`AcceptWithReport`) 완료. ③ `editorCancel` 분기(+`tell` 조회 정리) → 0.365(`TryCancelScenario`·`CancelWithScenario`) 완료. `battleDescription`·`htmlInput`·`sendMoney`행은 구간 내 변경이 없는 기존 분기이며, `ShowScenarioInputDialog` 소유자 포장은 `RawMenuControls.cpp`(13번) 쪽이라 2.5 R7에서 다룬다. 확인행(`HostFortNameConfirmation.cs`, 일반 확인 경로)은 구간 변경이 없으므로 `base`를 `68a9545e2`로 올렸다. 기존 검사 SaveFort 확인 18건(`HostFortNameConfirmationTests`·`SaveFortNameTests`·`SaveFortNameDialogTests`·`SaveFortDialogScenarioTests`) 통과(실패 0, `NETSTORM_DATA` 지정)로 회귀 없음을 확인했다.
- `PORT_MAP.tsv` 1행(`HostFortNameConfirmation.cs`) `base` 갱신. fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(9차).
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 18건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 22·부분 1·미착수 8(완료에 15번 추가. 완료: 4·5·6·7·8·9·10·11·14·15·18·19·20·21·22·23·24·25·26·27·28·29, 부분: 1, 미착수: 2·3·12·13·16·17·30·31), 0.4 순서 1-4(보류).

#### 0.370 — main.cpp 구간 대조와 잔여 확정 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 1번(`main.cpp`)의 구간(1247→HEAD) 전수 대조. 코드 변경은 없다.
- 확인: 구간 내 `main.cpp`를 건드린 커밋은 3건뿐이며(27+1+22 삽입·2 삭제 = 구간 통계 50+/2-와 일치) 전부 처분했다. ① `d7fc71b0e`(121) ErrorLog 27+/2- → 0.359(`UserDataRoot`)·0.360(`ErrorLog`)·0.361(호스트 시작 배선) 완료. ② `1d5b1f437`(122) `text` 연산 1+ → 0.351(이식)·0.364(`base` 갱신) 완료. ③ `5b4b80874`(184) QuickJS 22+(표시·자체 검사·`--quickjs-selftest`) → 14차 잔여([1.3](#dotnet-r8-requirements) 14차행: 계약 확정 뒤). 기존 검사 `HostErrorLogTests`·`DisplayTextInputTests`·`CaptureAndScriptTests` 18건 통과(실패 0, `NETSTORM_DATA` 지정)로 회귀 없음을 확인했다.
- 정합: 0.361 단위 기록의 "K1·M7.5·M6·M8 구간 잔여"는 1번행 4행 전체를 뭉뚱린 표현이었으므로 바로잡는다. 구간 내 `main.cpp` 잔여는 QuickJS뿐이다. M7.5 assertion 본체는 `ClientMain.cpp`(3번) 쪽이며, `HostFrameDriver`·`HostOptions`행의 구간 이전 staleness는 3번 호스트 루프(R1)에서 통째로 다룬다. 재검토 문서 1번행도 같은 내용으로 고쳤다.
- `PORT_MAP.tsv`·fixture 변화 없음(`base` 유지 — QuickJS 잔여). 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 18건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 21·부분 2·미착수 8(0.369와 같음. 부분: 1·15 — 1번의 구간 잔여는 14차 QuickJS로 좁혀졌다), 0.4 순서 1-4(보류).

#### 0.369 — 전체 회귀 재완주와 기준선 갱신 (2026-11-11, HJOW-Athlon)

- 범위: 0.4 순서 1-1의 기준선 갱신(0.3 규칙: 20단위마다 전체 완주. 0.345 이후 24단위 경과). 코드 변경은 없다.
- 확인: `dotnet test dotnetpj/Netstorm.sln -c Release` 완주 — Assets 366·Client 1,075·Core 1,934(합계 3,375, 실패 0). 첫 전체 실행에서 `NETSTORM_DATA`를 `originals/`로 잘못 지정해 Assets 21건(`CursorBitmapTests` 18·`GameDataLocatorTests` 1·`CampaignMenuTests` 1·`TypeAndConfigTests` 1)·Core 1건(`GameCursorTests`)이 실패했으나, 모두 "실제 게임 폴더의 실행 파일·사용자 진행 상태·부재한 추출 커서" 때문인 환경 원인임을 확인하고, 클론 데이터(`assets/game-data/`, `netstorm.tarc` 표지)로 Assets 전체·`GameCursorTests`를 다시 실행해 0건임을 확정했다(Client 전체 1,075건은 첫 실행에서 이미 통과). 0.3 기준선을 3,273(0.345) → 3,375로 새로 적었다.
- `PORT_MAP.tsv`·fixture 변화 없음. 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 전체 완주 1회 + 실패분 자산 지정 재실행 2회. 게임 창·오라클 실행 없음.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정(읽기만 함. `originals/`의 `options.cfg` 등 추적 파일 변경 없음). 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 21·부분 2·미착수 8(0.368과 같음), 0.4 순서 1-4(보류).

#### 0.368 — SquidPop 재대조 확정과 집계 정합 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 29번(`SquidPop.cpp`, R4 판본 분기 잔여) 확정. 코드 변경은 없다.
- 확인: `SquidPop.cpp`→`OriginalPopDisplay.cs`행 `base`(`7bf9248b`, 0.355)와 `SquidPop.cpp`→`RawPop.cs`행 `base`(`8f8ddf6c`, 0.356) 이후 해당 cpp 파일 변경이 없음을 `git diff <base>..HEAD`로 확인했다. 구간 내 변경 커밋(`14fd017c3`)이 두 `base`의 조상임을 확인했다. `SquidUnpop.cpp`는 구간 내 변경이 없어 두 관련행도 최신이다. 기존 검사 `X86RawPopTests`·`RawPopTests`·`X86RawUnpopTests` 13건 통과(실패 0, 기존 L1 포함)로 회귀 없음을 확인했다.
- 정합: 그간 단위 기록이 29번을 미착수로 세고 있었다(0.355·0.356에서 양행의 `base`를 이미 올렸으므로 완료가 맞다). 항목 기준으로 다시 세어 바로잡는다.
- `PORT_MAP.tsv`·fixture 변화 없음(`base` 이미 최신). 함께 옮긴 10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 기존 검사 13건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 21·부분 2·미착수 8(완료에 29번 추가. 완료: 4·5·6·7·8·9·10·11·14·18·19·20·21·22·23·24·25·26·27·28·29, 부분: 1·15, 미착수: 2·3·12·13·16·17·30·31), 0.4 순서 1-4(보류).

#### 0.367 — HTML 맵 단어 번역 배선 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 14번(`RawMenuHtml.inc`, R3) 잔여 전부. cpppj HEAD(`350641c48`)의 K4 맵 단어 번역 배선(`mapHooks.translate` 소유자 전달)을 C#에 반영했다.
- 구현: `HostHtmlFactory.BindMapCatalog`에 `translate` 선택 인자 추가와 `TellMapMenus.Bind` 전달. `TellMapMenusTests`에 `BindForwardsTranslateDevice`(전달·단어 매핑·없음 3단정) 추가. `PORT_MAP.tsv`의 `RawMenuHtml.inc` 2행(`GumpModal.cs`·`HtmlWindowTextParser.cs`) `base`를 `350641c48`로 올렸다(L1/port 유지).
- C# 모양 차이: cpppj의 `hooks.translate ? ... : identity` 삼항을 선택 인자 기본값(`null`이면 `Word` 내부 영어 그대로)으로 옮겼다. 유일한 호출자(`ClientHostGame`)는 인자 없이 불러 영어 동작 그대로다(결정 D4: 검증은 영어).
- 검증(게임 실행 없음, 이번 목표 지시): 깨끗한 빌드(경고 0·오류 0) 뒤 `vstest`로 `TellMapMenusTests` 8건 통과(실패 0). 번역 전달 무력화 변이(`Translate = null`)에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 8건 통과했다.
- fixture 변화 없음(L1 행). 함께 옮긴 10차 수정 없음(K4; NB08은 0.348).
- 부분 재대조로 남긴 것: 없음 — 14번 완료. 실제 번역표(xlat) 연결은 R3 잔여 소유자에서 잇고 종단 확인은 이정표 D1 캡처 비교에서 한다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 20·부분 2·미착수 9(완료에 14번 추가. 완료: 4·5·6·7·8·9·10·11·14·18·19·20·21·22·23·24·25·26·27·28, 부분: 1·15, 미착수: 2·3·12·13·16·17·29·30·31), 0.4 순서 1-4(보류).

#### 0.366 — 연결 활성 Native 대여 판정 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 26번(`RawConnectionActivity.cpp`, 3차 사전 구현). cpppj HEAD(`eeae45c69`)의 Native 대여 분기(같은 metadata면 같은 값만 빌리고 독립 등록을 유지)를 C#에 반영했다.
- 구현: `Netstorm.Net/RawConnectionActivity.cs`에 `ConnectionActivityRegistration`(타입 0x64·주소 0x004b6180·버전 1·불일치 0·길이 5 상수와 `TryBorrowTimerReceiver`) 추가. 새 검사 `ConnectionActivityRegistrationTests.cs`(대여·없음·다른 주소·버전·불일치·길이·관찰기 5종·표 없음 9건). `Core.Tests`에 `Netstorm.Net` 검사 참조 추가. `PORT_MAP.tsv`의 `RawConnectionActivity.cpp`행 `base`를 `eeae45c69`로 올렸다(port 유지, `.h`행은 그대로).
- C# 모양 차이: cpppj의 분배기 참조(`RawPacketDispatch&`)·`logic_error`를 표(`PacketRegistrationTable`)·`InvalidOperationException`으로 옮겼다. `RegisterCanonical`·`Observe`·`Version`·`Register` 호출은 분배기 본체가 없어 3차 로컬 구현 때 잇는다.
- 검증(게임 실행 없음, 이번 목표 지시): 깨끗한 빌드(경고 0·오류 0) 뒤 `vstest`로 신규 9건 통과(실패 0). Native 주소 무력화 변이에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 9건 통과했다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(M6-n Native 연결).
- 부분 재대조로 남긴 것: 대여 실패 뒤의 `Observe`·`Version`·`Register`와 대여 성공 뒤의 `RegisterCanonical` 호출(분배기 본체와 함께 최후순위 10단계).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 19·부분 3·미착수 9(완료에 26번 추가. 완료: 4·5·6·7·8·9·10·11·18·19·20·21·22·23·24·25·26·27·28, 부분: 1·14·15, 미착수: 2·3·12·13·16·17·29·30·31), 0.4 순서 1-4(보류).

#### 0.365 — SaveFort 시나리오 확정·취소·오류 문맥 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 15번(`RawMenuSaveFort.inc`, R3·R7) 중 S1 시나리오 분기. cpppj HEAD(`68a9545e2`)의 `editorAccept`·`editorCancel`·`reportError`·다중 줄 Enter 삼키기를 C# 소유자(`HostSaveFortDialog`)에 반영했다.
- 구현: 새 파일 `Host/SaveFortDialogContext.cs`(문맥 4종·`TryAcceptScenario`·`TryCancelScenario`·`AcceptWithReport`). `HostTextEntry.ReadEntryText`(전체 버퍼 Latin1 읽기, cpp 진단 문구) 추가. `HostSaveFortDialog`에 문맥표·`BindScenario`·`AcceptWithScenario`(시나리오 확정은 버퍼 본문, 일반은 오류 안내 감싸기)·`CancelWithScenario`(시나리오 취소는 예약)·`IsScenarioEnter`(다중 줄 Enter 삼키기)·해제 정리 추가. 새 검사 `SaveFortDialogScenarioTests.cs`(8건). `PORT_MAP.tsv`의 `HostSaveFortDialog.cs`행 `base`를 `68a9545e2`로 올렸다(부분·port 유지, `HostFortNameConfirmation.cs`행은 그대로).
- C# 모양 차이: cpppj의 `string_view` 바이트 본문·`std::function` 장치를 `string`(Latin1)·`Func`·`Action` 문맥으로 옮겼다. 표시 해독(`DecodeDisplayText`)은 R7 시나리오 호출자(UberScenario 상당)에서 하고 라우터는 원문 그대로 전달한다.
- 검증(게임 실행 없음, 이번 목표 지시): 깨끗한 빌드(경고 0·오류 0) 뒤 `vstest`로 신규 8건 통과(실패 0). 기존 SaveFort·이름 확인 10건 포함 합계 18건 통과(실패 0, `NETSTORM_DATA` 지정). 확정 거짓 무력화 변이에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 8건 통과했다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(9차).
- 부분 재대조로 남긴 것: `battleDescription`·`htmlInput`·`sendMoney` 형제 분기와 `ShowScenarioInputDialog` 소유자 포장(검증·클립보드 장치와 함께 R7 Edit 묶음) — 같은 파일의 S1 대화창 분리 잔여와 함께 2.5에서 처리한다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 18·부분 3·미착수 10(완료: 4·5·6·7·8·9·10·11·18·19·20·21·22·23·24·25·27·28, 부분: 1·14·15, 미착수: 2·3·12·13·16·17·26·29·30·31), 0.4 순서 1-4(보류).

#### 0.364 — UiScript 확정·매핑 기준 정합 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 1번(`main.cpp`→`UiScript.cs`) 잔여 중 `text` 연산 확정 + `PORT_MAP.tsv` 기준 정합 1건. 코드 변경은 없다.
- 확인: `main.cpp` 변경 중 `UiScriptRunner`에 닿는 것은 `text` 연산뿐이며, 0.351에서 이식·검사(`TextScriptOperation`)를 마쳤다. `CaptureAndScriptTests`·`DisplayTextInputTests` 합계 16건 통과(실패 0)로 회귀 없음을 확인했다. `main.cpp`행 중 Program·HostFrameDriver·HostOptions행은 ErrorLog·QuickJS·M-계열 구간 잔여로 그대로 둔다.
- 정합: `RawConnectionActivity.cpp`→`RawConnectionActivity.cs`행의 `base`(`ee4e45c6…`)가 저장소에 없는 커밋이라, 파일 생성 단위(0.126 `70ba25f1a`)로 되돌렸다. 이렇게 해야 `cpp_sync` 식의 기준 검사가 해당 행을 미반영으로 올바로 잡는다. 내용은 바꾸지 않았다.
- `PORT_MAP.tsv` 2행 수정.
- C# 모양 차이·fixture 변화·10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest` 16건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 18·부분 2·미착수 11(그간 기록의 완료 수가 어긋나 있었으므로 항목별로 다시 세어 바로잡는다. 완료: 4·5·6·7·8·9·10·11·18·19·20·21·22·23·24·25·27·28, 부분: 1·14, 미착수: 2·3·12·13·15·16·17·26·29·30·31), 0.4 순서 1-4(보류).

#### 0.363 — TellMapMenus 스크롤·무제한 목록 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 21번(`TellMapMenus.cpp`, R7·5차). cpppj HEAD의 50행 창 스크롤·이동행·번역·유지 캐시를 C#에 반영했다.
- 구현: `TellMapMenuHooks`에 창·유지·이동·번역 6장치와 `MapListPageRows`(50) 추가, `Append`에 페이징 분기(창 맞춤·절반 분할·이전·다음 문구) 추가. `SoloMapListState`를 가변(`Names`·`Flags` 리스트, `Limit` 기본 50, `WindowFirst`·`KeepCache`)으로 바꾸고 `Refresh`에 한도·증설·축소를 넣었다. `Bind`에 창 배선과 이동행(`ScrollMapList` 명령) 연결. `FortSelection.ScrollMapList`(0x02000134) 상수 추가. 검사 4건(문구·절단·번역·유지·묶음) 추가와 기존 호출처 2건(`IndexOf`) 수정. `PORT_MAP.tsv`의 `TellMapMenus.cpp`행 `base`를 `fdb9a2b9b`로 올리고 `RawSoloMapList.h`·`FortSelection.h` 2행을 새로 등록했다(L1 1·port 2).
- C# 모양 차이: cpppj의 벡터 상태·`std::clamp`·`to_string`을 리스트·`Math.Clamp`·불변 문자열로 옮겼다. 이동 행 선택 소비는 메뉴 소유자(R7 Edit 목록)에서 잇는다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `TellMapMenusTests`·`SoloMapCatalogTests`·`SoloMapCatalogStorageTests` 합계 17건 통과(실패 0, `NETSTORM_DATA` 자산 지정). 임시 경로 자산 부재 실패 3건을 확인하고 자산 지정으로 해결했다(코드 무관). 이동행 변이에서 2건 실패로 검출력을 확인한 뒤 원복하고 다시 17건 통과했다. L1 행 변경(배열→리스트)의 기존 호출처는 컴파일러가 잡아 고쳤다.
- fixture 변화 없음(L1 행 유지). 함께 옮긴 10차 수정 없음(5차).
- 남은 것: 재검토 문서 1절 31항목 중 완료 17·부분 3·미착수 11, 0.4 순서 1-4(보류).

#### 0.362 — 무영향 대응행 2차 확인과 base 갱신 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 변경이 닿지 않는 대응행 4행. 코드 변경 없이 `base`만 올린다.
- 확인: `EditorCatalogLedger.cs`·`EditorWorkspacePaths.cs`는 공백·`Deleted` 변경과 무관함을 확인했다. `GumpModal.cs`·`HtmlWindowTextParser.cs`는 `.inc` 변경 기호와 무관함을 확인했다. 별도 출력 경로 35건(`EditorCatalogWorkspaceTests`·`EditorFilesDeviceTests`·`HtmlWindowFactoryNativeTests`·`HostStartupSceneTests` 일부) 통과(실패 0).
- `PORT_MAP.tsv` 4행의 `base`를 `996012399`으로 올렸다.
- C# 모양 차이·fixture 변화·10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 임시 경로에 자산이 없어 `HostStartupSceneTests` 24건이 실패한 것을 확인하고, `NETSTORM_DATA` 지정 재실행과 정상 bin 대조(24건 통과)로 환경 원인임을 확정했다. 실제 검증은 자산 지정 35건 통과다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 16·부분 3·미착수 12, 0.4 순서 1-4(보류).

#### 0.361 — 호스트 시작 배선과 검사 (2026-11-11, HJOW-Athlon)

- 범위: 7차 시작 배선(`app/main.cpp`의 `SetupErrorLog`·최상위 실패·`--log-crash-test`). `main.cpp`행은 다른 구간 잔여로 `base`를 올리지 않는다.
- 구현: 새 파일 `HostErrorLog.cs`(시작 구성·치명 기록·미지 형식, 게임 실행 없이 검사되게 진입과 분리). `Program.cs`에 시작 호출·`--log-crash-test` 분기·최상위 기록 전달·인자 설명 1줄 추가. 새 검사 `HostErrorLogTests.cs`(구성·치명 경로 2건).
- C# 모양 차이: cpppj의 진입 내 자유 함수를 검사 가능한 공개 정적 클래스로 옮겼다. 비 CLS 예외용 typeless catch는 경고 억제와 함께 유지한다.
- 검증(게임 실행 없음, 이번 목표 지시): 깨끗한 빌드(경고 0·오류 0) 뒤 `vstest`로 3종 합계 14건 통과(실패 0)를 2회 반복 확인했다. 과정에서 병렬 실행 간섭 2건을 확인하고 고쳤다: ① 타 검사와 전역·콘솔을 공유해 깨지던 것을 직렬 컬렉션(`ErrorLogSerial`)으로 묶었다. ② 임시 경로 자산 부재 1건은 제외 사유를 기록했다. ③ 중첩 `try` 구조 오류 1건을 컴파일러가 잡아 고쳤다.
- fixture 변화 없음. 함께 옮긴 10차 수정 없음(7차).
- 다음 단위: 미착수 12항목 중 `ErrorLog` 호스트 배선 잔여 없음 — `RawCampaignSession`·`ClientMain` 대형 또는 `TellMapMenus`·`RawSurface` 묶음으로 복귀한다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음(`--log-crash-test` 종료 종류도 실행하지 않았다). 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 16·부분 3·미착수 12, 0.4 순서 1-4(보류).

#### 0.360 — ErrorLog 신규 이식과 검사 (2026-11-11, HJOW-Athlon)

- 범위: 7차 오류 로그 공통 구조(`platform/ErrorLog.h/.cpp` 508줄). 호스트 시작 배선은 다음 단위에서 잇는다.
- 구현: 새 파일 `dotnetpj/src/Netstorm.Client.Host/ErrorLog.cs`(수준·1024줄 링·4096자 절단·지연 파일 생성(BOM·머리말·링 선기록)·오류 이상 디스크 반영·예외 풀이(8깊이)·어설션(환경 창)·치명·처리되지 않은 예외 갈고리·표준 스트림 복제·`FileNameFor`·`ResetForTesting`·`CrashForTest`). 새 검사 `ErrorLogTests.cs`(이름 규칙·지연 생성·예외·어설션·재구성·갈고리 1회·스트림 복제·throw/인자오류 9건). `PORT_MAP.tsv`에 2행 추가(port).
- C# 모양 차이: cpppj의 Win32 SEH·CRT 갈고리·`streambuf`·`HANDLE`을 .NET 처리되지 않은 예외·`TextWriter` 분기·`FileStream` 공유 읽기로 옮겼다. 줄 끝은 `\n` 고정(원문과 같음). 관리 버퍼는 매번 비우고 OS 반영은 오류 이상에서만 한다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `ErrorLogTests`·`UserDataRootTests` 합계 12건 통과(실패 0). 검사 작성 중 열린 파일 공유 위반 2건을 확인하고, 구현에 매 기록 플러시와 검사에 공유 읽기 헬퍼를 넣어 해결했다(원문 `WriteAll` 즉시 기록과 같은 의미). 경고 임계 변이에서 2건 실패로 검출력을 확인한 뒤 원복하고 다시 12건 통과했다. 프로세스를 끝내는 종류(terminate·access·divzero·abort·stack)는 별도 프로세스 손 확인 대상이라 검사하지 않는다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(7차).
- 다음 단위: 호스트 시작 배선(`Configure`·갈고리·스트림 복제·최상위 실패 기록·`--log-crash-test`, main.cpp 잔여).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 16·부분 3·미착수 12, 0.4 순서 1-4(보류).

#### 0.359 — UserDataRoot 신규 이식과 검사 (2026-11-11, HJOW-Athlon)

- 범위: 5차 사용자 홈 루트·7차 로그 폴더의 경로 계산(`platform/UserDataRoot.h/.cpp`). 실제 파일 이관·로그 기록은 후속 단위에서 잇는다.
- 구현: 새 파일 `dotnetpj/src/Netstorm.Client.Host/UserDataRoot.cs`(폴더·파일 이름 상수·`Resolve`(재정의→Saved Games→USERPROFILE→LOCALAPPDATA, Linux는 `$HOME/.netstorm`)·`EnsureDirectory`). 새 검사 `UserDataRootTests.cs`(이름 규격·재정의 우선·생성/거부 3건). `PORT_MAP.tsv`에 2행 추가(port).
- C# 모양 차이: cpppj의 `filesystem::path`·`SHGetKnownFolderPath`·`mutex` 불필요분을 `string`·`Path`·`DllImport`로 옮겼다. 설치 폴더로 되돌아가지 않고 실패하면 예외라는 계약은 그대로 둔다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 3건 통과(실패 0). 실제 홈 대신 재정의·임시 폴더만 썼다. 폴더명 변이(`logs`→`Logs`)에서 1건 실패로 검출력 확인 후 원복하고 다시 3건 통과했다. 처음에 호스트가 아닌 Client 프로젝트 경로에 파일을 잘못 만들어 빌드에서 빠진 것을 확인하고 호스트 프로젝트 경로로 옮겼다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(5차 인프라).
- 다음 단위: `ErrorLog` 이식(7차 로그 기록·비정상 종료 갈고리)과 호스트 시작 배선.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 16·부분 3·미착수 12, 0.4 순서 1-4(보류).

#### 0.358 — ProductPresentation 언어·스커미시 항목 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 10번(`ProductPresentation.cpp/.h`, R8 선행). cpppj HEAD의 한·일 고정 본문·스커미시 목록 항목·언어 번호 인자를 C#에 반영했다.
- 구현: `ProductPresentation.cs`에 한·일 본문 상수·언어 번호 상수(7·5)·`TellBody(target, original, language)` 언어·스커미시 분기·비교 helper 정리. `ProductPresentationTests.cs`에 3건 추가. `PORT_MAP.tsv` 2행 `base`를 `89498ca11`로 올렸다(port 유지).
- C# 모양 차이: cpppj의 `std::string` 제자리 수정·`string_view`·기본 인자를 `string` 복사·`uint` 선택 인자로 옮겼다. 기존 2인자 호출은 그대로 컴파일된다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 9건 통과(실패 0). 검사 초안이 상수 자체를 참조해 동어반복이 된 것을 변이(상수 7→8)가 검출하지 못해 깨닫고, 리터럴 7·5 단정으로 고친 뒤 변이 3건 실패를 확인하고 원복·재통과했다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(11·12·13차).
- 부분 재대조로 남긴 것: `[SetDifficulty]`·`[Skirmish]` Config 구간(난이도·스커미시 화면 단위에서 배선과 함께).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 16·부분 3·미착수 12 (완료에 10번 추가. 부분: 1·4·14), 0.4 순서 1-4(보류).

#### 0.357 — TellFileMenus 언어 전환·표시 부호화 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 20번(`TellFileMenus.cpp/.h`, R3) 잔여 전부. `InvalidateLanguageFiles`와 표시 경계 `Narrow`를 C#에 반영했다.
- 구현: `TellFileMenus.InvalidateLanguageFiles`(빈 언어 정리·목록 7언어·소유 Config만 재적재·등록 보존) 추가. `Narrow`·`Limit`·`FitDisplayLimit`을 표시 부호화(`ExtendedText.EncodeDisplayText`) 기준으로 고쳤다(cpppj와 같은 계약; ASCII·1252 범위는 기존과 같은 바이트). 새 검사 1건(영어→한국어→모르는 언어 전환). `PORT_MAP.tsv` 2행 `base`를 `a951ca7ed`로 올렸다(L1 유지).
- C# 모양 차이: cpppj의 `std::find` 언어 목록·`unique_ptr` 소유 정리·`LoadFile` 분기를 배열·`Dispose`·`Reader` 호출로 옮겼다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `TellFileMenusNativeTests` 5건 통과(실패 0, 기존 L1 384·288·672행 포함 — ASCII 동작 불변 확인). 실행 출력에서 한국어 제목이 Latin1 깨짐으로 나온 원인을 추적했더니 적재(`LoadBytes` BOM 규칙, 기존 계약)와 `Narrow`(1252 전용, 구형)의 두 층위였고, 한국어 파일에 BOM을 두고 `Narrow`를 cpppj와 같은 표시 부호화로 고쳐 해결했다. 표시 구형 변이(1252 복귀)에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 5건 통과했다.
- fixture 변화 없음(L1 행). 함께 옮긴 10차 수정 없음(언어 경계; NB24는 0.348).
- 남은 것: 재검토 문서 1절 31항목 중 완료 15·부분 4·미착수 12(완료에 20번 추가, 부분에서 제외), 0.4 순서 1-4(보류).

#### 0.356 — 무영향 대응행 확인과 base 갱신 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 47파일의 변경이 닿지 않는 대응행 5행. 코드 변경 없이 `base`만 올린다.
- 확인: `HostInputMapper.cs`는 순수 키 코드 변환이라 큐 API를 쓰지 않는다(`TryPushBatch`·`FreeSlots` 무사용 확인). `GuideMissionScriptLifetime.cs`는 생성·반납만 하고 `Get`·`Section`을 쓰지 않는다. `SaveFortName.cs`의 확인·취소 함수는 대화창 생성 변경과 무관하다. `RawPop.cs`는 표시 지원 분기 변경의 호출자라 기존 검사로 회귀만 확인한다. 각 기존 검사(`ClientHostTests`·`MissionGuideInputTests`·`SaveFortNameTests`·`RawPopTests`)를 돌려 Client 25건·Core 11건 통과(실패 0)를 확인했다.
- `PORT_MAP.tsv` 5행의 `base`를 `8f8ddf6c2`로 올렸다.
- C# 모양 차이·fixture 변화·10차 수정 없음.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest` 36건 통과.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절 31항목 중 완료 14·부분 5·미착수 12(그간 단위 기록의 "파일" 수는 행·대응행 혼용으로 어긋나 있었으므로 항목 기준으로 바로잡는다. 완료: 5·6·7·8·9·11·18·19·22·23·24·25·27·28, 부분: 1·4·10·14·20, 미착수: 2·3·12·13·15·16·17·21·26·29·30·31), 0.4 순서 1-4(보류).

#### 0.355 — 보조 상태·장부 삭제·Pop 표시 지원 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 28번(`RawGameAuxState.h`)·4번(`EditorMapCatalogFiles.cpp/.h`)·29번(`SquidPop.cpp`, R4·R7). cpppj HEAD의 Join 생성자 헤더·장부 삭제 표식·메타 Sprite 후처리 허용을 C#에 반영했다.
- 구현: `RawGameAuxState.BattleStartPeerHeader`(초기값 1b·1·38) 추가. `EditorCatalogWorkspace.Deleted`(완료 상태 2+작업본 부재) 추가. `OriginalPopDisplay` 판본 분기에 메타·일반 Sprite 두 가상 표 추가. 새 검사 `RawGameAuxStateTests.cs`, `EditorCatalogWorkspaceTests`·`X86RawPopTests`에 각 1건 추가. `PORT_MAP.tsv` 5행(`RawGameAuxState`·`EditorMapCatalogFiles`→Workspace 2·`SquidPop`→`OriginalPopDisplay`·`SquidUnpop`→`OriginalUnpopDisplay`) `base`를 `7bf9248b3`으로 올렸다.
- C# 모양 차이: cpppj의 `FormPacketHeader` 값 멤버·`entries` 맵 조회를 같은 레코드·장부 상태로 옮겼다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 Core 3건·Client 3건 통과(실패 0, 기존 L1 포함). 변이 검사에서 Pop 분기 제거가 검출되지 않아 조사했더니 C# `UnpopDisplay`에 sprite 분기 자체가 없는 옛 결함(2026-11-08 cpp 선행분 미이식) 때문임을 확인하고, 해당 분기도 함께 이식한 뒤 양쪽 변이(Pop 제거·Unpop 제거)가 각각 검출됨을 확인하고 원복·재통과했다. 되돌리기 과정에서 변이 원복 누락 1건을 실행 출력으로 즉시 잡아 복구했다.
- fixture 변화 없음. 함께 옮긴 10차 수정 없음(R4·M6-n·1267).
- 미룬 것(같은 묶음): `BitmapFont` 일본어 대표 폭(R2 잔여 표시 연결 뒤), `RawConnectionActivity`·`LocalTcp/Udp` 지연 대여(3차 `RawPacketDispatch`·`RawPacketDelay` 뒤), `SquidPostPop` 파생 분배(R4 월드 조립 때).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 33파일 재대조(후속 단위에서 파일 묶음 단위로, 6개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.354 — SaveFortNameDialog 시나리오 입력 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 18번(`SaveFortNameDialog.cpp`, R3·R7). cpppj HEAD의 S1 시나리오 입력(`ShowScenarioInput`)을 C#에 반영했다.
- 구현: `SaveFortNameDialogHooks.InputTitle` 추가와 `ShowScenarioInput`(상한 2..16384·화면 맞춤·다중 줄 형식·번역 제목·모달) 추가. `SaveFortNameDialogTests`에 theory 2건(범위 거부·기하·형식·번역) 추가. `PORT_MAP.tsv`의 `SaveFortNameDialog.cs`행 `base`를 `9d35b9a34`로 올렸다(port 유지, `SaveFortName.cs`행은 그대로).
- C# 모양 차이: cpppj의 자유 함수·`std::clamp`를 메서드·`Math.Clamp`로 옮겼다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `SaveFortNameDialogTests` 4건 통과(실패 0). 상한 변이(`< 2` → `< 1`)에서 2건 실패로 검출력을 확인한 뒤 원복하고 다시 4건 통과했다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(9차).
- 부분 재대조로 남긴 것: `SaveFortNameDialogContext`의 시나리오 4필드와 `RawMenuControls.ShowScenarioInputDialog`(R3 소유자 없음, R7 Edit 묶음) — 같은 파일의 S1 대화창 분리·`ConfigOverwriteAndQueueLifetime` 잔여와 함께 2.5에서 처리한다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 35파일 재대조(후속 단위에서 파일 묶음 단위로, 6개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.353 — HtmlTextParser·HtmlWindowFactory 확장 본문 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 5번(`HtmlTextParser.cpp/.h`)·6번(`HtmlWindowFactory.cpp/.h`, R3) + `RawMenuHtml.inc`의 `localizedContent` 배선. cpppj HEAD의 K1 확장 본문 경계를 C#에 반영했다.
- 구현: `HtmlTextParserGlobals.ExtendedText`·`HasExtendedAt`·`Parse` 분기(토큰 통째 보존·`AfterHeading` 0) 추가. `HtmlWindowFactoryHooks.LocalizedContent`·`Show` 조건(빈 장치면 ASCII 관문 유지) 추가. `HtmlWindowTextParser.Globals` 공개와 `HostHtmlFactory` 배선(`ExtendedText && HasExtendedDisplayText`). 새 검사 `HtmlExtendedContentTests.cs`(토큰 보존·관문 끄기·내용 인정 3건). `PORT_MAP.tsv` 4행 `base`를 `e99c641aa`로 올렸다(L1 유지).
- C# 모양 차이: cpppj의 바이트 `string_view` 검사는 Latin1 4문자 창을 바이트로 바꿔 같은 `DecodeExtendedSequence`로 판정한다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 신규 3건+파서 4건+공장 기존 3건 합계 10건 통과(실패 0). 자산 루트 필요 1건(`SharedBackground...`)은 임시 경로에 자산이 없어 제외했고 코드 변경과 무관하다. 검사 훅 설계 오류 1건(빈 문자열에 거짓을 돌려주는 훅)을 실행 출력으로 확인하고 고쳤다. 확장 무력화 변이에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다.
- fixture 변화 없음(L1 행). 함께 옮긴 10차 수정 없음(K1).
- 부분 재대조로 남긴 것: `RawMenuHtml.inc`의 맵 단어 번역(`translate` 훅, `TellMapMenus` 페이지 목록과 함께), `RawMenuControls` 전체 소유자·`SaveFortNameDialog` 시나리오 입력(R7 Edit 묶음), 공장 전역 켜기(`RawMenuControls.EnableTextEntry` 상당, R3 잔여).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 35파일 재대조(후속 단위에서 파일 묶음 단위로, 4개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.352 — TextEntryBuffer·TextEntryGump 확장 경계 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 22번(`TextEntryBuffer.cpp/.h`)·23번(`TextEntryGump.cpp`, R3). cpppj HEAD의 K1 확장 붙여넣기·커서·선택·삭제·스크롤·자동완성 경계를 C#에 반영했다.
- 구현: `TextEntryBuffer.Paste`에 `maxLength` 인자와 확장 분기(문자 경계 선택·최종 바이트 상한·완전 토큰 걸러내기) 추가. `TextEntryGump`에 `SetScroll` 줄 문자 보정·`Handle` 진입 정규화+`Previous`/`Next` 지역 함수·방향/End/삭제/붙여넣기/확정 입력/길이 판정·`HandleConsole` 단어 역탐색·`Draw` 줄 선택·스크롤 보정 추가. 새 검사 `KoreanTextEntryTests.cs`(커서·상한·붙여넣기·ASCII 상한·자동완성 5건). `PORT_MAP.tsv` 4행 `base`를 `84db5219f`로 올렸다(L1 유지).
- C# 모양 차이: cpppj의 `std::span`·람다·`std::string_view`를 `ReadOnlySpan`·지역 함수·`ReadOnlySpan` 뷰로 옮겼다. `using` 할당이 필요한 임시 본문은 미리 읽지 않고 호출 때마다 `Buffer(token)`를 다시 조회한다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `KoreanTextEntryTests`·`TextEntryStorageTests` 합계 13건 통과(실패 0). 검사 헬퍼가 표시 바이트를 그대로 Latin1으로 읽어 실패한 것을 확인하고 해독 헬퍼(`DecodeDisplayText`)로 고쳤다(구현이 맞았다). PowerShell 텍스트 치환이 한글 리터럴을 깨뜨린 것을 확인하고 `write` 도구로 파일을 다시 썼다(BOM 없음·LF 확인). 확장 무력화 변이에서 2건 실패로 검출력을 확인한 뒤 원복하고 다시 13건 통과했다.
- fixture 변화 없음(L1 행). 함께 옮긴 10차 수정 없음(K1).
- 미룬 것: 그리기·스크롤 픽셀 대조(검사 글꼴에 확장 글리프가 없어 R5 표시와 함께), `HtmlTextParser`·`HtmlWindowFactory` 확장 훅·`RawMenuControls` 입력 에이전트 훅(다음 단위).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 37파일 재대조(후속 단위에서 파일 묶음 단위로, 4개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.351 — DisplayTextInput 신규 이식과 text 동작·검사 (2026-11-11, HJOW-Athlon)

- 범위: 6차 입력 인프라(`client/DisplayTextInput.h/.cpp` 신규 39파일 중 2개) + `app/main.cpp`의 `text` 연산자 1개. `TextEntry` 확장 경계 이식의 선행 조건이다.
- 구현: 새 파일 `dotnetpj/src/Netstorm.Client/Input/DisplayTextInput.cs`(태그 상수·`IsCommittedTextInput`·`CommittedTextBytes`·`MakeCommittedTextEvents`·`CommittedUtf16ToUtf8`·`DisplayTextToUtf16`·`HasExtendedDisplayText`·문자 경계 4종·`DisplayTextLine`·`FilterDisplayText`). `UiScript.cs`에 `text` 동작(`MakeCommittedTextEvents`+`TryPushBatch`, 부족 시 예외). 새 검사 `DisplayTextInputTests.cs`(cpppj 두 경우 미러 + `text` 종단 1건, 3건). `PORT_MAP.tsv`에 2행 추가(port). `main.cpp`→`UiScript.cs`행은 `text` 1개만 반영했으므로 부분 재대조로 `base`를 올리지 않았다.
- C# 모양 차이: cpppj의 `std::string` UTF-8 입출력·`std::span`·`std::vector`를 `string`·`ReadOnlySpan`·`배열/List`로 옮겼다. `CommittedUtf16ToUtf8`은 바이트 배열로 돌려준다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `DisplayTextInputTests`·`InputQueueTests` 합계 7건 통과(실패 0). C# `\x` 탐욕 해석을 피하려고 DEL 바이트는 `(char)0x7f` 결합으로 적었다. 검증 무력화 변이에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 7건 통과했다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(6차 인프라).
- 다음 단위: `TextEntryBuffer`·`TextEntryGump` 확장 경계 + `HtmlTextParser`·`HtmlWindowFactory` 확장 훅 + `RawMenuControls` 입력 에이전트 훅.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 39파일 재대조(후속 단위에서 파일 묶음 단위로, 4개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.350 — 확장 문자 부호화 신규 이식과 검사 (2026-11-11, HJOW-Athlon)

- 범위: 6차·11차 표시 부호화 인프라(`o/OriginalText` 확장 부분). `TextEntry`·`HtmlTextParser`·`HtmlWindowFactory` 확장 훅 이식의 선행 조건이다.
- 구현: 새 파일 `dotnetpj/src/Netstorm.Core/Original/ExtendedText.cs`(숫자 표·6구간 표·동적 한자 표(잠금)·`EncodeExtendedCodePoint`·`DecodeExtendedSequence`·`IsExtendedTextDigit`·`DecodeDisplayText`·`EncodeDisplayText`). 새 검사 `dotnetpj/tests/Netstorm.Core.Tests/ExtendedTextTests.cs`(정적 인덱스 4건·비대상·깨진 시퀀스·숫자 집합·동적 왕복·표시 왕복·BOM·거부·cpp 혼합 배치·U+3400 거부, 12건). `PORT_MAP.tsv`에 `OriginalText.h/.cpp`→`ExtendedText.cs` 2행 추가(port). 기존 `Assets/OriginalText.cs` 행은 그대로 둔다.
- C# 모양 차이: cpppj의 `std::optional`·`std::span`·`std::vector`·`std::mutex`를 `nullable`·`ReadOnlySpan`·`배열/List`·`lock`으로 옮겼다. cpppj 자유 함수는 정적 클래스 메서드로, 익명 네임스페이스 표는 비공개 정적 멤버로 두었다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 12건 통과(실패 0). 기대 인덱스는 손계산 후 1건(히라가나 あ)을 실행 출력과 대조해 정정했다(구간 합산 오류였고 구현이 맞았다). cpppj `KoreanTextTests`의 혼합 배치 수식·오프셋·U+3400 거부·BOM을 그대로 미러링했다. 구간 제거 변이에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 12건 통과했다.
- fixture 변화 없음(port 행). 함께 옮긴 10차 수정 없음(6차·11차 인프라).
- 다음 단위: `DisplayTextInput` 이식(`HasExtendedDisplayText`·문자 경계·`FilterDisplayText`·확정 입력 사건), 이어서 `TextEntryBuffer`·`TextEntryGump` 확장 경계.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 39파일 재대조(후속 단위에서 파일 묶음 단위로, 3개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.349 — InputEvent·Mission·QuickHelpWindow K1 경계 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 8번(`InputEvent.cpp/.h`)·9번(`Mission.cpp/.h`)·11번(`QuickHelpWindow.cpp/.h`, R1·R3). cpppj HEAD의 K1 한국어 확정 본문·표시 분리·캐시 무효화를 C#에 반영했다.
- 구현: `Input/InputEvent.cs`에 `TryPushBatch`·`FreeSlots` 추가. `MissionScript.cs`에 `Get("title")`·`Section` 표시 분리 + `LocalizedDisplayScript`(익명 임시 Config, `using` 반납) 추가. `QuickHelpWindow.cs`에 `InvalidateCache` 추가. 새 검사 `MissionScriptDisplayTests.cs`(표시 3건), `InputQueueTests`·`QuickHelpWindowTests`에 각 1건 추가. `PORT_MAP.tsv` 6행(`InputEvent` 2·`Mission` 2·`QuickHelpWindow` 2) `base`를 `af7e11ff1`로 올렸다. 손대지 않은 대응행(`HostInputMapper`·`GuideMissionScriptLifetime`)은 그대로 둔다.
- C# 모양 차이: cpppj의 `std::span` 묶음 입력은 `ReadOnlySpan<InputEvent>`로, 자유 함수 `LocalizedDisplayScript`는 임시 객체 수명을 `using`으로 관리하는 비공개 메서드로 옮겼다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 14건 통과(실패 0). `MissionScript` 섹션 기대값은 실행 출력으로 확정했다(끝 섹션 뒤 `fileName`행이 붙으므로 `[End]` 구분자를 둔 자료에 `"BodyX.\n"`). 표시 분리 무력화 변이에서 1건 실패로 검출력을 확인한 뒤 원복하고 다시 14건 통과했다. 자산 루트가 필요한 `ActualShowUsesExistingHtmlScrollGlyphAndCommonLifetimeDevices` 1건은 임시 경로에 자산이 없어 제외했고, 코드 변경과 무관함(별도 확인 필요 시 정상 bin에서 실행한다).
- fixture 변화 없음. 함께 옮긴 10차 수정 없음(K1).
- 미룬 것(같은 묶음의 대형·의존 잔여): `TextEntryBuffer`·`TextEntryGump` 확장 문자 경계(신규 `DisplayTextInput` 인프라 이식 뒤 별도 단위), `TellMapMenus` K4 번역(페이지 목록 소유자 작업과 함께), `HtmlTextParser`·`HtmlWindowFactory` 확장 훅(같은 인프라 뒤).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 39파일 재대조(후속 단위에서 파일 묶음 단위로, 3개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.348 — HtmlWindowMetadata·TellFileMenus NB08·NB24 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 7번(`HtmlWindowMetadata.cpp/.h`)·20번(`TellFileMenus.cpp`, R3 목록 소유자) + `RawMenuHtml.inc`의 NB08 배선. cpppj HEAD의 NB08(145)·NB24(145) 교정을 C#에 반영했다.
- 구현: `HtmlWindowMetadata.cs`에 `RepairInlineList` 훅과 분기(켜면 합친 전체+NUL·넘는 꼬리만 절단, 끄면 원문 동작), `HostHtmlFactory.cs`에서 제품 배선 `RepairInlineList = true`(RawMenuHtml.inc 대응). `TellFileMenus.cs`에 `FitDisplayLimit`(서로게이트 보존 절단)과 `Title` 80바이트 절단 적용. 새 검사 `HtmlWindowMetadataRepairTests.cs`(전체 삽입·원문 절단+낡은 꼬리·넘침 절단 3건), `TellFileMenusNativeTests`에 79바이트 절단 Fact. `PORT_MAP.tsv`의 `HtmlWindowMetadata` 2행 `base`를 `e82018f5c`로 올렸다(L1 유지).
- C# 모양 차이: cpppj의 `repairInlineList` 불리언 훅·`FitDisplayLimit` 자유 함수를 같은 이름의 훅 속성·정적 메서드로 옮겼다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `HtmlWindowMetadataRepairTests`·`TellFileMenusNativeTests`·`HtmlWindowFactoryNativeTests` 합계 10건 통과(실패 0, 기존 L1 124·106행 포함 — 끄기 기본값 불변 확인). NB08 무력화 변이에서 2건 실패로 검출력을 확인한 뒤 원복하고 다시 10건 통과했다.旧 기대값 교정 1건(`OriginalCopiesOnlyReplacementLength`): 旧 경로는 NUL을 다시 쓰지 않아 낡은 꼬리가 남으므로 기대값을 원문 동작 그대로(`A\nL1\nLu=%list,Tell,X\nB\n`)로 고쳤다.
- fixture 변화 없음. 함께 옮긴 10차 수정: NB08·NB24.
- 부분 재대조로 남긴 것(같은 파일의 언어 표시 잔여, `base` 미갱신): `TellFileMenus.InvalidateLanguageFiles`(K4 언어 전환 재적재, R3 잔여), `RawMenuHtml.inc`의 맵 단어 번역·`localizedContent`(확장 문자 토큰, `DisplayTextInput`·`OriginalText.DecodeExtendedSequence` 이식 뒤), `TellMapMenus.cpp`의 Previous/Next/of 번역(`translate` 훅 배선과 함께). 결정 D4에 따라 검증은 영어로만 했다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 타 세션 작업물(`.gitattributes`·`cpppj/CMakeLists.txt`·`package-cpppj.ps1`·`main.cpp`·`platform/CMakeLists.txt`·`tests/CMakeLists.txt`·`verify_cpp_package.py` 변경, QuickJS 파일)은 건드리지 않고 내 파일만 커밋한다.
- 남은 것: 재검토 문서 1절의 나머지 42파일 재대조(후속 단위에서 파일 묶음 단위로, 3개 부분 재대조 포함), 0.4 순서 1-4(보류).

#### 0.347 — AiDifficulty 신규 이식과 RawAiCreate 11차·NB10 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 25번(`RawAiCreate.cpp/.h`, R4 AI 관문) + 신규 39파일 중 `AiDifficulty.cpp/.h`(R8 11차). cpppj HEAD의 NB10 정리·11차 배율(157·159)을 C#에 반영했다.
- 구현: 새 파일 `dotnetpj/src/Netstorm.Core/Original/AiDifficulty.cs`(열거 `AiDifficultyLevel` Easy=1/Normal=2/Hard=3·설정 키·배율 상수·`FromValue`/`ToValue`·`ApplyAiDifficulty`·`SanitizeAiForcedEnemy`, 오프셋 +0x220/224/228·+0x234·+0x240). `RawAiCreate.cs`에 `AiCreateHooks.Difficulty` 추가와 생성 시 정리 복사본→난이도 복사본 순서 적용(입력 불변). 새 검사 `dotnetpj/tests/Netstorm.Core.Tests/AiDifficultyTests.cs`(값 매핑·쉬움/어려움/보통·하한·0/NaN·크기 거부·정리 4건·생성 통합 3건). `dotnetpj/PORT_MAP.tsv`에 `AiDifficulty` 2행 추가(port), `RawAiCreate` 2행 `base`를 `d904993ac`으로 올렸다(L1 유지).
- C# 모양 차이: cpppj 열거 `AiDifficulty`는 C# 정적 클래스와 이름이 겹쳐 열거를 `AiDifficultyLevel`로 두었다. cpppj 자유 함수·상수는 정적 클래스 멤버로 옮겼다.
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `AiDifficultyTests`·`RawAiCreateTests` 합계 21건 통과(실패 0, 기존 L1 `CreateMatchesNative32` 포함 — 보통 경로 불변 확인). NB10 제거 변이에서 `CreateSanitizesInvalidForcedEnemy` 1건 실패로 검출력을 확인한 뒤 원복하고 다시 21건 통과했다.
- fixture 변화 없음. 함께 옮긴 10차 수정: NB10. 11차 보통 경로가 원본과 같음은 L1 32행으로 확인했고, 쉬움/어려움 배율 검증(R8 11차)은 세션 AI 설정 소유자(0.306)에 배율 필드를 두는 후속 단위에서 한다.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 다른 세션의 QuickJS 파일은 건드리지 않았다.
- 남은 것: 재검토 문서 1절의 나머지 43파일 재대조(후속 단위에서 파일 묶음 단위로), 0.4 순서 1-4(보류).

#### 0.346 — RawAddRandom M6-n 반환형 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 24번(`RawAddRandom.cpp/.h`, R4 AI·Fort 채우기 연결). cpppj HEAD의 M6-n 반환형 추가(133)를 C#에 반영했다.
- 구현: `dotnetpj/src/Netstorm.Core/Original/RawAddRandom.cs`의 `AddRandomObjects`를 `void` → `uint`로 바꾸고 마지막 배치 결과를 그대로 돌려준다. 기존 호출처(문장 위치 호출)는 그대로 컴파일된다. `dotnetpj/tests/Netstorm.Core.Tests/AddRandomParentTests.cs`에 `AddRandomObjectsReturnsLastPlacementSid` 추가(1234u 관통 확인). `dotnetpj/PORT_MAP.tsv`의 해당 2행 `base`를 `655740c81`로 올렸다(L1 유지).
- C# 모양 차이: 없음(서명·주석만 HEAD와 같게 맞췄다).
- 검증(게임 실행 없음, 이번 목표 지시): 별도 출력 경로 빌드 뒤 `vstest`로 `AddRandomParentTests` 3건 통과(실패 0, 기존 720행 L1 포함). 반환형 변경이라 구 코드에서는 새 검사가 컴파일되지 않아 별도 변이 검사는 하지 않았다.
- fixture 변화 없음. 함께 옮긴 10차 수정 없음(M6-n Fort 채우기 연결).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 다른 세션의 QuickJS 파일(`cpppj/src/platform/ScriptEngine.*`·`cpppj/tests/ScriptEngineTests.cpp`·`cpppj/third_party/quickjs-ng/`)은 건드리지 않았다.
- 남은 것: 재검토 문서 1절의 나머지 44파일 재대조(다음은 `RawAiCreate` 11차·NB10 묶음이 크므로 `AiDifficulty` 신규 이식과 함께 별도 단위로), 0.4 순서 1-4(보류).

#### 0.345 — RawFrameChoose 10차 NB23 재대조와 전체 회귀 1-1 기록 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 27번(`RawFrameChoose.cpp`, R4 지형 본체) + 0.4 순서 1-1(백그라운드 전체 회귀 결과 기록).
- 구현: `dotnetpj/src/Netstorm.Core/Original/RawFrameChoose.cs`의 빈 분위 범위 `throw`를 전체 후보 선택으로 교체하고 메서드 주석에 NB23 반영 이력 추가. 새 검사 `dotnetpj/tests/Netstorm.Core.Tests/RawFrameChooseTests.cs`(빈 분위 4 theme·후보 1개·정상 구간·theme=-1, 7건). `dotnetpj/PORT_MAP.tsv`의 해당 2행 `base`를 `df7902e70`으로 올렸다(L1 유지).
- C# 모양 차이: cpppj의 `random_.Next`를 기존 C# 난수(`NetstormRandom.Next`)로 그대로 쓴다.
- 검증(게임 실행 없음, 이번 목표 지시): 백그라운드 전체 회귀가 잠그고 있던 `Core.Tests` bin을 피해 별도 출력 경로에 빌드한 뒤 `vstest`로 `RawFrameChooseTests` 7건 통과(실패 0). 구 코드 변이(old `throw` 복원)에서는 5건 실패로 검출력을 확인한 뒤 원복하고 다시 7건 통과했다. 전체 회귀 본체(0.343 코드 기준): Assets 366·Client 1,021·Core 1,886(합계 3,273, 실패 0) — 0.3 기준선을 이 값으로 새로 적었다. 0.344·0.345 변경분은 집중 검사로 덮는다.
- fixture 변화 없음. 함께 옮긴 10차 수정: NB23.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음. 다른 세션의 `cpppj/src/platform/ScriptEngine.*`·`cpppj/third_party/quickjs-ng/`는 건드리지 않았다.
- 남은 것: 재검토 문서 1절의 나머지 45파일 재대조(후속 단위에서 파일 묶음 단위로), 0.4 순서 1-4(오라클 갱신, 게임 실행이 필요해 이번 목표에서는 보류).

#### 0.344 — StyleTextMarkup 10차 NB02 재대조 (2026-11-11, HJOW-Athlon)

- 범위: 재검토 문서 1절 19번(`StyleTextMarkup.cpp/.h`, R3 위젯 본체). cpppj HEAD의 NB02 교정(147)을 C#에 반영했다.
- 구현: `dotnetpj/src/Netstorm.Client/Gump/StyleTextMarkup.cs`에 `OffsetMinimum/OffsetMaximum` 상수와 `StyleTextOffsetAllowed(x, y)` 공개 정적 판정 추가, `Option`의 E 분기를 `||`(한 축만 범위 안) → `StyleTextOffsetAllowed`(두 축 모두)로 교체, 메서드 주석의 옛 설명 수정. `dotnetpj/tests/Netstorm.Client.Tests/StyleTextMarkupTests.cs`의 옛 기대값 2건(`E -9.2` 적용 가정)을 NB02 계약(무시)으로 고치고 `EmbossOffsetRequiresBothCoordinatesInRange` 경계 theory 8건(cpppj `KnownBug_NB02`와 같은 값) 추가. `dotnetpj/PORT_MAP.tsv`의 해당 2행 `base`를 `5171b8c7b`로 올렸다.
- C# 모양 차이: cpppj의 파일 범위 상수·자유 함수는 C#의 private 상수·공개 정적 메서드로 옮겼다(호출자가 검사에서 직접 쓸 수 있게 공개).
- 검증(게임 실행 없음, 이번 목표 지시): `StyleTextMarkupTests` 45건 통과(실패 0). 솔루션 전체 빌드는 백그라운드 전체 회귀와 동시에 돌려 테스트 DLL 파일 잠금(MSB3026/3021) 경합으로 실패했으므로, 백그라운드 종료 뒤 다시 확인한다(코드 오류가 아니라 동시 실행 경합이다). 전체 회귀(0.4 1-1)는 백그라운드 실행 중이라 이 단위 결과에 포함하지 않았다.
- fixture 변화 없음(port 행, L1 아님). 함께 옮긴 10차 수정: NB02.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음.
- 남은 것: 재검토 문서 1절의 나머지 46파일 재대조(후속 단위에서 파일 묶음 단위로), 0.4 순서 1-1 결과 기록.

#### 0.343 — cpppj 영향 재검토 정적 분류 (2026-11-11, HJOW-Athlon)

- 범위: 0.4 순서 1-2의 정적 분류. `4d4b3a928..HEAD`의 `git diff`와 PORT_MAP 교집합을 나누고 신규 파일 39개를 차수표에 배정했다. 결과는 [재검토 문서](docs/dotnet-cpp-resync-20261111.md)에 있다.
- 구현: 새 문서 `docs/dotnet-cpp-resync-20261111.md`(교집합 47파일 표 ① 39·② NB 포함 8·③ 없음, 신규 39파일 배정표, 옛 미완 6항목 재확인표). 코드는 건드리지 않았다.
- C# 모양 차이: 없음(정적 분류만).
- 검증(게임 실행 없음, 이번 목표 지시): 빌드·검사를 돌리지 않았다. `cpp_sync.py`는 시간 초과라 쓰지 않고 같은 교집합 방법을 썼다. 전체 회귀(0.4 1-1)는 백그라운드에서 실행 중이다.
- PORT_MAP·fixture 변화 없음(`base`·floor 미갱신 — 재대조를 마친 행만 올린다).
- 함께 옮긴 10차 NB 수정 없음(② 8파일의 NB08·NB02·NB24·NB04·NB18·NB29·NB23은 해당 파일을 옮길 때 함께 옮긴다).
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요·cpppj·LEFT_JOBS.md 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음.
- 남은 것: 0.4 순서 1-1(전체 회귀 결과 기록), 1-2의 실제 재대조(후속 단위), 1-4(오라클 갱신, 게임 실행이 필요해 이번 목표에서는 보류).

#### 0.342 — 11.01 카피라이트 Copyright 2026 HJOW 반영 (2026-11-11, HJOW-Athlon)

- 범위: 0.4 순서 1-3. cpppj 제품 1291 `client/ProductPresentation.cpp`의 `Copyright 2026 HJOW` 줄을 dotnetpj 새 경로·기존 경로에 반영했다.
- 구현: `dotnetpj/src/Netstorm.Client/ProductPresentation.cs`의 `PatchCreditsBody`에 `Copyright 2026 HJOW<br>` 줄 추가(Development 줄 다음, cpppj 순서와 같음). `dotnetpj/src/Netstorm.Game/MainMenuView.cs`의 11.01 상세(`OpenCreditsDetail` item 2)에 같은 줄 추가와 메서드 주석에 0.342 이력. `dotnetpj/tests/Netstorm.Client.Tests/ProductPresentationTests.cs`의 본문 검사에 `Copyright 2026 HJOW` 포함 assert 추가.
- C# 모양 차이: cpppj의 `NetStorm` 표기를 기존 C# 본문의 `Netstorm` 표기는 그대로 두고 줄만 더했다(표기 변경 없음). 한국어·일본어 번역 본문은 새 경로에 아직 없어 추가하지 않았다(결정 D4: 언어 검증은 R8).
- 검증(게임 실행 없음, 이번 목표 지시): `dotnet build dotnetpj/Netstorm.sln -c Release` 경고 0·오류 0. `ProductPresentationTests` 6건 통과(실패 0). 전체 회귀(0.4 1-1)는 다음 단위에서 한다.
- PORT_MAP·fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음.
- 보호·실행·외부 접속: `originals/`·`originalCD/`·AGENTS.md·README 개요 미수정. 게임 창·오라클 실행 없음. 외부 접속 목록 변경 없음.
- 남은 것: 0.4 순서 1-1(전체 회귀 완주), 1-2(cpppj 영향 재검토 1247→1300), 1-4(오라클 갱신, 게임 실행이 필요해 이번 목표에서는 보류).

## 9. AGENTS 반영 기준과 이관 기록

<a id="agents-reflection"></a>

### 9.1 AGENTS.md 반영 기준 (재확인 2026-11-11)

AGENTS.md는 수정하지 않는다. dotnetpj에 닿는 사용자 변경은 모두 현재 계획에 반영했다.

| 사용자 변경 | dotnetpj 반영 |
| --- | --- |
| `ef0c6b91` 실행 파일 이름 `Netstorm.exe` | 기존 경로 0.96 적용. 결정 D2: R5에서 새 호스트로 넘기고 기존 경로는 `NetstormLegacy`로 내린 뒤 삭제 |
| `462c9a64` 11.01 표기·Credits 3항목 | 0.150 완료(새 경로·기존 경로) |
| `0190ba95` 외부 접속 문서화·유효성 검사 비활성화 | 0.151 완료, 이식 때마다 갱신(3.2) |
| `5867f35e`·`990a3541` 4차 로컬 MCP(전체 조작·캡처·소리 포함 녹화)·와이드 최대 1920×1080 | [1.3](#dotnet-r8-requirements) 4차, 결정 D3(dotnetpj에도 MCP) |
| `98b2f20c`·`aaafbbc4` 시나리오 편집 추가·5차 확장(자동 생성 파일 전부 사용자 홈, 읽기 전용 설치) | 1.3 5차·9차 |
| `851054a8`·`3051892a` 기존 파일 재활용·README 개요 보호 | 3.2 |
| `b5197d48` 알려진 버그 34건(현 10차) | 1.3 10차. 옛 0.8 7번은 아래 범위 확대로 해소(0.5 아래) |
| `551d27ae` 세 프로젝트 평가 기준 | 0.1(실행·플레이 가능 여부와 원본 동일성) |
| `75e4eed9` 글꼴 exe 내장 금지·exe 옆 `fonts/`·일본어 등 다른 글꼴 | 3.2·5절 2·7번 |
| `8136df49e`·`b5117f524`·`8661e1060` 7차 오류/예외 상세 로그 신설(로그 경로·`yyyyMMddHHmmss.log`·원인 추적 정보)과 차수 재번호, Linux `$HOME/.netstorm` | 1.3 7차(옛 번호 대응은 1.3 머리), Linux 경로는 결정 D5 |
| `22a0877c3` webpj 예외(8·9차 등) | webpj 전용이라 dotnetpj 변화 없음 |
| `60226ea93` 11차 난이도·12차 스커미시·13차 기타 언어, dotnetpj 범위 **4~13차**, `Copyright 2026 HJOW` | 1.3 11~13차, 0.4 1-3 카피라이트 |
| `610670750` cpppj 플랫폼 명시 | cpppj 전용 |
| `85551c6c2` 14차 QuickJS 추가, dotnetpj 범위 **4~14차**, webpj 예외 변경(4차 WebMCP·5차 localStorage·14차 불필요·12차 대상) | 1.3 14차(cpppj 구현 뒤 같은 계약), 0.2 R8·0.4 7단계 범위. webpj 예외 변경은 dotnetpj 변화 없음 |

<a id="dotnet-archive-log"></a>

### 9.2 이관 기록 — LEFT_JOBS.dotnetpj.old.md

**2026-11-11 이관(HJOW-Athlon, 문서만, 사용자 요청):** cpppj 마감분(제품 1300)과 AGENTS.md 최근 변경을 반영해 이 문서를 다시 정리했다. 정리 직전(`3ef66e80d / 20261111-151 DN`) 전체 원문을 [보관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341)에 그대로 보관했다(원문 SHA-256 `698670bf211d9ec89d63ae2ffca976e9c28e87dd52c74288fb1b2ef58d785209`, 보관본을 LF로 되돌려 원문과 바이트 일치 확인). 보관 파일 머리의 보관본 목록에 새 링크를 더했다.

- **본문에서 뺀 것:** 단위 기록 0.280~0.341 전체, 0.1·숫자 표의 단위별 집중 검사 나열, 0.9 R4의 단위별 `[x]` 하위 항목(묶음으로 줄여 [2.4](#r4-detail)에 남김), 옛 6절의 과거 이관 상세(아래 목록으로 줄임).
- **새로 넣은 것:** [0.4](#now) 남은 작업 순서표(재개 준비 1-1~1-4 포함), [0.5](#decisions) 결정 표(D1 HEAD 이식 방식·D2 `Netstorm.exe`·D3 MCP·D4 언어 앞당김·D5 Linux 경로), [1절](#resync-scope) cpppj 마감분 반영(영향 재검토 범위·4~13차 대응표·cpppj 요청), 0.1의 이식 범위 집계, [9.1](#agents-reflection) AGENTS 반영 표.
- **바뀐 해석:** R8 범위 4~8차 → 4~13차(번호 재배치 포함), 옛 0.8 7번 해소, 옛 "cpppj 미완 범위" 목록은 1-2에서 재확인 대상으로, 9-1 D에 cpppj F2 근거 추가.
- 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`·cpppj·AGENTS.md는 수정하지 않았다.

**2026-11-11 결정 반영(같은 날, HJOW-Athlon, 문서만):** 사용자가 D1~D5를 정했다(D1·D3~D5는 권고안, D2는 "새 경로가 이름을 이어받고 작업이 끝나면 기존 경로 제거"). 0.5를 확정 표로 바꾸고 0.4·1.3·2.1~2.3·3.2·4·5·9.1의 관련 문장을 고쳤다. 전환 시점(R5에서 메뉴·미션·이름·배포를 한 번에)과 기존 경로 삭제 시점(결정 5의 R6 기준)은 이 결정에 대한 AI 해석이다. 이정표 D1·D2와 결정 D1~D5가 헷갈리지 않게 이정표에는 "이정표"를 붙였다.

**2026-11-11 AGENTS 14차 반영(같은 날, HJOW-Athlon, 문서만, 사용자 요청):** 사용자 커밋 `85551c6c2`(14차 QuickJS 추가, dotnetpj 범위 4~13차 → 4~14차)를 머리말·0.2 R8·0.4 7단계·1.1·1.3(14차 행)·3.2 목표 순서·7.2 참고 문서·9.1에 반영했다. 당시 기록인 0.5 아래 해소 항목과 9.1·9.2의 지난 행은 그대로 두었다. 빌드·검사·게임 실행은 하지 않았다.

**이전 이관(요약, 상세는 각 보관본 머리):** [2026-11-10 — 0.279까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279)(HJOW-Athlon, AGENTS 8차 시나리오 편집·5차 확장·9차 알려진 버그·글꼴 외부 배포·평가 기준 반영), [2026-11-04 — 0.154까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)(HJOW-Athlon), [2026-11-02 — 0.126까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)(HJOW-Athlon), [2026-10-31 — 0.73까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)(HJOW-X3D), 2026-10-28 재수립 이관(VM-W11-Codex: 2026-10-22 이전 계획, 날짜별 완료·인계 절, 옛 0~11·14절).

**앵커:** 2026-10-28에 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 보관 파일에 같은 이름으로 남아 있다. `LEFT_JOBS.old.md`·`LEFT_JOBS.webpj.md`·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 둔다. 그런 링크로 들어왔다면 보관 파일의 같은 앵커를 본다. 옛 절 제목 앵커(`04-…`·`05-…`·`07-…`·`08-…`·`09-…`)는 이 문서의 대응 절 앞에 남겼다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
