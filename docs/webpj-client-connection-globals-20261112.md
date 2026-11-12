# webpj Client 수명 전역 세 묶음(Subscriber·연결 활동·연결표) — WB04-E325 (2026-11-12, 4-C)

3.1절 4-C 의 client **ClientConnectionGlobals**(기준선 28줄, 기준선 뒤 +17줄)·**ClientConnectionActivityGlobals**(29줄, +3줄)·**ClientSubscriberGlobals**(22줄, +6줄)를 각각 `core/client-connection-globals.js`·`core/client-connection-activity-globals.js`·`core/client-subscriber-globals.js` 로 옮겼다. 세 클래스는 같은 Client 가 Session·연결 교체보다 오래 쥐는 원문 전역의 **소유자**이며 규칙 본체가 아니다: 소비자(Init·월드 로드·Chat 접두·연결 활동 몸체·등록소)는 값을 복사하지 않고 이 객체의 상자/배열을 **빌린다**. 빌리거나 반납할 때 초기값으로 되돌리지 않는 것이 핵심 계약이다.

## 1. 규칙

- **Subscriber:** `my`(00540c74, PE 초기 `0xffffffff`)와 `login`(0054265c, PE 초기 1)을 `{ value }` 상자 두 개(`mySubscriber()`·`loginSubscriber()`)로, 현재 사용자 이름(PTR00540ca8)을 64BYTE `Uint8Array`(`handle()`, 최초 전체 0)로 보관한다. 접근자는 호출마다 같은 객체를 돌려주고(C++ 의 같은 주소), 상자 쓰기는 DWORD 폭으로 감긴다(`-1 → 0xffffffff`, `2^32+5 → 5`). 상자 형태는 기존 소비자가 이미 받는 형태다: `CampaignWorldGlobals.borrow({ my, login })`(E287)과 Tutorial Init 의 `globals`(`loginSubscriber` 상자 + `initialSubscriber()` 조회), Chat 접두의 `subscriber()` 조회. 주의: **Tutorial Init 의 `loginSubscriber` 쓰기 대상은 00540c74(my)이고 읽는 값 `initialSubscriber` 는 0054265c(login)** 이다(이름과 달리 현재 login 을 my 로 복사함 — 원본 관측 actualStores 가 00540c74 쓰기).
- **연결 활동:** 프레임 표(005c84e8, 22DWORD)와 벽시각 표(005c8548, 22DWORD)를 서로 다른 `Uint32Array` 로, 그 사이 Zone stamp(005c8540)·Zone+10 복사 출력(005c8544)을 별개 DWORD 상자로, 하트비트 기한(005c8620, double 상자 `DoubleBox`)과 자동 재시작(00531908, PE 초기 6)을 더한다. `tables()` 는 간극을 포함하지 않는 두 배열만 `RawConnectionActivity`(E309)에 빌려 준다. 표 정리는 프레임 22DWORD 만 지우며 간극·하트비트·자동 재시작은 암묵 초기화하지 않는다.
- **연결표:** `RawConnectionRegistry`(E308)를 확장한다. 인자 없이 만들면 독립 표(최초 BSS 0), `local` 을 주면 `local.clockSynchronization().connection`(005c8bf4)을 `{ value }` 접근자 상자로 감싸 `bindSynchronization` 으로 **사본 없이** 빌린다(빌려도 현재 값을 쓰지 않고, 쓰기는 DWORD 로 감겨 owner 의 같은 칸에 닿는다). `local` 이 null/undefined 이거나 `connection` 숫자가 없으면 거부한다(C++ `invalid_argument` "연결표의 실제 Client 동기화 owner 누락"). `shutdownDevices()` 는 앱 최종 종료 때의 `shutdownTrackedDevices()`(진행 중 부모·수신 중 노드 거부, 잔류 노드 정리, 새 발급 차단)다. 실제 `ClientLocalStateGlobals` 는 4-D 에서 옮기므로 그 전까지는 `clockSynchronization()` 최소 계약만 시험 장치가 만족한다(`resetClockSynchronization` 의 입력과 같은 형태).

## 2. 기준선 뒤 변경 판정

기준선 뒤 세 클래스를 바꾼 cpppj 커밋은 `bed549ff5`·`37dcdfc2c`·`a76499e35`·`50a5a1845`·`ec0b6e973` 다(`git log 1f00a25a..HEAD`, `git log -S` 로 추가 줄의 귀속 확인). 행 base 는 파일별 마지막 변경 커밋(`bed549ff`·`37dcdfc2`·`ec0b6e97`)이다.

- **② 반영:** Subscriber 의 `Handle()` 64BYTE(`bed549ff5`, 2026-11-07 "현재 사용자 이름을 실제 Client 수명으로 공유"), 연결 활동의 `HeartbeatEnd()`(double)·`AutoRestart()`(PE 초기 6)(`37dcdfc2c`, "실제 연결 활동과 자동 서버 이전"), 연결표의 `ShutdownDevices()`(`50a5a1845`, 앱 최종 종료의 표 정리 부분).
- **① 결과 영향 없음(GC 가 대신):** 연결표의 `devicePin`(`BindDevicePin`: 실제 Source 의 `shared_ptr` 을 호출 동안/등록 노드가 끝날 때까지 보유, `a76499e35`·`50a5a1845`)과 `ClientPacketSources::Shutdown` 래퍼(Source 순환 해소). JS 객체는 참조가 있는 동안 살아 있고 순환은 GC 가 수거한다.
- **③ 후속 단위 판정:** `PacketSources()` 의 `Resolve(connection)` 경로(Squid 대상 송신·전투 배송에서 실제 Source 선택)와 `PacketDelays()`(`ec0b6e973`, 실소켓 지연 FIFO·`lag` 옵션 선택·`ResetDefault`)는 소비자인 `ClientMain`·`ClientCampaignTransport`·Meta 를 옮길 때 단독 도달 여부와 함께 판정한다(E306 이 지연 FIFO 를 LAN 전용으로 보류한 것과 같은 묶음). 지금은 해당 메서드를 만들지 않았다.

## 3. 검사 (`tests/client-connection-globals.test.mjs` 16건)

- **원본 독립 관측 `subscriber-lifetime-edges`(v2) 120행:** 전 행의 10칼럼 형태를 확인하고 Tutorial Init 현재 login 복사 지점(004c335c) 10행을 구독자 전역 + 실제 `initializeTutorialMission` 에 연결해 재생한다(fileName 설정 callback 안의 my/login, `requestFort` 시점의 my, `header` 시점의 login, 효과 순서 clear→reset→set→request→header, `flags==1`). **첫 실행의 실패 1건은 시험 쪽 연결 실수였다:** Init `globals` 의 `loginSubscriber`/`initialSubscriber` 를 이름대로(login 상자/my 조회) 연결했으나 cpppj 시험은 `{…, MySubscriber(), LoginSubscriber()}` 순서로 연결한다(위 1절 주의). 바로잡은 뒤 10행 전부 일치했다.
- **cpppj 시험 이식:** `SubscriberGlobals_OriginalInitialValuesAndStableReferences`(초기값·주소 고정·이름 별도 저장)·`BoundConstructionDoesNotResetOrShareModes`(`borrow` 무쓰기, editor/missionPost 는 보기별)·`DirectBindingFallbackAndValueCopy` 의 상자 생략 분기(독립 0 저장소)·`ChatConstReferenceAndSharedOwnerLifetime`(현재값 재조회). C++ 값 복사/이동/교차 alias 대입(`CopyMoveAndCrossAliasAssignment`)은 JS 에 없는 언어 규칙이라 옮기지 않았다. `ConnectionActivityTests` 중 표 소유 시험 4건(`WallFrameStoresAndCurrentServerFollowNativeOrder`·`ClientEraseAndFrameZeroAreNotNormalized`·`IndependentTablesAndExplicitGapSurviveAcrossBodies`·`InvalidSenderRejectsBeforeWallOrStores`)을 전역 객체의 `tables()` 위에서 옮겼고(몸체의 signed64 변환·분배기 등록 시험은 E309 가 맡음), `ConnectionCleanup_ClientLongTablesPreserveAddressAndCurrentValues`·`ConnectionRegistration_ClientSynchronizationOwnerSurvivesBorrower` 의 계약을 최소 owner 장치로 옮겼다.
- **웹 계약 추가:** `DwordBox`/`DoubleBox` 폭 감김, 활동 전역의 하트비트·자동 재시작·Zone 칸이 프레임 표 정리와 독립인 것, 동기화 owner 누락 거부 네 갈래, `shutdownDevices()` 의 종료 순서/수신 중 거부/발급 차단.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 38개(구독자 12·활동 16·연결표 10)를 돌려 **3개가 남았고 모두 동등**하다: ① 연결표 생성자의 `!synchronization ||` 제거(`clockSynchronization()` 이 비어도 곧바로 `synchronization.connection` 접근이 `TypeError` 라 같은 오류 종류) ② 바인딩 직전의 `this.synchronization = { value: 0 }` 추가(바로 뒤 `bindSynchronization` 이 덮음) ③ `super()` 뒤 임의 필드 추가(관찰 가능한 계약 없음). 나머지 35개는 첫 실행에서 시험이 잡았다.
- **앱 수명:** 아직 Client 가 이 객체들을 만들지 않는다(PORT_MAP session 0). 4-D 의 Client/`ClientLocalStateGlobals` 조립이 `new ClientConnectionGlobals(local)`·`ClientSubscriberGlobals`·`ClientConnectionActivityGlobals` 를 한 번씩 만든다.
- 3.1절 4-C 는 22/30 이다. 다음: client `CampaignLoginServices`·`CampaignTransportServices`.
