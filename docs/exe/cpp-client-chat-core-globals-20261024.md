# Client 수명의 실제 Chatmanager·CoreData 소유자

2026-10-24 후속 **단위385 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 이전 [최소 접합 제안](cpp-client-chat-core-owner-proposed-20261024.md) 작성 뒤 신규 소유자 두 개와 실제 helper 어댑터를 구현했다. 이전 문서의 ‘소유자 미구현’은 작성 당시 단계이며, **Client.Run/전체 Session 연결은 현재도 미적용**이다. 공유 기존 소스/CMake/LEFT/git·공용 build/lib·게임/GUI에는 변경이나 실행이 없다.

제품 신규4파일은 `client/ClientChatManagerGlobals.h/.cpp`, `client/ClientCoreDataGlobals.h/.cpp`이고 검사 신규1파일은 `tests/ClientChatCoreGlobalsTests.cpp`다. 동결된 RawChatManagerPrefix/RawPlayerCoreData 본문과 원문 fixture는 수정하지 않았다. 두 소유자와 동결 helper 두 개·검사·작은 runner만 자체 경로에서 컴파일해 **최종 build2 종료0·컴파일 경고/오류0,5검사/실패0**을 확인했다. 최초 build1은 namespace 닫는 괄호 누락C1075와 검사fill의int→BYTE 경고가 있었고 이를 교정했다. 최초 로그는 `extracted/cpp-client-chat-core-globals-20261024-isolated/build1.log`로 보존한다. 실제 제품/전체CTest 증거로 확대하지 않는다.

## Chatmanager의 실제 할당과 current

`ClientChatManagerGlobals::Current()`는 실제 한 DWORD를 참조로 반환하고 새 owner의 최초값만0이다. `Allocate(2712/128)`는 **값 초기화 없는 실제 `new(std::nothrow) uint8_t[size]`**를 소유한다. host 포인터를32비트로 축소하지 않고 비영 opaque토큰을 map의 실제 unique_ptr/길이에 연결한다. 할당 실패는0이고 잘못된 크기/토큰 소진은 별도 장치 계약 오류다. `Bytes(token)`는 같은 실제 배열 span이며0/미등록 토큰은 접근 시 거부한다.

Allocate 자체는 current를 바꾸지 않는다. 실제 `InitializeChatManager`가 생성자 뒤 현재 슬롯을 대입한다. 이 부모는 이전 manager를 free하지 않으므로 current를 바꿔도 이전 manager와 그 내부128BYTE 할당의 토큰/주소는 계속 유효하다. 별도 원문free부모를 복원하기 전에는 모든 할당을 owner의 최종 파괴까지 보유한다. 이를 전체 Chat 종료/명령 수명 복원 완료로 표현하지 않는다.

`MakeClientChatManagerHooks(shared_ptr<...>)`는 같은 owner를 캡처해 **실제 allocate/memory/작은 CRTcopy**만 연결한다. 기존 모듈의 실제 append 기본 몸체는 유지하지만 registerCommand/readConfig/maximumRank/minorCount/core는 비어 있다. 따라서 공통 초기화에서 실제 명령 registry가 없으면 생성자 효과 뒤 첫 registerCommand 위치에서 실패한다. 이름만 저장하거나 빈 성공 callback을 자동 공급하지 않는다. hook가 살아 있는 동안 실제 owner와 할당도 살아 있다. 다른 CoreData/Subscriber 참조의 수명은 호출자가 별도로 보장해야 한다.

검사는 미초기화 BYTE 값을 읽지 않는다. 새 할당은 size/data만 확인하고 전체 fill을 명시한 뒤 그 값만 읽는다. 원문 helper 실행 뒤에는 생성자가 실제 작성한 DWORD/문자열 필드만 읽는다. 재초기화에서 이전 내부 할당을 보존하는 검사도 먼저 실제128BYTE 전체에 명시 입력을 작성했다.

## CoreData의 같은 실제 슬롯과 원문 helper

`ClientCoreDataGlobals`는 현재 span 슬롯·active DWORD0·고정 cached10BYTE0·현재 Players span 슬롯을 소유한다. current/Players의 최초NULL과 active0은 새 owner에서 한 번이며 Session 교체에 맞춘 reset은 없다. cached10BYTE는 PE 원시0과 CRT004feaf0→0042ce70의4/4/2 zero store에 맞춘 최초 입력이다. 이 생성자 초기값은 정적 PE 근거와 headless 신규 owner 검사 범위이며 별도 CRT 원문 실행 fixture라고 주장하지 않는다.

`Current()/Active()/Players()`는 **실제 슬롯 참조**, `Cached()`는 같은 고정10BYTE를 반환한다. 복사/이동을 금지해 current가 자기 cached를 가리켜도 주소가 이동하지 않는다. `Borrow(PlayerWorldState&,const uint32_t& metamap)`는 실제 두 owner/known을 복제하지 않고 이 저장소와 함께 RawPlayerCoreDataGlobals로 빌린다. `Read`는 **0048f0d0의 실제 진단85와 진단 복귀 뒤 현재 포인터 재조회**를 직접 실행하고, `Select`는 **0048f3b0의 owner쓰기·4/4/2캐시·현재alias 선택**을 직접 실행한다. current만 반환하는 성공 stub가 아니다.

Players 슬롯은 실제 외부 Player 할당을 **소유하지 않는다**. 실제 PlayerWorldState/metamap과 이를 빌린 callback도 외부 소유자가 더 오래 유지해야 한다. 앞서 확인한 실제 정리부모 **004b2df0**의 선택해제→Player해제/재생성 순서가 Client/Session에 연결되기 전에는 새 클래스만으로 Session 파괴 이후의 current 유효성을 보장하지 않는다. 현재 모드의bool 표현을 임시DWORD 사본으로 넘기는 우회도 추가하지 않았다.

## 검증된 범위와 통합 잔여

5검사는 실제 heap할당과 hook 공유 수명, 재초기화한 이전 manager/내부버퍼/정의 prefix 보존, Subscriber−1의 실제 설정 키, Chat/Core helper 합성, 필수 registry 누락의 생성 부분효과, active0의 자연생략, 두 shared사용자의 동일 슬롯/캐시 alias와 다음 새 owner 분리, 진단callback 뒤 현재포인터/현재Player base슬롯 재조회를 확인했다. 명령/Config/rank개수는 **명시 headless 장치 입력**이며 실제 Client registry를 복원한 증거가 아니다.

Core owner에는 별도 CampaignWorldGlobals를 넣지 않았다. root의 Client 전체 `CampaignWorldGlobals` owner를 Sessionlocalfallback+ref로 빌리는 방향과 중복하지 않기 위해서다. 원문 최초 **Subscriber−1/loginSubscriber1/editor0/missionPost0**, 이후 실제 모드쓰기·정리부모, 실제43f660 registry/42ddf0·42def0, cachedCoreData9×16과48fb30, 실제 rank개수와 공통 Run초기화·fortTechnologyUi/rankPicture 연결은 여전히 별도 통합이다. 이 단계에서 전체 Session/Client 사본patch를 준비하거나 공유 소스에 적용하지 않았다.

공용 등록은 제품 `src/client/ClientChatManagerGlobals.cpp`, `src/client/ClientCoreDataGlobals.cpp`와 검사 `tests/ClientChatCoreGlobalsTests.cpp`다. 선행 제품 `RawChatManagerPrefix.cpp`·`RawPlayerCoreData.cpp`가 필요하며 소유자5검사 자체의 추가 fixture macro는 없다. 원문 두 모듈의 기존644/804 fixture macro와 검사는 별도로 유지한다. 최종 소스/로그/격리 실행파일 SHA는 [manifest](../../extracted/cpp-client-chat-core-globals-20261024-validation.json)에 고정한다.
