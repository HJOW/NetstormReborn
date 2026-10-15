# 실제 캠페인의 받침 확정과 로컬 간헐천 연결

현재 PC와 마지막 전체/정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. AGENTS와 양 LEFT의 최신 인수인계를 확인했다. 이번 단위는 새 원본 기계어 관찰이나 GUI 실행 없이 기존 Confirm/Notice/Clear/Place/Packet·PlayerBounds·LocalGeyser 근거를 재사용한다. 보호 원본·AGENTS·dotnetpj는 변경하지 않는다.

`RawCampaignConstructionBindings`는 같은 `RawSurfaceWorld`의 Pool/hash/spot/TYPE188/프레임/장부와 실제 HP/SP를 빌린다. 로컬 간헐천이 보내는 `noIsland, x-2, y-2, Player0, 나머지0` 확정만 공개한다. 서버 SID 생성→실제512바이트 패킷 작성→가변길이 최종화→현재 `RawBattleBroadcast`→같은 `RawConstructionNotice` 직접 처리→예측 `Clear`→실제 `Place/Owner/Pop`→표시 갱신 순서다. 서버 SID를 로컬 예측 SID로 대체하지 않는다.

noIsland는 실제 TYPE의 HP/needsBuilder/데이스 분기에 속하지 않아 Notice의 일반 사제/운반/건설 Start를 선택하지 않는다. Player0은 Place의 SP 비용 분기를 자연 생략한다. 미연결 일반 hook은 예외이며 일반 TYPE 요청은 첫 factory 이전에 거부한다. 기록기가 켜진 상태 역시 명시 오류다. 이 범위를 일반 Construct 메뉴 활성화나 썬 캐논 건설 완료로 해석하지 않는다.

로컬 생성은 같은 실제 `RawPlayerGeyserBounds`와 `RawLocalGeyser`를 사용한다. Player9의 현재 flags/region/AI 포인터, 현재 ChunkMap·모드 참조·방향표·mapSize·영역 TYPE을 읽고, factory/Pop/empty 삭제/받침 확정을 실제 월드에 연결한다. 전용 최근5목록과 전체 간헐천 목록의 동일 객체 연결은 생성에서 거부한다. 자원은 `SquidRewardState.geyserPool` 주소에 직접 바인딩하며 `LocalGeyserState.resourcePool`에 복사/동기화하지 않는다.

## 실제 Session 설치와 호출

실제 `RawCampaignSession::Impl`이 첫 객체 생성 전에 `ConnectConstruction()`을 실행한다. `RawCampaignConstructionState`, 최근5목록, 실제 mapSize 초기BYTE0과 Player 방향9DWORD `{16448,4,0,6,2,7,3,5,1}` 및 바인딩을 같은 고정 주소에 소유한다. Player0 방향표와 수량3/4는 이번에 보호 PE 초기 데이터를 읽기 전용으로 확인했으며 새 기계어 고유입력으로 합산하지 않는다.

첫 실제 Session 검사에서 예측 noIsland 삭제가 Hit/Lifetime의 제한된 자산 분배 밖이라는 오류를 발견했다. 같은 Factory가 설치한 **noIsland 실제 VT `00506f08`**와 현재 타입이 함께 일치할 때만, 기존 `RawSquidDestroyDispatch`의 전체 삭제와 `RawSurfaceLifecycle`의 공통 Pre/실제 SurfacePostDestroy를 연결했다. 종속/송신/선택은 기존 Form/내용물/감시 분배를 유지한다. 연결은 실제 공통 삭제 후설치 뒤·첫 객체 전 수행하며 실패를 빈 성공으로 바꾸거나 검사를 약화하지 않는다.

공개 `ConfirmSupport(float,float)`와 `GenerateLocalGeysers()`는 Load 정상 종료 뒤에만 허용한다. 각 명시 요청에서 `PrepareConstruction()`이 실제 `assetState.localOwner/boss`, `core.processState.server`, `core.ownerMode.fort`, 현재 piece/bridge/noIsland/island 번호를 소규모 호출 상태로 공급한다. Player9/지도/자원/목록은 복사하지 않는다. 바인딩 자체는 불일치를 계속 검사한다. bounds의 battle/metamap은 `core.ownerMode.battle/challenge`, mission은 원본594fa0에 대응하는 `objectState.mission`의 동일 참조다. 읽기 전용 `Construction()`은 같은 Pool/Bounds/최근목록/자원/마지막 wire 확인에 사용하며 게임 요청은 Session API로 보낸다.

생성자에는 같은 world/hp/sp/rewardState·metadata.frames·chunks·Player9·실제 broadcast, 위 장기 상태와 최근5/전체 geysers 목록을 직접 넘긴다. 실제 Geyser PostPop/삭제/Regular/AI 등록은 세션의 기존 자산/수명 모듈을 사용한다. `MakeCampaignClientServices`는 현재 Config의 기본값 보존 `ReadInt`, 실제 `Client.WallSeconds`, 현재 Renderer의 `InvalidateAll`, 원본 진단 뒤 계속하는 로그 소비자를 공급한다. 외부 서비스가 누락된 독립 소비자는 기존 `FillMissing`의 명시 오류를 유지한다.

`GenerateLocalGeysers()`는 로그인/AI/미션 부모가 정한 위치에서만 명시 호출한다. 자동 호출/자동 Stage3는 추가하지 않는다. 실제 `CreateAi()` 뒤 nonnull Player AI가 있으면 Geyser PostPop의 `RawGeyserAiRegistrationBoundary`가 현재 owner/token/SID/operation을 보존해 거부한다. 이미 생긴 받침/서버SID/전체 목록 추가 등 원본 순서의 부분 효과를 되돌리거나, 해당 AI를 null로 바꿔 성공시키지 않는다.

## 검증 상태

`CampaignConstructionTests.cpp`의 보호 장면은 수정 TYPE74 공용표 대신 `GameAssets.TypeTable()`의 실제188개 TYPE와 실제 프레임/SHP를 그대로 사용한다. 공통 삭제는 실제 HP/SP/자원 pool에 후설치하고 실제 삭제 writer/NULL 연결 방송, Geyser PostPop·Damageable·AI 현재 조회·Regular 부착과 기존 DirtyRegions를 조립한다. 기본 삭제 관찰 callback으로 결과를 성공 처리하지 않는다.

독립 집중2개는 예측 조각 삭제·서버9SID·0x3f 길이/내용과 실제 hash0/spot, 일반TYPE의 쓰기 전 거부, 현재 비활성/활성 Player 전환, 실제 PlayerBounds, 전용 최근목록/전체목록, 실제2000수량/자원 차감/Regular 및 전역 불일치 거부를 다룬다. 실제 `CampaignSessionTests`의 추가3개는 설치 전 요청 거부/자동 미호출, 동일 Session의 예측 삭제와 패킷/공간, 현재 raw 참여 전환/요새 guard/최근3개/공유 자원9000→3000/Regular, 실제 CreateAi 후 nonnull 등록 경계와 실제 AI 해제를 검사한다. 명시 입력의 참여 flag 변경은 원본 로그인/Stage3 실행 근거로 해석하지 않는다.

상위가 client/tests CMake에 신규 모듈/검사를 등록했다. 실제 VT 수명 교정 뒤 **Release 종료0·경고0·오류0, 집중49개 실패0**이다. 그룹은 Construction2·Session15·ClientPause2·FrameInterval3·AiBrain4·AiInitialUpdate2·PlayerParticipation2·TypeRequiredEnergy3·rawGunTarget4·GunProcess7·FenceTarget5다. 이 중 이번 단위 신규 검사는 Construction2+Session3이고, ClientPause2는 함께 빌드한 별도 정지 소유권 단위의 검사다. 나머지는 원본/현재 getter·실제 Session 회귀이며 새 native 고유입력은0이다.

최종 로그는 `extracted/goal-construction-final-build.log`, `goal-construction-final-focused.log`다. 초기의 Session 예측 삭제1실패 로그 `goal-construction-pause-focused.log`와 초기 빌드도 그대로 보존한다. 최종 전체 CTest는 **1/1·내부1,256개 실패0·193.66초**로 통과했다. 전체 로그는 `extracted/goal-units62-66-ctest.log`, 내부 전체 출력은 `goal-units62-66-tests.log`다. 현재 소스/CMake/기존 fixture/로그 SHA와 초기 실패·VT 교정·집중/전체 결과는 [`cpppj/session-construction.json`](../../cpppj/session-construction.json)에 보존한다. 같은 기존 native 입력의 회귀 재생 수를 새 관찰 수에 더하지 않는다.

앞선 단위63의 `cpppj/session-ai.json`과 AI 상세 문서의 소스 SHA는 커밋 `c9bc0f4` 당시의 역사 스냅샷이다. 이번 Session/서비스/검사 확장 이후의 바이트가 그 이전 SHA와 같아야 한다는 의미가 아니며 단위65의 새 검증 결과와 구분한다.

일반 Construct/커서 preview 수명과 반환·로컬 요청/wire 수신·사제 접근점·건설 운반 teleportEffect/manaBolt 경로/선택/속도·도착·건설 Process/시각/표시/음향·생산/다리·자연 캠페인 진행은 이 단위 밖에 남는다. 원본 Windows10/11 전체화면 DirectDraw 오류도 계속 제외한다.
