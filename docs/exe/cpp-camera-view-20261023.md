# 실제 ViewHome/ViewYourPriest·Carrier preview·ReloadTheams 연결

2026-10-23, 현재/마지막 전체 정밀 디컴파일 호스트는 **VM-W11-Codex / 2026-10-23**으로 같다. AGENTS.md·공통/닷넷 인수인계를 다시 읽고 기존 refined C와 보호 PE의 실제 명령을 확인했다. 새 디컴파일·빌드·GUI·게임·커밋은 하지 않았다. 아래 새 검사는 아직 실행하지 않았다.

## CenterOn과 현재 Carrier 분기

`Netstorm.c:42152`, PE `00433ba0..00433c28`은 SID0이면 반환하고, 현재 raw+0b의 free/dead mask3이 있으면 다른 대상을 찾지 않고 반환한다. raw+0e/+12 좌표를 읽고 실제 `004ac550` TYPE의 전체 signed footX/+1d4,footY/+1d8을 각각 뺀 후 float32에 저장한다. `00433aa0`/`00497f20`의 현재 원점 적용 뒤에는 **004ac200을 다시 호출**한다. `00433c0e..00433c1b`에서 현재 TYPE+ec(flags2)의 mask0x30000을 보고 `00426080(sid)`을 실행한다. flags1이나 이전 TYPE 사본으로 판정하지 않는다.

공개 `CameraHome::CenterOn`은 위 경로를 사용하고, `CameraHomeHooks.contentPreview`는 Session의 기존 `StartSelectionContentPreview(pool,sid,objectState.loadingDepth,processes.AddRegular,assertion)`에 연결한다. 이 기존 helper의 원문은 `Netstorm.c:30884`, `00426080`이다. 실제005c89b8이0이고 raw+28의mask9가0이며 타입이70이상인 경우에 같은 Kernel에 **491, payload0x3e4ccccd(0.2)** Regular를 추가한다. 임의 카메라 표시 객체를 대신 만들지 않는다.

기존 `CameraHome`의 Reset은005c89b8 증가/감소를 독립 UI state로만 보유했으므로, Session 경로에는 현재 깊이 getter/setter를 추가했다. `Impl::Camera`가 같은 objectState.loadingDepth를 읽으며 setter는 기존 `world.SetLoadingDepth`로 실제 표면/사제/Carrier/자산의 같은 전역을 갱신한다. Reset은 현재 깊이+1 → 중심 이동/preview → **효과 뒤 현재 깊이−1** 순서이고, 호스트 장치 예외에서도 같은 깊이를 복구한다. UI state.suppressionDepth는 이 현재 값의 표시이며 실제 전역을 덮는 별도 카운터가 아니다. 독립 기존 Reset 검사는 훅이 없을 때 기존 state 저장소를 계속 사용한다.

## ViewHome과 ViewYourPriest

`RawCampaignSession::CenterCameraOn(uint32 sid,state,apply)`는 DWORD SID를 검증한 뒤 직접433ba0을 실행한다.0은 자연 반환한다. `ViewHome`은 현재 `playerState.battleOwner`로 같은180BYTE Player를 읽고 **+7c가 비0이면 그 SID만**,0일 때만+84를 사용한다. 첫 SID가 dead여도 두 번째로 바꾸거나 GameWorld.Home/Reset의 사제/신전 탐색을 하지 않는다. playerKnown/Challenge/원본 진단은 외부 `GameMenuActions`의 기존 명령 관문에 남긴다.

`004950d0`은 `Netstorm.c:119415`, PE `004950d0..004950ea`에서 `00494f10(owner)`의 비0 반환을433ba0으로 tail-jump한다. Session의 `ViewYourPriest(owner,state,apply)`는 기존 **RawPriestParticipation::Find** 본체를 재사용한다. `Netstorm.c:119322`의 살아 있는 목록 우선 → 첫 포획 Carrier → 마지막 유효 제단 경로이며 `RawPriestLookup::Find`만 호출하는 근사로 바꾸지 않는다. owner는 BYTE로 자르거나 로컬 소유자로 대체하지 않는 전체 DWORD다.

실제 `GameWorld::Home(false/true)`는 Session의 현재 ViewHome/로컬 ViewYourPriest에 연결했다. 새 `CenterOnObject(uint32 sid)`는 명령이 선택한 SID를, 새 `ViewPriest(uint32 owner)`는 Players 메뉴가 선택한 명시 owner를 Session에 전달한다. 세 경로 모두 현재 width/height와 같은 cameraHome를 사용하고 원점 적용 시 scrollX/Y → changed → SurfaceView의 같은 표시/위치 음향을 갱신한다. 원본 경로에 테두리 clamp나 반발자국 보정을 더하지 않는다. 비campaign 값형 월드의 기존 Home/스크롤 동작은 유지한다.

## ReloadTheams의 실제 목록과 지역 재계산

기존 `GameMenuActions`가 각 반복의 현재 count/SID 조회와 목록 순서를 소유한다. Session getter는 다음 실제 저장소를 매 호출에 다시 읽는다.

| 논리 목록 | 원문 전역 | 실제 저장소 |
| --- | --- | --- |
| First, second=false |0055a70c/0055a714|`assetState.outposts`|
| Second, second=true |005e189c/005e18a4|`core.bookkeeping.factories`|

`GameMenuReloadCount(bool second)`는 현재 count DWORD를 signed32로 해석한다. `GameMenuReloadSid(bool second,index)`는 같은 현재 Items 뷰의 DWORD SID를 반환하며 목록을 캐시하지 않는다. 두 목록 매핑은 `RawAssetPostPop.h:40` 및 기존 지역 소유 문서/원문을 따른다. `ReloadTheamAt(xbits,ybits,0)`은 좌표 float 비트를 보존해 기존 `ownership->UpdateRegion(x,y)` → `RawRegionOwnership::Update(...,mode0)`의 실제455380 투표/지형 도장을 호출한다. 이 명령 caller의0이외 mode는 명시 거부하며 별도 지역 근사를 만들지 않는다.

## 새 검사와 남은 검증

새 `CameraViewCommandsTests.cpp`의 네 검사는 현재 TYPE 재조회/flags2, SID0/free/dead/void 및 preview loading/extra 관문, 실제 깊이 공유/효과 뒤 현재 감소, 장치 예외 복구를 확인한다. 새 `CampaignViewCommandsTests.cpp`의 세 검사는 보호 미션을 실제 Session/GameWorld에 설치하여 Player 두 SID 우선순위·dead 첫 대상·현재 화면 크기·명시 SID/owner·같은 Kernel491 예약·Reset 중 실제 로딩 깊이·Reload 현재 목록을 확인한다. 별도 새 fixture 매크로는 없다. 기존 `CameraHomeTests.cpp`의 두 판본 native fixture는 Reset/중심 좌표 회귀검사로 유지한다.

이 변경은 새 native oracle을 실행하지 않았다. 기존 `tools/decomp_camera_home.py`의1182행/판본 Reset 관측과 기존 RawPriestParticipation/SquidSelection 관측을 재사용하며, 새 계약 검사를 native 전체 UI 관측이라고 주장하지 않는다. 새7검사와 기존 회귀검사의 빌드·실행, 실제 메뉴/키/화면의 사용자 동작 검증은 상위 단위에서 수행해야 한다. 원본 Challenge 미초기화 좌표 및 깊이UINT_MAX 감김의 기존 명시 제외는 유지한다.
