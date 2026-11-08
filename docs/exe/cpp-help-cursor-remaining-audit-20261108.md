# M7.1 helpWord / M7.2 메타 활성 커서 준비

2026-11-08, HJOW-Athlon. 현재/마지막 정밀 디컴파일 호스트가 일치함을 LEFT_JOBS에서 확인했다. 제품 코드·LEFT_JOBS·빌드·실행·테스트·커밋은 변경/수행하지 않았다. 이 폴더만 Git 제외 준비 자료다.

`audit.json`은 조사 당시 HEAD/29개 근거 파일 해시다. `original-fragments.c`는435220/441300/4413c0/464990/446a10/446a40/445aa0/445b30/497960의 C 발췌이며 `4413c0.asm.txt`는 보호 PE 정적 역어셈블이다. 구현 때 현행 M6 변경과 다시 대조한다.

## M7.1 — 원문 nullable helpWord 캐시

### 확정한 계약

원문435220은 `4413c0("helpWord",&0054db38)`을 SoundDir/MusicDir 뒤·technology/기록요청reset0/음향 옵션 앞에서 호출한다.0054db38은 최초NULL의 Client 전역이며 Session/World/메뉴를 다시 만들어도 현재 문자열을 보존한다.

4413c0은 단순 Config.Get 존재 조회가 아니다. 다음 선행 단계가 있다.

1.512BYTE scratch 첫BYTE를NUL로쓴다.
2.441300은 Config.Get이 성공하고 그 결과가 **현재 scratch와 다른 바이트 문자열**일때만 복사 후1을 반환한다. 결과가빈문자열이면 scratch도이미빈문자열이므로0이다.
3.441300의반환0이면 기존cachedpointer를읽지않고자연0을반환한다.
4.새비영문자열이있고cachedpointer가비NULL이면 `_stricmp(old,scratch)`를한다.0이면원래할당/정체성/대소문자를유지한다.
5.다르면옛할당free→새strlen+1할당→새pointer슬롯저장→NUL포함복사→1이다. 원문은새할당성공후옛할당을반납하는트랜잭션이아니다. 실제OS고갈/손상포인터는기존host경계로구분한다.

PE `4413d4`의scratch0쓰기→`4413d9 call441300`→`4413e3 je441457`을확인했다. 따라서아래세상태를분리해야한다.

| 기존cache | Config | 원문결과 |
|---|---|---|
|NULL|키부재|NULL/0반환유지|
|NULL|빈문자열|NULL/0반환유지|
|비NULL"Help"|키부재/빈문자열|같은"Help"할당유지|
|비NULL"Help"|"hELP"|같은"Help"할당/표기유지|
|NULL|"Help"|비NULL할당/1반환|
|비NULL"Help"|"Info"|원문순서로교체/1반환|
|외부명시비NULL빈문자열|키부재/빈문자열|비NULL빈문자열유지(NULL로합치지않음)|

Config의빈문자열을명시clear동작으로새로발명하거나,키부재/변경없음을cachedNULL로간주하지않는다. `ReadString("helpWord",freshEmpty)`를메뉴를열때마다반복하는현재구현은cache가아니므로부재/동일case의원문수명에맞지않는다.

### 파일/구조 제안

- `client/ClientHelpWord.h/.cpp` 같은단일owner를만들고 Client가생성때부터소유한다. 생성은NULL만만들며파일/GUI/Config조회가없다. 현재nullable소유문자열과표시바이트를갖고,같은cache의snapshot/identity관측을제공한다. 단순 `optional<string>`을매호출대입해같은문자열의정체성을바꾸지않는다.
- ConfigInterface의기존ReadString/ReadStringBytes를전역적으로case-insensitive로바꾸지않는다.441300과4413c0은별개원문계약이다. cache해석메서드에서fresh빈scratch→ReadString결과→현재nullable대상 `_stricmp` 순서만적용한다.
- `ClientMain.h/.cpp`에 `InterpretHelpWordOptions`와현재cachegetter를추가한다. 최초InterpretOptions의원문근접위치와실제`onChanged`/InterpretStateOptions/등록메시지재해석에서같은owner를갱신한다. 초기화직후cache값을필요한소비자보다먼저설치하고Session반납/언어변경에서임의NULLreset하지않는다.
- 실제교체만새heap객체/generation으로구별하면case동일의정체성을검토할수있다. `std::string_view`를영구보관하지않고메뉴호출때현재getter를읽거나그동기효과동안실제소유snapshot을pin한다. 새cache를ScriptStringRing/ClientStaticStrings임시고리로대체하지않는다.

### 소비자 전수 연결

| 현행파일/위치 | 변경할내용 |
|---|---|
|`CampaignObjectMenuClient.cpp::Impl::Ui`|fresh `ReadString`을없애고현재Clientcachegetter를사용한다. Factory/Variants콜백은선택위치의현재getter다.|
|`RawObjectMenu.h/.cpp`|현재 `ObjectMenuState.helpWord`는체인생성당시값사본이다. 실제메뉴기본도움말위치에서currentgetter를읽는별도hook/Read필드를추가하되독립fixture의기존값형fallback은유지한다. 이미존재하는modeRead패턴을따른다.|
|`CampaignObjectMenuClient.cpp::OpenProduction`|InventoryAboutMenu의optionalhelpWord콜백을같은cache조회로바꾼다. NULL은원문CRT위치의명시오류,비NULL빈문자열은정상문자열이다.|
|`ClientMetaObjectInput.cpp`|현재Config.Get→EncodeDisplayText를cachegetter로바꾼다. 두helpWord조회/사이assert정상복귀뒤현재재조회는NativeMetaObjectInput본문이유지한다.|
|`UberGump.cpp` 게임메뉴bar.about|직접Config.Get/재인코딩을현재cache로바꾼다. cache문구를Translate("Help")로다시번역하지않는다.|
|`RawMenuControls.h::RawHelpHooks`/`UberGump.cpp` help설치|현재nullablehelpWord표시getter를같은Clientowner에연결한다. 일반text/translate/configlookup과구별한다.|
|`RawMenuControls.cpp::HelpText`/`BaseHelp`|아래특수token61000002를실제현재helpWord로해석한다. 일반Help번역기본값은사용하지않는다.|

### BaseHelp 특수token/고리 경계

원문464990은node+7c의주제에"Help"를붙여전역00565df0을작성→BeginVertical0→현재node+78(없으면+7c)제목→**그뒤현재0054db38** 항목본문→현재root부착→소리다. 현행 `GumpHelp::Handle`은이순서를이미보존하고, `RawMenuControls::BaseHelp`는`GumpHelpState.body=0x61000002`를사용한다. 그러나 `HelpText(61000002)`가현재 `translate("Help")`라Configcache와다른문구를표시한다.

61000002는원문pointer대신Client전역값을읽는semantic token으로유지할수있다. 이token을HelpText에서resolve할때현재cachegetter를호출하면Begin/titlecallback뒤변경도반영된다. cacheNULL이면원문NULL문자열소비위치의명시오류를유지하고영어Help상수로성공시키지않는다.

다음영역은서로의소유문자열/참조가아니다.

- 정적도움말token61000001..5 중2는현재helpWord,1/3/5는themeHelp/F1Help/minimapGump 같은ASCII키다.
- node소유helpStrings는61100000에서증가한다. `SetNodeHelp`가옛pointer맵을지우고새비영token을발급한다.61000002를여기중복등록/반납하지않는다.
- HTML링크는62000000..6fffffff의살아있는body/현재범위를검사한다.
- queuedhelp주제의ScriptStringRing은80칸/매Store새token이며70000000별칭으로TYPE<256관문을피한다. 기존고리token이덮이면명시오류다. cache업데이트로고리칸을소비하거나`_stricmp`같다고고리문자열을deduplicate하지않는다.
- ClientStaticStrings의실제raw고리는별도장기할당/현재position이다. nullableglobalcache의저장소로쓰지않는다.

### 인코딩

한국어준비본 `korean/final-pending/assets/lang/korean/d/config.korean`에는helpWord="정보"가있다. cache는메뉴표시문구이므로표시용 `EncodeDisplayText`를한번만적용한바이트를소유하도록한다. 원문비교는그원문/표시바이트에 `_stricmp`를적용하고case동일이면old표기/할당을유지한다. UTF-8소스문자열을별도보관할경우표시바이트와역할을명시한다. `EncodeOriginalText`에한글허용을추가하거나설정/파일저장경계를넓히지않는다.

cache표시bytes를 `FontBytes`로다시인코딩하지않고,서식용주제/queuedScript문자열은기존byte관례를유지한다. TYPE의실제name/도움말key를helpWord번역문구와바꾸지않는다.512BYTEscratch의NUL폭을넘는명시입력은host물리오류로분류하며조용히자르지않는다.

### 이후검증계획

공통캐시부품의missing/empty/nonNULLempty/caseidentity/changed·한국어표시바이트와BaseHelp의titlecallback후현재값재조회는비MP집중검사로검증할수있다. 실제MP메타메뉴실행은제외한다. 원문C/PE에empty조기반환·free/allocate/store순서와specialtoken현재resolve를대조한뒤필요범위만컴파일한다. 이조사에서검사를실행하지않았다.

## M7.2 — 카메라의 실제 활성 커서

### 이미 있는 부모와 실패 위치

원문497960의같은zoom에서목표pixel이현재와다르면 `446a10(0)`→rootredraw→스크롤/현재두setter→마우스/dirty갱신→`446a40()`→rootredraw→redraw1이다. zoom변경분기는Lock/Unlock을호출하지않는다. `RawCameraCommit.cpp`가이순서를이미복원했다.

446a10은현재005325c8이0일때445aa0(force0),skip0일때cursor1을고른후현재depth를증가한다. callback이depth를바꾸어도마지막증가는현재값에적용한다.446a40은현재depth를DWORD감소하고결과0일때445b30미리보기생성→그뒤현재legalGump값으로cursor3/2선택이다.0에서Unlock하면ffffffff가되고생성하지않는다. clamp/bool/RAII자동짝추가로원문부분효과를바꾸지않는다.

`RawCursorActions`는이본문을이미갖는다. 현재문제는 `UberGumpCamera.cpp::CommitCameraNow`의hook이`world_`만찾고없으면직접depth증감만지원한다는것이다. depth비영이면기본메뉴/초기메타의1→2→1은그대로가능하다. depth0이면실제Activate/미리보기장치가필요하다고명시throw한다.

### 조사에서 드러난 추가 전제 차이

**현재MetaMap Session에는실제RawCampaignCursor가설치되지않는다.**

- `UberMeta.inc::PrepareMetaWorldRuntime`은preparedMetaMap RawCampaignSession생성→SpriteRefresh/FactoryDisplay→ClientMetaPointerInput만설치한다.
- Session생성자의MetaMap분기(1683부근)는기존Fort관리자를다시Install하지않고다른공통모듈/ConnectAi까지생성한다. ConnectCursor호출은없다.
- ConnectCursor의실제caller는일반Battle BeginLocalConnection(3829),Load꼬리(3937),ExistingEditor후반UI(3276)뿐이다.
- MetaMap은이들Load/Editor후반/일반Login을호출하지않고`loaded`도쓰지않는다. 따라서현행`HasCursorRuntimeForState()`는false다.
- `Cursor()`getter는!loaded&&!editorUiInstalling를거부하므로meta허용존재조회만추가해도실제사용은열리지않는다.
- 메타ClientMetaPointerInput의TakeObject/Update/HandleInput도기존Cursor()를호출하며RawCampaignCursor SetEffects가메타에설치된곳은없다.

따라서단순히world_조건을InputSession()/game.RequireSession으로교체하는것만으로M7.2완료로세지않는다. M6후현재소스에서이구조가달라졌으면재대조한다.

### 구현 범위 제안

1. **현재Runtime조회통일:** 카메라hook은매호출`UberGump::InputSession()`또는같은Borrowed ClientGameplayGlobals::RequireSession을조회한다. 둘이동시에존재/다른Session이면호스트owner계약오류다. 임시Session/복사cursorState/다른World를만들지않는다.
2. **메타본체준비:** preparedMetaMap에서기존sameWorld/Pool/Kernel/TYPE/Construction/Motion/placement를재사용하는명시준비API를추가한다. 일반BeginLocalConnection의소규모body준비부분은참고할수있으나`loaded=true`/Login/새Fort설치/Player/SP/난수reset을복사하면안된다. 실제의존연결순서는ConnectBuilding→ConnectMotion→Devastation/CarrierDrop/PriestPrayer→MakeBuilder→ConnectGameplayPlacement→ConnectCursor가현재일반준비의기준이며,메타가필요한하위만원문Caller에맞춰분리한다. 최초메타객체수신/사용자Take이전에한번준비한다.
3. **실제UI장치:** `MakeCampaignCursorClientEffects`의현재Gump/Renderer/커서/측정/마우스장치를같은preparedSession에설치한다. highlight/selection은ClientMetaPointerInput이이미가진실제같은선택기로연결하고Send는같은MetaConnectionRuntime/현재실제전송을사용한다. 성공상수/빈Send로대체하지않는다.초기palette/view는기존ConnectCursor가같은Client장치로설치하므로별도사본을만들지않는다.
4. **Native action진입점:** `CancelCursorForState`와유사하게현재본문존재만검사하는Lock/Unlock전용API를고려한다. `RawCampaignCursor::Lock/Unlock`은현재game.prepare를선행호출하므로원문446a10/446a40직접Caller와다른설정쓰기를선행하지않게상태용leaf를분리한다. 공통사용자Cursor()관문을통째로허용하면미설치입력/저장까지열리므로피한다.
5. **fallback:** 실제Runtime/커서없고depth비영일때만기존원문직접증감경계를유지한다. depth0의필수장치를가짜성공으로지우지않는다. 실제메타커서가준비됐으면world_부재를이유로fallback에가지않는다.
6. **반납:** `RetireWorldForState`는메뉴/Form/Renderercallback·입력·표시기를Session보다먼저반납한다. 새메타커서장치도같은수명에묶고장기Renderer에죽은Sessionrawpointer를남기지않는다. State정리의Cancel은같은cursor와현재depth를사용하며world_만확인하는새중복분기를추가하지않는다.

### 최소 후보 파일

- `UberGumpCamera.cpp`:매효과현재Session조회/기존depthfallback유지.
- `RawCampaignSession.h/.cpp`:preparedMetaMap cursor준비/실제state action진입과loaded관문분리.
- `RawCampaignCursor.h/.cpp`:필요시기존RawCursorActions를직접호출하는NativeLock/Unlock leaf. 계산본문복제없음.
- `UberMeta.inc`:메타Runtime선행준비/실제장치설치순서.
- `ClientMetaPointerInput.h/.cpp`:현재선택/highlight소유자의커서장치노출/주입과기존입력getter도달성.
- `CampaignCursorClientEffects.cpp`/Renderer수집경계는기존장치와currentcursor존재를재대조하며동일계산을다시쓰지않는다.

필요하위의원문caller/VT·현재owner감사뒤변경폭을확정한다. 메타창/MP/패킷/통신실행검사는제외하고C/PE정적대조·Release컴파일만으로코드접합을기록한다. 실제MP동작확인완료로표현하지않는다.
