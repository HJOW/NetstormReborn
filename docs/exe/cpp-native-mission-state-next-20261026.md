# 일반 미션의 Native State7과 준비된 World 대여 후속 설계

2026-10-26, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 변경하지 않았다. 기준은 10.78 `originals/Netstorm.exe` SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽고 원본 PE, 기존 정밀 C, 현재 제품 코드를 읽기 전용으로 대조했다. 이번 결과는 **후속 설계이며 아직 구현하지 않았다.** 제품 코드·기존 검사/fixture·원본·정밀 export를 바꾸거나 게임/빌드를 실행하지 않았다.

467의 `LoadMissionWorld` 앞 `CleanupNativeStateWorld(1)`과 `adoptInitializedPlayers=true`는 실제 Client9/덱 반납 및 Player9 재초기화 중복을 해결하는 필요한 조치다. 그러나 그 뒤 일반 Session/표면의 fresh 대여가 같은 장기 전역을 다시 초기화하고, 기존 Login은 State7 전체를 실행하지 않고 Zone 설치 뒤 State8로 진입한다. 따라서 이것만으로 원본 State7 전체가 연결됐다고 기록하면 안 된다. 기존 Editor 전용 `adoptPreparedWorld` 관문과 선택 UI scope를 넓히지 말고, 실제 State7을 통과한 일반 미션에 별도 준비 증빙과 설치 경로를 추가해야 한다.

## 원문과 현재 훅 이름의 중요한 차이

기존 `RawLocalMissionState::Starting`은 부모 순서를 보관하지만, 일부 훅/멤버 이름은 하위 본문의 의미와 다르다. 기존 `tools/decomp_local_mission_state.py`는 아래 하위들을 stub하여 `Selection`, `Seed`, `Music`, `Cursor`라는 관찰 문자열을 남긴다. 이 이름 또는 해당 fixture 통과만으로 실제 하위 의미가 연결됐다고 판단하지 않는다. 기존 fixture는 보존하고 새 독립 관찰과 실제 Client 검사를 추가한다.

| 기존 이름 | 실제 원문 의미와 현재 재사용 지점 |
| --- | --- |
| `clearSelection` / `004978d0` | 사용자 선택 해제가 아니다. `00540ce0`의 기본 배경 분모를 `00540ce4`의 현재 분모로 선택하며 달라질 때 `0059a8b0=1`을 쓴다. PE `004b74f1~004b74fd`는 실제 기본값을 push한다. `ClientCameraGlobals::Denominator()`와 `SelectDenominator(defaultValue, WorldRedraw)`를 재사용한다. `ClearClientUserSelection`에 연결하지 않는다. |
| `seed`, `randomSeed`, `ClientLocalStateGlobals::RandomSeed()` / `00440c60`, `005c85b8` | 게임 난수 시드가 아니라 **CD 표식 발견 결과 0/1**이다. 캐시 경로 `00558dd4`를 먼저 검사하고, 실패하면 논리 드라이브를 순회하여 `cd_netstorm_titanic_activision` 파일을 찾는다. 캐시 무효화/성공 경로 저장도 본문에 포함된다. `GameRandom` 또는 현재 시각을 대입하거나 이 값을 `00532710`의 시드로 쓰면 안 된다. 후속 구현에서 이름을 바로잡되 기존 물리 주소·로그인 별칭은 유지한다. |
| `stopMusic` / `00469fc0` | 음악 정지가 아니라 현재 `inBattle`에 따른 **장면 음악 시작**이다. State7은 `inBattle=1`을 쓴 뒤 호출하므로 `004558f0(0,4000)`의 실제 게임 난수를 소비하여 원소 곡을 선택한다. 실제 `ClientAudio::StartScene(true)`가 같은 Client `GameRandom`을 사용한다. |
| `refreshMission` / `00463a40` | 현재 Guide의 `+0x800` 객체가 있을 때 가상 `+0x28`을 호출한다. 현재 구현의 `tutorial->Gate().PauseForDialog()`에 해당한다. Guide의 Header/Init/Start를 다시 실행하는 지점이 아니다. |
| `refreshCursor` / `004993c0` | 커서 새로고침이 아니라 **Native Renderer 초기화**다. 앞64BYTE view, 목표/한계 `+48~+58`, 별도16DWORD snapshot `0059a900`, redraw1, MapRevision 증가를 실행한다. PE `004b75c5~004b75cc`는 실제 Renderer와 인자1을 전달한다. 기존 `Client::InitializeNativeRendererView()`/`ClientCameraGlobals::InitializeNativeRenderer()`를 재사용한다. |

근거는 정밀 `Netstorm.c:53159`의 CD 검색, `86526`의 장면 음악, `80871`의 Guide 가상 호출, `121859`의 배경 분모, `122821`의 Renderer 초기화다. `004978d0`의 실제 PE store는 현재 분모→redraw1이며, 정밀 C 표시 순서와 구별한다. 이 차이는 기존 `ClientCameraGlobals.cpp`에도 반영되어 있다.

## 원문 State7 부모의 실행 순서

정밀 `Netstorm.c:145829~145892`와 PE `004b74f1` 이후를 대조했다. 정밀 C는 일부 인자를 생략하므로 reason과 인자 폭은 실제 PE를 따른다.

1. 실제 기본 배경 분모를 `004978d0`으로 선택한다. **cleanup 전** `005c85c0` ZonePending을 지역 변수로 캡처하고 배경 이름 `00565e70`의 첫 BYTE만 0으로 쓴다.
2. 현재 블록 `+0x84`가 비영이면 현재 gameTicks `0055b4ac`와 sourceType `0055a420`을 캡처한다. 초기 지역 두 값은 0이다.
3. PE `004b7528 push 1`→`004b752a call 004b2df0`으로 전체 cleanup을 실행한다. 실제 Deck 해제, Core0 뒤 Runtime 반납, Pool/목록/Player 재초기화 순서를 지킨다. Guide 자체를 이 cleanup 때문에 새로 만들지 않는다.
4. `00440c60`의 실제 CD 검색 결과를 `005c85b8`에 쓴다. `0042e440`으로 현재 Chat의 부모에 숨김 명령을 보낸다. 이후 **현재 `+0x84`를 다시 조회**하여 비영일 때만 캡처한 gameTicks/sourceType을 복원한다. cleanup 전 판정값을 고정하여 재사용하지 않는다.
5. 현재 `00594fbc=1`을 쓰고 `00469fc0`으로 장면 음악을 시작한다. 현재 inMission과 missionPost가 둘 다 참이면 같은 Guide에 가상 `+0x28`을 보낸다. simulconnect가 비영이면 별도 `004e2430` 서버 부모가 필요하다.
6. 1단계에서 캡처한 ZonePending이 비영이면 `004b4be0`으로 Zone을 설치한다. 이때 인자는 캡처한 Zone 핸들이 아니라 **호출 직전 현재 블록 `+0x80`의 재조회 값**이다. 설치 후 free도 현재 `+0x80`을 재조회한다. free→현재 `+0x80=0`→next `+0x80=0`→`004c1e70` 맵 처리기 등록 순서다.
7. `004993c0(Renderer,1)`의 실제 Renderer 초기화→PlayerKnown0→`00466280(currentHudCommand,2,0,1)`의 실제 HUD mode2 순서다. 기존 호스트 정수 `uiMode`만 바꾸거나 refresh만 호출하는 것으로 대신하지 않는다.
8. `004180d0`이 **그 위치의 실제 벽시각**을 읽어 ActivityEnd `00544d58=wall+10`으로 쓴다. cleanup에 있는 별도 ReconnectSaveEnd 계산과 합치지 않는다. immediate 두 번째 DWORD `005c86cc=20`→`004b2bc0(8)` 즉시 요청 순서다.
9. 현재 `+0x84`를 다시 조회한다. 비영이면 savedGame1→현재 Fort 재접속 로드→재접속 진행창이다. 0이고 missionPost가 참이면 미션 시작 진행창이다. 0이고 missionPost가 거짓이면 다른 일반 전투/서버 UI 부모가 필요하다. 이후 현재 `battlePal` 조회→팔레트 적용을 실행한다.
10. 다른 상태와 같은 **144BYTE 공통 확정**을 거쳐 즉시 요청8을 같은 `Run()`에서 실행한다. State7 중간에 phase8/14를 직접 쓰거나, current 번호만8로 바꾸어 공통 확정을 생략하지 않는다. State8의 예약9와 이후 수신은 기존 큐 및 phase 확정 규칙을 유지한다.

ZonePending의 캡처가 특히 중요하다. cleanup은 실제 전역 ZonePending을 0으로 만들지만, 원문은 앞서 캡처한 값으로 Zone 설치 여부를 결정한다. 실제 요청 이미지와 현재/next 블록, 전역 카운터를 서로 복사본으로 대체하면 이 분기를 잃는다.

## 현재 연결과 미연결 경계

- `MissionFortRequest.cpp::WriteMissionFortRequest`는 현재 Fort를 선택한 뒤 실제 주소 필드의 두 DWORD와 마지막 BYTE, 포트, next7, Zone 복제 핸들, ZonePending 증가를 원문 위치에서 수행한다. `CaptureMissionFortZone`은 호스트 포인터를 DWORD로 자르지 않고 단독 소유한 실제86BYTE Zone과 논리 핸들1을 보관한다. 이 소유권을 유지하여 현재 핸들에 해당하는 이미지가 설치/해제되도록 해야 한다. `request.sequence`는 작성 시 캡처 값이며 나중의 ZonePending 별칭이 아니다.
- 현재 `UberGump::LoadMissionWorld`는 로딩 안내를 표시한 뒤 배경 첫 BYTE0/cleanup1을 호출하고 `adoptInitializedPlayers=true`로 실제 Player9를 대여한다. 이후 실제 Guide/Options/선택 Fort/장기 소유자를 연결하지만, 일반 `RawCampaignSession`은 여전히 fresh 대여를 사용한다. 이 조치는 State7 나머지 prefix·Zone·HUD·공통 확정의 완료 증빙이 아니다.
- `RawCampaignSessionLogin.inc::Login::StateHooks`는 주로8/9/10/13/14의 연결/로그인 부모를 연결한다. State7의 분모 선택·cleanup·CD 검색·Chat·음악·Guide·Zone 해제·맵 처리기·실제 HUD mode2는 연결하지 않는다. `uiMode`/`inputMode`의 현재 지역 값+refresh도 실제 Native HUD/입력 부모와 구별해야 한다.
- 현재 `Login::Begin`은 요청 next를 current에 복사하고 Zone을 직접 설치한다. request 이미지를 반납하고 현재 `+0x80`을 지운 뒤, current 번호를8로 써서 `Run/Poll/Pump`를 실행한다. 실제 State7 공통 확정 및 next `+0x80` 해제 위치를 모두 실행한 경로가 아니다. 새 경로에서 이 `Begin`을 그대로 다시 호출하면 Zone/서버/로그인을 중복 설치한다.
- `LoadMissionWorld` 말미의 실제 `StartScene(true)`와 Guide `PauseMissionWorldEntry(...PauseForDialog)`는 현재 로그인·World/HUD 생성 뒤에 있다. 새 State7에서 이들을 원문 위치로 옮긴 경우 같은 새 경로의 후행 호출은 제거해야 한다. 다른 호환 경로까지 일괄 제거하지 않는다. cleanup 내부 음악 호출은 당시의 실제 모드를 읽는 별도 원문 호출이므로 State7의 battle1 뒤 음악 호출과 합쳐 생략하지 않는다.

## cleanup 뒤 fresh 대여가 바꾸는 실제 물리

`adoptInitializedPlayers`는 `InitializePlayerRecords` 중복만 막는다. `RawCampaignSession.cpp` 생성자의 `gameplayUse`, `RawSurfaceWorld`의 `storageUse`, 후속 모드/Core 선택은 별개다.

| 현재 fresh 경로 | 원문 cleanup 이후 보존해야 할 값과 차이 |
| --- | --- |
| `ClientGameplayGlobals::Acquire()` | special/userCheat 두 설정만 보관하고 Player 제어 상태, 커서/배치, finder, editor/missionPost, Server/Boss, FullDeck, 에너지, 사용자 선택을 다시 초기화한다. Native cleanup 뒤 Server/Boss false가 true가 되는 등 명시 쓰기 순서가 바뀐다. `finder.sharedBoss`가 실제 권한 주소인 점은 유지하되 finder 전체 초기화를 암묵 추가하지 않는다. |
| `ClientGameplayGlobals::Use`의 일반 소멸 | PlayerKnown0·커서 초기값·editor0을 다시 쓴다. Native cleanup이 C++ 대여를 반납하는 경우에도 이 암묵 쓰기를 피하는 수명 정책이 필요하다. 기존 prepared 대여/State 반납 정책을 재사용하되 실제 부모 증빙이 있어야 한다. |
| `RawSurfaceStorage::Acquire()`의 GameAux reset | LostX/Y를 원문 cleanup의 **−10000**에서0으로 바꾸고, 현재 GeyserType을122로 바꾼다. ordinaryGeysers/비용/자원/환불/포획/Archipelago/사제 체력도 fresh 기본값으로 쓴다. 이 reset이 GameAux 모든 필드를 지우는 것은 아니다. GeyserFrameFlag, 저장 개수·기한, default SP, subscriber 표지는 이 함수가 쓰지 않는다. |
| 같은 Acquire의 Pool/hash/spot·권한·실패점 | generation 비영이면 Pool/hash/spot을 또 초기화한다. boardTarget 전체 기본값과 Server/Boss true를 쓴다. 실제 cleanup의 Pool 꼬리/삭제 이력·부분 레코드 쓰기 및 이후 모드 선택과 같은 호출로 합산하지 않는다. |
| `priestOwnerState.fill(0)` | 원문은 지정 owner1..8을 초기화한다. 전체9칸 fresh 쓰기는 owner0까지 바꾼다. |
| `sidLists.PrepareGeneralWorld()` | 여덟 SID 목록의 개수뿐 아니라 본문까지 지운다. 원문은 현재 등록된 헤더의 **개수만0**으로 쓰며 inactive 본문을 보존한다. 사용자 다중 선택의 개수/본문도 같은 관점으로 대조한다. |
| `totalMade={}` | 원문의3개 표 각각 앞255개 reset과 달리256번째 값까지0으로 만든다. |
| `graph={}; ResetRawGraphTable(...)` | 원문 부분 reset이 보존하는 reserved/구멍/stack 본문도 먼저0으로 만든다. |
| `islands={}; ResetRawIslandRecords(...)` | 실제100개 섬 레코드 부분 reset 밖의 padding BYTE를 먼저 지운다. |
| `chunks=ChunkMap{}` | 원문의 현재 크기/모드를 보관하는 Reset과 달리 기본16×16 크기/모드 및 물리 셀 전체를 새 값으로 바꾼다. |
| `RawSurfaceWorld::Impl`의 fresh 링크/모드 | CampaignLinks 전체와 ownerMode 기본값을 다시 쓴다. 반대로 기존 prepared bool 경로는 **Editor 전용이라 지역 `mode.editor=true`를 쓴다.** 일반 미션에서 bool만 켜면 잘못된 Editor 모드가 된다. |
| Session의 PlayerKnown/Core 후속 쓰기 | 비prepared이면 PlayerKnown1을 쓰고 `Core.Select(1,1)`을 다시 호출한다. `adoptInitializedPlayers=true`도 이것을 막지 않는다. 실제 Core 선택은 owner/known/cache/active와 연결된 효과이므로 순수 참조 연결로 간주하지 않는다. |

`RawSurfaceStorage::AcquirePrepared()`와 `ClientGameplayGlobals::AcquirePrepared()`의 본문은 중복 대여를 거부하고 대여/호스트 세대만 관리하므로 재사용할 수 있다. 그러나 이 두 함수가 존재한다는 것 자체는 Native State7 완료 증빙이 아니다. 현재 Session의 Editor 전용 선행검증은 올바르게 일반 사용을 거부하고 있으며 그대로 유지한다.

## 별도 준비 증빙과 일반 미션 설치 경로 제안

다음 이름은 설계안이다. 공개 서비스에 `prepared=true` 하나를 더 추가하거나 `ExistingEditor` 조건을 `|| Campaign`으로 넓히는 방식은 사용하지 않는다.

1. Client 수명의 Native State7 조정자를 둔다. World/Session이 없는 시점에도 실제 Camera, State 다섯 블록, Zone 요청 소유자, 장기 Player/Core/SP/Random/Kernel, HUD, Audio, Guide를 조회할 수 있어야 한다. `RawLocalMissionState`의 State7 부모 순서를 그대로 사용하되 앞 표의 잘못된 훅 이름과 실제 효과를 정정한다. 기존 Runtime 내부에서 스스로를 파괴하는 cleanup을 실행하지 않는다.
2. 실제 cleanup1 정상 반환 및 필요한 prefix의 성공을 기록하는 이동 전용 `MissionWorldPreparationPass`를 추가한다. 생성자는 외부에 공개하지 않고 같은 Native State7 조정자만 발급한다. 별도 `CampaignInstallMode::NativeMission` 또는 전용 생성 API를 둔다. **기존 `adoptPreparedWorld`의 Editor 전용 검증·전송 계약·Scope는 변경하지 않는다.**
3. 증빙에는 같은 Client/State/Zone 요청, SurfaceStorage와 세대, 실제 Player base와 Core owner, 같은 GameAux/Authority/Subscriber/SP/Random/Kernel을 묶는다. 앞선 실제 Runtime 반납 증빙과 새 World 준비 증빙을 혼동하지 않는다. 다른 Client, stale 세대, 중복 소비, 재초기화된 Player base, 이미 대여 중인 물리, 잘못된 실제 Core 선택, 현재 Zone 이미지 부재를 **게임 물리를 쓰기 전에** 거부한다.
4. Session/RawSurfaceWorld의 새 설치 API는 해당 증빙을 소비하여 두 실제 `AcquirePrepared()`와 `PlayerUse::AdoptCurrent()`를 조립한다. 기존 fresh 기본값·Player 재할당·Core 재선택·links 덮어쓰기·Editor 지역 모드 설정을 하지 않는다. 초기 지역 소비자에 필요한 battle/editor/mission/fort 값은 그 위치의 실제 Native 전역으로 연결한다. 기존 Editor의 고정 Core1 검사 등은 새 미션 계약과 별도로 유지한다.
5. 조립 위치는 실제 `installZone`에 진입하기 전까지 준비된 Player/물리를 빌리고, 그 훅에서 기존 `InstallLocalMissionZone`과 실제 전송/패킷/서버 장치를 실행하는 구조가 작다. C++ 장치 조립 자체가 게임 전역을 쓰지 않는지 검사해야 한다. 장기 State 실행기는 Session보다 먼저 존재하며, 장치 연결 뒤에도 같은 State 블록과 소유자를 계속 사용한다.
6. 기존 Login을 장치/핸들러 조립, 현재 Zone 설치, State8 이후 진행으로 나눈다. 새 Native 경로에서는 `Login::Begin`의 합성 Zone/직접 current8 쓰기를 호출하지 않는다. 원문 State7이 Zone 설치→현재 핸들 free/양쪽0→맵 핸들러→Renderer/HUD→진행창/팔레트→공통 확정→즉시8을 끝까지 소유한다. 같은 로그인/전송 모듈을 재사용해도 부모를 두 번 실행하지 않는다.
7. 실제 HUD mode2/3 설치와 Native Runtime 공개 시점을 분리한다. 부분 생성 Session을 일반 Draw/입력/미션 프레임에 먼저 공개하지 않는다. 공개 전에는 조정자가 필요한 실제 장치만 조회하며, 기존 Editor UI의 선택 공장 차단이나 Borrowed/World 관문을 우회하지 않는다.

증빙 발급 시점은 cleanup 뒤 모든 prefix가 끝났고 Zone 설치가 아직 시작되지 않은 위치가 적절하다. 단, 원문에 없는 추가 callback 때문에 현재 Zone 핸들/모드가 바뀌었다면 매조회 의미를 그대로 따라야 한다. 증빙을 이유로 이후 원문 조회를 캡처값으로 바꾸지 않는다. 예외가 나면 Native 앞선 부분 쓰기를 임의 롤백하거나 새 Session으로 성공 처리하지 않고 명시 실패/정리 경로로 반환한다.

우선 완결 범위는 실제 Zone 요청이 있는 일반 단독 미션, simulconnect0, reconnect `+0x84=0`으로 한정할 수 있다. 이 범위는 정상 분기 전체를 구현하고 검증한다. ZonePending0, 재접속 Fort, 일반 전투/멀티플레이 분기는 각각 원문에 따른 별도 경계로 남기며 빈 callback으로 성공시키지 않는다. 10.72는 대응 주소가 달라 별도 관찰 전까지 새 Native 미션 계약의 대상으로 넓히지 않는다.

## 후속 검증

- 독립 PE 부모 관찰은 State7 진입부터 공통144BYTE 확정과 즉시8까지 실행한다. 실제 입력 블록/ZonePending/전역 별칭을 주입하고 CODE/helper SHA를 보관한다. 큰 OS/음향/네트워크 장치만 명시하여 통제한다. 실제 배경 분모, CD 경로 검색, Renderer 초기화는 별도 실제 본문 관찰과 결합하여 기존 stub 이름에 의존하지 않는다.
- cleanup 전 `ZonePending=비영`, cleanup 중 전역0, 현재 Zone 핸들 교체/해제 시 재조회, `+84` 전후 변경, title의 뒤79BYTE 보존, gameTicks/source 캡처·복원 순서를 각각 구별하는 입력을 둔다. CD 발견0/1·캐시 유효/무효·드라이브 순서와 게임 난수 불변을 별도로 확인한다.
- 실제 Client→State3 noWorld Deck→미션7에서 덱 블록이 같은 저장소로 반환되고 Player9가 한 번만 새로 초기화되는지 확인한다. 새 Session 조립 전후 물리 전체를 비교하여 허용된 호스트 대여/세대와 장치 참조 외에 추가 쓰기가 없도록 한다. sentinel은 SID 목록 inactive 본문, TotalMade 각255번, Graph 구멍/stack, 섬 padding, 청크 모드/현재 크기/비활성 셀, priestOwner0, 현재 GeyserType, LostX/Y의 −10000에 둔다.
- Audio는 cleanup 시점과 State7 battle1 시점의 두 실제 호출을 순서대로 관찰한다. 실제 공유 GameRandom의 소비 횟수/상태를 비교하고 후행 `LoadMissionWorld` 호출 중복을 검출한다. Guide 자기 정지 깊이도 정확히 한 번 증가하는지 확인한다.
- 실제 Renderer 앞64BYTE/목표/snapshot/redraw/MapRevision과 실제 HUD mode2→로그인 mode3를 같은 Client로 확인한다. 장치 조립 전/공통 확정 전의 phase와 수신큐를 관찰하여 너무 이른 phase14, Zone 중복, 재진입, 미도킹 Draw를 검출한다.
- 증빙의 다른 Client/세대/Player, 중복 소비/동시 대여, 미완성 prefix, Zone 이미지 누락을 각각 실패시켜 기존 Editor Scope/일반 Session 호환 관문과 실패 전 물리 보존을 확인한다. 기존 Editor 준비 대여와 원본 fixture 본문은 그대로 재검사한다.
- 통합 뒤 같은 제품으로 초기 메뉴→일반 미션, State3 복귀→재진입, 미션 내 재시작, Guide 미션 및 비Guide 미션을 확인한다. Early 자연 진행을 이어가되 이 검증을 전체 캠페인/데모 완주 완료로 합산하지 않는다. 빌드·게임·LEFT·커밋은 통합 담당이 수행한다.

관련 근거: [Runtime 반납](cpp-state-runtime-retirement-20261026.md), [실제 전체 정리 통합](cpp-whole-state-runtime-20261026.md), [Native State3](cpp-native-main-menu-state-20261026.md), [장기 Deck/무World 생산 동기화](cpp-client-production-sync-runtime-20261026.md). 앞선 State7 덱 감사는 `extracted/cpp-add-island-468-staging/docs/exe/cpp-state7-deck-retirement-audit-20261026.md`에 있으며 준비본이고 Git 산출물은 아니다.
