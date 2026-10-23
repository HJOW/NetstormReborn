# Edit 전체 Snapshot의 다음 연결 경계

2026-10-23 단위377 뒤 후속 설계다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. AGENTS·공통/닷넷 인수인계를 재독하고 기존 정밀 C·보호 PE·현재 제품 소스만 읽었다. 새 디컴파일·원본 실행·GUI·빌드·검사·git과 제품 변경은 없다. 단위377의 실제 Objects/Temp 통합과 검증은 [현재 실행 기록](cpp-fort-objects-runtime-20261023.md)을 따른다. 아래 신규 API·검사 범위는 **제안이며 미구현**이다.

## 두 전체 부모의 정확한 순서

근거는 [10.78 정밀 원문](../../extracted/refined/originals/Netstorm.c)의152737~152884행이다. `this`는 호출 때 고정한 Template 한 칸이고, 현재 소유자·모드·manager는 원문이 읽는 위치에서 재조회한다.

| 부모 | 진입 관문 | 실제 child 순서 |
|---|---|---|
| **004bf9a0 Fort** | manager 비0(Template.cpp 0x530) → inFortMode00594fb8 비0(0x531) → this가 현재00540cac의 Template칸과 같음(0x532) | **TypeNames004bd6f0 → Technology → Objects004be430 → Territory004bf6d0 → Deck004befd0 → Money → Mission → Badges004be7f0 → Notify004bd360** |
| **004bf880 Battle** | manager 비0(0x50e) → inBattleMode00594fbc 비0(0x50f) → isMine(0x510) | **Technology → Deck → Objects → Territory → Money → Mission → CoreData004beb30 → Notify** |

Technology 구간은 두 부모에서 manager 비0(0x4a6)을 진단한 뒤 **현재 기술 부모0048fc60 → 현재 manager로 Create("Technology")0044d1b0 → 004bcea0**다. Money는 현재 manager로 Create("Money") 뒤004832c0이다. Mission은 manager 비0(0x1e8) 진단 → 현재 manager로 Create("Mission") → Guide00463a80이다. 각 child가 current 섹션을 바꾸어도 이미 받은 stream 참조는 유지한다. 부모 전체의 manager를 한 번 저장하거나 현재 선택 stream으로 쓰기를 대체하지 않는다.

**Battle에 TypeNames/Badges를 더하거나 Fort와 Deck 순서를 통일하면 원문과 달라진다.** Battle의004beb30은 Badges가 아니라 조건부 **CoreData 저장**이다. manager 비0/!inMetamapMode00594fc8 진단 후 missionPost00594fa4와 editor005c85a4가 둘 다0일 때 현재 Player+6c의10BYTE·005c89b4의WORD와 검증용 float들을 쓴다(152053~152107행). 이번 Fort 저장 단위에서 이를 임의로 Badges로 치환하지 않는다.

004bfac0은 `if (battle) Battle현재칸; if (fort) Fort현재칸;`이며 두 독립 if다. 004bfb50은 **처음 현재칸 manager가 비0인지 한 번 검사**한 뒤 같은 두 if를 수행한다. Battle 콜백이 현재 owner/fort를 바꾸면 두 번째 if와 Fort 호출은 바뀐 현재 값을 쓴다. `else if`, 두 호출의 공통 슬롯 캡처, 매 child마다 현재칸으로 this 교체를 하지 않는다.

## Money·Mission·Badges의 최소 writer

**Money004832c0**(105801~105816행)은 현재 SP를 저장하지 않는다. **같은 게임 난수004558c0(0x3f2) → float 변환 → PE0050d7c4의333.0f 합산 →0044cce0(stream,4BYTE)**다. 보호 PE의 해당 상수를 직접 읽어333.0f를 재확인했다. 난수 상태00532710은 기존 `RawCampaignSession::Impl::random`/`GameRandom`과 같아야 하며, 한 번 전진한다. 반환값333..1342는 float32로 정확히 표현된다. `Sp.Get`, 과거 Money, 별도 RNG/재시드로 대체하지 않는다. 과거 [잔여 감사](cpp-editor-gap-audit-20261023.md)의 ‘현재 SP’ 문장은 이 원문 근거로 정정했다.

**Mission00463a80**(80901~80912행)은 Guide00562ca0+800의 실제 Mission 포인터가 비0일 때만 쓴다. 이 관문은 Tutorial 여부가 아니다. 현재 `guideHooks.current`는 guided만 반환하므로 그대로 재사용하면 일반 Battle을 누락한다.

1. Guide 버퍼의 **현재 이름**을004bcbf0으로 기록한다. 첫 NUL 앞 길이의 낮은 **WORD2BYTE → NUL 제외 실제 문자열 전체 길이**의 두0044cce0 호출이다(150360~150384행). Mission Config의 정규화한 fileName이나 loadFort 이름을 쓰지 않는다.
2. 문자열 쓰기 뒤 Guide+800을 다시 읽고00482c40을 호출한다. 그 Mission 객체의 **+58 sequence DWORD를 읽어4BYTE쓰기 → 그 뒤 +54 flags DWORD를 읽어4BYTE쓰기**다(105427~105442행). 두 값을 처음에 함께 캡처하지 않는다.

Client 연결은 `UberGump::MissionRuntime::state`의 같은 sequence/flags와 현재 Guide 이름을 빌린다. 현재 생성 경로에서 이름 원문은 `FontBytes(mission_->Name())`/`missionRuntime_->init.name`에 있지만, Guide의 이름 수명과 같은지 불변식을 명시해야 한다. 새 `CampaignSnapshotMissionHooks`에는 `current()->MissionStartState*`와 `name()->string_view`를 두고 **guided 조건 없이 현재 Runtime**을 조회한다. Session 서비스에는 UberGump를 캡처한 현재 getter를 공급하고 Runtime 포인터를 장기 캡처하지 않는다. `ClearWorld`는 Runtime을 Session보다 먼저 반납한다. 현재가 없더라도 부모는 이미 새 Mission 섹션을 만들었으므로 **빈 새 섹션**이 남아야 한다.

**Badges004be7f0**(151852~151907행)은 manager 진단(0x412) → Create("Badges") → 처음 data+pos의 count BYTE 포인터 보관 → pos/end를1 진행하고 count0 쓰기 → **type0..254 전체255회**0047d7c0 조회다. 조회 DWORD가 비0이면 type BYTE와 count의 낮은 BYTE를 각각 인라인으로 쓰고 처음 보관한 count BYTE를1 올린다. 처음 count0 쓰기에는 일반 용량 진단이 없고 이후 두 BYTE 쓰기에는 각각 MemStream0x24 진단이 있다. 타입255는 순회하지 않으며 count256/음수라도 비0이면 항목을 쓰고 낮은 BYTE만 저장한다. getter 순서를 정렬한 파일 목록이나 ‘양수 count만’으로 바꾸지 않는다.

Badges getter0047d7c0은 전역 **[00593160,00594970),28×220BYTE**를 선형 검색하고 첫 type DWORD 일치 기록의 **+216 count DWORD**를 돌려준다(101987~102009행). 미일치는0이다. 28번째 기록00594894는 CRT004fcbe0이220BYTE 전부0으로 만드는 **type0 기록**이므로 getter/type0을 생략하면 안 된다(221958~221962행). 앞27개 초기 type은 보호 PE의 참조 DWORD 기준 `71,72,73,78,128,111,136,133,140,139,138,75,142,141,137,148,77,79,131,135,143,144,76,147,129,146,145`다. 전체 레코드의 나머지 필드는 MetaDisplay 초기값이므로 임의 덮어쓰기하지 않는다.

현재 Session에는 live Badges 장부/`RawFortMetaLoad` 인스턴스가 없다. 장부는 **Client 수명 한 개**를 소유하고 getter·기존 `RawFortMetaLoad::FindBadge/SetBadge/LoadBadgesSelected`와 새 writer가 같은 span을 빌리게 한다. 기존 로더의 private count 읽기를 중복하지 않도록 같은 장부의 `BadgeCount(type)` 공개를 추가할 수 있다. counts만0으로 쓰는0047d860의 실제 호출자는 Client 초기화00438dc0/004395df다. Session 생성·Template Clear 때마다 전체 장부를 새로 초기화하지 않는다. 원시 Fort Badges 섹션은 장부의 대체물이 아니다.

## 005c89c0 alias는 Client 전체가 소유한다

원본 manager 생성자 **004bcc60**은 manager+4/+8, vtable,+10c/+110을 쓰고 **005c89c0에DWORD0**을 쓴 뒤 파일명을 복사한다(150400~150423행). 객체 전체 저장004be430은 같은 DWORD에2를 쓰고 각 Terr/Chaff에서 당시의 낮은 BYTE를 읽는다. 로더004be610과 단일 객체/내용물의 버전 분기도 같은 DWORD를 쓴다.

현재는 `CampaignTemplateStorage::managerGlobal_[4]`와 `RawCampaignSession::Impl::objectState.version`이 별도다. 더구나 `SoloMapCatalog::Impl`도 **자기 CampaignTemplateStorage storage**를 소유하므로 메인 Storage와 Session만 연결해도 부족하다. 목록 분류/행의 임시 manager 생성도 실제 같은005c89c0을0으로 바꾸어야 한다.

권장 소유권/API는 다음처럼 한 가지로 좁힌다. **제안이며 이번 문서에서 구현하지 않았다.**

- `FortSerializationGlobals { std::uint32_t version{}; }`를 UberGump/Client 수명에 한 개 소유한다. `shared_ptr`로 메인 TemplateStorage, SoloMapCatalog의 임시 Storage, 독립 Session의 실제 Storage에 주입한다. 독립 검사에만 별도 Client를 뜻하는 자체 Globals를 허용한다.
- `CampaignTemplateStorage(shared_ptr<FortSerializationGlobals>)`와 `SerializationVersion()->uint32_t&`를 제공한다. Image 생성 시 같은 실제 DWORD의 BYTE뷰를 기존 `ConstructTemplateDataManager`에 넘긴다. **기존 byte array를 uint32_t&로 캐스팅하지 않는다.** 실제 uint32_t 객체를 먼저 소유하고 그 객체의 바이트에 접근한다.
- `FortObjectLoadState.version`을 필수 생성자로 받은 **uint32_t&**로 바꾸고 Session의 objectState를 `templateStorage->SerializationVersion()`에 직접 바인딩한다. 기존 `state_.version` 비교·대입과 ObjectWriter/`SaveFortObjects`의 참조가 곧 같은 전역이 된다. 독립 검사는 명시한 localVersion을 공급한다. 값 멤버+초기/마지막 복사나 proxy의 bit_cast로 alias를 가장하지 않는다.
- `SoloMapCatalogEffects` 또는 생성자에 같은 Globals를 전달한다. catalog 자체 헤더/manager는 계속 독립 소유하되 생성자의 전역0쓰기만 공유한다. 메인 Template9를 임시 분류 작업으로 덮어쓰지 않는다.

이 전역은 manager 생성 때만 원문대로0을 쓰며 `ClearAll`, Session 재생성, CreateSection에 추가0쓰기를 넣지 않는다. Clear는 manager 생성과 다르다. 먼저 이 alias를 완성한 뒤 generic Editor/NewFort에 실제 새 manager 생성을 연결한다. loadingDepth005c89b8·scope005c89b0·형변환표005c89bc는 별도 전역이며 이번 version 통합을 핑계로 함께 초기화하지 않는다.

## 기존 모듈과 전체 부모의 소유 분담

`RawFortSnapshotParent` 신규 모듈은 Fort/Battle 부모와 두 현재칸 wrapper의 **관문·순서·재조회만** 맡긴다. 기존 `SnapshotFortTypeNames/Technology/Objects/Territory/Deck`를 같은 Session에 재사용한다. Money는 Session의 private random을 쓰는 명시 child로, Mission은 위 실제 Client getter를 공급받은 child로, Badges는 같은 live 장부 child로 연결한다. 전체 부모가 시작 때 manager를 저장하지 않도록 `manager(slot)` getter를 두고 `create(manager,name)`의 호출 시점을 원문대로 보존한다. Technology의0x4a6와 Mission의0x1e8 진단은 부모에 둔다. 선택된 누락 child는 앞선 효과를 보존한 채 명시 실패해야 한다.

**Notify004bd360**(150825~150857행)는 현재 manager.fileName 첫 글자`.` 진단 → Template+8이 비0일 때+0c 진단/현재+8 재검사 후 반환 → inMission00594fa0 진단 → manager 비0 진단 → +10 preventSave 관문 →0044d8d0 순서다. 현재 `RawPlayerTemplateState::Changed`의 빈 이름 선행 throw는 원문이 첫 NUL을 읽을 수 있는 것과 다르므로 다음 Notify 연결 단위에서 별도로 교정한다. manager0의 실제 메모리 접근 경계와 빈 정상 C문자열을 혼동하지 않는다.

Session은 아직 `BindFileSave`를 설치하지 않았다. 현재 owner00540c70, readOnlyForts0054db30, hiddenPlayers0052f81e의 같은 현재 값을 `DataManagerFileSaveState`에 공급하여 **Changed→Storage.SaveFile→RawFortImage/SaveDataManagerFile→EditorFiles.Writer**를 연결한다. `EditorFilesTests`의 실제 작업본 검사 구성을 재사용한다. 저장 억제 판정은 이미지 생성 **뒤**에 있으므로 이른 Notify 생략으로 바꾸지 않는다. 이름 변경/NewFort/작업 namespace와 Client next 상태는 별도 [Client 후속 설계](cpp-editor-client-next-20261023.md)가 맡는다.

다음 독립 검증은 전체 부모의 원문 x86 trace(진단/두 모드/현재 owner·manager 변경/child 중단/순서), Money의 실제 난수 상태+4BYTE, Mission의 존재/빈 이름/문자열 후 포인터 교체/sequence 쓰기 후 flags 변경, Badges의 type0·254·255/중복/음수·256/count 포인터/용량 진단을 분리한다. 통합 검사는 **현재 월드 변경→전체 부모→실제 작업 파일→새 저장소·새 Session 재로드**, 그리고 **객체 버전 변경→catalog 임시 manager 생성→같은 version0 관측**을 포함해야 한다. 원문/모듈 통과만으로 Edit 사용자 경로·전체 게임 동일성 완료를 선언하지 않는다.
