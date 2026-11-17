# LEFT_JOBS.webpj — 현행 상태와 남은 작업

webpj(HTML + JavaScript 웹 클론)의 **현재 진행 상황·남은 작업(단계별)·유효한 결정**만 둔다. cpppj/공통은 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)가 기준이다. 지난 이력·끝난 체크 항목·만료된 계획은 [보관본](LEFT_JOBS.webpj.old.md)에 원문 그대로 있다(이번 정리 직전 전체는 [2026-11-17 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632)). 단위별 상세(근거 커밋·fixture·최초 실패와 교정·남은 경계)는 각 `docs/webpj-*.md` 단위 문서와 `webpj/PORT_MAP.tsv` 메모가 기준이다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → **3.1절 4-D(진행판·구간 지도·계획 4D-1~4D-10)** → 0.5(단위 착수/종료 절차) → 3.5(사용자 결정 대기) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 칸으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-17, 마지막 단위 WB04-E632)

| 항목 | 현행 기준 |
| --- | --- |
| 실행·플레이 | 클릭하여 시작→원본 자산 메인 메뉴·캠페인/데모 선택 목록·도움말/옵션·Credits 세 항목(11.01 Patch Credits 포함)과 개발용 지형 미리보기(`?map=`)까지 동작한다. **실제 미션 플레이는 아직 불가**다. 옮긴 월드 부품은 아직 앱 수명에 연결하지 않았고(`PORT_MAP` session 0), `MissionBegin`/`StartDemo`는 미션 월드에 연결되지 않았다 |
| 원본과의 동일성 | 메뉴는 **cpppj 오라클(L4a) M1 8장면**(Welcome·메인 메뉴 8버튼·Campaign·Demo·Credits·Help/Options 펼침 메뉴·General Help, 영어 4:3, 원본 비교 모드)이 기록한 원인 밖 픽셀 차이 0이다(구름 배경 등 원인 기록은 4절). Fort/지형·그림·SID와 월드 부품은 원본 10.78 x86 fixture 또는 독립 계산과 대조했다. 실제 미션의 L2/L3·전체 화면/음향·자연 완주는 미검증이다 |
| 마일스톤 | **M1 완료** · **M2 진행 중**(순서3·4-A~4-C·4-R 완료, 현재 단계 **4-D**의 계획 단계 **4D-1 외곽 Session 공개·전체 snapshot**, 3.1절) · M3~M5 미착수 |
| 4-D 진행 | 체크 **44/85**. cpppj `RawCampaignSession.cpp` 6,406줄 중 **typed 조립 완료 3,351줄(52.3%)**, 4-D 몫 남음 2,003줄(31.3%)·단독 도달 판정 269줄·다른 단계 481줄·웹 제외 302줄(3.1절 구간 지도). 완료 구간 안의 Native(10.78 Client) 분기(4D-8)와 웹 Client의 Session 소유(4D-6·4D-7)는 별도 계획 단계다. 4-D에 배정된 남은 85단위 / 18,414줄, 계획 10단계는 약 45~65 작업 단위로 예상한다 |
| 이식 진행도 | 대상 1,014단위(제외 146 별도) 중 **port 855(84.3%)·partial 39(3.8%)·todo 120(11.8%)**. 남은 159단위 / 34,223줄(2026-11-17 현재 cpppj 소스로 재집계)은 기존 기준선 84단위와 재기준선(4-R)에서 추가된 75단위다(단계 배정은 0.2·0.3절). 앱 수명에 연결된 단위(`session`)는 아직 0이다 |
| 마지막 작업 단위 | **WB04-E632** — 공용 Territory 부모/섬 검색/seed를 원본638+20,162+188행과 대조하고 같은 실제 Runtime·현재 섬/청크·Template 새 노드에 연결했다. 신규5건·집중39건 통과([단위 문서](docs/webpj-fort-territory-save-20261117.md)). 다음은 Money/Mission·Badges/CoreData와 전체 snapshot/외곽 공개·Load다 |
| 마지막 전체 검사 | 2026-11-17 WB04-E632: Node v22.23.3·v26.13.0 최종 전체 각각 **5,996건 통과, 실패·취소·건너뜀 0**. v24는 사용자 지시로 생략하고 호환을 가정한다 |
| 마지막 e2e | 2026-11-17 E616 Chromium **2/2 통과**(초기 페이지·독립 저장소 최초 실행/기존 설정·진행 재실행). Welcome/Welcome2·Options5·5·사용자3·2·Struggle 잠금/체크를 실제 클릭, 처리되지 않은 예외0. 실제 미션/월드 검사는 아니다 |

**최근 완료 요약:** 최초 실행 상태 웹 반영(E616 — 두 localStorage 키가 없는 독립 Chromium 저장소에서 Welcome/Welcome2·Struggle For Freedom 잠금/체크·Options 기본값을 실제 클릭으로 확인, 기존 사용자 설정·진행은 보존)과 cpppj1320 내부 미션 전환의 flush/종료 수신 재반영(E620)을 마쳤다. 세부 체크는 [2026-11-17 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632) 0.1절, 근거는 [원본 관찰](docs/exe/original-first-run-20261116.md)·[웹 검증](docs/webpj-first-run-20261116.md)·[미션 전환](docs/webpj-mission-transition-20261117.md)이다. 실제 미션 완료 효과 연결은 8단계 잔여다.

**다음 작업 (위에서부터):**

1. **4D-1a** — 전체 snapshot 자식 writer 4단위(RawFortMoneySave·RawFortMissionSave·RawFortBadgesSave·RawFortCoreWrite)의 캠페인 공유 호출을 확인해 `exclude-edit`에서 `port`로 교정하고 이식한다(E627~E632와 같은 방식). 이어서 4D-1b~d(전체 snapshot 본체·외곽 callback 대상·외곽 생성자)로 Session 외곽 공개를 마친다.
2. 그다음 3.1절 4-D 계획 순서(4D-2 `Load()` → 4D-3 Login·4-C 통합 시험 → … → 4D-10 L2)로 간다. 각 항목에 cpppj 근거 줄·함께 옮길 단위·검증 시험을 적어 두었다.
3. 4-D 항목이 선행 미이식으로 막히면 3.1절 진행 방법(사유 기록 후 다음 항목, 단계 전체가 막히면 5-A·5-B 병행)을 따른다.
4. 3.8절 허브/적응 판정 4건은 5-C·8단계 항목에서 처리한다.

### 0.2 단계 진행판

순서별 완료 기준은 3.2절, 단계별 체크 목록은 3.1절이다. **진행** 칸의 `n/m`은 3.1절 체크 항목 수다(보관본으로 옮긴 끝난 항목 포함). **남은 단위** 칸은 `PORT_MAP.tsv`에서 todo·partial인 cpppj 단위를 단계에 배정한 수와 현재 cpppj 소스 줄 수 합계다(partial은 파일 전체 줄이라 상한, `python -X utf8 tools/port_inventory.py --families wb`). **기존**은 옛 기준선부터 있던 단위, **재기준선**은 4-R에서 추가된 단위이며 그 배정은 판정 문서([E498](docs/webpj-rebaseline-surface-20261115.md)~[E502](docs/webpj-rebaseline-meta-20261115.md))의 후속 단계를 따른다(0.3절 목록).

| 순서·마일스톤 | 단계 | 내용 | 진행 | 남은 단위 / 줄 (기존 + 재기준선) |
| --- | --- | --- | --- | --- |
| 1~3 M1·M2 선행 | WB02-G·H, 3-A~3-E | 대응표·오라클 기반, 월드 부품 E1~E264 | 완료(2026-11-10) | — |
| 4 M2 | 4-A·4-B·4-C | 자료·설정·타입 표 / Fort·지형·섬 적재 / 패킷·로그인·내부 전송 | 완료(E265~E328, 4-C 3항목은 4-D 이월) | — |
| 〃 | 4-R 재기준선 | cpppj 제품 985 → 1313 전환과 오라클 재수집 | 완료(E497~E514) | — |
| 〃 | **4-D** | Session 조립·Tutorial1 Load/Start → **L2** | **진행 중 44/85 (계획 4D-1)** | 23 / 13,807 + 62 / 4,607 |
| 5 M2 | 5-A | 렌더러·Draw·시각 시스템 연결 | 진행 중 44/46 | 3 / 533 |
| 〃 | 5-B | 카메라·HUD·미니맵·커서 | 진행 중 9/13 | 5 / 1,039 + 4 / 407 |
| 〃 | 5-C | 게임 화면 허브(Uber·메뉴 컨트롤)·미션 진입 명령 → **M2 판정**(L4a) | 0/6(메인 메뉴·타이틀 State 3단위 선행 완료) | 10 / 5,769 |
| 6 M3 | 6 | 입력·게임 메뉴·사제/운반·경제·건설 연결 → L3 | 1/8(게임 메뉴·우클릭 메뉴·커서 선행 부품 다수 완료) | 10 / 1,034 + 2 / 199 |
| 7 M4 | 7 | 전투·의식·AI 연결 | 0/6(Hit·GunQueries·Archer·Motion·BombBlast 선행 완료) | 3 / 97 |
| 8 M4 | 8 | 미션 수명·안내·대본·소리·채팅·도움말 잔여 | 0/9(SoundProcess 선행 완료) | 30 / 5,941 |
| 9 M5 | 9 | 표시 품질(60/120)·브라우저/OS 확인·정적 배포 | 미착수 | 0 + 1 / 41 |
| 10 후순위 | 10 | AGENTS 4~14차 중 웹 해당분(4차 WebMCP·와이드, 5차 localStorage, 6차 미션 문구, 7차 console 로깅, 10차 버그, 11차 난이도, 12차 스커미시+Outpost, 13차 기타 언어 — 3.7절) | 일부 선행(11차 난이도 계산 E554·E555, Outpost 목록 수명 E556), 5차 대부분 충족 | 0 + 6 / 749 |
| **합계** | | | | **84 / 28,220 + 75 / 6,003 = 159 / 34,223** |

**2026-11-17 배정 변경:** cpppj `Load()`(4344..4351)와 `ConnectMotion`(3930)이 직접 설치하는 바인딩 6단위 — RawCampaignDevastation·RawCampaignMeltdown·RawCampaignRitual(옛 7단계)과 RawCampaignCarrierDrop·RawCampaignPrayer·RawCampaignPanic(옛 6단계) — 를 1절의 "현재 단계가 요구하는 부품만 앞당김" 규칙으로 4-D(4D-2)로 옮겼다. 줄 수는 cpppj1314~1320 변경(ClientMain 2,122·UberGump 2,686·ClientAudio 492줄)을 반영해 다시 셌다(이전 표 159 / 34,206).

**남은 일의 성격:** ① **조립**(4-D·5-C) — `RawCampaignSession`(7,118줄)·`ClientMain`(2,122줄)·`GameWorld`(1,157줄)·`UberGump`(2,686줄)·`RawMenuControls`(1,605줄)·`RawButtonControls`(811줄) 같은 허브가 이미 옮긴 부품을 한 월드로 묶는다. 여기서 처음으로 L2(session-dump)·L4a(미션 화면) 비교가 가능해지고, 부품 단계에서 보이지 않던 순서·수명 결함이 드러날 것이다. 재기준선에서 추가된 장기 월드 저장소(`RawSurface*`)·Native 수신기 묶음도 이 결선에 붙는다. ② **client 바인딩**(6·7) — `RawCampaign*` 바인딩·게임 메뉴·커서처럼 `o/` 부품을 세션에 잇는 얇은 단위가 많다. ③ **웹 적응**(8) — 소리·음악·동영상은 Web Audio 등으로 바꿔야 하는 adapt 단위다.

### 0.3 남은 이식량

**대응표 기준(2026-11-17 `tools/port_inventory.py` 재집계, 줄 수는 현재 cpppj 소스이며 기준선 `225553df` 판과는 기준선 뒤 cpppj 변경분만큼 다르다):**

| 범위 | 대상 | port | partial | todo |
| --- | --- | --- | --- | --- |
| 전체 | 1,014단위 / 147,028줄 | 855 (84.3%) / 112,805줄 (76.7%) | 39 (3.8%) / 19,142줄 | 120 (11.8%) / 15,081줄 |
| `o/` 월드·규칙 | 568단위 / 73,974줄 | 553 (97.4%) / 71,860줄 (97.1%) | 3 (0.5%) / 1,164줄 | 12 (2.1%) / 950줄 |
| `client/` 세션·UI·표시 | 445단위 / 72,619줄 | 301 (67.6%) / 40,510줄 (55.8%) | 36 (8.1%) / 17,978줄 | 108 (24.3%) / 14,131줄 |
| `zacket/` 내부 미션 전송 | 1단위 / 435줄 | 1 (100%) / 435줄 (100%) | 0 | 0 |

표의 port 줄 수는 현재 기준선 파일 전체의 규모이며 기존 행에 남은 ③ 후속 기능의 구현을 뜻하지 않는다. 분류(class)별로는 port 963(이식 대상)·adapt 51(웹 대체가 필요한 플랫폼 경계)·제외 146(exclude-mp 62·exclude-edit 51·exclude-host 32·exclude-legacy 1)이다.

**영역별 남은 양**(todo + partial, `tools/port_inventory.py`, 영역 기호는 [LEFT_JOBS.dotnetpj.md 2.7](LEFT_JOBS.dotnetpj.md#dotnet-inventory)과 같다):

| 영역 | 남은 단위 / 줄 | partial | todo |
| --- | --- | --- | --- |
| A 자료·파일·설정 | 5 / 716 | 3 / 597 | 2 / 119 |
| B 월드 코어(SID·슬롯·Pop·해시) | 6 / 1,073 | 1 / 709 | 5 / 364 |
| C Fort·지형·섬 적재 | 4 / 327 | 0 / 0 | 4 / 327 |
| D 세션·월드 조립·로그인·패킷·Client 전역 | 36 / 14,155 | 6 / 10,089 | 30 / 4,066 |
| E 표시·렌더러·카메라·HUD·시각 효과 | 8 / 1,206 | 4 / 506 | 4 / 700 |
| F 입력·커서·선택·게임 메뉴 | 12 / 818 | 4 / 266 | 8 / 552 |
| G UI 프레임워크·메뉴(Gump·HTML·Tell) | 22 / 7,564 | 11 / 4,222 | 11 / 3,342 |
| H 사제·운반·수확·경로 | 9 / 376 | 0 / 0 | 9 / 376 |
| I 건설·생산·경제·다리 | 16 / 1,483 | 1 / 48 | 15 / 1,435 |
| J 전투·비행·투사체·방어 | 9 / 657 | 1 / 163 | 8 / 494 |
| K AI | 1 / 28 | 1 / 28 | 0 |
| L 신전·제단·의식·승패 | 5 / 378 | 2 / 236 | 3 / 142 |
| M 미션 수명·안내·대본·튜토리얼 | 2 / 484 | 0 / 0 | 2 / 484 |
| N 소리·음악·동영상 | 13 / 3,374 | 4 / 2,066 | 9 / 1,308 |
| O 채팅·메시지 | 2 / 90 | 0 / 0 | 2 / 90 |
| X 요구사항 전용(4~14차) | 3 / 652 | 1 / 212 | 2 / 440 |
| Z 멀티플레이·LAN·Outpost | 6 / 842 | 0 / 0 | 6 / 842 |
| **합계** | **159 / 34,223** | 39 / 19,142 | 120 / 15,081 |

**재기준선에서 추가된 미완료 75단위의 단계 배정** (재기준선 판정 E498~E502의 후속 단계 기준, 단위 / 현재 줄):

- **4-D — 62단위 / 4,607줄**
  - 장기 월드 저장소 29단위 / 1,517줄([E498](docs/webpj-rebaseline-surface-20261115.md)) — 실제 10.78 Client는 시작 때 Storage에 Native 장치를 결합하므로 Storage 멤버는 4-D(4D-8)에서 옮기고, 전투·의식 등 소비 결선의 나머지는 소비자 단계(6·7)에서 연결한다: RawSurfaceAttack·RawSurfaceAssetDeletion·RawSurfaceAssetNotice·RawSurfaceBalloon·RawSurfaceBomb·RawSurfaceCarrier·RawSurfaceContent·RawSurfaceDaisDeletion·RawSurfaceDamageable·RawSurfaceDecoration·RawSurfaceDeletion·RawSurfaceDepend·RawSurfaceDudeRegular·RawSurfaceFactoryRegular·RawSurfaceFence·RawSurfaceFlare·RawSurfaceFrameRegular·RawSurfaceGump·RawSurfaceIce·RawSurfaceLifetime·RawSurfaceMana·RawSurfacePlayerRegular·RawSurfacePriest·RawSurfacePriestDeletion·RawSurfaceRegistryDeletion·RawSurfaceResidenceRegular·RawSurfaceReward·RawSurfaceRitual·RawSurfaceVortex
  - 전송·Native 수신기 14단위 / 1,077줄([E501](docs/webpj-rebaseline-transport-20261115.md)): RawPacketDelay·RawPacketFrame·ClientPacketSources·RawSquidReceive·RawSquidMutationReceive·RawSquidCompressedReceive·RawSquidVerification·RawPeerWorldSync·NativeLoginPacketReceivers·NativeGameplayPacketReceivers·NativeCarrierPacketReceivers·NativeConstructionPacketReceivers·ClientChatPacketReceiver·FortCoreDataLoad
  - 메타·전투 경계의 단독 도달 앞단 7단위 / 817줄([E502](docs/webpj-rebaseline-meta-20261115.md)): ClientMetaPacketReceivers·ClientChallengeGlobals·RawBattleCompletion·NativeMetaConnectedState·NativeBattleReturnState·ClientMetaSpriteRefresh·NativeMetaSpriteRefresh(두 Sprite 갱신은 5-C 화면 허브와 함께 확인)
  - 규칙·건설 장치 9단위 / 794줄([E500](docs/webpj-rebaseline-gameplay-20261115.md)): ClientConstructionGlobals·ClientConstructionRegions·ClientConstructionRandomPlacement·ClientConstructionPlacement·ClientConstructionDevices·ClientConstructionGem(Tutorial1 필요분은 4-D, 입력·건설 나머지는 6)·RawCampaignVortex·RawManoSpell·AssetDeletionVtables
  - 진단·Fort 저장 경계 2단위 / 303줄([E499](docs/webpj-rebaseline-boundaries-20261115.md)): SessionStateDump(웹 session-dump, 3.5절 1번)·NativeFortSaveVirtual
  - 시간 동기화 1단위 / 99줄: RawClockSynchronization(partial, 4D-3c)
- **5-B — 4단위 / 407줄**([E502](docs/webpj-rebaseline-meta-20261115.md)·[E500](docs/webpj-rebaseline-gameplay-20261115.md)): HudNsInfoMeta·NativePlayerInfoInput·NativeNsInfoInput·HudPlayerButtonDraw
- **6 — 2단위 / 199줄**([E501](docs/webpj-rebaseline-transport-20261115.md)): RawBuildReplay·ClientBuildReplay(건설 기록 재생)
- **9 — 1단위 / 41줄**([E499](docs/webpj-rebaseline-boundaries-20261115.md)): FrameRateOption
- **10 — 6단위 / 749줄**([E500](docs/webpj-rebaseline-gameplay-20261115.md)·[E499](docs/webpj-rebaseline-boundaries-20261115.md)): 12차 스커미시 SkirmishSetup·SkirmishStartIslands·RawIslandPartition·RawCentralIsland·RawBattleIsland, 표시 입력 DisplayTextInput

### 0.4 기준 값

| 항목 | 현행 기준 |
| --- | --- |
| cpppj 기준선 | **`225553df`(제품1313·표시 11.01).** 이식 원본은 기준선 판(`git show 225553df:경로`)이고, 이후 변경은 3.6절로 판정한다. cpppj 소스가 같은 마지막 변경 `979d65b5d`와 대조했다. **오라클 실행 파일**은 별도 빌드 `extracted/webpj-oracle/225553df/Netstorm.exe`(SHA-256 `5b52d96d…b77eb46`, Git 제외·HJOW-Athlon 전용)다. 이전 제품985 실행 파일과 오라클도 보존한다 |
| 이식 범위 | **`webpj/PORT_MAP.tsv` 1,160단위**(기준선 `o/`607·`client/`552·`zacket/`1). 분류: **port 963·adapt 51·exclude-mp 62·exclude-edit 51·exclude-host 32·exclude-legacy 1, 미분류 0**(E573·E588·E627~E632 캠페인 공유 함수 범위 복원). 상태: **port 855·partial 39·todo 120·제외(-) 146**, session/oracle 0. partial에는 기존 단위의 부분 이식 외에 0.5절의 초안 2개·E574 Session 기반 자료·E584·E585 Devastation 자격/타이머와 E595 Ritual 준비 함수·E601 건설 재생 실행/현재 라벨이 있다. 웹 소스는 `webpj/www`의 `.js`(자산 `data/` 제외) 134,386줄, 검사 파일 802개다(2026-11-17 재집계) |
| 이식 추적 | `webpj/tests/fixtures/manifest.json` **816항목**(original-x86 777·screenshot 13·source 10·independent 9·cpppj-oracle 5·original-binary 2, 2026-11-17 E632 갱신). 항목별 추가 이력은 단위 문서와 보관본에 있다 |
| 디컴파일 | webpj는 디컴파일 소스 분석이 필요하지 않다(AGENTS.md `c17aeede1`, 2026-11-15). 근거는 cpppj 소스·fixture이며 착수 때 디컴파일 호스트를 확인하지 않는다 |

### 0.5 매 단위 착수/종료

**미완성 초안 2개(partial):** `ClientRendererFrameGlobals`는 [E496](docs/webpj-renderer-cloud-queue-20261115.md)에서 구름 큐의 고정 칸 참조를 교정하고 실제 Renderer 앞단과 원본 관측에 대조했으나 프레임 본체의 실제 owner 연결이 남았다(5-A). `RawMinimapControls`는 2026-11-15 중단 시점의 처리기 초안과 집중 검사(최초 SID 풀 크기 오류 수정)까지이며 cpppj 전체 계약·원본 fixture·실제 메뉴/월드 허브 연결이 남았다(5-B). 두 항목은 체크 목록에서 완료로 세지 않는다.

1. AGENTS.md·이 문서·공통/dotnetpj 인계를 읽고 `git status`·`git fetch`를 확인한다. **같은 작업 트리에서 다른 세션이 cpppj·dotnetpj·공통 문서를 동시에 커밋할 수 있다(2026-11-06 실제 관측).** 공통 인계는 고치지 않는다. cpppj 소스는 사용자가 목표를 줄 때 cpppj 동시 작업을 따로 알리지 않으면 동시 작업이 없는 것으로 보고 필요한 수정을 할 수 있되(2026-11-17 사용자 지시, 1절 보호/분담) 완료 기능의 관련 검사를 통과해야 한다. cpppj 상태는 커밋 기록과 소스로 판단한다.
2. `node webpj/tools/port-map.mjs`로 대응표 검사와 기준선(행 base) 뒤 cpppj 변경을 본다. 이번 단위가 옮길 항목은 **3.1절의 현재 단계 목록에서 위에서부터** 고른다(대괄호 선행 관계를 지킨다). **기준선 판**의 소스(`git show 225553df:경로`)·그 파일을 대상으로 하는 cpppj 검사·fixture·단위 문서를 함께 읽는다. 기준선 뒤 그 파일이 바뀌었는지는 저장소 루트에서 `git diff --numstat 225553df HEAD -- <파일>`로 확인하고, 바뀌었으면 3.6절로 판정해 판정 커밋을 base와 단위 문서에 적는다. 디컴파일이 꼭 필요하면 마지막 수행 PC와 현재 PC를 먼저 비교하고 AGENTS.md의 재디컴파일 규칙을 따른다.
3. 원본/클론 게임(cpppj 오라클 수집 포함)·analyzeManager·별도 창 실행은 AGENTS.md의 확인/예외 규칙을 적용한다. 단위 검사나 정적 확인을 실제 실행으로 기록하지 않는다.
4. 3.0절 방식으로 구현한다. 필요한 공유 장치가 없으면 먼저 이식하고, 빈 성공 함수·별도 근사 월드로 우회하지 않는다. 멀티플레이 분기는 1절의 경계 규칙을 따른다.
5. 근거 커밋·연결 fixture/오라클(SHA/행 수)·검사와 최초 실패/교정·실제 연결/미연결·남은 경계·다음 시작점은 **단위 문서**에 남긴다. `PORT_MAP.tsv`의 web·base(정확히 8자리)·status·메모와 manifest(`node webpj/tools/fixture-manifest.mjs --write --only <아이디,…>`)를 갱신하고 `node webpj/tools/port-map.mjs` 오류 0을 확인한다. 이 문서는 **0.1절(머리글·진행도·마일스톤의 현재 단계·마지막 구현 커밋·다음 작업·마지막 전체 검사)·0.2절 진행 칸·0.3절 수치·0.4절 개수·3.1절의 해당 항목(`- [x]`와 단위 링크)·단계 머리글 `(n/m)`·4-D 진행판(0.1절 4-D 진행 행, 3.1절 구간 지도·계획 단계별 배분의 완료 줄과 현재 계획 단계)**만 고친다. 단계를 마치면 0.2절에서 다음 단계를 **진행 중**으로 바꾸고, 끝난 단계의 체크 목록은 보관본으로 옮겨 2.3절에 한 줄로 남긴다. 단위별 긴 설명과 기록 줄은 이 문서에 쌓지 않는다(7절). 이번 단위 파일만 이름으로 stage하고, 커밋 직전에 `git log --format=%s`로 그날 번호의 최대값(CP·DN·WB 공통 계열)을 다시 확인한 뒤 `YYYYMMDD-NN WB 요약`(끝에 Co-Authored-By 트레일러)으로 로컬 커밋한다. push하지 않는다.
6. **검사:** 커밋 전 전체 `node --test "tests/*.test.mjs"`(webpj 폴더)를 Node v26과 v22 두 버전으로 돌린다(1절 Node 지원; 2026-11-16 사용자 지시에 따라 v24는 생략·호환 가정). 문서 갱신 보조 스크립트는 저장소에 없으므로 이 문서는 위 5번 칸을 손으로 고치거나 필요하면 스크립트를 새로 만든다.

### 0.6 단계와 무관한 AGENTS 요구 (2026-11-11 점검, 2026-11-17 요약)

AGENTS.md가 세 클론 모두에 바로 요구하는 항목은 **모두 반영됐다**: 버전 11.01 표기·Credits 세 번째 항목 `Netstorm 11.01 Patch Credits`와 창 제목 `NetStorm 11.01 Patch Credits`·HJOW 개발과 AI 사용 내용(WB03-AC, 한국어 행 이름 `NetStorm 11.01 패치 크레딧` 확정), 카피라이트 `Copyright 2026 HJOW`(WB03-AE, [제품11.01](docs/webpj-product-1101-20261105.md) 6절, `tests/product.test.mjs` 단언), 메인 메뉴 전 "클릭하여 시작"(WB01), 외부 도메인/IP 접속 문서화와 클라이언트 유효성 검사 비활성화(WB02-E, [외부 접속](docs/webpj-external-access-20261105.md): 런타임 요청은 같은 출처 `./data/`뿐), 한국어 `fonts/` D2Coding woff2 사용, 에셋의 `webpj/www` 복사(`prepare-assets` → `www/data`). 점검 표 원문은 보관본 0.6절에 있다.

남은 조건부 일:

- 새 외부 요청(13차 CDN 글꼴 등)을 만드는 단위는 [외부 접속 문서](docs/webpj-external-access-20261105.md)와 `tests/external-access.test.mjs` 감사를 함께 고친다.
- `fonts/`의 Noto Sans·Noto Sans JP는 TTF뿐이라 쓰게 되면 Google Fonts CDN 대상이다(13차, 3.7절·4절 "다른 언어 글꼴"). 일본어 Patch Credits 본문을 추가할 때 카피라이트 줄도 넣는다(원본 비교 모드의 10.78 두 행 Credits는 그대로).
- Credits AI 문구는 4절 "Credits AI 문구" 결정(현행 유지 후 나중에 수정)을 따른다.

## 1. 범위와 유지되는 결정

- **계획 기준 (2026-11-06 재계획, 사용자 요청):** 웹은 멀티플레이와 Edit를 제외하므로 웹이 옮길 cpppj 코드는 사실상 고정돼 있다(cpppj 현황은 3.4절). 그래서 ① cpppj 기준선 고정(현재 `225553df`, 이후 변경은 3.6절 필터) ② cpppj 파일(클래스) 단위 이식과 해당 검사 이식(`PORT_MAP.tsv`로 범위·진행 관리, 3.0절) ③ 원본 x86 fixture 대조(L1)에 더한 cpppj 오라클 비교(L2~L4, 3.3절) ④ 내부 미션 전송·패킷·로그인 경로는 이식하고 LAN TCP·멀티플레이 UI·일반 전투 등만 제외(아래)로 진행한다. Outpost는 12차 스커미시와 함께 이식한다(아래 12차 예외).
- **단계 분할 (2026-11-10, 사용자 요청):** 순서3·4·5를 3-A~3-E·4-A~4-D·5-A~5-C로 나눴다(그때 이월한 경로·비행·총·의식 부품 6-P·7-P1~P3는 완료). 항목은 **현재 단계 목록에서만** 고르며, 기준선 뒤 변경이 있다는 이유로 건너뛰지 않고 기준선 판을 옮긴 뒤 변경을 판정한다(E213 선례). 뒤 단계 몫은 현재 단계가 그 부품을 요구하면(예: Session 생성자가 무조건 만드는 바인딩) 그 항목만 앞당긴다. 경위는 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)에 있다.
- **사용자 결정 (2026-11-05, 유효):** Credits AI 문구는 현행 유지 후 나중에 수정한다(4절). 한국어 행 이름 `NetStorm 11.01 패치 크레딧`은 확정이다. 원본 자료의 바깥 링크는 새 탭/창으로 연다(WB03-AD 구현). 개발 서버의 다른 PC 접속은 제한하지 않는다.
- **목표:** 원본10.78 싱글플레이를 HTML+JavaScript로 복원한다. 대상은 Early Missions6·Priest Training9·캠페인16의 **일반31경로와 데모3경로**다. 같은 자산을 쓰는 일반/데모도 별도로 취급한다([공통 인계1.2절](LEFT_JOBS.md)).
- **웹 범위와 멀티플레이 경계 (2026-11-06 정리):**
  - **이식 대상(싱글플레이에 필요):** 단독 미션·데모가 쓰는 Client/Session·World·State와, **내부 미션 전송(zlocal 양끝)·패킷 등록/길이/분배·미션 로그인·서버/클라이언트 미션 패킷·Zone 초기화**를 옮긴다. 원본 싱글플레이도 같은 프로세스 안에서 이 경로로 진행하므로, 이것을 빼면 미션이 돌지 않는다. 로그인 Ready·Player 번호·초기 SP 지정처럼 이름은 멀티플레이 같아도 단독 경로에서 실행되는 코드는 이식 대상이다.
  - **웹 제외:** LAN TCP 전송(`LocalTcpTransport`, 복합 전송의 TCP 원격끝, `CampaignTransportServices`의 TCP backend), Multiplayer 메뉴·LAN UI·참가자 목록·Commander, 일반 전투(State6·`saveBattle`/`!state.dat` 복구·Reconnect·서버 이전), 메타맵·Challenge·랭크, 공식 서버·클라이언트 유효성 검사. 또 Edit·커스텀 맵 작성/가져오기/저장·TEST01/TEST02 플레이·User Made Campaigns(`offical6`/`%{userSpec}` 목록), Quit/Exit, 게임 옵션의 해상도/전체화면, 설치/CD 탐지·레지스트리, Win32/DirectDraw/GDI/WINMM·Direct3D 11/FSR, cpppj의 C++ MCP 서버(웹은 4차를 WebMCP로 따로 구현, 3.7절). AGENTS 후순위 중 8차(AA·FSR)·9차(편집기 시나리오 편집)·14차(QuickJS, 웹은 불필요)도 제외다. 메뉴에도 노출하지 않으며 Edit 전용 CHEAT·`userCheat` 우회도 제공하지 않는다. 공통 Fort/지형 코드가 Edit에도 쓰인다는 이유로 캠페인용 이식을 막지는 않는다.
  - **12차 스커미시 예외 (AGENTS.md 사용자 커밋 `85551c6c`, 2026-11-11):** 스커미시는 웹 후순위 대상이다. 스커미시가 쓰는 MP 시작 규칙 월드 생성(중앙섬·참가자별 전투섬·MP 군도 밀도·MP 간헐천 생성기, cpppj 1293·1300)과 Campaign 메뉴의 Skirmish 항목·설정 화면은 순서 10에서 이식하며, 그 밖의 MP(위 웹 제외 목록)는 계속 제외한다. 해당 코드의 PORT_MAP 분류는 E498~E513에서 정했으며 실제 이식은 순서 10에 남았다(3.5절 5번).
  - **Outpost (2026-11-11 사용자 결정):** 스커미시 구현에 Outpost가 필요하므로 **스커미시를 구현할 때 Outpost도 함께 구현한다.** 원래 멀티플레이 전용이라 웹 제외였으나 이 결정으로 12차 범위에 들어온다. 이식 범위와 순서는 3.7절 "12차 Outpost 범위"다. 그 전까지는 지금처럼 Outpost 전용 경로를 명시적인 웹 제외 오류로 둔다.
  - **경계 처리:** cpppj 함수 안의 멀티플레이 분기는 원본의 단독 판정(`inSingle`·demo·server 조건)을 그대로 옮긴다. 단독에서 도달하지 않는 분기는 조용한 성공이나 no-op로 바꾸지 않고 명시적인 웹 제외 오류로 둔다(`MainLoop`의 웹 제외 단계 사유와 같은 방식). 직접 단독 관문이 없다고 해서 도달 불가로 단정하지 않으며, 근거를 단위 문서에 남긴다.
  - cpppj가 3차 작업에서 공유 파일(로그인 부모·Session·Player/SP setter·패킷 분배 등)을 바꾸면 단독 경로에 영향이 있는지 3.6절로 확인한다.
- **cpppj 기준 (2026-11-06 재정의):** 목표는 원본 10.78 싱글플레이 복원이며, 그 근거로 원본 기계어와 대조된 cpppj 기준선(`225553df`)을 옮긴다. cpppj의 파일·클래스·함수 경계와 이름, 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 보존한다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 `DataView`/TypedArray로 같은 배치를 쓴다. cpppj의 미복원·부분 연결·명시 미연결·실패 경계는 임의로 채우지 않고 같은 지점의 명시 오류·진단으로 옮긴다. dotnetpj의 근사를 정답으로 복사하지 않는다. 기준선 이후의 cpppj 변경은 3.6절로만 반영하고 미커밋 변경은 쓰지 않는다.
- **Node 지원 (2026-11-16 사용자 변경 지시):** 웹 코드·시험의 필수 실검사는 **Node v22·v26**이다(`webpj/package.json` `engines` `>=22`). **Node v24 검사는 생략하고 두 필수 버전이 통과하면 호환된다고 가정**한다. 이 가정을 v24 실검사 완료로 기록하지 않는다. HJOW-Athlon의 기본 node는 v26, v22는 `C:\Program Files\nodejs22\node.exe`다. Node v26 최적화 JIT가 신호 NaN 비트를 바꾸므로 NaN 비트를 대조하는 시험 파일은 맨 위에 `import './nan-exact.mjs'`를 둔다(NaN 비교를 느슨하게 하지 않음).
- **바깥 링크 (2026-11-05 사용자 결정, 2026-11-07 WB03-AD 구현):** Tell/도움말 링크의 바깥 주소는 원본의 셸 실행 대신 **브라우저 새 탭/창으로 연다**. 치환이 끝난 `http:`/`https:`/`mailto:`만 열고 `file:`·미치환 자리표시 등은 이유와 함께 콘솔로 알린다. 조건과 판정은 [외부 접속 문서7절](docs/webpj-external-access-20261105.md)을 따른다. 개발 서버(`server.js`)는 다른 PC 접속을 제한하지 않는다(모든 인터페이스 수신 유지).
- **구조:** `index.html` 수정은 최소화하고 `netstorm.js`가 앱 수명을 맡는다. `.css` 파일 없이 JS로 스타일을 지정한다. Canvas의8비트 인덱스/팔레트 합성과 최종 표시를 분리한다. 코어는 DOM/Canvas/Web Audio에 의존하지 않는다. 기존 CommonJS 서버와 `www/package.json`의 ES 모듈 경계, `www/{assets,core,render,ui,audio,platform}`를 유지한다. C++/C# 서버나 WebAssembly를 필수 런타임으로 추가하지 않는다.
- **시작/수명:** 사용자 시작 입력 안에서 오디오를 활성화한다. 미션 재시작·메뉴 복귀·`destroy/init`의 타이머/선택/음악/이벤트 소유권을 구분한다. 숨겨진 탭은 진행을 정지하고 복귀 때 시계 기준을 다시 잡아 부재 시간을 따라잡지 않는다(웹 적응).
- **화면/시간:** 영어4:3부터 비교하며 게임 영역의4:3·16:9·16:10, 입력 역변환·픽셀 비율을 다룬다. 현재768높이/1024·1228·1366너비는 임시 화면 정책이다. `requestAnimationFrame`은 표시 기회이고 게임 틱이 아니다. 요소별 원본 간격을 먼저 맞추고60/120프레임은 후순위로 둔다. 가장자리 스크롤은 미션이 입력을 받으며 포인터가 게임 영역 안에 있을 때만 동작하고 이탈/포커스 상실 시 멈춘다.
- **자산/저장:** 기본 소스는 `assets/game-data`·`fonts`·`webpj/translations`이며 실행 자료는 `webpj/www/data`에 준비한다. 느슨한 파일→TAFF 조회·타입/프레임/팔레트 의미·manifest 출처/해시를 보존한다. 웹 실행에서 보호 원본이나 로컬 설치를 요구하지 않는다. 설정/진행은 버전이 있는 JSON으로 localStorage에 두고(`netstorm.settings`·`netstorm.progress`, AGENTS 5차의 웹 처리와 같음) 저장 불가/손상 때도 계속할 수 있게 한다. 큰 저장소 도입은 실제 필요가 생길 때 판단한다.
- **언어:** 영어 원본 `.chfnt`, 한국어는 사용자 지시대로 `fonts/D2Coding.woff2`·`D2CodingBold.woff2`를 복사해 사용한다. TTC 추출로 되돌리지 않는다. 한국어 파일/섹션/주제가 없으면 영어로 대체한다. 번역 문구 검수는 **개발자가 발견하는 대로 요청**한다(2026-11-01). 메뉴/도움말 번역 완료를 미션 한국어 완료로 세지 않는다. 명칭은 AGENTS.md와 [공유 명칭](docs/korean-unit-names.md)을 따른다. 미션 브리핑·안내의 한국어는 cpppj 한국어 확정본과 같은 원문을 쓰는 것을 기본값으로 한다(3.5절 2번). 일본어 등 다른 언어는 순서 10(13차)에서 추가하며 woff2가 없는 글꼴은 Google Fonts CDN을 쓴다.
- **사용자 확인 경계:** [공통 인계0.3·1.1·3절](LEFT_JOBS.md)에 맞춰 **모든 자연 완주·전체 동일성·실제 사제 포획/희생·특정 적 공격·승리 재현/자동 공략은 사용자 확인 또는 별도 요청 대상**이다. 구현·정적 대조·허용된 부품 검사는 계속하되 이 항목만 남으면 다음 단계로 간다. 공중3종(훨리기그·맨오워·더스트 데빌)은 정적/코드 이식만 하며 새 전용 생산/생성/우클릭/행동 관측·검사를 추가하지 않는다.
- **보호/분담:** AGENTS.md·보호 원본/CD를 수정하지 않는다. **공통 인계 LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md와 dotnetpj 파일은 WB 단위에서 수정·커밋하지 않는다**(2026-11-06 사용자 주의). **cpppj 동시 작업 여부는 사용자가 목표를 제시할 때 명시한다. 별다른 언급이 없으면 cpppj 동시 작업이 없는 것으로 보고 필요한 cpppj 소스 수정도 할 수 있다**(2026-11-17 사용자 지시, 2026-11-16 수정 허용 지시를 이어받음). 동시 작업이 명시되면 그 지시를 따른다. 어느 경우든 완료 기능을 보존하고 관련 기존 검사를 확인한다. stage할 파일은 이름으로 지정한다. README 등 공통 파일은 기존 사용자 결정대로 일괄 갱신 때 반영한다. 새 문서/코드는 UTF-8, 새 상수·함수·자료형·클래스·멤버·반복문에는 설명/사용법을 먼저, 추가/수정 시점과 이유를 뒤에 쓰는 한국어 주석을 단다.

## 2. 완료 범위 요약

여기 있는 항목은 해당 부품의 완료이며 실제 캠페인/전체 동일성 완료가 아니다. 월드 부품은 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 단위별 상세·최초 실패·남은 경계는 각 단위 문서와 `PORT_MAP.tsv` 메모의 "남음:"을 따른다.

### 2.1 기반·메뉴·자료·월드 기초 (WB00~WB04-D5c)

| 단위 | 완료 범위와 현재 경계 | 근거 |
| --- | --- | --- |
| WB00·WB01 | 계획·ES 모듈 진입·클릭 대기/실패 재시도·Canvas/기본 입력·앱 수명 | [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a) |
| WB02·WB02-D | 자산 준비/manifest·TAFF/Config/GameFiles/GameData/Tell 조회·영어 글꼴 | [데이터층](docs/webpj-data-layer-20261028.md) |
| WB02-E·F | 웹 런타임 외부 접속 목록과 소스 정적 감사(0.6절) / 이식 추적 `manifest.json`·`tools/fixture-manifest.mjs` | [외부 접속](docs/webpj-external-access-20261105.md)·[이식 추적](docs/webpj-fixture-trace-20261105.md) |
| WB02-G·H | 이식 대응표 `PORT_MAP.tsv`·`tools/port-map.mjs`(분류 규칙·기준선 뒤 변경 보고) / 무브라우저 실행·cpppj 오라클 수집·비교, M1 8장면 대조 | [이식 대응표](docs/webpj-port-map-20261106.md)·[오라클 비교](docs/webpj-cpp-oracle-20261107.md) |
| WB03-M1·D~L·I | 원본 메인 메뉴, Tell/Campaign/Demo/Credits 목록, Help/Options·설정, F1/링크/스크롤/커서, 시작 환영/팁, `Done*` 저장. Edit 버튼과 User Made Campaigns 행은 제거(캠페인 최상위 6행). 미션 명령·Auto-Demo·현재 미션 문맥은 미연결 | [메인 메뉴](docs/webpj-main-menu-20261023.md)·[대화상자](docs/webpj-tell-dialog-20261028.md)·[목록](docs/webpj-campaign-lists-20261029.md)·[진행 저장](docs/webpj-progress-storage-20261030.md)·[시작 창](docs/webpj-startup-dialogs-20261030.md) |
| WB03-N~S | D2Coding·xlat·메뉴/Tell/캠페인 목록/도움말 한국어와 영어 대체. 미션 본문은 순서 8 | [글꼴/메뉴](docs/webpj-korean-20261031.md)·[Tell](docs/webpj-korean-tell-20261031.md)·[도움말 완료](docs/webpj-korean-help-types-20261101.md) |
| WB03-T | 자산 타입 116개의 프레임/그림 해석(남은 표시는 4절) | [도움말 그림](docs/webpj-help-pictures-20261101.md) |
| WB03-U~AB | `.fort`/188타입 플래그·영역 배치·본섬 마스크·지면/절벽·받침·소유자 색표 계산(465맵 독립 대조, 실제 미션 적재 증거 아님), CRT 정렬·64바이트 카메라·공통 그림/그림자 출력·개발용 지형 화면, 값형 Fort 객체 로더(실제 월드 연결은 4-B) | [Fort](docs/webpj-fort-reader-20261101.md)·[영역](docs/webpj-territory-layout-20261101.md)·[마스크](docs/webpj-terrain-mask-20261101.md)·[지면](docs/webpj-terrain-tiles-20261102.md)·[절벽/받침/색](docs/webpj-terrain-extras-20261102.md)·[출력](docs/webpj-world-render-20261102.md)·[지형 장면](docs/webpj-terrain-scene-20261102.md)·[객체 로더](docs/webpj-fort-object-load-20261102.md) |
| WB03-AC·AD | 제품 표기 `v11.01`·Patch Credits 창(0.6절) / 바깥 링크 새 탭·창 열기(실제 브라우저 확인은 3.5절 4번) | [제품11.01](docs/webpj-product-1101-20261105.md)·[외부 접속7절](docs/webpj-external-access-20261105.md) |
| WB04-A~C3 | 50바이트 SID 풀(1584전이 대조)·`SquidHash`/`RawSquidFinder`·장기 난수/게임 시계/Kernel·`GameAuxState`(보호 PE 초기값 51칸)·40칸 입력 큐와 `MainLoop` | [SID 풀](docs/webpj-sid-pool-20261104.md)·[해시/탐색기](docs/webpj-squid-finder-20261105.md)·[장기 상태1](docs/webpj-session-runtime-20261105.md)·[장기 상태2](docs/webpj-game-aux-state-20261105.md)·[큐/메인 루프](docs/webpj-main-loop-20261105.md) |
| WB04-D1~D5c | 공통 공간 해제 / `SquidUnpop`·삭제기 / 공통 Form 층 / `SquidFactory`(생성자 188칸) / Depend·Gump·Content Form 호스트 / Form 분배·Content 수명·보상/SP 저장소 / Process 호스트·Regular 공통 | [공간 해제](docs/webpj-squid-unpop-20261105.md)·[Unpop·삭제기](docs/webpj-squid-destroy-20261105.md)·[Form 층](docs/webpj-squid-forms-20261106.md)·[SquidFactory](docs/webpj-squid-factory-20261106.md)·[Form 호스트](docs/webpj-form-hosts-20261106.md)·[분배기·Content 수명](docs/webpj-content-lifetime-20261107.md)·[Process 호스트](docs/webpj-process-host-20261108.md) |

### 2.2 WB04-E 월드 부품 (단계 3-A, E1~E242)

E1~E242 단위 문서 링크 색인은 [보관본의 2.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)에 있다. 단위 문서는 `grep -l "WB04-E123" docs/webpj-*.md`처럼 번호로 찾는다(E1~E242 모두 검색됨). E243~E328은 2.3절, 그 뒤는 2.4절과 3.1절에 있다.

| 영역 | 단위 수 | 단위 번호 |
| --- | --- | --- |
| Player·소유자·State | 13 | E2~E9·E11·E13·E55~E57 |
| 객체 공통 수명·표시·명령·trace | 17 | E1·E14~E15·E24·E26·E41·E58~E61·E123·E213·E230·E235~E236·E240~E241 |
| 기반 유틸리티·입력 분배·패킷 길이·번역 장부 | 15 | E18~E23·E25·E27~E28·E33~E36·E232·E242 |
| 대본 명령·미션 설정·튜토리얼 판정 | 11 | E30~E32·E37~E38·E231·E233~E234·E237~E239 |
| Graph·다리·섬·Fort 메타 | 21 | E12·E39~E40·E42~E47·E161·E214·E220~E229 |
| 자원(너겟·간헐천·오벨리스크·마나·에너지) | 9 | E10·E16·E29·E71~E72·E75·E105·E107~E108 |
| 주문·Bomb·특수 건물 | 11 | E17·E48~E54·E109~E111 |
| 사제·신전·의식 | 17 | E62~E70·E78~E80·E106·E120·E162·E212·E219 |
| 운반·경로 | 20 | E73·E90~E103·E144~E145·E159~E160·E209 |
| 바리케이드·총·투사체 | 21 | E74·E81~E83·E85~E89·E112~E117·E124~E127·E210~E211 |
| 건설·배치(일반/사제) | 30 | E104·E128~E143·E146~E158 |
| AI | 21 | E76·E163~E182 |
| 유닛 생산·Dude 행동 | 10 | E121~E122·E185·E206~E208·E215~E218 |
| 시각 효과·구름·주변음 | 26 | E77·E84·E118~E119·E183~E184·E186~E205 |

### 2.3 순서3 마감·4-A~4-C (E243~E328)

체크 항목과 단위 링크 전체는 보관본에 있다: 3-B~3-E는 [순서3 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e264), 4-A는 [2026-11-11 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-through-wb04-e277)의 3.1절, 4-B는 [2026-11-11 이관 2](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-4b-through-wb04-e304), 4-C는 [2026-11-12 이관 3](LEFT_JOBS.webpj.old.md#archive-webpj-20261112-4c-through-wb04-e328). 단위 문서는 `grep -l "WB04-E259" docs/webpj-*.md`처럼 번호로 찾는다(E243~E328 모두 검색됨). 4-D 이후 단위는 2.4절과 3.1절 항목 끝에 있다.

| 단계 | 단위 | 완료 범위 |
| --- | --- | --- |
| 3-B 다리·표면·영역 공통 수명 | E243~E249 (8항목) | `o/` 8단위. 옛 단독 모델 SquidSpatial을 `exclude-legacy`로 제외(E246) |
| 3-C 자산 정기 처리·피해/삭제 수명 | E250~E256 (8항목) | `o/` 8단위(RawAssetPostPop·RawAssetRegular·RawLocalGeyser 등) |
| 3-D 공통 후처리·Graph 조합 마감 | E257~E258 (2항목) | RawGraph·SquidPostPop 조합 |
| 3-E 기준선 뒤 cpppj 변경 판정 | E259~E264 (6항목) | 판정 79단위, ② 재반영 6건, 나머지 ①/③(③ 반영 시점은 3.6절 표). 요약은 [판정-기타 문서 2절](docs/webpj-judge-misc-20261110.md) |
| 4-A 자료·파일·설정·타입 표 | E265~E273 (9항목) | 타입 레코드 완성과 Factory 연결(E265)·프레임 코드 표 검색(E266)·`.type` 해석(E267)·설정 층(E268)·원문 인코딩(E269)·번역표(E270)·파일 계층 adapt 8단위 대조(E271)·client 자료 읽기(E272)·자료 조회 순서 시험(E273). E272 이월분은 3.2절 4-B·4-D·5-A 칸 |
| 4-B Fort·지형·섬 적재 | E274~E304 (35항목) | Fort 파일 스트림·내용물·객체·이미지·Money·Template·DataManager 묶음(E274~E286), 캠페인 월드 적재 부모·Islandbuilder/TerrainBuilder 완료 확인(E287~E289), 정상 지형 부품(Context·ChunkMap·Growth·Bitmap·FrameChoose·Stamp·RegionPaint, E290~E296), 장기 저장소 3종(ProcessHost·Surface, E297~E298)과 `RawSurfaceWorld`(E299), GameWorld 이식 판정(옛 값형 어댑터 제외, E300), 지형 도장/일반 지형/Fort 적재 월드 연결(E301~E303), 실제 Tutorial 1 통합 시험(E304). 넘긴 것: 저장 객체 실제 적재·session-dump 대조(4-D), 기준선 뒤 장기 소유자 구조(3.6절 ③ 4-D), 객체 송신기 연결(4-C) |
| 4-C 패킷·로그인·내부 미션 전송 | E305~E328 (27/30단위) | 패킷 등록 표·분배기·연결 정리/등록/등록소/활동/폴링·송신 분배·Form/Squid 전송 writer(E305~E313), 로그인 서버·클라이언트 패킷·설정·Fort 이미지·타이머·로그인 부모·단독 미션 상태(E314~E320), 미션 플레이어 목록·건설 요청·간헐천 채집 등록(E321~E323), 단독 미션 내부 전송 `LocalMissionTransport`(E324), Client 수명 전역 3종(E325)·로그인/전송 서비스(E326)·복합 조정자와 바인더(E327)·Client 전송 생성기와 기본 패킷 등록 순수 부분(E328). 넘긴 것: ClientCampaignTransport 2단계·RawCampaignSessionLogin·통합 시험(4-D) |

### 2.4 4-R·4-D·5-A·5-B 진행분과 선행 부품 (E329~E632)

E329~E495의 체크 항목과 단위 링크는 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)의 3.1절·3.8절에, E496~E584에서 끝난 체크 항목은 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)의 3.1절에, E585~E632에서 끝난 체크 항목과 `RawCampaignSession 1`의 구간별 진행 기록은 [2026-11-17 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632)의 3.1절에 있다. 단위 문서는 `grep -l "WB04-E570" docs/webpj-*.md`처럼 번호로 찾는다.

| 범위 | 단위 | 완료 범위 |
| --- | --- | --- |
| 6-P·7-P1~P3 이월 부품 | E349~E371 | `o/` 경로 4단위와 프로세스 클래스(6-P 3/3)·비행 7(7-P1)·총/투사체 6(7-P2)·신전/제단/의식 7(7-P3) |
| 5-A 렌더러·Draw·시각 시스템 (44/46) | E372~E463 | 렌더러 소단위·수집·Draw·팔레트·입자·시각 시스템·KenCloud·GumpDraw, `RawMenuControls`가 쓰는 Gump 계열 모듈(E410~E432·E448). 시각 효과·입자 영역은 E390에서 모두 이식 |
| 6단계 선행 커서 부품 | E398~E401 | 커서 상태·수명 기반 4단위 |
| 5-B 카메라·HUD·미니맵 (9/13) | E464~E495 | 초기 카메라·카메라 입력, HudDock·HudDividerInput·InventoryGump·InventoryItemGump 분리(E470~E473), 사용자 선택·후보 선택·다중 선택(E495) |
| 3.8 원본 관측 연결 | E465~E495 | 기준선 `cpppj/tests` 감사로 찾은 미연결 원본 관측 약 4만 3천 행 연결. 첫 실행 대조에서 나온 이식 결함은 `applyHtmlCondition` 숨김 깊이 감김 1건. 허브·적응 판정 4건만 남음 |
| 4-R 재기준선 | E496~E514 | 구름 큐 고정 칸(E496), 제품 1313 오라클 후보·M1 8장면 재대조(E497), 신규 176단위 분류(E498~E502·E512), 기존 이식분 변경 37단위 판정과 ② 재반영(E503~E511), 대응표·오라클 전환(E513), 기존 웹 부품 상태 판정(E514) |
| 4-D A·B·C 선행 (16항목) | E515~E524·E561~E573·E583 | 시간 동기화 수신/정렬/Poll(E515~E517), 소유권·월드 설치·미션 Start/Tutorial·내용물 수명·생산 Deck(E518~E522), Guide 교체/해제(E523~E524), 자산/건설·사제 월드·Hit·GunQueries·애니메이션 Host·Archer·이동/경로·커서(E561~E569), ClientGameplayGlobals(E570), SoundProcess(E571), GameAux 공용 입력(E572), TypeNames/변환표(E573), BombBlast(E583) |
| 5-C·6·7·8·10 선행 부품 | E525~E560 | State·시간 알림·장식/사제 정기(E525~E528), Sprite 정기·아비어리 수신(E529~E530), 객체 소유자 명령(E531), 인트로·title·메인 메뉴 State(E532~E534), 게임 메뉴 명령·신전 명령 입력·지식 창·명령 표·메뉴막대(E535~E539), 사제/객체/Player/Factory 메뉴(E540~E546), 커서 잠금·위치·마나·사정거리(E547~E550), 하드너·트리즌·힐 주문 대상(E551~E553), 11차 난이도 계산(E554~E555), Outpost 목록 수명(E556), 전투 이탈(E557), 전투 옵션·연결 종료/감시·원격 Tell·기술 69h·전투 자원 프레임(E558~E560) |
| 4-D C 선행 `o/`·client 부품 (15항목) | E587~E632 중 | Vortex 이탈 부모(E587)·Territory 공유 writer(E588)·Damageable 정기 표(E590)·ProcessForm writer·Factory World 대여·장기 Factory·공용 Squid 송신(E591~E594)·건설 실패 패킷(E602)·Dais Process 자료(E617)·Fort snapshot 부모·TypeNames/Deck·내용물·객체/청크·Objects/영역·Territory writer(E627~E632, 편집기 전용 제외에서 캠페인 공유로 교정) |
| 4-D `RawCampaignSession` 생성자 typed 조립 (진행 중) | E574~E626 | 생성자 990..1880 전체 — 기반 owner·공유 입력·Player/SP·Form/방송(E574~E576), 섬/사제·Player/Template·Carrier/TYPE 파생표·Gun/커서 표시·피해/Hit·GunHit/투사체·소유자색/파편·Teleport·Archer/Missile(E577~E586), Vortex·자산·Runtime 훅(E589·E590·E595), InstallContents·Factory(E596~E599), Flare·건설 재생 Run/라벨·Construction·Damageable·Ice·CarrierSteal·주문·CarrierRegular·Aviary·ArcSpire·Flyer·Fence·Gem·삭제 분배·Regular(E600·E601·E603~E615), 송신 합성·World Runtime 게시·stream reader·Template/MakeRuntime·AI(E618·E619·E621~E625), Runtime pin(E626). 모두 cpppj 독립 시험과 같은 typed 분기이며 Native 분기·외곽 공개·Load는 3.1절 계획 |
| 공통 사건 반영 | E616·E620 | 최초 실행 상태(cpppj1314, 독립 Chromium e2e 2/2)·cpppj1320 내부 미션 전환 flush/종료 수신 재반영 |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식 (2026-11-06 재계획)

- **단위 크기:** 한 단위는 cpppj 파일 하나 또는 함께 움직이는 묶음(같은 클래스의 `.cpp/.h/.inc`와 생성 자료)과, 그 파일을 대상으로 하는 cpppj 검사로 이룬다. 함수 하나씩 나누지 않는다. 큰 파일은 공개 API 경계에서 나누고 남은 부분을 단위 문서와 PORT_MAP 비고에 적는다.
- **대응표 `webpj/PORT_MAP.tsv`:** 열은 `cpp`(저장소 기준 cpppj 경로)·`class`(분류)·`web`(웹 모듈 경로, 여럿이면 `;`)·`base`(이식·대조를 마친 cpppj 커밋)·`status`·`note`다. 분류는 `port`(이식 대상), `adapt`(창·입력·소리·파일·시계·캡처처럼 웹 대체가 필요한 플랫폼 경계), `exclude-mp`(1절의 LAN/멀티플레이 제외 목록), `exclude-edit`(Edit·UberEditor·Editor 계열), `exclude-host`(DirectDraw·Direct3D 11/FSR·MCP·CD 탐지·레지스트리·설치·Win32 진단), `exclude-legacy`(raw 풀 이전의 옛 단독 모델로 production 소비자가 없고 raw 구현이 대체한 단위)다. 상태는 `todo` → `partial`(웹 모듈이 파일 일부만 가짐) → `port`(이식, L1/cpppj 검사 통과) → `session`(앱 수명 연결) → `oracle`(cpppj 오라클 비교 통과) 순서로 올리고 제외 분류는 `-`다. **분류는 `tools/port-map.mjs`의 `RULES`가 정하며 대응표에서 손으로 바꾸지 않는다**(규칙과 사유를 고친 뒤 `--write`, 사유 조회는 `--why 이름`). `base`는 웹과 대조·판정한 cpppj 커밋이며 `todo` 행에는 기준선 뒤 변경을 판정했을 때만 적는다. 이식 원본은 언제나 기준선 판이다. 새 `www/core/*.js` 모듈은 반드시 대응 행의 web 칸(여럿이면 `;`)이나 `WEB_ONLY`에 넣는다(빠지면 전체 검사 실패). dotnetpj의 [PORT_MAP](dotnetpj/PORT_MAP.md) 형식을 참고하되 별도 파일로 둔다.
- **검사 이식:** cpppj `tests/`의 대상 검사를 같은 이름의 Node 검사로 옮긴다. 원본 x86 fixture는 `cpppj/tests/fixtures`를 직접 읽고 복사하지 않는다. 웹 적응·제외 경계를 확인하는 웹 전용 검사는 따로 표시한다.
- **생성 자료:** 생성자·vtable·타입·패킷 길이처럼 큰 표는 cpppj 소스에서 생성 스크립트로 만들고 `--check`로 재생성 결과가 같은지 검사한다(WB04-D4 `squid-constructors.js` 방식).
- **수직 진행:** 범위를 넓히기 전에 Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 통과시킨다. 그다음 Early Missions·캠페인 1-1/1-2, 34경로 순으로 넓히며, 각 단계에 필요한 파일만 PORT_MAP에서 골라 옮긴다. 2026-11-10 단계 분할(1절)은 이 원칙으로 돌아가기 위한 것이다.

### 3.1 단계별 항목 목록

순서3(3-A~3-E)·4-A~4-C·4-R은 마감됐다(2.3·2.4절, 보관본). 현재 단계는 **4-D**이며 남은 일은 아래 4-D 계획(4D-1~4D-10)이다. 5-A·5-B는 4-D가 선행 대기로 막혔을 때 병행해 열었고, 5-C·6·7·8은 2026-11-15에 미리 펼쳤다. 끝난 체크 항목은 단계가 끝나기 전이라도 문서 정리 때 보관본으로 옮기며, 머리글 `(n/m)`에는 계속 센다.

- **진행 방법:** 단계는 0.2절 순서대로, 단계 안에서는 위에서부터 진행하고 대괄호 `[… 뒤]` 선행 관계를 지킨다. 항목을 끝내면 `- [ ]`를 `- [x]`로 바꾸고 끝에 ` → [E번호](단위 문서) 요약`을 붙인다. 한 항목을 여러 단위로 나누면 링크를 이어 붙이고 마지막 단위에서 체크한다. 단계 머리글의 `(n/m)`과 0.2절 진행 칸을 함께 고친다.
- **수치:** 줄 수는 기준선 소스 줄(partial은 파일 전체 줄), `(+N)`은 기준선(또는 행 base) 뒤 cpppj 변경 줄 수다. partial 항목은 PORT_MAP 메모의 "남음:" 부분만 해당한다.
- **기준선 뒤 변경:** `(+N)`이 있는 항목은 **기준선 판을 옮기고** 바뀐 부분을 3.6절 ①/②/③으로 판정해 단위 문서·PORT_MAP base·메모에 남긴다(E213 선례: 기준선 typed 경로를 옮기고 Native 분기는 3.6절 ③ 표에 적음). 변경이 있다는 이유로 건너뛰지 않는다.
- **목록 밖 단위:** 다른 단계 몫을 끼워 넣지 않는다. 항목이 선행 미완료로 막히면 사유를 그 항목에 적고 같은 단계의 다음 항목으로 간다. 단계 전체가 막히면 사유를 0.1절 다음 작업에 적고, 선행이 갖춰진 이월 부품(6-P·7-P) 항목을 진행할 수 있다. 목록에 없던 단위가 꼭 필요하면 해당 단계 목록 끝에 항목을 추가하고 이유를 적는다.
- **끝난 단계:** 단계 머리글이 `(m/m)`이 되면 그 체크 목록을 보관본으로 옮기고 2.3절 표에 한 줄을 더한다(0.5절 5번).

#### 4-D Session 조립·Tutorial1 Load/Start (44/85)

**완료 기준(3.2절): L2** — Tutorial1 Load/Start 관측(보고서·session-dump·첫 프레임 논리 화면)이 cpppj와 같다. 임시 Player/AI/색 번호로 시작 성공을 만들지 않는다. 선행 4-B·4-C·4-R은 완료다.

**진행판 (2026-11-17, E632 기준):**

- **어디까지 왔나:** 끝난 체크 44항목(E330~E632, 보관본)은 앞 단계 이월 부품·Session 바인딩과, Session 생성자 내부(`.cpp` 990..1880)와 그 생성자가 부르는 Connect*·저장 writer를 **cpppj 독립 시험과 같은 typed 분기**로 조립한 것이다. 외곽 Session(생성자 4318..4330)·`Load()`·Login·프레임·종료와 GameWorld/ClientMain, 실제 10.78 Client가 쓰는 Native 분기는 아직 없다. 그래서 웹 Session은 아직 Node 시험 안에서만 생성되며 Tutorial1을 `Load()`하지 못한다.
- **이정표:** ① 4D-2 완료 → Node 시험에서 typed Session `Load()`가 Tutorial1 저장 객체까지 적재 ② 4D-3 완료 → 내부 미션 전송 Login 경유 Start(상태 14·`LocalConnectionReady`)와 4-C 통합 시험 ③ 4D-6·4D-7 완료 → 웹 Client가 실제 Session/GameWorld를 소유 ④ 4D-8 완료 → 오라클(실제 10.78 Client)과 같은 Native 분기 ⑤ 4D-10 → L2. 메뉴에서 미션을 골라 HUD가 있는 화면으로 들어가는 실제 진입은 5-C(M2)다.
- **남은 양:** 4-D 배정 85단위 / 18,414줄(0.2절) = 기존 23 / 13,807 + 재기준선 62 / 4,607. 아래 항목에 85단위가 모두 이름으로 나온다(`RawCampaignSession*`은 여러 계획 단계에 걸친 한 행). 예상 작업 단위는 지난 E574~E632(58단위, 2026-11-16~17)의 속도로 추정한 값이다.

**`RawCampaignSession.cpp` 구간 지도** (기준선 `225553df` 6,406줄을 함수 블록 경계로 나눔. 헤더 712줄은 서비스 형식이라 각 단계에서 필요한 칸만 옮긴다):

| 구분 | 줄 (비율) | 기준선 `.cpp` 구간 | 처리 |
| --- | --- | --- | --- |
| typed 조립 완료 | 3,351 (52.3%) | 1..1883 머리·필드·생성자, 1884..2904 Connect* 묶음(ConnectFlare~ConnectAi), 3020..3226 InstallContents·Factory·stream reader, 3345..3360·3392..3506 TypeNames·섬 population·MakeRuntime·Territory writer, 3654..3658 PrepareBuilding, 5825..5902·6003..6028 저장 writer | E574~E632. 이 구간 안의 Native 분기는 4D-8g |
| 4-D 몫 남음 | 2,003 (31.3%) | 4D-1 233 · 4D-2 656 · 4D-3 478 · 4D-4 136 · 4D-5 81 · 4D-6 236 · 4D-8 183 | 아래 계획 |
| 단독 도달 판정 | 269 (4.2%) | 4209..4279 전투 이탈·무승부·재접속·서버 이전(4D-5c), 4620..4642 메타 결과 통지(4D-7c), 5032..5147 개발자 명령·6400..6406 AI 디버그 명령·5773..5824 AddRandomObjects/CreateAt/DestroyAt(8단계 점 명령·대본 소비자와 함께) | 판정 뒤 이식 또는 웹 제외 오류 |
| 다른 단계 | 481 (7.5%) | 4552..4563·4742..4815 게임 메뉴·다리 품질(6), 4957..4975·5583..5649 운반/객체/사제 명령(6), 6126..6315 사제·지식·의식·기술·객체 메뉴·선택(6·7), 3227..3286·5688..5746 스커미시·중앙섬(10) | 해당 단계 |
| 웹 제외 | 302 (4.7%) | Existing Editor 적재·Checkpoint·회복(3287..3344·3361..3391·3507..3543·3570..3575·4352..4446·4483..4487·4510..4522), 일반 전투 재접속·자원(4100..4113·5747..5760), Save as(5911..5919·5983..6002) | 만들지 않는다(1절 경계 처리) |

**계획 단계별 배분:**

| 단계 | 내용 | Session `.cpp` 남은 줄 | 함께 옮길 단위 / 줄 | 예상 작업 단위 |
| --- | --- | --- | --- | --- |
| **4D-1** (진행 중) | 외곽 Session 공개·전체 snapshot | 233 | 제외→port 교정 예정 4 / 295 | 3~5 |
| 4D-2 | `Load()` 모듈 설치(typed) | 656 | 7 / 509 | 8~12 |
| 4D-3 | Login·로컬 연결(typed)·4-C 통합 시험 | 478 | 11 / 1,498 | 6~9 |
| 4D-4 | 프레임·카메라·Tutorial 게이트 | 136 | — | 2~3 |
| 4D-5 | 승패·종료·State 반납 | 81 | 7 / 626 | 3~4 |
| 4D-6 | GameWorld·Client 서비스 | 236 | 4 / 1,640 | 3~5 |
| 4D-7 | ClientMain·Client 전역 | — | 9 / 3,331 | 4~6 |
| 4D-8 | Native(10.78 Client) 분기 전환 | 183 + 완료 구간의 Native 분기 | 45 / 3,488 | 10~15 |
| 4D-9 | 웹 session-dump·오라클 시나리오 | — | 1 / 204 | 2 |
| 4D-10 | 재대조·L2 판정 | — | — | 2~4 |
| **합계** | | **2,003** | **84 / 11,296 + `RawCampaignSession*` 7,118 = 85 / 18,414**(4D-1 교정분 별도) | **약 45~65** |

**진행 방법:**

- **순서:** 4D-1부터 번호순이다. typed 경로(4D-1~4D-4)를 cpppj 독립 시험으로 먼저 세우고, Client 소유자(4D-5~4D-7)를 붙인 뒤 Native 분기(4D-8)로 오라클과 같은 조건을 만든다. 한 단계 안에서도 위 항목이 아래 항목의 입력이다. 선행 미이식으로 막히면 사유를 그 항목에 적고 다음 항목으로 간다.
- **근거:** cpppj 줄 번호는 기준선 `225553df` 판이다(`git show 225553df:cpppj/src/client/RawCampaignSession.cpp`). 큰 항목은 공개 API 경계로 나눠 착수 때 하위 항목을 더한다. 기준선 뒤 변경은 3.6절로 판정한다.
- **검증:** 각 항목의 cpppj 시험(대부분 `cpppj/tests/CampaignSessionTests.cpp` 250건 중 해당 시험)을 같은 이름의 Node 시험으로 옮긴다. 사제 포획/희생 재현·특정 적 공격·승리·공중3종 전용 생성/생산/행동 시험은 1절 경계대로 옮기지 않는다.
- **조립 방식:** 4D-5까지는 앞 단위처럼 Client 계약(최소 덕타입)을 모듈 머리글에 적고 시험이 그 형태의 장치를 만든다. 실제 Client/GameWorld가 생기는 4D-6·4D-7 뒤에 4D-10a에서 같은 시험을 실제 조립으로 다시 대조한다.
- **끝난 항목:** 이번에 옮긴 15개(4-D 선행 부품 E587~E632, 옛 C 구획)는 [2026-11-17 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632), 그 전 16개·13개는 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)·[2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)의 3.1절에 있다. 요약은 2.4절이다. 머리글 분모가 59에서 85가 된 것은 남은 일이 늘어서가 아니라 옛 큰 항목 15개(`RawCampaignSession 1~4`·`GameWorld`·`ClientMain` 등)를 아래 41항목으로 나눴기 때문이다.

**4D-1 외곽 Session 공개·전체 snapshot** (진행 중, 다음 시작점 4D-1a)

- [ ] **4D-1a 전체 snapshot 자식 writer 4단위** — `SnapshotTemplateBody`(5924..5982)가 부르는 `SaveFortMoney`·`SaveFortMission`·`SaveFortBadges`·`WriteFortCoreData`
  - 대상: o/RawFortMoneySave 49 · RawFortMissionSave 69 · RawFortBadgesSave 93 · RawFortCoreWrite 84(.cpp+.h, 현재 `exclude-edit` `[edit-fort-save]`)
  - 방법: E627~E632처럼 캠페인 공유 호출(신전 상승·의식 완료·기술 획득의 snapshot)을 확인해 `tools/port-map.mjs` 규칙을 고친 뒤 이식한다. 대응표 대상이 1,014에서 1,018단위로 늘 것으로 예상한다
- [ ] **4D-1b 전체 snapshot 본체** — SnapshotFort/SnapshotBattle 5903..5910 · SnapshotFortBody/SnapshotTemplateBody 5920..5982 · RefreshCurrentPlayerCore 4447..4482 · WriteFortCoreDataBody 4488..4509
- [ ] **4D-1c 외곽 callback 대상** — LearnNewTechnology 6080..6099 · GenerateLocalGeysers/GenerateWorldGeysers 5650..5687(Login 타이머의 `createInitialGeysers`) · PlaceBattleNuggets 5761..5772
- [ ] **4D-1d 외곽 생성자·소멸자 앞단** — 생성자 4318..4330(`learnNewTechnology`·`vortexSnapshot`·`writeCurrentFortCore`·`createWorldGeysers`·`placeBattleNuggets`·`refreshLocalRitual` 훅과 `gameplayUse->Publish`), 소멸자 4333..4343의 `BindRuntimePin({})`·`DetachConnectionTransport`(State 증빙·Withdraw는 4D-5a)
- 검증: cpppj `CampaignFortSnapshotTests`·`CampaignFortObjectsSnapshotTests`·`CampaignSession_TechnologyUsesCurrentLifetime`·`CampaignSession_LocalGeysersUseCurrentInputs`·`CampaignSession_LocalGeysersUseActualAiRegistration`·`CampaignSharedGameplay_TwoActualSessionsPublishAndRetireSameGlobals`·`CampaignSharedGameplay_ForeignAndPartialConstructionRetireLease`

**4D-2 `Load()` 모듈 설치 (typed)** — cpppj `Load()` 4344..4351의 순서는 ConnectBuilding → ConnectMotion → `runtime->Load()` → ConnectDevastation → ConnectCarrierDrop → ConnectPriestPrayer → MakeBuilder → ConnectGameplayPlacement → ConnectCursor → `loaded`이다. 실제 Client의 Login 콜백(4184)도 같은 `Load()`를 부른다. 하위 `o/` 모듈은 RawManoSpell을 빼고 모두 이식돼 있다.

- [ ] **4D-2a ConnectBuilding** 3668..3771 · ReceiveGeyserMined 3772..3776 — 건설 Data/Process/Arrival/Effects·ManaBolt Regular/Arrival·FactoryDraw·ShapeToBuffer·GeyserMined
- [ ] **4D-2b ConnectMotion 앞부분** 3782..3929 — RawCampaignMotionBindings(E568)의 실제 설치, Carrier Move/Mining/Choice/Escape/Transfer/Target/Pickup, PathRetarget, BridgeCarriers, BoardTarget, PlayerPlacementAnchor
- [ ] **4D-2c ConnectMotion 뒷부분** 3930..3981 — RawCampaignPanic 12 · RawCampaignRitual 161*(ConnectDaisLifecycle·ConnectRitual, E595 PrepareRitual 뒤 나머지) · Salvage 명령(RawSalvageCommand)
- [ ] **4D-2d AI 행동 Runtime** — ConnectAiMovement/ConnectAiRuntime/AiBridgePath/AI 조회/StartMotion 2905..3019(ConnectMotion 3961이 설치)와 AI 공개 전달 4834..4956
- [ ] **4D-2e 파괴·주문** — RawManoSpell 76(재기준선 todo) · RawCampaignDevastation 163*(E584 파편 자격·E585 타이머 펜스 뒤 나머지) · RawCampaignMeltdown 20(ConnectDevastation 끝에서 설치)
- [ ] **4D-2f 운반 투하·기도·건설 사제** — RawCampaignCarrierDrop 53 · RawCampaignPrayer 24 · MakeBuilder 3544..3569
- [ ] **4D-2g 배치·커서** — ConnectGameplayPlacement·MayPlaceGameplay·ConnectCursor 3576..3653(RawCampaignCursor E569의 실제 설치)
- [ ] **4D-2h `runtime->Load()`와 `loaded`** — RawCampaignWorldRuntime(E519)의 실제 저장 객체 적재(4-B 이월: `FortObjectLoad`/Money/Meta child와 session-dump 대조 준비), typed Tutorial1 `Load()` 통합 시험
- 검증: `CampaignSession_ActualWarBeginsGameWorld`·`CampaignSession_ActualMasterGameWorld`·`CampaignSession_AiMovementUsesRealPathNamesAndStop`·`CampaignSession_AutomaticAiFramesUseCurrentTimeAndWorld`·`CampaignSession_ConstructionBuilderUsesActualMovementAndTime`·`CampaignSession_ConstructionArrivalRunsActualProgressAndVisual`·`CampaignCarrierDrop_*`·`CampaignPriestPrayer_*`·`CampaignPriestPanic_*`·`CampaignDevastation_*`·`CampaignRitualProgress_Actual307RuneConstruction`·`CampaignSession_Cursor*`

**4D-3 Login·로컬 연결 (typed) → 4-C 통합 시험** — 실제 Client 경로는 `StartLocalConnection` → `BeginLocalConnection` → `Login.Begin` → Login 콜백에서 `Load()`와 `RegisterCarrierCommands`·`RegisterConstructionCommands`·`RegisterFactoryResearchCommands`(4179..4190)다.

- [ ] **4D-3a RawCampaignSessionLogin** 389 — 행 base `a328ac1f` 뒤 32커밋을 3.6절로 판정하고 캠페인 Login(구조·StateHooks·`Begin`·`InstallZone`·Pump)을 옮긴다. 일반 전투 `BeginGeneral`/`InitializeGeneral`/`LocalFort`/원격 Fort는 웹 제외 오류로 둔다
- [ ] **4D-3b 로컬 연결 공개 API** 4146..4200·4292..4317과 **ClientCampaignTransport 2단계**(378*, 4-C 이월: `InstallClientPacketDefaults`의 채팅 41h·연결 종료 2번·기술 목록 0x69·경로 자료/압축 Squid 관찰기)
- [ ] **4D-3c 전송 앞단 4단위 / 340줄** — RawClockSynchronization 99*(고정 Native 수신기 baseline·Session 단독 연결 잔여) · RawPacketDelay 108 · RawPacketFrame 44 · ClientPacketSources 89
- [ ] **4D-3d Squid 수신 5단위 / 391줄** — RawSquidReceive 88 · RawSquidMutationReceive 106 · RawSquidCompressedReceive 67 · RawSquidVerification 62 · RawPeerWorldSync 68, Session ReceiveSquid·Dais·Gun·Aviary·Flyer 5148..5261
- [ ] **4D-3e Login 콜백 명령 등록** — RegisterCarrierCommands 5262..5436 · Construction 명령/수신 4976..5031 · Factory 연구/업그레이드 4280..4291 · ChatDevices·mutable 송신·SendRemoteTell 4114..4145 · AllianceDevices 4201..4208
- [ ] **4D-3f 4-C 통합 시험** — cpppj `CampaignSession_LocalLoginActualHandshakeAndWorldLoad` 대응: Zone→접속→0x27→응답→월드 설치→상태 8·9·10·13·14, 패킷 종류가 0x27로 시작해 0x28·0x29를 포함하고 0x6e로 끝남, 옵션 22값·반경² 900·TYPE122 비용 3000. 함께 `CampaignSession_InitRequestUsedByActualLogin`·`CampaignSession_ActualLoginUsesClientCurrentCoreSlot`·`CampaignSession_WorldInputActualLoginThreeCarriersAndKernel`·`CampaignSession_LocalLoginMissionStartAiAlliances`·`CampaignSession_LocalLoginWorkshopHasActualPlacement`·`CampaignSession_CarrierMoveRequestThroughActualLocalTransportAndKernel`

**4D-4 프레임·카메라·Tutorial 게이트**

- [ ] **4D-4a 카메라·프레임** — Camera 3982..3999 · ResetCamera/RequestCameraCenter/CommitRequestedCamera/CenterCameraOn/ViewHome/ViewYourPriest 4523..4551 · StartConstruction/PrepareFrame/RunFrame/CameraGlobals/SetParticleView/ParticleSprites/Particles 4564..4587(입자 뷰는 5-A 연결)
- [ ] **4D-4b Tutorial 게이트·표시 장치** — MakeTutorialOne/Two/Later 6033..6075(웹 `raw-campaign-tutorial-*-bindings.js`의 실제 생성) · DrawTempleFrames 3659..3667 · DrawRainTemple/DrawTemple/DrawBridgeAdditional 6115..6125
- 검증: `CampaignTutorialOne_ActualSessionFrameAndInputPauseOwnership`·`CampaignTutorialTwo_ActualSessionOptionsAndInitialFrame`·`CampaignTutorialLater_ActualSessionsAndThreeDwordEvents`·`CampaignViewCommandsTests`·`CampaignSharedCamera_*`

**4D-5 승패·종료·State 반납**

- [ ] **4D-5a 종료 수명** — CheckMissionOutcome 4588 · State별 정리/반납 4816..4833(ClearProcessesForState·ReturnProcessBindingsForState·BindRuntimePin·ClearPlayerLinksForState·ReturnPlayerLinksForState·ResetAiForState·ReturnAiForState) · ReleasePlayerLinks/StateChildrenReturned/ReleaseEditorCommandReceivers/~Impl 4000..4057 · 외곽 소멸자의 State 증빙·Withdraw
- [ ] **4D-5b Client State 정리 5단위 / 427줄** — ClientStateWorldGlobals 112 · ClientStateWorldCleanup 102 · ClientStateStorageCleanup 71 · ClientStateRuntimeRetirement 99 · ClientUserSelectionCleanup 43
  - 막힘(E336 판정): `ClientStateWorldContext`가 `Client`·`CampaignTemplateStorage`·`RawMenuControls`·`ClientAudio`·`RawSurfaceStorage`의 실제 저장소를 요구한다. 4D-6·4D-7에서 Client 소유자가 생기면 풀린다(RawMenuControls·ClientAudio는 5-C·8단계 몫이라 그 칸은 명시 미연결로 둔다). ClientUserSelectionCleanup은 `RawCampaignSession.core()`와 `RawSquidSelection` 표시 갱신 연결을 기다린다
- [ ] **4D-5c 판정 구간** — 4209..4279(FinalizeBattleLeave·RejectDraw·RequestForcedAlliance·PollReconnectSave·BattleGate·BattleFinished·TakeServerForState·heartbeat·SaveServerFailure·BroadcastConnectionPlayer·RunConnectionLoginTimer)의 단독 도달 판정과 RawBattleCompletion 143 · NativeBattleReturnState 56(메타·전투 경계의 단독 도달 앞단, E502)
- 검증: `CampaignSession_StateRuntimeRetirement_*`·`CampaignStateRetirement_*`·`StateRuntimeCoordinator_*`·`CampaignSession_PlayerLinks_*`·`CampaignSharedKernel_FinalDrainPrecedesPlayerAndServiceRelease`·`CampaignSession_MissionOutcomeSharesCurrentWorld`

**4D-6 GameWorld·Client 서비스**

- [ ] **4D-6a GameWorld** 1,157의 4-D 몫(E300 판정 호스트 구역) — 캠페인 생성자(기준선 86..115: 화면 크기·카메라 Reset·선택/삭제 효과·`RawCampaignSession` 생성·공장 표시·`Load`/`LoadLocalConnection`)·Begin/Start/PrepareLocalConnection·CompleteLocalConnection·`~GameWorld`·ResetCampaignCamera·RefreshCampaignObjects. CompleteFortInstall(Existing Editor)·옛 값형 어댑터는 제외하고 입력 구역은 6단계다
- [ ] **4D-6b Client 서비스 3단위 / 483줄** — CampaignClientServices 373 · CampaignUserInputServices 36 · CampaignCursorClientEffects 74*(E348 막힘 해소 조건: Session 서비스 형식과 GameWorld)
- [ ] **4D-6c Session 공개 조회·장치** — 4590..4619 · 4643..4741 · 6029..6032·6076..6079·6100..6114 · 6316..6399(커서·에너지·Dude·이동 시작·World/Core/HP/Priests 조회)

**4D-7 ClientMain·Client 전역**

- [ ] **4D-7a ClientMain 단독 Client 조립** — ClientMain 2,122*의 Client 전역 소유자 생성(장기 Template/Storage/Session과 Client 수명 전역)과 `RawSurfaceStorage` 장치 결합(기준선 442..533: BindChecking·BindDisplayDiagnostics·BindGraphTrace와 Patch1078의 EnablePatchDestroyDiagnostics·BindPostPopDevices·BindTransmissionDevices·BindPlayerRegularState·BindResourceDevices … BindAllocationDiagnostics 약 27호출. Native 분기는 4D-8과 함께 켠다), ConfigInterface 356*의 현재 미션 연결
- [ ] **4D-7b 메인 루프 단독 단계** — 연결 감시·전투 자원 프레임·재접속 저장(3.6절 ③ `44e53bdc`·E511)의 단독 도달 판정과 이식, ClientDiagnostics 157 · ClientStaticStrings 78. 7차 console 로깅 구조(3.7절)를 여기서 앞당길 수 있다
- [ ] **4D-7c 메타 단독 앞단 5단위 / 618줄** — ClientMetaPacketReceivers 205 · ClientChallengeGlobals 180 · NativeMetaConnectedState 112 · ClientMetaSpriteRefresh 56 · NativeMetaSpriteRefresh 65(두 Sprite 갱신은 5-C 화면 허브와 함께 확인), Session MetaMenuType·NotifyPlayerResults 4620..4642 판정
- [ ] **4D-7d 채팅·HTML·렌더러 프레임·점 명령 전역 5모듈** — 완료: ClientChatManagerGlobals(E447)·ClientDotCommandGlobals(E443)·ClientChatWindowGlobals([E453](docs/webpj-client-chat-window-globals-20261114.md))·ClientHtmlWindowGlobals([E456](docs/webpj-html-window-gump-20261114.md)). 남음: ClientRendererFrameGlobals 26*(0.5절 초안, 5-A `ClientRendererFrame` 조립 때 실제 owner를 연결하고 체크)

**4D-8 Native(10.78 Client) 분기 전환** — cpppj 실제 Client는 10.78(`Patch1078`)에서 시작 때 `RawSurfaceStorage`에 Native 송신·자원·공정 장치를 결합하고(`ClientMain.cpp` 442..533), 그 뒤 `UsesNativeTransmissionDevices()`가 참이 된다. Session `.cpp`에만 이 분기가 153곳 있다. E574~E632는 cpppj 독립 시험과 같은 typed 분기로 조립했으므로, 오라클(실제 Client)과 L2를 맞추려면 이 단계가 필요하다. Storage 멤버는 여기서 옮기고 전투·의식 소비 결선의 나머지는 6·7단계에서 잇는다.

- [ ] **4D-8a RawSurface 삭제·수명·공통 11단위 / 578줄** — RawSurfaceAssetDeletion 95 · RawSurfaceAssetNotice 52 · RawSurfaceAttack 100 · RawSurfaceContent 77 · RawSurfaceDecoration 24 · RawSurfaceDeletion 46 · RawSurfaceDepend 42 · RawSurfaceFrameRegular 20 · RawSurfaceLifetime 55 · RawSurfacePlayerRegular 38 · RawSurfaceRegistryDeletion 29
- [ ] **4D-8b RawSurface 자원·생산·사제 9단위 / 399줄** — RawSurfaceCarrier 83 · RawSurfaceDudeRegular 62 · RawSurfacePriest 22 · RawSurfacePriestDeletion 24 · RawSurfaceFactoryRegular 53 · RawSurfaceMana 54 · RawSurfaceResidenceRegular 37 · RawSurfaceReward 33 · RawSurfaceGump 31
- [ ] **4D-8c RawSurface 전투·의식 9단위 / 540줄** — RawSurfaceBalloon 31 · RawSurfaceBomb 112 · RawSurfaceDamageable 97 · RawSurfaceFence 85 · RawSurfaceFlare 32 · RawSurfaceIce 41 · RawSurfaceDaisDeletion 44 · RawSurfaceRitual 63 · RawSurfaceVortex 35
- [ ] **4D-8d Native 건설·Vortex 8단위 / 718줄** — ClientConstructionGlobals 253 · ClientConstructionRegions 123 · ClientConstructionRandomPlacement 57 · ClientConstructionPlacement 54 · ClientConstructionDevices 36 · ClientConstructionGem 34 · AssetDeletionVtables 86 · RawCampaignVortex 75*(Native 건설 Body/Runtime 대여 2766~·BindVortexDevices·51h 등록). 입력·건설 나머지는 6단계
- [ ] **4D-8e Native 고정 수신기 6단위 / 445줄** — NativeLoginPacketReceivers 92 · NativeGameplayPacketReceivers 89 · NativeCarrierPacketReceivers 62 · NativeConstructionPacketReceivers 63 · ClientChatPacketReceiver 42 · FortCoreDataLoad 97, Session ReceiveFixedAlliancePacket·ReceiveFixedLoginPacket 4063..4099 · ReceiveFixedGameplayPacket·ReceiveFixedCarrierPacket 5437..5582, 3.6절 ③ "고정 Native 수신기 묶음"·"접속 통지·패킷 출처"
- [ ] **4D-8f 공통 후처리·Fort 가상** — SquidPostPop 709*(벌룬·비행·Dais 몸체 `setBody`·실제 World 몸체 등록·기본 장치/자원 후처리·AI 통지) · NativeFortSaveVirtual 99
- [ ] **4D-8g 완료 구간의 Native 분기 재대조** — E574~E632 각 단위 메모의 "Native 장기 … 잔여"를 Session `.cpp`의 `UsesNativeTransmissionDevices()` 분기와 하나씩 대조해 옮긴다. 3.6절 ③ "World 수명·장기 owner 묶음"과 앞 단위가 미룬 연결(D5b 보상 삭제 장부 `reward_lifecycle_*`·D5c World 장기 Host 대여와 `ProcessHostStorage`·Kernel procSumm)을 함께 닫는다
- 검증: `CampaignSharedStorage_*`·`CampaignSharedKernel_*`·`CampaignSession_PriestAuthorityReadsSharedOwnerWithoutPrepare`·`CampaignSession_GeyserAuthorityReadsSharedOwnerWithoutPrepare`·`CampaignSession_StateSidLists*`와 Native 고정 fixture를 쓰는 `*Native*` 시험

**4D-9 웹 session-dump·오라클 시나리오** (3.5절 1번)

- [ ] **4D-9a SessionStateDump** 204(adapt) — cpppj와 같은 판번호 1 TSV(SID 풀 해시·사용 슬롯 원문·자산 해석·Kernel/Regular·Player 9·동맹 81·SP 비트·Graph·TotalMade·난수)를 만드는 웹 덤프. 근거는 [덤프·고정 시각 문서](docs/exe/cpp-session-state-dump-20261109.md)
- [ ] **4D-9b 오라클 시나리오** — `tools/cpp-oracle.mjs`가 `--ui-script`의 `session-dump` 단계·`--fixed-frame-ms`를 넘기는 시나리오 형식과 Tutorial1 Load/Start 수집(창이 뜬다: AGENTS.md 실행 확인 규칙, HJOW-Athlon 등 예외 호스트, 3.3절)

**4D-10 재대조·L2 판정**

- [ ] **4D-10a 끝난 항목이 남긴 재대조** — 실제 Session/Client 부모와 Native 장기 경로를 설치한 뒤 같은 시험을 실제 조립으로 다시 대조한다(보관본 각 항목 끝 메모 요약)
  - 4-D 선행 바인딩(E518~E573, 옛 C 구획)의 덕타입 Session 계약 시험(CampaignTestPlay 제외)
  - CampaignGuideRelease(E524): cpppj 시험 3건의 실제 Client+GameWorld+UserInput 조립. Mission·MissionGuideInput의 Tutorial1 게이트는 순서 8
  - ClientGameplayGlobals(E570): 전체 Session/Client 부모·외곽 owner/자식 증빙
  - GameAux 공용 입력(E572)·TypeNames/변환표(E573)·RawBombBlastBindings(E583)의 실제 Session 연결
  - 6·7·8단계로 넘기는 것: RawCampaignArcherBindings(E567)의 저장 미션 전체 Kernel/첫 비행(7), RawCampaignMotionBindings·RawCampaignPathBindings(E568)의 저장 미션 전체 초기화·L3(6), RawCampaignCursor(E569)의 실제 부모·에너지/MayPlace/Builder·마나/사정거리 결선과 저장 미션 시험(6), SoundProcess(E571)의 SoundPlayer/실제 브라우저 장치(8)
- [ ] **4D-10b L2 판정** — Tutorial1 Load/Start의 보고서·session-dump·첫 프레임 논리 화면이 cpppj 오라클과 같다(임시 Player/AI/색 번호 금지). 결선된 단위를 `PORT_MAP`의 `session`으로 처음 승격하고, 4-D 체크 목록을 보관본으로 옮긴다(0.5절 5번)

#### 5-A 렌더러·Draw·시각 시스템 연결 (44/46)

**완료 기준(3.2절):** 렌더러·Draw·팔레트·입자/표시 client 단위와 `o/` 4단위가 cpppj와 같은 순서·결과이고, 전용 시험 없이 웹 조립 검사만 한 E197·E198·E201 시스템을 cpppj Session 시험으로 재대조한다. 4-D의 막힘 항목이 모두 선행 대기라(E348 판정) E372에서 병행해 열었다. 끝난 44항목은 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495) 3.1절, 요약은 2.4절이다. 이미 있는 웹 메뉴 화면(M1 8장면)은 별도 웹 구현이며 이 목록은 cpppj 클래스 대응 기준이다.

- [ ] client **ClientRendererFrame** 396줄(+ClientRendererFrameGlobals.h) — 렌더러 프레임 본체. KenCloud 메뉴 연결을 포함하며 4절 "메인 메뉴 구름 배경" 차이를 0으로 만든다
  - 선행 완료: 렌더러·화면·커서/공장 표시(Renderer E391·Screen E393·TileGroup/Gump/Kencloud E394~E396·GumpDraw/GumpTextDraw E405~E406·CursorDisplay E407·ObjectHit/FactoryDisplay E408~E409·ClientGameplayGlobals E570)와 `RawMenuControls`가 쓰는 Gump 계열(Gump 기반·버튼·글자·스크롤·메뉴 E410~E432, KnowledgeSort·MinimapGeometry·InventoryAboutMenu E448). 단위별 링크는 [2026-11-17 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632) 3.1절 5-A에 있다
  - **남은 선행:** **RawMenuControls** 본체 1,605줄(5-C 메뉴 허브 항목)·**GameWorld**(4D-6a). 따라서 이 항목은 4-D와 5-C 메뉴 허브 뒤에 끝난다. 판정 근거는 [화면 문서](docs/webpj-screen-20261113.md) 5절
- [ ] **E197·E198·E201 시스템**(전용 시험 없이 웹 조립 검사만 한 것)의 cpppj Session 시험 재대조 [실제 Session 조립 선행]. 7단계 마지막 항목과 같은 일이면 한 번만 한다

**5-A 남은 단위:** ClientRendererFrame 394 · ClientRendererFrameGlobals* 26 · GumpVisual* 113(같은 렌더러 묶음, `ClientRendererFrame` 항목에서 함께 판정).

#### 5-B 카메라·HUD·미니맵 (9/13)

**완료 기준(3.2절):** 화면 요소별 cpppj 대조. 끝난 9항목(카메라 적용 E463 뒤 초기 카메라·카메라 입력·HUD 분리·선택, E464~E495)은 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495) 3.1절이다. 줄 수는 이전 기준선 `1f00a25a` 파일 줄 합(.cpp/.h/.inc)이며 현재 소스는 `225553df`를 확인한다.

- [ ] client **UberHud.inc** 154줄 — HUD 조립(HudDock·HudDividerInput의 실제 Uber 직계 연결). 재기준선 HUD 앞단 4단위(0.3절)의 단독 경로도 여기서 확인한다
- [ ] client **RawMinimapControls** 183줄(partial 초안, 0.5절) — MinimapGeometry(E448) 연결과 cpppj 전체 계약·원본 fixture·메뉴/월드 허브 연결
- [ ] client **FloatingSpGump** 소비자 연결·**RawProductionControls**(393줄) 표시 연결 — 허브/세션 선행으로 6단계와 함께 판정
- [ ] **커서 표시**(`Cursor` partial 125줄·`RawCursor*`) — 6단계 선행 커서 부품(4/4 완료)과 이어 5-B 마감 때 남은 것을 판정한다

**5-B 남은 단위:** UberHud 156 · RawMinimapControls* 181 · InventoryItemGump* 186(E470~E473 분리 뒤 잔여, 5-B 마감 때 판정) · Cursor* 123 · RawProductionControls 393 + 재기준선 4단위 / 407줄.

#### 5-C 게임 화면 허브·미션 진입 명령 → M2 판정 (0/6)

완료 기준(3.2절): **M2** — Tutorial1 진입 직후와 N프레임 뒤 논리 화면이 cpppj와 차이 0(L4a). 메인 메뉴에서 미션을 고르면 실제 Session이 만들어지고 HUD가 있는 미션 화면이 뜬다. 4-D·5-A·5-B가 선행이다. 큰 허브는 공개 API 경계로 나눠 항목을 더한다.

- [ ] client **Uber 허브** 3단위 3,034줄 — 게임 화면 최상위 Gump와 루트 컨트롤(HUD·덱·미니맵·채팅 창의 실제 소유자). 3.6절 ③의 `UberGump` 행을 함께 판정: UberGump* 2,686 · UberRootGump 187 · RawUberRootControls 161
- [ ] client **메뉴·버튼 컨트롤 허브** 6단위 2,650줄 — 모달·등록 소유자·링크/명령 소비자, 게임 버튼 묶음, 문자 입력·채팅 메뉴: RawMenuControls 1,605 · RawButtonControls 811 · RawMenuHtml 65 · RawMenuLoading 11 · RawTextEntryControls 110 · RawMenuChat 48. 5-A `ClientRendererFrame`의 선행이다
- [ ] client **메인 메뉴·타이틀 상태** 4단위 317줄 — 이미 동작하는 웹 메뉴를 원본 상태 소유자에 맞춘다
  - 완료: NativeIntroState 64·TitleBitmapLifetime 78·NativeMainMenuState 90(E532~E534, 원본 관측 일치)
  - 남음: **ClientTitleBitmaps** 85 — 끝나면 이 줄을 체크한다
- [ ] **미션 진입 명령 연결** — `MissionBegin`·`StartDemo`·환영 창 명령을 실제 Session 생성에 연결(메뉴 → Fort 요청 → State 전환). `?map=` 개발 화면과 구분한다
- [ ] **3.8절 허브 선행 fixture** — `chat-callbacks`(30행)·`chat-input-recipients`(721행)·`html-input-dialog` 생성/공장 20행을 허브 조립과 함께 연결
- [ ] **M2 판정** — Tutorial1 진입 직후·N프레임 뒤 L4a(영어 4:3 → 16:9/16:10 → 한국어), 오라클 시나리오 추가

재기준선 메타 Sprite 갱신 2단위(ClientMetaSpriteRefresh·NativeMetaSpriteRefresh)는 4-D 몫(4D-7c)으로 셌지만 화면 허브와 함께 확인한다(0.3절).

#### 6 입력·게임 메뉴·사제/운반·경제·건설 연결 → M3 (1/8)

완료 기준(3.2절): 같은 입력 단계표에서 cpppj와 같은 상태·화면(L3). **M3:** Tutorial1·2와 캠페인 1-1·1-2를 사용자가 브라우저에서 검증할 수 있다. `o/` 규칙 부품(경로 6-P·커서 선행 포함)은 이미 옮겼으므로 이 단계는 client 바인딩·입력·메뉴와 세션 결선이다. 3.6절 ③ 표의 6단계 행(`GameWorld::SelectPriest` NB20·건설 실제 wire·`UberGump` 건설 기록)을 함께 반영한다. 끝난 1항목(커서: RawCampaignCursor E569, RawCursorActions·RawCursorPosition·RawCursorMana·RawCursorRange E547~E550)은 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584) 3.1절이다.

- [ ] client **입력 본체** 2단위 163줄 — 40칸 사건 큐의 게임 입력 분배: UserInput 122 · GameWorldInput 41
- [ ] client **게임 메뉴** — ESC 가로/하위 메뉴·명령·Player 메뉴. 남음: **RawGameMenus*** 273(partial)
  - 완료: RawPlayerMenu·GameMenuActions·GameMenuCommands·RawGameMenuBar(E535·E538·E539·E544, 원본 관측 일치)
- [ ] client **객체 우클릭 메뉴** — 건물·운반체·특수 대상 메뉴와 소유자 명령. 남음: **CampaignObjectMenuClient** 229
  - 완료: RawObjectMenuInfo·RawObjectMenu·RawObjectMenuVariants·RawObjectMenuOpen·ObjectOwnerCommands(E531·E541~E543·E546, 원본 관측 일치)
- [ ] client **사제·운반·경로 바인딩** — 운반 입력. 남음: RawCampaignCarrierInput 42(`MakeCarrierInput`, GameWorldInput·CampaignUserInputServices가 사용). 운반 투하·기도·공황(RawCampaignCarrierDrop·RawCampaignPrayer·RawCampaignPanic)은 Session `Load()`/`ConnectMotion`이 설치하므로 2026-11-17 4D-2로 앞당겼다
  - 완료: RawPriestMenu(E540)·RawPriestWorldBindings(E563)·PriestPlacementAssets(E562)·RawCampaignPathBindings(E568)
  - E568 후속: 4-D Session 설치 뒤 cpppj CampaignMotion의 thewarbegins·masterofwhirligigs 저장 사제/목적지 선택·전체 초기화와 같은 경로 루프를 재대조한다
- [ ] client **건설·생산·경제 바인딩** — 배치·건설·생산 덱 재계산·생산 유지·다리 붕괴. 남음: RawProductionMaintenance 173 · ClientProductionSync 46 · ClientBridgeDecay 42 · ProductionControlContext 18
  - 완료(4-D 선행): RawCampaignProductionBindings(E522, E215 이월 시험 3건)·RawCampaignConstructionBindings(E562)
  - 재기준선 건설 장치 `ClientConstruction*` 6단위 중 4-D에서 옮기지 않은 입력·건설 몫도 여기서 연결한다(0.3절)
- [ ] client **공장·지식 메뉴와 기술** — 연구·업그레이드·지식 메뉴, 테마 기술 명령. 남음: ThemeTechnologyCommand* 48(partial)
  - 완료: RawFactoryMenu(E545)·RawKnowledgeMenu(E537)
- [ ] **L3 판정** — Tutorial1 입력 단계표(이동·수확·건설·생산)의 상태·화면이 cpppj와 같다. cpppj `--test-autoplay`(기준선 뒤 기능, 4-R 선행)로 만든 고정 시드 구간도 쓸 수 있다

**6 남은 단위:** 기존 10단위 / 1,034줄(2026-11-17 바인딩 3단위를 4D-2로 이동) + 재기준선 RawBuildReplay·ClientBuildReplay 2단위 / 199줄(둘 다partial: [E601](docs/webpj-build-replay-runtime-20261116.md)의 실행/라벨·현재 수명은 완료, FILE 판독/최신 rec·Host·키/선택은 잔여. 3.6절 ③ 6단계의 `UberGump` F7/Ctrl+F6 입력과 함께).

#### 7 전투·의식·AI 연결 (0/6)

완료 기준(3.2절): 현재 Player/기술/SP/명령/난수로 연결하고 cpppj 복원 범위를 타입별로 기록한다. `o/` 부품(총·투사체 7-P2, 비행 7-P1, 신전·제단·의식 7-P3, AI 34단위)은 이미 옮겼다. 임시 방어 AI를 만들지 않는다. 포획/희생·특정 적 공격·승리 재현과 공중 3종 전용 검사는 1절 경계를 지킨다. 11차 난이도는 이 단계 뒤에 붙일 수 있다(3.7절).

- [ ] client **전투 바인딩** — 피격·궁수·이동·총 조회·폭발·Damageable 정기 표(3.6절 ③ `SquidProcess` 발사/회전 TYPE 두 칸 포함). 표 이식 완료: CampaignDamageableRegularTables7줄([E590](docs/webpj-session-assets-20261116.md), 4-D 선행). **4-D 전체 Session 설치와 아래 저장 미션/전체 사격 루프 재대조가 남아 있어** 체크는 유지한다
  - 완료(4-D 선행): RawCampaignHitBindings(E564)·RawCampaignGunQueries(E565)·RawCampaignArcherBindings(E567)·RawCampaignMotionBindings(E568)·RawBombBlastBindings 45줄(E583, 파편 캡처 선행)
  - E567 후속: 4-D Session 설치 뒤 cpppj CampaignFire_ActualInitialPositions·ActualWarBeginsLaunchAndFlight와 저장 사제를 쓰는 세 factory/clock loop 전체 조립을 재대조한다(현재는 공장 부분만 검사)
- [ ] client **파괴·주문 효과** — 기폭 입력. 남음: DetonateInput* 26(3.8절 `detonate-input` 769행은 RawObjectConversion 단위에서). RawCampaignDevastation·RawCampaignMeltdown은 Session `Load()`가 설치하므로 2026-11-17 4D-2e로 앞당겼다
- [ ] client **의식** — 선택 입력. 남음: RitualChoiceInput* 43. RawCampaignRitual은 `ConnectMotion`이 설치하므로 2026-11-17 4D-2c로 앞당겼다
- [ ] client **AI 잔여·전투 이탈** — AI getter의 실제 자산 연결: AiTypeRequiredEnergy* 28. 완료: RawBattleLeave(E557, 원본 64행 일치)
- [ ] **AI 세션 결선** — `o/RawAi*` 34단위를 실제 Session의 Player·SP·명령·난수에 연결하고 AI가 있는 미션의 N프레임 L3 비교
- [ ] **E197·E198·E201 등 전용 시험 없던 시스템**의 cpppj Session 시험 재대조(5-A에서 이월된 항목과 같은 것이면 한 번만 한다)

재기준선 장기 월드 저장소 중 전투·의식 몫(RawSurfaceBomb·RawSurfaceDamageable·RawSurfaceFence·RawSurfaceFlare·RawSurfaceIce·RawSurfaceBalloon·RawSurfaceRitual·RawSurfaceVortex·RawSurfaceDaisDeletion)의 Storage 멤버는 4D-8c에서 옮기고, 전투·의식 소비 결선의 나머지를 이 단계에서 연결한다.

#### 8 미션 수명·안내·대본·소리·채팅·도움말 잔여 → M4 (0/9)

완료 기준(3.2절): **M4** — 일반 31 + 데모 3경로의 진입·진행·저장 수명. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분한다. 소리·음악·동영상은 Web Audio/브라우저 디코더로 바꾸는 adapt 단위라 적응 판정(3.6절 ①②)을 먼저 한다. 6차 한국어 미션 문구(3.5절 2번)와 7차 console 로깅 구조(3.7절)를 이 단계까지 끝낸다.

- [ ] client **대본·대화창** 2단위 484줄 — 튜토리얼 대본 실행과 대화창 본문 배치: DialogScript 296, DialogBodyLayout 188
- [ ] client **소리 본체** 3단위 1,688줄 — 효과음·위치 음향·장치 계층(Web Audio 적응): Sound* 824(SoundList 공개 API는 E571 완료) · SoundDevice 372 · ClientAudio* 492. 완료: SoundProcess 361([E571](docs/webpj-sound-process-20261116.md), 4-D 선행)
- [ ] client **음악** 5단위 1,186줄 — 곡 선택·장면 음악·재생 수명: SoundMusic* 455, SoundMusicRuntime 193, SoundMusicFile 148, SoundMusicSelection 95, SoundSceneMusic* 295
- [ ] client **동영상** 5단위 500줄 — 인트로 동영상 제어·재생(브라우저 디코더 적응, 제공 여부부터 판정): RawMovieControls 142, MoviePlayback 76, MoviePcmOutput 83, MovieLifecycle 80, SmackerDecoder 119
- [ ] client **도움말·Tell 잔여** 10단위 986줄 — QuickHelp partial 4건·HTML 조건/링크·Tell 파일 메뉴·바깥 도움말 정책·NsInfo 문맥: QuickHelpHtml* 184, QuickHelpControl* 174, QuickHelpWindow* 160, QuickHelpRequest* 57, HtmlConditionEffect* 97, HtmlLinkText* 53, TellFileMenus* 108, HelpLinkPolicy* 73, ExternalHelpClient 47, RawNsInfoContext 33
- [ ] client **글자·설정·그림 잔여** 5단위 1,097줄 — 비트맵 글꼴·한국어 자원·옵션 저장·GIF partial과 화면 캡처 진단: BitmapFont* 465, KoreanResources* 212, ClientOptionsDisk* 165, GifImage* 141, GifScreenshot 114
- [ ] **미션 수명 통합** — 튜토리얼 게이트/명령/정지, Retry/Next/Abort/메뉴 복귀, 실제 진행 저장, 데모 360초 반복·Auto-Demo(cpppj 946~948과 같은 조건)
- [ ] **3.8절 적응 판정 fixture** — `audio-reinterpret`(193행)·`font-face-enum`/`font-face-lookup`(389행)·`scenemusic`·`renderer-projection`
- [ ] **M4 판정** — 34경로 진입·진행·저장 수명 검사, 한국어 미션 문구

#### 끝난 선행 부품 (체크 목록은 보관본)

6단계 선행 커서 부품 4/4(E398~E401), 6-P 경로 부품 3/3(E349~E351), 7-P1 비행 유닛 7/7(E358 마감), 7-P2 총·투사체 6/6(E364 마감), 7-P3 신전·제단·의식 7/7(E371 마감)은 모두 끝났다. 체크 목록과 단위 링크는 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)의 3.1절, 요약은 2.4절이다.

### 3.2 실행 순서 (순서 4~10)

순서 1~3과 4-A~4-C·4-R, 이월 부품 6-P·7-P1~P3는 끝났다(0.2·2.3·2.4절, 각 행의 원래 내용은 보관본). 아래 단계는 3.1절 체크 목록으로 진행하고, 큰 단위는 공개 API 경계로 나눈다. 한 단계를 한 번에 완료 처리하지 않고 cpppj 파일 단위로 나누어 기록한다.

| 단계 | 할 일 | 선행 | 완료 기준 |
| --- | --- | --- | --- |
| **4-D** Session 조립·Tutorial1 Load/Start | 3.1절 4-D 계획 4D-1~4D-10: 외곽 Session 공개·전체 snapshot → `Load()` 모듈(typed) → Login·로컬 연결과 **4-C 이월**(`ClientCampaignTransport` 2단계·`RawCampaignSessionLogin`·통합 시험) → 프레임·카메라·Tutorial 게이트 → 승패·종료·State 반납 → GameWorld·Client 서비스 → ClientMain·Client 전역 → Native(10.78 Client) 분기 → 웹 session-dump·오라클 시나리오 → 재대조·L2. **4-B 이월(E304)**의 저장 객체 실제 적재는 4D-2h, 앞 단위가 미룬 연결(D5b 보상 삭제 장부·D5c World 장기 Host 대여)은 4D-8g, 3.6절 ③의 4-D 항목은 각 묶음 제목에 적은 계획 단계에서 반영 | 4-B·4-C·4-R | **L2:** Tutorial1 Load/Start 관측(보고서·session-dump·첫 프레임 논리 화면)이 cpppj 와 같음. 임시 Player/AI/색 번호로 시작 성공을 만들지 않음 |
| **5-A** 렌더러·Draw·시각 시스템 연결 | 남은 것: `ClientRendererFrame` 본체와 KenCloud 메뉴 연결(4절 구름 차이 0), 전용 시험 없이 웹 조립 검사만 한 E197·E198·E201 시스템의 cpppj Session 시험(`DamageableBurningSessionTests` 등) 재대조. 나머지(`Renderer`·`DrawSort`·타입별 Draw·팔레트 전환·입자/표시 client 단위·`o/` 4단위·E272 이월 색표/자산 조회)는 끝났다 | 4-D(실제 Session)·5-C 메뉴 허브 | Tutorial1 첫 프레임 논리 화면 일부(지형·객체)가 cpppj `snapshot` 과 같음 |
| **5-B** 카메라·HUD·미니맵·커서 | 실제 카메라·가장자리 스크롤, `UberHud`·덱·미니맵·`FloatingSpGump`·커서 표시, `o/` RawManaBoltRegular(클라이언트 카메라 투영 선행) | 5-A | 화면 요소별 cpppj 대조 |
| **5-C** 미션 진입 명령·M2 판정 | `MissionBegin`·`StartDemo`·환영 창 명령, 영어 4:3 다음 16:9/16:10·한국어 | 5-B | **M2:** Tutorial1 진입 직후와 N프레임 뒤 논리 화면이 cpppj와 차이 0(L4a). `?map=` 개발 화면과 구분 |
| **6** 입력·경제·건설 (WB06~08) | `UserInput`·선택/커서·QWASZX·ESC 메뉴·우클릭 메뉴, SP 저장/비용/회수/지갑·Deck/재충전/에너지, 사제 상태/이동/경로/수확/운반, 다리/건물 배치·빌더/생산·취소/실체화(client K·E·L 영역). E215 RawProductionDeck 의 재계산/Factory 상품 추가 시험 3건은 client RawCampaignProductionBindings 이식 때 대조 | 5-C·6-P | 같은 입력 단계표에서 cpppj와 같은 상태·화면(L3). **M3:** Tutorial1·2와 캠페인 1-1·1-2를 사용자가 브라우저에서 검증할 수 있음 |
| **7** 전투·의식·AI (WB09~10·WB12) | 총/투사체/피해·벽/타워·유닛/파생 효과, 신전/제단·지식/의식/보상/승패 조건, 공통 전략 AI 전체 행동·탐색/전략·종료 수명(client D·F 영역) | 6·7-P | 현재 Player/기술/SP/명령/난수로 연결하고 cpppj 복원 범위를 타입별로 기록. 임시 방어 AI를 만들지 않음. 포획/희생·특정 적 공격·승리 재현과 공중3종 전용 검사는 1절 경계를 지킴 |
| **8** 미션 수명·안내·소리 (WB11·WB13·WB15) | 튜토리얼 게이트/명령/정지·현재 미션 Tell·도움말/지식·한국어 미션, Retry/Next/Abort/메뉴 복귀·실제 진행 저장, 데모 360초 반복/Auto-Demo(cpppj 946~948과 같은 조건), 위치 음향/음악 수명·Wind Noise/좌우 교환, 채팅·메시지(`o/` RawMissionGuide, client G·H·I 영역) | 7 | **M4:** 일반31+데모3의 진입·진행·저장 수명. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분. 스크립트 도움말 Back은 원본 명령 재등록에 연결 |
| **9** 표시 품질·배포 (WB16) | 원본 간격 유지 뒤 60/120 표시·성능/메모리, Chrome/Edge/Firefox·GUI Linux, 정적 배포/하위 경로·새로고침·캐시/자산 버전/저장 호환 | 8 | **M5:** 개발 서버 없이 같은 공개 자료로 실행. 확인한 브라우저/OS·DPI·소리/저장 환경만 기록. 배포 제품이 정해지면 README 등 공통 문서 일괄 갱신 |
| **10** 후순위 요구 (AGENTS.md) | cpppj 4~14차 중 웹 해당분: 4차(WebMCP·와이드 화면)·5차(localStorage, 대부분 충족)·6차 한국어 미션 문구·7차 오류/예외 처리와 console 로깅·10차 알려진 10.78 버그 수정 중 웹 범위 해당분·11차 난이도·12차 스커미시(Outpost 함께 구현, 2026-11-11 사용자 결정)·13차 기타 언어. 8·9차는 제외, 14차(QuickJS)는 불필요. 차수별 처리·선행·cpppj 기준은 **3.7절** | 9 (차수별로 더 일찍 가능: 7차는 4-D, 6차 미션 문구는 8, 11차는 7 뒤) | 착수 때 차수별 체크 목록으로 펼침. 현행 기준선과 3.6절의 후속 ③ 판정을 따른다 |

**진행 기준:** M1 완료 → M2(순서 3 마감 뒤 4-A~5-C, Tutorial1의 실제 시작 상태와 표시) → M3(순서 6) → M4(순서 7~8, 범위 내 전체 싱글플레이) → M5(순서 9). M3/M4의 자연 완주·전체 동일성은 사용자 확인으로 남기며 AI 작업을 무기한 막는 완료 조건으로 두지 않는다. 사용자 제보는 해당 단계의 수정 작업으로 올린다.

### 3.3 cpppj 오라클 사용법

| 수단 (cpppj 기준선 실행 파일) | 얻는 것 | 창 |
| --- | --- | --- |
| `--run <자료> --window --mute-audio --frames N --set "k=v;…" --ui-script steps.tsv --ui-report report.tsv` (+ 환경 `NETSTORM_USER_DATA=<빈 폴더>`) | 프레임별 입력(click/right/key/esc/pointer 등)과 `snapshot`(팔레트를 적용한 논리 화면의 32비트 BMP)·`report`(메뉴/세션 관측 TSV). `tools/cpp-oracle.mjs`가 이 형태로 실행한다. `--no-audio`·`NETSTORM_CPP_NO_AUDIO`는 소리 장치가 없는 상태라 Options 소리 항목이 비활성으로 그려지므로 쓰지 않는다(WB02-H) | 뜸 |
| `--run <자료> --frames N --screenshot out.bmp` | N프레임 뒤 논리 화면 | 뜸 |
| `--mcp`: `netstorm_input`·`status`·`report`·`screenshot`·`wait_frames`·`record_start`/`record_stop` | 대화형 조작·관측·소리 포함 녹화(L4b 소리 시점·곡 선택) | 뜸 |
| `--dump-types`·`--inspect-fort`·`--inspect-mission`·`--dump-territories`·`--config-*`·`--inspect-*`·`--dump-font`·`--dump-gif` | 자료 해석 결과 | 없음 |
| `cpppj/tests/fixtures`(원본 x86 대조 자료) | L1 기대값 | — |

- **제한:** `--dump-world`는 저장된 Tutorial 미션만 받는 옛 `GameWorld` 보고서라 실제 Session 비교에 쓰지 않는다. 실제 Session 상태는 cpppj `session-dump`(3.5절 1번)로 볼 수 있으나 기준선 오라클 실행 파일에는 없다. 관측할 수 없는 범위는 단위 문서에 한계로 적는다.
- **같은 조건:** 같은 자료(`assets/game-data` 기준으로 cpppj 실행 폴더와 `webpj/www/data`의 해시를 대조), 같은 언어·화면 크기·설정(`--set`), 같은 시드와 프레임 단위 단계표, 같은 입력을 쓴다. 맞출 수 없는 조건(소리 장치 시각, 브라우저 시계 등)은 비교에서 빼고 기록한다.
- **실행 규칙:** 창이 뜨는 수집은 AGENTS.md의 실행 확인 대상이다(예외: HJOW-Athlon·VM-W11-Codex·vm-debian-codex). 수집은 해당 단위의 별도 단계에서만 하고, 결과 파일과 cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. `npm test`는 저장된 결과만 읽는다.
- **도구와 절차 (WB02-H):** 시나리오 묶음 `webpj/tests/fixtures/<묶음>-scenarios.json`에 단계표를 적고 `node webpj/tools/cpp-oracle.mjs --exe <기준선 EXE>`로 모은 뒤, `node webpj/tools/screen-compare.mjs --diff <폴더>`로 웹(무브라우저, 원본 비교 모드)과 비교한다. 차이는 고치거나 시나리오의 `expected`에 영역·픽셀 수·원인으로 적는다(영역 밖 0이어야 통과). 라벨 클릭은 cpppj `ControlPoint`처럼 스타일 명령을 뺀 일반 글자로 쓴다(`General Help - F1`). 기준선 EXE는 `cpppj/build`(다른 세션이 계속 다시 빌드)를 쓰지 않고 기준선 worktree에서 따로 빌드한다. 이 PC에는 현행 `extracted/webpj-oracle/225553df/`와 이전 `extracted/webpj-oracle/1f00a25a/`를 보존했다.
- **알려진 비교 차이:** 메인 메뉴 구름 배경(Kencloud 대 GIF 타일, 행별 1픽셀)은 비교 도구가 `background` 기준 장면으로 분류한다. 해소 시점과 근거는 4절 "메인 메뉴 구름 배경".
- **기준선 뒤 cpppj 가 고친 오라클 차이 (2026-11-10):** WB02-H 오라클 문서 5.1의 ③ 스크롤 화살표 색과 ④ Demo Tell 이름(`Demoville`)은 cpppj 가 원본대로 고쳤다. ② Options 의 Pause 행은 원문 조건(`00594fa4`/`005c85a4`)과 cpppj 가 같아 상태 차이(스크린샷은 미션 뒤 메뉴로 판단)로 남긴다. 제품 985의 옛 오라클은 보존했고 제품 1313 오라클은 E497에서 재수집했다. [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md)
- **의미:** 오라클과 같다는 것은 "cpppj와 같다"는 뜻이다. 원본과의 동일성은 cpppj가 원본과 대조한 범위만큼만 보증되므로 L1 원본 fixture 대조를 대체하지 않고 함께 쓴다. cpppj가 미검증·사용자 확인으로 둔 동작(자연 완주·포획/희생·특정 적 공격·승리)은 웹도 같은 경계로 둔다.

### 3.4 cpppj 현황과 webpj의 관계 (2026-11-17, cpppj 제품 1320 기준)

cpppj는 AGENTS 1~14차 구현과 M8 최종 배포 관문(1311)을 마쳤고, 2026-11-14 일시 중지 뒤 **2026-11-17 사용자 지시로 재개해 진행 중**이다(제품1320, 마지막 단위1321, [공통 인계 0절](LEFT_JOBS.md)). 웹은 E513에서 제품 1313으로 재기준선 전환했고, 그 뒤 cpppj1314(최초 실행)·1320(내부 미션 전환)은 E616·E620에서 재반영했다. 웹 후속 기능으로 판정한 ③은 실제 구현이 남았다. cpppj가 재개됐더라도, 사용자가 목표에서 cpppj 동시 작업을 명시하지 않으면 동시 작업이 없는 것으로 보고 cpppj 소스 수정이 가능하다(2026-11-17 사용자 지시, 1절 보호/분담).

| cpppj 범위 (제품 번호) | webpj 처리 |
| --- | --- |
| 1차 비멀티 UI(929)·Edit(943)·미션 수명(956)·내부 수명/FILE(980)·2차 호환(981) | 웹 이식의 기준 범위(기준선 판). 완료한 웹 키/reader/UI는 다시 세지 않는다. Edit 관련은 제외 |
| 3차 LAN·Outpost(~1279): 공유 파일에 `Actual*`/`shared*`/Native 연결이 쌓임 | LAN은 웹 제외. 공유 파일 변경의 단독 경로 영향은 3.6절로 확인. **Outpost는 12차 스커미시와 함께 이식**(1절, 3.7절) |
| 4~13차(4차 MCP·와이드 935~937, 5차 홈 저장 932~934, 6차 한국어 K1~K5 1270~1281, 7차 로그 1269, 8차 AA/FSR 939, 9차 시나리오 S1~S4 1277~1284, 10차 버그 34/34 판정·수정 10건 1285~1300, 11차 난이도 1292, 12차 스커미시 1293·1300, 13차 일본어 UI 1288) | 차수별 웹 처리는 3.7절. 6차 미션 문구는 3.5절 2번 |
| 11.01 카피라이트(1291) | **웹 반영 완료**(WB03-AE, 0.6절) |
| Frame Rate 60/75/120(1297·1298)·CD 10.72 문서(1299) | Frame Rate는 순서 9의 60/120 때 참고. 웹 목표는 10.78이라 10.72는 범위 밖 |
| 사용자 확인 경계(전체 동일성·자연 완주 등)·공중 3종 정적 작업만 | 웹도 같은 경계. 아직 없는 효과를 일반 성공으로 채우지 않고, 공중 3종 전용 생산·행동 검사를 추가하지 않음 |
| 14차 QuickJS(1301)·CD판 10.72 복원 보류(1302~1310)·M8 최종 관문(1311)·테스트 자동 플레이 `--test-autoplay`(1312~1313) | QuickJS는 웹 불필요, 10.72는 범위 밖. `--test-autoplay`는 현행 기준선 오라클에 들어 있어 6·7단계 L3 비교의 고정 시드 구간에 쓸 수 있다 |
| 재개 뒤 1314~1321(최초 실행·한국어 유닛 정보·미션 진입/전환·시험 기대값 교정) | 1314·1320은 E616·E620에서 재반영했다. 1315의 `xlat.korean` `Type` 항목 교정은 웹 번역표(메뉴·도움말 205줄)에 해당 항목이 없어 현재 영향이 없고, 8단계에서 cpppj 한국어 자원을 가져올 때 교정본을 쓴다. 1316·1318·1319·1321은 cpppj 시험만 고쳤으므로 그 시험을 웹으로 옮길 때 교정된 기대값을 쓴다 |

### 3.5 사용자 확인·요청이 필요한 것

1. **실제 Session 상태 비교 수단:** cpppj의 `--ui-script` `session-dump` 단계(SID 풀 해시·사용 슬롯 원문·자산 해석·Kernel/Regular·Player 9·동맹 81·SP 비트·Graph·TotalMade·난수, 판번호 1 TSV)와 `--fixed-frame-ms N`(가상 시계)은 현행 기준선 오라클 실행 파일에 들어 있다([덤프·고정 시각 문서](docs/exe/cpp-session-state-dump-20261109.md), E497·E513). 웹 쪽 남은 일(4-D): `tools/cpp-oracle.mjs`에 두 옵션을 넘기는 시나리오 형식과 같은 형식을 만드는 웹 덤프(재기준선 단위 SessionStateDump). 그 전까지는 `report`와 논리 화면 범위로 비교한다.
2. **한국어 미션 문구의 단일 원본:** cpppj는 6차 K1~K5(제품 1270~1281)로 공식 미션 `.korean` 본문(`assets/lang/korean/d/` 43개 등)을 제품에 반영했다. 웹이 같은 파일을 `webpj/translations`로 가져와 쓸지(BOM·줄바꿈은 준비 도구에서 정규화), 웹 번역을 따로 둘지 정해야 한다. 기본값은 "cpppj 확정본과 통일"이며 적용 시점은 순서 8(현재 미션 Tell·한국어 미션)이다. 현재 웹 `webpj/translations`에는 메뉴·도움말용 12개만 있다.
3. **오라클 수집의 실행 허용:** 예외 호스트가 아닌 PC에서 cpppj 창 실행이 필요하면 단위마다 확인을 받는다.
4. **바깥 링크의 실제 브라우저 확인:** WB03-AD의 새 탭/창 열림과 팝업 차단 여부는 실제 브라우저에서 확인하지 않았다(검사는 판정·앵커 생성까지).
5. **후순위 4~14차 착수 전에 정할 것 (3.7절, AGENTS.md `85551c6c`):** ① **12차 스커미시** — 결정·분류 완료: MP 시작 규칙 월드 생성 단위는 1절 예외로 이식 대상이 됐고(E500·E512 분류, 순서 10), Outpost는 2026-11-11 사용자 결정으로 함께 구현하며 금지 유닛 목록에도 cpppj처럼 Outpost를 둔다 ② **4차 와이드**의 웹 정책(AI 권고: 옵션 메뉴 해상도 항목 없이 창 크기·`?aspect=` 기준으로 게임 영역 16:9 논리 화면을 최대 1920×1080까지, 그래픽 배율은 그대로) ③ **4차 WebMCP** 도구 범위(조작·화면 캡처·소리 포함 녹화)와 대상 브라우저(착수 시점의 WebMCP 사양·지원 상태 확인).
6. **재기준선(4-R) — 결정·실행 완료:** 2026-11-15 사용자 결정에 따라 기준선을 제품 985(`1f00a25a`)에서 제품 1313(`225553df`)으로 올렸고 E513에서 전환을 마쳤다. 기존 행별 판정 이력은 보존했다(3.6절).
7. **cpppj 동시 작업 여부 — 결정 완료(2026-11-17 사용자 지시):** 사용자가 목표를 제시할 때 cpppj 동시 작업 여부를 명시한다. 별다른 언급이 없으면 동시 작업이 없는 것으로 보고 WB 단위에서 필요한 cpppj 소스 수정도 할 수 있다(1절 보호/분담·0.5절 1번). 동시 작업이 명시되면 그 지시를 따른다.

### 3.6 cpppj 변경 재반영

- **현행 기준선:** `225553df`(제품1313, E513). 기존 행별 판정과 아래 표의 미구현 ③ 항목은 계속 유효하다. 분류는 ① 웹 결과 영향 없음(구조 정리·진단·웹 제외 기능) ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫이다. cpppj fixture가 바뀌었으면 해당 웹 검사를 먼저 다시 돌린다.
- **기준선 이후의 변경:** 매 단위 착수 때 `node webpj/tools/port-map.mjs`의 `cpppj 변경` 줄을 본다(행 base 뒤 커밋을 단위별로 보고하고 안내를 붙인다). `exclude-*` 행은 ①로 기록만 하고, `port`/`adapt` 행은 ②/③을 판정해 ②는 `재반영`을 명시한 WB 단위로 반영한다. 판정을 마치면 그 행 base를 판정한 커밋으로 올리고 근거를 메모(①이라도)와 단위 문서에 남긴다. `fixture-manifest.mjs`도 같은 base 뒤 변경만 보고한다. 기준선 뒤 새로 생긴 파일은 `새 파일`로 보고되며 재기준선 때 분류한다.
- **현재 보고 (2026-11-17 재확인, `port-map.mjs`):** `cpppj 변경` **1건** — E616에서 cpppj1314의 ClientAudio·ClientMain·UberGump 3행을② 재반영/① 구조 정리로 판정해 base를 `bd2941f4`로 올렸다. 남은 것은 RawCampaignSessionLogin(행 base `a328ac1f` 이후 32커밋, 이식 때 판정). 이미 옮긴 단위 중 바뀐 37단위는 재기준선 때 E503~E511에서 ①/②/③으로 판정해 ②는 재반영하고 행 base를 올렸다. 그래서 2026-11-11(155건)·2026-11-15(92단위) 판정 대기 목록과 커밋 성격별 잠정표는 만료됐다(보관본). 남은 ③은 아래 목록이다.
- **재기준선 결과 (4-R, E497~E514):** 기준선 뒤 `cpppj/src`의 `o/`·`client/`·`zacket/`이 301커밋·768파일(+26,257/−3,270줄) 바뀌었고 남은 허브가 가장 많이 바뀌어(RawCampaignSession 5,715→7,118줄, ClientMain 1,797→2,133줄, UberGump 2,207→2,683줄) HEAD 판을 바로 옮기기로 했다. 오라클 재빌드·M1 재대조(E497), 신규 176단위 분류(E498~E502·E512), 변경 37단위 판정(E503~E511), 대응표·manifest 전환(E513), 기존 웹 부품 상태 판정(E514)을 마쳤다. 기존 fixture의 실제 `cpppjBasis`와 행별 `base` 이력은 보존했다. 10차 버그 수정·6/13차 표시 부호화처럼 순서 10 몫인 변경은 ③으로 남겼다.
- **판정을 마치고 반영 시점만 남은 ③ (아래 표):** 해당 단계에서 반영하면 이 표에서 지우고 대응표 메모를 고친다. Native/실제 연결과 아직 미구현인 후순위 기능을 포함한다. 행별 상세(기준 커밋·바뀐 함수·판정 이유)는 각 행 `PORT_MAP.tsv` 메모, 판정 문서([공통층 E259](docs/webpj-judge-common-20261110.md)·[Player·사제 E260](docs/webpj-judge-player-priest-20261110.md)·[섬·자원 E261](docs/webpj-judge-island-resource-20261110.md)·[건설·Dude E262](docs/webpj-judge-construction-dude-20261110.md)·[전투 E263](docs/webpj-judge-combat-20261110.md)·[기타 E264](docs/webpj-judge-misc-20261110.md)), [대응표 문서 5·9절](docs/webpj-port-map-20261106.md)과 [2026-11-11 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-through-wb04-e277) 3.6절 원래 표에 있다. 판정을 마친 ①·②는 PORT_MAP base·메모에 있다.

**4-D — 4-C 마감 이월** (4D-3·4D-8)

- `RawSurfaceWorld` 일반 Pop/삭제·대상·ProcessForm의 실제 Storage 송신 연결은 **E594 완료**. 임시 함수 대체를 제거했다. Native mutable Client/기본 Form writer와 전체 Session Runtime 공개는 잔여다([단위 문서](docs/webpj-surface-transmission-20261116.md))
- `RawSquidTransmission` 압축 수신 복호기 `RawSquidCompressedReceive`·월드 동기화 `RawPeerWorldSync`(E513 대응표 등록, 이식은 4-D 후속)
- `GameClock` 시간 동기화 보정(`3e9dc2c7`, 보정 0이면 같음)
- 아비어리 `ReadProcess`·`WritePacket`(E264), 공유 Regular **수신** 단독 도달 재확인(E259). `WriteFormPacket`은 **E591에서 단독 도달 확인·공용 Host 부모 이식 완료**이며 상위 Storage/World 송신 결선은 남았다
- `UberGump` 로그인 안내("Creating Islands."·`ShowLoginProgress`, `9cb0547f`~`11fbb988`) 단독 도달 확인
- 완료 기록: `RawFormPacket` 삭제 패킷 writer 헤더 선기록·진단 복귀(`b8cadde1`)는 E312에서 ② 반영 완료

**4-D — 고정 Native 수신기(canonical) 묶음** (한 묶음으로 처리, 4D-8e)

- `PacketRegistrationTable` `PersistCanonicalBaseline`·`canonicalReceive`(장기 고정 수신기 승격, `899c934dc`, E305)·`RawPacketDispatch` `CanonicalPacketReceiver`/`RegisterCanonical*`/`SnapshotReceiver`(`ec0b6e973`, E306)
- `RawConnectionActivity` 등록의 Native 고정 수신기(nativeEntry 0x004b6180) 대여 분기(`eeae45c69`, E309)
- `RawMissionLogin`의 Native Login 고정 수신기를 분배기에 대여하는 분기(E319)
- `CampaignTransportServices` `BindCompositeCampaignTransport`의 고정 Native 수신기 분기(종료 패킷 2/기술 0x69 의 `canonicalReceive`에 대한 `RegisterCanonical` lease 표식, `a76499e35`; 일반 수신기 재등록은 E327 완료)
- `ClientCampaignTransport` `InstallClientPacketDefaults`의 고정 Native 수신기 묶음(`RegisterCanonicalDirect`로 등록하는 C3 빈 수신기 004437f0와 Login/Gameplay/Carrier/Construction 수신기 — 웹은 일반 직접 수신기로 등록, canonical 승격은 이 묶음과 같이, E328)

**4-D — 접속 통지·패킷 출처·섬 조회** (4D-3c·4D-8e)

- `RawConnectionRegistry` 접속 통지 `StoreConnectPacket`/`SendConnectPacket`/`RegisterConnectPacketReceiver`(LocalMissionTransport 생성자의 type 1 수신기 등록 포함, E324; `621efab7e`·`efc78ee56`): 실제 TCP 접속의 초기 패킷 복제·LAN 전용이며 대응표 밖 신규 파일 `RawPacketCopy`와 고정 Native 수신기에 기댄다. 그 전까지 `buffers` 칸은 비어 있어야 한다(E308)
- `ClientConnectionGlobals` `PacketSources().Resolve(connection)`(Squid 대상 송신·전투 배송의 실제 Source 선택, `a76499e35`)·`PacketDelays()`(실소켓 지연 FIFO·`lag` 선택·`ResetDefault`, `ec0b6e973`) — `ClientMain`/`ClientCampaignTransport`/Meta의 소비 위치와 함께 단독 도달 여부 판정(E325)
- 4-B 이월: `RawIslandRuntime` NoWorld 자유 함수 3개(`RawIslandExists`/`Owner`/`Bounds`, 섬 저장소를 만들 때, E261)

**4-D — World 수명·장기 owner 묶음** (4D-7a·4D-8)

- `RawSurfaceWorld` 기준선 뒤 +780줄(`14fd017c`): 월드가 Display/PostPop/Pop/Unpop/Factory/Frame/Owner/Neighbors/Connect/Surface/BaseDestroy/VirtualDestroy/Common/Lifecycle/Events/Dependencies를 Storage 장기 소유자에서 빌림·Runtime pin·자산별 Bindings 15종·직렬화 전역 선택·`SynchronizePeer`/`TransmitDeletion`/`AttachBaseAsset`/`BindMetaRefresh`. E299 변형 확인이 관찰하지 못한 건물 공간·Form 층·감시 Form 생성/Unpop·진단/현재 TYPE 빌림 연결은 Tutorial1 Session 검사에서 확인한다
- `RawSurfaceStorage` 기준선 뒤 +413줄 자산 장기 소유자 연결(`14fd017c`)·SidPool 소진 복구 BindAllocation 연결(진단/회수/덤프/Tell, E298)
- `RawProcessHostStorage` 장기 기본 장치 보관(`BindLifecycleDefaults`/`BindFormFactoryDefaults`/`BindFormTransmissionDefaults`)·fire/rotate 애니메이션 TYPE 참조·`ClearCall` owner pin(E297, `417f22d2`)
- `SquidDestroyLifecycle` Native PATCH 분기(E213)와 같은 시기의 `RawSquidDestroy`·`SquidPostPop`·`SquidReward`
- 공통 Form/Process 층 Native 분기(`SquidFormDispatch`·`SquidProcess`·`SquidFactory`·`SquidFrame`·`DependFormHost`·`GumpFormHost`·`ContentFormLifetime`, E259)
- Native 조회기·getter: Player·사제·신전(E260), 섬·다리·자원 전역(E261), 건설·운반·Dude 공유 getter(E262), 전투 역할 getter·폭탄/펜스 생성자(E263), 플레어·아이스 타워·아비어리 `FlyerDiedIn`(E264), `RawBombTargets` 현재 TYPE 사거리 직접 읽기(레코드 `range` 칸은 E265 완료)
- `TellSection` 대체 tell 적재(`config.LoadFile`, 현재 미션 Tell 연결 때)·`SidPool` 패치판 할당 진단 복귀·소진 시 다리 회수(`BindAllocation`)
- `ClientMain` 메인 루프 단계(연결 감시·전투 자원 프레임·재접속 저장, `44e53bdc`)의 단독 도달·순서 재확인

**4-D~ (E335 이월, 4D-3c·4D-7)**

- `RawClockSynchronization` 본체(서버 응답 004c1e70·Commander 연결 004c1eb0·수신 004c1f60·Poll 004c2070; 데이터 구조 `ClockSynchronizationData`는 E335, 수신·정렬·Poll은 E515~E517 완료, 고정 Native 수신기 baseline·Session 연결이 남음)
- `RawMonitorTime`(상태 E335, 본체 `StepMonitorTime`·`SubmitTemporaryChatFormat`은 [E526](docs/webpj-monitor-time-20261115.md) 완료; Client Config writer·실제 Chat manager 부모는 4-D 조립)
- `RawPacketCopies.duplicate`/`adopt`의 실제 소비자(State+80 가변 Zone 복제, Session 조립)

**5-B 선행 완료(E593)** — `SquidDisplay` `BindView`/`CurrentView`와 Storage 현재 World/Client getter를 연결했다. 프레임 카메라/Renderer 상위 결선은 5-B 잔여다([단위 문서](docs/webpj-surface-factory-lifetime-20261116.md))

**6**

- `GameWorld::SelectPriest` NB20(`0817fa87`: P/R 사제 선택이 커서에 붙은 건설 조각을 먼저 취소 — `hooks.pieceHeld`/`cancelPiece`, `PriestSelectionShortcut` 새 필드; 선택 입력 이식과 함께 webpj 10차 대상, E300)
- 건설 실제 wire `ActualConfirm`·`ActualPlace`(4-C 패킷 writer와 한 묶음, E262)
- `UberGump` `PollBattleResources`·F7/Ctrl+F6 건설 기록 재생 입력
- `RawPlayerParticipation` 동맹 협상 키 단독 도달 재확인(E260)

**6-P·7-P2 이월** — `RawPathRuntime`의 `friend RawPathData` 웹 대응(6-P, E264), `RawGunHit.applyOverride` Rain 오버라이드(7-P2, E263)

**재기준선 판정 후속** (상세는 각 판정 문서)

- 4-D·5-C·6·9·10 — [E511](docs/webpj-rebaseline-hubs-20261115.md): `ClientMain` 장기 Client/Session 수명·프레임 장치, `UberGump` 미션/World·입력/화면 및 후순위 메뉴, `RawPlayerRegular` Template Runtime 보유, `RawRunePostPop` 같은 풀의 Dais 공유 검색
- 6·10 — [E507](docs/webpj-rebaseline-language-20261115.md) 언어·파일: `BitmapFont` 일본어 글꼴/폭, `ConfigFileRead`·`KoreanResources`·`BaseFile` 한국어 공식 미션 번들과 부분 번역 우선순위, `InputEvent` 조합 문자 일괄 입력, `LanguageOptions` 일본어 선택, `ProductPresentation` 일본어 Credits·난이도·스커미시, `QuickHelpWindow` 언어 전환 시 열린 창·캐시 수명, `TellFileMenus` 긴 제목 NB24·언어 전환 재적재
- 9·10 — [E510](docs/webpj-rebaseline-later-gameplay-20261115.md) 게임 규칙: `RawGameMenus` Frame Rate·Difficulty, `PriestSelectionShortcut` NB20, `RawAiCreate` NB10·난이도(계산·AiCreate 연결은 [E554·E555](docs/webpj-ai-difficulty-20261115.md) 완료), `RawCarrierPickup` NB18, `RawWorldGeyser` NB04, `RawWorldGeyserBounds` NB29·스커미시 참가자 수

**소비자 단위마다** — `GameAuxState` 새 장기 전역 칸(보호 PE 초기값과 함께, base `8defb347`; 총·발사·회전 프로세스 TYPE 세 칸은 E561에서 추가). `SquidProcess` 발사/회전 애니메이션 프로세스 TYPE 두 칸은 E561에서 반영 완료

### 3.7 AGENTS 후순위 요구(4~14차)의 웹 처리

AGENTS.md(사용자 커밋 `85551c6c`, 2026-11-11): "cpppj의 4~14차 목표 내용이 webpj 후순위 요구사항에도 포함된다(단 7차는 브라우저 console 출력, 4차는 WebMCP로 구현, 5차는 localStorage를 대신 이용, 14차는 플랫폼 자체가 JS를 지원하여 QuickJS 엔진을 쓸 이유가 없고, 8차·9차는 webpj 작업에서 제외)". 순서 10(3.2절)에서 차수별 체크 목록으로 펼친다. cpppj 기준은 [공통 인계 0.4·1절](LEFT_JOBS.md)의 제품 번호다.

| 차수 | 요구 요약 | 웹 처리 | 현재 | 시점·선행 |
| --- | --- | --- | --- | --- |
| 4차 | 로컬 MCP(게임 내 모든 조작·화면 캡처·녹화(소리 포함)), 16:9 와이드 최대 1920×1080(그래픽 배율 조정 없음) | **WebMCP로 구현**(C++ MCP 서버는 이식하지 않음, `exclude-host` 유지). 와이드는 웹 화면 정책으로 처리하고 옵션 메뉴 해상도 항목은 두지 않음(AGENTS의 웹 부적합 예시) | 미착수. 게임 영역 4:3·16:9·16:10과 768높이 임시 화면 정책 있음(1절 화면/시간) | WebMCP 관측·조작 도구는 미션 입력·HUD가 생긴 뒤(5-C 이후), 녹화는 소리(순서 8) 뒤. 정할 것은 3.5절 5번 ②③. cpppj 935~937 참고 |
| 5차 | 설정·진행 등 자동 생성 파일을 사용자 홈 아래로, 커스텀 맵 두 곳 읽기·Edit 목록 스크롤 | **localStorage 사용** | **대부분 충족:** 설정 `netstorm.settings`(WB03-F)·진행 `netstorm.progress`(WB03-H)를 버전 있는 JSON으로 localStorage에 보관하고 저장 불가/손상 때도 계속한다. 커스텀 맵·Edit 목록은 Edit 제외라 해당 없음 | 새로 저장할 값(11차 난이도 등)이 생기면 같은 방식으로 추가 |
| 6차 | 한국어 지원 | 동일 | 메뉴·Tell·도움말 완료(WB03-N~S), 미션 문구는 영어 대체 | 순서 8 현재 미션 Tell 때 cpppj K1~K5 확정본과 통일(3.5절 2번) |
| 7차 | 오류/예외 처리·로깅 공통 구조, 비정상 종료 직전 상세 로그 | **브라우저 console 출력**(파일 저장 대신) | 명시 오류·`MainLoop` 웹 제외 단계 사유가 모듈마다 흩어져 있음 | 4-D Session 조립 때 진단 경로와 함께 앞당길 수 있음, 늦어도 순서 10. cpppj 1269 참고 |
| 8차 | AA·FSR 1.0·2560×1440/3840×2160 | **제외** | — | — |
| 9차 | 편집기 시나리오 편집·english 없는 Test Battle 수정 | **제외**(Edit 제외) | — | — |
| 10차 | 10.78 알려진 버그 34건 수정 | 동일(웹 범위 해당분) | 미착수 | cpppj 판정 34/34·수정 10건을 3.6절 ②로 반영. 12차가 대상이 되면서 간헐천 예산·Even 간헐천(NB04·NB29)처럼 스커미시가 쓰는 건도 후보다 |
| 11차 | 난이도: Options에 Difficulty(쉬움/보통/어려움, 기본 보통). 쉬움은 적 AI 다리 건설·유닛 생성 주기 ×4, SP가 충분할 때의 채취 유닛 수 절반(최소 1). 어려움은 주기 ×½ | 동일(설정 저장은 localStorage) | **계산·AiCreate 연결 완료**(E554·E555), Options UI·Session hook 공급 미착수 | 적 AI 연결(순서 7) 뒤. cpppj 1292: AI 설정 +0x220/224/228 주기 배율, 난이도는 AI 단위 값 |
| 12차 | 스커미시: Campaign 메뉴에 Skirmish, 자체 설정 UI(AI 수·금지 유닛 최대 5(Outpost 포함)·AI별 난이도/동맹·시작/취소), MP처럼 무작위 섬·간헐천 | **동일(`85551c6c`에서 제외 → 대상으로 바뀜). Outpost도 함께 구현(2026-11-11 사용자 결정)** | Outpost 목록 수명(E556)과 스커미시 단위의 대응표 분류(E500·E512)만 완료 | 11차·순서 8 뒤. MP 시작 규칙 월드 생성은 1절 예외로 이식 대상이 됐다(3.5절 5번 ①). cpppj 1293·1300 `prepareWorld` 참고. Outpost는 아래 "12차 Outpost 범위" |
| 13차 | 기타 언어(일본어는 추후 추가 예정) | 동일. woff2 없는 글꼴은 Google Fonts CDN | 미착수 | cpppj 1288(일본어 UI 509키) 참고. CDN 요청은 외부 접속 문서·감사 검사와 함께(4절 "다른 언어 글꼴") |
| 14차 | QuickJS 엔진 탑재 | **불필요**(플랫폼이 JS를 지원) | — | — |

**12차 Outpost 범위 (2026-11-11 사용자 결정: 스커미시 구현 때 Outpost도 구현):** 스커미시 체크 목록을 펼칠 때 아래 항목을 함께 넣는다. 기준은 cpppj의 Outpost 경로와 스커미시(1293·1300)이며, 기준선 판을 옮기고 기준선 뒤 변경은 3.6절로 판정한다.

1. **분류·이식 완료:** `o/RawOutpostLifecycle`을 E512에서 `skirmish-outpost` 규칙으로 `port`로 옮겼고 E556에서 이식했다(원본 768행 일치, [단위 문서](docs/webpj-raw-outpost-lifecycle-20261115.md)).
2. **남은 확인:** Outpost 두 목록(배치 추가·거리 후보)은 Factory 목록과 다른 전역이므로, 이미 있는 `raw-asset-post-pop-state.js`의 `outposts`/`sharedOutposts`(Factory PostPop이 쓰는 목록)와 같은 저장소인지 cpppj 소유 관계로 먼저 확인한 뒤 연결한다.
3. **지역 소유 연결:** `RawRegionOwnership`의 outpost 지역 통지 분기(`MakeOutpostRegionOwnershipHooks`)는 E249에서 웹 범위 밖으로 남겼다. 이것을 옮겨 `raw-region-ownership.js`에 연결한다.
4. **이미 Outpost 분기를 가진 웹 모듈 재확인:** Factory 그리기·PostPop·연구·업그레이드(Outpost VT `0x509210`), 운반체 반환 후보(`raw-carrier-choice.js`의 outposts 목록)·도착 검사, AI 객체 통지(`outpostType`), 폭탄 상태(`outpost` 126), `GameAuxState.outpostType`(TYPE 126)은 공통 코드라 이미 옮겼다. 실제 Outpost 객체로 이 분기를 지나는 통합 장면을 만들어 cpppj와 대조한다.
5. **client 쪽:** Outpost 분기가 있는 client 단위(RawFactoryMenu·RawPriestMenu·RawCampaignOwnership·RawCampaignSession·GameWorld 등 `port`/`todo` 행, 기준선 뒤 새 파일 ClientConstructionRegions·RawSurfaceAssetDeletion·RawSurfaceFactoryRegular·SkirmishSetup)는 그 단위를 옮길 때 Outpost 분기를 웹 제외 오류로 두지 않고 실제 경로로 옮긴다. 이미 웹 제외 오류로 옮긴 곳이 있으면 이 단계에서 바꾼다.
6. **금지 유닛·건설:** 스커미시 금지 목록의 Outpost 항목(cpppj 27개 후보 중 하나)과, 스커미시에서 Outpost를 짓고 쓰는 경로(건설 목록·배치·AI 사용)를 cpppj 스커미시와 같게 한다. 일반 캠페인·데모에서는 원본처럼 Outpost가 나오지 않는지 함께 확인한다.
7. **검증 경계:** cpppj는 Outpost를 3차(MP) 범위로 구현했고 MP 실행 검증은 사용자 확인으로 남겼다([공통 인계](LEFT_JOBS.md) 0.3절). 웹은 스커미시 안에서의 Outpost 생성·삭제·운반 반환·지역 소유를 부품 검사와 cpppj 오라클 비교로 확인하고, LAN MP 경로는 계속 제외한다.

### 3.8 원본 관측 연결 이월 (E465~E466 감사, 2026-11-15)

E465에서 `RawCameraPan`/`RawEdgeScroll`/`ClientCameraGlobals`의 "cpppj에도 직접 시험이 없다"가 오기라는 것을 발견했다(실제로는 원본 관측 fixture 259/135/140행이 있었고 연결하자 첫 실행 전체일치). 같은 오기를 찾기 위해 기준선 `cpppj/tests` 전체를 감사했고 아직 연결하지 않은 항목을 `docs/webpj-fixture-audit-20261115.md`(표 A: fixture 38개, 표 B: 시험 파일이 있는 단위 48개)에 모았다. 이미 이식한 모듈을 원본 관측에 대조하는 일이라 새 모듈 이식보다 검증 이득이 크고 첫 실행 불일치가 곧 이식 결함이다. 단위마다 시험 연결 → manifest/PORT_MAP 메모 → 모듈 머리 주석의 "직접 시험 없음" 정정 순서로 한다.

- [ ] 채팅 계열 나머지(허브 선행): `chat-callbacks`(30행)·`chat-input-recipients`(721행) — 실제 Client(`RawMenuControls`·`RawChatRuntime`)에 연결된 `ChatWindowClientTests` 가 소비하므로 허브/Client 조립 이식 때 연결한다
- [ ] 점 명령/HTML 나머지: `html-input-dialog` 생성/공장 20행(SaveFortNameDialog 이식 때)
- [ ] 입력 명령/설정 나머지: `audio-reinterpret`(193행)·`font-face-enum`/`font-face-lookup`(389행)은 웹의 ClientAudio(Web Audio 적응)·FontStore(GDI 열거 대응 없음)에 같은 형태의 경계가 없어 적응 판정(3.6절 ①②)이 먼저, `detonate-input`(769행)은 RawObjectConversion 단위에서
- [ ] 렌더러/음악: `renderer-projection`·`scenemusic`

## 4. 이관해도 남겨야 할 미해결 한계

아래는 아직 해소되지 않은 항목이다. 완료한 Tell/링크/도움말 번역/아이콘을 다시 미완료로 만들지 않는다.

| 항목 | 현재 한계와 처리 단계 |
| --- | --- |
| 실제 지형/소유권 | 미리보기의 중립색·가운데 카메라·고정 구름은 개발 정책. 저장 noIsland.owner를 최종 owner로 오해하지 않음. 신전 원소 투표/themeMode·도장/flood 순서·실제 카메라 시작/제한/스크롤은 4-B~5-B에서 연결 |
| 지형 난수/색표 | `buildTerrainFrames`와 절벽의 게임/CRT 난수는 실제 세션에서 공유. 절벽 제외 0x10/0x18의 선택, 최초 팔레트/ColorSort 시점·`useBanners` 기본값, 소유자→색 번호를 4-B·5-A에서 확정 |
| edgeFarm | spots·finder·MayPlace·지면 Retype 뒤 실제 월드에 구현. `RawFarmCluster`의 3×77 별도 시드/30000실패 한도 등 원본 순서를 유지. 타입121에 isle 색표가 있다고 추정하지 않음([근거6절](docs/webpj-terrain-extras-20261102.md)) |
| 보상/SP 세션 연결 | 2026-11-07 WB04-D5b: `SquidReward`·`ScrambledSpStore`·`ContentFormLifetime`은 부품 대조까지다. SP 재시드 원천(004559d0 시각)·사제 추적 offset 소유자(0054d4e0)·실제 지갑(`bindPatchWallet`, brain+2d8)·수입 알림/원격 패킷/위치 소리 장치·Client 커서/boss/현재 너겟 getter·`SquidRewardState.bindGameAux`와 World 절반 HP 모드·같은 World TYPE/로딩 비용표 대여는 4-D·6에서 연결한다. 장부는 `SquidPostPopState`(WB04-E)와 같은 객체여야 한다 |
| 영역 회전 | 기존 실제 파일은 방향0. 회전2/3의 실제 저장 사례가 생기면 원본 결과와 추가 대조. 독립 기하 계산 통과를 실제 파일 관측으로 세지 않음 |
| 입력/커서 | 커서2~14·17·18은 쓰는 미션 입력과 함께 추가. 본문 끌기/캡처 중 Overlay의 원문 입력(`004d62b0` 계열) 계약은 미확정. 2026-11-07 WB02-H: 원본처럼 매 프레임 마지막 포인터 위치를 메인 메뉴에 다시 알리게 했다(정지 포인터 아래 펼침 메뉴 행 강조, cpppj 오라클 일치). 미션 입력(UserInput)도 같은 폴링 계약으로 이식한다 |
| 메인 메뉴 구름 배경 | 2026-11-07 WB02-H 오라클 대조: 원본/cpppj는 Kencloud 출력(`ClientKencloudGlobals::Draw`·`o/RawKencloud*`), 웹 메뉴는 GIF 단순 타일이라 일부 행이 가로 1픽셀 어긋난다(4:3에서 37,054픽셀, 원본 스크린샷도 cpppj 쪽). Kencloud 부품(E118·E119)을 메뉴에 연결하는 5-A에서 해소하고 `m1-scenarios.json`의 구름 차이 기록을 0으로 바꾼다 |
| Options/저장된 값 | Help0/Options−40은 측정값이며 일반 위치 규칙/와이드 메뉴 배치는 미확정. Wind Noise·Speaker Swap·Edge Scroll·Auto-Demo는 해당 시스템과 실제 연결 확인 필요. 시작 팁·설정→GameData 동기화/Done* 저장은 이미 구현 |
| 도움말 고급 표시 | `<info>` 머리글/능력치, `#`/`&` 상자 맞춤·색 치환은 미확인. 레이어0/그림자 없는 도움말 그림 경계를 유지한다. 스크롤 화살표는 웹의 검정이 원본과 맞다(cpppj 도 2026-11-10 원문대로 현재 팔레트 검정 005a4058 로 고침, [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md) 3.1절) |
| 에너지 기호 | `mana.8~11`은 현재 10.78 클러스터 8개 밖이라 0크기. 실제 원본 F1 모습 관측은 미확인. 원본 실행이 필요하면 AGENTS.md의 확인/예외 규칙 적용 |
| Tell/언어 | `<background>`는 이름만 보관(`nsisles` 자료/사용 근거 미확정). 현재 미션·`{@미션.키}`·브리핑/안내 `.korean`·스크립트 도움말 Back은 4-D·8에서 연결. 한국어 영어대체/긴 줄/고DPI도 해당 화면과 확인 |
| 메뉴 확장 | 두 열 메뉴는 범위 내 실제 소비자가 필요할 때 원본 계약으로 이식. `%{userSpec}`·User Made Campaigns는 제외이며 새 과제로 돌리지 않음 |
| 입력/오디오/환경 | 실제 스피커 청취·모든 브라우저/GUI Linux·고DPI/모니터 이동은 미확인. 일부 Edge/Chromium 기록을 전체 플랫폼 지원으로 쓰지 않음 |
| 다른 언어 글꼴 | AGENTS 글꼴 규칙: `fonts/`의 Noto Sans·Noto Sans JP는 TTF뿐이라 webpj는 Google Fonts CDN을 쓴다. 웹의 현재 범위(영어·한국어)는 D2Coding woff2만 쓰므로 아직 소비자가 없다. 13차(3.7절)에서 다른 언어를 추가하는 단위가 CDN 글꼴 요청을 [외부 접속 문서](docs/webpj-external-access-20261105.md)의 허용 목록과 `tests/external-access.test.mjs` 소스 감사에 함께 반영한다(현재 감사는 `./data/` 같은 출처 요청만 허용). cpppj 일본어 UI는 외부 `NotoSansJP-*`를 쓰며 가변 글꼴 기본 인스턴스가 Thin이라 굵기 확인이 남아 있다([공통 인계](LEFT_JOBS.md) 0.3절) |
| README webpj 상태 | 2026-11-16 정리에서 README.md·README.en.md의 webpj 상태를 WB04-E584 기준(검사 5,561건, 대응표 1,006단위 중 port 842, 재기준선 완료, 세션 본체 결선 진행 중)으로 고쳤다. README는 공통 문서이므로 WB 단위에서 고치지 않고 다음 일괄 갱신 때 반영한다 |
| Credits AI 문구 | 2026-11-05 사용자 결정: 현행 문구(`OpenAI Codex, ChatGPT and Anthropic Claude Code`, 한국어 동일 취지)를 **유지하고 나중에 수정**한다. cpppj/dotnetpj 문구(Codex·ChatGPT만)와 다르며, 수정 지시가 오면 `www/assets/product.js`의 영어/한국어 섹션과 `tests/product.test.mjs`의 문구 검사를 함께 고친다. 한국어 행 이름 `NetStorm 11.01 패치 크레딧`은 확정. 카피라이트 줄 추가는 이 결정과 별개이며 WB03-AE에서 반영했다 |

## 5. 검증과 완료 판정

| 단계 | 확인 방법과 책임 |
| --- | --- |
| L1 모듈 | 원본10.78 fixture(`cpppj/tests/fixtures`)와 이식한 cpppj 검사로 반환/전체 슬롯·전역·정수/float/double 비트·효과/난수 순서를 대조. Adler 일치는 체크섬 비교이며 바이트 직접 비교와 구분 |
| L2 시작 상태 | 같은 Fort/언어/옵션/시드와 같은 Load/Start 시점에서 cpppj 오라클과 비교. Session 덤프를 쓸 수 있으면 SID·타입·owner·좌표/HP/프레임·Player/동맹/SP·Graph·장부·난수, 그 전에는 `report`와 논리 화면 |
| L3 입력 진행 | cpppj와 같은 단계표(프레임·입력)로 진행한 상태·보고서·화면을 비교. 표시 주사율·비동기 자산 로딩이 게임 난수/시계를 바꾸지 않도록 검사 |
| L4 화면/소리 | L4a: 같은 상태의 논리 8비트 화면이 cpppj `snapshot`과 차이 0. L4b: cpppj MCP 녹화와 소리 시점·곡 선택 비교. 전체 외관/청취 동일성은 사용자 확인. 원본 캡처의 창 테두리/레터박스/HDR·방송 음성을 분리 |
| L5 자연 플레이 | 사용자가 자연 승패→신규 완료값0→1 저장→새로고침/재실행→다음 미션 해제를 확인. 강제 승리/합성 완료값/옛 CP 완주로 대체하지 않음 |

- JS의 BYTE/WORD/DWORD 폭·signed/감김·리틀엔디언·float32 저장·double 시간·x87 중간 정밀도를 명시한다. `Math.imul`/`Math.fround`/`DataView`/BigInt는 필요한 위치에만 사용한다. 비트 차이를 허용 오차로 숨기지 않는다.
- 게임 난수·CRT 난수·SP 쓰기/예약/삭제·callback 뒤 현재 상태 재조회를 보존한다. 렌더링용 상태를 규칙의 별도 월드로 만들지 않는다. 사본 조회/쓰기 대여/양수reset 뒤 수명도 각 모듈에서 명시한다.
- 원본 fixture/보호 자료를 웹 출력으로 덮어쓰지 않는다. 최초 실패/교정·미연결 장치·원본과 다른 보호/웹 적응·누락 fixture의 skip을 기록한다. 검토 커밋과 실제 반영 커밋은 분리한다.
- 기존 Node 검사와 변경에 맞는 집중 검사를 사용한다. 브라우저/자산/통합 검사는 관련 입력·화면·수명이 바뀔 때 수행한다. 소수 단위마다 긴 전체 미션/자연 완주/배포 반복을 자동 실행하지 않는다.
- 확인한 OS/브라우저 버전·논리/물리Canvas·화면 배율/픽셀 비율·소리/저장 환경을 기록한다. UI 스모크와 실제 게임/동일성 증거를 합산하지 않는다.

## 6. 작업 명령과 참고

```powershell
# 저장소 루트에서 정적 개발 서버를 실행한다(공개 루트 webpj/www, 포트9803).
npm --prefix webpj start
# Node 내장 전체 검사. 부품 작업 중에는 관련 tests/*.test.mjs만 집중 실행할 수 있다(webpj 폴더에서 node --test "tests/*.test.mjs" 도 같다).
# Node v22·v26을 실검사한다(2026-11-16 사용자 지시). v24 검사는 생략하고 두 버전 통과 시 호환을 가정한다. 기본 node는 v26, Node22 경로: C:\Program Files\nodejs22\node.exe:
#   & "C:\Program Files\nodejs22\node.exe" --test "tests/*.test.mjs"   (webpj 폴더에서; Node 22 는 인자 없는 --test 의 탐색 범위가 달라 glob 을 준다)
npm --prefix webpj test
# 이식 추적 목록 검사(해시 비교·대응표 base 뒤 cpppj 근거 소스 변경 보고). 전체 --write 는 수 분~십수 분이 걸리므로 단위마다 --only 를 쓴다.
node webpj/tools/fixture-manifest.mjs
node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>
# 이식 대응표 검사·분류/상태 개수·기준선(행 base) 뒤 cpppj 변경 보고. --write 는 행/분류를 다시 만들고 사람 칸을 보존한다. --why 이름 은 분류 사유.
node webpj/tools/port-map.mjs
node webpj/tools/port-map.mjs --why CompositeMissionTransport
# 웹 무브라우저 실행(논리 화면 BMP·보고서), 저장된 cpppj 오라클과 비교, 오라클 재수집(창이 뜸: AGENTS.md 실행 확인 규칙)
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
node webpj/tools/screen-compare.mjs --diff <폴더>
node webpj/tools/cpp-oracle.mjs --exe extracted/webpj-oracle/225553df/Netstorm.exe [--id 시나리오]
# 준비 자료를 검사한다. 자료 변경이 필요한 단위에서만 prepare-assets로 재생성한다.
npm --prefix webpj run check-assets
npm --prefix webpj run prepare-assets
# 저장소에 있는 Playwright Chromium 초기 페이지 스모크이다.
npm --prefix webpj run test:e2e
```

개발 URL은 `http://localhost:9803/`이며 `?debug&mute&aspect=16:9&scaling=nearest&lang=ko`를 조합할 수 있다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 **게임 규칙 없는 개발용 지형 화면**이다. 시작 클릭 뒤 방향키/드래그로 점검하고 Esc로 메뉴 복귀한다. `previewMap(name)`도 같은 개발 경로다.

- [공통 인계](LEFT_JOBS.md)·[dotnetpj 인계](LEFT_JOBS.dotnetpj.md)·[cpppj 소스 맵](cpppj/SOURCE_MAP.md)
- [RawSurfaceWorld](cpppj/src/client/RawSurfaceWorld.h)·[RawCampaignSession](cpppj/src/client/RawCampaignSession.h)·[RawFortObjectLoad](cpppj/src/o/RawFortObjectLoad.h): 실제 소유자/세션 조립 경계
- [실행 자산](docs/runtime-resources.md)·[형식 목록](docs/formats/README.md)·[가상 파일 조회](docs/formats/vfs.md)·[공유 명칭](assets/unit-names.korean.json)
- [25종 복원 기록](docs/exe/cpp-unit-restoration-20261022.md)·[현재 checking 공백](docs/exe/cpp-checking-consumer-gaps-20261103.md)·[월드 장기 소유](docs/exe/cpp-state-world-owner-plan-20261025.md): 과거 범위는 최신 공통 인계와 함께 확인
- [스크린샷](docs/screens/README.md)·[녹화](docs/videos/README.md)·[애니메이션 시간](docs/videos/animation-timing.md)
- [cpppj 빌드·실행 옵션](docs/cpp-build.md)·[cpppj 로컬 MCP](docs/exe/cpp-mcp-server-20261106.md)·[dotnetpj 포트 맵 형식](dotnetpj/PORT_MAP.md): 오라클 실행과 대응표 형식
- [이식 대응표 문서](docs/webpj-port-map-20261106.md): 분류 규칙·단독 경로 판정·3.6절 판정 근거(WB02-G)
- [오라클 비교 문서](docs/webpj-cpp-oracle-20261107.md): 무브라우저 실행·오라클 수집/비교 규칙·M1 대조 결과(WB02-H)
- [분배기·Content 수명 문서](docs/webpj-content-lifetime-20261107.md): Form 분배·Content 삭제 수명·보상/SP 저장소·NaN 비트 규칙·3.6절 5행 판정(WB04-D5b)
- [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md): 웹 이식 중 발견해 cpppj 에서 처리한 시험 결함·원본 차이(해소된 항목 포함)

## 7. 문서 정리 기록

- **기록 방식:** 단위마다 0.5절 5번의 칸만 고치고 단위별 기록 줄은 이 절에 쌓지 않는다(단위 문서와 커밋 메시지가 기록이다). 이 절에는 문서 정리·사용자 결정·작업 중단 같은 단위 밖 사건만 적는다.
- **2026-11-17, HJOW-Athlon, 사용자 요청(인계 문서 정리·만료/상세 내용 이관·cpppj 인계와 소스를 근거로 4-D 진행 위치와 남은 계획을 세부 기재, 문서만):** `3a3ac6bc5 / 20261117-25 WB` 시점 원문 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632)에 바이트 그대로 보존했다(150,201바이트, SHA-256 `f30e119ad47e7292391bde7d2910100a3c4719a244041e52d6527055c571f51b`). **뺀 것:** 0.1절의 재개 인계 문단·E616 완료 체크 3항목·옛 다음 작업, 0.6절의 완료 요구 점검 표, 3.1절 4-D의 A~E 구획(끝난 C 15항목, `RawCampaignSession 1`의 E574~E626 구간별 진행 기록, 옛 큰 항목 15개), 5-A `ClientRendererFrame`의 선행 완료 모듈 나열, 5-C·6·7단계 완료 하위 목록의 관측 수·단위 문서 링크(단위 문서는 E번호로 검색), 3.7절 Outpost 범위의 끝난 1·2번 상세, 7절 2026-11-16 기록의 상세. **새로 넣은 것:** 0.1절 4-D 진행 행과 다음 작업, 3.1절 4-D 진행판(이정표·`RawCampaignSession.cpp` 구간 지도·계획 단계별 배분)과 계획 4D-1~4D-10(41항목: cpppj 근거 줄·함께 옮길 단위·검증 시험), 2.4절 E585~E632 요약 행, 3.4절 재개 뒤 cpppj 변경 행, 3.5절 7번. **바로잡은 것:** 4-D 머리글(42/57로 남아 있던 것을 실제 44/59로 센 뒤 계획 분할로 44/85), 3.1절 4-D 재기준선 수(64/4,747 → 62/4,607), 바인딩 6단위 배정(6·7단계 → 4D-2, cpppj `Load()`·`ConnectMotion`이 설치), 0.2·0.3절 줄 수(현재 cpppj 소스 재집계 159 / 34,223)와 partial·todo 비율(3.9·11.9% → 3.8·11.8%), 3.4절 cpppj 현황(일시 중지 → 2026-11-17 재개, 제품1320). **발견:** 실제 10.78 Client는 시작 때 Storage에 Native 장치를 결합하므로(`ClientMain.cpp` 442..533) 웹의 typed 조립만으로는 오라클과 L2를 맞출 수 없다. 이것을 4D-8로 계획했다. 코드·검사·게임 실행은 하지 않았고 `LEFT_JOBS.md`·`LEFT_JOBS.dotnetpj.md`·cpppj·AGENTS.md·README는 수정하지 않았다.
- **2026-11-17 사용자 지시:** cpppj 동시 작업 여부는 사용자가 목표를 제시할 때 명시한다. 별다른 언급이 없으면 동시 작업이 없는 것으로 보고 cpppj 소스 수정도 할 수 있다. 0.5절 1번·1절 보호/분담·3.4절·3.5절 7번에 반영했다.
- **2026-11-16:** 사용자 요청 정리(끝난 체크 항목·만료 내용 이관, 4-D·5-A 가독성 재구성, 체크 누락 확인, README 최신화)와 cpppj 소스 수정 허용 지시(0.5절·1절 반영). 기록 원문은 [2026-11-17 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632) 7절에 있다.
- **2026-11-15, HJOW-Athlon, 사용자 요청(인계 문서 정리·cpppj 소스 기준 계획 재수립):** `fa8c2931f / 20261115-51 WB` 시점 원문을 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)에 보존하고, 끝난 체크 항목을 빼고 단계 진행판·남은 단위 배정·5-C~8 체크 목록·4-R 재기준선 제안·집계 도구 `tools/port_inventory.py`를 넣었다.
- **2026-11-11 이전:** 2026-11-11 인계 정리와 AGENTS.md `60226ea9`·`85551c6c` 반영, 스커미시의 Outpost 결정(1절·3.7절), 2026-11-10 Node v26 신호 NaN 대응·글꼴 라이선스 파일 이름 변경·단계 분할, 2026-11-09 첫 재구성. 각 기록의 원문은 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584) 7절과 그 이전 보관본에 있다.

**AGENTS.md 반영 기준(2026-11-17 확인, 마지막 AGENTS 변경 `c17aeede1`):** AGENTS.md는 수정하지 않는다. webpj에 해당하는 사용자 변경(`551d27ae` 진행 요약 기준·`75e4eed9` 글꼴·`8136df49`~`22a0877c` 후순위 범위·`60226ea9` 11~13차와 카피라이트·`85551c6c` 14차와 웹 후순위 4~14차·`c17aeede1` dotnetpj·webpj는 디컴파일 소스 분석 불필요)을 이 문서에 반영했다(0.4절·0.6절·3.7절).

**오래된 내용은 보관본에 날짜순으로 있다:** [2026-11-17 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261117-through-wb04-e632)·[2026-11-16 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)·[2026-11-15 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)·[4-C 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261112-4c-through-wb04-e328)·[4-B 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-4b-through-wb04-e304)·[2026-11-11 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-through-wb04-e277)·[순서3 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e264)·[2026-11-10 단계 분할 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)·[2026-11-09 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)·[2026-11-06 재계획 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261106-through-wb04-d5a)·[WB04-A까지의 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a). **그 문서의 “현재/다음/미구현”은 당시 기록이며, 이후 작업은 이 현행 문서를 기준으로 진행한다.**
