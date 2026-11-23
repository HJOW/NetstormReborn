# LEFT_JOBS.dotnetpj — 현행 상태와 남은 작업

**2026-11-23 · 목표 진행 중·마지막 완료0.883(Dude 정기 본체·1600행 전수·고정26/26) · 다음0.884(정기 전체 회귀) · 현재 단계 R4(R4-2·R4-4·R4-5·R4-6) · cpppj 제품1326 / 11.01 · 전체 목표 미완료**

현행 인계는 이 문서가 기준이다. 단위별 근거는 `docs/dotnet-canon-0NNN-*.md`이다. 0.798~0.837의 재개 순서 1번 단위별 연쇄·중지 인계 세부와 6절 단위별 완료 행 원문은 [0.837 정리 전 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)에, 0.733~0.797의 단위별 재개 연쇄·주제별 표 행과 1.1절 R4-6 단위별 완료 문장 원문은 [0.797 정리 전 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)에, 0.645~0.732의 단위별 완료 문장, 0.640 회귀 점검 상세, 0.546~0.676 완료 기록 원문은 [0.732 정리 전 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)에 있다. 0.600~0.635는 [0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635), 0.613까지의 경과와 파일 계열별 체크 목록은 [0.613 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에 있다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), 웹은 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 따른다.

<a id="dotnet-overview"></a>

## 0. 현재 상태와 다음 작업

| 경로 | 실행·플레이 | 원본과의 동일성 |
| --- | --- | --- |
| 유일한 실행 경로 `Netstorm.Game` (`Netstorm.exe`) + 내부 `Netstorm.Client` | 0.807에서 구 근사 실행기를 제거하고 직역 호스트가 이름을 승계했다. 타이틀·메뉴·도움말·Options·Credits·캠페인 선택·Edit 목록과 이름 창이 동작한다. 개발용 `--mission <키> --handle <이름>`으로 일반31미션의 시작 상태 일부를 조립한다. 월드 표시·입력·규칙의 실제 경로가 남아 플레이 불가다. | 1024×768 메뉴 **12장면**의 논리 BMP가 cpppj와 바이트 일치했다(0.381~0.388). L2는 **0/34**, 데모3경로 DN 진입 미연결이며 L3·L4b·L5는 미착수다. |

- **검사 기준선.** 마지막 전체0.880는 Assets376·Core3789·Client2135 합계6300통과/실패0·기존 실장치 음향1미실행이다.0.876의 모든 이름/중복과 신규28건(0.877 4·0.878 8·0.879 16)을 보존했다. NETSTORM_DATA 없이Release오류0·기존경고6, 같은검사/실행사본2/4/6과제품/검사/대응2038파일불변이다. 다음정기전체0.884. [0.880](docs/dotnet-canon-0880-20261123.md)
- **L2 기준선.** 마지막수집extracted/dotnet-l2-0880/. 일반31양쪽·CPP데모3의65실행모두종료0·오류0·누락0·원본1392/실행6불변이다. DN31전체덤프/34지표는0.876과같고0.879대표3도같다. M1 **0/34**(cpp_only997·dn_only21), DN데모3진입미연결이다. CPP차이는기존미초기화SP행3열범위뿐이다. [0.880](docs/dotnet-canon-0880-20261123.md)

<a id="now"></a><a id="09-바로-다음-작업"></a><a id="r4-board"></a>

### 0.1 재개 순서와 최근 완료 요약

**재개 인계(2026-11-23,0.879 완료·사용자 요청 일시 중지).** 0.841 기본 Host 지연 설치/동일 자원 예약,0.842 기본 수명·현재 VT35종/피해·건설 사건,0.843 기본 Dais/Path writer·실제 PID/FormPacket을 유지한다.0.844 첫 전체6,016통과 뒤65진입 중5개가 현재 Native Runtime의 Asset 미설치로 실패했다. 알려진 ProcessForm만 기존 Host 가상 삭제와 현재 Client 선택/전송에 연결했고 비 ProcessForm 자산은 명시 거부한다. Source Kill/현재 Runtime 우선 관문은 유지했다. 수정 집중10/10·실패5전체덤프0.840동일 뒤 전체6,018/실패0·기존 음향1미실행·같은빌드65종료0·DN31덤프/34지표0.840동일·0.843대표동일·원본1392/실행6/소스1932불변을 확인했다. 기존0.840 이름/중복과 추가52를 보존했다.0.842의새검사41→37과자료선택자를교정했다(기본13/정기피해14, 구현35는83행중32/밖3). M1 0/34·고정18/26·실제Load/Editor공개미연결/세설치값false·플레이불가는그대로다. 0.845는 RawConstructionData 전체codec/수신·실제PATCH Host/0x37등록과현재37/1헤더·Native Construction writer를연결했다. 독립3PE5340관측/새7포함Core179·Client72통과·대표3덤프/지표0.844동일·원본1392/실행6/소스1935불변이다. CD Wrapper는실제fields/식별자입력이며CDHost는미복원경계로남겼다. 0.846은GunProcess전체raw/본체/codec·실제flags0Host/삭제수명과Native현재writer를연결했다. 독립2996+통지904관측·새12포함Core191/Client72통과·대표3덤프/지표0.844동일·원본1392/실행6/소스1942불변이다. 현재GunType/헤더/역할상자를일반준비가보존한다. 0.847은총3ah수신/현재Gun owner·실제감시/체인/전송·고정F설치한행을연결했다. 새12포함Core191/Client205통과·대표3덤프/지표0.844동일·원본1392/실행6/소스1945불변이다. 0.848은같은901f12934제품으로전체6049/실패0·기존음향1미실행·0.844기존이름/중복/+31보존과같은빌드65종료0·DN31덤프/34지표0.844동일·0.847대표동일·원본1392/실행6/소스1945불변을확인했다. 0.849는공통FlyerProcess/codec·실제flags8Host/typed삭제와Native현재writer를연결했다. C38/B30/W1024총1092관측·새10포함Core201/Client72통과·대표3덤프/지표0.848동일·원본1392/실행6/소스1950불변이다. 0.850은FlyerPostPop전체/공통등록·분배·반납을P329+Sun391총720관측과대조했다. 같은Flyer예약/flags8부착뒤전문화·Wind동일Regular한번부착·공통깊이복귀를검증했고새6포함Core221/Client205통과·대표3덤프/지표0.848동일·원본1392/실행6/소스1953불변이다. 0.851은같은Flyer현재owner/Host/Kernel에39h수신·86h기한명령/고정F두행을연결했다. 새27포함Core221/Client232통과·대표3덤프/지표0.848동일·원본1392/실행6/소스1956불변이다. 0.852는같은29c5f4241제품으로전체6092/실패0·기존음향1미실행·0.848이름/중복/+43보존과같은빌드65종료0·DN31덤프/34지표0.848동일·0.851대표동일·원본1392/실행6/소스1956불변을확인했다. [0.852](docs/dotnet-canon-0852-20261123.md). 0.853은훨리베이스48BYTE/전체상태기계/홈통지/38h writer·실제Host/Native writer를연결했다. 독립1734+128관측·새10포함Core231/Client232통과·대표3덤프/지표0.852동일·원본1392/실행6/소스1962불변이다. [0.853](docs/dotnet-canon-0853-20261123.md). 0.854는같은Sun현재owner/Host/Kernel에38h 수신/고정F한행을연결했다. 새13포함Core231/Client245통과·대표3덤프/지표0.852동일·원본1392/실행6/소스1965불변이다. [0.854](docs/dotnet-canon-0854-20261123.md). 0.855는맨 오 워 풀48BYTE/상태기계/난수홈통지/73h writer·실제Host/Native writer를연결했다. 독립1602+36관측·새10포함Core241/Client245통과·대표3덤프/지표0.852동일·원본1392/실행6/소스1970불변이다. [0.855](docs/dotnet-canon-0855-20261123.md). 0.856는같은3f1746660제품으로전체6125/실패0·기존음향1미실행·0.852이름/중복/+33보존과같은빌드65종료0·DN31덤프/34지표0.852동일·0.855대표동일·원본1392/실행6/소스1970불변을확인했다. [0.856](docs/dotnet-canon-0856-20261123.md). 0.857은같은Rain현재owner/Host/Kernel에73h수신·기존38/73의Sun/Rain공통48BYTE선택·고정F한행을연결했다. 새15포함Core241/Client260통과·대표3덤프/지표0.856동일·원본1392/실행6/소스1973불변이다. [0.857](docs/dotnet-canon-0857-20261123.md). 0.858은데빌메이커80BYTE/생성·74h codec/전체packet사전검사를이식했다. 독립1894관측·새6포함Core247/Client260통과·대표3덤프/지표0.856동일·원본1392/실행6/소스1975불변이다. [0.858](docs/dotnet-canon-0858-20261123.md). 0.859는데빌메이커실제Frame/후처리/홈통지/FormDestroy/원주·Host/Native writer를이식했다. 독립3039관측·새9포함Core256/Client260통과·대표3덤프/지표0.856동일·원본1392/실행6/소스1981불변이다. [0.859](docs/dotnet-canon-0859-20261123.md). 0.860은같은4fb6fd3e4제품으로전체6155/실패0·기존음향1미실행·0.856이름/중복/+30보존과같은빌드65종료0·DN31덤프/34지표0.856동일·0.859대표동일·원본1392/실행6/소스1981불변을확인했다. [0.860](docs/dotnet-canon-0860-20261123.md). 고정본문23/26이며실제Root비행생산/음향/큰Frame·나머지3본문/전체26F설치는후속이다. [0.851](docs/dotnet-canon-0851-20261123.md). [0.850](docs/dotnet-canon-0850-20261123.md). 고정본문19/26이며전체26F설치/Root의Gun큰장치/나머지공중·타이머·애니메이션/정기와Load·Editor는미완료다. [0.849](docs/dotnet-canon-0849-20261123.md). [0.848](docs/dotnet-canon-0848-20261123.md). [0.847](docs/dotnet-canon-0847-20261123.md). [0.846](docs/dotnet-canon-0846-20261123.md). [0.845](docs/dotnet-canon-0845-20261123.md). [0.844](docs/dotnet-canon-0844-20261123.md) · [0.843](docs/dotnet-canon-0843-20261123.md).

1. **다음0.884 — 정기 전체 회귀.** 0.883은Dude정기 본체와 원문1600행 전수대조를연결했다. 새5검사포함Core5통과·대표3덤프/지표0.880동일·원본1392/실행6/소스2046불변이다. [0.883](docs/dotnet-canon-0883-20261123.md). 다음은 0.884 정기 전체 회귀로 0.881~0.883 신규41건과 기존 이름/중복을 보존한다. 이후 DaisRegular 본체(TempleRitual/RunePostPop 선행)와 세션 분배, Kernel/Process 초기 등록으로 R4-4를 닫고 R4-7·R4-8(M1)로 잇는다. RawGunTarget의실제보호자산cannon-asset-ray48은후속검사다.
   - 대상: 실제 Load/Editor prefix·후반 UI/최종 공개의 세 설치 값 쓰기 부모와 같은 실제 Root/생산 공장, 채팅/메시지 공장, 현재 지도/카메라/선택 수명, 같은 미니맵 전역 공급. Source ClientMain447~536의 실제 PostPop 선택 재검사·Transmission·Process·Resource·Reward·Deletion·PlayerRecords·표시/음향/AI 장치와 현재 Menu 데모 getter를 실제 owner로 조립한다. 0.838에서 일반 Root의 Native 전송/Process 선택과 현재 지도 getter를 설치했다. CreateAiWallet은 실제 Native Reward 장치에 공급해야 한다. 실제 게임 Root의 네 자식/메시지 공장과 바깥 UI 장치·0.838의 같은 현재 지도 getter를 재사용하는 HUD 작성자, 살아 있는 선택 객체의 수명을 잇는다. 실제 F7의 상위 일반 입력, NSInfo의 메타 배치/입력/특수 그림, Dais의 실제 current records 하위도 후속이다.
   - 선행 완료(중복 금지): 첫 장치 설치/같은 상태·깊이(0.814), 지갑 전체/현재 records·실제 Host 메모리 대여·AI 통지 진입점과 공유 Surface의 Player owner 빌림/혼용 거부(0.815), Player 고유Pre/약한 records owner(0.813)와 Residence/Dude, HUD 도킹·전체 Uber 부모/현재 this DockView·구분선 입력/공통 HUD 수명(0.817~0.819), 미니맵 기하/셀 그림·공유 FullRefresh와 실제 RawMinimapControls(공유 MapRevision 대여·기본 GumpRedraw/외부 출력 등록) 수명(0.821~0.822), NSInfo 생성/일반 부착(0.823), 생산 격자·정보·동기화·실제 생산 부모(0.825~0.826·0.829), 현재 OwnerColorTableState/ProductionControlContext·장기 Finder(0.827), 최초 고정 RGB/primary/9표·캐시 부모(0.830), RawBaseFileIO·같은 경로 검색·실제 CRT/TAFF·색표 캐시(0.831·0.833·0.834), Game 최초 팔레트·TYPE/사용자 캐시·소유자 옵션(0.835), ClientProductionSync 장기/대여 분기·생산 본체/초기 거부·색표7 소비자(0.837), 실제 Root 전송/Process 선택·같은 지도 getter/BSS0(0.838), Native Player 정기/현재 상태·Template/같은 Host(0.839). 묶음별 범위는 [1.1절 R4-6](#r4-detail)과 0.1 요약표를 본다.
   - 주의: 기존83VT 실제 분배/고유 파생·Runtimepin을 잇고 미설치를 일반 Pre/Post/no-op으로 통과시키지 않는다. prepared 상태나 동일 깊이 검사를 완화하지 않는다. 실제 지도 영역을 전체viewport로 대체하지 않는다. Root의 세 번째 NSInfo와 F6 RawKnowledgeMenu를 혼동하지 않는다. 후속 UI/World 소비자도 같은 현재 색표/OwnerTables를 빌려야 하며 별도 표를 만들지 않는다. 실제 색표 연결을 NativeRoot/World 전체 설치 완료로 세지 않는다. 설치 후/Editor 성공 범위를 0.837의 거부 검사로 완료 처리하지 않는다. 초기 Source 미니맵 공장은 Open이 아닌 Construct와 기존 bitmap 준비/전체 도킹의 Push→Attach를 사용한다.
   - 소견: 최신 `extracted/dotnet-0837/next-0838-static.md`. 이전 소견은 extracted/dotnet-0816/next-0817-static.md·미니맵 extracted/dotnet-0820/next-0821-static.md·부모 연결 extracted/dotnet-0821/next-0822-static.md·Root 공장 extracted/dotnet-0822/next-0823-static.md·생산 하위 extracted/dotnet-0824/next-0825-static.md·현재 부모 연결 extracted/dotnet-0826/next-0827-static.md다.

2. **NativeGameplay 26F 고정 수신 완료.** 현재 API `BindSquidPacketDevices`·`ReceiveFixedGameplayPacket`에 26개 entry 전체가 실제 본체로 연결됐다(기존 24개: [0.790](docs/dotnet-canon-0790-20261121.md)·0.791·[0.801](docs/dotnet-canon-0801-20261122.md)·[0.806](docs/dotnet-canon-0806-20261122.md), 신규 3b/3c: [0.882](docs/dotnet-canon-0882-20261123.md)). 남은 것은 Root의 실제 언어/이름/ScreenSize/FormPacket/수신 장치 공급, 같은 Host의 파생 수신/변경/전체 가상 삭제를 현재 Runtime·저장소 전송 관문과 함께 잇는 일이다. Dude 전용PostPop·사제 보호막/낙하 음향 등 기존 큰 경계도 실제 설치해야 한다. 검토한26행·본문·누락 하위 모듈과 기존 독립Path writer 자료는extracted/dotnet-0788/next-0789.md에 있다. 순수observer나 구Mission세션·일반삭제Pre/Post로 결선을 대신하지 않는다.
3. **R4-6 나머지와 R4 마감.** 위 항목 다음은 [1.1절 R4-6 남음](#r4-detail)의 전송·수신, Root 장치·UI, 표시·입력, 시작·수명 묶음을 잇고 R4-2~R4-5의 일치 대조, R4-7·R4-8(M1)로 넘어간다. 이어서 R5~R11과 후순위 요구사항(3절)을 진행한다. 다음 정기 전체 회귀는0.884이다.
4. **사용자 검사 범위(2026-11-22).** 캠페인1-1~1-5·스커미시에 한해 아군 AI 활성화로 포획/희생·특정 적 공격·시나리오 승리를 재현할 수 있다. 그 밖의 스테이지 해당 검사는 사용자 수동 확인으로 남긴다. 단순 진입/대기/생산/건설 검사는 진행한다. DN 자동 플레이의 실제 연결은 아직 없다. 원본1392파일은 마지막 전체65진입(0.876) 전후와 0.839 대표3시작에서 불변이다.

**최근 완료 요약(0.639~0.883).** 단위별 완료 문장과 집중 검사 수는 보관본과 각 단위 문서에 있다. 완료 범위와 남은 일의 기준은 1.1·1.2절 체크 항목이다.

| 주제 | 단위 | 핵심 | 남은 일 |
| --- | --- | --- | --- |
| AI 런타임 | 0.639~0.641(0.641 부분)·0.645~0.650·0.653~0.658·0.661~0.663·0.665·0.815 | AI 생성부터 준비·전략·전체 프레임·통지·월드 등록·경제·시설 생산까지 본체 이식과 세션 결선, 현재 지갑 전체/실제 Host 메모리 대여·장기 객체 통지 진입점(독립2520관측) | [R4-5](#r4-detail) |
| 정기 처리 | 0.642·0.643·0.651·0.659·0.839·0.842 | 장식·Player·스프라이트·매몰 정기 본체와 세션 분배, Native Player 현재 상태/Template 후단·실제 Host 예약, Native 기본 수명/현재 VT35종·현재 피해/건설 사건 | R4-4 |
| 업그레이드·연구·생산 Deck | 0.666·0.667·0.682·0.683·0.685~0.687 | 업그레이드 요청/완료, 연구·생산 Deck·기술 표시의 실제 세션 연결, 58h 분배·연구 운반 | R4-5, [1.2](#r6-preports) |
| 플레어·건설 | 0.669~0.671·0.673~0.675·0.677~0.679·0.681 | 플레어·건설 진행/표시 본체와 제품 장치·음향 | 1.2 |
| 파편·35h | 0.809~0.811 | 전체 본체/수명·독립1196관측·건물 캡처2577/기존 작성336관측·실제 Host/Game 파편/섬 정리와 같은 삭제 분배 설치 API | 일반 Root 전체 Native 삭제·캡처/월드 뷰와 나머지 고정 수신, 1.1 |
| 로그인·전송·Client 전역 | 0.689~0.691·0.693~0.695·0.697~0.699·0.701~0.703·0.705·0.707·0.715 | 패킷·연결표·Poll·로그인/재접속·옵션·상태 전이·Clock과 장기 Client 전역 | R4-6 |
| Runtime·Player 장기 수명 | 0.706·0.709~0.711·0.713·0.714·0.717~0.719 | 실제 Runtime 대여/반납, 장기 Player·Core·옵션 전역, 현재 Runtime pin | R4-6 |
| Client 프레임·장기 Host | 0.721~0.723·0.725~0.727·0.838 | Clock/Frame 공유, 프레임 실행 순서, 장기 Surface·Host Runtime | R4-6 |
| Native State 정리·저장소·반납 | 0.729~0.731·0.733~0.735·0.813~0.814 | State 정리 부모(원문294행), 실제 전역·저장소·장기SID, 실제 자식 증빙/Runtime 반납·제품 Surface 대여, MetaZone 본체/실제Client표·Runtime삭제경계와 Player 고유Pre/현재 records owner(독립360관측), 첫 장치 설치/동일Surface 장부·Graph·로딩·Native삭제깊이 공유 | R4-6(전체 가상 삭제 분배) |
| Root 자원·장기 자산표·선택 | 0.737~0.739·0.741~0.743·0.745~0.747·0.841 | NSInfo/서버 목록·장면 음악·Template/State 공유·State 효과분배/Root경계·첫메뉴 자산표·선택/강조/실제Clear/Root커서 | 기본 Host 지연 설치/Native 공장·동일 자원 예약(0.841) 완료, 0.842 기본 수명/정기35VT 완료, 나머지 전체 정기·직렬화 장치와 월드 표시 설치, R4-6 |
| 커서·카메라·Renderer·Host 표시 | 0.749~0.751·0.753~0.755 | 커서 고정 데이터·수명·위치와 현재 checking 공유, 카메라 16DWORD·실제 snapshot, Renderer 장면/캐시/dirty·Native submit·288원문 출력, Renderer 공개·Camera 초기화·구름 token·현재 Surface·Root 실제 Cancel | Native Frame/Root UI, 같은 지도 owner의 실제 HUD 작성자, 전체 커서 효과/입력, CameraCommit/Present |
| Chat 계열 | 0.757~0.759·0.761~0.763·0.765~0.767·0.769~0.771·0.773~0.775·0.777 | 실제 Subscriber·Chat 4800BYTE, manager·명령표·Chat 시작, 점 명령 분배, 제출·수신자·형식/제출 부모, Identity·일반 메시지/패킷, ChatWindow 상태·기하·전체 부모·공장/가상 분배, 공통 UI 소유권, 스타일 그림 출력, 중계·고정 등록, 장기 Client41h 수신 바인딩 | NativeRoot/HUD 창 생성·Enter, 공통 41h 전송 설치, 제출/송신 부모, Chat 명령 효과 |
| Root 수신·전송 관찰기 | 0.778~0.779·0.781~0.783·0.785~0.787 | UI 수신 원문·실제 Root UI 수신(뉴스·알림), 장기 패킷 기록/Root 소유자, 기본 18행·송신 관찰기, 장기 설정/버전 응답, 3f/30/7a/5f/7d 관찰기, 69h 장기 목록, 장기 CS/7bh | 전체 기본 설치 부모, Root Source/Record/Sent, 435220 요청 reset, NativeMapView 공급자 |
| 객체 패킷 수신·Host entry | 0.789~0.791·0.802~0.803·0.806·0.811 | 수신/변경/검증/압축/WORD 하위 본체, 실제 이름 83표·부착, Host 2c/7a/4a/4b·6a/7e/33/34/70의 실제 본문9개와변경/삭제5개·채집5bh·전체 개발자31h·섬 정리35h | 나머지8본문·전체 26F, Root ScreenSize/FormPacket·수신 설치 |
| 경로 Runtime·Data·본체·탐색 | 0.793~0.795·0.797~0.799·0.801 | 경로 Runtime·독립 관측, Data typed/Native 버퍼, Process 8종·전송 분배, BFS 탐색·Start/Cleanup 본체·Host 검색 공유, 건설 환불·Cleanup 하위·실제 SP 공유와 Halt/Arrival 본체·Host 부모 조립과전체Path/Start/Motion·30h | 제품Root전체장치설치와가상삭제 |
| HUD 도킹·전체 부모/공통 조립 | 0.817~0.819·0.821~0.822 | 도킹/같은 지도 owner(2150), 전체 Uber 부모/현재 this(160), 입력/방송(13398)·공통 Frame2186행과 실제 전체 UI 소유/가상 분배/수명·재배치, 미니맵 기하/셀 그림·공유 갱신 지원(4722/1028/120), 실제 공통 미니맵/기본 영역·출력/통지 수명(1474) | 실제 게임 Root의 나머지 패널/메시지 공장·현재 지도/선택/미니맵 전역 공급/바깥 UI 장치/Native MapView, R4-6 |
| 실행 경로 전환·Host 진단 | 0.805·0.807 | Host 부착 예외 소유권·독립 공장 진단, 구 근사 실행기 제거와 `Netstorm.Game`/`Netstorm.exe` 이름 승계·공통 ICO·갈무리11 배포 | R5 월드 화면, [0.3](#korean-font-change)·[0.4](#exe-icon) |
| NSInfo·생산 패널·색표 | 0.823·0.825~0.827·0.829~0.830·0.835·0.837 | NSInfo 전체 생성/일반 부착·공통 UI 소유, 생산 격자/항목/안내·정보/단축키/동기화/유지·실제 생산 부모/예산/출력, 현재 owner 색표·생산 문맥·장기 Finder, 최초 고정 RGB/primary/9표·캐시 부모, Game 최초 팔레트·TYPE/사용자 캐시·소유자 옵션, ClientProductionSync 장기/대여 분기·생산 본체/초기 거부·색표7 소비자 | 실제 Load/Editor 설치 부모·Root/생산 공장(0.1 1번), NSInfo 메타 배치/입력/특수 그림 |
| 파일 IO·검색·CRT | 0.831·0.833·0.834 | RawBaseFileIO 현재 FILE/키·부분 입출력 전체 본체(독립557관측), 같은 경로 Device/검색 수명·실제 Windows 검색·전체 열기 정책(독립1396관측), 등록 TAFF/현재 FILE·실제 CRT/캐시 reader·writer 입구 | RawFileName 나머지 조회·BaseFile 고리([1.1](#r4-detail) 시작·수명) |
| 정기 전체 회귀·L2 | 0.644부터4단위마다(0.880까지) | 전체6300통과/실패0·음향1미실행·신규28/기존이름/중복보존·같은빌드65종료0·원본1392/실행6/소스2038불변·M1 0/34 | 다음0.884 · [0.880](docs/dotnet-canon-0880-20261123.md) |

<a id="first-run"></a>

### 0.2 최초 실행 상태 — DN 적용 대기

원본 관찰과 cpppj 1314 구현이 완료됐다. DN은 빈 사용자 루트·무옵션 데이터 사본에서 아래 세 항목을 메뉴/설정 결선과 함께 확인한다. [원본 관찰](docs/exe/original-first-run-20261116.md) · [cpppj 1314](docs/exe/cpp-first-run-1314-20261116.md)

- [ ] Welcome(OK) → Welcome2(Play Early Missions / Main Menu) → 메뉴 버튼. Welcome2가 열릴 때 `welcomeGiven=1`을 사용자 루트에 저장하고 재실행 시 Tips 분기를 확인한다. `HostStartupScene.SaveOptions`의 설치 데이터 쓰기도 사용자 루트로 이동한다.
- [ ] Struggle For Freedom 최초 상태는 1번만 선택 가능하고 1~6번 모두 체크가 없다. 자기 `Done*`는 체크, 직전 `Done*`는 해금에 쓰고 저장·재실행을 확인한다. 실제 미션 결과 연결은 R6/R7에서 검증한다.
- [ ] 최초 Options는 소리·음악·바람·가장자리 스크롤·Auto-Demo·팁 ON, 스피커 교환 OFF, 효과음·음악 각각 5, 800×600·전체화면 요청이다. 기존 사용자 값과 진행을 보존하고 전체화면 재실행을 확인한다. 메뉴 일치 시험의 1024×768 창 모드는 시험 조건이다.

<a id="korean-font-change"></a>

### 0.3 한국어 글꼴 — DN 적용 대기

AGENTS.md의 현행 지정은 갈무리11이다.0.807에서 새 Game 빌드/게시/ZIP에 일반·굵은 TTF와 라이선스를 외부 fonts로 배치했고 D2Coding 배포 의존을 제거했다. 한국어 화면 연결은 R8 6차다. [공통 기준](LEFT_JOBS.md#korean-font-change) · [cpppj 1322](docs/exe/cpp-korean-galmuri-1322-20261117.md)

- [ ] `KoreanFontFamily` 공통 상수와 글꼴 파일·라이선스 설정을 마련해 로딩·측정·그리기·한글 대체·개발 표시가 같이 사용하도록 한다.
- [ ] 유일한 Game 경로의8비트 출력에 갈무리11의 비트맵 글리프를 적용한다. 구 실행기는 제거했으며 TTC 판독기 검사는 실제 TTC로 유지한다.
- [x] 0.807에서 Game.csproj/배포 스크립트의 복사·필수 파일·고지·안내를 맞추고 두 TTF/라이선스의 ZIP 전체 바이트를 검증했다.
- [ ] 작은 메뉴·도움말·브리핑과 영한 혼합 제목에서 일반/굵은 획·폭·기준선·줄 높이·잘림을 확인한다. 영어 `.chfnt`와 일본어 Noto Sans JP를 함께 대조한다.

<a id="exe-icon"></a>

### 0.4 실행 파일 아이콘 — EXE/ZIP 적용 완료, 창 표시 확인 남음

AGENTS.md 지정에 따라0.807에서 공통 [assets/Netstorm.ico](assets/Netstorm.ico)(1,078바이트, SHA-256 `bd4c18c5…f01e4f068`)를 Game.csproj ApplicationIcon으로 지정했다. 원본32×32/16×16의 두 원시 이미지가 개발/게시/ZIP EXE와 전수 일치한다. 빌드는 원본 EXE를 읽지 않는다. [0.807](docs/dotnet-canon-0807-20261122.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

- [x] 0.807에서 공통 ICO를 재사용하고 Game.csproj ApplicationIcon과 실제 EXE의 두 원시 이미지/그룹을 전수 대조했다. 해시는0.807 보고서에 기록했다.
- [x] 0.807에서 개발/게시 EXE와 최종 ZIP의 EXE가 전체 바이트 같고 동일 아이콘/11.01 버전임을 검증했다.
- [ ] DesktopGL 창·작업 표시줄의 추가 아이콘 설정 필요 여부를 확인한다. 탐색기 사람 확인은 사용자에게 남긴다.

<a id="dotnet-stage-board"></a><a id="stage-criteria"></a>

## 1. 단계별 작업과 완료 기준

| 단계 | 현행 상태와 완료 기준 |
| --- | --- |
| R0 기준선·동기화 | 완료. cpppj 기준선·PORT_MAP·`cpp_sync.py`. |
| R1 화면 호스트·입력 큐 | 완료. 8비트 호스트·입력 큐·비교 도구. 실제 월드 입력 소비는 R6. |
| R2 그리기·팔레트·글꼴 | 원시 본체 완료. 실자산 어댑터·수집/정렬·종류별 분배·`SpriteDraw`/그림자 공유와 `owner-capture` C행은 R4/R5에서 연결. |
| R3 Gump UI·메뉴 → D1 | 메뉴 L4a 12장면 일치. 메뉴 상위 소유자·모달·등록·대화창·텍스트 입력과 하위 화면 연결 잔여. 미룬 장면은 Language 자식 CJK(R8), Multiplayer 섬(R5/R11), Edit 이동행(R7). [UI 잔여](#r3-r2-detail) |
| **R4 월드 세션 조립 → M1** | **R4-2·R4-4·R4-5·R4-6 병행 진행 중**(0.689 이후는 주로 R4-6). 34경로(Early 6·Training 9·캠페인 16·데모 3)의 같은 시드 Load/Start `session-dump`가 cpppj와 일치해야 한다(L2). [R4 체크](#r4-detail) |
| R5 월드 화면·카메라·HUD → D2 | 렌더러 수집·정렬·dirty/팔레트, 타입별 Draw와 효과, 카메라·전체화면 가장자리 스크롤, HUD·덱·미니맵·인벤토리. 진입 직후와 N프레임 뒤34경로 L4a 차이0 및 압축 해제 실행을 대조한다. D2의 Game/Netstorm.exe 이름·배포 승계는 사용자 지시로0.807 완료했다. |
| R6 플레이·규칙·AI·음향 → M2/M3 | Tutorial1 → Early Missions → 1-1·1-2 순서로 입력·커서·선택·메뉴, 사제/경로/운반/수확, 건설/생산/경제/다리, 전투, 의식/승패, AI, 소리/음악/동영상, 채팅/경고를 연결한다. M2는 고정 입력열 L3와 대표 캡처 일치, M3는 1-1·1-2가 기존 경로 수준 이상으로 진행되는 것. 구 근사 실행기와 전용 자동 실행 검사는0.807에서 제거했고 공통/L1 부품 검사는 유지한다. [선행 완료와 잔여](#r6-preports) |
| R7 미션 전체·Edit·설정·배포 → M4 | 34경로 진입·진행·저장, Retry/Next/Abort, 데모 2주기, Edit 왕복 저장·Save as·Test Battle/복귀, `options.cfg` 시점·전체화면 재실행, A~F 중 C·D·F 판정. ZIP 전수 대조와 압축 해제 실행, `Netstorm.exe` 외 실행 파일 사용법 문서화. 배포본 `Netstorm.exe`의 아이콘이 원본 실행 파일 아이콘과 같은지 확인한다(AGENTS 2026-11-19 추가, [0.4](#exe-icon)). |
| R8 AGENTS 4~15차 | 아래 차수별 요구에 도달할 때마다 기능을 검증한다. 공통 파일을 옮기는 과정에서 이미 들어온 부분도 실제 경로에서 확인한다. [요구사항](#dotnet-r8-requirements) |
| R9 60/120 FPS | Frame Rate 60/75(원본)/120과 양보 대기의 프레임 시간 대조. 비행체 표시 보간은 cpppj D2 결정에 따른다. |
| R10 Linux | DesktopGL와 파일 장치의 경로·권한·원자 저장을 확인한다. `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`를 사용한다. |
| R11 로컬 멀티플레이·Outpost | TCP/IP 로컬만 지원한다. `Netstorm.Net` 일부 사전 구현. 루트 서버 독립 모듈은 로컬 실행에 필요한 범위를 먼저 판정하고 실동작은 사용자 확인으로 둔다. |

<a id="r4-detail"></a>

### 1.1 R4 세션 조립 체크 (1/8 완료)

단위별 완료 문장은 [0.797 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)·[0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)·[0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)과 각 단위 문서에 있다. 아래에는 하위 단계별 완료 범위와 남은 일만 둔다. 통합 조각을 마칠 때 같은 조건의 일괄 덤프로 M1 진척을 확인한다.

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
  - 남음: Kernel/Process 초기 등록과 초기 덤프 비교, Dais 정기(본체·세션 분배)와 Dude·Disc 세션 분배, AI 전체 행동의 실제 매 프레임 호출·장식 재생·연소 등 나머지 틱 난수원을 잇는다. 정기 부착 시점(0.631에서 분해한 RNG 25,611 격차의 틱 구간)도 맞춘다. Dude 정기 본체는 0.883에서 연결했다.
  - 다른 단계로 넘긴 것: 거주 구역 사건 2(주민 생성), 펜스 이웃 새로고침(이벤트 1), 장식 가상98 전체 몸체·삭제 송신, Player 화면 로그·DataManager 통지는 R6이다. 펜스 표식·연결선 삭제의 송신·통지 일치는 R4-6에서 대조한다.
- [ ] **R4-5 AI 초기화.**
  - 완료(생성·본체): 생성·초기 갱신·해제·동맹 난이도(0.639~0.641), 준비·전략·전체 프레임·통지·월드 등록 본체와 장치 조립·세션 결선(0.645~0.650), 지역 경계·분산 SP·자금·지도 상한(0.653~0.654), 그래프 조회·경제 본체·신전 생산 TYPE(0.657~0.658), 경제 세션 연결(0.662). [최근 경제 결선](docs/dotnet-canon-0662-20261120.md)
  - 완료(하위 행동 본체): 수집자(0.655)·배치 선행(0.661)·시설 생산(0.663, 세션 0.665) 본체, 업그레이드 요청·수신·사건3 완료(0.666~0.667), 실제 업그레이드/연구·생산 Deck·내용물 통지의 세션 연결(0.682~0.686), 공유 분배기·58h 등록·연구 운반(0.687).
  - 남음: 제품 양끝 요청 전송, 수집자·시설과 나머지 하위 행동의 세션 호출자, 신전 삭제, `SetAiParameter`·`CrankAiMoney`의 실제 호출자, 매 프레임 실행과 258 완료·brain·SP·Player L2 대조.
- [ ] **R4-6 내부 로그인·전송·Client 전역.** 메뉴 핸들 출처와 이름 꼬리 16~19바이트, 로컬 Login→Ready→Start 실제 패킷, 요새 이미지·`Bind*`·State 수명을 연결한다. 실제 handshake 검사와 메뉴/`--mission`의 진입 덤프를 대조한다.
  - 완료(0.689~0.844, 묶음별 범위·핵심은 [0.1 요약표](#now)): 로그인·전송·Client 전역(0.689~0.715), Runtime·Player 장기 수명(0.706~0.719), Client 프레임·장기 Host(0.721~0.727), Native State 정리·저장소·반납(0.729~0.735·0.813~0.814), Root 자원·장기 자산표·선택(0.737~0.747), 커서·카메라·Renderer·Host 표시(0.749~0.755), Chat 계열(0.757~0.777), Root 수신·전송 관찰기(0.778~0.787), 객체 패킷 수신·Host entry(0.789~0.811), 경로 Runtime·Data·본체·탐색과 건설 환불·정지/도착 부모(0.793~0.801), HUD 도킹·전체 부모/공통 조립(0.817~0.822), NSInfo·생산 패널·색표(0.823~0.837), 파일 IO·검색·CRT(0.831~0.834), 기본 Host 지연/동일 자원 예약·Native 수명/정기35종·Dais/Path writer·현재 Runtime ProcessForm 삭제(0.841~0.844, 전체 가상 자산은 미완료). 단위별 완료 문장은 [0.797 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)·[0.837 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)과 단위 문서([0.747](docs/dotnet-canon-0747-20261121.md) · [0.787](docs/dotnet-canon-0787-20261121.md) · [0.819](docs/dotnet-canon-0819-20261122.md) · [0.837](docs/dotnet-canon-0837-20261123.md))에 있다.
  - 남음(전송·수신, 근거 `cpppj/src/client/ClientCampaignTransport.cpp`): 전체 `NativeGameplay26F` 26항목 분배는 0.882 검사로 보존하고 Host 공급·Install/Bind까지 연결했다. 남은 것은 공통 41h가 `Root.ReceiveNativePacketChat`을 부르는 전송 설치, 전체 InstallClientPacketDefaults/MakeClientCampaignTransport 부모와 Carrier/Construction/Meta 고정 F 설치, Root의 실제 Source/Record/Sent·송신/제출, 전체 435220 옵션 부모의 Requested만 0으로 재설정하는 호출(FILE/enabled를 닫는 동작으로 대신하지 않음), NativeLoginPacketReceivers의 실제 MonoGame Root 설치 호출, HostSession 실제 송신 부모(구 `Mission.RawCampaignSession`의 sendChatMutable 입구로 대신하지 않음), 나머지 순수 observer/장기 수신, 장기 CS/7bh의 나머지 Meta/UI/투영/배열 소비자와 Root 생성 부모.
  - 남음(Root 장치·UI): Root의 언어/이름/ScreenSize/FormPacket/수신 장치 공급, NativeRoot/HUD 창 생성·Enter와 알림 등록 ID의 실제 HUD 작성자, NSInfo HUD/메타 UI/Root reply 저장, 장면음악 장치·UI/음향/Pool·전체 State 설치, 0.838의 현재 지도 getter를 재사용하는 실제 HUD 도킹, Native 색표 7 getter/zone 옵션 변경 부모, 세션 자산 InBattle 최초 상태, Chat 실제 명령 효과.
  - 남음(월드·경로): Cleanup의 실제 상위 설치·예약/의식/6ch·전체 가상 삭제 장치, 제품 Root의 전체 Path/Start/Motion 설치(본체/API/Host·30h는0.801 완료), MetaZone의 전체 Native World 자산 가상 삭제 분배 설치(원문 본체/Client표/pin 경계는 0.734~0.735 완료, 일반 삭제 Pre/Post로 대신하지 않음), World/건설 효과 pin 전달.
  - 남음(표시·입력): Native Frame/Root UI 수집·그리기와 같은 지도 owner의 실제 HUD 작성자, 전체 커서 효과/Drop/Previews/Position 호출자, SavedCameraKey/무효점·CameraCommit/Present/Pan(0.838의 같은 현재 지도 getter를 재사용하고 실제 HUD 도킹이 그 저장소를 작성).
  - 남음(시작·수명): 전체 로그인/제품 Root 등록과 제품 Clock 프레임 설치, 전체 Native 시작의 intro/장면음악/나머지 지도 초기화와 Guide Init, 나머지 음향/저장/옵션/전체 EnergySupply, non-mine Notify의 실제 DataManager/추출 부모, Source의 Editor/LAN/Meta 재인계, 원본 큰 메뉴 부모 3행(0.707), 앞선 Native 초기화 주소 세대와 전체 bridge(0.714), Native WorldRuntime 장치 일원화, 미션 없는 장기 Surface/주변음·영화 갱신 잠금과 나머지 루프 단계(0.723), 메뉴 Kernel/효과·전체 State/자산 재시작(0.725), RawFileName 나머지 조회·BaseFile 고리·ClientStaticStrings의 소비자.
- [ ] **R4-7 데모와 보류 검사.** `StartDemo` 3경로 덤프, `player-world-x86.tsv` 1,680행, F3에서 미룬 Fort/지형 바인딩·미션 Fort 경로·로그인 객체 전송·미션 재적재 검사를 잇는다.
- [ ] **R4-8 M1 판정.** 같은 시드·고정 프레임·빈 사용자 루트로 34/34 상태 일치. 로드 뒤 프레임 진행에서만 생기는 차이는 cpp 후속 프레임 덤프로 입증한 뒤 R6으로 넘긴다.

<a id="r6-preports"></a>

### 1.2 후속 단계에서 이미 이식한 범위

- [x] **R6-3 배치 본체(0.609~0.611).** 권한·관계, 지형 메이커 2종, Player 앵커·서라운딩·파이프라인을 이식했고 MayPlace 3건 전수 일치를 확인했다. 실제 농지 호출은 R4-2(0.614)에서 결선했다. 사용자 건설 입력 통합은 R6-3에서 검증한다. [0.611](docs/dotnet-canon-0611-20261118.md)
- [x] **펜스 본체 8종(0.587~0.592).** `RawFenceLifetime`·`Neighbors`·`Line`·`MarkerClear`·`Rebuild`·`Create`·`PostPop`·`Regular`과 해당 저장소 갈고리를 이식했다. 세션 Pop·정기 결선은 0.636에서 마감했다. 표식·포스 필드 생성은 0.644에서 연결했다([0.644](docs/dotnet-canon-0644-20261119.md)). R6의 타깃·타입 부모·Damageable 정기 라우팅과 연결선 생성·이웃 새로고침은 열려 있다. [펜스 후처리와 정기 처리](docs/dotnet-surface-fence-postpop-20261117.md)
- [x] **시설 업그레이드 본체(0.666~0.667).** 요청·수신·공통58h writer와 사건3 그림/완료·실제 Host 정리·현재 AI 통지 본체를 이식했다. 가상/건설/표시/음향과 요청 API는0.682~0.683, 연구 내용물/Deck·부모 통지는0.686에서 실제 세션에 연결했다. 공유 분배기·58h 등록·연구 운반은 0.687에서 연결했다([0.687](docs/dotnet-canon-0687-20261120.md)). 제품 요청 전송과 UI, 세 비행장 홈 통지·게임 내 HUD 메시지(0.682)는 후속이다. [요청](docs/dotnet-canon-0666-20261120.md) · [그림/완료](docs/dotnet-canon-0667-20261120.md)
- [x] **플레어·건설 선행 본체(0.669~0.671).** 플레어 생성/정기, 건설 진행/완료/실제Host 수명, 밀어내기/실패 섬 정리를 원본 관측으로 대조했다. 실제 건설 표시·낙하·음향·전송·배치 통지·세션 장치는 후속이다. [플레어](docs/dotnet-canon-0669-20261120.md) · [건설 진행](docs/dotnet-canon-0670-20261120.md) · [건설 효과](docs/dotnet-canon-0671-20261120.md)
- [x] **건설 표시 본체·프로세스 수명(0.673~0.675).** 생성/동기화/입자 선택1,776행, 프레임/Draw1,317행과 실제10.78 Kernel/Form/공통 목록 수명12건을 확인했다. 제품의 실제 캡처·팔레트·출력/음향·세션 장치는 후속이다. [생성](docs/dotnet-canon-0673-20261120.md) · [프레임](docs/dotnet-canon-0674-20261120.md) · [수명](docs/dotnet-canon-0675-20261120.md)
- [x] **건설·플레어 실제 장치(0.677~0.681).** 공장 가상 그림·실제 SHP 캡처(0.677), 같은 WorldRendering/Host/Kernel·COL 팔레트·현재 카메라/dirty/8비트 출력의 제품 건설 표시 장치(0.678), 플레어의 실제 장치·정기·음향(0.679), 실제 HP/시계/권한·진행음/완료음·공간 재등록/밀어내기의 건설 진행/완료 장치(0.681)를 연결했다. private 효과의 DropIsland 낙하/전송, 실제 건설 요청, Carrier 플레이 효과, 나머지 캡처·전용 색표·R5 월드 뷰는 후속이다. [0.678](docs/dotnet-canon-0678-20261120.md) · [0.681](docs/dotnet-canon-0681-20261120.md)
- [x] **간헐천 수명과 벌룬 본체(0.583~0.584).** 세션 Pop·사건 0 정기는 0.634에서 결선했다. 채굴 `RawGeyserMined`은0.803에서독립2312관측/Host5bh를연결했다. Root의전체음향/가상삭제와운반상위는남았다. `RawBalloonFrame`은0.795의Path본체에포함되며제품Root설치가남았다. AI 등록은 R4-5에서 잇는다. [간헐천](docs/dotnet-surface-geyser-20261117.md) · [벌룬](docs/dotnet-surface-balloon-20261117.md)
- [x] **Outpost 전처리 본체(0.582).** 장기 삭제 분배·목록 압축·부모 호출을 집중 검사했다. 전체 Outpost 수명과 LAN 통합은 R11에서 이어간다. [Outpost](docs/dotnet-surface-outpost-20261117.md)
- [ ] **공통 장치의 실제 미션 연결.** `RawSurfaceAssetDeletion/Notice`·`Content/Reward`·`Lifetime/Deletion`·`PriestDeletion`·`Damageable/Fence/Balloon`은 대응 본체 또는 분배 장치가 있다. 큰 파일의 미이식 분기와 호출자 장치는 R4-2~R6에서 확인한다. `RawIceTower`의 성장 정기·완성형 파괴·재생성/삭제와 Native 설치도 남아 있다. [R4-1 감사](docs/dotnet-r4-1-close-20261117.md)

<a id="r3-r2-detail"></a><a id="carried-limits"></a>

### 1.3 앞 단계에서 이월한 작업

- [ ] **R3 UI.** `ParseDialogPage`·`ConfigOverwriteAndQueueLifetime` 잔여와 MessageGump, 맵/파일/기술 목록·Guide·모달·명령 소유자, QuickHelp TYPE/AST, Console/클립보드/Save as/숫자 창, ChatWindow의 NativeRoot 창 생성·Enter(상태·기하·전체 부모·공장은 0.767~0.774 완료), 배경 GIF 노드·창, 하위 화면 라벨과 실제 명령 소비. HTML/스타일 그림의 실제 자산/본문·메뉴 출력은0.773 완료이며 Native 색표7/옵션 변경 부모는 후속이다.
- [ ] **R2 화면 통합.** 실자산·그리기 결선과 구름 카메라·시간 이동·dirty 갱신·구름 파일 fallback(R5). 빠른 물리 입력 이력과 옵션 변경 중 소리 수명은 R6.
- [ ] **파일·호스트 경계.** `--set` 중복 처리 차이, `MatchDisk` 절대/긴 경로, Linux `WriteAtomic`·`ReadOnly`, 이름 정규화·옛 인코딩, Fort 이름/Template 소유자·Save as는 해당 소비자를 옮길 때 대조한다. CD 슬롯 폭·경계의 하네스와 기존 단위의 예외·할당·복사 경계는 보관본 2.6과 단위 문서를 참조한다.

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a><a id="04-기존-dotnetpj-소스의-재수정-판정"></a><a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a><a id="dotnet-user-decisions"></a>

## 2. 계속 유효한 결정

- **원본 10.78 경계 보존.** 화면은 8비트 인덱스 버퍼·256색 팔레트, MonoGame은 창·입력·오디오 장치·최종 출력이다. cpppj의 파일/클래스·호출 순서·부작용·정수 폭·float32 저장·목록·난수·PATCH 50바이트 슬롯 배치를 보존한다. 플랫폼 장치는 .NET/MonoGame으로 대체하고 OS 전용 동작은 호스트에 둔다. cpppj 미복원·보류 경계는 명시 진단으로 드러낸다. CD 10.72 제품 복원은 cpppj 범위다.
- **D1(사용자 결정).** cpppj HEAD의 요구사항 코드가 공통 파일에 섞여 있으면 함께 옮긴다. 기본 영어·보통·75 FPS·4:3 창 모드에서 원본 동작을 먼저 대조하고 후순위 기능은 R8에서 검증한다. 파일 장치는 사용자 루트 정책을 초기 이식부터 적용한다.
- **D2(사용자 결정,0.807 적용 완료).** 2026-11-22 사용자가 즉시 전환을 지시해 구 실행기를 제거하고 직역 호스트가 Game/Netstorm.exe를 승계했다. 기존 settings.json은 가져오지 않으며 홈의 기존 저장값을 삭제하지 않았다. 전용 자동 실행 검사16개는 은퇴시켰고 공통 부품/L1 검사는 유지한다. R5 전환/R6 삭제라는 이전 에이전트 해석은 폐기한다.
- **D3~D5(사용자 결정).** R8에서 로컬 MCP와 한국어·일본어 화면을 검증한다. Linux는 `$HOME/.netstorm/` 아래에 저장한다. Template 판독·433 Fort 왕복 검사의 기존 결정은 [Template 계획](docs/dotnet-template-storage-plan-20261101.md)에 있다.
- **사용자 지시.** 정기 전체 회귀는 4단위당 1회만 수행한다(2026-11-19). 포획/희생·특정 적 공격·승리 재현은2026-11-22 지시로캠페인1-1~1-5·스커미시에서만아군AI 활성화로허용하며나머지는사용자수동확인으로둔다.

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

현행 AGENTS 변경은 갈무리11 지정 `2c8ca3bf8`, 15차 추가 `faecb9cbd`, 실행 파일 아이콘 `d047bf9a3`([0.4](#exe-icon)), webpj favicon `11b16b778`(DN 해당 없음)까지 확인했다(2026-11-23 재확인, 이후 변경 없음). 아래는 새 경로 기준이다.

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
| **15 Reset All** | **미착수.** Options 맨 아래에 Reset All을 제공한다. 취소 시 현재 설정·진행을 보존하고 확인 시 `options.cfg`를 포함한 설정·게임 진행을 초기화한 뒤 종료한다. 다시 실행해 최초 상태를 검증한다. cpppj1325에서 확인/취소·초기화·종료와 최초 상태 복원을 구현했다(2026-11-22). DN은 해당 Source를 옮길 때 대조한다. [AGENTS.md](AGENTS.md) |

**공통 완료 사항.** 기존 배포의 `Netstorm.exe` 이름과 압축 스크립트, 11.01·Credits 3항목·HJOW/AI 크레딧 및 카피라이트(0.150·0.342), 폐기 클라이언트 유효성 검사 비활성·외부 접속 문서화(0.151). 0.807에서 직역 호스트가 `Netstorm.exe` 이름을 승계하고 원본 아이콘을 EXE/ZIP에 적용했다(창 표시 확인은 [0.4](#exe-icon)). 새 소비자가 생기면 외부 접속 목록을 갱신한다. [배포 스크립트](dotnetpj/package-dotnetpj.ps1) · [외부 접속](docs/dotnet-external-endpoints-20261103.md)

<a id="resync-scope"></a><a id="cpppj-requests"></a><a id="dotnet-inventory"></a>

## 4. cpppj 동기화와 이식 목록

- **cpppj1326이 현행 제품(1327 검증 관문).** 1325의Reset All과1326의테스트AI 피해문맥이 추가됐으며 해당 소비자 이식 시 재대조한다. DN 제품 기준 동기화는 아직1323이다. 0.606의 오라클 재빌드 기준은 1322다(1323은 아이콘 리소스만 추가). 마지막으로 문서화한 변경 분류는 0.530의 1312·1313이다. `979d65b5d..HEAD`의 `cpppj/src` 변경 38파일 중 PORT_MAP과 겹치는 것은 20파일이며, 그중 7파일은 이미 변경 뒤 커밋을 base로 재대조했다(2026-11-23 0.837 정리 때 행별 `git merge-base --is-ancestor`로 재확인). base가 변경보다 앞선 13파일은 `BitmapFont.cpp/.h`(1322 글꼴), `LocalTcpTransport.cpp`·`LocalUdpTransport.cpp`(1320 미션 전환), `ClientMain.cpp/.h`·`ProductPresentation.cpp`·`UberGump.cpp`(1325 Reset All, `ClientMain.cpp`의 일부 행은 1322부터), `RawCampaignSession.cpp/.h`·`SessionStateDump.cpp/.h`·`o/RawDamageableHit.cpp`(1326 테스트 AI 진단)다. 해당 소비자를 옮길 때 재대조를 마친 뒤 PORT_MAP 기준 커밋을 올린다. 1314(최초 실행)는 0.2, 1322는 0.3, 1325는 3절 15차와 함께 반영한다(1323 아이콘은 0.807에서 반영). [1312·1313 처분](docs/dotnet-ai-alliance-delta-20261115.md) · [1320](docs/exe/cpp-mission-transition-1320-20261117.md) · [cpppj 현행](LEFT_JOBS.md)
- **기존 재대조 규칙.** CPP 소스는 커밋된 HEAD를 사용한다. 해당 소비자를 옮길 때 바뀐 cpp 소스·fixture·검사를 함께 확인하고, cpppj 테스트 자동 플레이와 관전 경계는 R5/R6에서 잇는다. [오라클 자동 플레이](docs/testing/cpp-autoplay-test-guide.md)
- **현재 집계(2026-11-23, 0.883 뒤).** PORT_MAP 물리1803줄(자료1798행, port1310·L1 481·session7), 연결fixture492개다. 부분대응 행·줄 집계는 다음 전체 회귀(0.884)에서 확정하며, 부분이식을포함하므로전체분기·Host결선/플레이완료를뜻하지않는다.0.883은기존PORT1794행순서·값/base·fixture491개주석/값/해시를보존하고선정4쌍·fixture1행을추가했다.
- **파일 계열 목록.** 현행 [PORT_MAP](dotnetpj/PORT_MAP.tsv)과 `python -X utf8 tools/port_inventory.py --families dn`을 사용한다. 예전 단계 배정 및 대응 파일의 미완 부분은 [보관본 2.4~2.7](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에서 찾는다. 단위 작업은 해당 계열이나 큰 파일 하나를 옮기고 검사·호스트 결선까지 확인한다.
- **cpppj 전달 항목.** 상태 덤프와 고정 시각 요청은 해결됐다. fixture 헤더·모듈 연결 상태표, `GraphTests` 슬롯 열 범위와 `ClearProcesses` 뒤 정기 조회 진단의 이전 요청은 보관본 1.4에 있다. 변경이 필요하면 사용자에게 전달한다.

<a id="verify-levels"></a><a id="user-checks"></a><a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a><a id="dotnet-commands"></a><a id="dotnet-ops"></a>

## 5. 검증과 작업 규칙

- **L1** 원본 x86 fixture, **L2** 시작 상태 덤프, **L3** 고정 시계·입력열 뒤 상태 해시, **L4a** 논리 화면 픽셀, **L4b** 소리 시점·곡 선택, **L5** 자연 완주·저장·재실행. 파일 이식·집중 검사·실제 세션 결선·전체 일치를 각각 기록한다. `snapshot`과 `snapshot-output`을 구별하고 실행 근거에 SHA·시드·고정 시각·프레임·스크립트·옵션을 남긴다.
- **사용자 확인.** L5와 전체 화면/청취·MP/LAN·공격형 공중3종 실제 생산/조작은 사용자 확인 범위다. 포획·희생·특정 적 공격·승리는 캠페인1-1~1-5·스커미시에 한해 아군AI 활성화로 테스트할 수 있고 다른 미션은 사용자 확인으로 남긴다(2026-11-22). 창 실행은 AGENTS.md의 명시 확인·예외 PC 규칙을 따른다. Windows 10에서 통과하면 Windows 11 실기와 사용자 관찰만 남은 상태에서도 다음 단계로 진행한다. 물리 DPI·4K FSR·UAC는 R8 이후, `DSERR_NODRIVER` 실장치 재검사는 출력 장치가 있는 PC에서 한다. [소리 장치 기록](docs/dotnet-sound-device-20261114.md) · [공통 실행 범위](LEFT_JOBS.md#rules-verify)
- **공통 기능 경계.** CHEAT 메뉴는 Edit에서만 표시한다. 연녹색 직사각형 분석은 보류다. 폐기된 인터넷 master/root·검증·ping/진단은 비활성 상태를 유지한다. 도움말 링크와 LAN은 해당 사용자가 선택한 경로를 따른다. [공통 계약](LEFT_JOBS.md#rules-verify)
- **오라클과 자료.** `cpppj/build/bin/Release/Netstorm.exe`를 PowerShell에서 빌드한다. 실행은 `extracted/dotnet-oracle-data/` 사본과 분리한 사용자 루트로 진행하며 `originals/` 해시를 전후 확인한다. 0.606의 VM 소프트웨어 GPU 환경은 [재측정 기록](docs/dotnet-l2-0606-20261117.md)에 있다. 일반 덤프 비교와 최초 실행 검증의 옵션을 구별한다.
- **검사 명령.** `dotnet build dotnetpj/Netstorm.sln -c Release`, `dotnet test dotnetpj/Netstorm.sln -c Release`, `python -X utf8 dotnetpj/tools/cpp_sync.py`를 쓴다. 실제 TYPE 검사는 기본 데이터 또는 `NETSTORM_DATA`로 선택한 보호 자료 사본을 읽는다. 2026-11-19 사용자 지시로 정기 전체 회귀는 4단위당 1회만 수행한다(0.880 완료 뒤 다음0.884). fixture는 사본 없이 한 소비자 프로젝트에 링크하고 `cpp_sync.py --update --add`로 매니페스트에 등록한다. GUI 명령·변이 검사·등록 규칙 전문은 보관본 6.1에 있다. [L2 도구](docs/dotnet-l2-batch-20261114.md) · [L4a 도구](dotnetpj/tools/l4a_compare.ps1) · [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md)
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
| 0.738~0.755 | 2026-11-21 | Root 자원·장기 자산표·선택, 커서 데이터·수명·위치, 카메라 snapshot, Renderer 소유자·출력과 실제 Host 표시 장치 |
| 0.757~0.787 | 2026-11-21 | Chat 계열(Subscriber·manager·명령표·제출·Identity·ChatWindow·공장·중계·41h 수신), Root UI 수신·패킷 기록·기본 관찰기·버전 응답·장기 CS/7bh |
| 0.789~0.797 | 2026-11-21 | 객체 패킷 수신 하위·Host 고정 본문 9개, 경로 Runtime·Data·Process·탐색/시작/정리 하위, 0.796 전체 검사·65시작 수집 |
| 0.798~0.803 | 2026-11-22 | 건설 환불·경로 정리 하위, 사제 정지/도착 Host 부모, 전체 Path/Start/Motion·Native 30h, 내용물·감시·삭제5종 수신, 간헐천 채집·5bh, 0.800 전체5,681회귀·65시작 감사 |
| 0.804~0.816 | 2026-11-22 | Host 부착 예외·독립 공장 진단, 31h 개발자 명령, 구 실행 경로 제거·Game/Netstorm.exe 승계(0.807), 섬 정리 파편·건물 캡처·35h, Player 고유 삭제·첫 장치 설치·현재 AI 지갑, 0.804·0.808·0.812·0.816 전체 회귀(마지막 5,817)·65시작 감사 |
| 0.817~0.824 | 2026-11-22 | HUD 도킹·전체 Uber 부모·구분선 입력/공통 HUD, 미니맵 기하/셀 그림·공통 미니맵 출력/통지 수명, NSInfo 생성·공통 UI 소유, 0.820·0.824 전체 회귀(5,848·5,879)·65시작 감사 |
| 0.825~0.840 | 2026-11-22~23 | 생산 격자/정보/동기화/부모·owner 색표·생산 문맥, 최초 ColorSort·캐시 부모, 파일 IO·같은 경로 검색·실제 CRT/TAFF·색표 캐시, 시작 색표·캐시·소유자 옵션, 생산 동기화·색표7 소비자와 중지 인계, 0.828·0.832·0.836 전체 회귀(마지막 5,942)·65진입 감사, 0.838 실제 Root 전송/Process 선택·현재 지도 getter, 0.839 Native Player 정기/현재 Template/Host, 0.840 전체5966/65진입 감사 |
| 0.841~0.844 | 2026-11-23 | 기본 Host 장치 지연 설치/Native 공장·동일 자원 예약(146검사), 같은 Native 기본 수명·현재 VT35종/피해·건설 사건(새37 포함198검사), 기본 Dais/Path writer·실제 PID/FormPacket(새7 포함233검사),0.844현재Runtime Form삭제/검사선택자교정·전체6018/65진입·원본/소스보존 |
| 0.845 | 2026-11-23 | 건설 상태 codec/수신·실제 PATCH Host/0x37 등록과 Native 기본 writer·현재 헤더, 독립3PE5340관측·새7포함251검사·대표3덤프/원본보존, CD Host 경계 명시 |
| 0.846 | 2026-11-23 | GunProcess 전체raw/본체/codec·실제flags0 Host/삭제와 Native 현재 writer·헤더/역할, 독립2996+904관측·새12포함263검사·대표3덤프/원본보존 |
| 0.847 | 2026-11-23 | 총3ah 수신·현재 Gun owner/실제 감시/체인/전송과 고정F 한행 설치·19/26본문, 새12포함396검사·대표3덤프/원본보존 |
| 0.848 | 2026-11-23 | 전체6049통과/음향1미실행·0.844이름/중복과신규31보존·같은빌드65종료0·DN31덤프/34지표0.844동일·원본1392/실행6/소스1945불변 |
| 0.849 | 2026-11-23 | 공통 FlyerProcess/상태codec·flags8 Host/typed삭제·Native 현재 writer, 독립1092관측·새10포함273검사·대표3덤프/원본보존·P329후속 |
| 0.850 | 2026-11-23 | 비행 후처리720관측·동일Flyer/Regular예약과 공통가상분배/깊이/반납, 새6포함426검사·대표3덤프/원본보존 |
| 0.851 | 2026-11-23 | 비행39h 수신/86h 기한명령·현재Host/Kernel/고정F두행과21/26본문, 새27포함453검사·대표3덤프/원본보존 |
| 0.852 | 2026-11-23 | 전체6092통과/음향1미실행·0.848이름/중복과신규43보존·같은빌드65종료0·DN31덤프/34지표0.848동일·원본1392/실행6/소스1956불변 |
| 0.853 | 2026-11-23 | 훨리베이스48BYTE/상태 기계/홈통지/38h writer·실제Host/Native writer, 독립1734+128관측·새10포함463검사·대표3덤프/원본보존 |
| 0.854 | 2026-11-23 | 훨리베이스38h 수신/현재Host/고정F한행과22/26본문, 새13포함476검사·대표3덤프/원본보존 |
| 0.855 | 2026-11-23 | 맨 오 워 풀48BYTE/상태기계/난수홈통지/73h writer·실제Host/Native writer, 독립1602+36관측·새10포함486검사·대표3덤프/원본보존 |
| 0.856 | 2026-11-23 | 전체6125통과/음향1미실행·0.852이름/중복과신규33보존·같은빌드65종료0·DN31덤프/34지표0.852동일·원본1392/실행6/소스1970불변 |
| 0.857 | 2026-11-23 | 맨 오 워 풀73h 수신/38·73교차기존PID/현재Host·고정F한행과23/26본문, 새15포함501검사·대표3덤프/원본보존 |
| 0.858 | 2026-11-23 | 데빌메이커80BYTE/생성·74h codec/전체packet사전검사, 독립1894관측·새6포함507검사·대표3덤프/원본보존·실제Process후속 |
| 0.859 | 2026-11-23 | 데빌메이커실제Frame/후처리/홈통지/FormDestroy/원주·Host/Native writer, 독립3039관측·새9포함516검사·대표3덤프/원본보존 |
| 0.860 | 2026-11-23 | 전체6155통과/음향1미실행·0.856이름/중복과신규30보존·같은빌드65종료0·DN31덤프/34지표0.856동일·원본1392/실행6/소스1981불변 |
| 0.861 | 2026-11-23 | 데빌메이커74h/실제104BYTE Flyer예약·현재SHP/속도공급·실제종료·고정24/26, 새18포함534검사·대표3덤프0.860동일·보호원본/실행/소스보존 |
| 0.862 | 2026-11-23 | 총애니메이션 공통codec/썬캐논본체·실제Host/Native writer·두PE2560관측, 좌표값복사교정뒤새13포함547검사·대표3동일/보호자료보존 |
| 0.863 | 2026-11-23 | 아처회전/각상태발사/기하helper·실제PATCH Host/Native writer·두PE1690관측, 새6포함553검사·대표3동일/보호자료보존 |
| 0.864 | 2026-11-23 | 전체6192통과/음향1미실행·0.860이름/중복과신규37보존·같은빌드65종료0·DN31덤프/34지표0.860동일·원본1392/실행6/소스1995불변 |
| 0.865 | 2026-11-23 | 캐논썬/썬더/반더발사·nullable빔생성wrapper·실제Host/Native writer·독립5951관측, 새10포함563검사·대표3동일/보호자료보존 |
| 0.866 | 2026-11-23 | 크로스보우발사/공유회전·실제Host/Native writer·독립2638관측, 새6포함569검사·대표3동일/보호자료보존 |
| 0.867 | 2026-11-23 | 아이스80BYTE/냉각Step·공통Fire재사용·실제Host/Native writer·독립3585관측, 새7포함576검사·대표3동일/보호자료보존 |
| 0.868 | 2026-11-23 | 전체6215통과/음향1미실행·0.864이름/중복과신규23보존·같은빌드65종료0·DN31덤프/34지표0.864동일·원본1392/실행6/소스2007불변 |
| 0.869 | 2026-11-23 | 미사일128/104BYTE 생성/Frame·실제Factory/Host/진단삭제, 두PE2184관측·새15포함591검사·대표3동일/보호자료보존 |
| 0.870 | 2026-11-23 | 실제포구/방향/사정선·파생투사체/충돌명령8962관측·새19포함610검사·대표3동일/보호자료보존 |
| 0.871 | 2026-11-23 | 아이스반사점/후보/공유난수·실제Finder/Reward·지역8BYTE1395관측·새6포함616검사·대표3동일/보호자료보존 |
| 0.872 | 2026-11-23 | 전체6255통과/음향1미실행·0.868이름/중복과신규40보존·같은빌드65종료0·DN31덤프/34지표0.868동일·원본1392/실행6/소스2022불변 |
| 0.873 | 2026-11-23 | 실제총피해/압축시각/DPS·미사일Hit/Refresh·두PE2058관측·새5포함621검사·대표3동일/보호자료보존 |
| 0.874 | 2026-11-23 | 반사탄수명/플로터1783추가관측·새4포함625검사·제품소스/PORT불변·대표3동일/보호자료보존 |
| 0.875 | 2026-11-23 | 디스크정기/실제방향자료306관측·같은Host/Kernel/Frame·새8포함633검사·대표3동일/보호자료보존 |
| 0.876 | 2026-11-23 | 전체6272통과/음향1미실행·0.872이름/중복과신규17보존·같은빌드65종료0·DN31덤프/34지표0.872동일·원본1392/실행6/소스2029불변 |
| 0.877 | 2026-11-23 | 현재TYPE애니메이션필드/Clone·Native디스크36VT/실제Kernel수명·새4포함655검사·대표3동일/보호자료보존 |
| 0.878 | 2026-11-23 | 실제표적Finder/사거리/관측각/사정선·현재Player/Gun.Frame3454관측·새8포함663검사·대표3동일/보호자료보존 |
| 0.879 | 2026-11-23 | 같은World/Host전투공장·현재역할/시계·첫새16포함679검사·대표3동일/보호자료보존·사용자요청일시중지 |
| 0.880 | 2026-11-23 | 전체6300통과/음향1미실행·0.876이름/중복과신규28보존·같은빌드65종료0·DN31덤프/34지표0.876동일·원본1392/실행6/소스2038불변 |
| 0.881 | 2026-11-23 | 전투애니메이션3b/3c수신·공통송신·새11포함Client11통과·대표3덤프/지표0.880동일·원본1392/실행6/소스2040불변 |
| 0.882 | 2026-11-23 | 전투공장공급·3b/3c고정entry·Install/Bind·전체26F분배·새18포함Client18통과·대표3덤프/지표0.880동일·원본1392/실행6/소스2043불변 |
| 0.883 | 2026-11-23 | Dude정기본체·원문1600행전수대조·새5포함Core5통과·대표3덤프/지표0.880동일·원본1392/실행6/소스2046불변 |

정기 전체 회귀·일괄 덤프는 0.644부터 4단위마다(0.644·0.652·0.656·0.660·0.664 … 0.728 … 0.800 … 0.844) 수행했다. 단위별 근거는 `docs/dotnet-*.md`와 Git 기록에 있다.

- **2026-11-23 이관(0.837).** 정리 직전 0.837 인계 전문을 보존했다. [0.837 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)
  - 옮긴 것: 0.1절 중지·재개 인계 문단의 0.837 세부 수치, 재개 순서 1번에 누적된 0.814~0.837의 단위별 "…는 0.8xx 완료다" 연쇄와 소견 파일 나열, 1.1절 R4-6의 단위별 완료 bullet 12개(0.689~0.819), 6절 단위 범위 표의 0.798~0.822 단위별 행, 6절 0.797 이관 기록의 세부 목록.
  - 줄인 것: 재개 순서 1번을 대상·선행 완료·주의·소견 하위 목록으로 나눴다. R4-6 완료는 0.1 요약표를 가리키는 묶음 한 줄로 합치고 남음 묶음은 그대로 두었다. 0.1 요약표에 실행 경로 전환(0.805·0.807)·NSInfo/생산 패널/색표·파일 IO 행을, 6절에 0.798~0.837 범위 네 행을 더했다.
  - 바로잡은 것(낡은 값): 3절 공통 완료 사항의 "원본 아이콘 미완료"(0.807에서 EXE/ZIP 적용, 창 표시 확인만 남음), 0.1 4번의 "마지막 전체65시작 0.832"(실제 0.836), 0.1 요약표 범위(0.834 → 0.837), 4절 cpppj 변경과 PORT_MAP 교집합(14 → 20파일, base 미갱신 5 → 13파일: 1325·1326 추가), PORT_MAP·fixture·이식 집계(1,506줄/417개/580단위 → 1,640줄/452개/624단위), 3절 AGENTS 재확인 날짜.
  - 다시 확인한 것: 0.2 최초 실행·0.3 글꼴 화면 적용·0.4 창 아이콘 확인·3절 15차 Reset All은 DN에서 여전히 미완료다. AGENTS.md 마지막 변경은 `11b16b778`이다. 체크 항목의 상태와 단계 분류는 바꾸지 않았다.
- **2026-11-21 이관(0.797).** 0.797 인계 전문을 보존하고 재개 순서·주제별 표·1.1절 R4-6 단위별 완료 bullet 29건을 묶음으로 줄였다. 그때 옮긴 것·줄인 것·바로잡은 것의 목록은 [0.837 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)의 6절에 있다. [0.797 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)
- **2026-11-21 이관(0.732).** 정리 직전 0.732 인계 전문을 보존하고 0.1절 단위별 완료 문장 88건을 주제별 요약표와 1.1절 완료 범위로 줄였다. 그때 옮긴 것·바로잡은 것·새로 넣은 것의 목록은 [0.797 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)의 6절에 있다. [0.732 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)
- **2026-11-19 이관.** 0.635 인계 전문을 보존하고 R4 체크의 단위별 문장 22건을 하위 목록으로 줄였다. [0.635 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)
- **2026-11-18 이관.** 0.613 인계 전문을 보존했다. 완료/중단 단위의 경과, 중복 체크, 이전 이식량·AGENTS 이력, 파일별 목록과 명령 예시는 보관본을 참조한다. [0.613 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)
- **이전 이관.** [0.545 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)과 보관 파일 머리의 날짜별 색인을 참조한다. 아래는 이전 문서에서 참조하는 앵커다. 같은 이름의 상세는 보관본에 있다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
