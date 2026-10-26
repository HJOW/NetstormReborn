# SaveGoMain 뒤 실제 Native State3 부모와 공통 확정 — 467

- 작업 PC: HJOW-Athlon, 2026-10-26.
- 기준: `7711020e`의 단위466 제품. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23과 현재 호스트가 일치한다. 디컴파일 소스·원본 파일은 변경하지 않았다.
- 범위: SaveGoMain이 예약한 next3을 실제 Native 부모/공통144BYTE 확정/다음 Poll4까지 소비한다. Source3 집중172개·전체3,194개는 통과했고, 기본 진입과 SaveGoMain의 실제 GUI 통과를 확인했다. 이후 실제 미션에서 드러난 HUD/State7 수명을 보완한 Source6의 전체·GUI 최종 검증은 진행 중이며 앞선 결과와 구분한다.

## 원문 분기와 순서

패치판 `004b6fa9`는 State3일 때 current/immediate/next의 `+84`를 각각0으로 쓴다. `GetInt("intro")`가0이면 `004b2bc0(39)`를 호출한 뒤 `005c8754=1`을 쓴다. **005c8754는 별도 전역 사본이 아니라 immediate 블록의 +8c, 즉 `state.immediate[35]`다.** 이 경로는 일반 메뉴 prefix로 넘어가지 않는다.

일반 경로는 이전 phase를 signed로 비교해6 이하 또는11에서 sourceType을0으로 쓰고, 현재 결과에 따라 victory/defeat/titleMenu 배경을 고른 뒤 결과를0으로 쓴다. 남은 sourceType이 있으면 실제 TYPE 이름과 data 디렉터리의 `.gif` 존재로 별도 배경을 고른다. 이전 Battle14의 종료 통지 뒤 전체 공통 준비로 들어간다.

State1과 State3은 다음 실제 준비를 공유한다.

1. 전체 `CleanupNativeStateWorld(0)`와 기존 자식 증빙/Runtime 반납.
2. 현재 Chat 숨김, 이전 listener 반납, 같은 Player/장기 연결표의 새 local listener/client9.
3. Fort1. State1만 Editor1/missionPost1/FortKeepGenus=-1/FullDeck1을 추가한다.
4. server/boss1, 실제 Core1 선택, Native Renderer 준비, Chunk Reset/mode1, 실제87BYTE 미션 옵션 초기화, StateAuxFC4=0.
5. 첫 HUD1. State1은 기존 한정 prepared Session scope를 유지하며 State3은 Session 없는 실제 Client Deck/ProductionSync 연결을 사용한다.

State3은 pending/current 이름의 **첫 BYTE만**0으로 쓰고 제목을 갱신한 뒤 Guide를 해제한다. `WelcomeGiven=0`이면 Tell Welcome만 실행한다. 비영이면 Tell Blank→실제 startupStateCheck를 실행하며, 반환0이고00542530이 비영일 때0054da8c가0이면 UserNotUpdated,0053190c가 비영이면 Tips를 순서대로 연다. 첫안내00542530을0으로 쓰는 위치는 WelcomeGiven 비영 분기 끝이다.

그 뒤 공통 두 번째 HUD1→벽시각+10→ser22.mus→실제 Fort palette→next4를 실행한다. State3은 섬 개수 조회와 AddIsland 예약을 실행하지 않는다. 기존 `ApplyExistingEditorAfterCheckpoint`의 실제 State3 분기를 사용한다.

## 구현과 수명

- `NativeMainMenuState`가 위 State3의 선택·직접 쓰기·부분효과 순서를 소유한다. 파일/OS/UI/공통 prefix/tail은 실제 장치를 빌리며, 미선택 장치는 요구하지 않는다.
- `PrepareNativeFortState`로 State1과 State3의 실제 prefix를 공유한다. 기존 State1의 prepared Session 생성/파일 checkpoint/한정 UI scope/최종 commit 경계는 유지한다.
- State1/3의 startup은 같은 `EditorStartupStateHooks`와 실제 파일 부재 관문·벽시각을 사용한다. 기본 옵션을 강제로0으로 만들지 않는다.
- 첫안내00542530(초기1), 사용자 갱신0054da8c(BSS0), TellTips0053190c(초기1), TipNumber00531910(초기1)의 **네 DWORD**를 `ClientLocalStateGlobals`가 각각 장기 소유한다. `tellTips`와 바로 뒤의 `tipNumber`는 실제 Client 설정 변경 알림과 `InterpretOptions`의 현재값 ReadInt writer를 공유하며 음수/상위32비트를 bool로 줄이지 않는다. 원문435220은 soundQuality/soundVolume/musicVolume/maxSimulSounds 뒤에 tellTips→tipNumber를 읽는다. 현재 부분 옵션 부모도 기존 Audio 옵션 읽기 뒤 같은 순서로 이 writer를 호출하지만 음향 장치 적용을 포함한 전체435220 복원 완료로 주장하지 않는다.
- 결과005c85ac는 이미 있던 장기 `ClientAudio::SceneMusicState::resultState`를 그대로 canonical 주소로 사용한다. `ResultState()`는 mutable/const 참조이며 새 LocalState 결과0을 만들지 않는다. 실제 `CampaignClientServices`는 그 주소를 Login 장치에 공급하고 Login Timer의 기존 requestPhase도 같은 주소를 빌린다. 독립 fixture만 기존 지역0 fallback을 유지한다. SceneMusic 알고리즘은 원래 같은 필드를 읽으므로 변경하지 않았다.
- `PollEditorState`는 next1/next3/그 뒤 next4를 같은 장기 `RawLocalMissionState`로 분배한다. State3 공통 ret 뒤에는 World가 없으므로 `CommitExistingEditorInstall`이나 사용자 입력 공장을 호출하지 않는다.
- 호스트 메인 메뉴 표시는 실제 phase3/current0/immediate0/next4와 World 반납을 확인한 뒤 공개한다. 기존 Host `MainMenu()`를 그 자리에 호출하면 listener/실행 중 State를 반납하거나 Native 전역을 다시 써 버리므로 사용하지 않는다. Blank의 Host 편의 분기도 State3 실행 중에는 같은 원칙을 따른다.

## 신규 검사

`NativeMainMenuStateTests.cpp`의 독립 다섯 사례는 실제 `ClientLocalStateGlobals`, `RawLocalMissionState`, 기존 tail을 사용한다.

- 기본 안내 순서와 두 이름 첫BYTE만 변경, 전체 공통 ret, 다음 Poll4, 섬 조회·AddIsland 생략.
- WelcomeGiven0의 Welcome만 선택/startup 및 첫안내 flag 소모 생략.
- 실제 startup 질문 반환의 Tips 생략 및 Tell 예외에서 앞선 효과 보존/공통 commit 미실행.
- intro0의 정확한 immediate39와 같은 immediate[35]=1 예약, 일반 메뉴 prefix 미실행.
- UserNotUpdated Tell callback이 바꾼 현재 TellTips를 다시 읽어 Tips 선택을 생략하는 순서.

기존 검사 본문과 기대값을 바꾸지 않았다. 이 검사는 선택된 외부 장치의 실제 창·파일을 실행한 검증으로 합산하지 않는다.

`ClientMainMenuGlobalsTests.cpp`의 신규 **네 사례**는 실제 Client를 Run 없이 구성하여 네 DWORD의1/BSS0/1/1 초기값·고정 독립 주소, immediate[35]와의 비중첩, 설정 변경/실제 옵션 부모의 tellTips·tipNumber 현재값과 전체32비트 쓰기, 설정 누락/범위 초과 처리, 같은 Audio/Scene 결과 DWORD에 대한 실제 Login Timer Abort의4 쓰기와 외부 callback 후속 변경을 확인한다. 창/게임 루프/옵션 파일 저장은 실행하지 않는다.

## 독립 PE 관측과 추가 C++ 대조

[`decomp_native_main_menu_state.py`](../../tools/decomp_native_main_menu_state.py)는 보호된10.78 PE의 State3 선택/직접 쓰기/현재 재읽기와 같은 공통 ret의 실제 x87 산술을 관측한다.41개 입력을 두 x87 control에서 실행한 **82관측·27,640명령·정밀도 차이0**이며, 일반 완료58회와 intro39·changedState·중간10종 중단을 구분한다. 기본/Welcome/Startup/첫안내·UserUpdated/Tips/source/outcome/Battle/intro0, callback 뒤 현재값 재읽기를 포함한다.

동일 도구의 `--verify`는 PE/함수표/기반 도구/생성기/fixture SHA를 확인하고82관측을 재실행하여 **차이0**으로 통과했다. [원본 fixture](../../cpppj/tests/fixtures/native-main-menu-state-x86.tsv)와 [관측 보고서](../../cpppj/native-main-menu-state-native-20261026.json)를 보존한다. 제품 초안 작성 뒤 수행한 독립 PE 재검증이며 제품 예상값으로 fixture를 만들지 않았다.

추가한 `NativeMainMenuStateNativeTests.cpp`의 **신규 C++ 검사1개**는82행 전체의 다섯144BYTE 블록, 여섯 전역, 두 이름/배경 버퍼, Activity timer, 중단/도달 상태, 효과 순서와 입력 큐를 같은 제품 부모에 대조한다. Source3 집중/전체 통과 근거에 포함되며 기존 다섯 부모 검사·네 Globals 검사와 별개다.

독립 관측에서 빈 Guide와 Battle14 패킷 생성은 실제 하위 body를 실행했지만 cleanup/Chat/listener/Core/Renderer/Chunk/options/HUD/큰 Tell/startup/read/음향/TYPE/파일/팔레트/EditorPlayers는 명시 외부 장치다. State39는 도달점에서 멈춘다.82관측을 전체 GUI/OS 장치나 미복원 선택 분기의 실행 완료로 합산하지 않는다.

## 현재 통합 검증 상태

- **Source3:** 집중172개·전체3,194개 통과. 새 전역4검사와 독립 PE82행을 소비하는 추가 C++1검사도 포함한다.
- **실제 GUI:** 기본 진입과 SaveGoMain→실제 Native State3/공통 확정/메인 메뉴 복귀 통과를 확인했다. 이는 이 경로의 근거이며 사용자 전체 목표의 완료 판정이 아니다.
- **최종 Source6:** 집중181고유/189관측·신규29·전체3,194개·실패0, 같은 게임 GUI8건과 자연Early2 A~I→Tutorial3를 통과했다. 초기HUD의 실제 Runtime 가드, State7 reason1/Player대여와 첫 일반World HUD3을 포함한다. [최종 통합 문서](cpp-editor-menu-runtime-integration-20261026.md)와 [보고서](../../cpppj/editor-menu-runtime-integration-20261026.json)를 본다.

## 정확히 남아 있는 선택 경계

- 현재 기본 saved-state reader는 파일 부재 경로까지 연결돼 있다. 기존 `d/!state.dat`가 있으면 open/8Player 복구/질문 장치의 후속 연결이 필요하며 파일 삭제나 옵션 강제0으로 우회하지 않는다.
- 승패/소스가 선택한 저장상태 삭제, 이전 Battle14 종료 통지, 제목 callback이 State를 바꾼 다른 설치 분기는 이번 일반 Editor 경로의 자연 생략과 구별한다. 제품에서 선택되면 정확한 미연결 위치를 보고해야 한다.
- intro0은 실제 State39를 예약하며, State39 본체가 미연결이면 그 선택 위치에서 명시 중단한다. intro 옵션을 바꿔 일반 메뉴 성공처럼 합성하지 않는다.
- UserUpdated의 유일 원문 writer는 WinMain00438dc0의 security.dat/명령줄 검증004e3720이다. 현재 무인자·해당 파일이 없는 기본 경로는 BSS0을 유지하며, 비기본 보안 검증 writer는 아직 미연결이다. 이 값을 Config 옵션으로 대신 쓰거나 임의1로 만들지 않는다. Tutorial 소멸의 denySalvage/Ascend 장기 owner 및 별도 늦은 AI 해제 부모, 전체 미션 자연 완주·원본 표시/음향 동일성도 별도 후속이다.

현재 근거는 기본 Native State3 진입/SaveGoMain 메뉴 복귀와 위 한정 검사 범위다. 미복원 State 선택/저장상태 복구, 편집 도구·새 맵·Test Battle·시나리오, Early Missions/Priest Training/캠페인 전체와 데모3개의 자연 완주 및 원본 화면/음향 동일성은 사용자 전체 목표의 잔여로 유지한다.468 편집 작업본 목록 연결은 별도 staging 초안이며 이번 Source6 제품 검증에 포함하지 않는다.
