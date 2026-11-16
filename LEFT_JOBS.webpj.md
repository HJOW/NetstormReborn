# LEFT_JOBS.webpj — 현행 상태와 남은 작업

webpj(HTML + JavaScript 웹 클론)의 **현재 진행 상황·남은 작업(단계별)·유효한 결정**만 둔다. cpppj/공통은 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)가 기준이다. 지난 이력·끝난 체크 항목·만료된 계획은 [보관본](LEFT_JOBS.webpj.old.md)에 원문 그대로 있다(이번 정리 직전 전체는 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)). 단위별 상세(근거 커밋·fixture·최초 실패와 교정·남은 경계)는 각 `docs/webpj-*.md` 단위 문서와 `webpj/PORT_MAP.tsv` 메모가 기준이다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판·남은 단위) → 3.1(단계별 체크 목록) → 0.5(단위 착수/종료 절차) → 3.5(사용자 결정 대기) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 칸으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-17, 마지막 단위 WB04-E629)

| 항목 | 현행 기준 |
| --- | --- |
| 실행·플레이 | 클릭하여 시작→원본 자산 메인 메뉴·캠페인/데모 선택 목록·도움말/옵션·Credits 세 항목(11.01 Patch Credits 포함)과 개발용 지형 미리보기(`?map=`)까지 동작한다. **실제 미션 플레이는 아직 불가**다. 옮긴 월드 부품은 아직 앱 수명에 연결하지 않았고(`PORT_MAP` session 0), `MissionBegin`/`StartDemo`는 미션 월드에 연결되지 않았다 |
| 원본과의 동일성 | 메뉴는 **cpppj 오라클(L4a) M1 8장면**(Welcome·메인 메뉴 8버튼·Campaign·Demo·Credits·Help/Options 펼침 메뉴·General Help, 영어 4:3, 원본 비교 모드)이 기록한 원인 밖 픽셀 차이 0이다(구름 배경 등 원인 기록은 4절). Fort/지형·그림·SID와 월드 부품은 원본 10.78 x86 fixture 또는 독립 계산과 대조했다. 실제 미션의 L2/L3·전체 화면/음향·자연 완주는 미검증이다 |
| 마일스톤 | **M1 완료** · **M2 진행 중**(순서3·4-A~4-C·4-R 완료, 현재 단계 **4-D**의 D 구획 `RawCampaignSession` 조립, 0.2절) · M3~M5 미착수 |
| 이식 진행도 | 대상 1,010단위(제외 150 별도) 중 **port 851(84.3%)·partial 39(3.9%)·todo 120(11.9%)**. 남은 159단위 / 34,206줄은 기존 기준선 84단위 / 28,203줄과 재기준선(4-R)에서 추가된 75단위 / 6,003줄이다(단계 배정은 0.2·0.3절). 앱 수명에 연결된 단위(`session`)는 아직 0이다 |
| 마지막 작업 단위 | **WB04-E629** — 캠페인 기술/객체 공용 내용물 writer를 원본706행과 대조하고 같은 실제 Form6/Robust·finder·Template stream에 연결했다. 신규3건 통과([단위 문서](docs/webpj-fort-content-save-20261117.md)). 다음은 객체/영역·Money/Mission/CoreData 자식과 외곽 공개·Load다 |
| 마지막 전체 검사 | 2026-11-17 WB04-E629: Node v22.23.3·v26.13.0 최종 전체 각각 **5,982건 통과, 실패·취소·건너뜀 0**. v24는 사용자 지시로 생략하고 호환을 가정한다 |
| 마지막 e2e | 2026-11-17 E616 Chromium **2/2 통과**(초기 페이지·독립 저장소 최초 실행/기존 설정·진행 재실행). Welcome/Welcome2·Options5·5·사용자3·2·Struggle 잠금/체크를 실제 클릭, 처리되지 않은 예외0. 실제 미션/월드 검사는 아니다 |

**재개 인계 (2026-11-17):** 사용자 목표 재개에 따라 진행 중이다. E619에서 실제 World Runtime 게시1825와 배치/지형·소유자 후처리 조립을 마쳤다. E621에서 같은 Template의 TypeNames/Objects/Contents reader도 연결했다. E622에서 최초 Template 선택·MakeRuntime과 Fort 준비/공유 population 부모도 연결했다. E623에서 지역/승패와 ConnectAi2777..2834의 참여·본체/초기/지갑도 연결했다. E624에서 실제 객체 통지/World 등록·준비/전략/경제도 연결했다. E625에서 생성자 안 ConnectAi의 나머지 배치/생산·다리/건설 선택도 연결했다. E626에서 실제 World Runtime pin 전달과 정상/예외 범위 반납도 연결했다. E627에서 외곽 callback의 공통 snapshot 부모를 이식 대상으로 교정하고 원본2,728행과 대조했다. E628에서 그 TypeNames/Deck writer도 원본2,060행과 대조하고 같은 실제 Template/생산 덱에 연결했다. E629에서 기술/객체 내용물 writer도 원본706행과 대조하고 같은 실제 Form6/Robust·finder·Template stream에 연결했다. 다음은 외곽 생성자4318..4330의 callback/공개 수명과 Load다. cpppj1320의 내부 미션 전환 flush/종료 수신도 E620에서 재반영했고 기존 Source의 실제 누적 패킷 전달과 종료 뒤 번호 재사용을 검사했다. 외곽 Session/Load·실제 미션 플레이는 미완료이며 공중3종 새 전용 생성/생산/행동 검사와 Native 장기 수명의 기존 경계를 유지한다.

**최초 실행 상태 웹 반영 완료(WB04-E616):** [원본 관찰](docs/exe/original-first-run-20261116.md)·[cpppj1314 확정](docs/exe/cpp-first-run-1314-20261116.md)·[웹 검증](docs/webpj-first-run-20261116.md)이 기준이다. 기존 사용자의 localStorage 설정·진행을 초기화하지 않는다. 웹 해상도/전체화면·Edit·MP/온라인 검증 제외를 유지하며, 실제 미션 완료 효과 연결은8단계 잔여다.

- [x] 두 localStorage 키가 없는 독립 Chromium 저장소에서 클릭 시작→Welcome(OK)→Welcome2(Play Early Missions / Main Menu)→메뉴를 실제 클릭했다. Welcome2를 열 때 welcomeGiven=1 저장과 재실행 Tips 분기를 확인했다 → [E616](docs/webpj-first-run-20261116.md)
- [x] Struggle For Freedom은1번만 활성·2~6번 잠김·전부 체크 없음이다. 잠긴2번 클릭 무반응, 검사 입력 DoneTheWarBegins 저장→재실행 뒤 자기 체크/다음 행 해금을 확인했다. 실제 미션 클리어 검증과 구별한다 → [E616](docs/webpj-first-run-20261116.md)
- [x] 웹 Options 소리·음악·바람·가장자리 스크롤·Auto-Demo·시작 팁 ON, 스피커 교환 OFF·음량5·5다. Settings와 Web Audio fallback을 교정하고 기존3·2를 UI로 저장·재실행해 유지했다 → [E616](docs/webpj-first-run-20261116.md)

**다음 작업 (위에서부터):**

1. **4-D D `RawCampaignSession 1`을 이어 간다.** 생성자 `.cpp` 1536행까지의 모듈 조립,1537의 typed 신전 상승([E589](docs/webpj-session-vortex-20261116.md)),1538..1610의 실제 자산/PostPop 결선([E590](docs/webpj-session-assets-20261116.md))을 마쳤다. 선행 Host/Factory·공유 상태/공간·표시 부모와 실제 Storage/World 송신은 [E594](docs/webpj-surface-transmission-20261116.md)까지 연결했다. **1611..1649행 `RawWorldRuntimeHooks` 값 구성**도 마쳤다([E595](docs/webpj-session-runtime-20261116.md)). **1650행 `InstallContents` 내부3020..3049의 통지 접두**도 마쳤다([E596](docs/webpj-session-content-notices-20261116.md)). **3050..3134행 ContentForm 수명/운반·Unpop/생성**도 마쳤다([E597](docs/webpj-session-content-lifetime-20261116.md)). **3135..3156행 기술/Deck·생산/동기화**도 연결했다([E598](docs/webpj-session-production-20261116.md)). **3157..3192행 Factory 연구/업그레이드/입력**까지 연결하여 typed InstallContents1650 조립을 마쳤다([E599](docs/webpj-session-factories-20261116.md)). **1651행 ConnectFlare**도 연결했다([E600](docs/webpj-session-flare-20261116.md)). Construction이 참조하는 **재생 실행/현재 라벨 부모**도 먼저 옮겼다([E601](docs/webpj-build-replay-runtime-20261116.md), FILE/Host/키는6단계 잔여). 환불/배치 취소 writer·수신 부모도 옮겼다([E602](docs/webpj-construction-failure-packets-20261116.md)). **1652 Construction**도 같은 World/SP/방송·장기 최근 목록/방향·현재 재생과 실제 기본 패킷에 연결했다([E603](docs/webpj-session-construction-20261116.md)). **1653 Damageable**도 같은 Dude/해방·전후처리·신전/간헐천 수명에 연결했다([E604](docs/webpj-session-damageable-20261116.md)). **1654 IceTower**도 같은 현재 권한/Host/후처리에 연결했다([E605](docs/webpj-session-ice-20261116.md)). **1655 CarrierSteal**도 같은 Form6/현재 방송·이동/정지·예약에 연결했다([E606](docs/webpj-session-carrier-steal-20261116.md)). **1656..1658 BombPostPop/AcidZap/AcidFence**도 같은 실제 접두·수신·공간/피해에 연결했다([E607](docs/webpj-session-spells-20261116.md)). **1659..1664 CarrierRegular**도 같은 너겟/Host·실제 미리보기·현재 표면/낙하와 공유 task에 연결했다([E608](docs/webpj-session-carrier-regular-20261116.md)). **1665..1667 Sun/Wind/RainAviary**도 같은 현재 Host/역할/시계·설정과 후설치 Flyer에 결선했다([E609](docs/webpj-session-aviaries-20261116.md), 새 전용 공중3종 실행 없음). **1668 ArcSpire**도 같은 현재 공간/자산·피해/빔·프레임/공통 Regular에 결선했다([E610](docs/webpj-session-arc-spire-20261116.md)). **1669 Flyer**도 같은 Process/Frame/Target·후처리와 현재 SID 진단/방송에 결선했다([E611](docs/webpj-session-flyers-20261116.md), 공중3종 새 전용 실행 없음). **1670 Fence**도 같은 현재 공간/TYPE·표식/연결선·공유 예약과 수명에 결선했다([E612](docs/webpj-session-fence-20261116.md)). **1671..1679 GemReward/Buried**도 같은 현재 제외 목록/게임 난수·Deck 장치와 Host/공통 장부에 결선했다([E613](docs/webpj-session-gems-20261116.md)). **1680..1756 독립 typed 자산 삭제 래퍼/가상 분배**도 같은 현재 부모와 기존 fallback에 결선했다([E614](docs/webpj-session-deletion-20261116.md), Native 장기 분기는 잔여). **1757..1814 전체 Regular**도 같은 현재 몸체/함수 값과 실패 콘솔에 결선했다([E615](docs/webpj-session-regular-20261116.md), Native 장기 분기는 잔여). **1815..1824 송신 합성/다리 효과·로딩 깊이**까지 연결했다([E618](docs/webpj-session-transmission-20261117.md), DaisProcess 선행 [E617](docs/webpj-dais-process-data-20261117.md) 완료). **1825..1865 World Runtime 게시·배치/지형/소유자 후처리 조립**도 마쳤다([E619](docs/webpj-session-terrain-20261117.md)). 같은 Template의 TypeNames/Objects/Contents 원시 reader도 연결했다([E621](docs/webpj-session-streams-20261117.md)). 최초 Template/MakeRuntime·Fort 준비/공유 population 부모도 연결했다([E622](docs/webpj-session-install-20261117.md)). 지역/승패·AI 참여/본체·초기/지갑과 생성 전 동맹 판정도 연결했다([E623](docs/webpj-session-ai-brains-20261117.md)). 객체 통지/World 등록·준비/전략/경제도 연결했다([E624](docs/webpj-session-ai-world-20261117.md)). 나머지 배치/생산·다리/건설 선택도 연결했다([E625](docs/webpj-session-ai-production-20261117.md)). E626에서 실제 World Runtime pin 전달과 정상/예외 범위 반납도 연결했다. E627에서 외곽 callback의 공통 snapshot 부모를 이식 대상으로 교정하고 원본2,728행과 대조했다. E628에서 그 TypeNames/Deck writer도 원본2,060행과 대조하고 같은 실제 Template/생산 덱에 연결했다. E629에서 기술/객체 내용물 writer도 원본706행과 대조하고 같은 실제 Form6/Robust·finder·Template stream에 연결했다. 다음은 외곽 생성자4318..4330의 callback/공개 수명과 Load다. Motion 뒤 AI Movement/Runtime과 실제 행동은 그 설치 구간에서 이어 간다. Native mutable 송신/기본 Form writer와 나머지 장기 실행기는 실제 Client 결선에서 함께 옮긴다. 이어 외곽 공개·같은 Template의 TypeNames → Objects/Contents·`Load`로 간다(3.1절 4-D D).
2. 이어서 4-D D의 2~4번(로컬 연결·로그인, 프레임·카메라, 승패·종료)과 `GameWorld`, E 구획(`ClientMain`·`ClientCampaignTransport` 2단계·4-C 통합 시험·**L2 판정**)으로 간다.
3. Session 허브가 서면 막혀 있던 A 구획(State/World 정리 4모듈·선택 정리)과 B 구획(Client 서비스 3모듈)을 연결한다. 재기준선 추가분의 4-D 몫(`RawSurface*` 29·전송/Native 수신기 15·메타 7·규칙/건설 11 등, 0.3절)은 각 결선 지점에서 함께 옮긴다.
4. 3.8절 허브/적응 판정 4건은 5-C·8단계 항목에서 처리한다.

### 0.2 단계 진행판

순서별 완료 기준은 3.2절, 단계별 체크 목록은 3.1절이다. **진행** 칸의 `n/m`은 3.1절 체크 항목 수다(보관본으로 옮긴 끝난 항목 포함). **남은 단위** 칸은 `PORT_MAP.tsv`에서 todo·partial인 cpppj 단위를 단계에 배정한 수와 현재 cpppj 소스 줄 수 합계다(partial은 파일 전체 줄이라 상한, `python -X utf8 tools/port_inventory.py --families wb`). **기존**은 옛 기준선부터 있던 단위, **재기준선**은 4-R에서 추가된 단위이며 그 배정은 판정 문서([E498](docs/webpj-rebaseline-surface-20261115.md)~[E502](docs/webpj-rebaseline-meta-20261115.md))의 후속 단계를 따른다(0.3절 목록).

| 순서·마일스톤 | 단계 | 내용 | 진행 | 남은 단위 / 줄 (기존 + 재기준선) |
| --- | --- | --- | --- | --- |
| 1~3 M1·M2 선행 | WB02-G·H, 3-A~3-E | 대응표·오라클 기반, 월드 부품 E1~E264 | 완료(2026-11-10) | — |
| 4 M2 | 4-A·4-B·4-C | 자료·설정·타입 표 / Fort·지형·섬 적재 / 패킷·로그인·내부 전송 | 완료(E265~E328, 4-C 3항목은 4-D 이월) | — |
| 〃 | 4-R 재기준선 | cpppj 제품 985 → 1313 전환과 오라클 재수집 | 완료(E497~E514) | — |
| 〃 | **4-D** | Session 조립·Tutorial1 Load/Start → **L2** | **진행 중 41/56** | 17 / 13,385 + 62 / 4,607 |
| 5 M2 | 5-A | 렌더러·Draw·시각 시스템 연결 | 진행 중 44/46 | 3 / 533 |
| 〃 | 5-B | 카메라·HUD·미니맵·커서 | 진행 중 9/13 | 5 / 1,039 + 4 / 407 |
| 〃 | 5-C | 게임 화면 허브(Uber·메뉴 컨트롤)·미션 진입 명령 → **M2 판정**(L4a) | 0/6(메인 메뉴·타이틀 State 3단위 선행 완료) | 10 / 5,766 |
| 6 M3 | 6 | 입력·게임 메뉴·사제/운반·경제·건설 연결 → L3 | 1/8(게임 메뉴·우클릭 메뉴·커서 선행 부품 다수 완료) | 13 / 1,123 + 2 / 199 |
| 7 M4 | 7 | 전투·의식·AI 연결 | 0/6(Hit·GunQueries·Archer·Motion·BombBlast 선행 완료) | 6 / 441 |
| 8 M4 | 8 | 미션 수명·안내·대본·소리·채팅·도움말 잔여 | 0/9(SoundProcess 선행 완료) | 30 / 5,916 |
| 9 M5 | 9 | 표시 품질(60/120)·브라우저/OS 확인·정적 배포 | 미착수 | 0 + 1 / 41 |
| 10 후순위 | 10 | AGENTS 4~14차 중 웹 해당분(4차 WebMCP·와이드, 5차 localStorage, 6차 미션 문구, 7차 console 로깅, 10차 버그, 11차 난이도, 12차 스커미시+Outpost, 13차 기타 언어 — 3.7절) | 일부 선행(11차 난이도 계산 E554·E555, Outpost 목록 수명 E556), 5차 대부분 충족 | 0 + 6 / 749 |
| **합계** | | | | **84 / 28,203 + 75 / 6,003 = 159 / 34,206** |

**남은 일의 성격:** ① **조립**(4-D·5-C) — `RawCampaignSession`(7,118줄)·`ClientMain`(2,133줄)·`GameWorld`(1,157줄)·`UberGump`(2,683줄)·`RawMenuControls`(1,605줄)·`RawButtonControls`(811줄) 같은 허브가 이미 옮긴 부품을 한 월드로 묶는다. 여기서 처음으로 L2(session-dump)·L4a(미션 화면) 비교가 가능해지고, 부품 단계에서 보이지 않던 순서·수명 결함이 드러날 것이다. 재기준선에서 추가된 장기 월드 저장소(`RawSurface*`)·Native 수신기 묶음도 이 결선에 붙는다. ② **client 바인딩**(6·7) — `RawCampaign*` 바인딩·게임 메뉴·커서처럼 `o/` 부품을 세션에 잇는 얇은 단위가 많다. ③ **웹 적응**(8) — 소리·음악·동영상은 Web Audio 등으로 바꿔야 하는 adapt 단위다.

### 0.3 남은 이식량

**대응표 기준(현재 cpppj 소스 줄 수 = 기준선 `225553df` 판, 2026-11-16 재확인):**

| 범위 | 대상 | port | partial | todo |
| --- | --- | --- | --- | --- |
| 전체 | 1,010단위 / 146,612줄 | 851 (84.3%) / 112,406줄 (76.7%) | 39 (3.9%) / 19,125줄 | 120 (11.9%) / 15,081줄 |
| `o/` 월드·규칙 | 564단위 / 73,584줄 | 549 (97.3%) / 71,470줄 (97.1%) | 3 (0.5%) / 1,164줄 | 12 (2.1%) / 950줄 |
| `client/` 세션·UI·표시 | 445단위 / 72,602줄 | 301 (67.6%) / 40,510줄 (55.8%) | 36 (8.1%) / 17,961줄 | 108 (24.3%) / 14,131줄 |
| `zacket/` 내부 미션 전송 | 1단위 / 426줄 | 1 (100%) / 426줄 (100%) | 0 | 0 |

표의 port 줄 수는 현재 기준선 파일 전체의 규모이며 기존 행에 남은 ③ 후속 기능의 구현을 뜻하지 않는다. 분류(class)별로는 port 959(이식 대상)·adapt 51(웹 대체가 필요한 플랫폼 경계)·제외 150(exclude-mp 62·exclude-edit 55·exclude-host 32·exclude-legacy 1)이다.

**영역별 남은 양**(todo + partial, `tools/port_inventory.py`, 영역 기호는 [LEFT_JOBS.dotnetpj.md 2.7](LEFT_JOBS.dotnetpj.md#dotnet-inventory)과 같다):

| 영역 | 남은 단위 / 줄 | partial | todo |
| --- | --- | --- | --- |
| A 자료·파일·설정 | 5 / 716 | 3 / 597 | 2 / 119 |
| B 월드 코어(SID·슬롯·Pop·해시) | 6 / 1,073 | 1 / 709 | 5 / 364 |
| C Fort·지형·섬 적재 | 4 / 327 | 0 / 0 | 4 / 327 |
| D 세션·월드 조립·로그인·패킷·Client 전역 | 36 / 14,166 | 6 / 10,100 | 30 / 4,066 |
| E 표시·렌더러·카메라·HUD·시각 효과 | 8 / 1,206 | 4 / 506 | 4 / 700 |
| F 입력·커서·선택·게임 메뉴 | 12 / 818 | 4 / 266 | 8 / 552 |
| G UI 프레임워크·메뉴(Gump·HTML·Tell) | 22 / 7,561 | 11 / 4,219 | 11 / 3,342 |
| H 사제·운반·수확·경로 | 9 / 376 | 0 / 0 | 9 / 376 |
| I 건설·생산·경제·다리 | 16 / 1,483 | 1 / 48 | 15 / 1,435 |
| J 전투·비행·투사체·방어 | 9 / 657 | 1 / 163 | 8 / 494 |
| K AI | 1 / 28 | 1 / 28 | 0 |
| L 신전·제단·의식·승패 | 5 / 378 | 2 / 236 | 3 / 142 |
| M 미션 수명·안내·대본·튜토리얼 | 2 / 484 | 0 / 0 | 2 / 484 |
| N 소리·음악·동영상 | 13 / 3,349 | 4 / 2,041 | 9 / 1,308 |
| O 채팅·메시지 | 2 / 90 | 0 / 0 | 2 / 90 |
| X 요구사항 전용(4~14차) | 3 / 652 | 1 / 212 | 2 / 440 |
| Z 멀티플레이·LAN·Outpost | 6 / 842 | 0 / 0 | 6 / 842 |
| **합계** | **159 / 34,206** | 39 / 19,125 | 120 / 15,081 |

**재기준선에서 추가된 미완료 75단위의 단계 배정** (재기준선 판정 E498~E502의 후속 단계 기준, 단위 / 현재 줄):

- **4-D — 62단위 / 4,607줄**
  - 장기 월드 저장소 29단위 / 1,517줄([E498](docs/webpj-rebaseline-surface-20261115.md)) — Tutorial1 시작에 필요한 멤버만 4-D에서 옮기고 전투·의식 등 나머지는 소비자 단계(6·7)에서 연결한다: RawSurfaceAttack·RawSurfaceAssetDeletion·RawSurfaceAssetNotice·RawSurfaceBalloon·RawSurfaceBomb·RawSurfaceCarrier·RawSurfaceContent·RawSurfaceDaisDeletion·RawSurfaceDamageable·RawSurfaceDecoration·RawSurfaceDeletion·RawSurfaceDepend·RawSurfaceDudeRegular·RawSurfaceFactoryRegular·RawSurfaceFence·RawSurfaceFlare·RawSurfaceFrameRegular·RawSurfaceGump·RawSurfaceIce·RawSurfaceLifetime·RawSurfaceMana·RawSurfacePlayerRegular·RawSurfacePriest·RawSurfacePriestDeletion·RawSurfaceRegistryDeletion·RawSurfaceResidenceRegular·RawSurfaceReward·RawSurfaceRitual·RawSurfaceVortex
  - 전송·Native 수신기 14단위 / 1,077줄([E501](docs/webpj-rebaseline-transport-20261115.md)): RawPacketDelay·RawPacketFrame·ClientPacketSources·RawSquidReceive·RawSquidMutationReceive·RawSquidCompressedReceive·RawSquidVerification·RawPeerWorldSync·NativeLoginPacketReceivers·NativeGameplayPacketReceivers·NativeCarrierPacketReceivers·NativeConstructionPacketReceivers·ClientChatPacketReceiver·FortCoreDataLoad
  - 메타·전투 경계의 단독 도달 앞단 7단위 / 817줄([E502](docs/webpj-rebaseline-meta-20261115.md)): ClientMetaPacketReceivers·ClientChallengeGlobals·RawBattleCompletion·NativeMetaConnectedState·NativeBattleReturnState·ClientMetaSpriteRefresh·NativeMetaSpriteRefresh(두 Sprite 갱신은 5-C 화면 허브와 함께 확인)
  - 규칙·건설 장치 9단위 / 794줄([E500](docs/webpj-rebaseline-gameplay-20261115.md)): ClientConstructionGlobals·ClientConstructionRegions·ClientConstructionRandomPlacement·ClientConstructionPlacement·ClientConstructionDevices·ClientConstructionGem(Tutorial1 필요분은 4-D, 입력·건설 나머지는 6)·RawCampaignVortex·RawManoSpell·AssetDeletionVtables
  - 진단·Fort 저장 경계 2단위 / 303줄([E499](docs/webpj-rebaseline-boundaries-20261115.md)): SessionStateDump(웹 session-dump, 3.5절 1번)·NativeFortSaveVirtual
  - 시간 동기화 1단위 / 99줄: RawClockSynchronization(partial, 3.1절 4-D C)
- **5-B — 4단위 / 407줄**([E502](docs/webpj-rebaseline-meta-20261115.md)·[E500](docs/webpj-rebaseline-gameplay-20261115.md)): HudNsInfoMeta·NativePlayerInfoInput·NativeNsInfoInput·HudPlayerButtonDraw
- **6 — 2단위 / 199줄**([E501](docs/webpj-rebaseline-transport-20261115.md)): RawBuildReplay·ClientBuildReplay(건설 기록 재생)
- **9 — 1단위 / 41줄**([E499](docs/webpj-rebaseline-boundaries-20261115.md)): FrameRateOption
- **10 — 6단위 / 749줄**([E500](docs/webpj-rebaseline-gameplay-20261115.md)·[E499](docs/webpj-rebaseline-boundaries-20261115.md)): 12차 스커미시 SkirmishSetup·SkirmishStartIslands·RawIslandPartition·RawCentralIsland·RawBattleIsland, 표시 입력 DisplayTextInput

### 0.4 기준 값

| 항목 | 현행 기준 |
| --- | --- |
| cpppj 기준선 | **`225553df`(제품1313·표시 11.01).** 이식 원본은 기준선 판(`git show 225553df:경로`)이고, 이후 변경은 3.6절로 판정한다. cpppj 소스가 같은 마지막 변경 `979d65b5d`와 대조했다. **오라클 실행 파일**은 별도 빌드 `extracted/webpj-oracle/225553df/Netstorm.exe`(SHA-256 `5b52d96d…b77eb46`, Git 제외·HJOW-Athlon 전용)다. 이전 제품985 실행 파일과 오라클도 보존한다 |
| 이식 범위 | **`webpj/PORT_MAP.tsv` 1,160단위**(기준선 `o/`607·`client/`552·`zacket/`1). 분류: **port 959·adapt 51·exclude-mp 62·exclude-edit 55·exclude-host 32·exclude-legacy 1, 미분류 0**(E573·E588·E627~E629 캠페인 공유 함수 범위 복원). 상태: **port 851·partial 39·todo 120·제외(-) 150**, session/oracle 0. partial에는 기존 단위의 부분 이식 외에 0.5절의 초안 2개·E574 Session 기반 자료·E584·E585 Devastation 자격/타이머와 E595 Ritual 준비 함수·E601 건설 재생 실행/현재 라벨이 있다. 웹 소스는 `webpj/www`의 `.js`(자산 `data/` 제외) 134,031줄, 검사 파일 799개다(2026-11-17 재집계) |
| 이식 추적 | `webpj/tests/fixtures/manifest.json` **809항목**(original-x86 770·screenshot 13·source 10·independent 9·cpppj-oracle 5·original-binary 2, 2026-11-17 E629 갱신). 항목별 추가 이력은 단위 문서와 보관본에 있다 |
| 디컴파일 | webpj는 디컴파일 소스 분석이 필요하지 않다(AGENTS.md `c17aeede1`, 2026-11-15). 근거는 cpppj 소스·fixture이며 착수 때 디컴파일 호스트를 확인하지 않는다 |

### 0.5 매 단위 착수/종료

**미완성 초안 2개(partial):** `ClientRendererFrameGlobals`는 [E496](docs/webpj-renderer-cloud-queue-20261115.md)에서 구름 큐의 고정 칸 참조를 교정하고 실제 Renderer 앞단과 원본 관측에 대조했으나 프레임 본체의 실제 owner 연결이 남았다(5-A). `RawMinimapControls`는 2026-11-15 중단 시점의 처리기 초안과 집중 검사(최초 SID 풀 크기 오류 수정)까지이며 cpppj 전체 계약·원본 fixture·실제 메뉴/월드 허브 연결이 남았다(5-B). 두 항목은 체크 목록에서 완료로 세지 않는다.

1. AGENTS.md·이 문서·공통/dotnetpj 인계를 읽고 `git status`·`git fetch`를 확인한다. **같은 작업 트리에서 다른 세션이 cpppj·dotnetpj·공통 문서를 동시에 커밋할 수 있다(2026-11-06 실제 관측).** 공통 인계는 고치지 않는다. cpppj는 2026-11-16 사용자 허용에 따라 필요한 소스 수정이 가능하되 완료 기능의 관련 검사를 통과해야 한다. cpppj 상태는 커밋 기록과 소스로 판단한다.
2. `node webpj/tools/port-map.mjs`로 대응표 검사와 기준선(행 base) 뒤 cpppj 변경을 본다. 이번 단위가 옮길 항목은 **3.1절의 현재 단계 목록에서 위에서부터** 고른다(대괄호 선행 관계를 지킨다). **기준선 판**의 소스(`git show 225553df:경로`)·그 파일을 대상으로 하는 cpppj 검사·fixture·단위 문서를 함께 읽는다. 기준선 뒤 그 파일이 바뀌었는지는 저장소 루트에서 `git diff --numstat 225553df HEAD -- <파일>`로 확인하고, 바뀌었으면 3.6절로 판정해 판정 커밋을 base와 단위 문서에 적는다. 디컴파일이 꼭 필요하면 마지막 수행 PC와 현재 PC를 먼저 비교하고 AGENTS.md의 재디컴파일 규칙을 따른다.
3. 원본/클론 게임(cpppj 오라클 수집 포함)·analyzeManager·별도 창 실행은 AGENTS.md의 확인/예외 규칙을 적용한다. 단위 검사나 정적 확인을 실제 실행으로 기록하지 않는다.
4. 3.0절 방식으로 구현한다. 필요한 공유 장치가 없으면 먼저 이식하고, 빈 성공 함수·별도 근사 월드로 우회하지 않는다. 멀티플레이 분기는 1절의 경계 규칙을 따른다.
5. 근거 커밋·연결 fixture/오라클(SHA/행 수)·검사와 최초 실패/교정·실제 연결/미연결·남은 경계·다음 시작점은 **단위 문서**에 남긴다. `PORT_MAP.tsv`의 web·base(정확히 8자리)·status·메모와 manifest(`node webpj/tools/fixture-manifest.mjs --write --only <아이디,…>`)를 갱신하고 `node webpj/tools/port-map.mjs` 오류 0을 확인한다. 이 문서는 **0.1절(머리글·진행도·마일스톤의 현재 단계·마지막 구현 커밋·다음 작업·마지막 전체 검사)·0.2절 진행 칸·0.3절 수치·0.4절 개수·3.1절의 해당 항목(`- [x]`와 단위 링크)·단계 머리글 `(n/m)`**만 고친다. 단계를 마치면 0.2절에서 다음 단계를 **진행 중**으로 바꾸고, 끝난 단계의 체크 목록은 보관본으로 옮겨 2.3절에 한 줄로 남긴다. 단위별 긴 설명과 기록 줄은 이 문서에 쌓지 않는다(7절). 이번 단위 파일만 이름으로 stage하고, 커밋 직전에 `git log --format=%s`로 그날 번호의 최대값(CP·DN·WB 공통 계열)을 다시 확인한 뒤 `YYYYMMDD-NN WB 요약`(끝에 Co-Authored-By 트레일러)으로 로컬 커밋한다. push하지 않는다.
6. **검사:** 커밋 전 전체 `node --test "tests/*.test.mjs"`(webpj 폴더)를 Node v26과 v22 두 버전으로 돌린다(1절 Node 지원; 2026-11-16 사용자 지시에 따라 v24는 생략·호환 가정). 문서 갱신 보조 스크립트는 저장소에 없으므로 이 문서는 위 5번 칸을 손으로 고치거나 필요하면 스크립트를 새로 만든다.

### 0.6 단계와 무관한 AGENTS 요구 (2026-11-11 점검)

AGENTS.md가 세 클론 모두에 바로 요구하는 항목이다. 단계 진행과 관계없이 지켜야 하며, 미반영 항목은 작은 WB 단위로 먼저 처리할 수 있다.

| 요구 | webpj 상태 | 할 일 |
| --- | --- | --- |
| 버전 11.01 표기·Credits 세 번째 항목 `Netstorm 11.01 Patch Credits`·창 제목 `NetStorm 11.01 Patch Credits`·HJOW 개발과 AI 사용 내용 | 완료(WB03-AC, [제품11.01](docs/webpj-product-1101-20261105.md)). 한국어 행 이름 `NetStorm 11.01 패치 크레딧` 확정 | — (AI 문구는 4절 "Credits AI 문구" 결정 유지) |
| **11.01 카피라이트 `Copyright 2026 HJOW`** (사용자 커밋 `60226ea9`, 2026-11-11) | **완료**(WB03-AE, [제품11.01 6절](docs/webpj-product-1101-20261105.md)) — 영어·한국어 Patch Credits 본문의 개발 줄 다음에 `Copyright 2026 HJOW` 줄, `tests/product.test.mjs` 위치·문구 단언 | — (일본어 본문은 13차에서 다른 언어를 추가할 때 같은 줄을 넣는다. 원본 비교 모드의 10.78 두 행 Credits는 그대로) |
| 메인 메뉴 전 "클릭하여 시작" 화면 | 완료(WB01) | — |
| 외부 도메인/IP 접속 문서화·클라이언트 유효성 검사 비활성화 | 완료(WB02-E, [외부 접속](docs/webpj-external-access-20261105.md)): 런타임 요청은 같은 출처 `./data/`뿐이고 유효성 검사·서버 진단은 연결하지 않음 | 새 외부 요청(13차 CDN 글꼴 등)을 만드는 단위는 문서와 `tests/external-access.test.mjs` 감사를 함께 고친다 |
| 글꼴: 한국어는 `fonts/` D2Coding, woff2가 없는 글꼴은 Google Fonts CDN | 한국어는 `D2Coding.woff2`·`D2CodingBold.woff2`를 복사해 사용. `fonts/`의 Noto Sans·Noto Sans JP는 TTF뿐이라 쓰게 되면 CDN 대상 | 13차 착수 때(3.7절·4절 "다른 언어 글꼴") |
| 에셋은 `webpj/www` 안으로 복사해 사용 | 준수(`prepare-assets` → `www/data`) | — |

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
- **보호/분담:** AGENTS.md·보호 원본/CD를 수정하지 않는다. **공통 인계 LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md와 dotnetpj 파일은 WB 단위에서 수정·커밋하지 않는다**(2026-11-06 사용자 주의). **cpppj 소스는 현재 작업이 진행 중이지 않아 필요 시 수정 가능**하다는 2026-11-16 사용자 지시가 우선하며, 완료 기능을 보존하고 관련 기존 검사를 확인한다. stage할 파일은 이름으로 지정한다. README 등 공통 파일은 기존 사용자 결정대로 일괄 갱신 때 반영한다. 새 문서/코드는 UTF-8, 새 상수·함수·자료형·클래스·멤버·반복문에는 설명/사용법을 먼저, 추가/수정 시점과 이유를 뒤에 쓰는 한국어 주석을 단다.

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

### 2.4 4-R·4-D·5-A·5-B 진행분과 선행 부품 (E329~E584)

E329~E495의 체크 항목과 단위 링크는 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)의 3.1절·3.8절에, E496~E584에서 끝난 체크 항목은 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)의 3.1절에 있다. 단위 문서는 `grep -l "WB04-E570" docs/webpj-*.md`처럼 번호로 찾는다.

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
| 4-D D `RawCampaignSession 1` (진행 중) | E574~E584 | 생성자의 기반 owner·World·Player/Core·난수/SP(E574), 공유 입력·진단(E575), Player 공장·동맹·Form/방송 테이블(E576), 섬·소리·사제·SP·종료(E577), Player152·Template·신전/섬 소유권(E578), Carrier/Balloon·TYPE 파생표(E579), 총 검색·에너지·마나/사정거리 표시(E580), 피해 상태·AI 통지·Hit 효과(E581), GunHit·투사체·Gun 장애물(E582), 소유자색·파편 캡처·팔레트/입자(E584) |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식 (2026-11-06 재계획)

- **단위 크기:** 한 단위는 cpppj 파일 하나 또는 함께 움직이는 묶음(같은 클래스의 `.cpp/.h/.inc`와 생성 자료)과, 그 파일을 대상으로 하는 cpppj 검사로 이룬다. 함수 하나씩 나누지 않는다. 큰 파일은 공개 API 경계에서 나누고 남은 부분을 단위 문서와 PORT_MAP 비고에 적는다.
- **대응표 `webpj/PORT_MAP.tsv`:** 열은 `cpp`(저장소 기준 cpppj 경로)·`class`(분류)·`web`(웹 모듈 경로, 여럿이면 `;`)·`base`(이식·대조를 마친 cpppj 커밋)·`status`·`note`다. 분류는 `port`(이식 대상), `adapt`(창·입력·소리·파일·시계·캡처처럼 웹 대체가 필요한 플랫폼 경계), `exclude-mp`(1절의 LAN/멀티플레이 제외 목록), `exclude-edit`(Edit·UberEditor·Editor 계열), `exclude-host`(DirectDraw·Direct3D 11/FSR·MCP·CD 탐지·레지스트리·설치·Win32 진단), `exclude-legacy`(raw 풀 이전의 옛 단독 모델로 production 소비자가 없고 raw 구현이 대체한 단위)다. 상태는 `todo` → `partial`(웹 모듈이 파일 일부만 가짐) → `port`(이식, L1/cpppj 검사 통과) → `session`(앱 수명 연결) → `oracle`(cpppj 오라클 비교 통과) 순서로 올리고 제외 분류는 `-`다. **분류는 `tools/port-map.mjs`의 `RULES`가 정하며 대응표에서 손으로 바꾸지 않는다**(규칙과 사유를 고친 뒤 `--write`, 사유 조회는 `--why 이름`). `base`는 웹과 대조·판정한 cpppj 커밋이며 `todo` 행에는 기준선 뒤 변경을 판정했을 때만 적는다. 이식 원본은 언제나 기준선 판이다. 새 `www/core/*.js` 모듈은 반드시 대응 행의 web 칸(여럿이면 `;`)이나 `WEB_ONLY`에 넣는다(빠지면 전체 검사 실패). dotnetpj의 [PORT_MAP](dotnetpj/PORT_MAP.md) 형식을 참고하되 별도 파일로 둔다.
- **검사 이식:** cpppj `tests/`의 대상 검사를 같은 이름의 Node 검사로 옮긴다. 원본 x86 fixture는 `cpppj/tests/fixtures`를 직접 읽고 복사하지 않는다. 웹 적응·제외 경계를 확인하는 웹 전용 검사는 따로 표시한다.
- **생성 자료:** 생성자·vtable·타입·패킷 길이처럼 큰 표는 cpppj 소스에서 생성 스크립트로 만들고 `--check`로 재생성 결과가 같은지 검사한다(WB04-D4 `squid-constructors.js` 방식).
- **수직 진행:** 범위를 넓히기 전에 Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 통과시킨다. 그다음 Early Missions·캠페인 1-1/1-2, 34경로 순으로 넓히며, 각 단계에 필요한 파일만 PORT_MAP에서 골라 옮긴다. 2026-11-10 단계 분할(1절)은 이 원칙으로 돌아가기 위한 것이다.

### 3.1 단계별 항목 목록

순서3(3-A~3-E)·4-A~4-C·4-R은 마감됐다(2.3·2.4절, 보관본). 현재 단계는 **4-D**다. 5-A·5-B는 4-D가 선행 대기로 막혔을 때 병행해 열었고, 5-C·6·7·8은 2026-11-15에 미리 펼쳤다. 끝난 체크 항목은 단계가 끝나기 전이라도 문서 정리 때 보관본으로 옮기며, 머리글 `(n/m)`에는 계속 센다.

- **진행 방법:** 단계는 0.2절 순서대로, 단계 안에서는 위에서부터 진행하고 대괄호 `[… 뒤]` 선행 관계를 지킨다. 항목을 끝내면 `- [ ]`를 `- [x]`로 바꾸고 끝에 ` → [E번호](단위 문서) 요약`을 붙인다. 한 항목을 여러 단위로 나누면 링크를 이어 붙이고 마지막 단위에서 체크한다. 단계 머리글의 `(n/m)`과 0.2절 진행 칸을 함께 고친다.
- **수치:** 줄 수는 기준선 소스 줄(partial은 파일 전체 줄), `(+N)`은 기준선(또는 행 base) 뒤 cpppj 변경 줄 수다. partial 항목은 PORT_MAP 메모의 "남음:" 부분만 해당한다.
- **기준선 뒤 변경:** `(+N)`이 있는 항목은 **기준선 판을 옮기고** 바뀐 부분을 3.6절 ①/②/③으로 판정해 단위 문서·PORT_MAP base·메모에 남긴다(E213 선례: 기준선 typed 경로를 옮기고 Native 분기는 3.6절 ③ 표에 적음). 변경이 있다는 이유로 건너뛰지 않는다.
- **목록 밖 단위:** 다른 단계 몫을 끼워 넣지 않는다. 항목이 선행 미완료로 막히면 사유를 그 항목에 적고 같은 단계의 다음 항목으로 간다. 단계 전체가 막히면 사유를 0.1절 다음 작업에 적고, 선행이 갖춰진 이월 부품(6-P·7-P) 항목을 진행할 수 있다. 목록에 없던 단위가 꼭 필요하면 해당 단계 목록 끝에 항목을 추가하고 이유를 적는다.
- **끝난 단계:** 단계 머리글이 `(m/m)`이 되면 그 체크 목록을 보관본으로 옮기고 2.3절 표에 한 줄을 더한다(0.5절 5번).

#### 4-D Session 조립·Tutorial1 Load/Start (41/56)

**완료 기준(3.2절): L2** — Tutorial1 Load/Start 관측(보고서·session-dump·첫 프레임 논리 화면)이 cpppj와 같다. 임시 Player/AI/색 번호로 시작 성공을 만들지 않는다. 선행 4-B·4-C·4-R은 완료다.

- **조립 방식:** 실제 `Client`/`GameWorld`/`RawCampaignSession`은 D·E 구획에서 조립한다. 그 전 항목은 앞 단위(E325~E328)처럼 **client 계약(최소 덕타입)**을 모듈 머리글에 적고 시험이 그 형태의 장치를 만든다. 실제 Session/Native 장기 장치 설치와 cpppj 시험의 실제 조립 재대조는 D·E에서 한다(아래 "끝난 항목이 남긴 재대조").
- **순서:** 앞 항목이 뒤 항목의 입력이므로 위에서부터 진행하고, 선행 미이식으로 막히면 사유를 그 항목에 적고 다음으로 간다. 큰 단위는 공개 API 경계로 나누어 착수 때 세부 항목을 더한다.
- **줄 수:** 항목의 줄 수와 `(+N)`은 이전 기준선 `1f00a25a`와 그 뒤 추가분 기록이다. 현재 기준선 `225553df` 소스를 착수 때 확인하고 그 이후 변경은 3.6절 ①/②/③으로 판정한다.
- **끝난 항목:** 이번에 옮긴 16개(A 1·B 1·C 14, E518~E573·E583)는 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584), 그 전 13개는 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)의 3.1절에 단위 링크와 함께 있다. 요약은 2.4절이다.

**A. Client 수명 전역 소유자** (참조를 빌려 주는 소유자, 값 사본 없음)

- [ ] client **ClientState 정리 4모듈** — State/World 전역과 정리·Runtime 반납 순서
  - 대상: ClientStateWorldGlobals 108줄(+8)·ClientStateWorldCleanup 101줄(+2)·ClientStateStorageCleanup 71줄(+1)·ClientStateRuntimeRetirement 99줄
  - 막힘(E336 판정): `ClientStateWorldContext`가 `Client`·`CampaignTemplateStorage`(B 구획)·`RawMenuControls`(L 영역)·`ClientAudio`(H 영역)·`RawSurfaceStorage`의 실제 저장소에 Native 주소별 접근을 연결하는 바인딩이라 그 소유자들이 선행이다 — B 구획과 Session 조립(C·D) 뒤
- [ ] client **사용자 선택·커서 전역 4모듈** — 네 모듈이 끝나면 체크
  - 완료: ClientCursorGlobals 25줄·ClientCursorStateCleanup 27줄([E402](docs/webpj-client-cursor-globals-20261114.md), 장기 주소/reset·실제 Cancel/Prepare/현재 카메라·Renderer 연결), ClientUserSelectionGlobals 77줄([E495](docs/webpj-user-selection-native-fixtures-20261115.md), RawUserSelection/RawUserPick과 함께, fixture multiple-selection 601행)
  - 남음: **ClientUserSelectionCleanup** 43줄 — `RawCampaignSession.core()` Session 허브와 `RawSquidSelection` 표시 갱신 연결을 기다린다
- [ ] client **채팅·HTML·렌더러 프레임·점 명령 전역 5모듈** — 다섯 모듈이 끝나면 체크
  - 완료: ClientChatManagerGlobals 123줄(E447)·ClientDotCommandGlobals 56줄(E443)·ClientChatWindowGlobals 24줄(+4)([E453](docs/webpj-client-chat-window-globals-20261114.md))·ClientHtmlWindowGlobals 8줄([E456](docs/webpj-html-window-gump-20261114.md))
  - 남음: **ClientRendererFrameGlobals** 26줄(partial 초안, 0.5절) — [E496](docs/webpj-renderer-cloud-queue-20261115.md)에서 구름 고정 칸과 실제 Renderer 앞단을 검증했다. 실제 owner는 5-A `ClientRendererFrame` 조립 때 연결한다

**B. 미션 시작 경로** (Init → Fort 요청 → State7 → Zone/Guide)

- [ ] client **Session에 주입하는 Client 서비스 3모듈** — CampaignClientServices 333줄(+56)·CampaignUserInputServices 34줄(+4)·CampaignCursorClientEffects 72줄(+2, partial)
  - 막힘(E348 판정): `RawCampaignSession.h`의 서비스 형식과 `Client`·`GameWorld`·`RawButtonControls`를 요구하는 접합 코드 — D(RawCampaignSession) 뒤

**C. Session 바인딩** (Tutorial1 Load/Start에 닿는 것만 먼저, 나머지는 6·7·8단계에서 연결)

- [x] o **RawDaisProcessData** 107줄 — 생성자1815 DaisTransmission 합성의 선행 원문40BYTE·빈 Process·6ah 선택 수신/writer와 실제 Host/Kernel 수명을 옮겼다. cpppj 전용 시험/원본 fixture 없음: 소스 계약6건(모든 mask·parent/force1024조합·부분 실패·실제 Host) 통과. 실제 Session/Native6ah 수신 결선은 잔여다 → [E617](docs/webpj-dais-process-data-20261117.md)

- [ ] o **RawClockSynchronization** 99줄(partial, 재기준선 단위)
  - 완료: 일반 등록·서버 초기화·연결·수신([E515](docs/webpj-clock-synchronization-receive-20261115.md)), CRT 표본 정렬을 합성 350건과 추가 3,500건으로 대조([E516](docs/webpj-clock-sort-20261115.md)), Poll 본체와 실제 cpppj 장면 24건 대조([E517](docs/webpj-clock-poll-20261115.md))
  - 남음: 고정 Native 수신기 baseline 수명, Session 실제 단독 연결(D 구획)
- [ ] client **나머지 `RawCampaign*Bindings`** — Tutorial1 Load/Start가 요구하는 것만 앞당겨 이 구획에 개별 항목으로 더하고, 나머지는 해당 단계에서 옮긴다
  - 이미 끝남: RawCampaignCursorDisplay(E407)·RawCampaignParticles(5-A)·Hit(E564)·GunQueries(E565)·Archer(E567)·Motion·Path(E568)·Cursor(E569)·RawBombBlastBindings(E583)
  - 남은 것의 배정: RawCampaignDevastation(partial, E584 파편 캡처 자격·E585 빈 타이머/타이머용 펜스 Rebuild 장치만)·RawCampaignMeltdown·RawCampaignRitual → 7단계, RawCampaignCarrierInput·RawCampaignCarrierDrop·RawCampaignPrayer·RawCampaignPanic → 6단계, RawCampaignVortex(재기준선, E589 typed 설치/공유 helper는 partial; Native 장기 대여·전체 Runtime 결선 잔여) → 4-D 규칙 묶음(0.3절)

- [x] o **RawConstructionFailurePackets** 33줄 — 생성자1652의 환불/취소 기본 효과가 요구하는 선행 모듈이다. signed 금액/좌표 비트·모든 matching abstract flags8 삭제와 현재 상태 재조회 source 계약7건 통과. 실제 Session/Native 수신 결선은 잔여다 → [E602](docs/webpj-construction-failure-packets-20261116.md)

- [x] o **RawVortexDeparture** 165줄 — D 구획 생성자1537행 `InstallVortexDeparture`가 요구하는 공통 부모라 선행 항목으로 더했다. 신전51h 수신·섬 상승 완료·Fort 지역/6BYTE 기록·현재 표면/추가 후보 정리. 전용 cpppj 시험/원본 관측은 없으며 실제 Pool/Hash/섬 목록/Ascend의 신규13 소스 계약 검사로 대조했다. 실제 Session/Native 설치는 다음 바인딩이다 → [E587](docs/webpj-vortex-departure-20261116.md)

- [x] o **RawFortTerritoryRecover 공유 writer** 73줄 — `RawCampaignVortex.inc:24`도 사용하는004bd600 쓰기와 Session3489..3505의 실제 Template9 결선. 원본 writer200행 전체 raw/trace와 신규5검사 일치. 회복004b5ec0/774행은 ExistingEditor 전용 제외다 → [E588](docs/webpj-fort-territory-write-20261116.md)

- [x] client **CampaignDamageableRegularTables** 7줄 — 원래7단계 표를 D1 예약 피해가 먼저 소비하므로 선행 항목으로 더했다. 원본83VT 정적 자료의 공통13·일반Hit31·BaseHit45 범위를 전수 대조했으며 함수 실행 수를 늘리지 않는다 → [E590](docs/webpj-session-assets-20261116.md)

- [x] o **ProcessForm 공용 writer 부모** — 단독 RawSurfaceWorld.cpp176..179가 요구하는 WriteFormPacket/현재·장기 기본 전송 표/Regular writer 선택과 실제 pin·호출 수를 먼저 연결했다. 신규15건·관련45건 통과. Source의 NoWorld 기본 Lifecycle/FormFactory와 상위 Storage/World 송신은 잔여다 → [E591](docs/webpj-process-form-writer-20261116.md)
- [x] o **Factory 현재 World 대여 부모** — 같은 Pool/현재 Unpop·종속 분리·내용물 TYPE·전송·호출 전체 pin과 명시 반납을 실제 World에 연결했다. 신규11건·관련89건 통과. 공유 postPopState/Factory의 Storage 장기 소유권 전환은 다음이다 → [E592](docs/webpj-factory-runtime-20261116.md)
- [x] client **장기 Factory/공유 상태와 실제 기본 부모** — Factory·공통 장부/깊이·Display/Unpop/Building/기본 종속 분리를 Storage 한 객체로 전환하고 현재 World·Session 대여를 연결했다. 신규17건·관련140건 통과. 실제 Squid 송신/Native 기본 Form writer와 나머지 실행기 소유권은 잔여다 → [E593](docs/webpj-surface-factory-lifetime-20261116.md)
- [x] client **실제 공용 Squid 송신/삭제 부모** — Storage 한 송신기/예약·현재 World/장기 Host·현재 HP·typed 대상을 실제 writer에 연결하고 임시 함수 대체를 제거했다. 신규19건·관련174건 통과. Native mutable Client/기본 Form writer·Peer 동기화는 잔여다 → [E594](docs/webpj-surface-transmission-20261116.md)

- [x] o **RawFortSnapshotParent** 111줄 — 외곽 callback의 캠페인 공유 snapshot 부모를 편집기 전용 제외에서 교정했다. 네 진입의 현재 전역/고정 this·stream/부분 중단378 포함 원본2,728행을 전수 대조했다. 실제 Template/자식 직렬화·Session callback 결선은 잔여다 → [E627](docs/webpj-fort-snapshot-parent-20261117.md)

- [x] o **RawFortSnapshotBasics** 86줄 — 캠페인 공용 TypeNames/Deck writer를 제외에서 교정하고 원본2,060행을 대조했다. 같은 실제 Template9/State·생산 덱에 고정 stream을 연결하여 신규4건 첫 통과. 외곽 snapshot/callback·Load는 잔여다 → [E628](docs/webpj-fort-snapshot-basics-20261117.md)

- [x] o **RawFortContentSave** 110줄 — 캠페인 기술/객체 공용 내용물 writer를 제외에서 교정하고 원본706행을 대조했다. 같은 실제 Form6/Robust·독립 finder·Template stream의 쓰기/무효화·설치 전 공개 관문을 연결하여 신규3건 통과. Native 가상 대여/전체 snapshot·외곽/Load는 잔여다 → [E629](docs/webpj-fort-content-save-20261117.md)

**D. Session 본체** (`RawCampaignSession` 7,118줄 = 헤더 712줄 + `.cpp` 6,406줄, 기준선 `225553df`) — 공개 API 경계로 나눈다

- [ ] **RawCampaignSession 1: 구성·소유 상태·Load** (partial, 진행 중)
  - 범위: 생성자(Impl 상태)·`Load`·Player/동맹/기술/SP/Deck/AI 초기화·`RawSurfaceWorld`/`RawWorldCore` 접합(같은 Storage의 실제 Squid Pop/삭제·현재 Host 결선 E594)
  - 4-B 이월: 저장 객체 `FortObjectLoad`/Money/Meta child의 실제 적재와 session-dump 대조
  - 결선 완료(`.cpp` 1536행까지, E574~E586): 실제 owner/World·공유 입력·Player/SP·진단 → Player 공장·동맹·Form/방송 테이블 → 섬/사제/Sound·SP·종료 → Player/Template/소유권 → Carrier/Balloon/TYPE 파생표 → Gun/에너지/커서 표시 → 피해 상태/AI/Hit 효과 → GunHit/투사체/Fence/Blocker → 소유자색/파편 캡처/Particles → TeleportPrefix/현재 TYPE 생성자 ABI/연소 전역 → Archer/Missile/IceRicochet([기반 자료](docs/webpj-session-owners-20261116.md)·[최근 연결 E586](docs/webpj-session-combat-20261116.md))
  - 공유 쓰기 선행: `WriteEditorTerritory`3489..3505 실제 Template9/현재 노드 수명 결선([E588](docs/webpj-fort-territory-write-20261116.md)). 편집기 회복 함수는 제외다.
  - Vortex 생성자1537의 typed 상승/수신·공유 명령/큐/WORD 통지 결선([E589](docs/webpj-session-vortex-20261116.md)). Native 장기 Construction/Storage·외곽 Runtime/Load/Login은 잔여다.
  - 자산 생성자1538..1610의 실제 AssetBindings/PostPop 등록·거주 구역/예약 피해·가상80·Gun/Factory 효과 결선([E590](docs/webpj-session-assets-20261116.md)). 첫 저장 객체/전체 Kernel6699·Native는 잔여다.
  - 전송 선행: ProcessForm Host writer/현재·기본 전송 표/실제 pin 결선([E591](docs/webpj-process-form-writer-20261116.md)), Factory 실제 World 대여/호출 전체 pin([E592](docs/webpj-factory-runtime-20261116.md)). 공유 postPopState/Factory·표시/Unpop의 Storage 소유권 전환([E593](docs/webpj-surface-factory-lifetime-20261116.md))과 실제 공용 송신/삭제([E594](docs/webpj-surface-transmission-20261116.md))까지 마쳤다. Runtime1611..1649 값 구성/현재 준비 함수([E595](docs/webpj-session-runtime-20261116.md))도 완료했으며 실제 게시1825는 남았다.
  - Runtime1611..1649: 실제 표적/피해/입자·Dude/거주/공장·의식후단·Player/Peer·공용 삭제/의존·주문 훅 값 구성과 현재 상태 준비 완료([E595](docs/webpj-session-runtime-20261116.md)). 후속 모듈 설치/World 게시1825는 잔여다.
  - InstallContents3020..3049 통지 접두: Gump 등록/공통 삭제·현재 SP/진단 Player 통지·현재 Form 상태/포획 안내 부모 완료([E596](docs/webpj-session-content-notices-20261116.md)). 수명/운반/생산은 잔여다.
  - InstallContents3050..3134 수명/운반·현재 Unpop/가상 부착·ContainedCreate 완료([E597](docs/webpj-session-content-lifetime-20261116.md)). 같은 실제 Sound47 예약 객체를 한 번 부착하며 생산은 잔여다.
  - InstallContents3135..3156 실제 기술·TotalMade/금지표·장기 Deck 메모리·생산/동기화 완료([E598](docs/webpj-session-production-20261116.md)). Factory 연구/업그레이드/입력은 잔여다.
  - InstallContents 마지막 Factory 연구/업그레이드/입력 완료([E599](docs/webpj-session-factories-20261116.md)). typed 생성자1650 조립을 마쳤으며 실제 World 게시/외곽 Session은 남았다.
  - 생성자1651/ConnectFlare의 현재 SHP·난수·World/실제 Regular 예약/부착·시각·소리 연결 완료([E600](docs/webpj-session-flare-20261116.md)). Native/World 게시·실제 효과 프레임은 잔여다.
  - Construction 선행: 환불/배치 취소 wire와 같은 Pool/Hash의 모든 matching abstract 삭제 완료([E602](docs/webpj-construction-failure-packets-20261116.md)). Session 송신/Native 수신기 결선은 잔여다. 건설 재생의 Raw Run/라벨·Client 현재 context/수명/라벨 부모 완료([E601](docs/webpj-build-replay-runtime-20261116.md)). FILE/Host/키는6단계 잔여다.
  - 생성자1652/ConnectConstruction2731..2774의 같은 World/SP/방송·장기 최근5칸/방향9DWORD·현재 재생·기본 mutable 패킷/후설치 건물·Graph·이동 소비자 완료([E603](docs/webpj-session-construction-20261116.md)). Native 장기 건설 owner/전체 World 게시는 잔여다.
  - 생성자1653/ConnectDamageable2626..2705의 같은 Dude/해방·피해전후/신전·간헐천 수명 결선 완료([E604](docs/webpj-session-damageable-20261116.md)). 실제 Session Load 뒤 Dude 도착/낙하·경로와 붕괴 캡처 재대조는 남았다.
  - 생성자1654/ConnectIceTower2145..2173의 같은 실제 Regular 예약/부착·절삭Hash/WORD·현재 Server/boss/loading/후처리 결선 완료([E605](docs/webpj-session-ice-20261116.md)). World/Load 뒤 실제 Kernel 재성장 반복/삭제 관문 재대조는 남았다.
  - 생성자1655/ConnectCarrierSteal2419..2440의 실제 Form6·현재 방송/이동·공유 카운터/헤더·492 예약/가상 PostPop 결선 완료([E606](docs/webpj-session-carrier-steal-20261116.md)). 실제 Load/40경로·송신 전 효과/60h 등록은 후속 재대조다.
  - 생성자1656..1658의 BombPostPop/AcidZap/AcidFence와 같은 실제 난수/Host·61h/공간·피해/친군사격 결선 완료([E607](docs/webpj-session-spells-20261116.md)). 실제 Load/Kernel·61h 등록/캡처/자동 처리는 후속 재대조이며 공중3종 전용 실행은 제외했다.
  - 생성자1659..1664의 같은 너겟/미리보기·현재 표면/낙하·공유 task/UI 약한 수명 결선과 유한 delay float 저장 교정 완료([E608](docs/webpj-session-carrier-regular-20261116.md)). 신규16건/관련41건 통과. 실제 World 분배/Load·포획은 후속 재대조다.
  - 생성자1665..1667의 세 Aviary를 같은 현재 Host/역할·시계/설정 owner와 후설치 Flyer에 결선 완료([E609](docs/webpj-session-aviaries-20261116.md)). 새3건은 조립/현재 참조만 확인했다. 공중3종 새 전용 생성/생산/행동 검사는 제외하며 실제 World/Load는 후속이다. 후속1668 ArcSpire까지 연결했다.
  - 생성자1668의 ArcSpire와 같은 현재 자산 range/피해율·flags1 finder·동맹/편집·빔/피해/프레임·공통 Regular 결선 완료([E610](docs/webpj-session-arc-spire-20261116.md)). 신규7건·기존 원본665행 재생 통과. 실제 World/Load·Kernel/교전은 후속이다.
  - 생성자1669의 Flyer Process/Frame/Target·후처리·현재 SID 진단/86h writer 결선 완료([E611](docs/webpj-session-flyers-20261116.md)). RawCheckingFlag 포인터 값형 복사도 보존했다. 신규5건은 조립/참조/메타·플래그이며 공중3종 새 전용 실행은 없다. 실제 World/Load/Native·비행은 후속이다.
  - 생성자1670의 Fence 아홉 몸체와 같은 현재 TYPE 사거리/동맹·실제 표식/소유권·공유 Regular 예약/현재 시계·수명 피해 전처리 결선 완료([E612](docs/webpj-session-fence-20261116.md)). 신규8건/관련30건 첫 통과. 실제 World 분배/게시·Load·Native는 후속이다.
  - 생성자1671..1679의 GemReward/Buried와 같은 현재 지역 상태/건설 제외 목록·게임 난수/실제 Deck 장치·Host 두 예약/공통 장부 결선 완료([E613](docs/webpj-session-gems-20261116.md)). 신규7건 교정 후 통과·관련17건 통과. 실제 World 분배/게시·Load·Native는 후속이다.
  - 생성자1680..1756의 독립 typed 피해→내용물→건설·사제 삭제 래퍼와 현재 VT/TYPE 수명 자격·파생 Pre/Post·최종 가상 삭제 분배 결선 완료([E614](docs/webpj-session-deletion-20261116.md)). 신규8건/관련16건 첫 통과. Native 진단/장기 수명·실제 World 게시/Load는 잔여다.
  - 생성자1757..1814의 독립 typed BuriedRegular·현재 Player/사제 후단과 실제 중첩 Regular·다리/장식/특수 주문·실패 콘솔 결선 완료([E615](docs/webpj-session-regular-20261116.md)). 신규7건 첫 통과·관련55건 통과. Native 장기 Regular와 실제 Runtime 게시/Load는 잔여다.
  - 생성자1815..1824의 같은 실제 Dais→Aviary→Construction→전투/자산 송신 합성·현재 헤더/버퍼·낙하/다리/로딩 깊이 결선 완료([E618](docs/webpj-session-transmission-20261117.md)). 신규8건/집중18건 통과. 공중3종 새 전용 실행 없음, 실제 Native 장기 설치/World 게시/Load는 잔여다.
  - 생성자1825..1865의 실제 World Runtime 게시·같은 풀의 전체 배치/일반 지형·현재 군도/Player root·소유자 후처리 조립 완료([E619](docs/webpj-session-terrain-20261117.md)). 신규7건/집중33건 통과. FrameCodeTable/Canon 진단의 실제 조립 형식을 교정했고 Native 장기 설치·외곽 공개/Load는 잔여다.
  - 같은 Template의 TypeNames·Objects/Contents 원시 reader와 현재255BYTE 변환표·실제 ContainedCreate 결선 완료([E621](docs/webpj-session-streams-20261117.md)). 신규6건/집중23건 통과. 지역/전역 manager를 구별하며 실제20 Terr·Chaff/부분 범위와 실패 깊이·현재 할당을 보존한다. 전체 Runtime/Load의 실제 소비는 잔여다.
  - 생성자1866..1871 최초 Template 분기/MakeRuntime과 실제 Fort 사제 준비·공유 population/alternate 부모 결선 완료([E622](docs/webpj-session-install-20261117.md)). 신규9건/집중36건 통과, typed 역할15주소는 보호 PE와 직접 대조했다. 실제 motion/전체 MayPlace·AI·Load/외곽 공개는 잔여다.
  - 생성자1871..1880 지역/승패 현재 입력과 ConnectAi2777..2834의 실제 참여·본체/초기/생성 전 동맹·난이도/HP 지갑·소멸 반납 결선 완료([E623](docs/webpj-session-ai-brains-20261117.md)). 신규7건/집중35건 통과, AiAllianceSide27줄을 선행 이식했다. 전체 AI/Load·외곽 공개는 잔여다.
  - ConnectAi2835..2879의 실제 생성/삭제 통지·World 등록·준비/전략/경제와 현재 Path 목표/선택 수명·반납 결선 완료([E624](docs/webpj-session-ai-world-20261117.md)). 신규6건/집중46건·메타데이터10건 통과. 실제 Player 가상 삭제 전체와 행동/외곽/Load는 잔여다.
  - ConnectAi2880..2902의 실제 배치/Regular·콜렉터·다리 후보/탐색·시설 생산/부족 좌표·건설 선택 결선 완료([E625](docs/webpj-session-ai-production-20261117.md)). 신규5건/집중81건 통과, 다리 역할/FrameCodeTable 결함을 교정했다. 생성자 내부 조립은 마쳤고 외곽 공개/Load·Motion 뒤 AI Movement/Runtime은 잔여다.
  - World347..370의 같은 실제 Runtime pin 전달과 Factory/Host·PostPop/종속 Unpop의 정상/예외 범위 반납 완료([E626](docs/webpj-runtime-pin-20261117.md)). 신규6건 통과. Native 미설치 장기 묶음·외곽 공개/Load는 잔여다.
  - **다음:** 외곽 생성자4318..4330의 callback/공개 수명 → 같은 Template의 raw reader로 `Load` → 실제 Login(2번 항목). cpppj1320의 선행 내부 미션 전환 재반영은 E620에서 마쳤다
- [ ] **RawCampaignSession 2: 로컬 연결·로그인** — `LoadLocalConnection`/`PrepareLocalConnectionForState`/`ConnectLocalConnectionForState`/`PollLocalConnectionState`/`PumpLocalConnection`/송신 + **RawCampaignSessionLogin** 234줄(+182, 4-C 이월 `Impl::Login`, 현재 389줄)
  - 선행 재반영: cpppj1320의 기존 내부 전송3모듈 flush/직접 수신을 연결했다. 이전 Source 종료 후 번호 재사용·실제 buffered 패킷 전달 등 신규6건/관련59건 통과([E620](docs/webpj-mission-transition-20261117.md)). 외곽 Session/실제 Login은 잔여다.
- [ ] **RawCampaignSession 3: 프레임·카메라·표시 입력** — `RunFrame`/`PrepareFrame`·카메라 홈/중심/명령(`CenterCameraOn`·`ViewHome`·`ViewYourPriest`·`RequestCameraCenter`/`CommitRequestedCamera`)·입자 뷰(5-A 연결)
- [ ] **RawCampaignSession 4: 승패·종료·Editor 제외 정리** — `CheckMissionOutcome`·종료 수명(웹 제외인 Existing Editor 계열은 만들지 않는다)
- [ ] **GameWorld** 1,153줄(+5) — E300 판정에 따른 호스트 구역(4-D/5-A/5-B/6 배분)에서 4-D 몫

**E. Client 본체·기본 등록·통합**

- [ ] client **ClientMain** (partial adapt 1,797줄, +377) — 단독 도달 메인 루프 단계(연결 감시·전투 자원 프레임·재접속 저장, 3.6절 ③ `44e53bdc`)와 `Client` 전역 조립(위 A 소유자들 생성). 웹 적응(창/OS/DirectDraw 제외) 분리
  - 선행 `o/` 부품 `RawConnectionHealth`·`RawConnectionEnded`([E559](docs/webpj-connection-tell-packets-20261115.md))·`RawBattleResourceFrame`([E560](docs/webpj-battle-resource-frame-20261115.md))은 이식했다. 실제 Client 연결 표·Player 송신·스펠 보충/SP 송신/간헐천 장치 결선이 남았다
- [ ] client **ClientCampaignTransport 2단계** (4-C 이월) — `InstallClientPacketDefaults`의 고정 Native 수신기 묶음·채팅 41h·연결 종료 2번·기술 목록 0x69·경로 자료/압축 Squid 관찰기(3.6절 ③ 4-D)
  - 선행: 연결 종료 `RawConnectionEnded`(E559)·기술 목록 `RawTechnologyTypesPacket`(E560)은 이식했다. 재기준선 단위 NativeLoginPacketReceivers·NativeGameplayPacketReceivers·NativeCarrierPacketReceivers·NativeConstructionPacketReceivers·ClientChatPacketReceiver·RawSquidCompressedReceive(0.3절)가 남았다
- [ ] **4-C 통합 시험** (4-C 이월) — cpppj `CampaignSession_LocalLoginActualHandshakeAndWorldLoad` 대응: Zone→접속→0x27→응답→월드 설치→상태 8·9·10·13·14, 패킷 종류가 0x27 로 시작해 0x28·0x29 를 포함하고 0x6e 로 끝남, 옵션 22값·반경² 900·TYPE122 비용 3000
- [ ] **4-D L2 판정** — Tutorial1 Load/Start 의 `session-dump`(웹 덤프 형식·`tools/cpp-oracle.mjs` 의 `--ui-script`/`--fixed-frame-ms` 시나리오, 3.5절 1번, 현행 기준선 오라클은 이 기능을 포함하며 웹 덤프·시나리오 연결은 미구현)·첫 프레임 논리 화면. 7차 오류/예외 console 로깅 구조(3.7절)를 이 Session 조립 때 앞당길 수 있다

**끝난 항목이 남긴 재대조** (D·E 설치 뒤, 보관본의 각 항목 끝 메모 요약):

- C 구획 바인딩(E518~E573)은 덕타입 Session 계약으로 시험했다. 실제 Session/Client 부모와 Native 장기 Storage 경로를 설치한 뒤 같은 시험을 실제 조립으로 다시 대조한다(CampaignTestPlay 제외)
- CampaignGuideRelease(E524): cpppj 시험 3건의 실제 Client+GameWorld+UserInput 조립 재대조. Mission·MissionGuideInput의 Tutorial1 게이트는 순서 8에서 연결
- ClientGameplayGlobals(E570): 전체 Session/Client 부모·외곽 owner/자식 증빙 결선 재대조
- RawCampaignArcherBindings(E567): 저장 미션의 전체 Kernel/첫 비행 시험은 설치 뒤 7단계. RawCampaignMotionBindings·RawCampaignPathBindings(E568): 저장 미션 전체 초기화·L3는 6단계
- RawCampaignCursor(E569): 실제 Session/GameWorld/UserInput 부모·에너지/MayPlace/Builder·마나/사정거리 결선과 저장 미션 시험은 6단계 L3에서
- SoundProcess(E571): SoundPlayer/실제 브라우저 장치는 8단계. GameAux 공용 입력(E572)·TypeNames/변환표(E573)·RawBombBlastBindings(E583)의 실제 Session 연결은 D1에서

**4-D 남은 단위** (현재 cpppj 줄 수, `*`는 partial):

- Session·Client 허브: RawCampaignSession* 7,118 · ClientMain* 2,133 · GameWorld 1,157 · RawCampaignSessionLogin 389
- Client 전역·서비스: ClientStateWorldGlobals 112 · ClientStateWorldCleanup 102 · ClientStateRuntimeRetirement 99 · ClientStateStorageCleanup 71 · ClientUserSelectionCleanup 43 · CampaignClientServices 373 · CampaignUserInputServices 36 · CampaignCursorClientEffects* 74 · ClientDiagnostics 157 · ClientStaticStrings 78
- 전송·공통: ClientCampaignTransport* 378 · SquidPostPop* 709 · ConfigInterface* 356
- 합계: 기존 17단위 / 13,385줄 + 재기준선 64단위 / 4,747줄(0.3절 목록). 체크 줄의 줄 수는 기준선 값이고 여기 줄 수는 현재 cpppj 값이다

#### 5-A 렌더러·Draw·시각 시스템 연결 (44/46)

**완료 기준(3.2절):** 렌더러·Draw·팔레트·입자/표시 client 단위와 `o/` 4단위가 cpppj와 같은 순서·결과이고, 전용 시험 없이 웹 조립 검사만 한 E197·E198·E201 시스템을 cpppj Session 시험으로 재대조한다. 4-D의 막힘 항목이 모두 선행 대기라(E348 판정) E372에서 병행해 열었다. 끝난 44항목은 [2026-11-15 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495) 3.1절, 요약은 2.4절이다. 이미 있는 웹 메뉴 화면(M1 8장면)은 별도 웹 구현이며 이 목록은 cpppj 클래스 대응 기준이다.

- [ ] client **ClientRendererFrame** 396줄(+ClientRendererFrameGlobals.h) — 렌더러 프레임 본체. KenCloud 메뉴 연결을 포함하며 4절 "메인 메뉴 구름 배경" 차이를 0으로 만든다
  - 선행 완료(렌더러·커서·공장 표시): TileGroupCollection·GumpCollection·ClientKencloudGlobals(E394~E396), Renderer 494줄([E391](docs/webpj-renderer-20261113.md), 원본 x86 Order 261행·Dirty 133행 일치)·ScreenPresentation·Screen(E393), RawCampaignCursorDisplay([E407](docs/webpj-campaign-cursor-display-20261114.md), GumpDraw·GumpTextDraw E405·E406 포함), RawCampaignFactoryDisplay·ObjectHitVtables 83표([E409](docs/webpj-campaign-factory-display-20261114.md), RawObjectHit E408 포함), ClientGameplayGlobals(E570)
  - 선행 완료(`RawMenuControls`가 쓰는 Gump 계열, 전용 PE fixture가 없는 것은 기준선 소스 계약·독립 계산·실제 장치 연결로 대조):
    - Gump 기반: GumpCreate([E410](docs/webpj-gump-create-20261114.md))·GumpTree([E411](docs/webpj-gump-tree-20261114.md))·GumpLifetime([E412](docs/webpj-gump-lifetime-20261114.md))·GumpEvents·GumpInput·GumpFrame([E413](docs/webpj-gump-events-input-frame-20261114.md))·GumpRedraw·GumpText·GumpHelp([E414](docs/webpj-gump-redraw-text-help-20261114.md))·GumpBackground([E415](docs/webpj-gump-background-20261114.md))·GumpDecoration([E419](docs/webpj-gump-decoration-20261114.md))
    - 버튼·글자: ButtonGump([E416](docs/webpj-button-gump-20261114.md))·StoneButtonGump([E417](docs/webpj-stone-button-gump-20261114.md))·StyleText([E418](docs/webpj-style-text-fragments-20261114.md))·Markup([E420](docs/webpj-markup-picture-20261114.md))·StyleTextMarkup([E421](docs/webpj-style-text-markup-20261114.md), NB02 3.6② 반영)·StyleTextBuffer([E422](docs/webpj-style-text-buffer-20261114.md))·TextGump([E423](docs/webpj-text-gump-20261114.md))
    - 스크롤: ScrollingTextGumpMarkup([E424](docs/webpj-scrolling-text-gump-markup-20261114.md))·ScrollGump([E425](docs/webpj-scroll-gump-20261114.md))·ScrollGumpWidgets([E426](docs/webpj-scroll-gump-widgets-20261114.md))·ScrollingTextGumpWidgets·StyleTextBufferStore([E427](docs/webpj-scrolling-text-gump-widgets-20261114.md), 기준선 뒤 주석 변경 3.6①)
    - 메뉴: MenuGump·MenuGumpLayout([E428](docs/webpj-menu-gump-layout-20261114.md))·MenuGumpControls([E429](docs/webpj-menu-gump-controls-20261114.md))·MenuGumpDraw([E430](docs/webpj-menu-gump-draw-20261114.md))·MenuGumpInput([E431](docs/webpj-menu-gump-input-20261114.md))·MenuGumpWidgets([E432](docs/webpj-menu-gump-widgets-20261114.md), 기준선 뒤 `4ee69f4f` ② 반영)
    - 기타: KnowledgeSort·MinimapGeometry·InventoryAboutMenu([E448](docs/webpj-knowledge-sort-minimap-inventory-about-20261114.md), 소비자 연결은 후속)
  - **남은 선행:** **RawMenuControls** 본체 1,605줄(5-C 메뉴 허브 항목)·**GameWorld**(4-D D). 따라서 이 항목은 4-D D와 5-C 메뉴 허브 뒤에 끝난다. 판정 근거는 [화면 문서](docs/webpj-screen-20261113.md) 5절
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

- [ ] client **Uber 허브** 3단위 3,031줄 — 게임 화면 최상위 Gump와 루트 컨트롤(HUD·덱·미니맵·채팅 창의 실제 소유자). 3.6절 ③의 `UberGump` 행을 함께 판정: UberGump* 2,683 · UberRootGump 187 · RawUberRootControls 161
- [ ] client **메뉴·버튼 컨트롤 허브** 6단위 2,650줄 — 모달·등록 소유자·링크/명령 소비자, 게임 버튼 묶음, 문자 입력·채팅 메뉴: RawMenuControls 1,605 · RawButtonControls 811 · RawMenuHtml 65 · RawMenuLoading 11 · RawTextEntryControls 110 · RawMenuChat 48. 5-A `ClientRendererFrame`의 선행이다
- [ ] client **메인 메뉴·타이틀 상태** 4단위 317줄 — 이미 동작하는 웹 메뉴를 원본 상태 소유자에 맞춘다
  - 완료: NativeIntroState 64([E532](docs/webpj-native-intro-state-20261115.md), 원본 17행 일치)·TitleBitmapLifetime 78([E533](docs/webpj-title-bitmap-lifetime-20261115.md), 원본 33행 일치)·NativeMainMenuState 90([E534](docs/webpj-native-main-menu-state-20261115.md), 원본 82관측 일치)
  - 남음: **ClientTitleBitmaps** 85 — 끝나면 이 줄을 체크한다
- [ ] **미션 진입 명령 연결** — `MissionBegin`·`StartDemo`·환영 창 명령을 실제 Session 생성에 연결(메뉴 → Fort 요청 → State 전환). `?map=` 개발 화면과 구분한다
- [ ] **3.8절 허브 선행 fixture** — `chat-callbacks`(30행)·`chat-input-recipients`(721행)·`html-input-dialog` 생성/공장 20행을 허브 조립과 함께 연결
- [ ] **M2 판정** — Tutorial1 진입 직후·N프레임 뒤 L4a(영어 4:3 → 16:9/16:10 → 한국어), 오라클 시나리오 추가

재기준선 메타 Sprite 갱신 2단위(ClientMetaSpriteRefresh·NativeMetaSpriteRefresh)는 4-D 몫으로 셌지만 화면 허브와 함께 확인한다(0.3절).

#### 6 입력·게임 메뉴·사제/운반·경제·건설 연결 → M3 (1/8)

완료 기준(3.2절): 같은 입력 단계표에서 cpppj와 같은 상태·화면(L3). **M3:** Tutorial1·2와 캠페인 1-1·1-2를 사용자가 브라우저에서 검증할 수 있다. `o/` 규칙 부품(경로 6-P·커서 선행 포함)은 이미 옮겼으므로 이 단계는 client 바인딩·입력·메뉴와 세션 결선이다. 3.6절 ③ 표의 6단계 행(`GameWorld::SelectPriest` NB20·건설 실제 wire·`UberGump` 건설 기록)을 함께 반영한다. 끝난 1항목(커서: RawCampaignCursor E569, RawCursorActions·RawCursorPosition·RawCursorMana·RawCursorRange E547~E550)은 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584) 3.1절이다.

- [ ] client **입력 본체** 2단위 163줄 — 40칸 사건 큐의 게임 입력 분배: UserInput 122 · GameWorldInput 41
- [ ] client **게임 메뉴** — ESC 가로/하위 메뉴·명령·Player 메뉴. 남음: **RawGameMenus*** 273(partial)
  - 완료: RawPlayerMenu([E544](docs/webpj-raw-player-menu-20261115.md), 원본 292입력)·GameMenuActions([E535](docs/webpj-game-menu-actions-20261115.md), 원본 610+144+384행)·GameMenuCommands([E538](docs/webpj-game-menu-commands-20261115.md), 원본 PE 이름표 43개·이름→큐 21행)·RawGameMenuBar([E539](docs/webpj-raw-game-menu-bar-20261115.md), 원본 510행)
- [ ] client **객체 우클릭 메뉴** — 건물·운반체·특수 대상 메뉴와 소유자 명령. 남음: **CampaignObjectMenuClient** 229
  - 완료: RawObjectMenuInfo([E546](docs/webpj-raw-object-menu-info-20261115.md), 원본 1,434입력)·RawObjectMenu([E541](docs/webpj-raw-object-menu-20261115.md), 717입력)·RawObjectMenuVariants([E542](docs/webpj-raw-object-menu-variants-20261115.md), 1,024행)·RawObjectMenuOpen([E543](docs/webpj-raw-object-menu-open-20261115.md), 788입력)·ObjectOwnerCommands([E531](docs/webpj-object-owner-commands-20261115.md), 256+256행)
- [ ] client **사제·운반·경로 바인딩** — 운반 입력/투하·기도·공황. 남음: RawCampaignCarrierInput 42 · RawCampaignCarrierDrop 53 · RawCampaignPrayer 24 · RawCampaignPanic 12
  - 완료: RawPriestMenu([E540](docs/webpj-raw-priest-menu-20261115.md), 원본 905행)·RawPriestWorldBindings([E563](docs/webpj-priest-world-bindings-20261116.md), 4-D 선행)·PriestPlacementAssets([E562](docs/webpj-campaign-construction-bindings-20261116.md))·RawCampaignPathBindings([E568](docs/webpj-campaign-motion-path-20261116.md), 4-D 선행)
  - E568 후속: D/E Session 설치 뒤 cpppj CampaignMotion의 thewarbegins·masterofwhirligigs 저장 사제/목적지 선택·전체 초기화와 같은 경로 루프를 재대조한다
- [ ] client **건설·생산·경제 바인딩** — 배치·건설·생산 덱 재계산·생산 유지·다리 붕괴. 남음: RawProductionMaintenance 173 · ClientProductionSync 46 · ClientBridgeDecay 42 · ProductionControlContext 18
  - 완료(4-D C 선행): RawCampaignProductionBindings([E522](docs/webpj-campaign-production-bindings-20261115.md), E215 이월 시험 3건)·RawCampaignConstructionBindings([E562](docs/webpj-campaign-construction-bindings-20261116.md))
  - 재기준선 건설 장치 `ClientConstruction*` 6단위 중 4-D에서 옮기지 않은 입력·건설 몫도 여기서 연결한다(0.3절)
- [ ] client **공장·지식 메뉴와 기술** — 연구·업그레이드·지식 메뉴, 테마 기술 명령. 남음: ThemeTechnologyCommand* 48(partial)
  - 완료: RawFactoryMenu([E545](docs/webpj-raw-factory-menu-20261115.md), 원본 76행)·RawKnowledgeMenu([E537](docs/webpj-raw-knowledge-menu-20261115.md), 원본 25+50관측)
- [ ] **L3 판정** — Tutorial1 입력 단계표(이동·수확·건설·생산)의 상태·화면이 cpppj와 같다. cpppj `--test-autoplay`(기준선 뒤 기능, 4-R 선행)로 만든 고정 시드 구간도 쓸 수 있다

**6 남은 단위:** 기존 13단위 / 1,123줄 + 재기준선 RawBuildReplay·ClientBuildReplay 2단위 / 199줄(둘 다partial: [E601](docs/webpj-build-replay-runtime-20261116.md)의 실행/라벨·현재 수명은 완료, FILE 판독/최신 rec·Host·키/선택은 잔여. 3.6절 ③ 6단계의 `UberGump` F7/Ctrl+F6 입력과 함께).

#### 7 전투·의식·AI 연결 (0/6)

완료 기준(3.2절): 현재 Player/기술/SP/명령/난수로 연결하고 cpppj 복원 범위를 타입별로 기록한다. `o/` 부품(총·투사체 7-P2, 비행 7-P1, 신전·제단·의식 7-P3, AI 34단위)은 이미 옮겼다. 임시 방어 AI를 만들지 않는다. 포획/희생·특정 적 공격·승리 재현과 공중 3종 전용 검사는 1절 경계를 지킨다. 11차 난이도는 이 단계 뒤에 붙일 수 있다(3.7절).

- [ ] client **전투 바인딩** — 피격·궁수·이동·총 조회·폭발·Damageable 정기 표(3.6절 ③ `SquidProcess` 발사/회전 TYPE 두 칸 포함). 표 이식 완료: CampaignDamageableRegularTables7줄([E590](docs/webpj-session-assets-20261116.md), 4-D 선행). **D/E 전체 Session 설치와 아래 저장 미션/전체 사격 루프 재대조가 남아 있어** 체크는 유지한다
  - 완료(4-D 선행): RawCampaignHitBindings([E564](docs/webpj-campaign-hit-bindings-20261116.md))·RawCampaignGunQueries([E565](docs/webpj-campaign-gun-queries-20261116.md))·RawCampaignArcherBindings([E567](docs/webpj-campaign-archer-bindings-20261116.md))·RawCampaignMotionBindings([E568](docs/webpj-campaign-motion-path-20261116.md))·RawBombBlastBindings 45줄([E583](docs/webpj-bomb-blast-bindings-20261116.md), 파편 캡처 선행)
  - E567 후속: D/E Session 설치 뒤 cpppj CampaignFire_ActualInitialPositions·ActualWarBeginsLaunchAndFlight와 저장 사제를 쓰는 세 factory/clock loop 전체 조립을 재대조한다(현재는 공장 부분만 검사)
- [ ] client **파괴·주문 효과** 3단위 209줄 — Devastation·Meltdown·기폭 입력: RawCampaignDevastation 163(partial, E584 파편 캡처 자격 16VT·E585 빈 타이머/타이머용 펜스 Rebuild 장치만) · RawCampaignMeltdown 20 · DetonateInput* 26(3.8절 `detonate-input` 769행은 RawObjectConversion 단위에서)
- [ ] client **의식** 2단위 204줄 — 의식 진행·선택 입력: RawCampaignRitual 161 · RitualChoiceInput* 43
- [ ] client **AI 잔여·전투 이탈** — AI getter의 실제 자산 연결: AiTypeRequiredEnergy* 28. 완료: RawBattleLeave([E557](docs/webpj-raw-battle-leave-20261115.md), 원본 64행 일치)
- [ ] **AI 세션 결선** — `o/RawAi*` 34단위를 실제 Session의 Player·SP·명령·난수에 연결하고 AI가 있는 미션의 N프레임 L3 비교
- [ ] **E197·E198·E201 등 전용 시험 없던 시스템**의 cpppj Session 시험 재대조(5-A에서 이월된 항목과 같은 것이면 한 번만 한다)

재기준선 장기 월드 저장소 중 전투·의식 몫(RawSurfaceBomb·RawSurfaceDamageable·RawSurfaceFence·RawSurfaceFlare·RawSurfaceIce·RawSurfaceBalloon·RawSurfaceRitual·RawSurfaceVortex·RawSurfaceDaisDeletion)은 4-D에서 Tutorial1 필요분을 옮긴 나머지를 이 단계에서 연결한다.

#### 8 미션 수명·안내·대본·소리·채팅·도움말 잔여 → M4 (0/9)

완료 기준(3.2절): **M4** — 일반 31 + 데모 3경로의 진입·진행·저장 수명. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분한다. 소리·음악·동영상은 Web Audio/브라우저 디코더로 바꾸는 adapt 단위라 적응 판정(3.6절 ①②)을 먼저 한다. 6차 한국어 미션 문구(3.5절 2번)와 7차 console 로깅 구조(3.7절)를 이 단계까지 끝낸다.

- [ ] client **대본·대화창** 2단위 484줄 — 튜토리얼 대본 실행과 대화창 본문 배치: DialogScript 296, DialogBodyLayout 188
- [ ] client **소리 본체** 3단위 1,663줄 — 효과음·위치 음향·장치 계층(Web Audio 적응): Sound* 824(SoundList 공개 API는 E571 완료) · SoundDevice 372 · ClientAudio* 467. 완료: SoundProcess 361([E571](docs/webpj-sound-process-20261116.md), 4-D 선행)
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
| **4-D** Session 조립·Tutorial1 Load/Start | RawCampaignSession·ClientMain*·client 전역·GameWorld를 cpppj 부모 호출 순서로 조립(`RawSurfaceWorld`/`RawWorldCore`, Player/동맹/기술/SP/Deck/AI 초기화, Start/Guide·State 전환). **4-C 이월:** `ClientCampaignTransport` 2단계·`RawCampaignSessionLogin`(Session `Impl::Login`)·4-C 통합 시험. **4-B 이월(E304):** Tutorial1 저장 객체(`FortObjectLoad`/Money/Meta child)의 실제 적재와 session-dump 대조. **앞 단위가 미룬 연결:** 보상의 삭제 장부(`reward_lifecycle_*`, D5b)·World 장기 호스트 대여와 Client 접합 장면(`ProcessHostStorage`·Kernel procSumm, D5c)·3.6절 ③의 4-D 항목·client UI 4단위 판정. 웹 Session 덤프와 오라클 시나리오 형식(3.5절 1번). 재기준선 추가 65단위(0.3절) | 4-B·4-C·4-R | **L2:** Tutorial1 Load/Start 관측(보고서·session-dump·첫 프레임 논리 화면)이 cpppj 와 같음. 임시 Player/AI/색 번호로 시작 성공을 만들지 않음 |
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

### 3.4 cpppj 현황과 webpj의 관계 (2026-11-16, cpppj 제품 1313 기준)

cpppj는 AGENTS 1~14차 구현과 M8 최종 배포 관문(1311)을 마치고 제품 1313에서 사용자 요청으로 일시 중지 상태다([공통 인계 0절](LEFT_JOBS.md), 마지막 cpppj 소스 변경 `979d65b5d`). 웹은 E513에서 제품 1313으로 재기준선 전환했으며, 웹 후속 기능으로 판정한 ③은 실제 구현이 남았다. 필요한 cpppj 소스 수정은 2026-11-16 사용자 지시로 허용된다(1절 보호/분담).

| cpppj 범위 (제품 번호) | webpj 처리 |
| --- | --- |
| 1차 비멀티 UI(929)·Edit(943)·미션 수명(956)·내부 수명/FILE(980)·2차 호환(981) | 웹 이식의 기준 범위(기준선 판). 완료한 웹 키/reader/UI는 다시 세지 않는다. Edit 관련은 제외 |
| 3차 LAN·Outpost(~1279): 공유 파일에 `Actual*`/`shared*`/Native 연결이 쌓임 | LAN은 웹 제외. 공유 파일 변경의 단독 경로 영향은 3.6절로 확인. **Outpost는 12차 스커미시와 함께 이식**(1절, 3.7절) |
| 4~13차(4차 MCP·와이드 935~937, 5차 홈 저장 932~934, 6차 한국어 K1~K5 1270~1281, 7차 로그 1269, 8차 AA/FSR 939, 9차 시나리오 S1~S4 1277~1284, 10차 버그 34/34 판정·수정 10건 1285~1300, 11차 난이도 1292, 12차 스커미시 1293·1300, 13차 일본어 UI 1288) | 차수별 웹 처리는 3.7절. 6차 미션 문구는 3.5절 2번 |
| 11.01 카피라이트(1291) | **웹 반영 완료**(WB03-AE, 0.6절) |
| Frame Rate 60/75/120(1297·1298)·CD 10.72 문서(1299) | Frame Rate는 순서 9의 60/120 때 참고. 웹 목표는 10.78이라 10.72는 범위 밖 |
| 사용자 확인 경계(전체 동일성·자연 완주 등)·공중 3종 정적 작업만 | 웹도 같은 경계. 아직 없는 효과를 일반 성공으로 채우지 않고, 공중 3종 전용 생산·행동 검사를 추가하지 않음 |
| 14차 QuickJS(1301)·CD판 10.72 복원 보류(1302~1310)·M8 최종 관문(1311)·테스트 자동 플레이 `--test-autoplay`(1312~1313) | QuickJS는 웹 불필요, 10.72는 범위 밖. `--test-autoplay`는 현행 기준선 오라클에 들어 있어 6·7단계 L3 비교의 고정 시드 구간에 쓸 수 있다 |

### 3.5 사용자 확인·요청이 필요한 것

1. **실제 Session 상태 비교 수단:** cpppj의 `--ui-script` `session-dump` 단계(SID 풀 해시·사용 슬롯 원문·자산 해석·Kernel/Regular·Player 9·동맹 81·SP 비트·Graph·TotalMade·난수, 판번호 1 TSV)와 `--fixed-frame-ms N`(가상 시계)은 현행 기준선 오라클 실행 파일에 들어 있다([덤프·고정 시각 문서](docs/exe/cpp-session-state-dump-20261109.md), E497·E513). 웹 쪽 남은 일(4-D): `tools/cpp-oracle.mjs`에 두 옵션을 넘기는 시나리오 형식과 같은 형식을 만드는 웹 덤프(재기준선 단위 SessionStateDump). 그 전까지는 `report`와 논리 화면 범위로 비교한다.
2. **한국어 미션 문구의 단일 원본:** cpppj는 6차 K1~K5(제품 1270~1281)로 공식 미션 `.korean` 본문(`assets/lang/korean/d/` 43개 등)을 제품에 반영했다. 웹이 같은 파일을 `webpj/translations`로 가져와 쓸지(BOM·줄바꿈은 준비 도구에서 정규화), 웹 번역을 따로 둘지 정해야 한다. 기본값은 "cpppj 확정본과 통일"이며 적용 시점은 순서 8(현재 미션 Tell·한국어 미션)이다. 현재 웹 `webpj/translations`에는 메뉴·도움말용 12개만 있다.
3. **오라클 수집의 실행 허용:** 예외 호스트가 아닌 PC에서 cpppj 창 실행이 필요하면 단위마다 확인을 받는다.
4. **바깥 링크의 실제 브라우저 확인:** WB03-AD의 새 탭/창 열림과 팝업 차단 여부는 실제 브라우저에서 확인하지 않았다(검사는 판정·앵커 생성까지).
5. **후순위 4~14차 착수 전에 정할 것 (3.7절, AGENTS.md `85551c6c`):** ① **12차 스커미시** — 결정·분류 완료: MP 시작 규칙 월드 생성 단위는 1절 예외로 이식 대상이 됐고(E500·E512 분류, 순서 10), Outpost는 2026-11-11 사용자 결정으로 함께 구현하며 금지 유닛 목록에도 cpppj처럼 Outpost를 둔다 ② **4차 와이드**의 웹 정책(AI 권고: 옵션 메뉴 해상도 항목 없이 창 크기·`?aspect=` 기준으로 게임 영역 16:9 논리 화면을 최대 1920×1080까지, 그래픽 배율은 그대로) ③ **4차 WebMCP** 도구 범위(조작·화면 캡처·소리 포함 녹화)와 대상 브라우저(착수 시점의 WebMCP 사양·지원 상태 확인).
6. **재기준선(4-R) — 결정·실행 완료:** 2026-11-15 사용자 결정에 따라 기준선을 제품 985(`1f00a25a`)에서 제품 1313(`225553df`)으로 올렸고 E513에서 전환을 마쳤다. 기존 행별 판정 이력은 보존했다(3.6절).

### 3.6 cpppj 변경 재반영

- **현행 기준선:** `225553df`(제품1313, E513). 기존 행별 판정과 아래 표의 미구현 ③ 항목은 계속 유효하다. 분류는 ① 웹 결과 영향 없음(구조 정리·진단·웹 제외 기능) ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫이다. cpppj fixture가 바뀌었으면 해당 웹 검사를 먼저 다시 돌린다.
- **기준선 이후의 변경:** 매 단위 착수 때 `node webpj/tools/port-map.mjs`의 `cpppj 변경` 줄을 본다(행 base 뒤 커밋을 단위별로 보고하고 안내를 붙인다). `exclude-*` 행은 ①로 기록만 하고, `port`/`adapt` 행은 ②/③을 판정해 ②는 `재반영`을 명시한 WB 단위로 반영한다. 판정을 마치면 그 행 base를 판정한 커밋으로 올리고 근거를 메모(①이라도)와 단위 문서에 남긴다. `fixture-manifest.mjs`도 같은 base 뒤 변경만 보고한다. 기준선 뒤 새로 생긴 파일은 `새 파일`로 보고되며 재기준선 때 분류한다.
- **현재 보고 (2026-11-16, `port-map.mjs`):** `cpppj 변경` **1건** — E616에서 cpppj1314의 ClientAudio·ClientMain·UberGump 3행을② 재반영/① 구조 정리로 판정해 base를 `bd2941f4`로 올렸다. 남은 것은 RawCampaignSessionLogin(행 base `a328ac1f` 이후 32커밋, 이식 때 판정). 이미 옮긴 단위 중 바뀐 37단위는 재기준선 때 E503~E511에서 ①/②/③으로 판정해 ②는 재반영하고 행 base를 올렸다. 그래서 2026-11-11(155건)·2026-11-15(92단위) 판정 대기 목록과 커밋 성격별 잠정표는 만료됐다(보관본). 남은 ③은 아래 목록이다.
- **재기준선 결과 (4-R, E497~E514):** 기준선 뒤 `cpppj/src`의 `o/`·`client/`·`zacket/`이 301커밋·768파일(+26,257/−3,270줄) 바뀌었고 남은 허브가 가장 많이 바뀌어(RawCampaignSession 5,715→7,118줄, ClientMain 1,797→2,133줄, UberGump 2,207→2,683줄) HEAD 판을 바로 옮기기로 했다. 오라클 재빌드·M1 재대조(E497), 신규 176단위 분류(E498~E502·E512), 변경 37단위 판정(E503~E511), 대응표·manifest 전환(E513), 기존 웹 부품 상태 판정(E514)을 마쳤다. 기존 fixture의 실제 `cpppjBasis`와 행별 `base` 이력은 보존했다. 10차 버그 수정·6/13차 표시 부호화처럼 순서 10 몫인 변경은 ③으로 남겼다.
- **판정을 마치고 반영 시점만 남은 ③ (아래 표):** 해당 단계에서 반영하면 이 표에서 지우고 대응표 메모를 고친다. Native/실제 연결과 아직 미구현인 후순위 기능을 포함한다. 행별 상세(기준 커밋·바뀐 함수·판정 이유)는 각 행 `PORT_MAP.tsv` 메모, 판정 문서([공통층 E259](docs/webpj-judge-common-20261110.md)·[Player·사제 E260](docs/webpj-judge-player-priest-20261110.md)·[섬·자원 E261](docs/webpj-judge-island-resource-20261110.md)·[건설·Dude E262](docs/webpj-judge-construction-dude-20261110.md)·[전투 E263](docs/webpj-judge-combat-20261110.md)·[기타 E264](docs/webpj-judge-misc-20261110.md)), [대응표 문서 5·9절](docs/webpj-port-map-20261106.md)과 [2026-11-11 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-through-wb04-e277) 3.6절 원래 표에 있다. 판정을 마친 ①·②는 PORT_MAP base·메모에 있다.

**4-D — 4-C 마감 이월**

- `RawSurfaceWorld` 일반 Pop/삭제·대상·ProcessForm의 실제 Storage 송신 연결은 **E594 완료**. 임시 함수 대체를 제거했다. Native mutable Client/기본 Form writer와 전체 Session Runtime 공개는 잔여다([단위 문서](docs/webpj-surface-transmission-20261116.md))
- `RawSquidTransmission` 압축 수신 복호기 `RawSquidCompressedReceive`·월드 동기화 `RawPeerWorldSync`(E513 대응표 등록, 이식은 4-D 후속)
- `GameClock` 시간 동기화 보정(`3e9dc2c7`, 보정 0이면 같음)
- 아비어리 `ReadProcess`·`WritePacket`(E264), 공유 Regular **수신** 단독 도달 재확인(E259). `WriteFormPacket`은 **E591에서 단독 도달 확인·공용 Host 부모 이식 완료**이며 상위 Storage/World 송신 결선은 남았다
- `UberGump` 로그인 안내("Creating Islands."·`ShowLoginProgress`, `9cb0547f`~`11fbb988`) 단독 도달 확인
- 완료 기록: `RawFormPacket` 삭제 패킷 writer 헤더 선기록·진단 복귀(`b8cadde1`)는 E312에서 ② 반영 완료

**4-D — 고정 Native 수신기(canonical) 묶음** (한 묶음으로 처리)

- `PacketRegistrationTable` `PersistCanonicalBaseline`·`canonicalReceive`(장기 고정 수신기 승격, `899c934dc`, E305)·`RawPacketDispatch` `CanonicalPacketReceiver`/`RegisterCanonical*`/`SnapshotReceiver`(`ec0b6e973`, E306)
- `RawConnectionActivity` 등록의 Native 고정 수신기(nativeEntry 0x004b6180) 대여 분기(`eeae45c69`, E309)
- `RawMissionLogin`의 Native Login 고정 수신기를 분배기에 대여하는 분기(E319)
- `CampaignTransportServices` `BindCompositeCampaignTransport`의 고정 Native 수신기 분기(종료 패킷 2/기술 0x69 의 `canonicalReceive`에 대한 `RegisterCanonical` lease 표식, `a76499e35`; 일반 수신기 재등록은 E327 완료)
- `ClientCampaignTransport` `InstallClientPacketDefaults`의 고정 Native 수신기 묶음(`RegisterCanonicalDirect`로 등록하는 C3 빈 수신기 004437f0와 Login/Gameplay/Carrier/Construction 수신기 — 웹은 일반 직접 수신기로 등록, canonical 승격은 이 묶음과 같이, E328)

**4-D — 접속 통지·패킷 출처·섬 조회**

- `RawConnectionRegistry` 접속 통지 `StoreConnectPacket`/`SendConnectPacket`/`RegisterConnectPacketReceiver`(LocalMissionTransport 생성자의 type 1 수신기 등록 포함, E324; `621efab7e`·`efc78ee56`): 실제 TCP 접속의 초기 패킷 복제·LAN 전용이며 대응표 밖 신규 파일 `RawPacketCopy`와 고정 Native 수신기에 기댄다. 그 전까지 `buffers` 칸은 비어 있어야 한다(E308)
- `ClientConnectionGlobals` `PacketSources().Resolve(connection)`(Squid 대상 송신·전투 배송의 실제 Source 선택, `a76499e35`)·`PacketDelays()`(실소켓 지연 FIFO·`lag` 선택·`ResetDefault`, `ec0b6e973`) — `ClientMain`/`ClientCampaignTransport`/Meta의 소비 위치와 함께 단독 도달 여부 판정(E325)
- 4-B 이월: `RawIslandRuntime` NoWorld 자유 함수 3개(`RawIslandExists`/`Owner`/`Bounds`, 섬 저장소를 만들 때, E261)

**4-D — World 수명·장기 owner 묶음**

- `RawSurfaceWorld` 기준선 뒤 +780줄(`14fd017c`): 월드가 Display/PostPop/Pop/Unpop/Factory/Frame/Owner/Neighbors/Connect/Surface/BaseDestroy/VirtualDestroy/Common/Lifecycle/Events/Dependencies를 Storage 장기 소유자에서 빌림·Runtime pin·자산별 Bindings 15종·직렬화 전역 선택·`SynchronizePeer`/`TransmitDeletion`/`AttachBaseAsset`/`BindMetaRefresh`. E299 변형 확인이 관찰하지 못한 건물 공간·Form 층·감시 Form 생성/Unpop·진단/현재 TYPE 빌림 연결은 Tutorial1 Session 검사에서 확인한다
- `RawSurfaceStorage` 기준선 뒤 +413줄 자산 장기 소유자 연결(`14fd017c`)·SidPool 소진 복구 BindAllocation 연결(진단/회수/덤프/Tell, E298)
- `RawProcessHostStorage` 장기 기본 장치 보관(`BindLifecycleDefaults`/`BindFormFactoryDefaults`/`BindFormTransmissionDefaults`)·fire/rotate 애니메이션 TYPE 참조·`ClearCall` owner pin(E297, `417f22d2`)
- `SquidDestroyLifecycle` Native PATCH 분기(E213)와 같은 시기의 `RawSquidDestroy`·`SquidPostPop`·`SquidReward`
- 공통 Form/Process 층 Native 분기(`SquidFormDispatch`·`SquidProcess`·`SquidFactory`·`SquidFrame`·`DependFormHost`·`GumpFormHost`·`ContentFormLifetime`, E259)
- Native 조회기·getter: Player·사제·신전(E260), 섬·다리·자원 전역(E261), 건설·운반·Dude 공유 getter(E262), 전투 역할 getter·폭탄/펜스 생성자(E263), 플레어·아이스 타워·아비어리 `FlyerDiedIn`(E264), `RawBombTargets` 현재 TYPE 사거리 직접 읽기(레코드 `range` 칸은 E265 완료)
- `TellSection` 대체 tell 적재(`config.LoadFile`, 현재 미션 Tell 연결 때)·`SidPool` 패치판 할당 진단 복귀·소진 시 다리 회수(`BindAllocation`)
- `ClientMain` 메인 루프 단계(연결 감시·전투 자원 프레임·재접속 저장, `44e53bdc`)의 단독 도달·순서 재확인

**4-D~ (E335 이월)**

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

1. **분류 변경 완료:** `o/RawOutpostLifecycle`을 E512에서 `skirmish-outpost` 규칙으로 `port`/`todo`로 옮겼다. 구현은 아래 2번부터 남았다.
2. **`RawOutpostLifecycle` 이식 — 완료([E556](docs/webpj-raw-outpost-lifecycle-20261115.md), 원본 768행 일치):** 원본 004557c0 `PostPop`(첫 Pop·일반 객체를 배치 추가 목록과 거리 후보 목록에 중복 없이 추가)과 00455840 `PreDestroy`(일반 객체의 모든 중복 제거 뒤 base 호출), 훅(지역 소유 갱신·작업장 PostPop·Damageable PreDestroy). 현행 기준선 `.cpp` 101줄이다(옛 기준선은 79줄). 두 목록은 Factory 목록과 다른 전역이므로, 이미 있는 `raw-asset-post-pop-state.js`의 `outposts`/`sharedOutposts`(Factory PostPop이 쓰는 목록)와 같은 저장소인지 cpppj 소유 관계로 먼저 확인한 뒤 연결한다.
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
- **2026-11-16, HJOW-Athlon, 사용자 요청(인계 문서 정리·체크 누락 확인·README 최신화, 문서만):** `bf69ee4ca / 20261116-23 WB` 시점 원문 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)에 바이트 그대로 보존했다(SHA-256 `0008365f5a90bc2c4340b5269a1c642e30aad13b7837f98cd1224d842fda46c2`). **뺀 것:** 3.1절의 끝난 체크 항목(4-D 16·6단계 1)과 끝난 4-B·4-C 구획, 5-A `ClientRendererFrame` 항목에 한 줄로 나열됐던 선행 완료 경과(하위 목록으로 재구성), 0.5절의 2026-11-15 중단 인계 두 문단, 3.6절의 만료된 판정 대기(2026-11-11 155건·2026-11-15 92단위)·커밋 성격별 잠정표·재기준선 절차, 3.2절의 끝난 단계 행, 7절 지난 정리 기록의 상세. **새로 넣은 것:** 0.1절 다음 작업 목록, 0.2절 재기준선 추가 79단위를 포함한 단계별 남은 단위와 합계, 0.3절 영역별 남은 양 표와 재기준선 79단위 배정 목록, 2.4절 E496~E584 요약, 3.1절 항목의 "완료·남음·막힘" 하위 목록과 4-D "끝난 항목이 남긴 재대조", 3.6절 ③ 표의 단계·주제별 목록 재구성. **바로잡은 것:** 0.2절 7단계(8→7단위, RawBombBlastBindings E583)·10단계(Outpost E556 완료로 기존 단위 0), 0.4절 웹 소스 줄 수·검사 파일 수, 3.4절 cpppj 현황(제품 1300 → 1313), 3.5절 1·5·6번, 4절 README 행. **체크 누락 확인:** 체크하지 않은 항목마다 대응 단위를 `PORT_MAP.tsv` 상태와 대조했으며, 대상 단위가 모두 끝났는데 체크되지 않은 항목은 없었다(여러 단위를 묶은 항목은 각 하위 목록에 끝난 단위를 표시했다). 코드·검사·게임 실행은 하지 않았고 `LEFT_JOBS.md`·`LEFT_JOBS.dotnetpj.md`·cpppj·AGENTS.md는 수정하지 않았다.
- **2026-11-16 사용자 지시:** 현재 cpppj 작업은 진행 중이지 않으므로 필요한 cpppj 소스 수정은 허용한다. 이미 완료된 기능이 망가지면 안 된다. 0.5절 착수 규칙과 1절 보호/분담에 반영했다.
- **2026-11-15, HJOW-Athlon, 사용자 요청(인계 문서 정리·cpppj 소스 기준 계획 재수립):** `fa8c2931f / 20261115-51 WB` 시점 원문을 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)에 보존하고, 끝난 체크 항목을 빼고 단계 진행판·남은 단위 배정·5-C~8 체크 목록·4-R 재기준선 제안·집계 도구 `tools/port_inventory.py`를 넣었다.
- **2026-11-11 이전:** 2026-11-11 인계 정리와 AGENTS.md `60226ea9`·`85551c6c` 반영, 스커미시의 Outpost 결정(1절·3.7절), 2026-11-10 Node v26 신호 NaN 대응·글꼴 라이선스 파일 이름 변경·단계 분할, 2026-11-09 첫 재구성. 각 기록의 원문은 [2026-11-16 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584) 7절과 그 이전 보관본에 있다.

**AGENTS.md 반영 기준(2026-11-16 확인, 마지막 AGENTS 변경 `c17aeede1`):** AGENTS.md는 수정하지 않는다. webpj에 해당하는 사용자 변경(`551d27ae` 진행 요약 기준·`75e4eed9` 글꼴·`8136df49`~`22a0877c` 후순위 범위·`60226ea9` 11~13차와 카피라이트·`85551c6c` 14차와 웹 후순위 4~14차·`c17aeede1` dotnetpj·webpj는 디컴파일 소스 분석 불필요)을 이 문서에 반영했다(0.4절·0.6절·3.7절).

**오래된 내용은 보관본에 날짜순으로 있다:** [2026-11-16 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261116-through-wb04-e584)·[2026-11-15 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261115-through-wb04-e495)·[4-C 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261112-4c-through-wb04-e328)·[4-B 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-4b-through-wb04-e304)·[2026-11-11 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261111-through-wb04-e277)·[순서3 완료 목록](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e264)·[2026-11-10 단계 분할 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)·[2026-11-09 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)·[2026-11-06 재계획 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261106-through-wb04-d5a)·[WB04-A까지의 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a). **그 문서의 “현재/다음/미구현”은 당시 기록이며, 이후 작업은 이 현행 문서를 기준으로 진행한다.**
