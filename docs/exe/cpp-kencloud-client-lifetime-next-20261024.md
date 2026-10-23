# Kencloud의 실제 Client·난수·팔레트 접합 잔여

단위385 준비를 위한 **미적용 사본 제안과 정적 조사**다. AGENTS와 공통/닷넷 인계를 다시 읽었고 현재/마지막 정밀 디컴파일 호스트는 HJOW-Athlon /2026-10-23으로 일치한다. 공유 제품·CMake·LEFT·git·공용 build/lib·GUI 및 앞서 동결한 [Kencloud 계산 자료](cpp-kencloud-native-20261024.md)는 변경하거나 실행하지 않았다. 아래 격리 사본 문법/콘솔 검사만 수행했다.

## 결론과 제안 범위

**같은 GameRandom은 Session 생성 이전부터 필요하다.** 최초 메뉴의 구름에서 쓰는00532710도 SP/게임/저장과 같은 전역이다. 현재 `ClientRandomGlobals`는 CRT holdrand만 공유하고 GameRandom은 [Session.cpp326](../../cpppj/src/client/RawCampaignSession.cpp:326)의 값멤버다. 앞서 준비한 설치 getter는 loaded=false Session의 현재 객체만 빌리는 최소안이며 월드 없는 메뉴/Session 교체 수명을 해결하지 못한다.

새 [2파일 소유권 patch](../../extracted/cpp-client-game-random-20261024-proposed.patch), SHA **d181375733140d0c34235ad3b794cf49f21dfc404f416bfaa7e4ba47919510e7**은 기존 ClientRandomGlobals에 실제 GameRandom과 mode를 두고 Session의 기존 모든 소비자가 그 객체를 직접 참조하게 하는 **소유권 단계만** 제안한다. 실제 시작 SP 초기화 이동·추가·생략은 하지 않았다. 이 patch만으로 원문 시작/전투 진입의 난수 소비 순서가 완성됐다고 표시하면 안 된다.

## 원문 시작과 현재 코드의 소비 순서

[정적 PE 기록](../../extracted/cpp-client-game-random-20261024-proposed/static-pe.json)은 게임 실행 없이 mapped image 초기값과 정확한 call 주소를 읽었다. **00532710의 실제 초기값은55(0x37)**이며 BSS0이 아니다. 005c89a8 randomMode는BSS0이다. 독립 Session의 기존 GameRandom default0을 실제 Client의 초기값으로 복제하면 안 된다.

| 원문 위치 | 실제 순서·대응 | 현재 차이 |
|---|---|---|
|WinMain `004391ed`|CreateWindow 성공/GetWindowRect 직후→`0040eba0`. 옵션 읽기·초기 화면·메뉴·Session 상당 기능보다 앞|Client::Run에는 대응이 없다. Session ctor723이 현재 SP.Initialize를 매번 실행한다.|
|SP `0040eba2`|`004559d0`→현재 mode 저장/1쓰기→`00455970`→원래 mode 복원|기존 `RawGameRandomSeed::Reseed()`와 `Initialize()`가 본체다. 같은 CRT DWORD/실제 GameRandom을 받는다.|
|Seed `00455970`|mode0이면고정0x38d535, 그외 실제time32→CRT100회버림→추가3회 곱을00532710에 기록|현재 SP의 reseed lambda→seed.Reseed가 같은 계산을 실행한다. Unix초/CRT를 추측해 별도 시드로 바꾸지 않는다.|
|SP `0040eba9`/`0040ebe2`|GameRandom(100) 한번→offset/추적offset→32회GameRandom(32) 교환→순열을 다시 항등으로→32개0x24 할당/초기화완료표시|`ScrambledSpStore::Initialize()`가 기존 계산 본체다. 재시드 반환값을 다시 SetState해도 같은값 대입이며 난수 호출이 추가되는 것은 아니다.|
|초기 palette `00439426`|fortPal→GamePalSpec→`004a4850(path,1)`|ClientMain.cpp263의 실제 PathSpec 경로는 재사용한다. 현재 초기 Screen호출은 default clear0이며 원문 초기 arg1과 전체 장치 일치를 주장하지 않는다.|
|초기 ColorSort `00439537`|초기 논리RGB005c7954→0043b430의 객체 내부 고정 double RGB복사|현재 계산 함수는 있으나 이 장기 입력 소유·호출 부모는 별도다. 매 화면 palette로 바꾸지 않는다.|
|일반 frame `00436450`→`004994b0`|현재 Renderer가 있으면 메뉴/월드 공통 표시 경로. cloud draw에 Session존재 관문이 없다|현재 UberGump 생성은 CloudFileSpec을 정적 GIF로 읽고 Compose가 Tile만 한다. 실제 생성/난수 소비를 실행하지 않는다.|

현재 `RawCampaignSession::Impl`의 순서는 services 소유→GameRandom 값멤버→SP와HP생성→같은 random/CRT/mode를 빌린 seed생성→ctor에서 추적offset 바인딩→SP.Initialize→후속 설치다. 첫 Session 이전에는 게임난수/SP를 생성·소비하지 않는다. 두 번째 Session도 새 GameRandom0/SP 저장소를 만들고 실제 Reseed/100+32 난수 호출을 반복한다. 원문 WinMain의 전역 SP 최초 초기화 한 번과 같은 순서라고 보지 않는다. 실제 SP Get/Set/Rotate의 lazy 초기화는 원문 초기화완료 전역을 검사하지만 `0040eba0` 자체를 명시 호출하면 다시 초기화한다는 구별도 유지한다.

## 소유권 patch의 안전한 좁은 효과

- 기존 ClientRandomGlobals의 복사/이동 금지·CRT1/Subscriber init0·장기 CRT wrapper는 그대로다. 그 같은 Client수명에 **GameRandom55 한 개**와 GameMode0 한 DWORD를 추가한다. 다른 난수 알고리즘이나 CRT 대체를 만들지 않는다.
- Session은 `localRandom`의 기존0 fallback을 남기고 `GameRandom& random`을 services.randomGlobals의 실제 Game() 또는 그 fallback에 연결한다. mode도 기존0 fallback/같은 owner 참조다. SP·seed·객체·AI·지형·Money 등 현재 `random` 소비자 본문은 바꾸지 않는다. 사후 state복사/동기화가 없다.
- `services`가 참조 소비자보다 먼저 선언되어 shared_ptr를 보유한다. Client/menu/Session 반납 순서에서 현재 Session은 owner보다 먼저 사용을 끝낸다. 새 Session이 owner 참조를 받았다는 이유로 최초값55나0을 다시 쓰지 않는다.
- 이제 Client의 cloud hook은 `client.RandomGlobals()->Game()`을 직접 빌릴 수 있다. 실제 Session의 random도 같은 객체라 설치중 loaded 관문/menu.world_ 조회/죽은 Session 참조 저장이 필요 없다. 앞선 설치 getter 사본의 원문 기록은 수정하지 않으며, 새 장기 소유권을 채택하면 그 getter 없이도 같은 객체를 전달할 수 있다.
- **기존 ctor의 SP.Initialize는 그대로 남는다.** 현재 동작을 유지한 소유권 제안이며 최초 메뉴/Session 교체까지 원문 소비 위치를 바로잡은 최종 patch가 아니다.

### 시작 SP 공유에 반드시 더 필요한 부분

원문 전역 SP 저장소 자체와 초기화완료0054db24,offset00568af8,순열00531890,32×9DWORD물리,추적offset0054d4e0도 같은 Client수명이어야 한다. GameRandom만 공유하고 Session의 SP 초기화를 임의 생략하면 미초기화 SP/lazy 재시드가 나중 위치로 이동한다. 반대로 임시 SP를 Client 시작에 만들었다 버리면 실제 Session SP와 다른 저장소/추가 재시드가 된다. 둘 다 원문 연결이 아니다.

현재 [SpStore::BindTrackingOffset](../../cpppj/src/o/SpStore.cpp:32)는 **초기화 뒤 또는 다른주소 재바인딩을 거부**한다. Session.cpp723은 `priestState.regen.patch.sequence`에 먼저 바인딩한다. 따라서 Client에서 먼저 SP.Initialize한 뒤 새 Session의 값멤버 sequence에 나중 연결하는 방식은 성립하지 않는다.0054d4e0도 처음부터 장기 한 DWORD로 소유하고 SP와 사제 소비자가 같은 실제 주소를 빌리게 해야 한다. 공유 SP/추적offset의 실제 필드 alias 변경,원문 시작 시점의 실제time32/할당 입력,Session생성/재진입에서 명시재시드가 필요한 별도 부모들을 먼저 조립해야 한다. 이 조사에서는 시작 SP 계산을 추측하여 Client::Run에 삽입하지 않았다. 기존 SP의 `fill` 입력을 원문 미초기화 할당과 동일하다고 새로 선언하지도 않는다.

## 실제 Cloud 전역과 파일 부모의 최소 소유자

Client에 별도 Cloud 소유자를 하나 두되 **난수 객체는 소유하지 않는다**. cache/end/nextRow/lastRow/phase/두dirty/이전dirty512/현재dirty512/seed물리/이름버퍼·설정 DWORD를 실제 같은 저장소로 두고 동결 두 모듈의 refs/spans에 함께 빌리는 설계가 최소다. cache0쓰기/free순서는 부모가 소유하며 해제 장치가 대신 current0을 쓰지 않는다. 재귀생성/callback 뒤 재조회에 대응해 이전 할당과 현재 토큰을 구별한다.

| 원문 전역 | 최초 현재값/연결 |
|---|---|
|00569ffc/end00569ff0/next00569ff8/last00569ff4|BSS0, 단일512×512BYTE 할당과 실제 주소 덧셈|
|phase0056b804,dirty0056b7fc/0056b800,두512행표|BSS0. 화면 생성/Session 교체 때 임의 reset하지 않는다.|
|cloudName0056b7a0 / paletteName005c8910|서로 다른 현재 NUL 물리. 의미상255자 제한을 추가하지 않는다.|
|00540a34/00540a38|PE228/245. 화면 RGB 검색값이나ColorSort 포인터가 아니다.|
|00540a7c/80/84/88/8c/90|PE72/2/10/32/99999/400. count는 현재 signed DWORD다.|
|00540a98..|PE의16개 고정 행순서. 단위모듈 fixture에서 보존했다.|

`FortTypeConversionStorage`의 소유/해제 패턴은 참고할 수 있으나 그255BYTE/증가1 토큰은 cache+262144/행 offset 산술에 그대로 쓸 수 없다. `RawAiBrainMemory`도 현재 Bytes가 정확한 base토큰만 받으므로 부분주소 조회를 그대로 제공하지 않는다. Cloud 소유자는 크기가 겹치지 않는 DWORD 가상주소와 실제262144BYTE 블록을 대응하고, **주소부터 남은 span**, 길이0 one-past,해제뒤 거부를 제공해야 한다. host 포인터를DWORD로 잘라 저장하지 않는다.

파일 합성은 기존 `ConfigInterface::PathSpec("CloudFileSpec")`, `BaseFileSystem`, `DecodeGif`를 재사용한다. 실제 존재 검사의 의미/원본 decoder 실패 반환·치수쓰기 계약은 별도로 맞춰야 하며 현재 C++ DecodeGif의 모든 예외를 임의0성공으로 바꾸지 않는다. decoder 성공시 색 번호 전체를 같은 소유자 블록에 보관하고 실제 width/height를 scratch에 쓴 뒤 반환한다. **512치수판정/free는 LoadKencloudFile 본문에 남긴다.** 별도 IndexedImage 사본을 현재 cache인 것처럼 그리면 안 된다. scratch264BYTE의 미작성 초기 입력은 현재 할당 공급자로 명시하고 임의0을 원문 stack값으로 주장하지 않는다.

옵션은 현재 DWORD를 signed bit view로 읽어 누락을 보존하는 `ReadInt`를 사용한다. 원문43442행의 cloudiness/speed/fury/spacer/recenter/cloudAlgo/kenCloud 읽기와 실제 초기값을 구별한다. 현재 `Client::InterpretOptions()`는 이 키들을 아직 읽지 않는다. onChanged는 이미 `InterpretFortFileOptions`를 호출하므로 새 Cloud callback으로 덮어쓰지 않고 기존 호출을 보존하는 좁은 연결이 필요하다. 옵션해석과 palette전환/build를 임의로 합치지 않는다.

## GamePalSpec·Screen·추가표 접합

`TransitionFortPalette`의 필수 경계는 Client의 같은 palette/cloud 이름·cache·현재 cloudSpeed·실제 renderer DWORD·dirty를 빌려 합성한다. buildCloud는 `BuildKencloud(cloud.Globals(),같은 owner.Game(),devices)`, stepCloud는 `StepKencloud`이며 실제 계산이 빠진 성공 callback을 넣지 않는다. pathSpec은 **현재 같은 원본 NUL 입력**을 해석한 ConfigInterface의 반환경로를 안정된 BYTE버퍼로 보관한다. PathSpec 지역 string을 반환한 span이 가리키게 하거나 나중 callback에서 사본으로 입력 주소를 바꾸지 않는다.

loadPalette는 현재 [Client::LoadPaletteFile](../../cpppj/src/client/ClientMain.cpp:614)에 완성된 GamePalSpec 경로와 원문 clear1을 그대로 전달한다. DataDir를 다시 붙이지 않는다. SceneMusic의 이미확장자포함파일용 LoadScenePalette는 DataDir 경로+clear0 별도 경로로 유지한다. 초기 Screen 모드설정 전의 원문 arg1은 현재 bit80없는 Surface.fillBlt 요구경계가 남아 있으므로 이 연결까지 빈 성공으로 확장하지 않는다.

buildMaps는 기존 계산 본체를 재사용하되 [ColorSort 고정초기RGB 수명](cpp-palette-colorsort-owner-20261024.md)을 먼저 연결해야 한다. 원문0043b430은 초기RGB를 객체 내부 double로 복사하며 이후0043c010은 현재그객체를 캡처한다. Client 장기 ColorSort 입력/검색자료와 Kencloud BYTE캐시 및 GameRandom은 다른 소유자다. 현재Screen의 파일마다 재계산되는 ShadeMaps/RefreshPaletteConsumers만으로 이 전체 부모의 성공을 가장하지 않는다. 초기입력과현재팔레트가같을때만의검사는수명차이를드러내지못한다.

## 프레임 표시와 실제 사용자 검증의 남은 경계

원문 Renderer004994b0은 cloud bit8/현재00463fe0 관문 뒤 animating bit0x20이면 **0049956a→00470c50**으로 cache0생성 및 현재speed캡처/step를 진행한다. 현재 rectangle/dirty/시차조건 뒤 **00499c9e/00499cce→004711d0**이 실제같은cache를 blit하며, 마지막 **00499f85→00470ca0**이 이전dirty와현재dirty/전체dirty를 정리한다. 별도004978a7→00471410도 실제 표시경로다. 이 부모들은 이번 동결3함수의 범위가 아니다. Client::Frame에 Step 하나를 무조건 추가하면 자연관문/표시·dirty 순서를 잃는다. UberGump::Compose의 정적 Tile을 그대로 둔 채 생성만 실행한 것도 실제 cloud 표시 완료가 아니다.

후속 순서는 (1) SP/추적offset 포함 공통 시작수명 (2) Cloud 실제 메모리/파일/옵션 소유자와 같은 난수 (3) 기존 palette전환부모의 필수합성 (4) Renderer frame/draw/dirty 원문부모 연결이다. 각 단계의 원문/부분효과 검사를 먼저 고정하고 실제 사용자 검증에서는 월드 없는 첫메뉴→Editor설치중 loaded=false→서로다른palette변경→같은이름자연반환→두Session교체→해상도재생성의 객체주소·난수소비순서·cache/dirty·화면변화를 확인해야 한다. 실제 원본 파일 보호와 공용 게임 GUI 실행은 root의 작업단계에서만 수행한다.

## 이번 사본의 검증 기록

[신규 사본 디렉토리](../../extracted/cpp-client-game-random-20261024-proposed)는 당시2기준원시파일/2제안사본/patch/정적PE보고서/생성도구/검사2개/독립Runner/빌드로그를 보존한다. 사본 Session `/Zs`는 종료0·관측경고0이다. 첫 직접 링크는 기존CrtRandom.cpp 누락으로 LNK2019 2개/LNK1120를 보존했고, 그 읽기전용 의존성만 추가한 **build2 종료0·관측경고오류0**, **2검사0실패**다. 기존공용 lib는 사용하지 않았다.

검사 범위는 실제PE 최초 GameRandom55와CRT1/모드0분리·고정 첫전진값, 두사용자의같은 객체주소/수명, 먼저고정한 Kencloud v2 첫입력의7회소비·최종state2488585150/spacer89가 실제장기owner자체에남는지다. 실제 Client/두Session을 실행한 검사가 아니며 SP시작위치/전체난수열동일성 검증도 아니다. 동료 읽기검토에서 services/NSDMI/owner참조수명의명백오류는찾지못했다. 최종 SHA/UTF8/AST/JSON/링크·동결Kencloud불변은 [소유권 제안 verification](../../extracted/cpp-client-game-random-20261024-proposed/verification.json)에 기록한다.
