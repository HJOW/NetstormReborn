# webpj 설정·송금·AI 진단·시나리오 객체·저장 카메라 입력 분배 이식 — WB04-E27

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물일곱 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/ConfigurationCommandInput`·`client/SendMoneyInput`·`client/AiDebugCommandInput`·`client/ScenarioObjectInput`·`client/SavedCameraKey`(10.78)를 옮겼다. [E25](webpj-view-utility-inputs-20261108.md)와 같은 모양이다: Guide/Gump를 지난 일반 입력 하나를 받아 소비 여부를 돌려주고, 외부 효과는 훅으로 주입하며, 도달하지 않은 장치는 요구하지 않고, 앞선 부분 효과는 되돌리지 않는다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/configuration-command-input.js` | 54279c/54291c/542a10, 00439983 | ChangePal(키를 먼저 고름: 전투 `battlePal`·도전 `chalPal`·그 밖 `fortPal` → `set(키, 요청)` → `get(키)`로 현재 값 재조회 → `palette`), ChangePriority(공유 DWORD에 y를 먼저 쓰고 → `thread(1→1, 2→15, 그 밖→0)` → `highPriorityValue = "<현재 signed>"` 설정문), SetFortType(편집기가 아니면 진단 2012 → `state(owner)` 28바이트의 +0x11에 y 하위 BYTE → 현재 owner를 다시 읽어 `notify`). `planNativeThreadPriority`는 WinMain 국소 부모의 요청 계산(범위 밖 wanted를 clamp하지 않음) |
| `www/core/send-money-input.js` | 4c9350 등 | 여섯 명령. 고정 300/700/1000·All은 이체(AI 플래그 `0x20`: [고정 금액만] `inSingle` 진단 → `addLocal(−금액)` → `wallet`, 연결 Player `0x1`: `addOwner` → 11바이트 패킷 `{1,0x6c,11,0,대상,로컬,0,금액 LE}` `send`), X 입력·All 확인은 대상만 창에 전달. All은 확인 순간의 SP를 두 번 읽고 float→64비트 절삭 하위 32비트 정수. `confirmSendMoney`는 첫 LF 앞을 CRT atol 형으로 읽고 항상 1로 닫음 |
| `www/core/ai-debug-command-input.js` | 413d50 | ToggleReporting(메타맵이 아니고 y>0일 때 Player owner(9 이상 `RangeError`)의 brain 토큰 → 본문 +0xb00 DWORD 0↔1), ToggleDebug(y 전체를 알려진 진단 주소 식별자로 보아 29↔99). 공통 뗌 관문은 `dispatchAiDebugCommandInput`에 보존, `applyAiDebugCommandState`는 뗌을 가리지 않는 raw child |
| `www/core/scenario-object-input.js` | 4d6eca..4d6f0d | 다섯 객체 명령. 대상은 y의 낮은 WORD(0이면 소비만), 아니면 `objectInput(SID, 사건 코드 전체)`를 호출하고 반환값은 쓰지 않음 |
| `www/core/saved-camera-key.js` | 005caeb0, 00439928, 497e50, 004d5020 | 저장 10칸(`SavedCameraPositions`: 80바이트 `Uint32Array` 이미지라 NaN 페이로드까지 보존), 초기화(−10000), 현재 중심 계산(정수 감김·절반 0 쪽 절삭 후 float 나눗셈), 유효성(0 이상 256 미만), 숫자 키 분배(정지 → 공개 언어별 기호 표 → Shift 기호는 선택음 뒤 중심 저장, 숫자는 유효할 때만 `center`; Ctrl/Alt 비트는 제거하지 않음) |

## 2. 검증 (`tests/command-inputs.test.mjs`, 14건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음):

- `configuration-commands-x86.tsv` 238행(Console/정지 독립·Guide/Gump 선행 소비 제외·외부 효과 뒤 현재 전역 재조회·부분 효과 순서·State 28바이트 출력), `configuration-command-names-x86.tsv` 30행(이름→번호만).
- `send-money-input-x86.tsv` 888행 전부와 그중 AI(flags `0x20`/`0x22`) 312행의 위치별 진단 줄(`inSingle`, `Userinput.cpp` 0x913/0x8e9/0x8c2).
- `ai-debug-input-x86.tsv` 200행, `scenario-object-input-x86.tsv` 480행, `saved-camera-key-x86.tsv` 1750행(언어·수정키·유효성·현재 Renderer 중심·소리/캡처/중심 중단 시 전체 저장 장부).

cpppj 계약 검사 이식: 우선순위 공유 DWORD와 OS/설정 중단의 부분 효과, 스레드 우선순위 계획, SetFortType의 owner 재조회와 State 28바이트 거부, All이 현재 금액을 읽음과 확인 버튼의 항상 1, 여섯 이름표 번호 중복 없음, AI 진단 이름표·Player 범위·토큰 0·메타맵, 객체 이름표와 SID 0·장치 요구, 저장 카메라의 장치 지연 요구·부분 쓰기·유효성·중심 계산.

최초 실패와 교정: 14건이 처음부터 통과했다.

## 3. 경계

- 대본 큐(`QueueScriptCommand`/`ScriptStringRing`): 이름 fixture의 인자·문자열 고리·진단 열은 대본 큐 단위에서 대조한다(여기서는 이름→번호만 확인). `ai-debug-names`·`ai-debug-toggle-names`도 같다.
- 실제 장치(Client 설정 manager·`RawPlayerTemplateState`의 State 생성/확장/파일 저장·`SoloMapCatalog` 분류 reader·`ClientGameplayGlobals`의 카메라 소유자·`GameplayGlobals`의 안정 진단 저장소·`CameraHome`)는 훅으로만 주입했다. 해당 cpppj 계약 검사(`SetFortTypeActualStateNotifyAndSavedKindReader`·`ClientOwnerAcrossRendererAndWorld`·`ActualClientOptionsAndStableDebugWords`·`PrioritySameClientConfigReaderAndPartialEffects`의 실제 Client 부분)는 장치 단위에서 대조한다.
- MP: ChangePal의 도전(challenge) 분기와 AI 진단 reporting의 메타맵 양성은 cpppj와 같이 고정 관측 범위 밖이다(훅/getter 자리만 보존).
- CD 10.72는 범위 밖이다. 이 입력들을 `GameMenuCommands` 소비 순서(Console/정지 관문 앞뒤)에 끼우는 일은 Client 입력 부모를 옮길 때 한다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등). 남은 작은 client 입력 후보는 `CreateAtInput`·`DetonateInput`·`ProductionShortcuts`·`RitualChoiceInput`(미옮긴 하위 장치 의존)이고, 작은 o 후보는 `RawEnergySupply`·`RawTotalMade`·`RawProjectileGeometry`·`RawMemStream`·`RawFileName`, 대본 부모 `RawAddRandom`/`RawDestroyAt`/`RawMakeAi`/`RawCreateAt`이다.
