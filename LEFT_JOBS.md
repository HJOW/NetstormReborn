# LEFT_JOBS — cpppj 및 공통 인수인계

cpppj와 공통 사항의 **현재 기준, 완료 범위, 남은 작업, 유효한 사용자 결정**을 정리한다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다.

**2026-11-03 문서 정리:** 단위808까지의 원문과 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md#archive-20261103-through-808)에 보존했다. 완료 범위는2절, 재개 후 작업은3절을 따른다. 2026-11-04 새 `/goal`로840 뒤 일시 정지를 해제하고841부터 재개했다. **853 성장 아이스·썬 바리케이드·너겟 후처리를 완료했고 다음854이며 최신인계는0.24절이다.**0.4~0.10절의 일시 정지는 당시 이력이다. cpppj 전체 목표와 객체 메뉴/커서·Edit·수명·후속 단계는 미완료다.

- **2026-11-04 최신 사용자 요청:**직전중단작업과목표를재개한다. goal상태active를확인했다. 기존전체목표/우선순위·실제포획/희생·특정적공격·승리/사용자직접확인·신규MP검사제외·단위인계/커밋·push금지를유지한다.

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-11-04, 단위853 완료·전체 미완료)

| 항목 | 현행 기준 |
| --- | --- |
| 진행 상태 | **목표active·853완료 뒤 계속 진행.** 성장 아이스·썬 바리케이드·너겟의 선택 몸체/현재 물리·프레임 읽기 순서와 같은 실제 Session 예약을 연결했다. 다음854는 Vortex 등이며 최신0.24절이다. |
| 현재 제품 | **853 / 표시11.01.** 세 비공중 몸체의 현재 후처리/물리와 Session 예약을 연결했다. 다른 파생/전체Pop은 미완료이며 checking잔여12다. |
| 실행·플레이 가능 여부 | 무인자 GUI 시작과 미션 플레이 경로가 있다. 일반31+데모3의 마지막 전체 진입 검증은 **645의644 제품**이며 현재 제품의 전체 회귀 결과가 아니다. |
| 원본과의 동일성 | 부품별 원문 대조와 집중 검사를 누적했다. **전체 동일성·모든 시나리오 자연 완주는 미확인**이며 사용자 확인 대상이다 |
| EXE SHA-256 | `e275535ffd4522d30470ab11444114df9a54751cf767804629c8bf7f09b33807` (853 / 11.01) |
| 최신 빌드·집중 검사 | **853 집중138개(새15) 첫 실행 실패0.** Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0. 성장 P212행·썬 바리케이드330행/너겟288행 및 기존 비공중 부모·공통 Pop·실제 Session을 확인했다. 첫7소스 보존·게임/MP/GUI0이다. |
| checking 잔여 | **12함수=부모6+부분3+MP분류3.**840에서00449710/00449900 완료. 완료부모36/완료부분3이다. 부속00427d30과850의004214a0은 추가 부모로 세지 않는다. 부모004a7510·부분004b54e0/004b8cb0은 같은 집계 안에서8번MP 미완료 이월이다. |
| 마지막 GUI | **794에서 793 제품**의 시작/메인 메뉴를 확인. 815 GUI 통과로 합산하지 않는다 |
| 전체 검사 기준 | 실행 대상66/전체 빌드 대상71. 전체71 마지막 컴파일은779. 전체 C++ 마지막 실행은549의3389개 중7개 실패(580단언); 마지막 전체 통과는510의508제품3337개. 이후 집중 통과를 전체 통과로 쓰지 않는다 |
| 전체 34·ZIP | 645의644 제품, 최종34/34 진입·비정지 시각 증가·창DC RGB·Native Frame 확인. ZIP SHA-256 `6485febc36080dc90e9eec43965a842f85d39e81c82a003a96b83b34b3694b73`, 1376항목/게임 자산1358개, 한글😀 경로 압축 해제 실행 확인 |
| 보호 기준 | **2760개/fixture940개 불변.**853의 새 fixture/입력·기대값 변경0이다. `extracted/cpp-resume-853/protected-after.json`에 보존했다. |
| 마지막 전체 정밀 디컴파일 | **HJOW-Athlon (192.168.0.94) / 2026-11-03**. 양판본 정밀 산출물과 컴파일 기록은 [2026-11-03 기록](docs/exe/cpp-decompilation-20261103.md)을 따른다 |

### 0.2 재개 절차

1. 사용자 재개 요청 뒤 AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 해당 인계도 읽는다. `git status`와 `git fetch`로 다른 PC/세션 변경을 확인하며 push는 하지 않는다.
2. 현재 호스트가 마지막 디컴파일 호스트와 다르면 cpppj 작업/디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 문서에 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 불완전하므로 쓰지 않는다.
3. 실제 게임·별도 창 실행은 AGENTS.md의 확인 규칙을 따른다. 예외는 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94) 및 사용자가 직접 요청한 원본 수동 컨트롤/녹화 분석의 해당 단계다. 다른 PC는 실행 목적·필요성 설명 후 명시적 확인이 필요하다.
4. 최신 정적 범위 정정 원시는 `extracted/cpp-resume-831/`, 최신 제품/검사 원시는 `extracted/cpp-resume-853/`이며 Git 제외·작업 PC 전용이다. 커밋되는 증거는 `cpppj/*.json`·`docs/exe/cpp-*.md`다. 다른 PC에 원시가 있다고 가정하지 않는다.
5. 다른 세션의 dotnetpj/webpj·staged 파일·실행 출력은 포함하지 않는다. 한 단위를 마치면 0.1절·해당 완료/잔여 항목·단위 문서를 갱신하고 해당 파일만 로컬 커밋한다.
6. 네 Gun809·Canon810→811·전환812·레벨813·ContentForm814·Graph 참조 감소/SID0조회815는 완료했다. 816의 sentinel/Free와817의 flags8 부품도 완료했다. 818의 Add/Flood/Rebuild/Allocate/PostPop/PostDestroy와819의 Detach/영역 setter 현재 실행을 완료했다. 820~821에서 Boardcoord0041cdb0의 현재 각도/세 직접caller·방향 네 소비자·실제 바람/입자를 연결했다. 822 주변음/날씨 scheduler를 완료했다. 823은004182c0 시간 알림/임시서식 부모를 연결했다. 824에서00471650의 미도달 진단/현재 수명을 대조했다. 825에서0044c060 연소 갱신의 진단 복귀/실제 Session 회복·생성·삭제를 완료했다. 826에서00472f70 마나 공급원 목록/카운터 진단을 완료했다. 827에서004c0e20은 첫 영역이 아닌 첫 청크record[3] 시드이고 두caller는 군도 전용 분기임을 확인했다. 일반 성장에 삽입하지 않고 군도 상위와 함께 미완료로 남긴다. 828에서 제단00448bd0의 현재 진단 복귀/같은 실제 공간·프레임과 현재 권한을 연결했다. 829에서 숫자 송금004c9350의 현재 checking/진단·대상 수명을 연결했다. 830에서004d62b0의 세 checking이 고정 송금 AI 가지임을 확인하고 실제 원문 위치/현재 서비스에 연결했다. checking17과 객체 메뉴·커서 상태/수명도 남는다. 831에서004b8cb0의 서버 전환/재접속을 같은 부분 집계 안에서8번MP로 미완료 이월했다. 832에서00489ff0의 현재 진단 복귀/같은 Client·Host 삭제/기존 재지정을 연결했다. 833에서0048b340의 현재 SID 진단/항상참 반환과 실제 도착 자기 삭제·겹침 정리를 연결했다. 834에서 공통 Unpop004afe50 일반 공간/진단을 부분 연결했으나 contained 종단이 남아 checking15를 유지한다. 834 뒤의 일시 정지는 새 `/goal`로 해제했고835~840을 완료했다. 840 뒤 일시 정지는 최신 `/goal`로 해제했고841부터 진행한다(0.11절). 포획 안내 실행/실제 포획 등 사용자 제외 검사는 계속 건너뛴다.

### 0.3 유지되는 제한과 보류

- **사용자 직접 확인:** 모든 시나리오의 자연 완주·전체 동일성, 실제 사제 포획/희생·특정 적 공격·시나리오 승리 재현. AI는 별도 요청 없이 자동 공략/테스트 지원을 진행하지 않는다. 짧은 메뉴·진입·기능 검증과 부품 원문 대조는 가능하다.
- **공중 3종:** 훨리기그·맨오워·더스트 데빌은 정적 분석·코드 작업만 한다. 새 전용 생산·생성·우클릭·행동 관측/검사는 하지 않는다.
- **멀티플레이:** 비멀티·UI·Windows 10 호환성 뒤 원문 근거로 사전 구현한다. 신규 단위 검사·게임·실제 통신 검사는 하지 않는다. 사전 구현을 동작/안정성 검증 완료로 쓰지 않는다.
- **사용자 보류:** 스펠 효과 도움말 그림 추가 분석(530, 임시 숨김 유지), 연녹색 직사각형 추가 분석(원본에도 관찰, RDP 원인 미확정). `.english` 부재 Test Battle 개선은 영구 보류가 아니라 **8차 요구**다.
- **환경 한계:** 물리 입력/캡처는762의 새 부팅·RDP세션1에서도 두 번 timeout이었다. 실제 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·실제 앱 전환·실제 PCM/청취·사용자 활성 전체화면의 물리 출력·실제 CD 삽입은 미검증이다.
- **원문/호스트 경계:** 32비트 FILE/FileSpec/SEH·스택 배치/미정값 자체 일치, 부분 I/O/실제 할당 실패와 전체 진단 UI는 별도 한계다. 612의 반사 입력, VFX signed WORD/정상 표본·양수/회전0/원색의 근거를 전체 입력·픽셀로 확대하지 않는다.

### 0.4 2026-11-04 단위834 직전 인계 이력 (새 /goal로 일시 정지 해제)

- **사용자 결정/직전 커밋:**사용자가 현재 작업을 마무리하고 문서·Git 커밋까지 끝나면 목표 일시 정지를 요청했다.834의 아래 검증·인계를 완료했고 이 문서 뒤 단위 커밋 후 update_goal paused를 호출한다. 후속835는 시작하지 않는다. 직전833은 `8a555c99 / 20261104-15 CP`다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 이번 시작에 AGENTS/이 문서/dotnet 인계·hostname/status/fetch를 다시 확인했다.
- **834 일반 경로 완료:**004afe50의 원문 접두 진단/현재 checking·void 선행·저장 TYPE/현재 TYPE·저장 spot byte·옆면/해시/표시 순서를 같은 실제 공간과 Client World/독립Session에 연결했다. 기존 사전 체인/표시 전수검사로 이미 실행했어야 하는 부분 효과를 생략하던 차이를 교정했다. 기존 guard 검사도 원문 근거로 교정했고 fixture 기대값은 불변이다. [834 기록](docs/exe/cpp-squid-unpop-checking-20261104.md).
- **함수 전체 미완료:**현재 contained인 종단004ac6b0→004ad230은 실제 종속 링크/부모 통지·내용물 UI 연결이 남는다. BindContainedUnpop의 장치를 누락하면 선택 시 명시 실패한다. CPU1300행 중 해당60행은 두 하위 요청을 외부 경계로 관측한 것이며 실제 하위 연결/실행 완료가 아니다. 따라서 **checking15=부모9+부분3+MP분류3·완료부모33/부분3 유지**,새 완료부모0이다.
- **제품/검증:**제품834/11.01·EXE SHA는0.1절이다. CPU1300행/진단670행·재관측 일치,집중10개(새3) 실패0이다. raw6450BYTE/네 해시172544BYTE/점유65536BYTE 전체 체크섬과 모든 현재 필드/trace를 대조했고 옆면 쓰기는 기존 실제 RawBuildingSpace다. 실제 같은 Client/Storage6대여(세 진단×2회)와 기존3618단계/VT151/프레임120/도착24·Session 회복 수명도 통과했다. Session 회귀를 새 Unpop 진단의 실제 Session 발생 재현으로 확대하지 않는다. 빌드2회/프로브3회 종료0·경고/오류0이며 기존2758개 불변·새 fixture 포함2759개/939개다. 원시는 `extracted/cpp-resume-834/`다.
- **최초 실패/정적 입력 교정:**첫 Native3221225477/EIP40eaf0는 복사 하한410000 아래 하위 함수 누락이었다. 원문77BYTE를 전용61e000에 옮기고 상대CALL만 연결했다. 두 번째 Native1은 두 번째 CALL 오프셋1a 대신 실제1c 교정으로 해결했다. 첫 소스/EXE/map·두 번째 소스/각 로그를 보존했다. 첫 C++ 실행 전 정적 검토에서 검사 입력의 VT 내부 항목501e04를 실제 기본VT501dd0으로 고치고, 제품 소스 첫 설명 한 줄을 현행 순서로 고쳤다. C++ 검사 실패는0이며 원본/제품 동작/기대 결과를 실패에 맞춰 변경하지 않았다.
- **재개 우선:**Unpop contained004ac6b0의 실제 getter/prev·next·부모head·통지와004ad230의 현재 GumpForm/입력 큐 종단을 같은 World/Session/UI에 연결하고 남은 상위 소비자를 대조한다. 실제 Form 전용 Unpop을 asset에 잘못 호출하거나 이 경계를 빈 성공으로 대체하지 않는다. 다음 작업 단위 번호는835이고 다음 당일 커밋 번호는17부터 확인한다. 기존 Pop·표시·미니맵/건물·원본 fixture를 유지한다.
- **잔여/분류 유지:**831의004b8cb0 서버 전환/재접속·부분004b54e0·부모004a7510은 같은 집계 안에서8번MP로 미완료 이월이다.827 첫 청크record[3] 시드/두caller도 군도 상위와 함께 미완료다. 비멀티1~4/Windows7잔여→MP무검사 사전 구현→4~8차 전체 목표는 미완료이며 사용자 재개 전 진행하지 않는다.
- **검사 제외/운영:**실제 게임/GUI·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용·물리/PCM 재시도·전체71/CTest/34미션/ZIP 실행은0이다. 두 빌드 세션은 모두 종료0을 확인했다. 이후에도 같은 UseMultiToolTask/parallel4 설정·단일 빌드 핸들 추적·진행 중 소스 변경 금지·단위별 인계/커밋·push금지를 유지한다. DN/WB를 수정/커밋하지 않았다.

### 0.5 2026-11-04 단위835 완료·다음836

- 새 `/goal`로 재개해 AGENTS/이 문서/dotnet 인계·hostname/status/fetch를 확인했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**으로 현재 호스트와 일치한다.
- 공통 종속004ac6b0→004ad230/CD004ac160→004acf40의 실제 getter/prev·next·부모head·진단복귀/부모통지·Form검색/UpdateContents39 예약을 같은 World/Session/Client 입력큐에 연결했다. Session 엔진을 World가 weak참조로 빌리고 프로세스 정리 뒤 반납한다. [835 기록](docs/exe/cpp-squid-contained-unpop-20261104.md).
- 집중19개(새11) 최종 실패0·Release2회 종료0·경고/오류0이다. 최초 TYPE74를 빈 base로 가정한 검사 단언1실패는 보호 banner/flags 정확 값으로 교정했고 제품 수정0·첫 소스/로그/덤프를 보존했다. 새 독립CPU/fixture0이며 보호2759개/fixture939개 변경0이다. 제품/EXE SHA는0.1절이다.
- **836 진행 중(빌드/검사 전):**Factory current TYPE/진단과 PATCH !free의 void포함 실제Unpop·생성순서·PostTake진단복귀 소스를 작성했다. 실제 World/Session 현재TYPE·진단을 연결하고 계약검사를 확장 중이다. 최신 검증 제품은835다. 원문004af610은 모든 !free 슬롯에서 가상Unpop(0)을 호출하지만 현재Factory::Take는 contained선행거부/void호출생략과 생성자 전void쓰기를 갖는다. PostTake의LiveBytes contained거부도 대조해야 한다. 이 상위 연결 전에는004afe50 전체 완료로 세지 않으며 checking15=부모9+부분3+MP3·완료부모33/부분3을 유지한다. 다음 당일 커밋번호18부터 확인한다.
- 공통Pop004b02d0의 현재TYPE/좌표/spot부분변경·해시/SHP·firstPop/Activate와 checking진단은 별도 미완료다. 실제 포획/희생/특정 적 공격/승리·사용자 직접 확인·신규MP/통신·공중3종 전용·물리/PCM 재시도는 제외한다. 전체71/CTest/34미션/ZIP 반복0·push0·DN/WB 변경0이다.

### 0.6 2026-11-04 단위836 완료·다음837

- PATCH Take의모든 !free→실제Unpop(0),현재state&fc뒤현재constructor→TYPE→void와PostTake의7f6→현재zOrder쓰기→7fa진단복귀를연결했다. World/Session도같은현재TYPE·Client진단을빌린다.83개VT+48/+2c를PE전수확인했다. [836 기록](docs/exe/cpp-factory-take-unpop-20261104.md).
- 집중35개(새8) 첫실행 실패0·Release1회 종료0·경고/오류0이다. 실제Session4조합과 기존원문생성6992행/생성자359·Unpop1300행/3618단계·VT151을유지했다. 새CPU/fixture0·보호2759개/fixture939개변경0이다. 제품/EXE SHA는0.1절이다.
- **checking14=부모8+부분3+MP3·완료부모34/부분3**이다.835의종단뒤836의Take호출자까지연결해004afe50을완료집계했다. Factory자체SID/TYPE불일치상세로그·진단정상복귀와CD기존Take/PostTake/contained경계는별도잔여로남는다. Factory전체·시나리오전체완료가아니다.
- **837 진행 중(빌드/검사 전,우선순위1):**현재VT 지원목록으로object.count를교정하고실제메뉴회귀를추가중이다. 최신검증제품은836이다. 객체메뉴 Summary/회수의object.count가현재TYPE ctorVT와현재VT쌍일치를요구하는차이를고친다. 원문004ada70은현재VT+80으로직접분배하며83개중81개공통레벨/제단5076b8/다이스507950이다. 실제GUI실행없이현재Summary소비자로검사가능하다. 다음당일커밋번호19부터확인한다.
- 후속자료: CanBridge449710/BridgeCommand449900은진단복귀/내용물finder선행·현재동맹/권한이남고실제희생플레이는제외다. 공통Pop4b02d0은현재TYPE/프레임조회시점·spot부분효과/checking·firstPop/Activate경계가남는다. 실제포획/희생/특정적공격/승리·사용자직접확인·신규MP/통신·공중3종전용·물리/PCM재시도는계속제외한다. 전체71/CTest/34미션/ZIP반복0·push0·DN/WB변경0이다.

### 0.7 2026-11-04 단위837 완료·다음838

- 객체메뉴Summary/회수의object.count에서현재TYPE ctorVT와현재rawVT쌍강제를제거하고현재지원VT+80을선택한다. 보호PE83VT=공통81/제단1/다이스1을확인했다. [837 기록](docs/exe/cpp-object-count-vtable-20261104.md).
- 집중6개(새3) 최종실패0·Release2회종료0·경고/오류0이다. 새검사둘은첫실행통과했고Unknown검사의별개builder callback 예외복구가정2단언은완성메뉴밖count명시실패경계로분리했다. 제품수정0·첫소스/로그보존. 제단/다이스는초기frame0을읽기만했고Pop/Frame/의식/희생/전투를실행하지않았다. 보호2759개/fixture939개변경0·새CPU/fixture0이다.
- **838 진행 중(빌드/검사 전,우선순위1):**공통현재가상80분배/순환RAII와실제제단callback·Session명시소비자소스를작성하고새2검사를추가했다. 최신검증제품은837이다. RawAltarSacrifice::Count의DaisAt한번→유효SID의현재VT+80재분배를실제Session공통분배기에연결한다. 현재temple_.Count직접호출은다이스VT가base인경우차이를낸다. RewardCount/Fort저장/객체메뉴/PlayerCoreRefresh/설치runtime등명시virtual80소비자도공유대상이다. 반복SID중첩은0성공으로숨기지않고호스트순환경계로거부한다. 기본level의freeguard/역방향DaisVT+다른TYPE guard와Temple.Stage내부직접Count는현재경계로구별한다. 다음당일커밋번호20부터확인한다.
- checking14=부모8+부분3+MP3·완료부모34/부분3유지. Factory자체SID/TYPE불일치진단·CD경계·CanBridge/BridgeCommand·공통Pop등은미완료다. 실제포획/희생/특정적공격/승리·사용자직접확인·신규MP/통신·공중3종전용·물리/PCM재시도는계속제외하며전체71/CTest/34미션/ZIP반복0·push0·DN/WB변경0이다.

### 0.8 2026-11-04 단위838 완료·다음839

- 제단Count의typedDais한번→Valid→현재VT80 callback과Session공통VirtualCount를연결했다. Reward/Fort저장/정보·회수·변형/CoreRefresh/runtime/이름/Factory/AI의확인된명시가상80을공유하고공통Has·Factory특정caller guard는유지한다. activeSID첫멤버/RAII로같은SID재귀를거부하고예외뒤정리한다. [838 기록](docs/exe/cpp-nested-count-vtable-20261104.md).
- 집중8개(새2) 첫실행실패0·Release1회종료0·경고/오류0이다. 실제같은Session/UI에서TYPE165/프레임0/레벨7을유지해VTbase/Dais의제단TitleVII/I와cycle예외뒤조회복귀를확인했다. 의식/포획/희생/Frame/Pop/전투함수실행0·보호2759개/fixture939개변경0·새CPU/fixture0이다. 제품/EXE SHA는0.1절이다.
- **당시839 준비 이력:** 현재 getter 읽기 정의역/TYPE 범위 교정은839에서 완료했다(0.9절). 당시 검증 제품은838이었다. Stage 등 내부 Count와 Dais checking은840에서 이어 완료했다(0.10절).
- 독립AltarCount의미연결fallback은실제DaisVT에만허용한다. Temple.Stage등내부직접Count·나머지메뉴·Factory진단·CanBridge/BridgeCommand·공통Pop은별도잔여다. checking14=부모8+부분3+MP3·완료부모34/부분3유지. 실제포획/희생/특정적공격/승리·사용자직접확인·신규MP/통신·공중3종전용·물리/PCM재시도제외·전체71/CTest/34미션/ZIP반복0·push0·DN/WB변경0이다.

### 0.9 2026-11-04 단위839 완료 이력

- Has4ad0a0/CD4acdc0의free/예약SID선행거부를Slot+TYPE범위로교정하고DaisCount4478f0/CD481310의TYPE165강제를현재types/frames물리범위로바꿨다. setter의실제쓰기guard·프레임/코드밖거부는유지한다. [839 기록](docs/exe/cpp-current-count-getter-20261104.md).
- 새3/집중13개첫실행실패0·Release2회종료0·경고/오류0이다. Level3584/Count1920명시입력은전체raw/공간/난수불변·효과0이며새CPU관측이아니다. 기존메뉴8·setter/원문96행재생도통과했다. side80Count만으로signed변이검출을주장하지않고PE movsx/기존bit_cast정적대조를근거로한다. 보호2759개/fixture939개변경0·새CPU/fixture0이다.
- **당시840 준비 이력:** Dais checking/내용물 finder·현재 권한/동맹/TYPE·내부 현재 가상Count를 이어 복원했다. 검증·최초 오류 교정·완료 집계·사용자 요청 일시 정지는0.10절을 따른다. 실제 포획/희생/공격/승리 재현은 진행하지 않았다.
- checking14=부모8+부분3+MP3·완료부모34/부분3유지. 위의다음원문부모외나머지메뉴/Edit·State·파일·Factory진단·공통Pop·MP/4~8차는미완료다. 사용자직접확인/실제포획·희생·특정적공격·승리/신규MP·통신/공중3종전용/물리·PCM재시도제외·전체71/CTest/34미션/ZIP반복0·push0·DN/WB변경0이다.

### 0.10 2026-11-04 단위840 완료·문서/커밋 뒤 사용자 요청 일시 정지

- **현재 사용자 결정:**840의 검증·문서·로컬 커밋까지 마친 뒤 목표를 일시 정지한다.841 이상의 새 단위는 시작하지 않는다. 직전839는 `21cf8caf / 20261104-21 CP`이며 이번 커밋 번호는22다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**으로 일치하고 산출물 변경0이다.
- **840 완료:**CanBridge00449710/CD00481910의 finder 선행/first·next 보존→진단 복귀 뒤 현재TYPE/genus·owner·kind/사제 종류·동맹, BridgeCommand00449900/CD00481ab0의 현재boss/NULL p18e 진단 뒤 반환, MoveAwayDepth00427d30의 저장 조건/currentchecking/현재 반환을 연결했다. 내용물finder13c 복귀 뒤 currenthead와 실제 Session의 현재 서비스도 연결했다. [840 기록](docs/exe/cpp-dais-checking-20261104.md), [JSON](cpppj/dais-checking-20261104.json).
- **내부 가상80:**Stage는 프레임 선행 저장/가상Count1회·판본별표를 사용한다. 실제PE00532620은{1,3,64,120}, CD0052e2b4는{10611,3,64,120}다. EscapeFrame/StartSacrifice/FinishSacrifice의 확인된 호출도 같은 Session 분배기를 사용하며 실제 의식/희생 효과는 실행하지 않았다. 세 기존 효과검사 소스는 장치 연결의 컴파일만 확인했다.
- **검증/오류:**집중17개(새6) 첫 실행 실패0이다. 최초 컴파일C2248 1건은 첫 소스/로그를 보존하고 protectedPid 대신 실제Form+0x12 DWORD를 읽어 수명 검사를 유지했다. 최종Release 종료0·경고/오류0이다. 새CPU/fixture0·보호2759개/fixture939개 변경0이다. 제품840/11.01·EXE SHA-256은 `33fcd2b5e91edeb4a4a2308984792156f6c83ea7ebdb2f483aa3121e6a8e48e0`이며 원시는 `extracted/cpp-resume-840/`다.
- **잔여/재개점:**checking12=부모6+부분3+MP3·완료부모36/부분3이다. 이번 두 부모를 닫고 부속00427d30은 추가 부모로 중복 계산하지 않는다. 공통Pop004b02d0·Factory 자체 진단·나머지 객체 메뉴/커서·Edit·State/파일·Windows 호환성·MP/4~8차는 미완료다. 새 단위에 착수하지 않았으며 재개 시3절 우선순위와0.2절을 따른다.
- 실제 포획/희생/특정 적 공격/승리·사용자 직접 확인·신규MP/통신·공중3종 전용·물리/PCM 재시도는 계속 제외한다. 전체71/CTest/34미션/ZIP 반복0·push0·이번CP단위의DN/WB 수정·커밋 포함0이다.
- **별도 세션 변경 보존:**840 마무리 시 dotnetpj의 별도 미커밋 소스/검사/프로젝트·fixture manifest 변경을 확인했다. 해당 파일은 수정·정리·stage·이번CP 커밋에 포함하지 않는다. 재개 시 git status로 별도 변경을 다시 구별한다.

### 0.11 2026-11-04 단위841 완료·다음842

- 새 사용자 `/goal`로840 뒤 일시 정지를 해제했다. AGENTS/CPP·DN 인계, HJOW-Athlon/192.168.0.94, 마지막 디컴파일 HJOW-Athlon/2026-11-03, git status/fetch를 다시 확인했다. 별도 DN 미커밋 변경은 보존하고 CP 커밋에 넣지 않는다.
- **841 완료:**Take004af610의SID범위1a7 진단→현재 물리 조회, TYPE불일치의현재 두 이름/상세00436d20/문자열0·1ba 진단→현재 가상Unpop/현재 생성자/후단을 연결했다. 상세/진단 callable을 복사해 실행 중 장치 교체의 수명을 보존하고 실제 Client 상세 owner·같은 World/Session에 연결했다. [841 기록](docs/exe/cpp-factory-take-diagnostic-20261104.md), [JSON](cpppj/factory-take-diagnostic-20261104.json).
- **검증:**집중27개(새8) 첫 실행 실패0·Release2회 종료0·경고/오류0이다. 첫 실행 전 정적 검토에서 CD의 기존 진입TYPE 참조와 실제 Client 상세 관찰을 보강했고 첫 소스를 보존했다. 실제 Client 상세를 빌린 World2대여·비멀티 Session과 원문 생성/파생6992행·생성자359행/기존Take8·현재Count5를 확인했다. 새CPU/fixture0·보호2759개/fixture939개 불변이다. 제품841/11.01·EXE SHA는0.1절이며 원시는 `extracted/cpp-resume-841/`다.
- **잔여/재개점:**checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. Factory 전체 완료가 아니며 CD Take/contained/Form·Create/Construct/PostCreate 등 나머지 부모/쓰기와 공통Pop·객체 메뉴/커서·Edit·State/파일·Windows 환경 한계·MP/4~8차가 남는다. 다음은3절 우선순위와 공통 Pop의 현재 읽기/부분 효과를 이어 대조한다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 유지이며 산출물 변경0이다.
- 실제 포획/희생/특정 적 공격/승리·사용자 직접 확인·신규MP/통신·공중3종 전용·물리/PCM 재시도는 제외한다. 전체71/CTest/34미션/ZIP 반복0·push0·이번CP단위의DN/WB 수정/커밋 포함0이다. 단위별 인계/로컬 커밋을 계속한다.

### 0.12 2026-11-04 단위842 완료·다음843

- AGENTS/CPP·DN 인계와 git status/fetch·현재 호스트를 다시 확인했다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전841은 `11c86cdd / 20261104-23 CP`다.
- **842 완료 범위:**공통Pop004b02d0의현재TYPE/저장 입력 범위·좌표→미니맵→부분spot/옆면→grid교체checking→늦은현재SHP/해시/next진단→현재island/표시/firstPop/후단진단을 같은 실제 World/Session/Frame 재등록에 연결했다. 보호83VT+30은모두004ad470이다. [842 기록](docs/exe/cpp-squid-pop-parent-20261104.md), [JSON](cpppj/squid-pop-parent-20261104.json).
- **검증:**집중40개(새15)최종실패0·Release2회종료0·경고/오류0이다. 새15개는첫실행통과. 기존 다리검사1개/7단언의사전무변경가정·실패상태재사용을원문부분변경/독립입력으로교정했고제품소스/fixture기대값수정0이다. 해당묶음6개재실행/총46회다. 기존Pop4600단계/VT151·건물128시퀀스/240단계이상·3판본SetFrame·표시/실제메뉴를확인했다. 새CPU/fixture0·보호2759개/fixture939개불변이며제품842/11.01·EXESHA는0.1절이다. 원시는 `extracted/cpp-resume-842/`이고최초/최종소스를경로별보존했다.
- **다음843/미완료 경계:**004ac560의현재서버/SID별플래그·송신·현재깊이진단과SHP/기타하위의전체진단을 이어 복원한다. 이번 어댑터는35c/void해제/35f와기존비전투정규화까지이므로 전체 Pop/Activate 완료로 세지 않는다. **checking12=부모6+부분3+MP3·완료부모36/부분3 유지**다.3절 우선순위의객체메뉴/커서·Edit·State/파일·Windows 한계·MP/4~8차도남는다.
- 실제 포획/희생/특정 적 공격/승리·사용자 직접 확인·신규MP/통신·공중3종 전용·물리/PCM 재시도·전체71/CTest/34미션/ZIP 반복은 제외했다. push0·DN/WB 수정/커밋 포함0이다. 단위별 인계/로컬 커밋 뒤 계속 진행한다.

### 0.13 2026-11-04 단위843 완료·다음844

- AGENTS/CPP·DN인계·hostname/status/fetch를다시확인했다. 현재/마지막전체정밀디컴파일은 **HJOW-Athlon / 2026-11-03** 일치이며산출물변경0이다. 직전842는 `f60de2a8 / 20261104-26 CP`다.
- **843완료범위:**004ac560의void/현재서버/SID별플래그/실제writer·송신전후상태/현재깊이/392진단복귀를같은World/Session/Client장부에연결했다. PE83VT의+54는82개004ad170/1개00455ae0이며전자는정밀목록누락을PE에서직접읽어35BYTE·기본압축00432fe0으로복원했다. [843 기록](docs/exe/cpp-squid-activate-20261104.md), [JSON](cpppj/squid-activate-20261104.json).
- 공통2048BYTE버퍼/현재헤더/삭제표식/실제설정키cs·owner캐시/누적은같은장기GameAux를빌린다. 일반Squid/Form과실제Session방송분배기가이를공유한다. 새MP/패킷/통신전용검사0이며비멀티Session의일반호출을패킷바이트/통신검증으로세지않는다. 별도수신자/삭제선택·프로세스writer전체/decoder·재동기화의허용/owner캐시부모는8번잔여다.
- **검증:**집중51개(새11)첫실행실패0·Release제품2회/집중대상1회종료0·경고/오류0이다. 정적추출의누락/0반환가정은제품검사전에PE로교정했고첫빌드뒤압축선언DWORD/고정WORD를보강했다. 첫실행전기존프레임검사3곳에실제writer/명시빈연결표를연결했고서버입력/flags/기대값은불변이다. 빌드중소스변경0·최초18/검사시점21소스/로그보존·기존fixture기대값변경0이다. 기존Pop4600/VT151·건물128시퀀스/240이상·3판본SetFrame·표시/실제메뉴를유지했다. 새CPU/fixture0·보호2759개/fixture939개불변이며제품843/11.01·EXESHA는0.1절이다. 원시는 `extracted/cpp-resume-843/`다.
- **다음844/미완료:**00419850의상세오류/e1진단정상복귀·현재SHP와공통postPop의현재TYPE/나머지하위를대조한다. 전체Pop완료로세지않아 **checking12=부모6+부분3+MP3·완료부모36/부분3유지**다. 객체메뉴/커서·Edit·State/파일·Windows한계·MP/4~8차도계속남는다.
- 실제포획/희생/특정적공격/승리·사용자직접확인·공중3종전용·물리/PCM재시도·전체71/CTest/34미션/ZIP반복은제외했다. push0·DN/WB수정/커밋포함0이며단위인계/로컬커밋뒤계속한다.

### 0.14 2026-11-04 단위844 완료·다음845

- AGENTS/CPP·DN인계와hostname/status/fetch를확인했다. 현재/마지막전체정밀디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물변경0이다. 직전843은 `bd36f6e6 / 20261104-27 CP`다.
- **844완료:**00419850의signedDWORD두배감김·checking무관상세/e1정상복귀·선택TYPE/저장frame/현재SHP를실제FrameSize/Client상세와World/비멀티Session에연결했다. caller004ace40의abstract인자는고정0이다. postPop/Graph/이웃은현재TYPE배열을명시대여하며독립snapshot소유계약은보존한다. [844 기록](docs/exe/cpp-rift-shape-current-types-20261104.md), [JSON](cpppj/rift-shape-current-types-20261104.json).
- **검증:**집중97개(새15)최종실패0이다. 첫97개중91개통과/6개실패(18단언)였고새15개는모두첫통과했다. 기존RawGraph의Pop grid로그장치미연결과옛전체무효과가정을교정했다. 명시projected사전계산은무효과로유지하며실제Pop의후처리실패앞공간/state/depth부분효과를분리검사했다. fixture입력/기대값·제품추가수정0이다. 두묶음18개재실행의17개통과뒤교정검사자체의x20/21별도버킷가정1단언실패를원문2칸해시에맞게고쳐해당1개만재통과했다. 세이름교체는새검사로세지않았다. Release제품/두집중대상1회+교정검사대상2회종료0·경고/오류0·빌드중소스변경0이다. 최초15/교정·최종16소스·모든실패로그보존·보호2759개/fixture939개불변·새CPU/fixture0이며제품844/11.01·EXESHA는0.1절이다. 원시는 `extracted/cpp-resume-844/`다.
- **다음845/미완료:**공통004b0d30의그래프enabled재조회/현재TYPE재조회·저장genus/80b진단복귀·선행Validate와004da650의실제mouseType/기억좌표/Select(0,1)/원래found의현재extra재검사를이어복원한다. 전체프레임코드재적재/전체후처리/전체Pop완료로세지않아 **checking12=부모6+부분3+MP3·완료부모36/부분3유지**다. 객체메뉴/커서·Edit·State/파일·Windows한계·MP/4~8차도남는다.
- 실제포획/희생/특정적공격/승리·사용자직접확인·신규MP/패킷/통신전용·공중3종전용·물리/PCM/CD재시도·전체71/CTest/34미션/ZIP반복은제외했다. push0·DN/WB수정/커밋포함0이며단위인계/로컬커밋뒤계속한다.

### 0.15 2026-11-04 단위845 완료·다음846

- 직전844는 `5626fa85 / 20261104-28 CP`로커밋했다. 현재/마지막전체정밀디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물변경0이다.
- **845완료:**004b0d30의메타맵선행/현재그래프활성·TYPE재조회/80b정상복귀·그래프뒤저장genus/비용→현재공급→저장공장군→현재AI/로컬owner·TYPE통계→실제선택재검사→깊이감소를같은그래프/장부에연결했다. 실제PATCH공통앞전체Validate는제거하고독립명시preview/Activate·CD계약은유지했다. [845 기록](docs/exe/cpp-post-pop-current-parent-20261104.md), [JSON](cpppj/post-pop-current-parent-20261104.json).
- 004da650은현재mouseType/저장float비트→같은World finder→현재selected관문→Select(found,0,1)→원래found의현재extra를연결했다. 선택거부0/제단재지정반환으로tail을바꾸지않는다. UI약한수명반납뒤검색결과가없거나다른현재선택이면장치를요구하지않는다. Session정리시외부World대여를분리하며AI통지뒤현재Client owner를읽는다.
- **검증:**집중130개(새17)첫실행실패0·Release제품/세집중대상2회종료0·경고/오류0이다. 첫검사전정적검토로UI미연결시원문검색/다른선택조기반환과기존Graph/PostPop단언2곳을교정했다. 영역뒤Add실패의그래프부분효과와선택실패앞등록/통계를보존하며원문fixture입력/기대값변경0이다. 최초16/최종18소스·교정전4파일/빌드로그보존·빌드중소스변경0·보호2759개/fixture939개불변·새CPU/fixture0이다. 제품845/11.01·EXESHA는0.1절이며원시는 `extracted/cpp-resume-845/`다.
- **다음846/미완료:**현재가상+20의파생분배와파생몸체자체Validate/실제부모·쓰기경계를대조한다. 공통몸체연결을파생전체/전체Pop완료로세지않아 **checking12=부모6+부분3+MP3·완료부모36/부분3유지**다. 전체프레임코드재적재/나머지객체메뉴·커서/Edit/State/파일/군도/Windows한계/MP·4~8차도남는다.
- 실제포획/희생/특정적공격/승리·사용자직접확인·신규MP/패킷/통신전용·공중3종전용·물리/PCM/CD재시도·전체71/CTest/34미션/ZIP반복은제외했다. push0·DN/WB수정/커밋포함0이며단위인계/로컬커밋뒤계속한다.

### 0.16 2026-11-04 단위846 빌드전중단·인계/커밋뒤사용자요청목표일시중지

- **현재사용자요청:**지금작업을바로중단하고인수인계문서작성/Git커밋뒤목표를일시중지한다. 구현/검사를즉시중단했다. 이작성본은미완료이며재개요청전후속구현/빌드/검사를시작하지않는다. 직전검증커밋은 `99ff1a5f / 20261104-29 CP`다.
- **검증기준유지:**마지막제품845/11.01·집중130개첫실행실패0과EXESHA는0.1절이다. **846빌드/C++검사/CPU/GUI실행0**이며현재EXE는845그대로다. 현재/마지막전체정밀디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물변경0이다.
- **작성본8파일:**Carrier/Damageable의PATCH전체Validate제거/선택장치·최초genus접근/현재TYPE대여·로딩깊이와Session두연결,공통owner→localOwner읽기순서,기존Carrier C++단언교정과새검사정의13개(새파일12+기존파일1)를보존했다. **새Carrier파일의12개검사정의는CMake미연결**이다. 실제Session검사·실행기작성도남았다. [상세중단인계](docs/exe/cpp-carrier-current-parent-wip-20261104.md), [미검증JSON](cpppj/carrier-current-parent-wip-20261104.json).
- **정적근거/보호:**양판본부모2쌍+패치getter2개/기계어4개·83VT+20을대조했다. Carrier3표의cc=00426fc0만확인했고83전체의cc유효성을주장하지않는다. 원문실행관측이아니다. 중단소스/교정전두검사·정적원시와보호해시는 `extracted/cpp-resume-846/`에있으며Git제외/현재PC전용이다. 보호2759개/fixture939개불변·새fixture0·기존Native입력/기대값변경0이다.
- **재개우선:**작성본/현재참조수명검토→새검사CMake/기존Carrier fixture정의연결→같은실제Session검사작성→소스해시보존/단일관련빌드/허용집중검사→보호/제품/인계갱신/846완료커밋순이다. 논의했던135개는아직확정/실행목록이아니다. 상세인계의World.TakeChanged관찰을실제소유권변경검증으로확대하지않는다.
- **잔여/운영:**checking12=부모6+부분3+MP3·완료부모36/부분3유지다. 나머지현재가상20/파생자체검사·객체메뉴/커서/Edit/State/파일/군도/Windows한계/MP·4~8차도미완료다. 실제포획/희생/특정적공격/승리·사용자직접확인·공중3종전용·신규MP/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP반복은제외했다. 중단시실행빌드없음·DN/WB수정/커밋포함0·push0이다. 인계/미완료작성본커밋뒤update_goal paused를호출하고중지한다.

### 0.17 2026-11-04 단위846 완료·다음847

- 사용자요청으로직전작업/목표를재개했고goal상태 **active** 를확인했다. AGENTS/CPP·DN인계·hostname/status/fetch를다시확인했다. 현재/마지막전체정밀디컴파일은 **HJOW-Athlon / 2026-11-03** 일치이며중단커밋은 `ab4fd3d2 / 20261104-30 CP`다. 0.16절은당시중단이력이다.
- **846완료:**00427720/0044bf80의선택장치·현재TYPE/로딩/간헐천/extra/flags1과최초004ad080접근을원문위치로연결했다. 실제PATCH공통전수Validate를제거해앞선가상확인/지면/비용효과를보존하며CD/독립사본계약을구별한다. Session두Carrier가같은현재TYPE/로딩·간헐천DWORD를빌린다. 공통owner→localOwner의두mov순서도보존한다. [846 기록](docs/exe/cpp-carrier-current-parent-20261104.md), [JSON](cpppj/carrier-current-parent-20261104.json).
- 실제Session은명시현재Damageable VT50a820과flags400끄기/켜기·로딩/간헐천TYPE로같은World지면하위도달을확인했다. World.TakeChanged표지를실제소유권변화검증으로확대하지않으며Frame/전투/포획/희생은실행하지않았다. 명시메모리준비는복구했다.
- **검증:**집중135개(새14)첫실행실패0·Release제품/두집중대상1회종료0·컴파일경고/오류0이다. 기존Carrier세판본7344행과공통Pop/프레임/메뉴를유지했다. WIP의C++인공사전검사단언교정은원문순서로검증했고기존Native입력/기대값은불변이다. 재개때중단소스8개/보호파일/845EXE를확인했고첫빌드10소스/로그를보존했다. 보호2759개/fixture939개불변·새CPU/fixture0·빌드중소스변경0이다. 제품846/11.01·EXESHA는0.1절이며원시는 `extracted/cpp-resume-846/resume-20261104/`다.
- **다음847/미완료:**Factory/나머지현재가상20파생의자체Validate·비가상부모호출/현재분배를대조한다. 전체Pop/다른파생완료로세지않아 **checking12=부모6+부분3+MP3·완료부모36/부분3유지**다. 객체메뉴/커서/Edit/State/파일/군도/Windows한계/무검사MP·4~8차도남는다.
- 사용자직접확인·실제포획/희생/특정적공격/승리·신규MP/패킷/통신·공중3종전용·물리/PCM/CD재시도·전체71/CTest/34미션/ZIP반복은제외했다. DN/WB수정/커밋포함0·push0이며단위인계/로컬커밋뒤계속한다.

### 0.18 2026-11-04 단위847 완료·다음848

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전846은 `506542a8 / 20261104-31 CP`이며 목표 active를 유지한다. AGENTS/CPP·DN인계·상태/hostname을 다시 확인했다. 정밀 산출물 변경0이다.
- **847완료:**004538f0의 선택 예약/생성·프레임·중복 경고/삭제 장치를 원문 위치에서 요구해 실패 앞부분 효과를 보존했다. 00453780의 저장 프레임/첫 시작값과 두 현재 가상80→현재TYPE/개수 순서를 구별하고 같은 실제 Session VirtualCount를 연결했다. CD 기존 사전 검사 계약은 유지했다. [847 기록](docs/exe/cpp-factory-current-parent-20261104.md), [JSON](cpppj/factory-current-parent-20261104.json).
- **정적 범위:**두 PATCH Factory VT의80=004ad0f0/Has004ad0a0·두 CD VT의80=004ace00을 PE/정밀 원문으로 확인했다. 세 Aviary의 직접 부모는 Factory가 아닌0044bf80 Damageable이므로 기존 경로를 바꾸지 않았다. 아웃포스트 wrapper의 선택 장치 변경은 정적 대조/빌드이며 MP 동작 검증이 아니다.
- **검증:**집중77개(새13)/18묶음 첫 실행 실패0·Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0이다. 일반 Factory Native372행(아웃포스트 kind/TYPE126 제외)·Carrier7344행의 기존 기대값과 공통 Pop/프레임/메뉴를 유지했다. 같은 Session의 명시 일반 슬롯은 공통 콜백 뒤 현재 미지원 VT 실패·깊이 감소 보존·정상 VT 복귀를 확인했고 Frame 틱/생산/전투/의식은 실행하지 않았다. 첫6소스/실제 빌드 핸들55723·종료/자식 해제와 로그를 보존했다. 보호2759개/fixture939개 불변·새CPU/fixture0·빌드 중 소스 변경0이다. 제품847/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-847/`다.
- **다음848/잔여:**Factory 타입번호 전역의 현재 공유와 다른 현재 가상20 파생 자체 검사/비가상 부모·분배를 이어간다. 전체 Pop/다른 파생 완료로 세지 않아 checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. 객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 남는다. 사용자 직접 확인·공중3종 전용·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

### 0.19 2026-11-04 단위848 완료·다음849

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전847은 `b7887131 / 20261104-32 CP`이며 목표 active를 유지한다. 원본/정밀 산출물 변경0이다.
- **848완료:**PATCH005411ac/005411b4/00541250의 현재 전체 DWORD를 두 예약 비교/공통 뒤 프레임 제외에서 각각 읽는다. 보호 PE 초기85/87/126은 양 판본에서 확인했다. 기존 Client/Storage GameAux가 세 값을 장기 소유하고 실제 Session Factory는 같은 주소를 빌린다. 기존 researchOutpost의 AI 제외/연구/업그레이드도 같은00541250으로 연결했다. Runtime 생성/반납에서 초기값을 다시 쓰지 않으며 독립 값 사본 계약도 유지한다. [848 기록](docs/exe/cpp-factory-type-globals-20261104.md), [JSON](cpppj/factory-type-globals-20261104.json).
- **검증:**집중85개(새8)/20묶음 첫 실행 실패0·Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0이다. 일반 Factory Native372행·Carrier7344행의 기존 입력/기대값·공통 Pop/프레임/메뉴를 유지했다. 실제 같은 Client 두 Session은 명시 일반 TYPE74/Factory VT509148의 예약 Form·현재 두 번호/같은 주소를 확인했고 일반 프레임 helper의 공통 뒤 현재 제외 DWORD도 확인했다. 실제 Frame/Process 틱·생산/행동·Outpost wrapper/MP 연구/업그레이드는 실행하지 않았다. 첫9소스/실제 빌드 핸들28313·종료/자식 해제와 로그를 보존했다. 보호2759개/fixture939개 불변·새CPU/fixture0·빌드 중 소스 변경0이다. 제품848/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-848/`다.
- **다음849/잔여:**다른 현재 가상20 파생 자체 검사/현재 부모·분배와 다른 TYPE 전역 소비자를 이어간다. RawAssetPostPop의 Residence/Geyser 등 전체 사전 검사와 선택 장치를 원문에 대조할 차례다. 전체 Pop/다른 파생 완료로 세지 않아 checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. 객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 남는다. 사용자 직접 확인·공중3종 전용·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

### 0.20 2026-11-04 단위849 완료·다음850

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전848은 `477e6939 / 20261104-33 CP`이며 목표 active를 유지한다. AGENTS/CPP·DN인계·hostname/status/fetch를 다시 확인했고 원본/정밀 산출물 변경0이다.
- **849완료 범위:**Residence0049a2b0/Geyser004852e0의 실제 PATCH 선택 장치/부분 효과·같은 free/contained 물리 쓰기와 signed 목록 비교→배열 store→현재 count inc를 연결했다. Geyser의 벡터 사전 수집을 같은 실제 RawSquidFinder의 효과→Next로 바꿨고 독립 벡터 입력도 유지한다. 양 판본 WORD setter004ac220/CD004abb00의 현재 Bridge TYPE 통지와 World BridgeState 참조도 추가했다. 다른 파생/CD의 사전 검사 원칙을 유지하며 새 현재 finder도 독립 사전 검사의 대안으로 인식한다. [849 기록](docs/exe/cpp-asset-current-parents-20261104.md), [JSON](cpppj/asset-current-parents-20261104.json).
- **검증:**첫106개(새19)/24묶음 통과 뒤 추가 원문 대조로 WORD 통지2개를 더해 최종108개(새21)/24묶음 첫 실패0이다. Release 제품/두 집중 대상 빌드2회 종료0·컴파일 경고/오류0이다. 해당 두 Asset Native376행·Factory372행·Carrier7344행의 기존 입력/기대값과 공통 Pop/프레임/메뉴를 유지했다. 실제 finder는 첫 효과의61.next=63 변경 뒤60→61→63을 방문했다. 실제 같은 Session은 네 단계 빈 해시 위치의 공유 목록/Regular Form·현재 비용을 확인했고 AI1~8은 명시 null DWORD 입력으로 저장/복구해 원문의 실제 null-AI 순회만 실행했다. nonnull AI 하위·Process/Frame 틱·실제 행동 검증이 아니다.
- 첫/두 번째6소스·실제 핸들68199/67183 종료/자식 해제와 로그를 보존했다. 빌드 중 소스 변경0·검사 뒤 실행 코드 변경0이며 Binding 설명 주석1곳만 실제 공유 범위에 맞게 교정하고 전후 해시/문구를 보존했다. 보호2759개/fixture939개 불변·새CPU/fixture0이다. 제품849/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-849/`다.
- **다음850/미완료:**부속004214a0 NotifySurface는 현재 checking 뷰가 있으나 내부 TYPE 사본/원문 진단 정상 복귀·실제 Client 연결이 남는다. 기존779의51함수와 별도 부속 공백이며 checking12=부모6+부분3+MP3·완료부모36/부분3을 유지한다. 그 뒤 다른 현재 가상20 파생/전체 Pop·다른 TYPE/모드 전역·객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 이어간다. 사용자 직접 확인·포획/희생/특정적공격/승리·공중3종 전용·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

### 0.21 2026-11-04 단위850 완료·다음851

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전849는 `7386bc03 / 20261104-34 CP`이며 목표 active다. AGENTS/CPP·DN인계·hostname/status/fetch를 다시 확인했고 원본/정밀 산출물 변경0이다.
- **850완료:**PATCH004214a0의 현재 Bridge DWORD/TYPE·extra→BYTE nibble→현재 checking·선택 진단/정상 복귀를 연결했다. CD00448c10은 관련 호출자가 버리는 포인터 계산뿐이며 raw/진단 미접근을 유지한다. 같은 World BridgeState TYPE·장기 checking 뷰와 World의 현재 Storage Client 장치/Session 실제 서비스 callable을 사용한다. 현재 실행 callable의 자체 연결 반납 수명과 물리 free/예약 SID도 보존한다. [850 기록](docs/exe/cpp-building-notify-current-20261104.md), [JSON](cpppj/building-notify-current-20261104.json).
- **검증:**정밀 함수만 실행한 원문2264행/4528관측과 재관측2264행이 byte 일치다. PATCH 진단164/명시 중단6·CD 진단0이며 raw 전체/128슬롯 전체 Adler/원문 trace·현재 DWORD/복귀를 대조했다. CD 반환 포인터는 실제 관측 후 void API 비교에서 제외했다. 집중119개(새9)/27묶음 첫 실패0·Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0이다. 기존 옆면128장면/공간 단계·Asset376행/Factory372행/Carrier7344행의 기대값·공통 Pop/실제 World/Session을 유지했다. 실제 Session은 명시 일반 슬롯의 현재 TYPE/checking/서비스 복귀만 확인하고 입력을 복구했으며 Frame/Process/행동은 실행하지 않았다.
- 첫9소스·핸들39969 종료/자식 해제와 로그를 보존했다. 빌드 중 소스 변경0·첫 검사 실패/교정 재실행0이며 기존2759개/fixture939개 불변이다. 새 bridge-notify fixture1개 포함 보호2760개/fixture940개다. 제품850/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-850/`다.
- **다음851/잔여:**부속 완료는 기존779의51함수와 별도여서 checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. Gun/Generator/장식 등 다른 현재 가상20 파생 자체 검사/현재 부모·전체 Pop·다른 TYPE/모드 전역을 이어간다. 객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 남는다. 실제 게임/진단UI·포획/희생/특정적공격/승리·사용자 직접 확인·공중3종 전용·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

### 0.22 2026-11-04 단위851 완료·다음852

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전850은 `edc7db85 / 20261104-35 CP`이며 목표 active다. AGENTS/CPP·DN인계·hostname/status/fetch를 다시 확인했고 원본/정밀 산출물 변경0이다.
- **851완료:**PATCH0041c150·0046a520/600/aca0/810·004694b0은 실제 가상 진입에서 도달한 장치만 요구한다. 그래프 설정/애니메이션 제거·확보 뒤의 실패 앞 부분 효과와 현재 서버/extra·제네레이터의 최초 theme 조회를 유지한다. GunSun의 WORD 복원을004ac220의 물리 store→현재 Bridge DWORD 통지로 연결하고 Geyser와 같은 setter를 공유한다. 실제 Session Angle은0..3을 같은 World Graph의 현재 TYPE 진단/서비스 복귀·물리 BYTE 쓰기로 전달한다. 명시 Validate/Activate·CD 및 나머지 파생 계약은 남긴다. [851 기록](docs/exe/cpp-asset-gun-current-20261104.md), [JSON](cpppj/asset-gun-current-20261104.json).
- **검증:**집중105개(새15)/17묶음 첫 실행 실패0·Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0이다. 기존 양판본 여섯 몸체636행·Gun 진단1344행·Residence/Geyser376행·Notify2264행·Carrier7344행의 기대값과 공통 Pop/실제 Session을 유지했다. 새 Native/fixture0이며 보호2760개/fixture940개 불변이다. 실제 Session은 명시 일반 슬롯의 그래프 진단 복귀만 실행했으며 Frame/Process 틱·Gun 생성/발사/행동은 실행하지 않았다.
- 첫4소스·핸들74798 종료/자식 해제와 로그를 보존했다. 빌드160.5031421초·빌드 중 소스 변경0·첫 검사 실패/교정 재실행0이다. 제품851/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-851/`다.
- **다음852/잔여:**checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. 다른 현재 가상20 파생(Blocker/스파이어·애시드/성장 아이스·썬 바리케이드/Vortex 등)의 선택 장치/부모와 다른 TYPE/모드 전역·전체 Pop을 대조한다. 공중3종은 코드/정적 분석만 하며 전용 검사를 추가하지 않는다. 객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 남는다. 실제 게임/진단UI·포획/희생/특정적공격/승리·사용자 직접 확인·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

### 0.23 2026-11-04 단위852 완료·다음853

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전851은 `d4fe4e13 / 20261104-36 CP`이며 목표 active다. AGENTS/CPP·DN인계·hostname/status/fetch를 다시 확인했고 원본/정밀 산출물 변경0이다.
- **852완료:**PATCH0048eda0은 현재 서버·최초·로딩0의 통지와 직접 Damageable 순서를 사용하며 같은 세 VT에 extra9 차단을 추가하지 않는다.0048c080/0048c7a0은 extra9·최초→확보 성공 때 event0/지연1초·0.3초→직접 부모를 실행한다. 기존 하위 helper는 유지하고 상위 실제 가상 진입의 일괄 Validate를 제거했다. 도달한 통지만 요구하고 확보/생성 뒤 실패 앞 부분 효과와 현재 물리 수명을 보존한다. 명시 Validate/Activate·CD/나머지 파생 계약은 유지한다. [852 기록](docs/exe/cpp-asset-fence-current-20261104.md), [JSON](cpppj/asset-fence-current-20261104.json).
- **검증:**집중120개(새15)/19묶음 첫 실행 실패0·Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0이다. 기존 Blocker96행과 두 펜스의 P 후처리100행만 선택해 trace/전체 입력 슬롯 불변을 대조했고 공격/수신/통신 행은 실행하지 않았다. Gun636행/진단1344행·Residence/Geyser376행·Notify2264행·Carrier7344행 및 공통 Pop/실제 Session도 유지했다. 실제 Session의 두 Regular Form 지연·카운트·같은 시계를 확인했으며 Frame/Process 틱·발사/공격은 실행하지 않았다.
- 첫3소스·핸들48023 종료/자식 해제와 로그를 보존했다. 빌드52.926605초·빌드 중 소스 변경0·첫 검사 실패/교정 재실행0이다. 새 Native/fixture0·보호2760개/fixture940개 불변이다. 제품852/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-852/`다.
- **다음853/잔여:**checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. 성장 아이스·썬 바리케이드·너겟/Vortex 등 다른 현재 가상20 파생/부모와 다른 TYPE/모드 전역·전체 Pop을 대조한다. 공중3종은 코드/정적 분석만 하며 전용 검사를 추가하지 않는다. 객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 남는다. 실제 게임/진단UI·포획/희생/특정적공격/승리·사용자 직접 확인·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

### 0.24 2026-11-04 단위853 완료·다음854

- 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전852는 `64a3929e / 20261104-37 CP`이며 목표 active다. AGENTS/CPP·DN인계·hostname/status/fetch를 다시 확인했고 원본/정밀 산출물 변경0이다.
- **853완료:**PATCH00457560/0048ead0/00484f90의 상위 실제 가상 진입에서 도달한 전체 몸체만 요구한다. 썬 바리케이드 내부 Validate/VT 재검사를 제거하고 네 방향 저장→현재 owner DWORD42→쌍 예약/현재 좌표/연결선→현재 연결DWORD46→최종 예약→Damageable 순서를 같은 물리 슬롯에 유지한다. 성장 아이스는 확보 뒤 현재 프레임 조회가 생성자 경계보다 먼저 발생하도록 식 평가 순서를 고쳤다. 너겟의 두 독립 확보/난수/생성 순서는 유지한다. 명시 Validate 계약은 남기고 옛 guard 단언만 그 API로 교정했으며 fixture 변경0이다. [853 기록](docs/exe/cpp-asset-tail-current-20261104.md), [JSON](cpppj/asset-tail-current-20261104.json).
- **검증:**집중138개(새15)/23묶음 첫 실행 실패0·Release 제품/두 집중 대상1회 종료0·컴파일 경고/오류0이다. 기존 성장 P 후처리212행은 예약 trace만 비교하고 원문150/151의 프레임 수를 일반 TYPE70/75에 공급했다. 원문 전체 초기 슬롯의 같은 재생으로 확대하지 않는다. 썬 바리케이드330행은 기존 전체250BYTE/trace·너겟288행은 확보/난수/생성/부모 trace를 대조했다. 기존 Blocker96/두 펜스P100·Gun636/진단1344·Residence/Geyser376·Notify2264·Carrier7344와 공통 Pop/Session도 유지했다. Session은 성장event3a6/너겟event1·3의 실제 예약/시계와 썬 바리케이드 flags0 직접 부모만 확인하며 틱·성장/생산/행동/공격은 실행하지 않았다.
- 첫7소스·핸들37499 종료/자식 해제와 로그를 보존했다. 빌드59.7383367초·빌드 중 소스 변경0·첫 검사 실패/교정 재실행0이다. 새 Native/fixture0·보호2760개/fixture940개 불변이다. 제품853/11.01·EXESHA는0.1절이며 원시는 `extracted/cpp-resume-853/`다.
- **다음854/잔여:**checking12=부모6+부분3+MP3·완료부모36/부분3 유지다. Vortex와 다른 현재 가상20/전체 Pop·TYPE/모드 전역을 대조한다. 공중3종은 코드/정적 분석만 하며 전용 검사를 추가하지 않는다. 객체 메뉴/커서·Edit·State/파일/군도·Windows 한계·무검사MP와4~8차도 남는다. 실제 게임/진단UI·포획/희생/특정적공격/승리·사용자 직접 확인·신규MP/패킷/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복은 제외했다. DN/WB 변경0·push0이며 단위 인계/로컬 커밋 뒤 계속한다.

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **진행 중.** 34경로 진입·진행은 가능하다. AI 작업으로 남은 것은 UI/Edit/시나리오/수명이다. 자연 완주와 전체 동일성은 사용자 확인 대상이다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **진행 중.** Windows 10 호환 전체화면과 저장 재시작은 확인했다. 남은 Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다. 10.72의 Windows 10 구동 불가 해결책은 10.72 작업 때 문서화한다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | 전용 단계는 미착수. 채팅 MP 분기·707의 공유 의식 선택/비미션 UI·709의Zsend/송신장치처럼 일부만 정적으로 작성했고 신규 MP 검사는 하지 않았다. |
| 4차 | **로컬MCP**로 dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화. **16:9 최대 1920×1080 지원**이며 이 단계에서 그래픽 크기 스케일 조정은 불필요하다. | 미착수. 기존 화면비 처리/원문BMP 수집과 별도의 완료 조건이다. |
| 5차 | **일반 권한·읽기 전용 설치 정상 동작.** 설정/진행은`%USERPROFILE%\Saved Games\Netstorm\Saves\`, 신규 맵은`CustomMaps\`다. 모든 자동 생성 파일/디렉터리(임시/백업/잠금/장부/캡처/진단 포함)와 대응 reader도 같은Netstorm하위로 옮긴다. 원래/사용자 맵 병행 읽기와 Edit 스크롤/무제한도 포함한다. | **구현 미착수.** 사용자aaafbbc4를733에서 반영하고12파일군/50정적후보를 고정했다. 실제 권한/전체 경로 검증은 남는다. |
| 6차 | 한국어 지원 | 미착수 |
| 7차 | Options의AA/FSR1.0 선택은 기본 off. FSR 지원 환경에는2560×1440/3840×2160을 추가하고 해당 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 목표로 업스케일한다. [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)를 참고한다. | 미착수 |
| 8차 | Edit 전용 시나리오 구성 UI(초기 자본/테크·적·동맹 등), 새 맵의 기본 english 자동 생성, english 부재 Test Battle 종료 개선 | **미착수.** 사용자98b2f20c의 AGENTS/TODO 갱신을728에서 반영했다.1~7차 뒤 진행한다. |

**단계 전환 기준 (2026-10-31 사용자 결정):**

- **자연 완주·전체 동일성:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상이 아니며, 단계에 남은 작업이 이것뿐이면 다음 단계로 넘어간다.
- **Windows 11:** Windows 10에서 오류 없이 구동되면 Windows 11에서도 문제가 없을 가능성이 높다고 본다. Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다.
- **제보 처리:** 사용자가 플레이 중 오류나 원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다.

60·120프레임 지원과 CD 10.72 전체 복원은 별도 후순위다. cpppj는 Windows용만 개발한다. dotnetpj 후순위 요구에도 4~8차가 포함되며, webpj는 Edit·멀티플레이를 제외한다.

### 1.2 현재 목표 범위 ([TODO.md](TODO.md))

- Early Missions 6개, Priest Training 9개, 캠페인 전체 16개, 데모 3개를 사용자가 플레이할 수 있고 원본 10.78과 완전히 같게 동작하도록 한다. 자연 완주와 전체 동일성의 확인은 사용자가 한다(1.1절).
- Edit 메뉴의 커스텀 맵/시나리오, 메인 메뉴의 비멀티 항목, 좌측 덱·미니맵·다리 단축키 QWASZX·ESC 메뉴를 구현한다.
- Windows 10/11 호환성을 해결한다. 원본과 다른 호환 처리와 이유는 한국어 코드 주석에 명시한다. 전체화면, 4:3/16:9/16:10, 가장자리 스크롤 요구를 유지한다.
- 그 뒤 멀티플레이를 무시험으로 사전 구현하고 4~8차를 진행한다(98b2f20c의 AGENTS/TODO 갱신).
- 버전 표기는11.01이며 Credits 목록에 세 번째 Netstorm 11.01 Patch Credits와 동일한 형식의 메시지 창(제목NetStorm 11.01 Patch Credits, HJOW 개발/AI 사용 설명)을 추가한다(추가 AGENTS.md).
- 외부 도메인/IP 접속 기능을 로컬 서버와 구별해 문서화하고, 클라이언트 유효성 검사는 비활성화한다(갱신 AGENTS.md,684 적용).
- 개발 작업 단위마다 인수인계를 갱신하고 커밋한다. 커밋에 실패해도 작업은 이어 간다.

대상은 원본 `d/offical1~5.english`의 MissionBegin 순서와 `tell.DEMOVILLE`의 데모 순서다.

| 구분 | 미션 키 (원본 표시 이름) |
| --- | --- |
| Early Missions (offical1) | Tutorial1 (Bridge the Gap), Tutorial2 (Secret Workshop), Tutorial3 (Capture The Priest), Tutorial4 (Tactical Combat), Tutorial5 (Subtle Defense), Tutorial6 (Raw Power) |
| Priest Training (offical2) | bc1menu76, bc2bridging76, bc3acid_bar76, bc4bow_rush76, bc5walkers76, bc6mows76, bc7sun_bars76, bc8fury76, bc9ssun_bars76 |
| 캠페인 1장 (offical3) | TheWarBegins, MasterOfWhirligigs, SaveTheIsland, FragileFortune, ThunderingPower, DissolvedAlliance |
| 캠페인 2장 (offical4) | TheNoose, RainVsRain, RunForIt, GuardMyBack, Surrounded |
| 캠페인 3장 (offical5) | BreakingThrough, ToTheRescue, Vicious, EnemyTerritory, FinalConfrontation |
| 데모 | DemoStormRages (The Storm Rages), GuardMyBack (Demo of "Guard My Back"), DissolvedAlliance (Demo of "Dissolved Alliance") |

일반 31경로(Early 6 + Training 9 + 캠페인 16)와 데모 3개를 합쳐 34경로다. 같은 자원을 쓰는 GuardMyBack·DissolvedAlliance도 일반/데모를 따로 검증한다([대응 근거](docs/gameplay/demo-campaign-map.md)).

<a id="agents-reflection-20261102"></a>

### 1.3 AGENTS.md 요구 반영 현황 (2026-11-03 정리)

AGENTS.md는 수정하지 않는다. 단계별 요구는 1.1절이 기준이며, 즉시 적용 요구의 cpppj 상태는 다음과 같다. DN/WB의 구현 완료 여부는 각 프로젝트 인계에서 확인한다.

| 요구 | cpppj 상태·근거 |
| --- | --- |
| Windows 실행 파일 `Netstorm.exe` | **완료(580·581).** CMake 대상명 NetstormCpp와 import library 이름은 유지·분리한다 |
| 11.01 표기·Credits 세 번째 항목 | **완료(685).** `Netstorm 11.01 Patch Credits`, 창 제목 `NetStorm 11.01 Patch Credits`, HJOW 개발·AI 사용 안내, 기존과 같은 Back/Cancel. [기록](docs/exe/cpp-product-1101-20261101.md) |
| 외부 도메인/IP 접속 문서화·클라이언트 유효성 검사 비활성화 | **완료(684).** [문서](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json). 구 서버 검증/security.dat writer·검증 getter·Not Validated 안내는 연결하지 않고 성공1도 합성하지 않는다. 사용자 선택 Help 링크와 로컬 LAN은 별도다 |
| Windows 10/11 AMD64·Windows 11 ARM 고려 | 현재 EXE AMD64(PE0x8664). ARM 빌드/실기 미검증이며 Windows 11 실기는 단계 전환 조건이 아니다 |
| 4차 로컬 MCP·1920×1080, 5차 모든 자동 생성 파일/reader 이관·무제한 목록, 6차 한국어, 7차 AA/FSR1.0·고해상도, 8차 시나리오 UI | **구현 미착수.** 기존 화면비·BMP·원문 언어 처리와 구별한다. 5차는733의 [12파일군/50정적후보](docs/exe/cpp-phase5-file-inventory-20261102.md) 조사만 완료 |
| 한국어·UTF-8·주석·용어 | 응답·새 코드 주석은 한국어, 새 문서/코드는 UTF-8. 한국어 글꼴은 `fonts/`의 D2Coding. 새 네임스페이스·상수·함수·구조체·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다. 명칭은 AGENTS.md의 번역/발음 표기를 따른다 |

## 2. 현재까지 완료한 범위와 검증 한계

### 2.1 완료 목록 — 부분 완료 범위를 명시

`[x]`는 아래에 적힌 범위의 완료다. 전체 기능·전체 동일성 완료를 뜻하지 않는다. 오래된 “다음/미연결” 대신 후속 단위의 실제 연결 근거를 따른다. 단위별 검사 수·실패/교정·당시 해시는 [정리 전 2.1절](LEFT_JOBS.old.md#archive-20261103-through-808)과 링크한 단위 문서에서 확인한다.

- [x] **시작·메인 메뉴:** 무인자 GUI·EXE 옆 `game/` 우선·cwd 독립(637), 기본 8/Intro 조건부 9버튼·Help/Options 팝업·실제 부모/모달/HTML 수명·45초 autoDemo(697~700), Credits 자동 전환(566), Help/Technical Help(603). [메인 부모](docs/exe/cpp-main-menu-parent-20261102.md), [휴대 실행](docs/exe/cpp-portable-startup-20261031.md)
- [x] **게임 UI·단축키:** QWASZX·독립 미니맵·Player List·Send Money, Shift 다중 선택/30칸 그룹 이동(701), N/Shift+N(702), P/R(703), H/F4/F5/Ctrl+F5(704), C/c(717), Tab 최근 5칸(718), 숫자/Shift 저장 10칸(719), U/u 손실 위치(751), F6 정지 경계(741), ESC 선택 정리/취소(742), 현재 demo 관문(743), 전체화면 F12(744), Pause(753), T/t(754), D/d(756), outside/모달/정지 분배(761). [선택 키](docs/exe/cpp-selection-key-input-20261102.md), [저장 카메라](docs/exe/cpp-saved-camera-key-20261102.md), [입력 분배](docs/exe/cpp-outside-input-routing-20261103.md). 앱 내 입력·집중 검사이며 물리 키 전체 검증은 아니다.
- [x] **객체 메뉴·편집 입력:** 83VT의 작성 13/비멀티 열기 5/특수 명령 9 연결(681), CHEAT 편집 전용(725), 실제 owner 변경/월드 갱신·Ctrl 다리 순환·모드1 취득/우클릭 취소·가상 ac/70·성공 음향 부모(747~750). [연결 표](docs/exe/cpp-object-menu-wiring-20261101.md), [편집 입력](docs/exe/cpp-editor-primary-selection-20261102.md), [메뉴 부모](docs/exe/cpp-object-menu-open-parent-20261102.md). 나머지 타입/상태별 명령 효과는 남아 있다.
- [x] **오벨리스크/스펠 전환 실제 진단 복귀(812):** 현재 Client 서비스와 실제 Factory/frame DWORD/QA WORD 바인더를 연결했다. 원문128+36행·집중6개(새3) 통과이며 공간/상위 입력은 명시 장치다. [812 기록](docs/exe/cpp-object-conversion-diagnostic-20261103.md). 전체 상태별 효과/GUI 완료와 구별한다.
- [x] **기본/Factory 레벨의 실제 진단 복귀(813):** 두 Session 연결점을 현재 Client 서비스로 바꾸고 원문 BYTE 저장→Factory 표시 순서를 유지했다. 양판본 원문96행·집중4개(새1) 최종 통과. 최초 검사 입력 오류1건 교정·제품/fixture 불변. [813 기록](docs/exe/cpp-squid-level-diagnostic-20261103.md). 전체 상태별 메뉴/GUI 완료는 아니다.
- [x] **ContentForm 부착 부모(814):** 현재 전투/checking·앞단 진단 복귀와 같은 실제 사제 참여/이름/음향/Tell·Client 기한을 연결했다. 일반 부착/기한/너겟 집중4개(새3) 통과. 포획 안내 본체와 실제 포획 재현은 미실행이며 원문/어셈블리·정적 연결/빌드 근거다. [814 기록](docs/exe/cpp-content-attach-parent-20261103.md).
- [x] **Graph 참조 감소/SID0 조회(815):** 현재 checking·진단 복귀 뒤 같은 행/후속 후보를 실제 영역 통지와 일반/지형/gameplay 앵커에 연결했다. 새 원문760입력/재관측760 일치·집중7개(새4) 최종 통과. 최초 검사 풀 입력 오류3건 교정·제품/fixture 불변. Graph5중2완료이며 나머지3은 잔여다. [815 기록](docs/exe/cpp-graph-reference-checking-20261103.md).
- [x] **Graph sentinel/실제 Free(816):** 현재 checking·두 번째 WORD 재조회와 진단 중 변경된 현재 행 쓰기를 연결했다. 새 원문1,080행·집중6개(새2) 통과, Release 빌드2회 종료0. CD 대응0045b560 교정. 나머지 중첩 caller는 미완료이며 checking 잔여27은 유지한다. [816 기록](docs/exe/cpp-graph-sentinel-checking-20261103.md).
- [x] **현재 표면 이웃 flags8(817):** y/x·모서리/중복·현재 지도/후보/보관 원천의 부품을 연결했다. 기존250대조·새32행과 집중6개(새2) 최종 통과. 최초4단언 실패는 경계 후보 raw spot 조회로 제품 수정·fixture 불변. Graph 실제 부모 연결은 다음이며 checking27은 유지한다. [817 기록](docs/exe/cpp-surface-neighbor-current-20261103.md).
- [x] **현재 Graph 부모/Client trace(818):** Add/Flood/Rebuild/Allocate·실제 postPop/postDestroy·World FullPool·현재 로그를 연결했다. 새 PE168행·C++집중34개(새3) 최종 통과, 실제 자산 Tutorial1 콘솔 초기 로드 종료0. 최초 입력/상태 관문/진단 listener 교정을 기록하고 기존 fixture 기대값을 유지했다. Add 완료이며 Detach 및 그 안의 Flood/sentinel·Region setter 잔여는 남는다. [818 기록](docs/exe/cpp-graph-live-checking-20261103.md).
- [x] **현재 Detach/영역 setter/시계(819):** 삭제 값 원천·일반 finder·현재 정책/Flood/sentinel·World 활성/분할 상태·현재 Client 시계를 연결했다. 새 PE440행·집중45개(새3) 실패0·최종 Release 경고/오류0이며 기존 fixture는 불변이다. sentinel/Flood 두 함수 완료로 checking24가 남는다. 최초 경고/필터·허용 범위 교정을 기록했다. [819 기록](docs/exe/cpp-graph-detach-live-20261103.md).
- [x] **Boardcoord 각도/방향 소비자(820):** 현재 CRT fmod/판본별 저장·진단 복귀를 네 방향 소비자와 썬 아처 종료각에 연결했다. 실제CPU1088행/재관측1088행·집중32개(새3) 최종 실패0. Unicorn112행 불일치와 최초 교정을 보존했다. 바람 caller 잔여라 checking24는 유지한다. [820 기록](docs/exe/cpp-board-angle-checking-20261104.md).
- [x] **Client 바람/현재 입자 owner(821):** 감쇠/공유 난수/각도·실제 Movie 가지·Storage0/1/0과 연소 현재 owner를 연결했다. 실제CPU1920행/재관측1920행·집중17개(새3)·현재owner의 기존 원문426행 통과. 각도 마지막caller 완료로 checking23이 남는다. 인접004180f0 주변음/날씨는822 완료이며004182c0 monitor 시간 알림은823 완료다. [821 기록](docs/exe/cpp-ambient-wind-20261104.md).
- [x] **주변음/날씨와 공유 State기한(822):** 현재 벽시각/120초·같은 난수·실제Client Wind 앞·음향뒤 현재 모달/flash·기존10초 재예약을 연결했다. 실제CPU1272행/재관측1272행·집중11개(새3) 최종 통과. 최초 미등록필터2건/교정은 기록에 보존했다. State19는MP잔여이며 monitorFormat은823 완료다. [822 기록](docs/exe/cpp-ambient-sound-20261104.md).
- [x] **시간 알림/임시 Chat 서식(823):** Wind 뒤180초·현재 guard/옵션/기한/이름·같은 Config/일반 제출·front 정상 복원을 연결했다. 실제CPU1176+12행/재관측·집중11개(새4) 통과. 최초 검사 컴파일 이름 충돌/Native 경고 교정은 기록에 보존했다. MP 실제 전송 검사는 하지 않았다. [823 기록](docs/exe/cpp-monitor-time-20261104.md).
- [x] **팔레트 Frame의 미도달 진단/현재 수명(824):** 실제 기계어로 진단 미도달을 증명하고 기존 Frame/Client Kernel·FrameTime/Form을 CPU1670행/재관측·집중10개(새2)로 대조했다. 최초 관측기 inline CALL/XMM 오류와4단언 실패는 원시 보존 후 관측기 ABI만 교정했다. checking22이며CD 미초기화stack 차이는 별도다. [824 기록](docs/exe/cpp-palette-frame-checking-20261104.md).
- [x] **연소 갱신 진단/현재 회복 소비자(825):** 실제CPU1100행/진단216·재관측과 집중8개(새4) 최종 실패0으로 저장 HP/첫 검색·현재 TYPE/위치음·실제 Client/Session Form 생성/회복 삭제를 대조했다. 최초 검사 컴파일/owner 조립 실패는 보존 후 교정했으며 checking21이다. [825 기록](docs/exe/cpp-damageable-burning-checking-20261104.md).
- [x] **마나 공급원 등록/현재 카운터(826):** CPU1084행/진단135·재관측·집중16개(새6) 실패0으로 중복 진단/저장 판단·signed 용량/전체 DWORD·실제 Client/두World/Session 공통postPop·현재TYPE/owner 통계를 대조했다. CD 첫 검색/do조건 순서도 별도 검사했다. checking20이며 재접속004a7510은MP 미완료로 이월한다. [826 기록](docs/exe/cpp-mana-registration-checking-20261104.md).
- [x] **군도 첫 시드 조회 범위 정정(827):** 첫 영역이라는 오기를 raw+5/record[3] 시드로 고치고 두caller가 군도 분기임을 정적으로 확인했다. 제품/빌드/검사0이며 함수 복원 완료는 아니다. 군도 상위와 함께 남겨 checking20을 유지했다. [827 기록](docs/exe/cpp-terrain-first-seed-scope-20261104.md).
- [x] **제단 해방 부모/현재 권한(828):**CPU 부모1596행/진단468·재관측·집중7개(새3) 실패0으로 현재 checking·저장 finder·진단 복귀/현재 범위·plain P/가상 공간과 실제 Client caller를 대조했다. 하위 세 진단/현재 권한은 정적 연결·빌드이며 실제 포획/희생/해방 사제 몸체는 실행하지 않았다. checking19이다. [828 기록](docs/exe/cpp-altar-release-parent-20261104.md).
- [x] **숫자 송금 확인/현재 대상 수명(829):**CPU 비멀티432행/진단112·재관측·집중9개(새4) 최종 실패0으로 현재 checking/진단·현재 대상 재조회/저장·실제 모달/Client SP·AI 지갑을 연결했다. 최초 관측기/검사 준비 실패를 보존·교정했고 checking18이다. 신규MP/공격/포획/희생/승리 검사는 하지 않았다. [829 기록](docs/exe/cpp-money-confirm-checking-20261104.md).
- [x] **UserInput 고정 송금 checking/원문 위치(830):**세 checking을 고정1000/700/300 AI 가지로 정정하고 실제 Userinput.cpp 세 줄/현재 Client 서비스·진단 복귀를 연결했다. 기존 AI312행/숫자CPU432행의 C++ 대조·집중5개(새1/Session 확장) 최종 실패0이다. checking17이며 전체 입력/다른7진단·Console UI 완료가 아니다. [830 기록](docs/exe/cpp-userinput-money-checking-20261104.md).
- [x] **재접속 State caller 범위 정리(831):**004b8cb0의 ForceServerDeath/StateReconnect 인자0/1·자동 서버 장애 caller와 실제 이름표를 정적으로 확인했다.8번MP에 미완료 이월하며 checking17/제품830을 유지했다. 새 빌드/기능·MP 검사0이다. [831 기록](docs/exe/cpp-state-reconnect-scope-20261104.md).
- [x] **공통 Halt 진단/현재 수명 복귀(832):**CPU2288행/진단258·재관측·집중8개(새3) 실패0으로 현재 checking·삭제 뒤 진단 정상 복귀·같은 Client/Host와 기존 실제 재지정을 연결했다. !deadFlag는 정적 미도달/CPU0행이며 원문stack 주소 자체는 별도 호스트 경계다. 최초 관측기 thiscall ABI 교정을 기록했고 checking16이다. [832 기록](docs/exe/cpp-path-halt-checking-20261104.md).
- [x] **공통 도착/항상참 SID 진단 복귀(833):**CPU860행/진단462·재관측·집중12개(새4) 최종 실패0으로 현재 checking·원문0041be80의 항상1/범위 진단·같은 실제 경로/Host 삭제 뒤 현재 metadata 겹침 정리를 연결했다. isRobust false4곳은 정적 미도달/CPU0회다. 최초 검사 조립1실패는 검사 입력만 고쳤고 checking15다. [833 기록](docs/exe/cpp-path-arrival-checking-20261104.md).
- [x] **Unpop 일반 공간/현재 진단의 부분 연결(834):**CPU1300행/진단670·재관측·집중10개(새3) 실패0으로 저장 TYPE/spot·현재 checking/TYPE·void→해시→표시와 같은 Client/World/Session을 연결했다. 기존3618단계/VT151도 통과했다. contained60행은 두 요청의 외부 관측이며 실제 종단 미연결이라 부모 전체는 미완료/checking15 유지다. 사용자 요청으로 이 단위 문서·커밋 뒤 목표를 일시 정지한다. [834 기록](docs/exe/cpp-squid-unpop-checking-20261104.md).
- [x] **Edit 대표 수명·저장:** 기존/새 Fort·섬 추가·전체 저장/Save as·이름/목록, 시나리오 시작값·방향별 동맹(590·597), 실제 Objects 스트림·손상 파일 정리(663~665), Early1 중단→Edit→시험 전투/Restart/복귀·Cancel/No(716), 같은 워크샾 owner0/2/8·Yes 저장/재적재(726). [Edit 수명](docs/exe/cpp-mission-editor-lifetime-20261102.md), [소유자 저장](docs/exe/cpp-editor-owner-save-20261102.md). 전체 도구/분기/실패를 완료한 것은 아니다.
- [x] **Edit Graph·지형 캐시:** 재적재 소속(721), 같은 후보 배치 직후 장부678/실제670·noGraph4(723), 두 섬/다리 연결·owner 변경·절단의 성분2→1→2와 원본 전체 출력(724), 지면99cache Client 수명(722). [대표 Graph](docs/exe/cpp-editor-graph-world-20261102.md). 원본에도 있는 장부 차이8은 정규화하지 않는다.
- [x] **시나리오 명령 연결:** 객체/송금·연기/번개·AddRandom/DestroyAt·기술 지급·음향/ViewSpot·HTML Input·기술 목록(676·682·683·686~690·705~708), 명시549문서/43이름·Menu 둘째 인자/큐 조사(715). [이름/메뉴 조사](docs/exe/cpp-dump-squids-20261102.md). 명시 목록 일치를 동적/TARC/Config 실행 전체 완료로 세지 않는다. Zsend/비미션 의식 요청(707·709)은 MP 정적 연결만 했다.
- [x] **공통 월드·AI·유닛의 복원 범위:** 공통 전략 AI의 현재 시각→비정지 AI→Kernel(320~322), 워크샾 25종 생성/이동/공격/표시(335~345), 의식·로컬/비로컬 보상·프레임말 통지(598·599·604·642), 세계 간헐천(609), 투사체/다리/펜스의 명시 입력 대조(552·611~613). [25종 점검](docs/exe/cpp-unit-restoration-20261022.md), [의식 검증](docs/exe/cpp-ritual-runtime-focused-20261021.md). 자연 플레이·공중 3종 새 전용 검사를 뜻하지 않는다.
- [x] **Client/Session·Guide·State의 연결 범위:** 장기 전역/Pool/Graph/Player·권한·커서·cleanup(425~486), 정상 State7 cleanup(1)/메인 cleanup(0)·Restart/Leave(638), 미션 진입 UI·7/8 확정(655~660), Guide 반납(671), 현재 Guide Fort 요청(713)·실제 Early1의 State1→7→8→14(714). [State 반복](docs/exe/cpp-state-repeat-20261031.md), [Guide 요청 실기](docs/exe/cpp-guide-fort-state-20261102.md). 다른 State/비동기 분기는 남는다.
- [x] **Renderer·카메라·HUD:** 실제 Frame·단일 표시 목록·구름/배율/부분 갱신·마나103 추가 Draw, 도킹/미니맵 내용·grid/debug 직접 writer 조사(618~639·648), ViewSpot·pan/Home/viewport(615·617·632·633·635·676), 부착 SP 투영(745), DrawWater(768), Hash/Fire/Spot/Board 진단과 BFS 배열 수명(776). [Renderer](docs/exe/cpp-renderer-frame-20261030.md), [진단 표시](docs/exe/cpp-renderer-diagnostics-20261103.md). 활성 caller 없는 grid/debug·관리 메뉴는 기본 UI에 노출하지 않는다.
- [x] **Chat·원문 언어:** 입력/자동완성/전달·공유 로그·기본 구분선 드래그/숨김/복원·설정 재실행(543·574·586·614·640), 일반 dot 명령/도킹 연결(desc는 MP 잔여), 언어 번호·다중 Xlat/요청·Chat 알림(710~712). [Chat 수명](docs/exe/cpp-ui-lifetime-current-20261031.md), [언어 요청](docs/exe/cpp-language-request-20261102.md). 전체 Chat 모드/번역 UI·한국어는 미완료다.
- [x] **파일 경로·시작·BaseFile:** 공유 256BYTE/10칸 이름 고리와 Client·VFS/저장/Options/Capture/CD/Help/Config(727~732), 디렉터리/모듈·6후보 setup/가상CWD/아카이브 등록/반납(734~737), Edit 조회/저장/삭제·직접 chmod 경계(738·740), MatchDisk/FileSpec(752), 전체 열기 부모·키/TAFF/current FILE·객체 덤프(755·757·763). [경로 고리](docs/exe/cpp-file-name-formatter-20261102.md), [BaseFile](docs/exe/cpp-basefile-open-policy-20261103.md), [Edit 쓰기](docs/exe/cpp-editor-mutation-ring-20261102.md). 손상/부분 I/O·다른 소비자·32비트 ABI는 남는다.
- [x] **파일 reader·자산 수명:** 존재조회/세 소비자(765), Help HTML stream writer(766), QuickHelp(767), DataManager/Template(769), Config(771), TYPE(777), GIF/타이틀 State42(781·782), 보조 효과음 현재 root(785), 팔레트(786), Xlat(787), 글꼴 캐시 순수부모→실제 FontStore(788→789), SHP 매핑부모→실제 공유 자산(790→791). [직접 74caller 분류](docs/exe/cpp-secondary-sound-root-20261103.md), [실제 SHP](docs/exe/cpp-native-shape-file-20261103.md). Script 캐시 직접 caller는792에서 CD/호스트의 비활성 10.78 경로로 분류했으며 기존 snapshot/캐시 정책을 유지한다.
- [x] **일반 명령·글꼴/음향 설정:** press 후처리(784), CreateAt/AI 직접 명령/MakeAi/조건부 AI 메뉴(770·772·773·775), Config/우선순위 명령(762), 글꼴·음질/글리프 캐시(764), raw ACP 31BYTE↔UTF-8 Config/디스크 경계(774). [일반 후처리](docs/exe/cpp-recent-command-tail-20261103.md), [글꼴 이름](docs/exe/cpp-font-face-encoding-20261103.md). MakeAi의 owner0 이전/삭제·전략·실제 설치 글꼴/다른 ACP/구형 무BOM 설정은 별도 한계다.
- [x] **checking 공유 owner와 후속 33부모:** 779의 기존 41관문 공유와 별도로, 접근점(783)·다리 낙하(793)/붕괴(795)·Gump 표시/소멸(796)·연소 3소비자(797→798)·건설 Form(800)·커서 취소(801)·펜스 2타입(802)·Gunanim(803)·Squid 표시(804→805)·Gump 부착(806)·Gun 통지(808)·네 Gun PostPop(809)·CanonDecoder(810→811)·Graph 참조 감소/SID0조회(815)·Graph Add(818)·Graph sentinel/Flood의 마지막 Detach 소비자(819)·Boardcoord각도 세caller(820~821)를 실제 소비자에 연결하고 PaletteFlash 미도달 진단/현재 수명(824)·연소 갱신/회복 소비자(825)·마나 공급원 등록/카운터(826)·제단 해방 부모/현재 권한(828)·공통 Halt 진단/수명(832)·공통 도착/항상참 SID 진단(833)을 대조했다. [함수별 색인](docs/exe/cpp-checking-consumer-gaps-20261103.md), [808](docs/exe/cpp-gun-notify-checking-20261103.md). 순수부모 준비본을 실제 연결 완료로 중복 계산하지 않는다. 807의 Gun/바리케이드 서버 관문 4곳 owner 분리와 809의 네 PostPop 진단 위치·복귀까지 완료다.
- [x] **Windows 10 화면 전환의 확인 범위:** DIB 호환 전체화면·가장자리 이동·세 화면비·저장/종료/새 프로세스 적용, F12(744), 화면 변경/불법 요청 복구·DD 마우스 조정/취소/확인/15초 만료·저장 재시작(746). [화면 전환](docs/exe/cpp-screen-mode-transition-20261102.md). 다른 장치/DPI·물리 입력/출력은 미검증이다.
- [x] **배포 스크립트·빌드 선택:** `cpppj/package-cpppj.ps1`의 인자/대화형 경로·빈 값 취소·실행 파일/자산 동봉(562), 최상위 EXE 실행(637), Game/Tests/All 선택/기본 테스트 OFF(739). [빌드 모드](docs/exe/cpp-build-modes-20261102.md). 현재 제품의 새 배포 ZIP/전체 회귀는 아직 없다.
- [x] **사용자 제보 수정:** 덱 재활성 지연은 원문 Fast 1초로 복원(677), 간헐천 그림자는 원문 대상 변환/색표로 수정(678). [덱](docs/exe/cpp-deck-reactivation-20261101.md), [그림자](docs/exe/cpp-translated-shadow-20261101.md). 사용자의 자연 플레이 재확인은 별도다.
- [x] **설정 매핑 저장 실패 대응:** 제품 674의 오류1224·백업/교체/지연 정리, 도구 675의 복구를 완료했다. [제품](docs/exe/cpp-options-mapped-save-20261101.md), [도구](docs/exe/cpp-tool-settings-writer-20261101.md). 과거 개별 원인 프로세스·1176/1177·부분 I/O와 전체 audio 재실행은 미확인이다.
- [x] **11.01/Credits·외부 검증 비활성화:** 684·685 적용 완료. 1.3절의 문구·처리 경계를 따른다.

- [x] **공통 contained 종단/실제 입력큐(835):**004ac6b0→004ad230의 실제 혼합링크·부모통지·UpdateContents39 예약과 World/Session weak대여를 연결했다. 집중19개(새11) 최종실패0·빌드2회 종료0·새CPU/fixture0·보호2759개 불변. Take 상위가 남아 checking15유지다. [835 기록](docs/exe/cpp-squid-contained-unpop-20261104.md).

- [x] **실제 Take Unpop/현재 TYPE·PostTake(836):**PATCH의모든 !free/void진단호출과현재state·constructor→TYPE→void·zOrder/후단진단복귀를같은World/Session에연결했다. 집중35개(새8) 첫실행실패0·빌드1회종료0·기존fixture불변으로004afe50완료/checking14다. Factory자체진단/CD별도경계는남는다. [836 기록](docs/exe/cpp-factory-take-unpop-20261104.md).

- [x] **객체메뉴직접가상80의현재VT분배(837):**TYPE생성자VT쌍강제를제거해실제Summary/회수에서현재지원VT를선택한다. 새3/집중6최종실패0·빌드2회종료0·보호불변이다. 제단의중첩가상80과다른Session소비자는838잔여다. [837 기록](docs/exe/cpp-object-count-vtable-20261104.md).

- [x] **제단중첩현재가상80/Session공유(838):**typedDais현재VT분배·명시virtual80소비자공유·activeSID RAII를연결했다. 집중8개(새2) 첫실행실패0·빌드1회종료0·보호불변이다. getter의free/TYPE165경계와Stage등내부호출은별도잔여다. [838 기록](docs/exe/cpp-nested-count-vtable-20261104.md).

- [x] **가상80getter읽기정의역(839):**Has의현재Slot/free·예약SID읽기와DaisCount의현재TYPE/frame범위를원문으로교정했다. Level3584/Count1920명시입력/새3·집중13첫실행실패0·기존원문96행불변·빌드2회종료0이다. setter쓰기·Stage등다른부모는별도잔여다. [839 기록](docs/exe/cpp-current-count-getter-20261104.md).

- [x] **다이스 checking/내부 현재 가상Count(840):**선행 내용물finder·진단 복귀 뒤 현재TYPE/owner/kind/동맹·권한과 NULL Path/저장depth·Stage/확인된 내부가상80을 연결했다. 새6/집중17 첫 실행 실패0·최초C2248 보존/수정 후 최종빌드 종료0·보호2759개 불변이다. 두 부모 완료로checking12이며 문서·커밋 뒤 사용자 요청으로 일시 정지한다. [840 기록](docs/exe/cpp-dais-checking-20261104.md).

- [x] **Take SID/TYPE 진단과 실제 상세 owner(841):**원문1a7·상세00436d20·문자열0/1ba 정상 복귀와 현재 Unpop/생성자·Client/World2대여·비멀티 Session을 연결했다. 집중27개(새8) 첫 실행 실패0·Release2회 종료0·보호2759개 불변이다. Factory 전체/CD·Form 등은 잔여다. [841 기록](docs/exe/cpp-factory-take-diagnostic-20261104.md).

- [x] **공통 Pop 현재 조회/부분 효과(842, 부분 완료):**현재TYPE·저장입력범위·spot/grid진단복귀·늦은SHP·해시/현재firstPop을같은World/Session/Frame에연결했다. 집중40개(새15)최종실패0·보호불변이다. Activate 서버/깊이 등은 남아 전체완료/checking감소로 세지 않는다. [842 기록](docs/exe/cpp-squid-pop-parent-20261104.md).

- [x] **Activate/현재writer/같은전송물리(843):**현재서버·SID별flags·전송전후상태·같은깊이/392정상복귀와83VT의실제35BYTE/압축writer를World/Session에연결했다. 집중51개(새11)첫실행실패0·Release제품2/집중대상1회종료0·보호불변이다. MP/패킷전용검사0이며전체Pop/checking완료로세지않는다. [843 기록](docs/exe/cpp-squid-activate-20261104.md).

### 2.2 미션별 확보한 증거

모든 행은 **과거 해당 제품의 증거**다. 현재 제품의 자연 완주 회귀로 합산하지 않는다. 전체 34 최신 기준은645/644 제품이다.

| 대상 | 확보한 증거 | 남은 사용자 확인 |
| --- | --- | --- |
| Early1 | 북서 간헐천 A~G·채취/귀환·SP0→600·Tutorial2 진입 | 남동 경로 실패 원인(417의 D/SP0에서 Next Tutorial 미발견), 실패/재시도/F8·화면/음향 |
| Early2 | A~I·신전/워크샾/지식/생산/발사기/회수·환불75·Tutorial3 진입. 마지막 자연 완주 회귀는467 제품 | 재시도/취소·화면/음향 |
| Early3~6 | 진입/Start·단계 판정·짧은 진행. Early3의468 Source1은 채취/귀환·SP2000→2400·A→C, Early5는 과거F까지 | 각 자연 완주/다음 미션. Early3 최종468 제품 완주 기록은 없고 attempt4 종료1은 보존 |
| 캠페인1-1 | 322의 자동 AI 적 신전 HP5000→0, 2026-10-28 사용자 완주·다음 안내 확인, 638 Restart/Leave | 현재 제품 완주·패배/Retry·보호막/포로색. 사용자 실행 PC/커밋/SHA 미제공 |
| 캠페인1-2 | 321의400프레임·645 진입. **2026-11-01 사용자 완주·적 사제 포획/희생·Next Missions로1-3 진입** | 현재 제품 회귀·패배/Retry. 제보 2건은677/678 수정 완료이며 사용자 재확인은 별도 |
| 캠페인1-3~1-6 | 진입/Start·321/645 진행, 575의1-6 무입력 신전 파괴 후 섬 중립/테마 복귀. 1-3 사용자 진입 | 자연 플레이·승패·후속 명령 |
| 캠페인2·3장 | 645에서 진입/Start·비정지 시각·창 출력·종료0 | 추가 TYPE 전체 수명·자연 플레이 |
| Priest Training 9개 | 645에서9/9 진입·비정지 진행·창 출력·종료0 | 자연 완주·완료 안내/복귀·bc8 전체 원소 선택지 |
| 데모 3개 | 645 실제 메뉴 진입. 567/569 첫 360초 만료→재시작→Exit Demo→Quit | 승패·자연 완주. 두 번째 360초 반복 검사는 3절 3번의 별도 잔여 |

### 2.3 음향·영상·캡처의 확인 범위

현재 위치 음향 조회/장치 순서(672), 곡 만료의 현재 battle/정리 순서(673), MovieGump/스킵·끝·CD 경로/캐시(594·595·654·655·670), 연속8비트BMP 수집(693~695)은 연결했다. [위치 음향](docs/exe/cpp-sound-current-view-20261101.md), [영상 owner](docs/exe/cpp-movie-override-20261101.md), [화면 수집](docs/exe/cpp-capture-client-20261102.md)을 따른다.

실제 PCM/청취·CD 삽입·활성 전체화면 물리 출력, 전체 영상 동일성, 수집 중 모드 변경/1000프레임 GUI/반복 주기·소리 녹화는 미검증이다. 672 전체audio 실패 원시는 `live/audio-v2`에 보존하며 기능별 후속 성공으로 지우지 않는다. 기존 BMP 수집은4차 MCP·소리 포함 녹화 완료가 아니다.

### 2.4 현재 이슈의 처리 위치

| 항목 | 현재 처리 |
| --- | --- |
| 물리 캡처/입력 | 환경 대기.762의 FrameArrived/window capture timeout 뒤 실제 환경 변화 전 반복 금지(3절7번) |
| Early1 남동 경로 | 자연 플레이에서의 미확정 실패. 사용자 확인/제보 후 후속 작업(3절6번) |
| 옵션 저장 | 1224 대응은 완료. 공유 삭제 거부 시 이전 파일/changed/복구 파일을 보존하고 실패 안내. 외부 매핑이 남으면 성공 백업/교체 폴더도 남을 수 있음. 부분 I/O·1176/1177 잔여(3절4번) |
| PlayerNoMake | 경고255DWORD/정리 `004328a0` 연결은453 완료. 수신 생성/검증 패킷 부모 `00433c30` writer는 후순위 MP(3절8번). CD `00487900`은 spot 정리이며 같은 함수가 아님 |
| Graph/글꼴/사용자 제보 | 대표 Graph와 캐시 수명은721~724, 비ASCII 글꼴 경계는774, 제보 2건은677/678 완료. 2.1절에 명시한 나머지 레이아웃/글꼴/플레이 한계만 남김 |

- [x] **Factory 현재 분배/선택 장치(847):**선택 예약/생성·프레임·경고/삭제의 부분 효과와 두 현재 가상80→TYPE/저장 프레임·같은 실제 Session을 연결했다. 집중77개(새13) 첫 실패0·빌드1회 종료0·보호2759개 불변이다. Factory 타입번호 전역 공유/다른 파생·전체Pop은 남아 checking12를 유지한다. [847 기록](docs/exe/cpp-factory-current-parent-20261104.md).

- [x] **Factory 현재 TYPE 전역(848):**세 전체 DWORD의 장기 owner·현재 예약 비교/프레임 제외와 같은 Session/기존 소비자를 연결했다. 집중85개(새8) 첫 실패0·빌드1회 종료0·보호2759개 불변이며 두 Session 실제 예약 Form을 확인했다. 다른 파생/TYPE 소비자·전체Pop은 남아 checking12 유지다. [848 기록](docs/exe/cpp-factory-type-globals-20261104.md).

- [x] **Residence/Geyser 현재 부모 연결(849):**선택 장치·부분 효과/물리 쓰기·signed 목록·실제 현재 finder·WORD 통지를 연결했다. 최종108개(새21) 첫 실패0·빌드2회 종료0·보호2759개 불변이다. 부속 NotifySurface의 TYPE/진단 복귀와 다른 파생·전체Pop은 남아 checking12 유지다. [849 기록](docs/exe/cpp-asset-current-parents-20261104.md).

- [x] **NotifySurface 현재 TYPE/진단 복귀(850):**같은 World/Session 현재 값과 실제 Client 진단을 연결했다. 원문2264행/재관측·집중119개(새9) 첫 실패0·빌드1회 종료0·기존2759개 불변/새fixture1개다. 기존51함수와 별도 부속 완료이며 checking12는 유지한다. [850 기록](docs/exe/cpp-building-notify-current-20261104.md).

- [x] **Gun·제네레이터·장식의 선택 장치/현재 부모(851):**실제 가상 진입의 도달한 장치·앞선 부분 효과·Sun WORD/현재 Bridge 통지와 같은 Session Graph 진단을 연결했다. 집중105개(새15) 첫 실패0·빌드1회 종료0·기존2760개 불변이다. 전체 Pop/다른 전역은 남고 checking12를 유지한다. [851 기록](docs/exe/cpp-asset-gun-current-20261104.md).

- [x] **바리케이드·두 펜스의 선택 후처리/직접 부모(852):**상위 실제 가상 진입의 일괄 Validate를 제거하고 선택 장치/부분 효과·직접 Damageable와 같은 Session 예약을 확인했다. 집중120개(새15) 첫 실패0·빌드1회 종료0·기존2760개 불변이며 checking12는 유지한다. [852 기록](docs/exe/cpp-asset-fence-current-20261104.md).

- [x] **성장 아이스·썬 바리케이드·너겟 현재 후처리(853):**선택 몸체/현재 물리 DWORD·프레임 읽기 순서와 같은 Session 예약을 연결했다. 집중138개(새15) 첫 실패0·빌드1회 종료0·기존2760개 불변이며 checking12는 유지한다. 명시 Validate/실제 본문을 분리했고 기존 guard 단언만 교정했다. [853 기록](docs/exe/cpp-asset-tail-current-20261104.md).

## 3. 앞으로 할 작업 (재개 후 권장 순서)

**현재는853완료 뒤854를 이어간다(0.24절). 아래 순서대로 잔여를 진행한다.**835~840의 contained/Take·현재 가상80/getter·Dais checking 두 부모 뒤 Factory 자체 진단을841에서 연결했다. 비멀티1~4번·Windows 10 호환성7번을 마친 뒤8번으로 간다.5·6번은 사용자 확인 목록이며 단계 전환을 막지 않는다. 아래 `[ ]`만 실제 잔여이고 완료한 하위 범위는2절이다.

### 1. 대상별 커서·오브젝트 우클릭·게임 UI

- [x] **현재가상80getter읽기정의역(839):**Has의free/예약SID 거부와DaisCount TYPE165 강제를 원문Slot/TYPE/frame 물리 범위로 교정했다. Stage의 현재 가상80은840에서 연결했으며 setter 자체의 나머지 쓰기/부모 계약은 별도다.

- [x] **현재가상80중첩/공유(838~840):**제단의typedDais 현재VT+80과 확인된Session 명시 소비자를 연결했다. getter 읽기 정의역/free/TYPE165는839 완료이며 Stage/EscapeFrame/Dais 내부의 확인된 가상80은840에서 연결했다. 효과 함수는 정적 대조·컴파일만 확인했다.

- [ ] 크리스탈/비석 특수 명령·제단/일반 건물의 나머지 타입·소유자·레벨·손상·건설/생산·포획/희생 상태별 메뉴·명령 효과·반납·선택기 잔여를 대조한다. 실제 포획/희생·공격 재현은 건너뛴다.
- [ ] 운반체 전체 종류와 나머지 커서 조합을 점검한다(536·576). 기본 Shift 그룹 선택·이동과 N/P/R/손실 위치 선택은 완료이며 공중 3종 새 전용 검사는 하지 않는다.
- [ ] ESC 하위/View·도킹 조합·DD 조정의 남은 표시·Chat 모드별 입력/표시와 전체 Console/모달 수명을 대조한다. 메인 메뉴의 전체 예외/할당 실패·CD 대응도 남는다. F9는 정지 앞, F6는 정지 뒤 예약이다. 이미 연결된 단축키·일반명령 후처리/글꼴/음질/파일 reader를 반복 완료 과제로 만들지 않는다.
- 활성 caller 없는 관리명령·grid/debug는 기본 UI에 임의 노출하지 않는다. 퇴역명령 `AdvanceRank/SpawnScreenChange/Stall/BridgeLikeCrazy/1/CreateRandomGuns/DumpProcessList/DumpSquids/DestoryAllPost`는 대조한 무효과를 유지한다. special `0055a480`은 현재값만 읽고 개발자키 `0055a484`를 켜지 않는다.
- 객체메뉴의 Dig Up/전환/다리/Meltdown/Detonate, 독립 미니맵/Send Money와 c2(Test Battle/편집 복귀)·c5(Leave)는 연결 완료다. RequestMenu 서버 전송·메타맵 열기2개·c6(Refresh Zones)/db(Declare a Draw)는8번이다.

### 2. Edit 메뉴·커스텀 맵/시나리오

- [ ] 다른 시나리오 조합·실제 설정/UI writer와 TARC/동적/Config 확장·남은 명령 부모를 대조한다. 687/715의 명시 목록·이름/큐 연결은 완료이며 모든 동적 실행의 증거가 아니다.
- [ ] 남은 지형·오브젝트·소유자 도구, 작업본 저장/재파싱·이름/목록·미션 전환의 다른 실패 수명, LeaveEditMode/State1/비동기 State7 분기를 점검한다. 716의 연속 왕복·726의 워크샾 owner 저장·724의 대표 Graph는 반복 미구현으로 세지 않는다.
- [ ] 원문 언어 UI/폰트 재생성·다른 언어 파일 실패 수명을 대조한다. 번호/Xlat/요청/기존 Chat 알림은 710~712·787 완료다. 590의 명시 `.french`는 프랑스어 UI 완료가 아니고 한국어는 6차다.
- 원문 목록 첫 50개 상한은 5차 전까지 유지한다(634). 실제 Fort 433개는 종류2가 432·종류0이 1개다. TEST01/TEST02 원본은 읽기만 하고 저장은 작업본에 한다. 10.78 기본 설명의 `{Text}` 리터럴(494)을10.72와 혼동하지 않는다.
- `.english` 부재 Test Battle 종료는 원본 복원 단계에서 유지하며 기본파일 자동 생성/시나리오 UI·부재 종료 개선은8차에서 한다.

### 3. 미션 수명·진행 저장·데모 반복

- [ ] Retry·Next Tutorial·Next Mission·Abort의 나머지 연속 전환과 참조/타이머·다리/AI 재고를 확인한다. 자연 승패가 필요한 검증은 사용자에게 남긴다.
- [ ] 세 데모의 재시작 뒤 두 번째 전체 360초 만료·추가 주기/정리·나머지 안내를 확인한다. 짧은 진입의 관례적 반복은 하지 않는다. GuardMyBack의 15초 안내와 DissolvedAlliance의 Demo 섹션 부재를 유지하며 Script demo 키만으로 모드를 추정하지 않는다.
- [ ] 비멀티 Native3의 남은 선택·State1/단독State7·전투 정리 `004b2df0`의 Tick/연속 전환, LeaveBattle 나머지 이유/CD·State6을 대조한다. State3 특이 분기는 도달 미입증/미연결이다.
- 정상 State7의 복사/로그·World 교체·7/8 확정(660), State1 공통 앞단(661), 현재 Guide 요청과 실제 Early1 전환(713·714)은 완료다. 일반 전투 저장/복구·StateReconnect·서버 이전·Zone0/메타 Poll·시계 패킷은 8번이며 기존 질문/삭제 UI를 유지한다.

### 4. 내부 수명·파일·checking 실제 연결

**현재 연결 기준:**809~833의 실제 checking 소비자와835~836의 공통 Unpop/Take를 완료했다.840에서 CanBridge00449710/BridgeCommand00449900의 진단 복귀·같은 실제 내용물 finder/Host/현재 서비스와 부속 MoveAwayDepth를 연결했다. 잔여12=부모6+부분3+MP3이며 자세한 완료/미완료 범위는 [함수별 공백](docs/exe/cpp-checking-consumer-gaps-20261103.md)과2절을 따른다.

- [ ] **checking 잔여12함수**를 [함수별 공백](docs/exe/cpp-checking-consumer-gaps-20261103.md)에서 이어 간다. 부모6/부분3/MP분류3이며779 당시51개 중 완료부모36/완료부분3이다.840의00449710/00449900과836의004afe50은 완료다. 부모004a7510은8번MP 미완료 이월이다. 남은 부분은0048fb30/004b54e0/004b8cb0이며 뒤 두 함수도8번MP로 이월한다. MP분류3(0042bc10/004b4df0/004b8b20)도8번이다.
- [x] **공통 Unpop 실제 종단/Take 상위(835~836):**004ac6b0→004ad230의실제종속링크/부모통지/현재GumpForm/입력큐와Factory::Take의void포함실제호출/현재TYPE·후단을연결했다.004afe50 checking부모완료/잔여14다.834 CPU contained60행자체는여전히외부요청관측이며835~836의실제부품연결검사와구별한다.
- [x] **부속 NotifySurface(850):**004214a0의 현재 TYPE/checking·진단 정상 복귀/자체 연결 반납과 같은 실제 World/Session·Client 장치를 연결했다. 원문2264행/재관측·집중119개(새9) 첫 실패0이다. CD의 버려지는 포인터 계산은 진단/raw 미접근을 유지했다. 기존51함수와 별도 부속 완료여서 checking12는 유지한다. [850 기록](docs/exe/cpp-building-notify-current-20261104.md).
- [ ] **공통 Pop 현재파생 나머지:**842~852의 기존 부모 뒤853에서 성장 아이스·썬 바리케이드·너겟의 도달한 몸체/현재 물리 쓰기·프레임 읽기 순서와 같은 Session 예약을 연결했다. Vortex와 다른 현재 가상20/전체 Pop·TYPE/모드 전역은854부터 남는다. 공중3종은 정적/코드만 하며 전용 검사를 하지 않는다. checking12는 유지한다. [853 기록](docs/exe/cpp-asset-tail-current-20261104.md).
- [ ] **군도 첫 시드/상위:**004c0e20/CD0049de40은 raw+5/record[3] 시드이며 두caller는 군도 분기다(827). 흩뿌리기004c13f0·밀도/성장/플레이어 배치·실제 상위/root와 함께 복원한다. 현재 일반 성장에 검사 호출을 임의 추가하지 않는다.
- [x] **Factory Take10.78의SID/TYPE진단(841):**1a7·상세복사/문자열0·1ba 정상 복귀와 현재 Unpop/생성자·같은 Client/World/Session 상세 owner를 연결했다. 집중27개(새8) 첫 실행 실패0·빌드2회 종료0·보호 불변이다. [841 기록](docs/exe/cpp-factory-take-diagnostic-20261104.md).
- [ ] Factory의CD Take/contained/Form과Create/Construct/PostCreate 등 나머지 부모/쓰기·진단 경계를 대조한다.836의Unpop 및841의진단 연결을Factory전체 완료로 확대하지 않는다.
- [ ] 비멀티 파생 프로세스·State/Session의 남은 수명과 전체 팔레트 전환·Chat 모드별 표시·시스템뉴스 UI를 연결한다. 원격/메타맵·desc는8번이다.
- [ ] 경로 고리/FileSpec·BaseFile의 **분류만 했거나 아직 연결하지 않은 소비자**, 최초 FILE/리소스 I/O·다른 쓰기/키/스트림·TAFF/보조 폴백·손상/할당/부분 I/O 실패 수명을 대조한다. 이미 실제 연결한 reader/FontStore/SHP·Edit 쓰기는 2.1절을 따른다. [785의 직접 74caller 분류](docs/exe/cpp-secondary-sound-root-20261103.md)는 모든 부모 구현 완료가 아니다. 등록 snapshot/무진단 optional·OS 직접 ReadFileBytes 계약을 보존한다.
- [ ] options.cfg의 실제 부분 I/O·1176/1177 실패 경계, 원문 FILE/FileSpec/SEH와 호스트 소유 경계의 남은 차이를 판정한다. `0044fa00`의 재접속 저장 소비자는 `004b54e0`과 함께8번이다. CD 일반 열기는 `004ba5d0`, 부분 파일은 `004ba630`이며 자동 정밀 후보의 반대 대응을 따르지 않는다(652). 실제 UNC 서버/다른 드라이브 상대CWD 등 미관측 경계도 보존한다.
- 원본32비트 스택/미정값 자체 일치(612)·원문 범위 밖 메모리·전체SEH/진단UI는 호스트 한계와 별도로 판단한다. guard 제거·임의 배열 확대·손상 후보 무조건 무시·진단6 강제·임시 속도로 오류를 숨기지 않는다.
- 비미션 전체 랭크 상승·조기 희생 안내(missionPost0,615)·연결 활동/리슨/수락/송신 writer와 일반 전투 saveBattle/metamap·비mission phase14 저장은 8번 후순위다.
- 내부 근거는 [장기 소유](docs/exe/cpp-state-world-owner-plan-20261025.md)·[State 공백](docs/exe/cpp-state-cleanup-client-gap-20261025.md)·[전역 반납](docs/exe/cpp-state-cleanup-global-owners-20261025.md)을 따른다. 오래된 PLAY 목록·ClearWorld 주석·428 문서만으로 후속 단위의 연결을 미구현으로 되돌리지 않는다.

### 5. 원본 전체 동일성 — 사용자 확인 대상

AI 미완료 체크 항목이 아니며 단계 전환을 막지 않는다. 부품을 구현할 때 원문과 대조하는 일과 사용자 제보 수정은 AI가 계속 맡는다.

- TYPE 전체 대기/건설/발사/피격/파괴/포획/희생 프레임·방향·색표·그림자·기준점·가림/파편/지형 효과, 캠페인2·3장 추가 TYPE 전체 수명
- 메뉴/브리핑·생산/지식·선택/사거리/비용·커서/회수/업그레이드·키·Pause/Help·카메라/View/도킹/Chat/PlayerList·Credits의 전체 조합/픽셀
- 건설/발사/적중/출격/귀환/연소/붕괴/의식/승패/음악의 파일·시점·반복·위치, 정지 중 시간/난수·정상 간격 AI, 장기 자원 누적/정지/크래시
- 녹화의 HDR 밝기·방송 음성은 게임 효과로 판단하지 않는다. 표시 변환 픽셀 일치와 전체 외관 동일성을 구별한다.

### 6. 자연 완주·자동 공략/테스트 지원 — 사용자 확인 또는 별도 요청 대상

모든 Early Missions·Training·캠페인·데모 자연 완주와 실제 승리→신규 완료값0→1 저장→재실행/다음 미션 해제는 사용자가 확인한다. 578의 명시 진행 키 저장과 기존 완료값1 보존을 자연 승리/신규 해제로 합산하지 않는다. 캠페인1-1/1-2 사용자 완주는2.2절의 당시 확인 범위다.

자동 Early3 후속·TEST02 로컬 AI/포획/희생 전략·좌표 준비 등은 별도 요청 전 진행하지 않는다. 옛 시도/계획은 [보관본](LEFT_JOBS.old.md#archive-20261103-through-808)에 있다. `CampaignForwardBattle_NaturalTempleAssault`의549 실패는 미해결이며 원래 함수/전략/승리 단언을 유지한다. TEST02 로컬 owner AI는 원문 누락이 아닌 테스트 지원 제안이다. 원문 `004c2b20`의 `owner!=myPlayerId || demo!=0` 조건을 유지하고 `gate.demo=1`로 우회하지 않는다. 별도 지원을 요청받으면 현재 원본 SHA/배치를 다시 확인하고 기본 게임/프로필과 분리하며 공유 난수 차이를 기록한다.

### 7. Windows 10/11 호환성과 전체화면

- [ ] 캡처가 정상화된 환경에서 물리 입력 QWASZX·ESC·Home·포커스를 검증한다. 762의 새 부팅/RDP세션1·전경비NULL에서도 기존 VSCode fresh선택/재선택2회 timeout이므로 실제 환경 변화 전 반복하지 않는다. 관측 없이 입력을 보내거나 앱 내 입력을 물리 입력으로 세지 않는다.
- [ ] 다른 모니터/DPI·실제 다른 앱 포커스 전환·DD 조정의 다른 비트/실제 장치 실패 경계를 확인한다. 746의 단일 마우스 조정/취소/확인/15초 만료·불법 모드 복구·저장 재시작은 완료다. 메시지 주입은 실제 앱 전환이 아니다.
- [ ] 실제 PCM/청취·사용자 활성 전체화면의 물리 출력과 실제 CD 삽입 W 경로를 확인한다. 594의 waveOut0/전경NULL,762의 원격오디오 1개/전경비NULL은 실제 PCM 열기/출력 성공이 아니다.
- 전체화면 종료 후 재실행·세 화면비·가장자리 스크롤은 확인한 범위에서 완료다. 관련 변경 뒤 필요한 회귀로 유지하며 무기한 `[ ]` 작업으로 반복하지 않는다. 원본과 다른 호환 처리·이유는 한국어 주석에 남긴다.
- Windows 10의 남은 AI 작업을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다(1.1절). ARM 실기 미검증도 명시한다.

### 8. 멀티플레이·배포·후속 단계

- [ ] **3차 TCP/IP 로컬 멀티플레이·Outpust**를 원문 근거로 사전 구현한다. 신규 단위/게임/통신 검사는 하지 않는다. 공식 서버 복구·구 클라이언트 유효성 검사는 연결하지 않는다.
- MP 이월 목록: RequestMenu 전송 `004d712e..004d717c`, 메타맵 열기 `004789b0/00479390`·번개 `00479650`·Challenge 이미지/텍스트 `00478d10`/참가자 삭제 `0042bc10`, c6/db·ChalSailPos 모드6·선행클릭/항해 메뉴·desc, 비미션 의식/랭크·조기 희생 안내, PlayerNoMake writer `00433c30`, frame22/주기64·timeout/UI·endpoint/server/boss·리슨/수락/22칸 송신.
- 일반 전투 `!state.dat` 저장/복구·saveBattle/metamap/비mission phase14·StateReconnect/서버 이전`004b8cb0`(ForceServerDeath/StateReconnect·자동 장애 caller,831)·Zone0/메타 Poll/`004c1e70` 시계 패킷·`004b54e0` 파일명/count 진단과 `0044fa00` 소비자·`004a7510` 참가자 재접속/팀 재편·실패 진단·LeaveBattle 원격 페널티를 포함한다(661의 분류). 기존 질문/삭제 UI는 유지한다. 정적 사전 구현과 검증 완료를 구별한다.
- [ ] **4차 로컬 MCP·해상도:** 모든 게임 조작·캡처·소리 포함 녹화, 16:9 최대 1920×1080을 제공한다. 그래픽 크기 스케일 조정은 이 단계의 요구가 아니다.
- [ ] **5차 사용자 저장 경로·무제한 맵 목록:** 모든 자동 생성 파일/디렉터리와 reader를 `%USERPROFILE%\Saved Games\Netstorm\` 아래로 옮긴다. 설정/진행은 Saves, 신규 맵은 CustomMaps. 원래/사용자 맵 병행 읽기·목록 스크롤/무제한을 구현한다. [733 조사](docs/exe/cpp-phase5-file-inventory-20261102.md)의 12파일군/50후보와 이후 추가 writer(HTML PieceList·옵션 임시/복구·!state/전체화면 표식·Edit 장부/잠금/pending·GIF/BMP·squids/procSumm·MCP 생성물)까지 포함한다. 이관 우선순위·두 설치 충돌/장부 identity·명시 export 정책을 정하고 일반 권한/읽기 전용 설치·다른 CWD/Unicode·저장/재적재/중단/교체/전체화면 재실행을 확인한다.
- [ ] **6차 한국어:** 메뉴/문자열/시나리오·글꼴을 지원한다. 글꼴은 D2Coding이며 명칭은 AGENTS.md를 따른다.
- [ ] **7차 AA/FSR1.0:** Options에서 각각 선택·기본 off. 지원 환경은 2560×1440/3840×2160을 추가하고 이 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 업스케일한다.
- [ ] **8차 Edit 시나리오 UI:** 초기 자본/테크·적·동맹 등을 별도 편집 UI로 수정하고 새 맵의 기본 english 자동 생성·english 부재 Test Battle 종료를 개선한다. 기존 수동 대본·저장/재적재를 보존한다.
- 배포 스크립트 자체는 완료다. 배포할 제품이 정해지면 README/BUILD_INFO를 갱신하고 그 제품의 ZIP·압축 해제 실행을 확인한다. 현재 검증 ZIP은644 제품이며808 배포 확인으로 쓰지 않는다.
- 60/120프레임·CD10.72 전체 복원/Windows 10 구동 불가 해결책은 별도 후순위다. Linux는dotnetpj 후순위이며cpppj는Windows 대상이다.

## 4. 운영 규칙과 유효한 사용자 결정

- **현재 진행 지시:**AGENTS/CPP·DN인계·hostname/status/fetch를 확인해853을 완료했고 목표 active로854 이후 전체 잔여를 진행한다. 최신0.24절이며 전체 목표/우선순위·제외 검사·단위 인계/커밋·push 금지를 유지한다.
- **단계 전환(2026-10-31):** 모든 자연 완주·전체 동일성은 사용자 확인이며 AI 잔여가 이것뿐이면 다음 단계로 간다. Windows 10 확인 뒤 Windows 11 실기 없이 넘어간다. 사용자가 오류/차이를 제보하면 해당 건을 AI 작업으로 올린다.
- **실행 빈도(2026-10-31):** 더 많은 기능 묶음 뒤 필요한 통합 확인을 한다. 소수 단위마다 전체 34·긴 Early 완주·전체 CTest/71대상/ZIP을 반복하지 않는다. 옛 3~5단위 기준은 자동 실행 조건이 아니다. 새 실제 실행은 변경과 직접 관련된 필요성을 먼저 판단한다.
- **검증 기록:** 정적 대조·명시 입력·컴파일·C++ 검사·GUI/물리 관측을 구별한다. 최초 실패/교정 원시를 보존하고 기대값을 제품에 맞춰 약화하지 않는다. 다른 제품의 성공·준비본을 현재 제품 통과 수로 합산하지 않는다. GUI/분석기 실행 720의 범위는 [720 기록](docs/exe/cpp-editor-graph-native-20261102.md)을 따른다.
- **CHEAT/테스트 조작:** CHEAT은 Edit 편집에서만 각 원문 조건으로 노출한다. `--test-bridge-patterns`·명시 로컬 AI/SP 등은 검사 조작임을 주석/기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다.
- **실행 파일/자료:** Windows 실행 파일은 `Netstorm.exe`다. CMake 대상명 NetstormCpp·OUTPUT_NAME Netstorm·ARCHIVE_OUTPUT_NAME NetstormCpp를 유지한다. 옛 `NetstormCpp.exe` 로그는 당시 이름으로 보존하고 원본/클론 프로세스는 전체 경로로 구별한다. 무인자 GUI·`--help/--run`·분석 CLI를 유지하며 EXE 옆 `game/netstorm.tarc`를 우선한다. cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다.
- **보호:** AGENTS.md·originals/originalCD 기존 파일은 수정/삭제하지 않는다(AGENTS의 명시 예외만 허용). 게임 실행 전후 원본 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild18.x를 쓴다. `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`로 빌드하며, 선행 CMake/MSBuild 자식이 끝난 뒤 한 번에 하나만 실행한다. exit_code0 확인 후 의존 검사를 한다. HJOW-X3D는 VS17(2022)/v143이며 BuildTools cmake의 전체 경로를 쓴다. [빌드 안내](docs/cpp-build.md) 참조.
- **UTF-8:** PowerShell 5.1에서 Python stdin으로 한국어를 보낼 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정한다. cp949 오류 시 `PYTHONUTF8=1/PYTHONIOENCODING=utf-8`을 설정한다. 원시 로그/Native fixture를 인코딩 정리로 덮어쓰지 않는다.
- **Git:** 이 문서/CPP 작업 파일은 기존 줄바꿈을 유지하고 확인/stage에 필요 시 `git -c core.autocrlf=false`·`-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정은 바꾸지 않고 최초 관측 TSV의 `-text`를 보존한다. DN/WB는 해당 문서·`.gitattributes` 규칙을 따른다. 단위별 인계 후 `YYYYMMDD-NN PJ 요약`으로 로컬 커밋한다. 당일 마지막 번호를 확인하며 cpppj=CP·dotnetpj=DN·webpj=WB·공통=CM이다. push는 금지다.

## 5. 참고 문서와 이관 기록

- [C++ 빌드](docs/cpp-build.md), [로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md), [모듈 맵](docs/exe/modules.md)
- [디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-11-03 양판본 디컴파일·컴파일](docs/exe/cpp-decompilation-20261103.md), [2026-10-23 양판본 디컴파일](docs/exe/cpp-decompilation-20261023.md), [analyzeManager](docs/analyze-manager.md)
- [준비본 보관](docs/exe/prepared/README.md): 443/444와 469의 일부는 후속 단위에 통합됐다. 보관본의 미적용/미빌드 문구를 현재 상태로 쓰지 않으며 base/patch/manifest/142파일 원시는 보존한다.
- 과거 전체 원문: [504까지](LEFT_JOBS.old.md#archive-20261027-through-504), [549의 0절](LEFT_JOBS.old.md#archive-20261029-section0-through-549), [561의 3절](LEFT_JOBS.old.md#archive-20261029-section3-through-561), [638까지](LEFT_JOBS.old.md#archive-20261031-through-638), [695까지](LEFT_JOBS.old.md#archive-20261102-before-cleanup), **[808까지·이번 정리 직전](LEFT_JOBS.old.md#archive-20261103-through-808)**.
- **이번 정리(2026-11-03, HJOW-Athlon):** 정리 전 원문을 보관 파일 끝에 그대로 추가하고 기존 보관 바이트를 유지했다. 대체된 실행/재개 지시·단위별 중간 수치/실패 나열·해소된 다음 작업·별도 요청 전 자동 공략 준비는 보관본으로 이관했다. 완료 항목은 2절의 `[x]`로, 미완료/부분·MP 이월·환경 대기·사용자 확인은 3절의 해당 위치로 정리했다. 제품 808/11.01·EXE 해시·마지막 디컴파일 호스트·일시중지는 유지했다. 문서/링크/보관 원문·해시·차이만 확인했으며 빌드/게임/기능 검사/디컴파일은 하지 않았다.

## 인수인계 문서 분담

cpppj/공통/디컴파일은 이 문서, dotnetpj는 LEFT_JOBS.dotnetpj.md, webpj는 LEFT_JOBS.webpj.md에 기록한다. cpppj 1차·Windows 10/11 호환·MCP가 완료돼 dotnetpj 구현에 활용할 수 있게 되면 인수인계를 LEFT_JOBS.md로 다시 통합한다. 현재는 프로젝트별 문서를 유지한다.
