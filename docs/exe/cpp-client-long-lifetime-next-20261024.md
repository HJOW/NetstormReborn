# Client 장기 전역과 Session 해제의 단계별 접합

2026-10-24 **후속386 사본 제안**이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 [Chat/Core 설계](cpp-client-chat-core-owner-proposed-20261024.md), [Core804 관측](cpp-player-core-data-20261024.md), [정리294 관측](cpp-state-world-cleanup-20261024.md), [저장상태542 관측](cpp-saved-state-read-20261024.md)을 읽고 실제 소유권 연결 순서를 좁혔다. 원문 몸체/최초 자료와 공유 기존 소스·CMake·LEFT·git·공용 build/lib·GUI는 변경하거나 실행하지 않았다.

첫 제안은 **ClientMain.h/.cpp와 새 검사1개, 총3파일**이다. [사본 patch](../../extracted/cpp-client-chat-core-owners-20261024-proposed.patch)는 이미 복원된 `ClientCoreDataGlobals`와 `ClientChatManagerGlobals`를 Client 생성 시 한 번 소유하고 같은 `shared_ptr` getter를 노출한다. menu_/Session보다 먼저 선언하여 소비자 반납 뒤에도 owner가 살아 있게 한다. **Session alias·실제 Chat startup·모드 정리·전체 checkpoint 성공을 연결한 patch는 아니다.** 구문 검사2소스가 종료0/경고오류0이며 새3검사는 실행하지 않은 초안이다. [검증 기록](../../extracted/cpp-client-chat-core-owners-20261024-proposed/verification.json)에 입력·사본 SHA와 한계를 남긴다.

[ColorSort 현재385 제안](cpp-colorsort-input-current385-20261024.md)과 ClientMain 두 경로가 겹친다. 전용 사본에 **ColorSort68ccbf77→이번 owner 제안** 순으로 각각 `git apply --check/apply --whitespace=error`를 실행해 네 단계 모두 종료0이었다. [순차 적용 기록](../../extracted/cpp-client-chat-core-owners-20261024-proposed/merge-check.json)에 최초15파일 SHA와 결과를 보존한다. 실제 문맥 충돌/수동 합성은 없으며 이 확인을 두 변경의 통합 빌드·실행으로 세지 않는다.

다른 작업자가3파일/3검사초안을 읽어 ctor 선언순서·menu 반납 때 owner 생존·Select0 뒤 Player 반납·명시 fill 뒤 BYTE 읽기에 명백한 오류가 없음을 확인했다. 이 읽기 검토도 실제 검사 실행과 구별한다.

## 현재 위치와 원문 순서

| 경계 | 현재 구현과 필요한 실제 수명 |
|---|---|
|`ClientMain.cpp:162` 생성자·`:178` 소멸자|Badges/TypeConversion/Random/LocalState는 이미 Client owner다. 현재 Chat4800BYTE 기록은 실제 manager2712BYTE와 다르다. 소멸은 Config callback 해제→menu_.reset→음향→화면/창 순서다. 새 두 owner는 이 소비자보다 오래 살며 getter는 할당/초기화를 실행하지 않는다.|
|`RawCampaignSessionLogin.inc:19,24,94,98,126`|currentCore/lastKnown10/coreActive가 현재 Session별 저장이다. Server Login·ClientPackets·LocalZone 세 소비자를 모두 같은 owner 참조로 바꾸어야 한다. 일부만 공유하거나 Begin마다 복제·reset하지 않는다.|
|`RawCampaignSession.cpp:333,3016`|Player1620BYTE는 Session vector다. 현재 소멸자는 AI/Kernel/통지 참조를 정리하지만 Native Core 선택해제는 하지 않는다. 공유 current가 이 vector를 가리키면 Session 파괴 뒤 dangling 된다.|
|`UberGump.cpp:288` ClearWorld|실제 UI/미션/입력을 먼저 반납하고 world_=nullptr 뒤 Kernel.Remove로 Session을 반납한다. 현재 이 경로에 전체 Native State 정리는 없다. current Subscriber를 menu/world 경유로 조회하면 이때 원래 전역 대신0이 된다.|
|`RawCampaignSession.cpp:431,738`|CampaignWorldGlobals는 값형 Session 멤버이고 ExistingEditor ctor는 editor1/missionPost1을 쓴다. 현재 다음 Campaign은 새 local 값형 editor0을 얻는다. 전체 전역을 공유하면 이 암묵적 초기화가 사라져 Editor 잔류를 먼저 해결해야 한다.|

실제 **004b2df0**은 **004b3127 known0→004b312d SelectCore(0,1)→004b31ad PlayerInitialize004910a0** 순서다. Core0은 이전 current10을 **4/4/2**로 캐시에 복사한 뒤 currentNULL/active0을 쓴다. known0은 외곽 부모의 별도 쓰기다. Player 해제·재생성 뒤 **004b3246 editor0**, 캡처 State29/33 외에는 **004b3257 SelectCore(1,1)**이므로 정상 정리의 최종 current가 새 Player를 가리킬 수도 있다. 소멸자에서 무조건 clear/캐시복사를 추가하거나 모든 옛 Player vector를 보존하는 방식은 이 부모를 대신하지 못한다. 마지막 Player 할당을 언제 반납할지도 최종 선택 뒤 소비자 수명과 함께 정해야 한다.

`CleanupStateWorld`의 주소식별 getter/setter는 같은 실제 필드를 직접 읽고 써야 한다. 현재 ownerMode.challenge는 bool인데 `ClientCoreDataGlobals::Borrow`는 const DWORD 참조를 받는다. bool을 재해석하거나 임시 DWORD에 동기화하지 말고, 후속 작은 API 변경에서 현재값 getter 또는 실제 canonical DWORD 표현을 정한 뒤 원문 진단 후 재조회를 다시 검사한다. 미선택 하위 hook은 선행 요구하지 않고, 도달한 필수 효과가 없으면 그 위치에서 중단하며 앞선 직접 쓰기를 되돌리지 않는다.

## Subscriber·SavedReader·SP의 공유 범위

Subscriber는 새 단독 DWORD를 중복 소유하기보다 기존 `CampaignWorldGlobals` 한 개를 Client에 두고 Session이 local fallback 또는 같은 객체의 참조를 빌리는 형태가 작다. 실제 Client 최초값은 **mySubscriber=0xffffffff/loginSubscriber=1/editor0/missionPost0**이다. 독립 Session의 기존 기본값과 Begin의 subscriber 선택 시점은 보존한다. `Client::CurrentSubscriber()`도 이 owner를 직접 읽어야 Chat의 두 Config 조회 사이 현재값 변화를 관측한다. 공유 전에는 앞의 Native editor0/missionPost 선택을 실제 전환 경로에 연결해야 하며 struct 전체 재초기화로 Subscriber를 지우지 않는다.

[SavedReader](../../cpppj/src/o/RawSavedStateRead.h)는 같은 실제 Player base getter·현재 Subscriber 두DWORD·previousHandle·State 꼬리 전역을 빌려야 한다. **9×16BYTE cachedCoreData00595360/metadata0048f350**은 lastKnown10BYTE005953f0와 다른 장부다. 현재 그 실제 장부/초기화0042ce70/metadata0048f350은 미연결 필수 장치이므로 새 owner의10BYTE를 대신 공급하지 않는다. 파일24BYTE ctor/read/close와 정확한 예외 장치도 현재 BaseFile vector 읽기로 완료된 것이 아니다. SavedReader의 이름은 실제 pending 이름 setter에 연결하고 현재 Fort 이름을 미리 바꾸지 않는다.

실제 WinMain에서 **004391ed→SP Initialize0040eba0**, **0043952c→PlayerInitialize004910a0**, **00439840→ChatInitialize0042e050**, **00439845→00422490**, **0043984a→BadgesReset0047d860** 호출을 PE 정적으로 확인했다. SP는 옵션/팔레트보다 앞에서 초기화된다. 현재 Session ctor `:731`은 매번 `sp.BindTrackingOffset(priestState.regen.patch.sequence);sp.Initialize()`를 실행한다. Client 공유 GameRandom만 연결한 현재 단계는 이 초기화 횟수/팔레트 이전 난수 소비까지 복원한 것이 아니다.

후속 SP owner는 같은 GameRandom·CRT/clock reseed를 빌리는 **한 ScrambledSpStore와 추적DWORD0054d4e0**를 Client에 소유해야 한다. 실제 offset00568af8·32WORD순열·32×9DWORD pool·firstPool/initialized도 같은 객체에 남긴다. 기존 사제 sequence 소비자들을 그 실제 추적필드 참조에 함께 연결한 뒤 Run의 원래 위치에서 한 번 초기화하며 Session마다 재초기화하지 않는다. 새 RNG/임시 tracking/프레임 복제로 합성하지 않는다.

## 다음에 묶을 최소 검증 단위

1. 이번3파일은 **소유자와 주소 고정만** 적용한다. 새 검사초안은 실제 Client(no Run)의 getter/할당 수명, 실제 Core0→캐시→빌린 Player 해제, Client 밖으로 유지한 owner/hook과 새 Client 분리를 확인한다. 미초기화 Chat BYTE는 명시 fill 뒤에만 읽는다. 자동 Session 정리나 Native startup을 실행한 검사로 세지 않는다.
2. Player 재할당/마지막 반납 정책과 Native cleanup을 먼저 연결하고, Login의 세 current/cache/active 참조를 한 번에 바꾼다. 정리 중 예외에는 prefix 효과가 남으며, 안전한 재시도/반납 판단 없이 예외를 삼키고 Player를 해제하지 않는다. Editor→Campaign의 editor0·missionPost·Subscriber 보존과 최종 재선택 주소를 실제 두 Session 검사로 검증한다.
3. 같은 canonical CampaignWorldGlobals와 Client getter를 연결한 뒤 실제 Chat startup/registerCommand/Config/Core/rank 장치를 잇는다. 이름3개를 저장하는 빈 registry로 성공시키지 않는다. 이어 SavedReader의 실제 Player·9기록 cache·파일 소유자를 공급한다.
4. SP 장기 owner와 startup 위치는 별도 단위로 옮겨 팔레트/사제/저장 writer의 **같은** RNG와 tracking을 검증한다. 최종HUD/State/loaded·전체 Editor 설치·실제 사용자 플레이 완료는 이 작은 수명 patch의 범위 밖이다.
