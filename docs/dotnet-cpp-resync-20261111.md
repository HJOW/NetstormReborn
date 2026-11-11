# dotnetpj cpppj 영향 재검토 — 제품 1247 → HEAD (2026-11-11)

- 범위: `4d4b3a928`(0.281 검토 기준, 제품 1247) `..HEAD`(e880110d5, 0.342 커밋 포함).
- 방법: `git diff --name-only/--stat`와 `dotnetpj/PORT_MAP.tsv`의 `cpp` 열 교집합, 파일별 `git log -- <파일>`의 CP 커밋 대조. `cpp_sync.py --ports`는 0.147 이후 시간 초과라 쓰지 않았다(LEFT_JOBS.dotnetpj.md 1.2의 방법과 같음).
- 이번 목표 지시에 따라 게임 실행 비교(L2/L3/L4a)는 하지 않았다. 이 문서는 정적 분류까지다. 실제 재대조(같은 입력 결과 비교)와 `base` 올리기는 각 파일을 건드리는 후속 단위에서 한다.
- 집계: 이 구간에 `cpppj/src`를 건드린 커밋 40건, 변경 파일 188개(이 중 PORT_MAP `cpp` 열에 있는 파일 47개), 신규 파일 39개. 문서 0.4의 "47커밋·184파일"은 제품 1300 시점 수치이고, 그 뒤 `cpppj/src` 변경은 없다(171~177은 DN/WB/CP 문서·dotnetpj 커밋이라 `cpppj/src`에 영향 없음).

## 1. PORT_MAP 교집합 47파일 분류

구분: ① 이미 옮긴 C#의 재대조 대상(파일을 건드릴 때 HEAD와 재대조) ② 10차 NB 수정 포함(옮길 때 함께 옮기고 단위 기록에 NB 번호) ③ 주석·형식만 변경. 이번 구간에 ③에 해당하는 파일은 없다(공백 2줄이 바뀐 `EditorMapCatalogFiles.cpp`도 `Deleted()` 추가가 있어 ①이다).

| # | cpppj 파일 | C# 대응 (PORT_MAP base·상태) | 구분 | 변경 요약과 관련 CP |
| --- | --- | --- | --- | --- |
| 1 | `src/app/main.cpp` | `Netstorm.Client.Host/Program.cs`(91d494c3, port), `Automation/UiScript.cs`(f0a08f08, port), `Host/HostFrameDriver.cs`(91d494c3, port), `Host/HostOptions.cs`(91d494c3, port) | ① (+R8 7차) | 7차 ErrorLog 설치·최상위 실패 기록·`--log-crash-test`(121), `text` ui-script 연산(K1 후속, 122). R8 7차 이식 때 함께 옮긴다 |
| 2 | `src/client/BitmapFont.cpp` | `Netstorm.Assets/BitmapFont.cs`(f95a8db2, L1), `Netstorm.Client/Display/BitmapFont.cs`(e074cad7, port), `Host/WindowsBitmapFont.cs`(e074cad7, port) | ① (+R8 13차) | 13차 일본어 대표 글리프를 히라가나 'あ' 전각 폭으로(149). R2 잔여·R8 13차에서 재대조 |
| 3 | `src/client/ClientMain.cpp` | `Netstorm.Client/Display/ScreenCapture.cs`(f0a08f08, port) | ① (+R8 F·13차·7차·6차) | 대형(2966행, 줄바꿈 복원 위주). F1 Frame Rate 하위 메뉴·즉시 반영(164), F2 양보 대기(166), L 일본어(149), K1(122), ErrorLog(121), M7.1 helpWord 캐시(120), 로그 버퍼(114), JoinTestBattle(108), 줄바꿈 경계(96), 지연 FIFO Poll(95), 무기 도표(88). R1 호스트 루프를 옮길 때 HEAD 기준으로 통째로 재대조한다 |
| 4 | `src/client/EditorMapCatalogFiles.cpp/.h` | `Host/EditorCatalogLedger.cs`(3ddeb19, port), `Host/EditorCatalogWorkspace.cs`(fc56410a, port), `Host/EditorWorkspacePaths.cs`(75c5dac, port) | ① (+R8 9차) | S1 `Deleted()` 추가(완료 이관 후 삭제 표식 구별, 132). 9차 편집 이식 때 재대조 |
| 5 | `src/client/HtmlTextParser.cpp/.h` | `Gump/HtmlTextParser.cs`(fb0f7378, L1) | ① (+6차) | K1 확장 표시 4BYTE 원자 문자 보존(122). L1 fixture는 기본 false 관문 유지. R3 잔여에서 재대조 → **0.353 완료**(`ExtendedText` 전역·토큰 보존 분기, 10건 통과) |
| 6 | `src/client/HtmlWindowFactory.cpp/.h` | `Gump/HtmlWindowFactory.cs`(9ad3ff97, L1) | ① (+6차) | K1 `localizedContent` 훅(확장 문자 내용 인정, 122). R3 잔여에서 재대조 → **0.353 완료**(훅·`Show` 조건·호스트 배선, 10건 통과) |
| 7 | `src/client/HtmlWindowMetadata.cpp/.h` | `Gump/HtmlWindowMetadata.cs`(9ad3ff97, L1) | ①② NB08 | NB08 `%` 인라인 목록 잘림 교정(`repairInlineList` 꺼짐 시 원문 동작 보존, 145). 옮길 때 함께 옮기고 "10차 NB08 포함"으로 기록한다 → **0.348 완료**(훅·분기·제품 배선, 10건 통과) |
| 8 | `src/client/InputEvent.cpp/.h` | `Host/HostInputMapper.cs`(91d494c3, port), `Input/InputEvent.cs`(ac9ea916, port) | ① (+6차) | K1 `TryPushBatch`·`FreeSlots`(확정 본문 원자 입력, 122). R1 입력 큐·R6 전체 입력에서 재대조 → **0.349 완료**(`Input/InputEvent.cs` 2행만 `base` 갱신, `HostInputMapper`행 유지) |
| 9 | `src/client/Mission.cpp/.h` | `Mission/GuideMissionScriptLifetime.cs`(f3735ed3, port), `Mission/MissionScript.cs`(8db90513, port) | ① (+6차) | K1 표시 언어 스크립트 분리(실행 대본 교체 없음, 122). R3·R6 미션 스크립트에서 재대조 → **0.349 완료**(`MissionScript.cs` 2행만 `base` 갱신, `GuideMissionScriptLifetime`행 유지) |
| 10 | `src/client/ProductPresentation.cpp/.h` | `ProductPresentation.cs`(108f76cc, port) | ① (+11/12/13차, 0.342 반영) | 11.01 카피라이트(155, 0.342에서 dotnetpj 반영 완료), 난이도 구간(157), 스커미시 Campaign 항목(158), L 일본어(149), K4/K5(136). `.cpp`의 한국어·일본어 고정 번역 본문과 Skirmish·SetDifficulty 구간은 R3 메뉴·R8 11/12/13차에서 옮긴다 |
| 11 | `src/client/QuickHelpWindow.cpp/.h` | `Gump/QuickHelpWindow.cs`(c6b19af, L1) | ① (+6차) | K1 `InvalidateCache`(언어 전환 뒤 재조회, 122). R3 잔여에서 재대조 → **0.349 완료** |
| 12 | `src/client/RawCampaignSession.cpp/.h` | `Mission/RawCampaignSession.cs`(cpp 800ff40c/h 4ee21387, port) | ①② NB04·NB18·NB29 (+11/12차) | 대형(687행). NB18 운반체 사제 줍기 거부(156), NB04 Even 구역 선발 교정·NB29 간헐천 예산(162), 스커미시 월드 MP 시작 규칙·NB23 종료 교정(169), 스커미시 설정 화면(158), 난이도 AI 설정(157), M6-n Fort 채우기(133), Native 고정 수신기(97/98/101/103/105/108). F3 세션의 관문·소유 상태 이식 때 HEAD 기준으로 재대조하고 NB 번호를 기록한다 |
| 13 | `src/client/RawMenuControls.cpp` | `Gump/HtmlBodyTextParser.cs`(ecb9802a, port; RenderEmbeddedTell 부분 게시기 행) | ① (+9차·6차·M7) | S1 Scenario 입력 Enter 처리·`ShowScenarioInputDialog`(132), K1 DisplayTextInput 취급(122), M7.1 helpWord NULL 진단(120/128), `InvalidateHelpLanguage`(122). RawMenuControls 전체 소유자 이식은 R3 잔여 범위이므로 HEAD 기준으로 옮긴다 |
| 14 | `src/client/RawMenuHtml.inc` | `Gump/GumpModal.cs`(9ad3ff97, port), `Gump/HtmlWindowTextParser.cs`(9ad3ff97, port) | ①② NB08 (+6차) | NB08 제품에서 `repairInlineList=true`(145), K4 한국어 잔재 교정(136), K1(122). R3 잔여에서 재대조 → **0.348 부분**(NB08 배선만, K4·`localizedContent` 잔여, `base` 유지) |
| 15 | `src/client/RawMenuSaveFort.inc` | `Host/HostFortNameConfirmation.cs`(75c5dac, port), `Host/HostSaveFortDialog.cs`(e814a8b8, port) | ① (+9차) | S1 Scenario 패널 연결(132). R3·R7 Edit에서 재대조 |
| 16 | `src/client/RawSurfaceStorage.cpp` | `Core/Original/RawSurfaceStorage.cs`(435cfa1d, port) | ①② (+월드 보호) | 월드 판정 자산 처리 보호(119), Dude 주문 초과 최초 처리 본문(112), 무기 도표(88). R4 월드 본체 조립 때 재대조 |
| 17 | `src/client/RawSurfaceWorld.cpp/.h` | `Core/Original/RawSurfaceWorld.cs` 등 4행(b8cadde1, port) | ① | 무기 도표(88), 월드 보호(119), Dude 본문(112). R4에서 재대조 |
| 18 | `src/client/SaveFortNameDialog.cpp`(+`.h` 시나리오 4필드) | `Gump/SaveFortName.cs`(4e56903c, port), `Gump/SaveFortNameDialog.cs`(6f7070e7, port) | ① (+9차) | S1 `ShowScenarioInput`(132). R3 잔여에서 재대조 → **0.354 부분**(대화창 본체·4건 통과, Context 4필드·`ShowScenarioInputDialog`는 R7, `base`는 `.cs`행만 갱신) |
| 19 | `src/client/StyleTextMarkup.cpp/.h` | `Gump/StyleTextMarkup.cs`(f7e1748e, port) | ①② NB02 | NB02 Emboss 오프셋 범위(-4..4) 고정 `StyleTextOffsetAllowed`(147). 옮길 때 함께 옮긴다 |
| 20 | `src/client/TellFileMenus.cpp/.h` | `Gump/TellFileMenus.cs`(f9291649, L1) | ①② NB24 (+6차·S1) | `Title` 80BYTE 절단(NB24)과 `InvalidateLanguageFiles` 선언·구현. NB08의 드라이브 루트 해석은 `o/BaseFile`·`o/FileSpecQuery`(47 밖) 쪽이다. R3 목록 소유자에서 재대조 → **0.348 부분**(`Title` 절단 완료, `InvalidateLanguageFiles` 잔여, `base` 유지) |
| 21 | `src/client/TellMapMenus.cpp` | `Gump/TellMapMenus.cs`(4869c6ba, L1) | ①② NB24 (+6차) | NB24 긴 제목 80BYTE 종료·폴백(145), K4(136). R3에서 재대조 |
| 22 | `src/client/TextEntryBuffer.cpp/.h` | `Gump/TextEntryBuffer.cs`(a4b04553, L1) | ① (+6차) | K1(122, 한글 조합 경로의 일부). R3 `RawTextEntryControls` 잔여에서 재대조 → **0.352 완료**(확장 붙여넣기 분기, 13건 통과) |
| 23 | `src/client/TextEntryGump.cpp` | `Gump/TextEntryGump.cs`(39ec3a5a, L1) | ① (+6차) | K1(122). R3 잔여에서 재대조 → **0.352 완료**(진입 정규화·방향/삭제/확정/길이·자동완성·그리기 경계, 13건 통과) |
| 24 | `src/o/RawAddRandom.cpp/.h` | `Core/Original/RawAddRandom.cs`(108f76cc, L1) | ① (+M6-n) | 마지막 배치 SID 반환(M6-n Fort 채우기 0046e630용, 133). R4 AI 나머지·R6 배치 경로에서 재대조 |
| 25 | `src/o/RawAiCreate.cpp/.h` | `Core/Original/RawAiCreate.cs`(53a0e4f9, L1) | ①② NB10 (+11차) | NB10 강제 AI 범위는 `RawCampaignConstructionBindings` 쪽(159, 이번 47 밖). 여기 변경은 11차 난이도 AI 설정 확인(157). R4 AI 나머지에서 재대조 → **0.347 완료**(`AiDifficulty` 신규 이식·`Difficulty` 훅·정리→배율 순서, 21건 통과) |
| 26 | `src/o/RawConnectionActivity.cpp` | `Netstorm.Net/RawConnectionActivity.cs`(ee4e45c6, port) | ① (3차 사전 구현) | Native F가 있으면 같은 metadata 대여(105). 최후순위 3차에서 재대조 |
| 27 | `src/o/RawFrameChoose.cpp` | `Core/Original/RawFrameChoose.cs`(5c2d0a1e, L1) | ①② NB23 | NB23 후보 8개 미만 타일의 quartile 빈 범위 교정(theme 없이 전체 후보, 169). 옮길 때 함께 옮긴다 |
| 28 | `src/o/RawGameAuxState.h` | `Core/Original/RawGameAuxState.cs`(da27e191, port) | ① (+12차) | JoinTestBattle 1bh 생성자 헤더(108). R4 세션 조립에서 재대조 |
| 29 | `src/o/SquidPop.cpp` | `Core/Original/OriginalPopDisplay.cs`·`RawPop.cs`(da27e191, L1) | ① | 메타/일반 Sprite VT 두 개 공통 후처리 허용(119). R4 판본 분기 잔여에서 재대조 |
| 30 | `src/o/SquidPostPop.cpp` | `Core/Original/RawPostPop.cs`(f95a8db2, L1) | ① | 최초 Dude·세 효과 예약의 장기 전용 본문 선택(119), Dude 본문(112). R4에서 재대조 |
| 31 | `src/client/LocalTcpTransport.cpp` / `LocalUdpTransport.cpp` | `Netstorm.Net/LocalTcpTransport.cs`·`LocalUdpTransport.cs`(a76499e3, port) | ① (3차 사전 구현) | 지연 FIFO(`RawPacketDelay`, 95). 최후순위 3차에서 재대조 |

NB 수정이 든 PORT_MAP 교집합 파일 정리(②, 단위 기록에 번호를 적을 것): `HtmlWindowMetadata`(NB08), `RawMenuHtml.inc`(NB08), `TellMapMenus`(NB24), `StyleTextMarkup`(NB02), `RawCampaignSession`(NB04·NB18·NB29, NB23 종료 교정 포함), `RawFrameChoose`(NB23), `RawAiCreate`(NB10은 바인딩 쪽·NB 확인용). `TellFileMenus`는 NB24(`Title` 절단)만 해당한다. NB09(길드 태그)·NB20(사제 선택·커서 조각)·NB10(강제 AI)은 이번 47 밖의 파일(`NativeMetaParticipantName`·`PriestSelectionShortcut`·`RawCampaignConstructionBindings` 등)에 있어 해당 파일을 옮길 때 함께 옮긴다. NB08의 드라이브 루트 해석은 `o/BaseFile`·`o/FileSpecQuery`(47 밖) 쪽이다.

## 2. 신규 39파일의 차수 배정 (1.3 표 기준)

| 신규 cpppj 파일 | AGENTS 차수·단계 | 비고 |
| --- | --- | --- |
| `client/ClientHelpWord.cpp/.h` | M7.1 도움말 캐시 → R3 잔여 | `ClientMain`·`RawMenuControls`의 helpWord 연결과 함께 옮긴다 |
| `client/DisplayTextInput.cpp/.h` | 6차 한국어 화면·입력 → R3 잔여(표시 부호화 경계) + R8 6차 | 결정 D1에 따라 경계는 함께 옮기고 검증은 영어 → R8 → **0.351 완료**(+`text` 연산자) |
| `client/EditorScenarioController.cpp/.h`, `EditorScenarioFiles.cpp/.h`, `EditorScenarioModel.cpp/.h`, `client/ScenarioDocument.cpp/.h`, `client/UberScenario.inc` | 9차 시나리오 편집 → R7 Edit | R7에서 묶음으로 옮긴다 |
| `client/FrameRateOption.cpp/.h` | 후순위 60/120프레임(F1) + 0.4 8단계 | 옵션 `Frame Rate` 하위 메뉴·즉시 반영. 요구사항 전용 모듈이라 R8 전이 아니라 8단계에서 이식한다 |
| `client/NativeCarrierPacketReceivers.cpp/.h`, `NativeConstructionPacketReceivers.cpp/.h`, `NativeFortPreparation.cpp/.h`, `NativeFortSnapshot.cpp/.h`, `NativeGameplayPacketReceivers.cpp/.h`, `NativeJoinTestBattle.cpp/.h`, `NativeLoginPacketReceivers.cpp/.h` | 3차 M6 LAN·Native → 최후순위 10단계 | 현재 `Netstorm.Net` 사전 구현과 같은 범위. 검사 없는 사전 구현을 늘리지 않고 10단계에서 옮긴다 |
| `client/RawSurfaceVortex.cpp` | R6 규칙(소용돌이 표시·규칙) | 해당 몸체 클래스를 옮길 때 L1 `arc-spire` 계열처럼 fixture와 함께 연결한다 |
| `client/SkirmishSetup.cpp/.h`, `client/SkirmishStartIslands.inc` | 12차 스커미시 → R8 12차 | 이미 옮긴 섬 생성(0.156)·간헐천 생성과 MP 시작 순서로 묶는 경로다 |
| `o/AiDifficulty.cpp/.h` | 11차 난이도 → R4 AI 나머지(+R8 11차) | AI 설정 +0x220/224/228 배율 필드. 0.306의 AI 설정 소유자에 HEAD대로 두고 보통이 원본과 같은지 먼저 검증한다 → **0.347 완료**(보통 불변은 L1 32행으로 확인, 배율 필드 후속) |
| `o/RawPacketDelay.cpp/.h` | 3차 지연 FIFO → 최후순위 10단계 | `LocalTcp/UdpTransport` 변경(위 31)과 함께 다룬다 |
| `platform/ErrorLog.cpp/.h` | 7차 오류 로그 → R8 7차 | `main.cpp`의 설치·기록 경로와 함께 옮긴다. 로그 위치·이름 규칙(`...\Netstorm\logs\yyyyMMddHHmmss.log`)은 결정 D1에 따라 처음부터 사용자 루트로 둔다 |

## 3. 옛 "cpppj 미완 범위" 재확인 (3.2, 0.4 1-2의 일부)

옛 목록 6항목 중 cpppj HEAD에서 구현된 것은 이식 대상으로 바꾼다(3.2):

| 옛 항목 | cpppj HEAD 상태 | dotnetpj 처리 |
| --- | --- | --- |
| View/zoom | 미완(LEFT_JOBS.old.md 9594·10004: "View/zoom·도킹 조합·모드별 표시·픽셀 대조는 남는다") | 경계 유지 |
| 도킹 | 부분 완료(HUD 루트·도킹/덱 연결은 완료, 조합·모드별 표시는 남는다) | R5 범위에서 HUD를 옮길 때 HEAD 계약을 따른다 |
| DD 조정·확인 UI 잔여 | 미완(해당 UI 잔여 언급이 HEAD에도 남는다) | 경계 유지 |
| Chat GUI 드래그 | 완료(543·640: 구분선 드래그/숨김/복원 확인) | R6 입력·R3 위젯 잔여에서 이식 대상으로 |
| 미니맵 내용물·grid·항목별 debug | 미완(미니맵 독립 토글·기본 표시는 완료(540), 내용물·grid·항목별 debug는 남는다) | 기본 토글은 R5에서, 내용물·grid·debug는 경계 유지 |
| Renderer 시차 경계 | 미완(LEFT_JOBS.old.md 131·154·169: 시차 자연 Frame 연결이 남는다) | 경계 유지 |
| (참고) 독립 미니맵·Chat 입력·전달 | 완료 | 이미 이식 대상(3.2 유지) |

## 4. `base`·floor 처리

- 이번 단위는 정적 분류만 했으므로 PORT_MAP `base`와 `floor-commit`(`f95a8db2`)을 올리지 않았다. `cpp_sync.py`는 계속 0종료가 아니다(예상된 상태).
- 재대조를 마친 행만 `base`를 올린다(0.4 1-2의 규칙). `ProductPresentation.cpp`는 0.342에서 카피라이트 줄만 반영했으므로 `base`를 올리지 않았다(난이도·스커미시·일본어 구간이 남아 있다).
- 0.344에서 `StyleTextMarkup.cpp/.h` 2행의 `base`를 `5171b8c7b`로 올렸다(NB02 재대조 완료, `StyleTextMarkupTests` 45건 통과). 0.345에서 `RawFrameChoose.cpp/.h` 2행의 `base`를 `df7902e70`으로 올렸다(NB23 재대조 완료, `RawFrameChooseTests` 7건 통과·L1 유지). 0.346에서 `RawAddRandom.cpp/.h` 2행의 `base`를 `655740c81`로 올렸다(M6-n 반환형 재대조 완료, `AddRandomParentTests` 3건 통과·L1 유지). 0.347에서 `RawAiCreate.cpp/.h` 2행의 `base`를 `d904993ac`으로 올리고 `AiDifficulty.cpp/.h` 2행을 새로 등록했다(11차·NB10 재대조 완료, 21건 통과·L1/port). 0.349에서 `InputEvent`·`Mission`·`QuickHelpWindow` 6행의 `base`를 `af7e11ff1`로 올렸다(K1 경계 재대조 완료, 14건 통과). 0.350에서 `OriginalText.h/.cpp`(확장 부분)→`ExtendedText.cs` 2행을 새로 등록했다(12건 통과·port). 0.351에서 `DisplayTextInput.cpp/.h`→`Input/DisplayTextInput.cs` 2행을 새로 등록했다(7건 통과·port). 0.352에서 `TextEntryBuffer.cpp/.h`·`TextEntryGump.cpp/.h` 4행의 `base`를 `84db5219f`로 올렸다(13건 통과·L1 유지). 0.353에서 `HtmlTextParser.cpp/.h`·`HtmlWindowFactory.cpp/.h` 4행의 `base`를 `e99c641aa`로 올렸다(10건 통과·L1 유지). 0.354에서 `SaveFortNameDialog.cpp`→`SaveFortNameDialog.cs` 1행의 `base`를 `9d35b9a34`로 올렸다(4건 통과·port 유지).
