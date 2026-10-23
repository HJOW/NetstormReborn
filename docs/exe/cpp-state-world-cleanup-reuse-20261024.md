# StateWorldCleanup 기존 몸체·수명 접합 조사 인계

**일시 중지용 정적 부분 인계**다. 현재/마지막 정밀 호스트는 **HJOW-Athlon / 2026-10-23**으로 같다. root387 build22536 진행 중의 소스를 읽었다. 사용자 중지 요청 이후 신규 탐색·모듈·제품 연결은 시작하지 않았으며, 이미 진행하던 읽기 결과만 회수했다. 공유 source/CMake/LEFT/git/build/lib/GUI와 원본은 변경·실행하지 않았다. 최초 [Native294](../../extracted/cpp-state-world-cleanup-20261024-native.json), [부모](../../cpppj/src/o/RawStateWorldCleanup.cpp), [기존 독립 문서](cpp-state-world-cleanup-20261024.md)는 불변이다.

실제 enum은 **34개**다. 최초 요청의33개는 root와 개수를 정정했다. 아래는 전체34행을 빠짐없이 기록하되, **A=확인한 기존 몸체**, **B=일부 경로/어댑터 또는 소유자 접합 미확인**, **C=이번 조사에서 정확 대응 미확인**으로 구별한다. C는 미구현 확정이 아니다. 주소 문자열 검색 결과나 같은 배열 길이만으로 재사용을 판정하지 않았다. 기존 UI 분석의 과거 호스트/당시 완료 범위를 현재 전체 정리 완료로 합산하지 않는다.

## 34개 enum 자식

| enum·실제 주소 | 분류·실제 동작과 기존 재사용 범위 | 현재 소유자·남은 경계 |
| --- | --- | --- |
| CloseAllConnections 004e29c0 | C. 원문005e4a18..005e4a6c의22포인터를 현재 연결과 비교하고 실제 연결 소멸 후0. | LocalMissionTransport의 존재만 확인했다. 그 객체를22칸 전체 정리와 동등하다고 판정하지 않았다. 실제 연결표/현재연결005c8bf4 소유자 미확인. |
| EndConnections 004e2a10 | C. 현재00540bc0이비0일 때005e4a1c..005e4a38의8칸을 같은 방식으로 정리. | 위22칸의 부분표와 같은 실제 owner가 필요하다. 단독미션의 두 끝 닫기로 대체하지 않는다. |
| ResumeGame 00460e30 | A. [GameClock](../../cpppj/src/o/GameClock.h)의 중첩 Resume, 마지막 해제만 누적정지시간 갱신. | Client의 같은 clock_/PopGamePause를 사용해야 한다. 별도 정지 카운터나 저장된 bool을 만들지 않는다. pauseDepth provider의 구체 공개 경계는 후속 확인. |
| ResumeAudio 004aa970 | B. [SoundMusic](../../cpppj/src/client/SoundMusic.h)의 해당 음소거 해제 경계와 [ClientAudio](../../cpppj/src/client/ClientAudio.h)가 존재. 원문은 양수깊이 감소→마지막에 예약음량 복원. | 실제 Client 음향 수명/음소거깊이·예약 두 음량을 같은 장치에 연결하는 정확 API 대조는 미완료. noAudio를 빈 성공으로 단정하지 않는다. |
| Clear446e70 00446e70 | A. [RawCursorLifecycle](../../cpppj/src/client/RawCursorLifecycle.h)의 실제 Cancel, [RawCampaignCursor::Cancel](../../cpppj/src/client/RawCampaignCursor.h) 경로. 취득 Form/반환·임시객체/미리보기·프로세스를 정리한다. | 같은 Session.cursor와 현재 커서 상태가 살아 있어야 한다. 단순 selectedSID0 또는 이후 ClearWorld로 대신하지 않는다. |
| Clear4c1f20 004c1f20 | A. [ResetClockSynchronization](../../cpppj/src/o/RawClockSyncReset.cpp): **005c8bf4→005c8c00→0055b4ac** 세DWORD0. | 실제 connection/replyMode/gameTicks 참조 owner는 아직 미접합. 명령큐 전체 clear나 GameClock.Reset과 다르다. |
| ResetClock 00460e70 | A. [GameClock::Reset](../../cpppj/src/o/GameClock.h): pausedTotal double만+0. | Client의 기존 clock_가 실제 owner. origin/depth/Frame·현재게임시각까지 초기화하지 않는다.387 등록/검증은 root 진행 범위. |
| Clear42b1b0 0042b1b0 | A. [ClearMetaZoneRecords](../../cpppj/src/o/RawMetaZoneCleanup.cpp): firstReset 비0이면7344BYTE0→flag0, 진입0이면479520(index)51회→표0. | 실제00531760과00546908의51×144BYTE 장기 owner 미접합. ClientLocalStateGlobals의 **5×144BYTE**와 다른 표다. 선택 marker 소멸 자식은 필수. |
| RefreshSp 0043dad0 | B. [CampaignClientServices](../../cpppj/src/client/CampaignClientServices.cpp)의 실제 Renderer invalidate/Menu Refresh 어댑터가 존재. | 현재 UI를 재수집하는 경계이며 원문 전체 Gump 가상 두 호출의 동일성은 별도다. SP setter를 다시 호출하거나 표시를 위해 SP 사본을 만들지 않는다. |
| ClearTemplates 004bfb00 | A. [ClearTemplateHeaders](../../cpppj/src/o/RawTemplateLifetime.cpp)와 [CampaignTemplateStorage::ClearAll](../../cpppj/src/client/CampaignTemplateStorage.cpp). 비0인9헤더만 실제 소유/빌림 분기로 반납한다. | 같은 services.templateStorage/header180BYTE와 실제 manager/노드 수명을 사용. pointer0 기록의 stale 부가필드를 무조건 지우지 않는다. 앞 저장이 완료된 뒤에만 선택한다. |
| LeaveMetamap 004795a0 | C. 원문 metamap 비0에서만 호출되는 자식이라는 부모 관문만 고정. | 해당 전체 몸체/실제 Client 메타맵 owner 대조 미완료. 42b1b0 또는479520과 같은 이름으로 합치지 않는다. |
| Clear445140 00445140 | C. 정확 제품 대응 미확인. | 커서/명령과 주소가 가깝다는 이유로 Cancel 또는 InputQueue clear를 공급하지 않는다. |
| ClearCursor 004d5c60 | A. [RawUserSelection::Clear](../../cpppj/src/client/RawUserSelection.cpp): enabled 관문→다중선택 정리→현재SID0→강조0. | GameWorld.ClearSelection/Session의 현재 activeSelection 수명을 재사용해야 한다. 새 기본 선택 객체 또는 Select(0,0,0)와 동일시하지 않는다. UI가 반납한 링크의 처리도 후속 접합 범위. |
| ClearPlayerLinks 0048fcd0 | A. [CleanupPlayerLinks](../../cpppj/src/o/RawPlayerLinksCleanup.cpp):9Player 각+9c0→+80→비0Deck free→현재base+680. | 같은 Session.players와 **같은 RawProductionDeck::Release(token)**. Player 전체/기술 객체/Player배열을 free하는 함수가 아니다.387 새 몸체를 다시 구현하지 않는다. |
| ClearKernel 00471af0 | A. [Kernel::Clear](../../cpppj/src/o/Kernel.h)의 현재 슬롯1..39999 단일 순회/가상소멸 뒤 슬롯0. | Client.GetKernel과 RawSurfaceWorld의 Kernel이 각각 존재한다. 원문 단일 전역 Kernel을 어느 현재 소유권에 합성할지는 미확정이다. 두 Kernel 소멸자나 반복 Clear를 원문 한 번과 같다고 세지 않는다. |
| KillGumps 004666d0 | A. [GumpLifetime::KillExceptType](../../cpppj/src/client/GumpLifetime.h), 등록1..999·현재 signed 제외타입11·dead 필터. | Uber/Raw controls의 같은 등록표와 현재 제외BYTE가 필요하다. SetControls 빈 목록/전체 menu 파괴로 대체하면 보존 자식과 count/serial이 달라진다. |
| CleanupGumps 004662b0 | A. 기존 [GumpLifetime](../../cpppj/src/client/GumpLifetime.h)와 UberGump.CleanupGumps 지연 큐 경계. | Kill 직후 같은 큐·파생 소멸/OnExit·Form·예약입력 수명을 유지한다. 논리 HTML retired 부모까지 포함한 실제 연결은 별도이며, 큐를 무조건 비워 성공으로 처리하지 않는다. |
| ReplaceUber 004c77c0 | B. [UI reset 분석](cpp-ui-reset-analysis.md)의 기존 root 이동/ReplaceRoot helper는 존재. 원문은 새Uber200BYTE 생성→임시등록칸 제거→살아있는 자식 Push/Attach→루트교체. | 현재 frontend Uber wrapper 생성과 원문004c7160 전체 파생생성은 구별해야 한다. Client.menu_.reset 또는 InitializeRoot 반복은 정확 대체가 아니다. 현재 공용 트리·실제 새Uber child 생성 수명 접합 미완료. |
| Clear43b540 0043b540 | B. 원문은00531b08의9DWORD에0..8을 순서대로 저장. [CoreRecords 조사](cpp-player-core-records-20261024.md)에서 Core장부와 별개임을 고정했다. | 기존 BridgeConnectState.ownerColors가 실제 대응 후보/저장소다. 호출 가능한 정확 reset API 접합은 미확인. Core9 초기화나 struct 전체 대입으로 대신하지 않는다. |
| Reset422490 00422490 | B. [BridgeDecayScan::Reset](../../cpppj/src/o/Bridge.cpp)은PATCH first15000/last23001/cursorfirst/현재시각+주기 계산을 가진다. | 원문 현재 period0052f968·gameTime0055b4d0과 같은 현재 decay state를 빌리는 provider 대조 미완료. 새 scan상태나 wall시각으로 대체하지 않는다. |
| Clear4c2450 004c2450 | A. [RawTotalMade::ResetCounts](../../cpppj/src/o/RawTotalMade.cpp): 실제 local/localSecondary/global 세 표의 첫255DWORD만0. | 같은 core.bookkeeping의 세 배열.256번째 물리칸·금지표·productionDirty 등 다른 필드는 유지한다. |
| MapMode 004437f0 | A. 원문 **단일 ret**이며 [RawChatChannels](../../cpppj/src/client/RawChatChannels.cpp)에도 그 빈 몸체가 구별돼 있다. | 새 UI refresh/상태변경을 넣지 않는다. 부모의 두 호출 위치는 그대로 보존한다. 이는 미복원 자식을 빈 성공으로 바꾸는 경우와 다르다. |
| SceneMusic 00469fc0 | B. [ClientAudio::StartScene](../../cpppj/src/client/ClientAudio.cpp)/[SoundSceneMusic](../../cpppj/src/client/SoundSceneMusic.h) 부모가 존재. 원문 현재battle에 따라ser22 또는게임난수 원소곡→프레임음악→곡길이 전역. | 현재 실제 음악장치/곡감독/게임난수 owner 동일성의 전체 대조는 미완료. CRT나 새 RNG, 곡만 직접 Play하는 호출로 대체하지 않는다. |
| Clear472830 00472830 | C. 원문00592938부터20포인터의 각 **pointee+0c DWORD**를0으로 쓴다. | 실제 포인터표/대상 owner 미확인. enum의 과거 Graph/명령 추정이나 배열 자체0·Kernel.Clear로 재사용 판정하지 않는다. |
| Clear4b2b50 004b2b50 | B. 원문 고정this00542508→004b28f0. [SquidHash::Reset](../../cpppj/src/o/SquidHash.cpp)은 이미 할당된 네 head배열 reset 경로가 있다. | 같은 core.hash의 정상 preallocated 경로와 전체 raw allocation/header alias를 구별한다. 후자까지 복원 완료라고 하지 않는다. selected단계와 실제 저장소가 유지돼야 한다. |
| Clear4328a0 004328a0 | C. 원문0054d4e8의255DWORD0만 확인. | 현재 owner/정확 몸체 미확인. 길이가255라는 이유로 restrictions/TotalMade 표와 같다고 판정하지 않는다. |
| Clear4ab940 004ab940 | C. 현재005c7c44 포인터의0x4000DWORD=65536BYTE0. | core.spots는 후보이나 길이만으로 동일성을 확정하지 않았다.005c7c44의 실제 저장소 SourceMap 확인 후 연결해야 한다. hash head WORD표와 합치지 않는다. |
| Reset568b60 0046edf0 | A. [RawIslandRuntime::Reset](../../cpppj/src/o/RawIslandRuntime.h)의 count+100×32BYTE 원본 필드 reset. padding 보존. | Session.islandBytes를 빌린 기존 islands 객체. Core10BYTE/9장부와 다른3204BYTE다. 새 IslandRuntime을 만들어 다른 표를 지우지 않는다. |
| Reset54c8a0 00432360 | B. 원문 this.mode 저장→1→현재폭/높이의 각12BYTE 셀 지정필드 초기화→mode복구. 그 뒤 부모가첫DWORD1. | Session.chunks의 실제 raw layout/mode와 동일 owner인지 이번 비교 미완료. ChunkMap 전체 대입이나 같은폭 새맵으로 우회하지 않는다. |
| ResetGraph 00462b20 | A. [RawGraph::ResetTable](../../cpppj/src/o/RawGraph.cpp)의 앞251행 surfaces/inUse WORD0→Graph0/inUse·reserved lowBYTE1→sentinel7dfd. | 같은 core.graph. reserved 상위BYTE·나머지행·pool·stack은 보존한다. Rebuild(false)나 Graph전체0과 다르다. |
| Clear4a9e50 004a9e50 | B. [Sound](../../cpppj/src/client/Sound.h)에 해당 주소의 기존 경계가 존재. 원문은 현재buffer 상태를 조회해 선택된 항목만 반납한다. | 정확 공개 API/같은 Client sound buffer 이름표·장치 연결 대조 미완료. StopAll 또는 음소거로 대신하지 않는다. |
| InitializePlayers 004910a0 | B. 원문은기존9개의+0 factory block free→Player1620BYTE free/NULL→새할당/0→9개각new120/용량30/개수0/기본필드/동맹 효과다. | Session의1620BYTE와 용량30 관련 기존 초기화는 있으나 **전체 부모의 같은 할당·해제 수명**은 미확정. RawPlayerWorld::Create(152), vector.clear/assign 또는 PlayerLinks 정리와 같은 함수가 아니다. |
| ResetZone 004bfd60 | B. 원문 고정this005c86c0에DWORD0→WORD+40, 정확6BYTE다. | Login.zoneWords/Zone()6BYTE 경계가 있지만 실제 Client장기 고정 Zone owner와의 동일성은 미확정. 뒤2BYTE를 포함한8BYTE0이나 State블록 전체0으로 바꾸지 않는다. |
| InterpretOptions 00435220 | B. [Client::InterpretOptions](../../cpppj/src/client/ClientMain.h)/ApplyAudioOptions·현재ReadOnlyForts callback이 존재하나 코드도부분복원이라고 명시한다. | 현재설정·창/언어/소리/팔레트·게임 옵션 owner를 같은 위치에서 재해석해야 한다. ReadOnly 한DWORD만 해석하거나 새로운Client 생성으로 전체부모 성공을 가장하지 않는다. |

## enum 이외의 선택 hook

| hook·원문 | 확인한 재사용·실제 소유자 | 남은 접합/중단 계약 |
| --- | --- | --- |
| disconnect 004e2940 | LocalMissionTransport/Session.Login 전송은 존재. | Native reason/client 분기의1..8 또는9 뒤10/11을 같은 현재 연결표에 적용하는 정확몸체 대조 미완료. 앞22/8포인터 정리와 별개다. |
| assertion State.cpp0x77a | 원문 fort/battle/metamap 중복 관문의 진단. | 진단복귀 후 계속하는 실제 장치가 필요하다. 미연결은 같은 위치에서 중단하며 모드를 강제로 고쳐 성공시키지 않는다. |
| pauseDepth 00460de0 | 같은 Client GameClock/정지 깊이를 읽어 ResumeGame→ResumeAudio 뒤 재조회. | 독립 카운터나 진입 때 캡처한 상태를 반환하지 않는다. 실제 공개 depth provider의 정확 서명 확인은 남았다. |
| saveFort 004bf9a0 | 기존 [RawFortSnapshotParent](../../cpppj/src/o/RawFortSnapshotParent.h)와 Session SnapshotFortBody는 같은 Storage/기술/객체/Money/Mission/Badges/Notify를 이미 조립한다. | 현재 manager/slot·FileSave/readOnly 실제장치를 재사용. cleanup의 현재시점에 loaded 선행관문을 추가하지 않도록 외곽 계약 확인 필요. 저장 실패를 성공으로 바꾸지 않는다. |
| saveBattle 004bf880 | 같은 독립 Snapshot 부모의 Battle 몸체가 존재. | 실제 Battle CoreData writer/현재Core·SP/RNG까지 동일한 Session 접합은 미완료. ExistingEditor 전용 CoreWrite API를 무조건 호출하지 않는다. |
| resetAi 00413ae0 | Session.RemoveAi/실제 AI 수명 모듈들이 존재. | 413ae0와 정확동등성/같은 owner의 전체하위 비교를 이번에 완료하지 못했다. 이름만으로 연결하지 않는다. |
| resetPriest 00492240 | 기존 RawPriestParticipation/Session.RefreshPriestParticipation이 같은 Player 장부를 다룬다. | 이 주소의 정확 몸체/효과순서 대조 미완료. 단순 priest목록 clear로 대체하지 않는다. |
| resetOtherPlayer 00447810(owner,0) | 기존 Temple/Player 관련 몸체는 존재. | 이번에 정확 API/장부를 확정하지 못했다. resetPriest/Player전체초기화와 합치지 않는다. |
| setSp 0040edb0 | 기존 Session.SetSp→같은 ScrambledSpStore/Session.random·실제추적DWORD 경계. | 원문이 캡처한 owner/float비트를 그대로 전달. 새 SP표·RNG·AddSp 통지로 치환하지 않는다. 표시 RefreshSp는 부모가별도로 호출한다. |
| selectCore 0048f3b0 | [SelectPlayerCoreData](../../cpppj/src/o/RawPlayerCoreData.cpp)/[ClientCoreDataGlobals](../../cpppj/src/client/ClientCoreDataGlobals.h)의 같은 current/cache/Player뷰 API가 존재. | Session.Login.currentCore/cachedCore와 Client장기 owner는 아직 별도다. 핵심 미접합이다. 이전Player 해제 전0선택/4·4·2cache복사/NULL·active0을 끝내고, 후반29/33 외1선택은 새Player의 alias여야 한다. |
| resetPool 004af170(0) | 기존 SidPool::Reset(0)은 원문004abb10의 pool/free-list 몸체다. | 004af170은 그밖의004952d0 두40DWORD표 초기화도 포함한다는 기존 근거가 있다. SidPool.Reset 단독을 전체 wrapper 완료로 세지 않는다. 같은 pool은UI/Kernel/Core정리 뒤까지살아있어야 한다. |
| wallSeconds 00460d70 | Client.WallSeconds의 그때 실제 벽시각 조회. | 부모 마지막위치에서새조회→double+10.0→005c8610. 고정 FrameTime/게임시각/새시계로 대체하지 않는다. |

## 직접 저장소 provider와 가장 중요한 수명 경계

| provider | 지금 확인한 같은 실제 저장소 | 미완료/금지 대체 |
| --- | --- | --- |
| read32/write32 | 00540cac/00540c70/00595344는현재 PlayerWorldState의templateOwner/battleOwner/playerKnown.594fb8/fbc/fc8는현재world의fort/battle/challenge,5c85a4/594fa4는현재editor/missionPost 경계. | Native부모의나머지직접전역은아직주소별Client/Session매핑을전부완료하지못했다. 전체globals={} 또는bool을임시DWORD에복사해동기화하는방식은금지. 선택store를같은실제필드에직접적용해야한다. |
| write16 | 005c89b4는같은ClientRandomGlobals.FortCoreSeed의실제DWORD. | 하위WORD만0,상위WORD보존. 별도Session seed나전체DWORD0 금지. |
| write64Bits | 부모가쓰는5950b8/5950c8/5950c0/565e68/5c8618/595358/5c8610/5c85f0/5c85f8의각현재double필드. | 실제owner개별매핑미완료. 주소가Core장부근처라는이유로currentCore/cache와같다고하지않는다. 벽기한5c8610과ClientLocalState의activityEnd544d58도같다고단정하지않는다. |
| players | Session.players는현재9×180BYTE. RawPlayerWorld 등은현재고정span을빌린다. | Native는각store마다현재base재조회한다.4910a0재할당뒤이전span을보유한모듈/현재Core를그대로두면안된다. 새vector를두번째원시Player라고공급하지않는다. |
| templateManager | 같은CampaignTemplateStorage.Headers의현재slot×20 첫DWORD. | 최초manager를읽은뒤여덟전역쓰기/저장분기를수행하고Fort저장복귀때만slot재조회하는부모순서보존. 이름/parsedfort존재로manager비0을가장하지않는다. |
| typeDefault | 현재world의mutable TYPE[type].cost(float,+c4) 참조를빌려야한다. | 입력type00541240/defaultSP00540bdc의정확한현재owner/접합은미완료. immutable assets사본이나중간float에써서동기화하지않는다. |

부모의핵심은 **PlayerLinks→Kernel→Gump Kill→Cleanup→Uber교체→9색표→seed.low16/known0→SelectCore(0,1)→Pool/나머지표→Graph→Player재할당→Zone→모드쓰기→조건부SelectCore(1,1)** 이다. State29/33외에는끝에서currentCore가다시Playeralias일수있다. 그러므로Session파괴시점의소유권정책과Native정상반환상태를구별해야한다. 부모앞에서ClearWorld/menu.reset/전체structreset을하면기존저장·커서·자식소멸·cache복사가보던대상을잃는다.

재개후최소후속은새전체cleanup어댑터를한번에쓰는것이아니다. **004910a0 전체부모와현재Player/factoryblock소유권을정확대조한뒤,같은Client Core의Select0→cache→이전Player해제/새할당→선택적Select1을한수명단위로설계**하는것부터권고한다. 기존Core선택/장부·Kernel/Pool/Graph/Island/Template몸체를중복복원하지않는다. 새독립Player초기화모듈이필요한지는그대조후판정하며현재필요하다고확정하지않았다. 이번에는새모듈/제품patch/Native관측을작성하지않았다.

**남은 작업:** C행 7개와 B행의 정확 자식/owner 대조, disconnect·AI/Priest/447810·Battle 저장·Pool wrapper, 직접 전역 주소 전체 매핑, 단일 Kernel/공용 Gump/ClientCore와 SessionPlayer의 수명 접합이다. 387에 이미 등록된 Clock/Core9/DeckRelease/PlayerLinks/MetaZone을 새로 만들지 않는다. 388 [CoreRefresh 사본](cpp-player-core-refresh-session-next-20261024.md)과 Subscriber/Primary는 별도 동결 후속이며 이번 조사로 적용 완료라 하지 않는다. 사용자 요청에 따라 여기서 조사를 멈춘다.
