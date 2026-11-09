# webpj 이식 대응표와 기준선 재반영 — WB02-G

2026-11-06~07, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서1이다. cpppj 기준선 `1f00a25a`(제품985·표시11.01)의 소스를 **파일 단위**로 웹 대상·제외로 나눈 대응표 `webpj/PORT_MAP.tsv`를 만들고, 기존 웹 모듈(WB02~WB04-D5a)의 대응을 기록했다. 3.6절 표의 9곳은 기준선과 다시 비교해 판정했고, 기준선 뒤 cpppj 커밋(`a328ac1f`)도 단독 경로 영향을 판정했다.

코드·자산의 게임 동작은 바꾸지 않았다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 웹은 cpppj 소스를 근거로 하므로 디컴파일 산출물을 읽지 않았다(마지막 전체 정밀 디컴파일은 공통 인계 기준 HJOW-Athlon / 2026-11-03).

## 1. 파일

| 파일 | 역할 |
| --- | --- |
| `webpj/PORT_MAP.tsv` (신규) | 이식 대응표. 기준선 단위 984행, 열 `cpp·class·web·base·status·note` |
| `webpj/tools/port-map.mjs` (신규) | 대응표 생성(`--write`)·검사(기본)·기준선 뒤 cpppj 변경 보고·분류 사유 조회(`--why 이름`). 분류 규칙 `RULES`와 웹 전용 모듈 목록 `WEB_ONLY`를 가진다 |
| `webpj/tests/port-map.test.mjs` (신규) | 형식·규칙·상태 조건, 경계 분류 표본 29개, 기존 대응·3.6절 판정 기록, 기준선 완전성·웹 모듈 대응, `--write` 보존(5검사) |
| `webpj/tools/fixture-manifest.mjs` (수정) | cpppj 근거 소스의 재검토 기준을 "manifest basis와 대응표 base 중 뒤 커밋"으로 바꿨다. 판정한 변경을 매번 다시 보고하지 않는다 |
| `webpj/tests/fixture-manifest.test.mjs` (수정) | 적용 범위 검사에서 `PORT_MAP.tsv`(원본 대조 자료가 아님)를 뺐다 |
| `webpj/tests/fixtures/manifest.json` (계산 칸 갱신) | WB04-D5a 4항목의 `webCommit`을 `worktree`에서 `4d5b486a`로 기록. cpppjBasis는 그대로 |

## 2. 대응표 형식

한 행은 기준선 `cpppj/src/{o,client,zacket}`의 **단위**(같은 폴더·같은 이름의 `.cpp/.h/.inc`)다. 이름이 다른 `.inc`(예: `UberGump.cpp`가 포함하는 `UberEditor.inc`·`UberHud.inc`)는 분류가 다를 수 있으므로 따로 행을 둔다. `platform/`·`app/`은 원본에 없는 새 코드(콘솔·BMP·진입점)라 넣지 않았다.

| 열 | 관리 | 뜻 |
| --- | --- | --- |
| `cpp` | 도구 | 대표 파일. `.cpp`→`.h`→`.inc` 순서로 있는 것 |
| `class` | 도구 규칙 | `port`/`adapt`/`exclude-mp`/`exclude-edit`/`exclude-host`/`exclude-legacy`(2026-11-10 WB04-E246 추가). 대응표에서 손으로 바꾸지 않고 `RULES`와 사유를 고친 뒤 `--write`한다 |
| `web` | 사람 | 대응 웹 모듈(webpj 기준, `;` 구분) |
| `base` | 사람 | 웹과 대조·판정한 cpppj 커밋. 이식분은 판정한 기준선, `todo` 행은 기준선 뒤 변경을 판정했을 때만 그 커밋을 적는다. **이식 원본은 언제나 기준선 판**이며 base가 더 뒤라도 그 사이 변경은 ①(웹 결과 영향 없음)으로 판정된 것이다 |
| `status` | 사람 | `todo`→`partial`→`port`→`session`→`oracle`, 제외 분류는 `-` |
| `note` | 도구+사람 | `[규칙ID]` 접두(도구)와 남은 범위·3.6절 판정·경계 메모(사람) |

`partial`은 3.0절 상태에 이번에 더했다. WB03 UI처럼 원본 분석·스크린샷으로 먼저 만든 모듈이나 파일의 일부만 옮긴 모듈을 `port`(파일 단위 이식·검사 통과)와 구별하기 위해서다. 검사 조건: 제외 분류⇔`-`, `partial` 이상은 `web`·`base` 필수, `todo`는 `web` 없음, 웹 모듈은 실제 파일, base는 실제 커밋, 메모 접두는 현재 규칙 ID.

`--write`는 기준선 단위 목록과 규칙으로 행·분류·접두를 다시 만들며 `web`·`base`·`status`·사람 메모를 보존한다. 기준선을 바꾸면(`BASELINE` 상수와 LEFT_JOBS 3.6절) 새 단위가 `todo`로 들어오고 사라진 단위는 보고 후 빠진다.

## 3. 분류 규칙과 근거

근거는 각 cpppj 파일의 머리 주석, `src/CMakeLists.txt`·`o/CMakeLists.txt`의 등록 주석, 그리고 호출 위치다. 위에서부터 처음 맞는 규칙을 쓴다. `node webpj/tools/port-map.mjs --why 이름`으로 단위별 사유를 볼 수 있다.

| 규칙 | 분류 | 단위 | 근거 |
| --- | --- | --- | --- |
| mp-lan-tcp | exclude-mp | LocalTcpTransport | LAN TCP 프레임(982). 단독은 zlocal 양끝만 쓴다 |
| mp-saved-battle | exclude-mp | RawSavedStateRead·SavedStateFiles·RawStartupStateCheck | 일반 전투 저장상태(!state.dat)·재접속 질문. 웹은 저장상태 파일이 없다 |
| mp-outpost | exclude-mp | RawOutpostLifecycle | Outpost 전용. RawRegionOwnership의 outpost 분기에서만 포함 |
| mp-zsend | exclude-mp | HtmlZsend | `$Zsend` 87h 송신, WB02-E 미연결 결정 |
| edit-editor | exclude-edit | 이름에 `Editor`가 든 15단위 | UberEditor·섬 추가 창·작업본 파일·Test Battle·편집 복귀 등 |
| edit-island-add | exclude-edit | RawIslandAdd·RawIslandPlacement | 편집기 섬 추가 |
| edit-fort-save | exclude-edit | Fort Save/Write·snapshot 16단위 | Save as와 snapshot writer. Battle snapshot·CoreData 기록은 일반 전투와 공유 |
| edit-existing-fort | exclude-edit | TypeNames/Contents/Deck/DeckRestore/TerritoryRecover/Subscriber 계열·FortTypeConversionStorage 9단위 | RawCampaignSession의 `ReadExistingFortDeck` 등 "기존Editor" 경로. 미션 적재는 RawCampaignWorldLoad/RawFortWorldLoad가 맡는다 |
| edit-new-fort | exclude-edit | RawNewFort·RawCampaignNewFort·NewFortManagerDevices | 새 맵 manager·보호 작업본. 987 뒤로 원격 Fort도 같은 생성을 쓴다 |
| edit-fort-name | exclude-edit | RawFortCurrentName·FortCurrentNameDevices | Editor 현재 Fort 이름·원격 Fort 경로 |
| edit-map-list | exclude-edit | FortSelection·SoloMapCatalog·RawSoloMapList·TellMapMenus | 편집/커스텀 맵 목록(WB03-I 제외) |
| host-cd / host-gpu / host-display-mode / host-tech-help / host-memory / host-win32 | exclude-host | CD 탐색4·D3D11/FSR2·모드 전환3·기술 도움말2·new_handler2·Win32 3 | 3절 웹 제외 목록(설치/CD·해상도/전체화면·Direct3D 11/FSR·Win32) |
| host-dev-files | exclude-host | F7 객체 덤프·procSumm·DataManager 덤프·CreateHelpList 6단위 | 개발자 진단 파일 출력. 화면 진단(RendererDiagnostics·trace)은 port로 남겼다 |
| host-capture | exclude-host | 연속 화면 BMP 저장 5단위 | Capture 명령의 파일 출력 |
| legacy-spatial | exclude-legacy | SquidSpatial 1단위 | (2026-11-10 WB04-E246) raw 풀 이전의 옛 `std::map` 공간 등록 모델. cpppj production 소비자가 없고(CMake 등록과 `SpatialTests`·`PopTests` 마지막 시험뿐) 같은 갱신을 SquidPop/SquidUnpop(raw 풀)이 대체해 웹은 raw 구현을 이식했다. 웹은 옛 fixture `spatial-x86`를 대조에 쓰지 않는다 |
| adapt-screen / adapt-sound / adapt-file / adapt-options / adapt-main / adapt-external / adapt-screenshot / adapt-log | adapt | 44단위 | 화면 장치·커서·OS 입력 폴링·창 제목·클립보드, DirectSound·Smacker, FILE/VFS·파일 읽기·캐시, options.cfg, WinMain, URL 실행, 화면 저장, trace 로그 |
| zacket-default / o-default / client-default | port | 나머지 851단위 | 원본 공용 규칙(`\Ns\O\`)·클라이언트·단독 내부 전송 |

### 3.1 단독 경로 판정(이름은 멀티플레이 같지만 port)

LEFT_JOBS 1절 경계("원본 싱글플레이도 같은 프로세스의 내부 미션 전송·패킷·로그인으로 진행")에 따라 다음은 `port`다.

- **복합 전송:** `UberGump.cpp:852`가 단독 미션에도 `MakeClientCampaignTransport(…, CompositeTransportRole::InternalMission, …)`를 `BindConnectionTransport`로 바인딩한다. 그래서 `CompositeMissionTransport`·`ClientCampaignTransport`·`CampaignTransportServices`(CampaignLoginServices가 포함)·`RawTransportPolling`은 이식 대상이며, TCP 원격끝 분기만 명시 웹 제외 오류로 둔다.
- **로그인/패킷:** `RawMissionLogin`·`RawMissionLoginPackets`·`RawMissionLoginTimer`·`RawMissionClientPackets`·`RawMissionServerPackets`·`RawMissionSettingsPacket`·`PacketRegistrationTable`·`RawPacketDispatch`·`RawPacketLengths`·`RawConnection*`·`zacket/LocalMissionTransport`. `RawFortImage`는 로그인 sendFort(0x2a)의 SaveImage 직렬화라 port다.
- **State 정리:** `RawMetaZoneCleanup`·`RawClockSyncReset`·`RawStateZoneReset`은 단독 State 정리에서도 부른다.
- **HUD Chat/Console:** `ChatWindow`·`RawChat*`·`ClientChat*`·`RawChatManagerPrefix`는 cpppj 비멀티 UI 범위(929)에 든다. 원격 송수신 분기는 경계 규칙으로 옮긴다.
- **기타:** `RawBattleLeave`(단독 LeaveBattle), `SendMoneyInput`(캠페인 AI 동맹), `RawPlayerMenu`, `ClientNsInfoGlobals`.
- **확인 보류 메모:** `RawRitualChoice`·`RitualChoiceInput`은 CMake 주석상 MP 사전 구현이지만 의식 보상 선택이 단독에서도 도달할 수 있어 port로 두고 WB10에서 확인한다. `RawFactoryPostPop`의 아웃포스트 분기, `RawFortSnapshotParent`(edit-fort-save)의 단독 의식 뒤 호출(RawRitualRefresh) 여부도 해당 단위에서 확인한다.

### 3.2 집계

| 분류 | o | client | zacket | 계 | 상태 |
| --- | --- | --- | --- | --- | --- |
| port | 507 | 343 | 1 | 851 | (2026-11-06 작성 때 port 41, partial 76, todo 735; 현재 값은 LEFT_JOBS.webpj.md 0.3절) |
| adapt | 9 | 35 | 0 | 44 | partial 9, todo 35 |
| exclude-mp | 3 | 3 | 0 | 6 | - |
| exclude-edit | 31 | 24 | 0 | 55 | - |
| exclude-host | 4 | 23 | 0 | 27 | - |
| exclude-legacy | 1 | 0 | 0 | 1 | - |
| 계 | 555 | 428 | 1 | 984 | |

`exclude-mp`가 6단위뿐인 것은 cpppj의 LAN 사전 구현이 대부분 공유 파일(로그인 부모·Session·Player/SP setter·패킷 분배) 안의 분기이기 때문이다. Multiplayer 메뉴·LAN UI·참가자 목록·일반 전투는 cpppj 3차에서 아직 파일로 생기지 않았다. 기준선 뒤 새 파일은 도구가 "새 파일"로 보고하며, 재기준선 때 분류한다.

이식 원본 규모: port 852단위가 약 13만 7천 줄의 대부분이다. 이식을 마친 `port` 41단위는 WB04 core(SID 풀·해시·탐색기·난수·시계·Kernel·입력 큐·Unpop/Destroy·Form·Factory·Form 호스트)와 WB03의 정렬/Tell/제품 표시다.

## 4. 기존 웹 모듈 대응

webpj/www의 JS 모듈 86개 중 83개를 126행에 대응했다. 나머지 3개(`assets/menu-assets.js`·`assets/terrain-scene.js`·`ui/terrain-preview.js`)는 웹 전용(`WEB_ONLY`, 자산 경로 묶음·개발용 지형 미리보기)으로 표시했다. 대응은 모듈 주석의 cpppj 경로·클래스 이름, manifest의 `cpppjSources`, 단위 문서를 근거로 했다.

- `port`(41): 단위 문서에 파일 단위 이식·원본 fixture 대조가 기록된 행. 예: `SidPool`·`SquidHash`·`RawSquidFinder`·`CrtRandom`·`GameClock`·`Kernel`·`InputEvent`·`RawGameAuxState`·`SquidFactory`·`SquidForms`·`DependFormHost`·`ContentFormHost`·`DrawSort`·`TellParameters`·`TellTip`·`ProductPresentation`.
- `partial`(85): 파일의 일부만 가진 WB04 행(`RawDependCreate`·`RawBuildingSpace`·`Squid`·`BaseProcess`·`SpStore`·`RawFortObjectLoad` 등)과, WB03에서 원본 분석·스크린샷으로 만든 메뉴/Tell/도움말/글꼴/지형 행. 남은 범위는 각 행 메모에 적었다.
- **base:** 이번 단위에서 판정한 모든 대응 행은 `1f00a25a`, 기준선 뒤 `a328ac1f`를 판정한 6행은 `a328ac1f`다. WB03 UI 행의 base는 "기준선에서 대응을 기록했다"는 뜻이며, 파일 단위 재대조는 그 파일을 `port`로 올리는 단위에서 한다.

## 5. 3.6절 9곳 판정 (기준선 `1f00a25a` 비교)

분류: ① 웹 결과 영향 없음 ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫. ②는 이번에도 없었다(이전 ② 1건은 WB04-D5a에서 반영).

| cpppj 소스(웹 근거→기준선 사이 커밋) | 웹 모듈 | 판정 | 근거 |
| --- | --- | --- | --- |
| `o/Config` `0ded2f07`→`2ca39d0b`·`225f2322` | assets/config.js | ① | `LoadBytes`를 `AppendDecodedBytes`로 나눴을 뿐 XOR 감지·첫 NUL·서명/BOM 처리 순서가 같다. `AppendArguments`의 `loadFile`은 기본값 빈 함수라 기존 `readFile` 경로가 그대로다. 나머지는 주석 |
| `o/ConfigInterface` `2ede4027`→`8d2f81a1`·`1ba89851`·`2ca39d0b`·`02fcaacd` | assets/config.js·game-data.js | ① | Startup의 파일 인자 순서(identity·user·options·dev·guild·setup)는 같고 `RawFileName` 10칸 고리 소비 순서만 원문대로 바뀌었다. 실제 FILE/FileSpec 장치·5차 사용자 홈 우선 읽기·인자 첫 NUL 절단은 웹이 game-data에서 설정 버퍼를 직접 조립하고 브라우저 저장소를 쓰는 adapt 경계라 결과에 영향이 없다 |
| `client/TellSection` `6dd8d803`→`2ca39d0b`·`5a2cd047` | ui/tell-parameters.js·game-data.js | ③ | 대체 tell 적재가 `config.LoadFile` 경로로 바뀌었다 → WB05 현재 미션 Tell 연결 때 같은 경로로 옮긴다. Decode/Encode의 `DisplayText` 교체는 cpppj 6차 한국어 표시 부호화라 웹(자체 한국어 경로)에는 ① |
| `o/RiftTypeTable`(+`RiftType.h`) `700475b5`→`fbc27346` | assets/type-flags.js 외 | ③ | 타입 레코드에 `defaultFrame`(+0x128) 보관. 웹 `type-table.js` 정의 해석에는 있으나 Factory용 타입 레코드에는 없다 → WB03-M2/WB05 Draw·타입 레코드 연결 때 |
| `o/CanonDecoder` `5bdb5bed`→`167866d9` | assets/territory.js | ① | 패치판 checking의 금 간 프레임 진단, 명시 프레임 범위 진단 복귀, `Side()` 물리 범위 검사만 더했다. checking이 꺼진 정상 경로의 칸·좌표·라벨은 같고 웹은 진단을 명시 오류로 둔다. 커서 배치 소비자는 WB06 |
| `o/RawTerrainContext` `552656c0`→`3eec0830`·`1fe5fae7` | assets/terrain-mask.js·terrain-fringe.js | ③ | `Edition()`·`ArchipelagoMode()` 접근자만 더했다 → 군도 지형 이식 때 |
| `o/RawIslandPostPop` `deaf73f8`→`0c06fa66` | assets/island-supports.js | ③ | 패치판 throw→진단 복귀, 서쪽 조회 뒤 현재 좌표 재조회, `FootDelta`(DWORD 감김 뒤 signed 덧셈), 장치 지연 요구. 정상 값에서 웹의 정적 3×3 받침 소유자 결과는 같다 → WB04-E PostPop 이식 때 |
| `o/SidPool` `6035f810`/`b7065914`→`90ffe7e5`·`7b78cc20`·`435cfa1d` | core/sid-pool.js | ③ | 2026-11-06 WB04-D3 판정 유지. 패치판 할당 진단 정상 복귀·소진 시 현재 World 다리 회수(최대 50)·물리 슬롯 friend → WB05 World 수명 때 |
| `o/RawSquidContainedUnpop` `d678fcad`→`f83db66c` | core/squid-contained-unpop.js | ②→반영 | WB04-D5a에서 반영(이전 기록) |

같은 단위에서 `depend-create-parent` 등 WB04-D5a 4항목의 manifest `webCommit`을 실제 커밋 `4d5b486a`로 기록했다(3.2절 순서3의 "착수 때 D5a 항목 4개의 webCommit 갱신"도 이로써 끝났다).

## 6. 기준선 뒤 cpppj 변경 (`a328ac1f`, 987 원격 Fort)

`1f00a25a..HEAD`의 `cpppj/src` 변경은 `a328ac1f` 하나(8파일, +88/−9)다.

| 단위 | 분류/상태 | 판정 | 근거 |
| --- | --- | --- | --- |
| client/RawCampaignSessionLogin.inc | port/todo | ① | 추가된 `OpenRemoteFort`·`ReadRemoteFort`와 `serverHooks.loadFortImage`·`receive.openFortImage/readFortImage`는 원격 owner 전용(`SelectRemoteFortPath`가 자기 owner면 진단)이다. 단독(로컬 플레이어+AI)은 미션 Template을 쓰므로 도달하지 않는다 |
| client/RawCampaignSession(.h) | port/todo | ① | 원격 Fort 서비스 칸 2개 추가 |
| client/CampaignTemplateStorage(.cpp/.h) | port/todo | ① | `ReadFile`의 이미지 읽기 장치를 `ImageReadDevices`로 공용화(같은 결과)하고 원격 수신 `ReadImage`를 더했다 |
| client/UberGump.cpp | port/partial | ① | 원격 Fort 경로/작업 namespace 서비스 연결만 추가 |
| client/FortCurrentNameDevices·NewFortManagerDevices | exclude-edit | ① | 원격 경로 조회·UTF-8 경로 변환 |

여섯 행의 base를 `a328ac1f`로 기록해 보고에서 내렸다. 이 단위들을 옮길 때도 이식 원본은 기준선 판이다.

**작업 중 추가된 `1cd2b05a`(20261106-63 CP, 참가자별 월드 동기화·대상 지정 송신·공유 Regular writer, 15파일 +174/−8):** 도구가 8단위와 새 파일 `o/RawPeerWorldSync.cpp/.h`를 보고한다. 8단위(`RawCampaignSession`·`RawCampaignSessionLogin.inc`·`RawSurfaceWorld`·`RawCampaignAssetBindings`·`SquidProcess`·`RawSquidTransmission`·`RawPathData`·`RawWindAviaryProcess`)는 모두 `todo`라 이번 단위에서 판정하지 않고 **각 파일을 옮기는 단위에서 판정**한다. 특히 `SquidProcess`·`RawSquidTransmission`·`RawPathData`의 공유 writer 변경이 단독 경로 결과를 바꾸는지는 그때 기준선 판과 비교한다. 새 파일은 재기준선 때 분류한다. 이 보고는 설계대로의 "이식 때 판정" 상태이며 대응표 오류가 아니다.

## 7. 검사

- `node --test tests/port-map.test.mjs tests/fixture-manifest.test.mjs tests/external-access.test.mjs`: 13/13 통과.
- 최초 실패와 교정: fixture-manifest의 적용 범위 검사가 새 검사 파일 주석의 `PORT_MAP.tsv`를 원본 대조 자료 이름으로 잡았다. 대응표는 fixture가 아니므로 그 검사에서 명시적으로 뺐다.
- `node webpj/tools/port-map.mjs`: 984행, 오류 0. 기준선(행 base) 뒤 변경 보고는 `a328ac1f` 판정 직후 0이었고, 이후 다른 세션의 `1cd2b05a` 8단위(모두 todo, 이식 때 판정)와 새 파일 2개가 보고된다(6절). `node webpj/tools/fixture-manifest.mjs`: 62항목, 불일치 0, cpppj 변경 보고 0.
- 전체 `npm --prefix webpj test`(Node v24.21.0): **434/434 통과**, 실패·건너뜀 0(WB04-D5a 429 + 신규 5), 약 51.5초.

## 8. 남은 경계와 다음 시작점

- 분류는 머리 주석·CMake 주석·호출 위치에 근거한 **파일 단위 1차 분류**다. 파일 안의 멀티플레이 분기는 행 분류와 별도로 이식 때 1절 경계 규칙(단독 관문 그대로, 도달하지 않는 분기는 명시 웹 제외 오류)으로 처리한다. 이식 중 분류가 틀린 것을 발견하면 `RULES`와 이 문서 3절을 고친다.
- `todo` 행의 이식 순서는 3.0절 수직 진행(Tutorial1 Load→Start→표시→입력→진행)을 따르며 대응표가 순서를 정하지 않는다.
- 다음은 3.2절 순서2 **WB02-H**(무브라우저 실행 `tools/headless.mjs`와 cpppj 오라클 `tools/cpp-oracle.mjs`, 첫 대상 M1 화면)이다. 순서3(WB04-D5b) 착수 때는 `node webpj/tools/port-map.mjs`로 기준선 뒤 변경부터 본다.
