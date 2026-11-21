# LEFT_JOBS.dotnetpj — 현행 상태와 남은 작업

**2026-11-21 · 사용자 재개 지시로 진행 중 · 마지막 완료0.783(장기설정/버전응답) · 다음0.784 전체회귀 · 현재 단계 R4(R4-2·R4-4·R4-5·R4-6 병행) · cpppj 제품1323 / 11.01 · 전체 목표 미완료**

현행 인계는 이 문서가 기준이다. 단위별 근거는 `docs/dotnet-canon-0NNN-*.md`이다. 0.645~0.732의 단위별 완료 문장, 0.640 회귀 점검 상세, 0.546~0.676 완료 기록 원문은 [0.732 정리 전 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)에 있다. 0.600~0.635는 [0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635), 0.613까지의 경과와 파일 계열별 체크 목록은 [0.613 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에 있다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), 웹은 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 따른다.

<a id="dotnet-overview"></a>

## 0. 현재 상태와 다음 작업

| 경로 | 실행·플레이 | 원본과의 동일성 |
| --- | --- | --- |
| 기존 배포 `Netstorm.Game` (`Netstorm.exe`) | 메인 메뉴와 캠페인 1-1·1-2를 플레이할 수 있다. 한국어 선택, 와이드 시야 확장, 테두리 없는 전체화면을 제공한다. 실행 오류만 수정하는 동결 상태다. | RGBA 합성·자체 UI·근사 `BattleSession`으로 동작한다. |
| 새 경로 `Netstorm.Client` + `Netstorm.Client.Host` | 타이틀·메뉴·도움말·Options·Credits·캠페인 선택·Edit 목록과 이름 창이 동작한다. 개발용 `--mission <키> --handle <이름>`으로 일반 31미션의 시작 상태 일부를 조립한다. 월드 표시·입력·규칙의 실제 경로가 남아 있어 플레이 불가다. | 1024×768 메뉴 **12장면**의 논리 BMP가 cpppj와 바이트 일치했다(0.381~0.388). L2는 **0/34**, 데모 3경로는 DN 진입 미연결이며 L3·L4b·L5는 미착수다. |

- **검사 기준선.** 마지막 전체 회귀는0.780이다. Assets366·Core3,368·Client1,741 합계5,475건 통과·실패0이며 기존 실장치 음향1건은 미실행이다. 기본 검사는 `NETSTORM_DATA` 없이 실행한다. 4단위 주기의 다음 정기 전체 회귀는0.784다. [0.780](docs/dotnet-canon-0780-20261121.md) · [회귀 점검](#regression-audit-0640)
- **L2 기준선.** 마지막65시작 일괄 수집은0.780의 `extracted/dotnet-l2-0780/`이다. 일반31경로 양쪽·CPP데모3경로65실행 모두 종료0·원본1,392파일과 빌드6파일 불변이다. DN31경로 전체 덤프 바이트와34경로 지표가0.776과 같고0.779 최종 대표3경로 바이트도 같다. M1은 **0/34**(cpp_only997·dn_only21은0.712부터 같음)이고 DN데모 진입은 미연결이다. 농지 대표3경로721셀은 프레임497·소유자68셀이 일치한다. [0.780](docs/dotnet-canon-0780-20261121.md) · [격차 분석](docs/dotnet-l2-0606-20261117.md)

<a id="now"></a><a id="09-바로-다음-작업"></a><a id="r4-board"></a>

### 0.1 재개 순서와 최근 완료 요약

**재개 순서(2026-11-21).** 0.783에서 같은 장기 GameAux 설정/0ch 생성 전역·네 미션 observer·32h 고정 F·0bh 실제 버전 응답을 연결했다. 신규4건 포함 집중39건 첫 통과·실제 내부Source의11BYTE 응답/현재 헤더 재조회·대표3시작 종료0/0.780 전체바이트 일치·원본1392파일 불변·기존396fixture 유지다. [0.783 근거](docs/dotnet-canon-0783-20261121.md)

1. **0.784 정기 전체 회귀.** 0.781~0.783의 장기 패킷 기록·기본18행/9종 observer·장기 설정/버전 응답 뒤 Release 전체와 동일 빌드65시작을 확인한다. 도구는 extracted/dotnet-0784/에 준비했고 아직 미실행이다. 제품/fixture를 고정해 전체 덤프·지표·원본1392파일·호스트6파일을0.780에 대조한다.
2. **0.785 전체 공통 등록 부모.** 기본18행/C3·9종 mutable 관찰기는0.782,네 미션 observer(2a/2b/67/81)·32h 실제7DWORD 고정 F·0bh 버전 응답/0ch 생성 전역은0.783 완료다. 나머지 순수 observer/장기 수신과 전체 InstallClientPacketDefaults/MakeClientCampaignTransport 부모·Root의 실제 Source/Record/Sent·송신/제출을 잇는다. 근거는 cpppj/src/client/ClientCampaignTransport.cpp다. 장기 파일/요청/프레임 소유자(0.781)는 완료됐으나 실제 전송 Record/Sent 소비자는 미연결이다. 전체435220 옵션 부모에서 Requested만0으로 재설정하는 호출도 남았고 FILE/enabled를 닫는 동작으로 대신하지 않는다. NativeLoginPacketReceivers API도 실제 MonoGame Root 설치 호출은 없다. NativeGameplay/Carrier/Construction·Meta 등 전체 고정 F 설치는 실제 현재 본문/Runtime 결선과 함께 진행한다. HostSession 실제 송신 부모는 미연결이며 구 Mission.RawCampaignSession의 sendChatMutable 입구로 대신하지 않는다. UI 수신 소비자는0.779에서 실제 Root에 설치했지만 공통41h가 Root.ReceiveNativePacketChat을 부르는 전송 설치는 남았다. Native Root/HUD 창 생성·Enter와 알림 등록 ID의 실제 HUD 작성자도 남았다. 뉴스의 실제 HTML/서버별 설정 저장 부모는 연결했으나 NativeMapView 공급자가 아직 없어 해당 분기는 명시 실패한다. Native 색표7 getter/zone 옵션 변경 부모와 세션 자산 InBattle 최초 상태도 원문과 맞춘다. 이어 Native Frame/Root UI 수집·그리기와 실제 지도 provider, 전체 커서 효과/Drop/Previews/Position 호출자, SavedCameraKey/무효점·CameraCommit/Present/Pan을 연결한다. 실제 MapViewRect가 없는 HostMainMenu는 뷰 소비에서 명시 실패한다. 전체 Native 시작의 intro/장면음악/나머지 지도 초기화와 Guide Init, 나머지 음향/저장/옵션/전체 EnergySupply, non-mine Notify의 실제 DataManager/추출 부모, NSInfo HUD/메타UI/Root reply 저장도 후속이다. RawFileName 나머지 조회·BaseFile 고리·ClientStaticStrings는 해당 소비자에서 잇는다.
3. **같은 작업 줄기.** MetaZone 원문 본체124입력은0.734, 실제Client표/현재Runtime pin·설치된가상분배전달은0.735 완료다. 현재 HostSession의 전체 Native World 자산 가상 삭제 분배 설치는 아직 미연결이며 일반 삭제의 Pre/Post 완료로 대신하지 않는다. [0.733](docs/dotnet-canon-0733-20261121.md) · [0.734](docs/dotnet-canon-0734-20261121.md) · [0.735](docs/dotnet-canon-0735-20261121.md). 이어서 [R4 체크](#r4-detail)→R11과 후순위 요구사항을 진행한다. 다음 정기 전체 회귀는0.784다.
4. **사용자 제외 검사 유지.** 실제 포획/희생·특정 적 공격·시나리오 승리는 재현하지 않고 사용자 수동 확인으로 남긴다. 원본1,392파일의 마지막 불변 확인은0.783 대표3시작이며 전체65시작은0.780이다.

**최근 완료 요약(0.639~0.783).** 단위별 완료 문장과 집중 검사 수는 보관본과 각 단위 문서에 있다. 완료 범위와 남은 일의 기준은 1.1·1.2절 체크 항목이다.

| 주제 | 단위 | 핵심 | 남은 일 |
| --- | --- | --- | --- |
| AI 런타임 | 0.639~0.641(0.641 부분)·0.645~0.650·0.653~0.658·0.661~0.663·0.665 | AI 생성부터 준비·전략·전체 프레임·통지·월드 등록·경제·시설 생산까지 본체 이식과 세션 결선 | [R4-5](#r4-detail) |
| 정기 처리 | 0.642·0.643·0.651·0.659 | 장식·Player·스프라이트·매몰 정기 본체와 세션 분배 | R4-4 |
| 업그레이드·연구·생산 Deck | 0.666·0.667·0.682·0.683·0.685~0.687 | 업그레이드 요청/완료, 연구·생산 Deck·기술 표시의 실제 세션 연결, 58h 분배·연구 운반 | R4-5, [1.2](#r6-preports) |
| 플레어·건설 | 0.669~0.671·0.673~0.675·0.677~0.679·0.681 | 플레어·건설 진행/표시 본체와 제품 장치·음향 | 1.2 |
| 로그인·전송·Client 전역 | 0.689~0.691·0.693~0.695·0.697~0.699·0.701~0.703·0.705·0.707·0.715 | 패킷·연결표·Poll·로그인/재접속·옵션·상태 전이·Clock과 장기 Client 전역 | R4-6 |
| Runtime·Player 장기 수명 | 0.706·0.709~0.711·0.713·0.714·0.717~0.719 | 실제 Runtime 대여/반납, 장기 Player·Core·옵션 전역, 현재 Runtime pin | R4-6 |
| Client 프레임·장기 Host | 0.721~0.723·0.725~0.727 | Clock/Frame 공유, 프레임 실행 순서, 장기 Surface·Host Runtime | R4-6 |
| Native State 정리·저장소·반납 | 0.729~0.731·0.733~0.735 | State 정리 부모(원문294행), 실제 전역·저장소·장기SID, 실제 자식 증빙/Runtime 반납·제품 Surface 대여, MetaZone 본체/실제Client표·Runtime삭제경계 | 위2번, R4-6 |
| Root 자원·장기 자산표·선택 | 0.737~0.739·0.741~0.743·0.745~0.747 | NSInfo/서버 목록·장면 음악·Template/State 공유·State 효과분배/Root경계·첫메뉴 자산표·선택/강조/실제Clear/Root커서 | 나머지 기본 장치와 월드 표시 설치, R4-6 |
| 커서 데이터·수명·위치·실제 진단 | 0.749~0.751 | 고정 배열/데이터·Take/Cancel/Activate·Query/SourceAllowed/Prepare·실제 자산/Canon/SHP·2148원본입력·실제 Gameplay/Session/Surface 현재 checking 공유 | 실제 Camera/렌더러와 Root Cancel·Drop/호출자 |
| 카메라·실제 snapshot | 0.753 |16DWORD/목표/분모·실제 Gameplay/커서/내용물/연소 뷰 공유·156관찰·진단 뷰 Session 수명 교정 | Renderer 초기화/그리기/표면 장치·Root Cancel·CameraCommit/입력 |
| Renderer 소유자·그리기·출력 | 0.754 | 장면/캐시/dirty·Native submit 재조회·선행 출력요청 소비·288원문출력·빈 Cancel 실제 Prepare 검증 | 실제 Host/Native Frame/표면 장치·Root Cancel 연결 |
| 실제 Host 표시/커서 장치 | 0.755 | Renderer 공개/Camera 초기화·구름 token/단일 redraw·현재 Surface getter·Root 실제 Cancel | Native Frame/Root UI/지도 provider·전체 커서 효과/입력 |
| 실제 Chat/Subscriber | 0.757 | PE 두DWORD/이름64BYTE·Chat4800BYTE·World/Session 공유·현재 getter·Root ResetOccupied | manager/명령표/Init/전체 송수신·창/UI |
| Chat manager 본체/할당 | 0.758 | 생성/설정/랭크644입력·실제 current/미초기화 할당/서식 캡처·Host 소유자 | 명령 등록/실제 시작·Core/TYPE 랭크 호출자·전체 제출/창 |
| 실제 명령표/Chat 시작 | 0.759 | 등록258지원/음수외부4·Issue122·최초 say 전체표·실제 Host 초기화 | 실제 Dispatch/제출·창/UI·전체 Native 시작 |
| 점 명령 분배/장기 검색 | 0.761 | 고정 문법512입력·같은 표/검색 owner.Dispatch·중첩 separator/현재 이름 뷰 | Chat 제출/창/UI·실제 명령 효과 |
| Chat 제출/수신자 배열 | 0.762 | 대상/본문 인접4097BYTE·383입력·실제 owner 확장/반납·100수신자 | 실제 제출/형식 부모·일반 전달/창/UI |
| 실제 형식/제출 분배 부모 | 0.763 | 두상자/캡처/UTF8 설정·Issue/메뉴/say/오류 경계·문화권 키 교정·Host API | 실제 Identity/메시지/창UI·나머지Root장치 |
| 실제 Identity/대상 검사 | 0.765 | 현재Player 배열/이름/주소/표면·350+48관측·같은Host 대상/제출 binding | 일반 메시지/패킷·CS·창UI 소비자 |
| 실제 일반 메시지/패킷 | 0.766 | 같은두상자/이름/할당·524+256관측·count/문화권 차이 교정·Host flow | 창UI 소비자·현재 전송/수신 설치 |
| ChatWindow 장기 상태/편집 | 0.767 | 공유상태/로그·189관측·실제Host TextEntry 대상·완성·뉴스 설정 | 전체창/공장·전송/수신·뉴스 호출/프레임토글 |
| 실제 Chat 기하/프레임 요청 | 0.769 | 같은네자식/3960관측·실제Tree/로그·프레임말 전환소비 | 전체창/가상분배·Enter Root입력 |
| ChatWindow 전체 부모 | 0.770 | 같은실제Console/버퍼·생성/입력/helper/수명·180관측/166부분집합/기존2292유지 | 실제 UI 공장/소유권·자식분배/Enter·송수신 |
| 공통 UI 소유권/문자열 | 0.771 | 같은Tree/실제Console 하위 owner·61100000 소유 문자열·548758 반납 | 전체Chat 공장/가상분배·Root입력/송수신 |
| 실제 스타일 그림 자산/출력 | 0.773 | 최초TYPE188/자산116·같은본문/메뉴·실제8비트 그림 출력 | Native색표7/옵션변경 부모·전체Chat 공장 |
| 전체 실제 Chat 공장/가상 분배 | 0.774 | 같은Console/공유로그/그림/메뉴·Host 설치·자식 부착/입력/수명 | NativeRoot 창생성/입력·제출/전송/수신 |
| Chat 중계/고정등록·관찰 | 0.775 | 원문132·같은mutable패킷/마스크·42dc90 F/observer·현재재조회 | 실제Client41h/연결·UI수신·Root입력/제출 |
| 장기 Client41h 수신 바인딩 | 0.777 | 같은현재Client/Registry/Player/마스크·실제내부Source 전달 | MonoGame Root의 Dispatcher/수신·입력/제출/송신 부모 |
| 정기 전체 회귀·L2 | 0.644부터 4단위마다(0.780까지) | 매회 실패 0·65실행 종료 0·원본 불변·M1 0/34 유지. 수집으로 드러난 미연결·순서 회귀를 0.644(AI 색 프레임·펜스)·0.680(플레어 분배)·0.684(시작 음향 순서)·0.688(Carrier 통지)에서 교정. 전체 검사 4,686건(0.644) → 5,475건(0.780) | 다음0.784 |

<a id="first-run"></a>

### 0.2 최초 실행 상태 — DN 적용 대기

원본 관찰과 cpppj 1314 구현이 완료됐다. DN은 빈 사용자 루트·무옵션 데이터 사본에서 아래 세 항목을 메뉴/설정 결선과 함께 확인한다. [원본 관찰](docs/exe/original-first-run-20261116.md) · [cpppj 1314](docs/exe/cpp-first-run-1314-20261116.md)

- [ ] Welcome(OK) → Welcome2(Play Early Missions / Main Menu) → 메뉴 버튼. Welcome2가 열릴 때 `welcomeGiven=1`을 사용자 루트에 저장하고 재실행 시 Tips 분기를 확인한다. `HostStartupScene.SaveOptions`의 설치 데이터 쓰기도 사용자 루트로 이동한다.
- [ ] Struggle For Freedom 최초 상태는 1번만 선택 가능하고 1~6번 모두 체크가 없다. 자기 `Done*`는 체크, 직전 `Done*`는 해금에 쓰고 저장·재실행을 확인한다. 실제 미션 결과 연결은 R6/R7에서 검증한다.
- [ ] 최초 Options는 소리·음악·바람·가장자리 스크롤·Auto-Demo·팁 ON, 스피커 교환 OFF, 효과음·음악 각각 5, 800×600·전체화면 요청이다. 기존 사용자 값과 진행을 보존하고 전체화면 재실행을 확인한다. 메뉴 일치 시험의 1024×768 창 모드는 시험 조건이다.

<a id="korean-font-change"></a>

### 0.3 한국어 글꼴 — DN 적용 대기

AGENTS.md의 현행 지정은 갈무리11이다. cpppj 1322의 외부 비트맵 출력·배포가 완료됐고, DN 배포판은 아직 D2Coding 1.3.2를 사용한다(2026-11-21 `package-dotnetpj.ps1` 확인). 새 호스트의 한국어 화면 연결은 R8 6차다. [공통 기준](LEFT_JOBS.md#korean-font-change) · [cpppj 1322](docs/exe/cpp-korean-galmuri-1322-20261117.md)

- [ ] `KoreanFontFamily` 공통 상수와 글꼴 파일·라이선스 설정을 마련해 로딩·측정·그리기·한글 대체·개발 표시가 같이 사용하도록 한다.
- [ ] 기존 배포의 한국어 출력과 새 경로의 8비트 출력에 `Galmuri11Bitmap-Regular-2.40.4.ttf`·`Galmuri11Bitmap-Bold-2.40.4.ttf`(family `Galmuri11Bitmap`)의 비트맵 글리프를 적용한다. D2Coding용 TTC face 선택은 전환 대상이며, TTC 판독기 검사는 실제 TTC로 유지한다.
- [ ] `Netstorm.Game.csproj`와 `package-dotnetpj.ps1`의 복사·필수 파일·고지·안내를 맞춘다. 두 TTF와 `Galmuri11-LICENSE.txt`를 exe 옆 `fonts/`에 두고, R5 배포 전환 시 설정을 승계한다.
- [ ] 작은 메뉴·도움말·브리핑과 영한 혼합 제목에서 일반/굵은 획·폭·기준선·줄 높이·잘림을 확인한다. 영어 `.chfnt`와 일본어 Noto Sans JP를 함께 대조한다.

<a id="exe-icon"></a>

### 0.4 실행 파일 아이콘 — DN 적용 대기

AGENTS.md 2026-11-19 추가(`d047bf9a3`)로 `Netstorm.exe`의 아이콘은 기존 게임 실행 파일 아이콘과 같아야 한다. 원본은 `originals/Netstorm.exe`의 아이콘 그룹 102(32×32·4bpp)와 144(16×16·4bpp)이며 탐색기 파일 아이콘은 102다. cpppj 1323이 두 그룹을 바이트 그대로 담은 공통 [assets/Netstorm.ico](assets/Netstorm.ico)(1,078바이트, SHA-256 `bd4c18c5…f01e4f068`)를 만들었고, webpj `favicon.ico`도 같은 파일이다. `Netstorm.Game.csproj`에는 아직 `ApplicationIcon`·아이콘 리소스가 없다(2026-11-21 확인). 빌드의 `.exe` 혼입 검사 때문에 원본 실행 파일을 빌드 중에 읽지 않고 공통 ICO를 쓴다. [cpppj 1323](docs/exe/cpp-exe-icon-1323-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

- [ ] 공통 `assets/Netstorm.ico`를 재사용한다(다시 추출하지 않는다). 사용한 파일의 해시를 단위 문서에 기록한다.
- [ ] 현재 `Netstorm.exe`를 만드는 `Netstorm.Game.csproj`에 `ApplicationIcon`을 지정한다. DesktopGL 창·작업 표시줄 아이콘에 내장 리소스가 따로 필요한지는 사용하는 MonoGame 버전에서 확인한다. R5에서 새 경로가 `Netstorm.exe`를 이어받을 때 같은 설정을 승계한다.
- [ ] 빌드된 exe의 아이콘 리소스를 원본 바이트와 대조하고, `package-dotnetpj.ps1` 산출 ZIP의 `Netstorm.exe`에도 적용됐는지 확인한다. 탐색기 표시는 사용자 확인으로 둔다.

<a id="dotnet-stage-board"></a><a id="stage-criteria"></a>

## 1. 단계별 작업과 완료 기준

| 단계 | 현행 상태와 완료 기준 |
| --- | --- |
| R0 기준선·동기화 | 완료. cpppj 기준선·PORT_MAP·`cpp_sync.py`. |
| R1 화면 호스트·입력 큐 | 완료. 8비트 호스트·입력 큐·비교 도구. 실제 월드 입력 소비는 R6. |
| R2 그리기·팔레트·글꼴 | 원시 본체 완료. 실자산 어댑터·수집/정렬·종류별 분배·`SpriteDraw`/그림자 공유와 `owner-capture` C행은 R4/R5에서 연결. |
| R3 Gump UI·메뉴 → D1 | 메뉴 L4a 12장면 일치. 메뉴 상위 소유자·모달·등록·대화창·텍스트 입력과 하위 화면 연결 잔여. 미룬 장면은 Language 자식 CJK(R8), Multiplayer 섬(R5/R11), Edit 이동행(R7). [UI 잔여](#r3-r2-detail) |
| **R4 월드 세션 조립 → M1** | **R4-2·R4-4·R4-5·R4-6 병행 진행 중**(0.689 이후는 주로 R4-6). 34경로(Early 6·Training 9·캠페인 16·데모 3)의 같은 시드 Load/Start `session-dump`가 cpppj와 일치해야 한다(L2). [R4 체크](#r4-detail) |
| R5 월드 화면·카메라·HUD → D2 | 렌더러 수집·정렬·dirty/팔레트, 타입별 Draw와 효과, 카메라·전체화면 가장자리 스크롤, HUD·덱·미니맵·인벤토리. 진입 직후와 N프레임 뒤 34경로 L4a 차이 0 및 압축 해제 실행 후 `Netstorm.exe`와 배포 대상을 새 호스트로 넘긴다. |
| R6 플레이·규칙·AI·음향 → M2/M3 | Tutorial1 → Early Missions → 1-1·1-2 순서로 입력·커서·선택·메뉴, 사제/경로/운반/수확, 건설/생산/경제/다리, 전투, 의식/승패, AI, 소리/음악/동영상, 채팅/경고를 연결한다. M2는 고정 입력열 L3와 대표 캡처 일치, M3는 1-1·1-2가 기존 경로 수준 이상으로 진행되는 것. M3 뒤 `NetstormLegacy`와 근사 검사를 제거한다. [선행 완료와 잔여](#r6-preports) |
| R7 미션 전체·Edit·설정·배포 → M4 | 34경로 진입·진행·저장, Retry/Next/Abort, 데모 2주기, Edit 왕복 저장·Save as·Test Battle/복귀, `options.cfg` 시점·전체화면 재실행, A~F 중 C·D·F 판정. ZIP 전수 대조와 압축 해제 실행, `Netstorm.exe` 외 실행 파일 사용법 문서화. 배포본 `Netstorm.exe`의 아이콘이 원본 실행 파일 아이콘과 같은지 확인한다(AGENTS 2026-11-19 추가, [0.4](#exe-icon)). |
| R8 AGENTS 4~15차 | 아래 차수별 요구에 도달할 때마다 기능을 검증한다. 공통 파일을 옮기는 과정에서 이미 들어온 부분도 실제 경로에서 확인한다. [요구사항](#dotnet-r8-requirements) |
| R9 60/120 FPS | Frame Rate 60/75(원본)/120과 양보 대기의 프레임 시간 대조. 비행체 표시 보간은 cpppj D2 결정에 따른다. |
| R10 Linux | DesktopGL와 파일 장치의 경로·권한·원자 저장을 확인한다. `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`를 사용한다. |
| R11 로컬 멀티플레이·Outpost | TCP/IP 로컬만 지원한다. `Netstorm.Net` 일부 사전 구현. 루트 서버 독립 모듈은 로컬 실행에 필요한 범위를 먼저 판정하고 실동작은 사용자 확인으로 둔다. |

<a id="r4-detail"></a>

### 1.1 R4 세션 조립 체크 (1/8 완료)

단위별 완료 문장은 [0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)·[0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)과 각 단위 문서에 있다. 아래에는 하위 단계별 완료 범위와 남은 일만 둔다. 통합 조각을 마칠 때 같은 조건의 일괄 덤프로 M1 진척을 확인한다.

- [x] **R4-1 객체 수명 공통층(0.546~0.600).** 미션에서 `HostSession` 별도 풀을 제거했다. 풀·공장·공간·건물 저장소 공유는 live 슬롯 바이트 대조로 확인했다. Dais·Player·Aviary 개별 몸체, 장기 Unpop·Pop·Display·Graph·Host 실행기, 실제 전송·파생 Runtime pin은 R6 장치 결선에서 잇는다. [마감 감사](docs/dotnet-r4-1-close-20261117.md)
- [ ] **R4-2 적재 Pop 전체 상태 일치.** cpp 근거는 `RawNormalTerrainWorldBinding`·`RawRegionPaint`·`RawRegionOwnership`·`Islandbuilder`·`SquidPostPop`이다.
  - 완료(섬·지형): 3×3 스탬프와 섬 `Prefix` 발화, 적재 깊이(0.601~0.604), 그래프 per-Pop 결선(0.605). [섬 후처리](docs/dotnet-session-island-postpop-20261117.md) · [그래프](docs/dotnet-session-terrain-graph-20261117.md)
  - 완료(요새·농지): 지원 타입 요새 일반 Pop(0.607), 농지 어댑터와 Generate 실결선(0.612~0.614)을 마쳤다. 대표 3경로의 농지 선택 집합·타입·소유자가 100% 일치했다. [요새 Pop](docs/dotnet-fort-general-pop-20261118.md) · [Generate](docs/dotnet-canon-0614-20261118.md)
  - 완료(지역·신전): 지역 칠하기·소유권 본체와 세션 연결(0.618~0.619), 신전 가상 Owner 본체와 결선(0.620~0.621), 사원 Pop 파생(0.622). [0.619](docs/dotnet-canon-0619-20261119.md) · [0.622](docs/dotnet-canon-0622-20261119.md)
  - 완료(공장): 후처리 본체(0.624), 정기 트리거 세션 연결과 발화 확인(0.625~0.626), Fort 경로 L2 대조(0.627), 정기 본체(0.628), 정기 분배(0.629), 초기 레벨 적재(0.630). [0.630](docs/dotnet-canon-0630-20261119.md)
  - 완료(장식): 장식 정기 본체와 세션 분배 결선(0.642). 사건 0의 단계별 HP 감소·개수 3 서버 삭제·나머지 공통 분배가 원문 순서와 같다. 가상98 전체 몸체·world.Destroy 송신은 R6에서 잇는다. [0.642](docs/dotnet-canon-0642-20261119.md)
  - 남음(일치 대조): 농지 프레임/소유자를 맞춘다(0.728 프레임 일치 thewarbegins 107/152·tutorial1 56/69·surrounded 334/500, 소유자 일치는 합계 68/721). 그래프는 surrounded 255/255지만 thewarbegins 253/255 등 일반 31경로 중 26경로에 차이가 남는다. 표시 등록·스탬프·장부 `totals`/`totalmade`도 덤프와 대조한다.
  - 남음(요새·공장): 비용이 있는 전용 요새 8종(71·75·78·89·122·123·124·158)은 R6 파생 본체에 의존하고, 지원표 밖 처리도 남았다. 공장 뒤 4건의 sid 재사용 순서와 업그레이드 그림 의존 고리도 맞춘다.
- [ ] **R4-3 신전·소용돌이 → Player 첫 지역.**
  - 완료: 신전 Pop → 소용돌이 → 섬 소유자 → Player `+0x78` 경로값이 일치했다(0.622, `thewarbegins`·`tutorial1`). `surrounded` 혼합 경로에서도 첫 지역 9행이 일치했다(0.623).
  - 남음: AI 뇌 경로(`bc1menu76` 사원 없는 6행)는 R4-5 뒤에 닫는다. 농지 가장자리 432칸은 R6·R11 투표 입력이다. 거주 구역 5칸 전파는 0.623 기준이며, 0.635 거주 구역 연결 뒤 `surrounded`는 다시 재지 않았다.
- [ ] **R4-4 정기 처리·프로세스 초기 등록.** 입력은 `RawAssetRegular`과 Player·Dais·Dude·Disc·장식 정기 처리다.
  - 완료(틱·정기 본체): 미션 틱 `AdvanceMissionFrame`(0.629), 고정 시각 1프레임 1틱과 덤프 시계(0.631), 매 프레임 `UpdateAmbientWind`(0.632)·주변음(0.633), 간헐천(0.634)·거주 구역(0.635)·펜스(0.636) Pop과 정기, 장식(0.642)·Player(0.643)·스프라이트(0.651)·매몰(0.659) 정기 본체와 세션 분배. [0.631 격차 분해](docs/dotnet-canon-0631-20261119.md)
  - 완료(펜스 생성·삭제): 표식·포스 필드 생성 본체 연결과 연결선·표식 삭제의 전체 파괴 결선(0.644). [0.644](docs/dotnet-canon-0644-20261119.md)
  - 남음: Kernel/Process 초기 등록과 초기 덤프 비교, Dais·Dude·Disc 정기, AI 전체 행동의 실제 매 프레임 호출·장식 재생·연소 등 나머지 틱 난수원을 잇는다. 정기 부착 시점(0.631에서 분해한 RNG 25,611 격차의 틱 구간)도 맞춘다.
  - 다른 단계로 넘긴 것: 거주 구역 사건 2(주민 생성), 펜스 이웃 새로고침(이벤트 1), 장식 가상98 전체 몸체·삭제 송신, Player 화면 로그·DataManager 통지는 R6이다. 펜스 표식·연결선 삭제의 송신·통지 일치는 R4-6에서 대조한다.
- [ ] **R4-5 AI 초기화.**
  - 완료(생성·본체): 생성·초기 갱신·해제·동맹 난이도(0.639~0.641), 준비·전략·전체 프레임·통지·월드 등록 본체와 장치 조립·세션 결선(0.645~0.650), 지역 경계·분산 SP·자금·지도 상한(0.653~0.654), 그래프 조회·경제 본체·신전 생산 TYPE(0.657~0.658), 경제 세션 연결(0.662). [최근 경제 결선](docs/dotnet-canon-0662-20261120.md)
  - 완료(하위 행동 본체): 수집자(0.655)·배치 선행(0.661)·시설 생산(0.663, 세션 0.665) 본체, 업그레이드 요청·수신·사건3 완료(0.666~0.667), 실제 업그레이드/연구·생산 Deck·내용물 통지의 세션 연결(0.682~0.686), 공유 분배기·58h 등록·연구 운반(0.687).
  - 남음: 제품 양끝 요청 전송, 수집자·시설과 나머지 하위 행동의 세션 호출자, 신전 삭제, `SetAiParameter`·`CrankAiMoney`의 실제 호출자, 매 프레임 실행과 258 완료·brain·SP·Player L2 대조.
- [ ] **R4-6 내부 로그인·전송·Client 전역.** 메뉴 핸들 출처와 이름 꼬리 16~19바이트, 로컬 Login→Ready→Start 실제 패킷, 요새 이미지·`Bind*`·State 수명을 연결한다. 실제 handshake 검사와 메뉴/`--mission`의 진입 덤프를 대조한다.
  - 완료(패킷·로그인 본체, 0.689~0.703): 패킷 복제/소유, 실제 연결표와 Source pin, 로컬 양끝/현재 ID Poll·Zone 별칭, 서버 Player/metadata, 옵션 전체 부모, 클라이언트 8종 수신/등록·Ready writer, 전체 로그인/재접속/현재 집계, 전체 타이머/진행 문구, 옵션 파생/설정 패킷, 로컬 상태 전이/확정, 내부 Source와 단일 ID 순회, Clock 동기화/Reset.
  - 완료(장기 소유자, 0.705~0.719): 장기 Client 전역·연결표와 기존 저장 모드·주변음·플레어 기한 공유(0.705), 실제 Runtime 준비 대여/공개/배송 pin(0.706), 52h·64h 선행 부모(0.707), 장기 Player 할당·공장 목록·옵션/Core/Player 전역(0.709~0.711), 호출 대여 명시 반납(0.713), Player/동맹 고정뷰(0.714), 고정 App 수신기·Session 엔트리 분배(0.715), 후처리·Process·공장의 현재 Runtime pin(0.717~0.719).
  - 완료(Client 프레임·State, 0.721~0.731): 메뉴 전 Clock/Player 초기화와 같은 Frame·게임 시각(0.721~0.722), 프레임 실행 순서 복원(0.723), 장기 Surface(0.725), 장기 Host Runtime·TYPE 물리 배열·weak 소유(0.726~0.727), Native State 정리 부모(0.729), 실제 State 전역·장기 월드(0.730), State 저장소 자식·장기 SID(0.731), 실제 자식 증빙·Core0 뒤 Runtime 반납과 제품 Surface 준비 대여(0.733), MetaZone 원문 부모·마커124입력(0.734), 실제Client표/현재Runtime pin·설치된가상삭제분배전달(0.735), NSInfo/서버 바이트 판독과 실제 호스트 소유자·옵션3값(0.737).
  - 남음: MetaZone의전체NativeWorld자산가상삭제분배설치(원문본체/Client표/pin경계는0.734~0.735완료), 실제Root의NSInfo HUD/장면음악장치·UI/음향/Pool·전체State설치(서버목록0.738/장기자원0.739/Template·Session공유0.741완료,Root reply/UI저장호출은후속), 전체 로그인/제품 Root 등록과 제품 Clock 프레임 설치, World/건설 효과 pin 전달, Source의 Editor/LAN/Meta 재인계, 원본 큰 메뉴 부모 3행(0.707), 앞선 Native 초기화 주소 세대와 전체 bridge(0.714), Native WorldRuntime 장치 일원화, 미션 없는 장기 Surface/주변음·영화 갱신 잠금과 나머지 루프 단계(0.723), 메뉴 Kernel/효과·전체 State/자산 재시작(0.725).
  - 완료(자원·자산·선택,0.738~0.747): 실제 서버목록/경로·장기 음악/Template·Session 공유·State 효과분배/Root경계·첫메뉴 자산표·장기 선택/강조/선택부모·객체5600입력/실제Clear/Root커서다. 나머지 UI/음향/Root 기본 장치와 월드 표시 설치는 위 남은 범위다. [0.747](docs/dotnet-canon-0747-20261121.md)
  - 완료(실제 채팅/구독자,0.757): 실제 Host My/Login/Handle·Chat4800BYTE와 World/Session 공유·현재 getter·Root ResetOccupied다. manager/명령표/시작·Guide Init/전체 송수신/창UI는 남았다. [0.757](docs/dotnet-canon-0757-20261121.md)
  - 완료(Chat manager 본체/할당,0.758): 생성/설정/랭크의644원문 관측과 실제 current·할당/캡처 수명·Host 소유자다. 명령표와 실제 공통 시작/랭크 장치·전체 제출/창은 남았다. [0.758](docs/dotnet-canon-0758-20261121.md)
  - 완료(실제 명령표/Chat 시작,0.759): 같은 say/count1 인접표와 Issue/Chat 등록·실제 Host manager 초기화다. Dispatch/제출/전체 UI와 전체 Native 시작은 남았다. [0.759](docs/dotnet-canon-0759-20261121.md)
  - 완료(점 명령 분배,0.761): 원문512와 같은 owner 표/장기 검색·중첩 separator·현재 오류 이름 뷰다. 실제 Chat 제출/창과 명령 효과 장치는 남았다. [0.761](docs/dotnet-canon-0761-20261121.md)
  - 완료(Chat 제출/수신자,0.762): 원문383의 인접 캡처/현재 기록과 실제 owner의 확장/반납 장치다. 실제 형식/제출 부모·일반 전달/창/UI는 남았다. [0.762](docs/dotnet-canon-0762-20261121.md)
  - 완료(실제 형식/제출 부모,0.763): 같은 두상자/캡처/설정·제출→분배 선택 장치와005cab30/Chat root/Host API·문화권 키 교정이다. 실제 Identity/메시지/창UI와 나머지Root는 남았다. [0.763](docs/dotnet-canon-0763-20261121.md)
  - 완료(실제 Identity/대상 검사,0.765): 현재 Player 배열을 매진입 빌리는 실제 Game/Surface/Subscriber/Chat 부모와 표시 전용 대상 검사·Host binding이다. 일반 메시지/패킷·CS/창UI 소비자는 남았다. [0.765](docs/dotnet-canon-0765-20261121.md)
  - 완료(실제 일반 메시지/패킷,0.766): 같은 두상자/Identity/이름/할당과 일반 조합·명시 패킷·Host flow다. count/문화권 차이2건을 재현/교정했고 원문524+256관측이 일치했다. 실제 창UI 소비자/현재 전송·수신 장치는 남았다. [0.766](docs/dotnet-canon-0766-20261121.md)
  - 완료(ChatWindow 장기 상태/편집,0.767): 같은 창/로그/무시/동맹/뉴스/완성과 안정문자열·원문189관측·실제Host TextEntry 대상/완성 설치다. 전체 창/공장/송수신·뉴스 수신과 프레임말 토글은 남았다. [0.767](docs/dotnet-canon-0767-20261121.md)
  - 완료(실제 Chat 기하/프레임 요청,0.769): 같은 네자식/3960관측과 실제 Host Tree/로그/현재모드·프레임말 전환 요청이다. 전체 창/자식 가상분배와 Enter의 Root 입력 호출자는 남았다. [0.769](docs/dotnet-canon-0769-20261121.md)
  - 완료(ChatWindow 전체 부모,0.770): 같은 실제 Console/버퍼의 생성/입력/helper/수명과180관측/기본166부분집합·기존2292유지다. 실제 공통 UI 공장/소유권·자식 가상분배/Enter Root와 송수신은 남았다. [0.770](docs/dotnet-canon-0770-20261121.md)
  - 완료(공통 UI 소유권/문자열,0.771): 같은Tree의 중첩/예외 owner·실제Console 하위 자식/문자열 scalar 반납·548758 공유 로그 반납 추적이다. 전체 Chat 공장/가상분배·Root 입력/송수신은 남았다. [0.771](docs/dotnet-canon-0771-20261121.md)
  - 완료(실제 스타일 그림 자산/출력,0.773): 같은 최초TYPE/정의/SHP·본문/controlled 메뉴와 실제 픽셀/색표 문맥이다. Native 색표7/옵션 변경 부모와 전체Chat 공장/입력·송수신은 남았다. [0.773](docs/dotnet-canon-0773-20261121.md)
  - 완료(전체 실제 Chat 공장/가상 분배,0.774): 같은Tree/Console/로그/그림/controlled 메뉴의 전체 공장·Host 설치·가상 입력/부착/소멸이다. 실제 NativeRoot/HUD 창 생성·입력/제출 소비자와 송수신은 남았다. [0.774](docs/dotnet-canon-0774-20261121.md)
  - 완료(Chat 중계/고정등록·관찰,0.775): 원문132와 같은mutable 패킷/마스크·42dc90 F/observer·현재 재조회다. 실제 Client41h receiver/연결·UI 수신과 Root 입력/제출은 남았다. [0.775](docs/dotnet-canon-0775-20261121.md)
  - 완료(장기 Client41h 수신 바인딩,0.777): 같은 현재 Registry/Player/마스크·실제 내부Source 원격/로컬 전달이다. MonoGame Root의 Dispatcher 부모·UI 수신/입력·제출/송신은 남았다. [0.777](docs/dotnet-canon-0777-20261121.md)
  - 완료(UI수신 원문/부분 버퍼,0.778): 기존 본체 원문238관측과 전체패킷/효과 일치·같은 mutable Span 입구/콜백 뒤 현재 읽기·선택 오류 전 기록 보존이다. 실제 Root 수신/뉴스/송신 설치는 남았다. [0.778](docs/dotnet-canon-0778-20261121.md)
  - 완료(실제 Root UI수신,0.779): 같은 실제 창/로그·현재 관계/설정·등록 버튼 알림·서버별 뉴스 저장/HTML 생성기와 장기 음향을 Root에 설치했다. 전체 공통41h 전송 설치·HUD 알림 ID 작성/Enter·제출/송신·NativeMapView 공급자는 남았다. [0.779](docs/dotnet-canon-0779-20261121.md)
  - 완료(장기 패킷 기록/Root 소유자,0.781): 같은 실제 Frame/UserDataRoot·21BYTE 접두/현재 선언길이·열기 실패 상태/요청 분리와 Root 지연 생성·최종 파일 반납이다. 실제 Source Record/Sent 소비자와435220 요청 reset은 남았다. [0.781](docs/dotnet-canon-0781-20261121.md)
  - 완료(기본18행/송신 관찰기,0.782): C3 7행·관찰 전용11행·9종 현재 필드/진단/WORD 감김/signed BYTE와 다른 수신기/표 보존이다. 전체 기본 설치 부모와 Root 실제 Source는 남았다. [0.782](docs/dotnet-canon-0782-20261121.md)
  - 완료(장기 설정/버전 응답,0.783): 같은 GameAux7DWORD/0ch 생성 전역·네 미션 observer·32h 고정 F·실제 Registry의0bh 응답이다. 전체 기본 설치 부모/나머지 고정 수신과 Root 실제 Source·Record/Sent는 남았다. [0.783](docs/dotnet-canon-0783-20261121.md)
  - 완료(독립 커서 데이터·수명,0.749): Take/Cancel/Activate와 자산 어댑터·고정 데이터의1575원본입력 대조다. 실제 Gameplay 커서/checking owner와 위치 준비/렌더러/Root Cancel 설치는 위 남은 범위다. [0.749](docs/dotnet-canon-0749-20261121.md)
- [ ] **R4-7 데모와 보류 검사.** `StartDemo` 3경로 덤프, `player-world-x86.tsv` 1,680행, F3에서 미룬 Fort/지형 바인딩·미션 Fort 경로·로그인 객체 전송·미션 재적재 검사를 잇는다.
- [ ] **R4-8 M1 판정.** 같은 시드·고정 프레임·빈 사용자 루트로 34/34 상태 일치. 로드 뒤 프레임 진행에서만 생기는 차이는 cpp 후속 프레임 덤프로 입증한 뒤 R6으로 넘긴다.

<a id="r6-preports"></a>

### 1.2 후속 단계에서 이미 이식한 범위

- [x] **R6-3 배치 본체(0.609~0.611).** 권한·관계, 지형 메이커 2종, Player 앵커·서라운딩·파이프라인을 이식했고 MayPlace 3건 전수 일치를 확인했다. 실제 농지 호출은 R4-2(0.614)에서 결선했다. 사용자 건설 입력 통합은 R6-3에서 검증한다. [0.611](docs/dotnet-canon-0611-20261118.md)
- [x] **펜스 본체 8종(0.587~0.592).** `RawFenceLifetime`·`Neighbors`·`Line`·`MarkerClear`·`Rebuild`·`Create`·`PostPop`·`Regular`과 해당 저장소 갈고리를 이식했다. 세션 Pop·정기 결선은 0.636에서 마감했다. R6의 타깃·타입 부모·Damageable 정기 라우팅과 표식 생성·연결선 생성·이웃 새로고침은 열려 있다. [펜스 후처리와 정기 처리](docs/dotnet-surface-fence-postpop-20261117.md)
- [x] **시설 업그레이드 본체(0.666~0.667).** 요청·수신·공통58h writer와 사건3 그림/완료·실제 Host 정리·현재 AI 통지 본체를 이식했다. 가상/건설/표시/음향과 요청 API는0.682~0.683, 연구 내용물/Deck·부모 통지는0.686에서 실제 세션에 연결했다. 제품 연구 분배·요청 전송/등록과 UI, 세 비행장 홈 통지·게임 내 HUD 메시지(0.682)는 후속이다. [요청](docs/dotnet-canon-0666-20261120.md) · [그림/완료](docs/dotnet-canon-0667-20261120.md)
- [x] **플레어·건설 선행 본체(0.669~0.671).** 플레어 생성/정기, 건설 진행/완료/실제Host 수명, 밀어내기/실패 섬 정리를 원본 관측으로 대조했다. 실제 건설 표시·낙하·음향·전송·배치 통지·세션 장치는 후속이다. [플레어](docs/dotnet-canon-0669-20261120.md) · [건설 진행](docs/dotnet-canon-0670-20261120.md) · [건설 효과](docs/dotnet-canon-0671-20261120.md)
- [x] **건설 표시 본체·프로세스 수명(0.673~0.675).** 생성/동기화/입자 선택1,776행, 프레임/Draw1,317행과 실제10.78 Kernel/Form/공통 목록 수명12건을 확인했다. 제품의 실제 캡처·팔레트·출력/음향·세션 장치는 후속이다. [생성](docs/dotnet-canon-0673-20261120.md) · [프레임](docs/dotnet-canon-0674-20261120.md) · [수명](docs/dotnet-canon-0675-20261120.md)
- [x] **건설·플레어 실제 장치(0.677~0.681).** 공장 가상 그림·실제 SHP 캡처(0.677), 같은 WorldRendering/Host/Kernel·COL 팔레트·현재 카메라/dirty/8비트 출력의 제품 건설 표시 장치(0.678), 플레어의 실제 장치·정기·음향(0.679), 실제 HP/시계/권한·진행음/완료음·공간 재등록/밀어내기의 건설 진행/완료 장치(0.681)를 연결했다. private 효과의 DropIsland 낙하/전송, 실제 건설 요청, Carrier 플레이 효과, 나머지 캡처·전용 색표·R5 월드 뷰는 후속이다. [0.678](docs/dotnet-canon-0678-20261120.md) · [0.681](docs/dotnet-canon-0681-20261120.md)
- [x] **간헐천 수명과 벌룬 본체(0.583~0.584).** 세션 Pop·사건 0 정기는 0.634에서 결선했다. 채굴 `RawGeyserMined`과 `RawBalloonFrame`은 R6, AI 등록은 R4-5에서 잇는다. [간헐천](docs/dotnet-surface-geyser-20261117.md) · [벌룬](docs/dotnet-surface-balloon-20261117.md)
- [x] **Outpost 전처리 본체(0.582).** 장기 삭제 분배·목록 압축·부모 호출을 집중 검사했다. 전체 Outpost 수명과 LAN 통합은 R11에서 이어간다. [Outpost](docs/dotnet-surface-outpost-20261117.md)
- [ ] **공통 장치의 실제 미션 연결.** `RawSurfaceAssetDeletion/Notice`·`Content/Reward`·`Lifetime/Deletion`·`PriestDeletion`·`Damageable/Fence/Balloon`은 대응 본체 또는 분배 장치가 있다. 큰 파일의 미이식 분기와 호출자 장치는 R4-2~R6에서 확인한다. `RawIceTower`의 성장 정기·완성형 파괴·재생성/삭제와 Native 설치도 남아 있다. [R4-1 감사](docs/dotnet-r4-1-close-20261117.md)

<a id="r3-r2-detail"></a><a id="carried-limits"></a>

### 1.3 앞 단계에서 이월한 작업

- [ ] **R3 UI.** `ParseDialogPage`·`ConfigOverwriteAndQueueLifetime` 잔여와 MessageGump, 맵/파일/기술 목록·Guide·모달·명령 소유자, QuickHelp TYPE/AST, Console/클립보드/Save as/숫자 창/ChatWindow, 배경 GIF 노드·창, 하위 화면 라벨과 실제 명령 소비. HTML/스타일 그림의 실제 자산/본문·메뉴 출력은0.773 완료이며 Native 색표7/옵션 변경 부모는 후속이다.
- [ ] **R2 화면 통합.** 실자산·그리기 결선과 구름 카메라·시간 이동·dirty 갱신·구름 파일 fallback(R5). 빠른 물리 입력 이력과 옵션 변경 중 소리 수명은 R6.
- [ ] **파일·호스트 경계.** `--set` 중복 처리 차이, `MatchDisk` 절대/긴 경로, Linux `WriteAtomic`·`ReadOnly`, 이름 정규화·옛 인코딩, Fort 이름/Template 소유자·Save as는 해당 소비자를 옮길 때 대조한다. CD 슬롯 폭·경계의 하네스와 기존 단위의 예외·할당·복사 경계는 보관본 2.6과 단위 문서를 참조한다.

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a><a id="04-기존-dotnetpj-소스의-재수정-판정"></a><a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a><a id="dotnet-user-decisions"></a>

## 2. 계속 유효한 결정

- **원본 10.78 경계 보존.** 화면은 8비트 인덱스 버퍼·256색 팔레트, MonoGame은 창·입력·오디오 장치·최종 출력이다. cpppj의 파일/클래스·호출 순서·부작용·정수 폭·float32 저장·목록·난수·PATCH 50바이트 슬롯 배치를 보존한다. 플랫폼 장치는 .NET/MonoGame으로 대체하고 OS 전용 동작은 호스트에 둔다. cpppj 미복원·보류 경계는 명시 진단으로 드러낸다. CD 10.72 제품 복원은 cpppj 범위다.
- **D1(사용자 결정).** cpppj HEAD의 요구사항 코드가 공통 파일에 섞여 있으면 함께 옮긴다. 기본 영어·보통·75 FPS·4:3 창 모드에서 원본 동작을 먼저 대조하고 후순위 기능은 R8에서 검증한다. 파일 장치는 사용자 루트 정책을 초기 이식부터 적용한다.
- **D2(사용자 결정).** 새 경로가 `Netstorm.exe`를 이어받고 완료 후 기존 경로를 제거한다. 적용 시점인 R5 전환/R6 삭제는 기존 에이전트 해석이다. 기존 경로의 `settings.json`은 새 경로로 가져오지 않는다. 근사 검사는 해당 코드와 함께 은퇴시키고 L1 대조로 확보한 검사는 새 모듈에서 잇는다.
- **D3~D5(사용자 결정).** R8에서 로컬 MCP와 한국어·일본어 화면을 검증한다. Linux는 `$HOME/.netstorm/` 아래에 저장한다. Template 판독·433 Fort 왕복 검사의 기존 결정은 [Template 계획](docs/dotnet-template-storage-plan-20261101.md)에 있다.
- **사용자 지시.** 정기 전체 회귀는 4단위당 1회만 수행한다(2026-11-19). 실제 포획/희생·특정 적 공격·시나리오 승리는 사용자 요청대로 재현하지 않고 사용자 수동 확인으로 둔다.

<a id="original-parity-plan"></a>

### 2.1 원본과의 의도적 차이(9-1 결정)

같은 입력의 원본 근거를 확보해 동작 차이를 판정한다. 현 구조로 원본 경험을 재현할 수 있으면 유지하고 필요하면 구조를 바꾼다. [2026-10-10 결정과 A~F 상세](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)

| 항목 | 완료 범위와 다음 작업 |
| --- | --- |
| A 시계·고정 틱 | 프레임 명령·붕괴 스캔·정지 시각 첫 단계 완료. 나머지 정수 틱·보간·대본/브리핑 지연·틱 기반 검사 이전은 R4/R6. [첫 단계](docs/dotnet-frame-parity-20261011.md) |
| B 입력 큐 / E 부분 다시 그리기 | 40칸 입력 큐 본체는 R1 완료, 실제 사건 소비는 R6. dirty 영역 표시 경로는 R5. |
| C 설정 / D 프레임 대기 / F 실행 환경 | 사용자 루트 `options.cfg` 저장 시점과 전체화면 재실행, 양보 대기 프레임 시간, 중복 실행·창 크기 등 관측 동작은 R7에서 판정한다. |

<a id="dotnet-r8-requirements"></a><a id="agents-reflection"></a>

## 3. AGENTS 4~15차 — R8 반영 범위

현행 AGENTS 변경은 갈무리11 지정 `2c8ca3bf8`, 15차 추가 `faecb9cbd`, 실행 파일 아이콘 `d047bf9a3`([0.4](#exe-icon)), webpj favicon `11b16b778`(DN 해당 없음)까지 확인했다(2026-11-21 재확인, 이후 변경 없음). 아래는 새 경로 기준이다.

| 차수 | 처리 및 검증 |
| --- | --- |
| 4 로컬 MCP·와이드 | stdio JSON-RPC로 모든 조작·상태 조회·화면 캡처·소리 포함 녹화. 4:3·16:9·16:10, 16:9 최대 1920×1080의 논리 해상도와 그래픽 크기 유지. [MCP](docs/exe/cpp-mcp-server-20261106.md) · [와이드](docs/exe/cpp-wide-resolution-20261106.md) |
| 5 사용자 데이터 | Windows `%USERPROFILE%\Saved Games\Netstorm\` 아래 모든 런타임 생성물. 설정·진행은 `Saves/`, 맵은 설치 경로와 `CustomMaps/` 양쪽 읽기, 새 Edit 맵은 `CustomMaps/` 저장. 목록 무제한 스크롤과 읽기 전용 설치 확인. [cpppj 사용자 루트](docs/exe/cpp-user-data-root-20261105.md) |
| 6 한국어 | 외부 갈무리11과 한국어 명칭 규칙. 표시용 부호화·`DisplayTextInput`의 선행 이식을 화면·번역과 연결한다. [전환 체크](#korean-font-change) |
| 7 오류·로그 | 공통 오류/예외 처리와 상세 로그(`logs/yyyyMMddHHmmss.log`), 시작 실패부터 추적 가능해야 한다. [cpppj 로그](docs/exe/cpp-error-log-20261111.md) |
| 8 AA·FSR 1.0 | Options에서 선택, 둘 다 기본 OFF. 지원 환경의 2560×1440·3840×2160은 내부 1920×1080에서 자동 FSR. [cpppj 출력 계약](docs/exe/cpp-aa-fsr-20261106.md) |
| 9 시나리오 Edit | 새 맵 기본 `.english` 자동 생성, 누락 시 Test Battle 종료 오류 교정, 편집 UI에서 초기 SP·테크·플레이어·동맹 수정. [cpppj 편집](docs/exe/cpp-scenario-s3-ui-20261111.md) |
| 10 알려진 버그 | cpppj의 34건 판정과 수정 10건을 해당 파일의 이식·검증에 반영하고 NB 번호를 기록한다. [판정표](docs/exe/cpp-known-bugs-status-20261111.md) |
| 11 난이도 | Options 행과 AI 배율 부품 선행. 기본은 보통(원본), 쉬움은 적 AI의 다리·생산 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 적 AI의 주기 ×½. 동맹 판정 포함. [cpppj 난이도](docs/exe/cpp-difficulty-d-20261111.md) |
| 12 스커미시 | Campaign에서 시작/취소, 적 수·AI별 난이도/동맹, 금지 유닛 최대 5개(Outpost 포함). 멀티플레이 규칙의 임의 섬·간헐천. [cpppj 스커미시](docs/exe/cpp-skirmish-sk-20261111.md) |
| 13 일본어 | 외부 Noto Sans JP, 일본어 UI와 본문 영어 대체. 번역 범위는 cpppj D3. [cpppj 일본어](docs/exe/cpp-japanese-l1-l3-20261111.md) |
| 14 QuickJS | quickjs-ng 엔진 탑재·자체 검사·라이선스 동봉. 추가 연동은 별도 사용자 결정 대상이다. [cpppj 엔진 계약](docs/exe/cpp-quickjs-engine-20261111.md) |
| **15 Reset All** | **미착수.** Options 맨 아래에 Reset All을 제공한다. 취소 시 현재 설정·진행을 보존하고 확인 시 `options.cfg`를 포함한 설정·게임 진행을 초기화한 뒤 종료한다. 다시 실행해 최초 상태를 검증한다. 2026-11-19 소스 확인 기준으로 cpppj에도 아직 기준 구현이 없다. [AGENTS.md](AGENTS.md) |

**공통 완료 사항.** 기존 배포의 `Netstorm.exe` 이름과 압축 스크립트, 11.01·Credits 3항목·HJOW/AI 크레딧 및 카피라이트(0.150·0.342), 폐기 클라이언트 유효성 검사 비활성·외부 접속 문서화(0.151). 새 소비자가 생기면 외부 접속 목록을 갱신한다. `Netstorm.exe`의 원본 아이콘은 아직 미완료다([0.4](#exe-icon)). [배포 스크립트](dotnetpj/package-dotnetpj.ps1) · [외부 접속](docs/dotnet-external-endpoints-20261103.md)

<a id="resync-scope"></a><a id="cpppj-requests"></a><a id="dotnet-inventory"></a>

## 4. cpppj 동기화와 이식 목록

- **cpppj 1323이 현행 제품.** 0.606의 오라클 재빌드 기준은 1322다(1323은 아이콘 리소스만 추가). 마지막으로 문서화한 변경 분류는 0.530의 1312·1313이다. `979d65b5d..HEAD`의 `cpppj/src` 변경 26파일 중 PORT_MAP과 겹치는 것은 11파일이며, 그중 6파일은 이미 변경 뒤 커밋을 base로 재대조했다(2026-11-21 확인). base가 변경보다 앞선 5파일은 `BitmapFont.cpp/.h`(1322 글꼴), `ClientMain.cpp`의 일부 행(1322), `LocalTcpTransport.cpp`·`LocalUdpTransport.cpp`(1320 미션 전환)다. 해당 소비자를 옮길 때 재대조를 마친 뒤 PORT_MAP 기준 커밋을 올린다. 1314(최초 실행)는 0.2, 1322는 0.3, 1323은 0.4와 함께 반영한다. [1312·1313 처분](docs/dotnet-ai-alliance-delta-20261115.md) · [1320](docs/exe/cpp-mission-transition-1320-20261117.md) · [cpppj 현행](LEFT_JOBS.md)
- **기존 재대조 규칙.** CPP 소스는 커밋된 HEAD를 사용한다. 해당 소비자를 옮길 때 바뀐 cpp 소스·fixture·검사를 함께 확인하고, cpppj 테스트 자동 플레이와 관전 경계는 R5/R6에서 잇는다. [오라클 자동 플레이](docs/testing/cpp-autoplay-test-guide.md)
- **현재 집계(2026-11-21, 0.780 전체 검사).** PORT_MAP 물리1,373줄(데이터1,368행, port880·L1 481·session7), 연결fixture396개다. 현행 cpppj1,239개 소스 단위(169,296줄) 중550개(95,868줄) 대응·689개(73,428줄) 미대응이다. 대응 수는 `tools/port_inventory.py`가 인식한 단위 기준이며 파일별 부분 이식을 포함하므로 기존 대응의 미이식 분기·호스트 결선이나 게임 플레이 완료를 뜻하지 않는다. 원자료는 `extracted/dotnet-0780/port-inventory.tsv`와 `.log`다.
- **파일 계열 목록.** 현행 [PORT_MAP](dotnetpj/PORT_MAP.tsv)과 `python -X utf8 tools/port_inventory.py --families dn`을 사용한다. 예전 단계 배정 및 대응 파일의 미완 부분은 [보관본 2.4~2.7](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에서 찾는다. 단위 작업은 해당 계열이나 큰 파일 하나를 옮기고 검사·호스트 결선까지 확인한다.
- **cpppj 전달 항목.** 상태 덤프와 고정 시각 요청은 해결됐다. fixture 헤더·모듈 연결 상태표, `GraphTests` 슬롯 열 범위와 `ClearProcesses` 뒤 정기 조회 진단의 이전 요청은 보관본 1.4에 있다. 변경이 필요하면 사용자에게 전달한다.

<a id="verify-levels"></a><a id="user-checks"></a><a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a><a id="dotnet-commands"></a><a id="dotnet-ops"></a>

## 5. 검증과 작업 규칙

- **L1** 원본 x86 fixture, **L2** 시작 상태 덤프, **L3** 고정 시계·입력열 뒤 상태 해시, **L4a** 논리 화면 픽셀, **L4b** 소리 시점·곡 선택, **L5** 자연 완주·저장·재실행. 파일 이식·집중 검사·실제 세션 결선·전체 일치를 각각 기록한다. `snapshot`과 `snapshot-output`을 구별하고 실행 근거에 SHA·시드·고정 시각·프레임·스크립트·옵션을 남긴다.
- **사용자 확인.** L5와 전체 화면/청취, 실제 사제 포획·희생·특정 적 공격·승리·MP/LAN, 공격형 공중 3종 실제 생산/조작은 사용자 확인 범위다. 창 실행은 AGENTS.md의 명시 확인·예외 PC 규칙을 따른다. Windows 10에서 통과하면 Windows 11 실기와 사용자 관찰만 남은 상태에서도 다음 단계로 진행한다. 물리 DPI·4K FSR·UAC는 R8 이후, `DSERR_NODRIVER` 실장치 재검사는 출력 장치가 있는 PC에서 한다. [소리 장치 기록](docs/dotnet-sound-device-20261114.md) · [공통 실행 범위](LEFT_JOBS.md#rules-verify)
- **공통 기능 경계.** CHEAT 메뉴는 Edit에서만 표시한다. 연녹색 직사각형 분석은 보류다. 폐기된 인터넷 master/root·검증·ping/진단은 비활성 상태를 유지한다. 도움말 링크와 LAN은 해당 사용자가 선택한 경로를 따른다. [공통 계약](LEFT_JOBS.md#rules-verify)
- **오라클과 자료.** `cpppj/build/bin/Release/Netstorm.exe`를 PowerShell에서 빌드한다. 실행은 `extracted/dotnet-oracle-data/` 사본과 분리한 사용자 루트로 진행하며 `originals/` 해시를 전후 확인한다. 0.606의 VM 소프트웨어 GPU 환경은 [재측정 기록](docs/dotnet-l2-0606-20261117.md)에 있다. 일반 덤프 비교와 최초 실행 검증의 옵션을 구별한다.
- **검사 명령.** `dotnet build dotnetpj/Netstorm.sln -c Release`, `dotnet test dotnetpj/Netstorm.sln -c Release`, `python -X utf8 dotnetpj/tools/cpp_sync.py`를 쓴다. 실제 TYPE 검사는 기본 데이터 또는 `NETSTORM_DATA`로 선택한 보호 자료 사본을 읽는다. 2026-11-19 사용자 지시로 정기 전체 회귀는 4단위당 1회만 수행한다(0.780 완료 뒤 다음0.784). fixture는 사본 없이 한 소비자 프로젝트에 링크하고 `cpp_sync.py --update --add`로 매니페스트에 등록한다. GUI 명령·변이 검사·등록 규칙 전문은 보관본 6.1에 있다. [L2 도구](docs/dotnet-l2-batch-20261114.md) · [L4a 도구](dotnetpj/tools/l4a_compare.ps1) · [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md)
- **착수·보호·커밋.** AGENTS.md·LEFT_JOBS.md와 이 문서, git status/log를 확인한다. dotnetpj 이식에 디컴파일은 필요 없다. AGENTS.md·기존 원본/CD 파일·README 개요·기존 fixture를 보호한다. 작업 단위마다 0.1절 재개 순서·요약표와 1.1절 체크, 단위 문서를 갱신하고 해당 파일만 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다. 단위별 완료 문장은 단위 문서에 쓰고 이 문서에는 누적하지 않는다. push 금지. 새 코드·문서는 UTF-8, AGENTS 범위의 코드 주석은 설명·사용법부터 한국어로 적는다. 이 문서와 보관본의 작업 트리 줄바꿈은 CRLF다(저장소 LF).

<a id="regression-audit-0640"></a>

### 5.1 0.640 회귀 지연과 실패 점검

`NETSTORM_DATA` 미지정에서도 Assets 배포 자료·Core 기술 표 검사가 기본 데이터 경로를 찾도록 고쳤고, `CanonMayPlaceTests`(약 20분 → 0.64초)와 `FortObjectSaveTests`(약 14~16분 → 0.66초)의 기록 누적 방식을 바꿔 단언을 유지한 채 Release 전체 검사를 프로젝트별 합계 약 7분으로 줄였다. 원인·시간표·TRX 위치·cpppj 대응 코드 비교는 [0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)의 5.1절에 있다.

<a id="dotnet-done-index"></a><a id="dotnet-unit-records"></a><a id="dotnet-archive-log"></a>

## 6. 완료 기록과 보관

| 단위 | 날짜 | 완료 범위 |
| --- | --- | --- |
| 0.546~0.600 | ~2026-11-17 | 객체 수명 공통층과 미션 저장소 공유(R4-1) |
| 0.601~0.636 | 2026-11-17~19 | 섬·지형·지원 요새 Pop과 그래프, MayPlace 본체·전수 대조, 농지 Generate, 지역·신전·사원 Pop, 공장 후처리·정기·초기 레벨, 미션 틱 결정론·바람·주변음, 간헐천·거주 구역·펜스 Pop과 정기 |
| 0.637~0.665 | 2026-11-19~20 | 직접 AI 명령, AI 생성·초기 갱신·준비·전략·프레임·통지·등록·수집자·그래프·경제·배치·시설 생산, 장식·Player·스프라이트·매몰 정기 |
| 0.666~0.688 | 2026-11-20 | 업그레이드·연구·생산 Deck, 플레어·건설 진행/표시/실제 장치, 공유 패킷 분배·연구 운반 |
| 0.689~0.720 | 2026-11-20 | 내부 로그인·전송·Client 전역, 장기 Runtime/Player 수명과 현재 Runtime pin |
| 0.721~0.737 | 2026-11-21 | Client 프레임·장기 Surface/Host, Native State 정리·저장소·실제 Runtime 반납·MetaZone 본체/Client삭제경계·NSInfo 호스트 소유자, 0.736 전체 검사·65시작 전수 감사 |

정기 전체 회귀·일괄 덤프는 0.644부터 4단위마다(0.644·0.652·0.656·0.660·0.664 … 0.728) 수행했다. 단위별 근거는 `docs/dotnet-*.md`와 Git 기록에 있다.

- **2026-11-21 이관.** 정리 직전 0.732 인계 전문을 보존했다. [0.732 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)
  - 옮긴 것: 0.1절의 단위별 완료 문장 88건(0.645~0.732)과 0절의 단위 bullet 7건(0.639~0.645)을 0.1절 주제별 요약표와 1.1절 완료 범위로 줄였다. 일시 중지 인계를 0.1절 재개 순서로 옮기고 지난 사실(다른 작업의 webpj 미커밋 변경 보존)은 보관본에만 두었다. 6절의 0.546~0.676 단위 나열은 단위 범위 표로, 5.1절 0.640 회귀 점검 상세는 요약 한 문단으로 줄였다.
  - 바로잡은 것: 0절 검사 기준선(0.712 → 0.732, 다음 0.716 → 0.736)과 L2 기준선(0.712 수집 → 0.728 수집), 5절 다음 정기 회귀(0.732 → 0.736), 4절 cpppj 현행 제품(1322 → 1323)·`979d65b5d..HEAD` 변경(23 → 26파일, PORT_MAP 교집합 7 → 11파일 중 base 미갱신 5파일)·이식 집계(0.720 → 0.732), 1절 R4 진행 표기(R4-2·R4-4 → R4-2·R4-4·R4-5·R4-6 병행). 0.1절 끝의 번호가 어긋난 "57. R4-2 나머지 차이와 R4-3~R4-8" 항목은 1.1절 머리 문장으로 흡수했다. 줄바꿈이 섞여 있던 작업 트리를 CRLF로 통일했다.
  - 새로 넣은 것: cpppj 1323 공통 아이콘 `assets/Netstorm.ico` 재사용(0.4), R4-4·R4-5·R4-6의 완료/남음 하위 목록, 1.2절 건설·플레어 실제 장치(0.677~0.681) 항목, 2절 사용자 지시 항목.
- **2026-11-19 이관.** 0.635 인계 전문을 보존하고 R4 체크의 단위별 문장 22건을 하위 목록으로 줄였다. [0.635 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)
- **2026-11-18 이관.** 0.613 인계 전문을 보존했다. 완료/중단 단위의 경과, 중복 체크, 이전 이식량·AGENTS 이력, 파일별 목록과 명령 예시는 보관본을 참조한다. [0.613 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)
- **이전 이관.** [0.545 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)과 보관 파일 머리의 날짜별 색인을 참조한다. 아래는 이전 문서에서 참조하는 앵커다. 같은 이름의 상세는 보관본에 있다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
