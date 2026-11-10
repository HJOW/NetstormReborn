# M7.3 조기/원격 희생 안내와 M7.4 이동 진단 marker — 작업1271

2026-11-11 / HJOW-Athlon. 마지막 Client 디컴파일 2026-11-03/HJOW-Athlon과 현재 호스트가 같고 새 디컴파일은 없다. **M7.3과 M7.4의 미연결 4개 장치(`CampaignUserInputServices.cpp`의 던지는 람다)를 원문 기준으로 연결했다.** 사제 희생·MP 송수신 실행은 하지 않았다(정적 대조·컴파일만). M6-n·M7.2·M8과 전체 목표는 미완료다.

## M7.3 — 조기/원격 희생 안내 (원문 `0042a360`의 case `0x11`)

보호 PE `0042a525`~`0042a54f`를 역어셈블해 확인했다. 사제가 다이스에 놓일 수 없는 시점이면(`call 00447a50`이 비영) 소유자가 로컬이면 `push 0x503d30`("`EarlySacrifice`") 뒤 `call 004cf960`(=`004cf730(name,0)` Tell), 아니면 `push eax(owner)`와 같은 문자열로 `call 004c9150`이다. `004c9150`은 70h 패킷(`z->tellLength < 256`, `.\Ui.cpp:0x4ef`)을 만들어 `004e2dc0(owner,packet)`으로 해당 소유자에게 보낸다.

- `earlyLocal` → `tell("EarlySacrifice")`(주입된 Tell 장치, 없으면 명시 오류).
- `earlyRemote(owner)` → 이미 구현된 `RawCampaignSession::SendPlayerTell(owner,"EarlySacrifice")`(004c9150/4c9200의 70h 송신). 새 패킷 작성기를 만들지 않았다.
- 입력 분기(`RawCarrierInput.cpp:100`)는 기존 그대로이며 로컬 소유자 비교 뒤 두 장치 중 하나만 호출한다.

## M7.4 — 이동 진단 marker와 커서 갱신

- `marker(point,color)` → 원문 `00434970`: 현재 카메라 snapshot으로 `497220` 투영(`ProjectCameraView`)한 화면점을 오른쪽 아래로, 현재 칸 크기(`0059a908/90c` = `RawCameraViewState::cellPixels`)만큼 왼쪽 위를 정한 뒤 **먼저 한 픽셀씩 줄이고** `004a2270`(기존 `DrawOutlineRectangle`)으로 세 번 그린다. 좌표 계산은 순수 함수 `PathMarkerRectangles`(DrawWater)로 분리해 signed DWORD 감김까지 검사한다.
- `refreshCursor` → 원문 `004a46d0`: 기존 `Screen::PresentPaletteClear`(전체 viewport 즉시 출력과 clip 복구만, 재그리기·flip 없음)를 `Client::PresentNativeCursorClear`로 노출해 연결했다.
- 이 marker는 원문 전역 `005453e0`이 켜졌을 때만 쓰이는 개발용 진단이며 일반 미션 부모가 0으로 초기화한다. 이를 켜는 새 CLI/Config를 만들지 않았다.

## 검증

Release/x64 빌드 355.99초/종료0, 경고·오류 0(`ClientMain.h` 변경으로 전체 재컴파일). 새 검사 `PathMarker_ThreeShrinkingCellRectanglesFollowOriginalOrder`·`PathMarker_WrapsSignedDwordWithoutUndefinedBehavior` 통과, `netstorm_menu_session_tests` 256건 중 실패 6건으로 1270 직후와 이름이 같다. 최종 EXE의 정상 Tutorial4 진입·생산/건설 요청·600프레임 대기·종료 코드 0(게임 시각 13.8→24.3초)을 확인했다. 사제 희생/포획·특정 적 공격·승리·MP·Join/Test Battle 실행은 0이다. 보호 원본/CD/fixture/AGENTS 2,808파일은 불변이다.

## 한계

- 조기 희생 안내의 실제 UI/패킷 동작은 사제 희생 재현이 필요해 **사용자 직접 확인 대상**이다. 원격 70h 송신은 기존 `SendPlayerTell`을 재사용한 정적 연결이다.
- marker의 실제 픽셀은 개발용 전역을 켠 상태가 필요해 실행 확인을 하지 않았다(좌표 계산만 검사).
