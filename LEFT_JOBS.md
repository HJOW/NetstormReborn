# LEFT_JOBS — cpppj 및 공통 인수인계

cpppj와 공통 사항의 **현재 기준, 완료 범위, 남은 작업, 유효한 사용자 결정**을 정리한다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다.

**2026-11-04 문서 정리:** 단위861까지 원문은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md#archive-20261104-through-861)에 보존했다. 완료 범위는2절, 남은 순서는3절이다. **2026-11-06 사용자 재개: 1번929/2번943/3번956 정리 뒤4번 내부 수명을 계속한다. 962의 파생 생산/중립·표면 분배에 이어963에서 사제Owner의 현재 진단/부분효과와 실제 Client 연결을 복원했다. 964에서 신전 missingIsland 상세/실제 Client 진단과 선행 부분효과를 연결했다. 965에서미분류42cd10을폐기CD확인으로분류하고제품7f/80을제외했다.966에서실제CRT부분전달/버퍼실패와1176·1177복구상태를같은제품함수로대조했다.967에서!color.dat의실제FILE/시작·현재9표소비를연결했다.제품967/11.01이며다음968도4번Factory/파일·비멀티수명과남은색표소비다. 전체 목표는 미완료이며 일시 정지 상태가 아니다.**

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-11-06, 단위967 완료·제품967·계속 진행·전체 미완료)

| 항목 | 현행 기준 |
| --- | --- |
| 진행 상태 | **967 완료·제품967·4번 내부 수명 계속 진행.** !color.dat의기존부모를실제CRT/VFS·장기9표와시작/표시에연결했다.다음968은남은색표소비와Factory/파일·비멀티수명이다.4→7→8순서유지. |
| 현재 제품 | **967 / 표시·파일·제품11.01.** 사용자Cache의실제9표파일과건물/커서/생산/메뉴/Screen 현재표소비를연결했다.이전966옵션·965폐기검증과932~939를유지하며4번전체완료가아니다. |
| 실행·플레이 가능 여부 | 무인자 GUI 시작과 미션 플레이 경로가 있다. 일반31+데모3의 마지막 전체 진입 검증은 **645의644 제품**이며 현재 제품의 전체 회귀 결과가 아니다. |
| 원본과의 동일성 | 부품별 원문 대조와 집중 검사를 누적했다. **전체 동일성·모든 시나리오 자연 완주는 미확인**이며 사용자 확인 대상이다 |
| EXE SHA-256 | `7a8dd2ceec393a0e632b50b80836daf8e6fd623ef2f0fcbdbbb600e41b633263` (967 / 11.01, 11,355,648바이트) |
| 최신 빌드·집중 검사 | **967 게임/색표집중Release2회종료0·경고/오류0·최종고유13검사(새5/기존8)통과.** 처음12도실패0이며첫건물전달/암호TAFF를추가했다.기존Native부모130/9표6관측을대조했으며새CPU/fixture는0이다. [967기록](docs/exe/cpp-colormap-current-file-20261106.md). |
| checking 잔여 | **8함수=부모3+부분2+MP분류3.** 779기준51개 중 완료 부모39/부분4다. 917~920은 UI1의 커서/객체/게임 메뉴 검사이며 집계 불변이다. 전체 Factory/파생 수명과 메타맵2표 등은 잔여다. |
| 마지막 GUI | **967 실제MCP 순차2실행:** tutorial1/Tutorial을각새2/Replay2·Continue/Abort메인·종료0·설치자료불변으로확인했다.miss입력37/관측26,동일프로필hit입력35/관측24·캐시2304BYTE/해시/mtime불변·800×600 PNG4장확인.포획/공격/승리/MP검증이아니다. |
| 전체 검사 기준 | 현재활성솔루션83프로젝트=Application78(검사이름73/게임1/probe·dump4)+라이브러리2+Utility3.967에서색표FILE집중대상1개추가/생성slnx/vcxproj직접확인.과거963의82분류·779전체71/549전체3389중7실패/510제품508의3337통과는당시이력이며현재집중을전체통과로쓰지않는다. |
| 전체 34·ZIP | 645의644 제품, 최종34/34 진입·비정지 시각 증가·창DC RGB·Native Frame 확인. ZIP SHA-256 `6485febc36080dc90e9eec43965a842f85d39e81c82a003a96b83b34b3694b73`, 1376항목/게임 자산1358개, 한글😀 경로 압축 해제 실행 확인 |
| 보호 기준 | **2796개/fixture976개 불변·새fixture0.** 다음기준은 `extracted/cpp-resume-967/protected-final.json`이다. |
| 마지막 전체 정밀 디컴파일 | **HJOW-Athlon (192.168.0.94) / 2026-11-03**. 양판본 정밀 산출물과 컴파일 기록은 [2026-11-03 기록](docs/exe/cpp-decompilation-20261103.md)을 따른다 |

### 0.2 재개 절차

1. 사용자 재개 요청 뒤 AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 해당 인계도 읽는다. `git status`와 `git fetch`로 다른 PC/세션 변경을 확인하며 push는 하지 않는다.
2. 현재 호스트가 마지막 디컴파일 호스트와 다르면 cpppj 작업/디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 문서에 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 불완전하므로 쓰지 않는다.
3. 실제 게임·별도 창 실행은 AGENTS.md의 확인 규칙을 따른다. 예외는 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94) 및 사용자가 직접 요청한 원본 수동 컨트롤/녹화 분석의 해당 단계다. 다른 PC는 실행 목적·필요성 설명 후 명시적 확인이 필요하다.
4. 최신 정적 범위 정정 원시는 `extracted/cpp-resume-831/`, 최신 제품927 원시는 `extracted/cpp-resume-927/`, 최신 집중 검사930 원시는 `extracted/cpp-resume-930/`, 최신 UI 정적 감사929 원시는 `extracted/cpp-resume-929/`, 최신 정적 범위 정정은 `extracted/cpp-resume-866/`이며 Git 제외·작업 PC 전용이다. 커밋되는 증거는 `cpppj/*.json`·`docs/exe/cpp-*.md`다. 다른 PC에 원시가 있다고 가정하지 않는다.
5. 다른 세션의 dotnetpj/webpj·staged 파일·실행 출력은 포함하지 않는다. 한 단위를 마치면 0.1절·0.4절·2.1절 완료 항목·3절 잔여·단위 문서를 갱신하고 해당 파일만 로컬 커밋한다.
6. 완료 범위는 2.1절, 잔여 우선순위는 3절을 따른다. 단위별 과거 인계(834~861)는 [보관본](LEFT_JOBS.old.md#archive-20261104-through-861)과 각 단위 기록 문서에서 확인한다.

### 0.3 유지되는 제한과 보류

- **사용자 직접 확인:** 모든 시나리오의 자연 완주·전체 동일성, 실제 사제 포획/희생·특정 적 공격·시나리오 승리 재현. AI는 별도 요청 없이 자동 공략/테스트 지원을 진행하지 않는다. 짧은 메뉴·진입·기능 검증과 부품 원문 대조는 가능하다.
- **공중 3종:** 훨리기그·맨오워·더스트 데빌은 정적 분석·코드 작업만 한다. 새 전용 생산·생성·우클릭·행동 관측/검사는 하지 않는다.
- **멀티플레이:** 비멀티·UI·Windows 10 호환성 뒤 원문 근거로 사전 구현한다. 신규 단위 검사·게임·실제 통신 검사는 하지 않는다. 사전 구현을 동작/안정성 검증 완료로 쓰지 않는다.
- **단위 작업의 상시 제외:** 각 단위에서 실제 게임/GUI·진단 UI, 포획/희생/특정 적 공격/승리, 신규 MP/패킷/통신, 공중3종 전용, 물리 입력/PCM/CD 재시도, 전체71/CTest/34미션/ZIP 반복은 실행하지 않는다(4절 실행 빈도). CP 단위에서 DN/WB 파일을 수정·커밋하지 않으며, 작업 트리의 다른 세션 미커밋 변경은 정리·stage하지 않고 보존한다.
- **사용자 보류:** 스펠 효과 도움말 그림 추가 분석(530, 임시 숨김 유지), 연녹색 직사각형 추가 분석(원본에도 관찰, RDP 원인 미확정). `.english` 부재 Test Battle 개선은 영구 보류가 아니라 **8차 요구**다.
- **환경 한계:** 물리 입력/캡처는762의 새 부팅·RDP세션1에서도 두 번 timeout이었다. 실제 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·실제 앱 전환·실제 PCM/청취·사용자 활성 전체화면의 물리 출력·실제 CD 삽입은 미검증이다.
- **원문/호스트 경계:** 32비트 FILE/FileSpec/SEH·스택 배치/미정값 자체 일치, 부분 I/O/실제 할당 실패와 전체 진단 UI는 별도 한계다. 612의 반사 입력, VFX signed WORD/정상 표본·양수/회전0/원색의 근거를 전체 입력·픽셀로 확대하지 않는다.

### 0.4 최신 인계 — 단위967 완료(4번 현재 색표 FILE/실제 표시)·제품967·계속 진행

- **967 완료:** 기존RawColorMapCache를현재CRT/VFS·장기9표/owner접두와시작/건물/커서/생산/메뉴/Screen에연결했다.사용자Cache/!color.dat이며파일순서2/7/6/4/3/5/1/0/8·현재8색을PE/정밀원문으로확인했다.Release2회경고오류0·첫12/최종고유13검사(새5/기존8)통과·실제짧은fread/암호TAFF·Screen현재표와새owner의hit를확인했다.실제2프로세스 tutorial1 각새2/Replay2/메인종료0·설치불변·동일프로필캐시2304BYTE/해시/mtime불변·PNG4장확인이다.새Native/fixture/MP검사없음·보호2796/976불변·제품967/11.01·마지막정밀HJOW-Athlon/2026-11-03. [967기록](docs/exe/cpp-colormap-current-file-20261106.md). **다음968도4번의두8행/나머지색표실제소비·Factory/파일/비멀티수명이다.전체checking8/4번·CD전체파일부모완료는아니다.**

- **966 완료:** 실제 제품 CRT 종료/교체실패 함수를 추출하고 목표 부재와 조회실패를 구별·복원실패를 별도 진단했다. 실제 파이프65570중1024접두전달뒤실패·changed/핸들 반납, 버퍼 실패, C/H 정상·commit 저장과5복구상태를 대조했다. 고유11검사(새4/기존7)·Release경고오류0이며1176/1177은명시주입한코드/상태다.물리디스크고장/OS자체1176·1177은호스트미관측으로분리한다.첫대상명MSB1009보존·새GUI/MP/Native없음·마지막GUI965·보호2796/fixture976불변·제품966/11.01·마지막정밀HJOW-Athlon/2026-11-03. [966기록](docs/exe/cpp-options-save-failures-20261106.md). **다음967도3절4번이다. 직접74호출의현행대조중활성43c4e0/!color.dat의실제접합공백을찾았다. RawColorMapCache 부모는있지만src호출은자기정의뿐이고, ClientMain은OwnerTables.Initialize만호출한다. ClientColorSortInput.Maps도현재표시소비자호출이없고Sprite/Preview/Screen은따로Build한다. 다음은사용자Cache의FILE수명과실제표소비자를함께연결하며전체부모가없다고되돌리지않는다. Factory/다른파일·비멀티수명도잔여다.**

- **965 완료:** 785의미분류seek42cd10은CD파일/offset4BYTE확인→논리수신자11/응답80이다.40c26d등록/부모호출/PE전역을읽고제품공유전송의7f/80송신·단일/집합수신을제외했다.용도분류는동작+검증비활성화정책에근거하며고정도메인을새로증명한것은아니다. Release경고오류0·새단위/MP/프로토콜/Native실행0·실제새2/Replay2/메인종료0·자료불변이다.최초kClosed컴파일오류는0x40으로교정/원시보존. 보호2796/fixture976불변·제품965/11.01·마지막정밀HJOW-Athlon/2026-11-03. [965기록](docs/exe/cpp-retired-cd-validation-20261106.md). **다음966도3절4번options부분쓰기·1176/1177교체실패와Factory/파일잔여다.**

- **964 완료:** 신전 missingIsland의 현재 count/좌표/owner 상세→식/파일/행을 실제 Client에 연결했다. 이전 Player 신전 해제와 상세의 부분효과를 보존한다. 새Native96행 재관측차이0·고유9검사(새3/기존6)·Release경고오류0이다. 최초 경로 이중 표기96단언 실패와 같은 Session 기대를 Native 바이트로 교정하고 원시를 보존했다. 기존 신전864+864행/실제 World와962/963 Session도 통과했다. 새GUI없음·마지막962실제4진입·보호2795/fixture975불변·새1→2796/976·제품964/11.01·마지막정밀HJOW-Athlon/2026-11-03. [964 기록](docs/exe/cpp-vortex-owner-diagnostic-20261106.md). **다음965도3절4번 Factory 외부heap/할당 실패·파일/부분I/O와 남은 소비자이며 계속 진행한다.**

- **963 완료:** PATCH00491790/CD0040bda0의 요청0/공통 진단 정상 복귀·WORD/알림 부분효과·현재 모드/로딩과 실제 Client 진단을 연결했다. 새Native144행 재관측차이0·고유9검사(새3/기존6)·Release경고오류0이다. 첫 검사 const조회 컴파일 실패를 World.SetLoadingDepth 공개 경로로 교정했고 원시를 보존했다. 기존 사제3528+1944행과962분배4검사도 통과했다. 기존 혼합challenge guard는 정적 교정만/미실행이다. 새GUI없음·마지막962실제4진입·보호2794/fixture974불변·새1→2795/975·제품963/11.01·마지막정밀HJOW-Athlon/2026-11-03. [963 기록](docs/exe/cpp-priest-owner-current-20261106.md). **다음964도3절4번 신전 missingIsland 진단/현재 부모·Factory/파일 수명이며 계속 진행한다.**

- **962 완료:** PATCH00454cb0/CD0047b580의 생산 갱신 선행 증가와 PATCH00462580/CD00416f20의 고정 중립을 현재 VT로 선택하고 기존 표면 재정의와 합성해 실제 World/Session에 연결했다. 새Native128행 재관측차이0·고유13검사(새4/기존9)·Release경고오류0이다. 첫 입력 준비2건/필터 중복 원시를 보존했다. 실제MCP 새2/Replay2·메인복귀·종료0·자료불변/PNG확인이다. 보호2793/fixture973불변·새1→2794/974·제품962/11.01·EXEc1fe5ff5…·마지막 정밀HJOW-Athlon/2026-11-03. [962 기록](docs/exe/cpp-asset-owner-current-20261106.md). **다음963도3절4번의 사제Owner 현재 진단/부분효과·Factory/파일 수명이며 계속 진행한다.**

- **961 완료(4번 공통 Owner)·사용자 요청 일시 정지 인계:** +74자산151ret4/내장Form8ret8을정적으로구분했고공통004adf00/CD004aefa0의현재모드/TYPE·진단정상복귀/목록부분효과와실제World대여를복원했다. 새비전투/비challengeNative72행·재관측차이0·기존비challenge744+744행/고유12검사(새7/기존5)·최종Release경고오류0이다. 최초Core입력/void교정뒤수정전6단언실패와미등록필터2는원시보존이다. 기존혼합challenge끝기대2개는정적교정만/미실행·새게임/GUI/MP검사없음이다. 전체파생/heap/파일완료가아니고checking8불변이다. 보호2792/fixture972불변·새1→2793/973·제품961/11.01·EXE3d37d0cc…·마지막정밀HJOW-Athlon/2026-11-03. [961 기록](docs/exe/cpp-owner-current-20261106.md). **이단위문서/커밋뒤사용자요청으로목표일시정지. 재개하면3절4번파생Owner/현재분배·Factory외부heap/파일부터962를진행한다.**

- **960 완료(4번 상위 BaseProcess/갱신):** 0041bd20/CD0048f590의타입진단→필드→부모→Kernel→현재Create/Take·타입/PID/가상부착→통지와생성부분실패를복원했다. 내장4VT의실제+74와갱신의두진단/현재Form도연결했다. 새로컬Native48행/재관측차이0·고유21검사(새5/기존16)·최종Release경고오류0이다. 수정전5단언/갱신2단언·옛rollback기대7단언/초기검사·Native경계교정은원시보존이다. 실제MCP 새2/Replay2·Continue/Abort메인·입력35/관측24·종료0/사본1395불변·PNG육안확인이다. 지정수신/MP는코드반영만/신규검사없으며전체SEH/heap실패·모든파생VT완료가아니다. 보호2791/fixture971불변·새1→2792/972·제품960/11.01·EXE9851fae7…·마지막정밀HJOW-Athlon/2026-11-03. [960 기록](docs/exe/cpp-baseprocess-attach-current-20261106.md). 다음961도같은4번현재VT+74/파생·Factory외부heap/파일수명이며checking8불변이다.

- **959 완료(4번 Process 파생 부착):** 004b1440/CD004af380의부모변경진단정상복귀/저장판정·현재서버/장치선택·공통Form후단을복원했다. 새서버0Native48행재관측차이0·고유11검사(새3/기존8)·최종Release경고오류0이다. 수정전실제Host4단언실패와첫World검사server입력1실패/교정은보존했다. 실제현재World진단/공장/Pop깊이는통과했으며직렬화·송신분기는원문코드반영/컴파일만했고신규MP검사없음이다. 상위Attach/Reattach/RefreshForm의선행거부·생성실패복구/외부heap은잔여다. 새게임/GUI없음·보호2790/fixture970불변·새1→2791/971·제품959/11.01·EXEe522cda5…·마지막정밀HJOW-Athlon/2026-11-03. [959 기록](docs/exe/cpp-process-attach-current-20261106.md). 다음960도같은4번상위부모/Factory실패수명이며checking8불변이다.

- **958 완료(4번 정렬 Content 부착):** PATCH004aec10의선행Validate/생산TYPE거부를실제소비위치로되돌리고저장VT/현재getter·정렬링크/부모/contained→Pop→통지를복원했다. 같은Client현재contentDWORD/World현재TYPE도대여한다. 새PE41행/재관측차이0·고유16검사(새3/기존13)·최종Release경고오류0이다. 최초검사include누락/후보변환경고4건은교정원시보존이다. Native는통지진입까지/가상장치명시이며사제꼬리·전체파생/ABI완료가아니다. CD004afff0차이는기록하고기존제한API는후속10.72다. 새게임/GUI/MP검사없음·기존보호2789/fixture969불변·새1→2790/970·제품958/11.01·EXE81de8a94…·마지막정밀HJOW-Athlon/2026-11-03. [958 기록](docs/exe/cpp-content-attach-current-20261106.md). 다음959도같은4번Process004b1440/CD004af380·Attach/Reattach/상위Factory실패수명이며checking8불변이다.

- **957 완료(4번 첫 checking/공통Pop 현재연결대조):** 현재원문/실제World·Session장치와83VT(+20/28/30/88/8c)/46후처리·표시실효81지원/메타2미연결을대조했다. 기존31검사(부모14/Activate10/세계크기3/간헐천4)·집중Release경고오류0이다. checking전체8/완료부모39·부분4는유지하고잔여전체8은8번에명시했다. 81표전체실행/모든파생몸체검증이나전체Pop완료가아니며공중3종/Outpost4표는코드만대조했다. 새소스/검사/CPU/fixture/게임/GUI/MP실행0·제품956/11.01/EXE23d35643…불변·보호2789/fixture969불변·마지막정밀HJOW-Athlon/2026-11-03. [957 기록](docs/exe/cpp-pop-current-scope-20261106.md). 다음958은같은4번의Process004b1440/CD004af380·ContentFormHost선행ValidatePop/체인/부분쓰기·상위Factory공유수명잔여다.

- **956 완료(3번 비멀티 구현/대조 정리):** 원문점프표의동일State1/2본문과현재호스트의직접2거부를찾아Poll/외곽/앞단/HUDscope를연결했다. 수정전새검사1실패→실제Client/준비Session/파일/Deck/UI/전체State확정/다음4통과·고유38검사(새1/기존37)/4개Release경고오류0이다. cleanup34Effect와현재owner/소비자를정적대조했고세연속세대/실제반납을재대조했다. State2/특이State3자연도달·전체동일성은미확인,MP/State6/Zone0/저장복구는8번·CD전체이관은차후10.72이며새게임/GUI/CPU/MP검사없음이다. 보호2789/fixture969불변·제품956/11.01·EXE23d35643…·마지막정밀HJOW-Athlon/2026-11-03. [956 기록](docs/exe/cpp-mission-state-scope-20261106.md). **다음957은3절4번첫미완료checking/내부수명이며4→7→8순서를유지한다.**

- **955 완료(3번 셋째 항목의 LeaveBattle):** 실제양수부모를같은Client전역/사용자삭제장치의공용함수로추출했고새PE22행/재관측차이0·두삭제의부분효과를대조했다. 기존인라인동작보존이며새결함교정주장이아니다. 새2/기존19=21검사·최종Release경고오류0,최초BYTE채움C4244교정기록보존이다. CD0041ae80/0042fd90와주소차이를문서화했고CD호스트전체이관은차후10.72범위이며State6/원격페널티는8번이월·신규MP검사없음이다. 새게임/GUI없음·기존보호2788/fixture968불변·새1개→2789/969·제품955/11.01·EXE19fd9ba9…·마지막정밀HJOW-Athlon/2026-11-03. [955 기록](docs/exe/cpp-battle-leave-reason-20261106.md). 다음956은같은3번Native/State/cleanup잔여다.

- **954 완료(3번 셋째 항목의 Tick/연속 전환):** Native Poll이새로Post한미션설치를같은Tick에서Take하던차이를고쳤고,미확정Fort의프레임/커서/패킷을실제Session·State확정까지닫았다. 최종새5/기존21=26검사·Release경고오류0·원래동작변이2사례/5단언검출후복원이다. 실제State1→7재설치/복귀(BridgeTheGap,입력5/TSV13)와Editor/TestBattle/복귀/메인(입력13/TSV8/제목8/PNG4)이각종료0·자료1395불변이다. 중간후보4진입통과/최초잘못된Fort입력종료1·새검사별칭/없는API/카메라장치실패는별도보존했다. 일시pending자체는MCP미관측·State3특이자연도달미입증이다. 보호2788/fixture968불변·제품954/11.01·EXE 6942d8f7…·마지막정밀HJOW-Athlon/2026-11-03. [954 기록](docs/exe/cpp-tick-fort-handoff-20261106.md). 다음955도같은3번Native/State/cleanup/LeaveBattle잔여다.

- **953 완료(3번 셋째 항목의 일반Fort기술/사제):** 004923e0/00492120/004921a0의기술4종·지역20개/20칸·bit80선택/중복전체제거·중립소유자/설정·실제초기배치를연결했다. 새Native13행/재관측차이0·C++고유13검사(기존양판본부모130행포함)·최종Release경고오류0이다. 실제Client/Session의일반Fort파일/Core/World/Deck/사제/checkpoint/UI/tail/공통State확정/최종공개까지통과했으나특이State3자연GUI도달은미입증이다. 최초경고1/옛중단기대4실패/새검사Pool입력2실패와교정은보존했다. 기존보호2787/fixture967불변·새1개→2788/968·제품953/11.01·EXE e9c2190a…·새게임/GUI/MP검사없음·마지막정밀HJOW-Athlon/2026-11-03. [953 기록](docs/exe/cpp-fort-priest-preparation-20261106.md). 다음954도같은3번Native/State/cleanup Tick/LeaveBattle잔여다.

- **952 완료(3번 셋째 항목의 실제 본문/CoreData):** State3 제목 뒤 continuation을 실제 공통 파일/미션/월드 본문에 연결했다. 앞단/HUD를 중복하지 않고 별도 명시 대여에서 Editor/post0을 보존하며 기존 Editor1/1 계약을 유지한다. Core10BYTE/seed.low16/로컬무결성 읽기를 복원했다. 일반Fort는 실제World/Deck 뒤004923e0 미복원에서 멈추므로 정상 설치 전체는 미완료다. 최종 새4/기존19=23검사·Release경고오류0·실제Edit/TestBattle/복귀/메인 종료0·입력13/관측8/PNG4·자료1395불변이다. 최초 span 컴파일 실패/사제경계1실패/검사경고1은 별도 보존 후 교정했다. 보호2787/fixture967불변·제품952/11.01·EXE c685e424…·마지막정밀HJOW-Athlon/2026-11-03. [952 기록](docs/exe/cpp-native-fort-continuation-20261106.md). 다음953은 같은3번004923e0의 실제 기술/사제 부모와 나머지다.

- **951 완료(3번 셋째 항목의 반환 계약):** State3제목뒤다른Fort설치callback이정상복귀해도항상예외가되던계약을LocalMissionParentResult반환으로교정했다. 실제RawState에서정상phase확정/미션조기ret/장치누락·부분실패를새3검사로확인했고기존8(독립PE82행포함)까지고유11/실패0·Release경고오류0이다. 실제호스트callback은계속명시미연결이며반환형만맞췄다. 새게임/GUI/fixture없음·보호2787/fixture967불변·제품951/11.01/EXE9ac79063…·마지막정밀HJOW-Athlon/2026-11-03. [951 기록](docs/exe/cpp-native-menu-continuation-result-20261106.md). 다음952는같은3번실제Fortcontinuation접합이며prefix/HUD중복·미션조기ret·호스트공개순서를보존해야한다.

- **950 완료(3번 셋째 항목의 두 연결):** 실제Editor서비스에현재Guide.demo조회(미션NULL이면0)를연결해ESC의SessionMissing종료를고쳤고,제목에현재mode64도전달했다. 제품949의실제Getter실패와새950의Edit/TEST01→TestBattle→편집복귀→메인/No종료0·실제OS제목EDITOR/TEST/EDITOR/MAIN을대조했다. 앱입력13/선택관측8·원문TSV6개/PNG4개보존(파일명재사용2곳은기록후도구고유화)·자료1395/보호2787/fixture967불변·기존제목3검사/Release경고오류0이다. 첫실행은목록설정실패,둘째949는실제Getter오류,셋째950은통과이며합산하지않는다. 제품950/11.01·EXE9ac4a32b…·마지막정밀HJOW-Athlon/2026-11-03. [950 기록](docs/exe/cpp-editor-guide-title-20261106.md). 다음951도Native3/State1/단독State7/LeaveBattle잔여다.

- **949 완료(3번 셋째 항목의 소비자 누락):** 일반LeaveBattle의공통abort삭제callback에도UserSavedStateFile을전달했다. 제품939실제대조에서설치파일삭제/사용자파일잔존·reader미연결종료1을재현했고,새949에서같은도구로두연속왕복/4진입·사용자삭제2회/설치1395파일불변·종료0을확인했다. 앱입력35/보고24·논리1024×768PNG3·기존15검사통과·Release경고/오류0·보호2787/fixture967불변이다. 제품949/11.01·EXE SHA b51f2aca…·마지막정밀HJOW-Athlon/2026-11-03. [949 기록](docs/exe/cpp-abort-user-saved-state-20261106.md). 다음950도Native3/State1/단독State7/LeaveBattle나머지이며존재파일reader/일반전투복구는8번잔여다.

- **948 완료(3번 둘째 항목):** DissolvedAlliance실제MCP한프로세스751.073초·300관측·자연360초두만료/세대3·세번째게임14.339/벽시각13.552초진행·ExitDemo메인종료0을확인했다. 안내별도판정통과·앱입력5개·자료1394/보호2787/fixture967불변·제품939고정·실패/재시도없음. [기록](docs/exe/cpp-demo-second-alliance-20261106.md). 다음949는비멀티 Native3/State1/단독State7/LeaveBattle 잔여 대조다.

- **947 완료(3번 둘째 항목):** GuardMyBack실제MCP한프로세스796.244초·362관측·자연360초두만료/세대3·세번째게임14.395/벽시각13.679초진행·ExitDemo메인종료0을확인했다. 안내별도판정통과·앱입력5개·자료1394/보호2787/fixture967불변·제품939고정·실패/재시도없음. [기록](docs/exe/cpp-demo-second-guard-20261106.md). 다음948은DissolvedAlliance 두 번째360초/세 번째 월드/정리다.

- **946 완료(3번 둘째 항목1/3):** 제품939실제MCP창으로DemoStormRages자연360초두번만료→월드1/2/3·새HUD→세번째게임13.332/벽시각13.209초진행→ExitDemo/메인/Quit종료0을확인했다. 최종794.182초·373관측·안내각22관측/약15초·앱입력5개·논리PNG5개다. 첫실행은보고쓰기실패로종료1이며통과가아니다. 둘째실행의최초후처리활성1/메인카운터0가정실패는원시/최초판정을보존하고별도result-v3로교정했다. 전면활성유지/물리입력확인이아니며시계/기한/AI/승패조작없음·실행1394/보호2787/fixture967불변·제품빌드없음. [946 기록](docs/exe/cpp-demo-second-storm-20261106.md). 다음947은GuardMyBack의두번째360초/세번째월드/정리이고그뒤DissolvedAlliance다.

- **945 완료(3번 첫 항목 정리):** 제품939실제MCP창한프로세스에서tutorial1(Early1)새진입2+Replay2=4진입,Continue/Abort/MainMenu연속왕복·정상종료0을확인했다. 앱입력37/보고26·논리800×600 PNG3개와미션/최종메인육안확인·실행자료/보호2787/fixture967불변. 총7프로세스중앞6개는도구의ID/언어설정/첫사용자안내기대오류이며최종통과에합산하지않는다. 제품/기존fixture불변·Python문법통과. [945 기록](docs/exe/cpp-mission-abort-replay-20261106.md). 다음946은3번둘째데모의두번째360초/주기·정리다.

- **944 완료(3번 첫 항목):** Begin→Restart→Fork→Close→Begin→Init→Native정리→재적재를실제Client/Session/작업본에서3회연속실행했다. Guide교체동안월드/Player/Kernel/Template/네타이머기한유지·Config등록복원,State정리의실제반납과새월드/안내/팔레트/UI횟수를확인했다. 새1/기존6 고유7검사통과·제품939불변. 첫추가컴파일C1128은검사TU만MSVC /bigobj로고쳐최종경고/오류0이다. [944 기록](docs/exe/cpp-mission-three-transitions-20261106.md). 다음945도3번첫항목의실제Abort메뉴/나머지전환이다.

- **943 완료(2번 정리):** 현재Edit메뉴/섬추가·배치·이동/그림·자르기/객체취득·소유자/LeaveEditMode/State1→7의실제소비자를대조했다. 섬2·섬창2·손상파일3·Image1·이동1·메뉴/메뉴바2·객체메뉴1의고유12검사통과·새코드/검사/빌드/게임실행0·제품939/보호2787/fixture967불변. 940~942와기존원문근거를연결해3절2번구현/대조를정리했다. 모든조합/외관/SEH·Factory전체는4~6번한계이며State3특이분기/연속미션전환은3번에남긴다. [943 기록](docs/exe/cpp-editor-implementation-closure-20261106.md). **다음944는3절3번첫항목**이다.

- **942 완료(2번):** 알려진 `FortObjectLoad_ActualFactoryOwnerHpPopAndContent` 실패를재현하고검사의필수Pop관측자와TYPE6원문생성자입력을교정했다. 제품진단/전송관문/기존owner·HP·내용물·해시기대값은불변이다. 양판본적재5·기본이름3·지연파일2·State UI3·Guide반납2의고유15검사최종통과·빌드경고/오류0·제품939/보호2787/fixture967불변. State1→미션요청→State7/예외안내반납은현재코드정적대조이며새패킷/실행검증아니다. [942 기록](docs/exe/cpp-editor-load-test-devices-20261106.md). 다음943도3절2번의남은편집도구/LeaveEditMode다.

- **941 완료(2번 둘째 항목):** 실제 편집 Session의 전체맵 snapshot 뒤 OS 쓰기공유거부에서 next91/phase/디스크바이트 보존·선행 snapshot/난수/Badges 유지, 잠금 해제 후 같은Session 재저장/next3·7섹션 재파싱을 확인했다. 새1/기존6 고유7검사 첫 통과·Release경고/오류0·제품939/보호2787/fixture967불변·게임/신규MP테스트0. [941 기록](docs/exe/cpp-editor-save-failure-retry-20261106.md). 다음942도3절2번의State1/비동기State7·남은도구다.

- **재개/순서 결정(2026-11-06 사용자 정정):** 3절을 앞부터 진행한다. 1번은929에서 완료했으므로 **2번 Edit/커스텀 맵/시나리오 → 3번 미션 수명 → 4번 내부 수명 → 7번 호환성 → 8번 후속 단계** 순서다. 5·6번 사용자 확인과 지정 제외 테스트는 건너뛴다. 이전의 “감사 항목에 종결 조건이 없으므로 4~8차를 우선” 결정은 철회한다. 현재 HJOW-Athlon은 마지막 디컴파일 호스트와 같으며 원본을 다시 실행하지 않고 2026-11-03 정밀 자료를 읽는다.
- **939 요약(7차 AA/FSR 1.0 1차):** AMD FidelityFX FSR 1 공식 헤더(MIT, `third_party/fsr1`, 수정 없음)를 CPU 상수 계산(`client/FsrConstants`)과 GPU HLSL(빌드 때 C++ 배열로 내장, 런타임 `D3DCompile`)에 쓴다. `client/GpuPresenter`(Direct3D 11, 하드웨어→WARP): 8비트 팔레트 프레임 업로드→팔레트 변환→(선택)FXAA식 안티앨리어싱→(선택)EASU→RCAS→레터박스 출력(flip discard 스왑 체인). `Screen::SetPostProcess`/`SetPresentSize`/`ReadPresented`: 켜지면 `Update`가 GDI 대신 완성 프레임 전체를 GPU로 표시(실패 시 GDI 복귀), 창 모드도 비율 맞춤/입력 역변환, 테두리 없는 창 판정은 표시 크기 기준. `Client`: 설정 `antiAliasing`/`fsr1`(기본 0), `SCREENW/H`=표시(목표) 크기·FSR 해상도(2560×1440, 3840×2160)는 논리 1920×1080+FSR 자동, 장치 없으면 1920×1080으로 낮춤, `NETSTORM_NO_GPU=1` 비활성, `ApplyPresentationOptions`. 옵션 메뉴(메인·게임 중)에 Anti-Aliasing/FSR 1.0 Upscaling 토글과 FSR 해상도 두 행(장치 있을 때만, 원문 918 관측은 훅 꺼진 기본에서 불변), MCP `netstorm_screenshot presented=true`, 한국어 번역 두 항목, 배포 스크립트의 `third-party\fsr1` 동봉. [939 기록](docs/exe/cpp-aa-fsr-20261106.md).
- **938 요약(6차 한국어 1차):** 한글을 8비트 글꼴 바이트 경로에 싣는 4바이트 확장 시퀀스(`0x1f`+25진 3자리, `o::EncodeOriginalText/DecodeOriginalText`), `BitmapFont`/`FontStore` 확장 글꼴(D2Coding을 GDI로 한 글자씩 256색 글리프로 생성·모노스페이스 전진 폭), 폭 표/그리기/렌더러 확장 글리프 콜백, 언어 번호 7 `korean`(`kKoreanLanguageNumber`, OS 자동 선택은 안 함), `Client` 한국어 번역표(`assets/lang/korean/xlat.korean` 502항목·서식 코드 전수 검증·`unit-names.korean.json`과 일치), 옵션 팝업(메인·게임 중) Language 하위 메뉴(`GameMenusHooks::compatibilityLanguages`, 비면 원문 918 관측 불변), 글꼴/번역표 탐색(`client/KoreanResources`: exe 폴더·게임 폴더와 상위 6단계의 `fonts/D2Coding*.ttc`, `lang/xlat.korean`), **`BaseFileSystem`의 `.korean`→`.english` 대체(읽기/존재/읽기 `Open`/와일드카드; 한국어 파일이 있으면 우선)**, 배포 스크립트의 fonts/lang 동봉과 안내문의 설정 위치(5차) 갱신. [938 기록](docs/exe/cpp-korean-language-20261106.md).
- **931~937 요약:** 931 언어/폰트 정적 재대조, 932~934 5차(사용자 홈 저장 경로·Editor 이관·Edit 목록 무제한/스크롤·읽기 전용 설치 검증) 완료, 935~936 4차 로컬 MCP 서버(`Netstorm.exe --mcp`: 조작/상태/보고서/화면 PNG/프레임 대기/소리 포함 녹화/종료), **937 4차 와이드 해상도: 1600×900·1920×1080 선택 추가, `IsSupportedScreenResolution`으로 허용 판정 목록화, 높이 800 초과 메인 버튼 y 보정.** 기록 [해상도 문서](docs/exe/cpp-wide-resolution-20261106.md), [MCP 문서](docs/exe/cpp-mcp-server-20261106.md), [5차 문서](docs/exe/cpp-user-data-root-20261105.md).
- **939 검증:** `netstorm_fsr_tests` 5/0(상수=공식 정의, 숨은 창 끝-끝 AA/FSR 네 조합), `screen_resolution` 3/0, `GameMenus_OptionsPostProcessEntriesOnlyWhenEnabled`·`GameMenus_Native918`·`mcp_parts`·`korean_text`·`user_data_root` 통과, `main_popup`/`renderer_presentation`의 1개는 기존 `CampaignSaveGoMain_*`. 실제 실행(창 모드·MCP): 기본 꺼짐은 논리 화면 그대로, `SCREENW=2560;SCREENH=1440`에서 논리 1920×1080·창 클라이언트 2560×1440·`presented` 캡처 2560×1440 FSR 결과 육안 확인, 옵션 팝업의 두 토글이 체크되며 GDI→GPU 출력 전환. 빌드 경고/오류 0. **미확인:** 미션/월드 화면, 전체화면(레터박스·가장자리 스크롤·커서 제한), 실제 4K 모니터, 창 모드 입력 역변환, ARM64, 소프트웨어 장치 성능(사용자 확인/후속). 통합 `netstorm_tests`는 재빌드 1시간 이상이라 개별 집중 검사로 대신했다.
- **938 검증:** 새 `netstorm_korean_text_tests` 10/0(부호화·글리프·FontStore·번역표·글꼴 탐색·언어 번호·Client 전환·`.korean`→`.english` 대체)과 `GameMenus_OptionsLanguageSubmenuOnlyWithCompatibilityLanguages`(원문 918 관측 불변)가 통과했다. 연관 `capture_client`·`config_path_load`·`script_bindings`(FontFace)·`product_ui`·`language_options`·`xlat_runtime` 검사는 0실패다. 처음 한글 확장을 `EncodeOriginalText`에 직접 넣었다가 설정 저장/유니코드 경로 검사 12개가 깨져 **표시 전용 `EncodeDisplayText`/`DecodeDisplayText`로 분리**하고 재검사해 해결했다(교훈: 원문용 부호화의 "1252 밖 문자 거부"에 설정 저장 BOM 정책과 경로 정책이 의존한다). 실제 실행(창 모드, MCP 화면 PNG)으로 한국어 옵션 팝업·Language 하위 메뉴·English→한국어 즉시 전환·`--mission Battle1` 진입(영어 브리핑)·압축 해제 배포판(1379항목)의 한국어를 확인했다. 집중 검사 68개 전체 실행(분리 수정 전 빌드)에서 48개가 완전 통과이며 나머지 20개의 실패는 대부분 `FocusedTestMain`의 장치 미연결 진단(`guide_release`/`world_generation` 151·`pop_context` 29·`file_tests` 5·`menu_session` 3 등)과 `StartupFileNames_ActualClientUnicodeStartupSaveAndReload`(3개, 단위732 경로 고리 검사)·`CampaignSaveGoMain_*`(1개)이다. 이들이 6차 이전부터의 실패인지 HEAD(1f5b4177) 기준선 빌드와 비교하는 중이며 결과는 3절 "미확인 실패"에 남긴다. 통합 `netstorm_tests`는 재빌드가 1시간 이상이라 개별 집중 검사로 대신했다. 빌드 경고/오류 0.
- **937 검증:** `netstorm_screen_resolution_tests` 2/0, 메인 버튼 배치 검사(8개 해상도) 통과, 실제 실행으로 1920×1080 메인 메뉴/옵션 해상도 목록(8개, 현재 표시)/미션 HUD·맵 뷰, 4:3 대비 동일 구성, SETSIZE→재시작 시 저장된 해상도 복원 확인. 빌드 경고/오류 0.
- **알려진 표시 결함(후속):** 높이 774 초과 해상도에서 좌측 덱 y=774..798에 80×25 검은 직사각형이 보인다(1280×800 기존 항목 포함). 768 기준 좌표의 숨은 요소로 추정하나 Gump를 확정하지 못했다. 사용자 제보 또는 후속 조사로 처리한다.
- **사용자 확인 필요(설계 결정, 5차):** (a) 작업 루트가 `CustomMaps`라 파일이 `CustomMaps\d\NAME.fort` 구조이고 첫 Edit 목록 때 원본 `d`의 fort 433개가 복사된다. (b) 목록 스크롤은 이동 행 클릭이며 마우스 휠은 입력 경로가 없어 미구현이다. (c) 이동 행 문구는 영어 고정(6차 번역).
- **미확인 실패(기준선 비교 안 함):** `netstorm_editor_default_name_tests`의2개중 `FortObjectLoad_ActualFactoryOwnerHpPopAndContent`는942에서검사장치/입력누락을교정해통과했다. 남은 `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은일반Battle/MP이월경로라제외하며, `netstorm_file_tests` 5개, `netstorm_main_popup_tests` 1개(`CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`)의 검사가 장치 미연결/기대 불일치로 실패한다(이름은 단위933 기록, 5차 문서). 사용자 정책/해상도 변경과 무관한 경로로 판단하나 직접 비교하지 않았다. **938 전체 집중 검사 관측(68개 실행·48개 완전 통과):** 위 외에 `guide_release`·`world_generation` 각 151(`CampaignSessionTests` 1028의 1024×768 건물 스프라이트 그림 비교 `actual==expected`와 `FocusedTestMain`의 장치 미연결 진단), `pop_context` 29, `menu_session` 3, `mission_entry` 1, `guide_fort_request` 1, `renderer_presentation` 1, `startup_*`/`file_directory`의 `StartupFileNames_ActualClientUnicodeStartupSaveAndReload` 3(경로 고리 `names.Index()`)이 실패한다. **HEAD(1f5b4177) 기준선 빌드와 비교해 `guide_release` 151개와 `startup_file_names` 3개가 6차 변경 전에도 똑같이 실패함을 확인했다(기존 실패).** `world_generation`은 `guide_release`와 같은 `CampaignSessionTests` 1028(건물 스프라이트 그림 비교)과 장치 미연결 진단을 공유하는 같은 계열이고, 나머지(`pop_context`·`menu_session`·`mission_entry`·`guide_fort_request`·`renderer_presentation`)는 장치 미연결 진단/`CampaignSaveGoMain`이라 기준선 개별 비교는 하지 않았다. 원인(1024×768 건물 그림 비교와 시작 경로 고리 `names.Index()`가 이 PC/환경에서 맞지 않음)은 별도 조사 대상이다.
- **940 완료:** 실제 Client Config 로더의 암호화 TARC→느슨한 파일→TARC 복귀, 동적 `{@미션.키}` 명령 이름, Tell 지역 param, 실제 HTML 메뉴 클릭/일반 큐/설정 writer와 동적 읽기 실패 뒤 반납/재생성을 대조했다. 영문549문서의 치환 위치980곳을 조사해 비멀티11명령967곳의 기존 연결·MP/관리10명령11곳의8번 이월·원본오자2곳 보존을 확인했다. 고유28검사(새2) 최종0실패·최종 빌드 경고/오류0·제품939/보호2787/fixture967불변. 첫 빌드의 auto선언2오류와 첫 실행의 검사구성/경로기대2실패는 검사만 교정했다. [940 기록](docs/exe/cpp-scenario-dynamic-config-20261106.md). **941에서 실제 저장 공유거부/재시도를 확인했으며 다음942도 3절2번**이다.8차 UI를 선행하지 않는다.
- **도구 주의:** Edit 도구는 파일 전체의 CRLF를 LF로 바꾼다. 이 문서와 `-text`/CRLF 저장 파일은 바이트 단위 편집만 쓴다. Bash 명령 텍스트의 `\\`가 `\`로, `\n`이 실제 줄바꿈으로 바뀌므로 **역슬래시가 들어가는 C++/Python 편집은 Write 도구로 스크립트 파일을 만들어 실행**한다. Bash의 `/p:` 인자는 경로로 바뀌므로 빌드는 PowerShell로, 필요한 `--target`만 빌드한다(`ClientMain.h` 수정 때 전체 재컴파일 2~10분, 빌드 중 소스 수정 금지). `git diff`는 `-c core.autocrlf=false` 없이 본다. UI 스크립트 TSV는 BOM 없이(bash `printf`) 쓰고, 화면 확인은 BMP를 PIL로 PNG 변환해 이미지로 읽는다. 게임 프로세스를 이름으로 강제 종료하는 명령은 권한 분류기가 거부하므로 시작한 백그라운드 작업만 TaskStop으로 중지한다. 게임 스모크용 Python에는 exe/게임 디렉터리의 **절대 경로**를 넘긴다.
- **제품/보호:** 제품939/11.01·11,248,128바이트·EXE SHA `06030769a5f21dcdbe37cae9a1d40522be02f1e0b6a3246dc103f926eeea5098`. `originals` 파일 지문은 실행 전후 동일(`4a10a1e1…`). 제외 범위는 0.3절, 마지막 정밀디컴파일은 HJOW-Athlon/2026-11-03이다. 원문은 수정하지 않았다.

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **진행 중.** 34경로 진입·진행은 가능하다. 비멀티 UI 구현/부품 대조는929에서 정리했다. 남은 AI 작업은 Edit/시나리오/내부 수명이다. 자연 완주와 전체 동일성은 사용자 확인 대상이다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **진행 중.** Windows 10 호환 전체화면과 저장 재시작은 확인했다. 남은 Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다. 10.72의 Windows 10 구동 불가 해결책은 10.72 작업 때 문서화한다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | 전용 단계는 미착수. 채팅 MP 분기·707의 공유 의식 선택/비미션 UI·709의Zsend/송신장치처럼 일부만 정적으로 작성했고 신규 MP 검사는 하지 않았다. |
| 4차 | **로컬MCP**로 dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화. **16:9 최대 1920×1080 지원**이며 이 단계에서 그래픽 크기 스케일 조정은 불필요하다. | **구현 완료(935~937).** 로컬 MCP 서버(`Netstorm.exe --mcp`: 조작/상태/보고서/화면 PNG/프레임 대기/소리 포함 녹화/종료)와 16:9 최대 1920×1080을 구현하고 실제 실행으로 확인했다([MCP](docs/exe/cpp-mcp-server-20261106.md), [해상도](docs/exe/cpp-wide-resolution-20261106.md)). 남음: 와이드 좌측 덱의 80×25 검은 직사각형(y=774..798) 원인 확정, 실제 전체화면/큰 창 물리 확인(사용자). |
| 5차 | **일반 권한·읽기 전용 설치 정상 동작.** 설정/진행은`%USERPROFILE%\Saved Games\Netstorm\Saves\`, 신규 맵은`CustomMaps\`다. 모든 자동 생성 파일/디렉터리(임시/백업/잠금/장부/캡처/진단 포함)와 대응 reader도 같은Netstorm하위로 옮긴다. 원래/사용자 맵 병행 읽기와 Edit 스크롤/무제한도 포함한다. | **구현 대부분 완료(932~934).** 설정/체크포인트/표식/캡처/진단·Editor 작업본/장부·글꼴 캐시·PieceList 이관, Edit 목록 무제한/50행 스크롤, 읽기 전용 설치·유니코드 경로 실행 검증 완료([기록](docs/exe/cpp-user-data-root-20261105.md)). 4차 MCP 녹화 산출물도 사용자 `Captures`에 저장한다(936). 남음: `CustomMaps` 배치/복사 방식 사용자 확인. |
| 6차 | 한국어 지원 | **진행 중.** 938 1차 완료(글꼴·번역표·Language 메뉴·배포). 남음: 미션/팁/도움말 `.korean` 본문·한글 버튼 그림·번역 검수 |
| 7차 | Options의AA/FSR1.0 선택은 기본 off. FSR 지원 환경에는2560×1440/3840×2160을 추가하고 해당 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 목표로 업스케일한다. [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)를 참고한다. | **939 1차 완료(미확인: 미션/전체화면/4K).** |
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
| 8차 시나리오 UI | **구현 미착수.** 4~7차는 932~939에서 구현했다(2.1절). 기존 화면비·BMP·원문 언어 처리와 구별한다. 5차는733의 [12파일군/50정적후보](docs/exe/cpp-phase5-file-inventory-20261102.md) 조사만 완료 |
| 한국어·UTF-8·주석·용어 | 응답·새 코드 주석은 한국어, 새 문서/코드는 UTF-8. 한국어 글꼴은 `fonts/`의 D2Coding. 새 네임스페이스·상수·함수·구조체·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다. 명칭은 AGENTS.md의 번역/발음 표기를 따른다 |

## 2. 현재까지 완료한 범위와 검증 한계

### 2.1 완료 목록 — 부분 완료 범위를 명시

- [x] **967 현재색표FILE/실제표시:** !color.dat의기존부모→CRT/VFS/장기9표·시작/건물/커서/생산/메뉴/Screen을연결했다.고유13검사와실제miss/hit2실행·캐시/자료불변을확인했다.두8행의모든별도표시소비자와전체색표/파일수명은잔여다. [967기록](docs/exe/cpp-colormap-current-file-20261106.md).

- [x] **966 옵션 실제 CRT/교체 복구:** 같은 제품 함수의 실제 파이프 부분전달·버퍼실패/핸들반납과C/H 5복구상태를 대조했다. 고유11검사통과·1176/1177은명시코드/상태이며물리디스크고장/OS자체반환은미관측이다. [966기록](docs/exe/cpp-options-save-failures-20261106.md).

`[x]`는 아래에 적힌 범위의 완료다. 전체 기능·전체 동일성 완료를 뜻하지 않는다. 오래된 “다음/미연결” 대신 후속 단위의 실제 연결 근거를 따른다. 단위별 검사 수·실패/교정·당시 해시는 링크한 단위 문서와 보관본([808까지](LEFT_JOBS.old.md#archive-20261103-through-808), [861까지](LEFT_JOBS.old.md#archive-20261104-through-861))에서 확인한다. 항목 안의 checking 잔여 수는 그 단위 당시 값이며 현재 값은 0.1절이다.

#### 2.1.1 기능·UI·파일 범위

- [x] **965 파일소비자42cd10분류/폐기검증차단:** 원문7f등록·CD파일/offset4BYTE확인/11번응답80을추적해일반FILE목록에서제외했다. 공유전송에서7f/80을제외했고Release/일반싱글4진입을확인했다. 새MP/프로토콜검사는없다. [기록](docs/exe/cpp-retired-cd-validation-20261106.md).
- [x] **964 신전Owner 상세/Client 진단:** 섬 누락의 현재 count/ftol 좌표/owner 상세→원문 진단을 같은 Client에 연결하고 선행 Player/상세 부분효과를 보존했다. Native96행/고유9검사·기존864+864행이며 전체 파생/Factory/파일은 잔여다. [기록](docs/exe/cpp-vortex-owner-diagnostic-20261106.md).
- [x] **963 사제Owner 현재 진단/부분효과:** 요청0/공통 진단 정상 복귀와 WORD/알림·현재 모드/로딩·같은 Client를 복원했다. Native144행/고유9검사·기존3528+1944행이며 전체 파생/Factory/파일은 잔여다. [기록](docs/exe/cpp-priest-owner-current-20261106.md).
- [x] **962 파생Owner 생산/중립·현재 분배:** 실제 World/Session의 생산 접두/실패 부분효과·젬 중립·표면 색 분배를 복원했다. Native128행/고유13검사·실제4진입·자료불변이다. 파생 전체/Factory/파일 잔여는3절4번이다. [기록](docs/exe/cpp-asset-owner-current-20261106.md).
- [x] **961 공통Owner현재값/진단:** 현재모드/TYPE·진단정상복귀/늦은목록·실제World대여를복원했다. 새72행/고유12검사통과이며+74자산Owner와Form부착의정적계약을구분했다. 파생/heap/파일은당시잔여이며961의일시정지는962재개로해제됐다. [기록](docs/exe/cpp-owner-current-20261106.md).
- [x] **960 상위BaseProcess/갱신·생성부분수명:** 진단/현재필드·Kernel/늦은지정진단·내장4VT/갱신을복원했다. 새48행/고유21검사·실제4진입통과이며전체SEH/외부heap/파생VT는잔여다. [기록](docs/exe/cpp-baseprocess-attach-current-20261106.md).
- [x] **959 Process파생부착진단/현재후단:** 부모변경진단정상복귀·현재장치선택/공통부착을복원했다. 새서버0Native48행/고유11검사통과이며직렬화/송신은코드만반영했다. 상위Attach/Reattach/Factory실패수명은잔여다. [기록](docs/exe/cpp-process-attach-current-20261106.md).
- [x] **958 PATCH정렬Content현재부착/늦은Pop:** 저장VT·현재getter/그룹과정렬링크→부모/contained→Pop→통지·현재TYPE대여를복원했다. 새41행/고유16검사통과이며Process/Factory·사제꼬리/전체Pop잔여는유지한다. [기록](docs/exe/cpp-content-attach-current-20261106.md).
- [x] **957 비멀티checking/공통Pop현재연결대조:** 현재83표/실효표시81·메타2와실제World/Session장치·기존31검사를대조했다. 전체checking8/파생·Form/Process/Factory잔여는유지하고제품956불변이다. [기록](docs/exe/cpp-pop-current-scope-20261106.md).
- [x] **956 State2진입/3번비멀티범위정리:** 원문동일분배의직접2제한을교정했고실제Session확정/원문점프표/cleanup34case·고유38검사로대조했다. 자연도달/전체동일성·MP/내부잔여와구분해다음4번으로진행한다. [기록](docs/exe/cpp-mission-state-scope-20261106.md).
- [x] **955 LeaveBattle 양수 이유/두삭제·CD/State6분류:** 실제공용부모와원문22행/부분효과를대조했다. 고유21검사/Release경고오류0·CD차이기록·MP이월이며새게임없음이다. [기록](docs/exe/cpp-battle-leave-reason-20261106.md).
- [x] **954 Tick/미확정Fort/실제State1→7:** 새예약은다음Tick으로남기고실제World공개를정상commit에연결했다. 대조변이/고유26검사·실제두경로를확인했다. 3번나머지는잔여다. [기록](docs/exe/cpp-tick-fort-handoff-20261106.md).
- [x] **953 일반Fort기술/사제·정상확정:** 실제부모/목록/배치장치를연결했고새PE13행/실제Session확정/기존130행·고유13검사로대조했다. 자연특이분기/3번나머지는잔여다. [기록](docs/exe/cpp-fort-priest-preparation-20261106.md).
- [x] **952 State3 실제Fort/CoreData 연결:** 실제 본문을 접합하고 Editor/post0·Core/seed/무결성을 확인했다. 정상Fort의 사제 후처리는 잔여·집중23검사와 실제Editor왕복은 통과다. [기록](docs/exe/cpp-native-fort-continuation-20261106.md).
- [x] **951 State3 continuation반환계약:** 정상확정/조기ret결과전달과누락·실패보존을복원했다. 새3/기존8=11검사통과·실제호스트본문은여전히잔여다. [기록](docs/exe/cpp-native-menu-continuation-result-20261106.md).
- [x] **950 실제편집Guide조회/TestBattle제목:** 편집ESC의현재데모조회누락과시험제목+64전달을교정했다. 구제품실패/새제품실제왕복·OS제목·종료0·기존제목3검사통과. Native3특이분기등은계속잔여다. [기록](docs/exe/cpp-editor-guide-title-20261106.md).
- [x] **949 공통중단사용자저장파일삭제:** 실제부모의누락인자를교정하고구제품실패/새제품두삭제·설치보존/메인종료0·기존15검사를확인했다. 전체Native/reader복구완료로확대하지않는다. [기록](docs/exe/cpp-abort-user-saved-state-20261106.md).
- [x] **948 DissolvedAlliance두전체주기:** 실제두자연만료/세대3·안내원문대조·세번째진행·ExitDemo메인종료0·자료불변을확인했다. [기록](docs/exe/cpp-demo-second-alliance-20261106.md).
- [x] **947 GuardMyBack두전체주기:** 실제두자연만료/세대3·안내원문대조·세번째진행·ExitDemo메인종료0·자료불변을확인했다. [기록](docs/exe/cpp-demo-second-guard-20261106.md).
- [x] **946 The Storm Rages두전체주기:** 실제MCP동일프로세스794.182초·자연360초두만료/세대3·각15초안내·세번째비정지진행·ExitDemo메인종료0을확인했다. 최초실행/판정오류는별도보존·자료불변·제품939고정이다. [기록](docs/exe/cpp-demo-second-storm-20261106.md). 나머지두데모는3번잔여다.
- [x] **945 실제미션중단/계속/재시작/메인:** 제품939MCP창한프로세스로4진입/두번연속왕복·종료0·자료불변을확인했다.앞6도구실패는별도보존·새C++빌드없음.3번첫항목정리뒤다음946은데모둘째주기다. [기록](docs/exe/cpp-mission-abort-replay-20261106.md).

- [x] **944 Guide연속교체/월드3세대:** 실제참조/등록/네기한과Init/Native반납/재설치횟수를확인했다.원문286/831과고유7검사통과·검사TU /bigobj·제품939불변.3번나머지전환은계속진행한다. [기록](docs/exe/cpp-mission-three-transitions-20261106.md).

- [x] **943 원문Edit구현/대조정리:** 메뉴/도구/LeaveEditMode현재연결과실제섬·창·그림·이동·손상파일12검사를확인했다.940~942/기존근거를연결해3절2번완료범위를정리하고다음944는3번으로진행한다. [기록](docs/exe/cpp-editor-implementation-closure-20261106.md).

- [x] **942 편집 객체 적재/파일·State 경계:** 기존적재검사의필수Pop관측/TYPE6생성자입력을교정해양판본owner/HP/내용물/해시를유지했다.고유15개최종통과·제품939불변·State1/7정적대조. [기록](docs/exe/cpp-editor-load-test-devices-20261106.md).

- [x] **941 편집 저장 공유거부/재시도:** 실제Session 저장실패의next/phase/기존파일과선행snapshot유지,같은Session재저장/7섹션재파싱을확인했다.고유7개첫통과·제품939불변. [기록](docs/exe/cpp-editor-save-failure-retry-20261106.md).

- [x] **940 동적 시나리오/TARC/UI 설정 쓰기:** 현재Client 파일/임시Config/지역param/실제HTML메뉴·큐·writer와 읽기실패반납/재생성을 대조했다. 고유28개 최종통과·제품939불변. 3절2번 첫 항목 완료이며 저장/도구/전환 수명은 다음941이다. [기록](docs/exe/cpp-scenario-dynamic-config-20261106.md).

- [x] **시작·메인 메뉴:** 무인자 GUI·EXE 옆 `game/` 우선·cwd 독립(637), 기본 8/Intro 조건부 9버튼·Help/Options 팝업·실제 부모/모달/HTML 수명·45초 autoDemo(697~700), Credits 자동 전환(566), Help/Technical Help(603). [메인 부모](docs/exe/cpp-main-menu-parent-20261102.md), [휴대 실행](docs/exe/cpp-portable-startup-20261031.md)
- [x] **게임 UI·단축키:** QWASZX·독립 미니맵·Player List·Send Money, Shift 다중 선택/30칸 그룹 이동(701), N/Shift+N(702), P/R(703), H/F4/F5/Ctrl+F5(704), C/c(717), Tab 최근 5칸(718), 숫자/Shift 저장 10칸(719), U/u 손실 위치(751), F6 정지 경계(741), ESC 선택 정리/취소(742), 현재 demo 관문(743), 전체화면 F12(744), Pause(753), T/t(754), D/d(756), outside/모달/정지 분배(761). [선택 키](docs/exe/cpp-selection-key-input-20261102.md), [저장 카메라](docs/exe/cpp-saved-camera-key-20261102.md), [입력 분배](docs/exe/cpp-outside-input-routing-20261103.md). 앱 내 입력·집중 검사이며 물리 키 전체 검증은 아니다.
- [x] **객체 메뉴·편집 입력:** 83VT의 작성 13/비멀티 열기 5/특수 명령 9 연결(681), CHEAT 편집 전용(725), 실제 owner 변경/월드 갱신·Ctrl 다리 순환·모드1 취득/우클릭 취소·가상 ac/70·성공 음향 부모(747~750). [연결 표](docs/exe/cpp-object-menu-wiring-20261101.md), [편집 입력](docs/exe/cpp-editor-primary-selection-20261102.md), [메뉴 부모](docs/exe/cpp-object-menu-open-parent-20261102.md). 나머지 타입/상태별 명령 효과는 남아 있다.
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
- [x] **Windows 10 화면 전환의 확인 범위:** DIB 호환 전체화면·가장자리 이동·세 화면비·저장/종료/새 프로세스 적용, F12(744), 화면 변경/불법 요청 복구·DD 마우스 조정/취소/확인/15초 만료·저장 재시작(746). [화면 전환](docs/exe/cpp-screen-mode-transition-20261102.md). 다른 장치/DPI·물리 입력/출력은 미검증이다.
- [x] **배포 스크립트·빌드 선택:** `cpppj/package-cpppj.ps1`의 인자/대화형 경로·빈 값 취소·실행 파일/자산 동봉(562), 최상위 EXE 실행(637), Game/Tests/All 선택/기본 테스트 OFF(739). [빌드 모드](docs/exe/cpp-build-modes-20261102.md). 현재 제품의 새 배포 ZIP/전체 회귀는 아직 없다.
- [x] **사용자 제보 수정:** 덱 재활성 지연은 원문 Fast 1초로 복원(677), 간헐천 그림자는 원문 대상 변환/색표로 수정(678). [덱](docs/exe/cpp-deck-reactivation-20261101.md), [그림자](docs/exe/cpp-translated-shadow-20261101.md). 사용자의 자연 플레이 재확인은 별도다.
- [x] **설정 매핑 저장 실패 대응:** 제품 674의 오류1224·백업/교체/지연 정리, 도구 675의 복구를 완료했다. [제품](docs/exe/cpp-options-mapped-save-20261101.md), [도구](docs/exe/cpp-tool-settings-writer-20261101.md). 966에서 실제CRT부분전달/버퍼실패와1176/1177상태별복구를 대조했다. 과거 개별 원인 프로세스·물리디스크실패/OS자체1176·1177·전체audio재실행은미관측이다.
- [x] **11.01/Credits·외부 검증 비활성화:** 684·685 적용 완료. 1.3절의 문구·처리 경계를 따른다.

#### 2.1.2 checking 소비자·현재 상태 연결 (779~833)

- [x] **운반체 커서 경계(918):** 자기 위치 크리스탈 후보/화물·사제 HP 절반/무지면·빈 다이스 접근을 실제 Session에서 검증했다. 새28폴링/직접8평가·고유10사례 첫 통과·Release1명령 경고/오류0·제품916/보호2787/fixture967 불변이다. [918 기록](docs/exe/cpp-carrier-cursor-boundaries-20261105.md).

- [x] **전체 운반체 커서·화물 메뉴(917):** 실제8종의 기본/신전 경계·내부/화물 후보/소유·편집·마비·실제 그룹264폴링과 화물64메뉴를 검증했다. 새4/기존22의고유26사례·최종9명령29실행 실패0이며 초기 검사85단언을 보존했다. 검사 Release3명령 경고/오류0·제품916/11.01·보호2787/fixture967 불변·checking8 유지다. 나머지 상태/명령 효과는3절1번이다. 당시 일시 정지는918의 사용자 재개 지시로 해제했다. [917 기록](docs/exe/cpp-carrier-cursor-ui-20261105.md).

- [x] **손상 회수 현재 모드·제단/다이스 UI(916):** 초기false 사본으로 HP 절반 미만의 환불0이 누락되던 실제 연결을 현재 World battle 참조로 수정했다. 손상48/제단쌍48/빈제단32/무통신 가상삭제2의130메뉴·고유36사례 최종 통과다. 초기 제품4/검사guard2단언·정규화mtime 재컴파일 원인을 보존했다. Release3명령 경고/오류0·제품916/11.01·보호2787/fixture967 불변·checking8 유지다. [916 기록](docs/exe/cpp-altar-damage-ui-20261105.md).

- [x] **생산/지식 하위 메뉴·회수 조건(915):** 실제 내용물/Deck과 두 하위 메뉴의32상태, 예측/서버 순서2·금지/제외/모두 생산 중3·회수 조건80메뉴를 검증했다. 새117메뉴·고유23사례(새4/기존19) 최종 통과이며 초기 검사 입력/기대54단언을 보존했다. Release6명령 경고/오류0·제품913/11.01·보호2787/fixture967 불변·checking8 유지다. 성공 요청/효과·손상/제단 잔여는 다음916 UI1이다. [915 기록](docs/exe/cpp-production-object-ui-20261105.md).

- [x] **일반 건물 상태별 실제 메뉴/입력(914):** 네 워크샾 소유자/레벨/편집/SP의256메뉴·세 제네레이터 mission/소유자/HP 경계96메뉴·순환/자금 부족과 건설 거부를 검증했다. 새 메뉴371작성/72클릭 시도·고유17사례 최종 통과다. 초기 검사 경로/장치46단언과 경고1을 교정·기존 기대/fixture 불변이며 제품913/11.01·checking8 유지다. 성공 업그레이드/생산/회수·제단 잔여는 다음915 UI1이다. [914 기록](docs/exe/cpp-building-state-ui-20261105.md).

- [x] **특수 대상 메뉴·선택된 스펠 삭제 UI 수명(913):** 크리스탈/비석·스펠의 명시 모드·소유자 메뉴/클릭·전환·팝업 반납을 검사하고 삭제 후 Select(0)의 반납된 PID 재조회를 원문 별도 Clear로 수정했다. 고유20사례 최종 통과·초기4실패와 검사 장치 교정을 보존했다. 제품913·Release5명령 경고0·보호2787/fixture967 불변·새 Native/fixture0·checking8 유지다. UI1 전체/자연 도달·픽셀 동일성 완료는 아니다. [913 기록](docs/exe/cpp-special-object-ui-20261105.md).

- [x] **공통 Form Pop의 현재 진단/권한·공유 깊이(912):** 실제 Factory·공통 부착/Session Content 후단을 같은 깊이에 연결했다. 새 Native2,560입력/재관측 일치·고유41사례 첫 통과·14명령41실행/실패 단언0이다. Pop 자식의 기존 수동 기대1곳을 원문으로 교정했다(CHECK50→50). Release1명령 경고0·기존2786/fixture966개 불변·새fixture1개·checking8 유지다. 사용자 지시로 다음913부터 UI1번을 우선한다. [912 기록](docs/exe/cpp-form-pop-current-20261105.md).

- [x] **공통 Form 부착의 현재 TYPE/VT·부분 쓰기(911):** 실제 Factory/Gump/Depend/Process 후단과 현재 Session 번호를 연결했다. 새 Native864입력/재관측 일치·고유37사례 최종 통과·26명령72실행/실패 단언0이다. Content thunk 분류/기존 수동 검사2곳을 원문으로 교정했다. Release2명령 경고0·기존2785/fixture965개 불변·새fixture1개·checking8 유지다. 공통 Pop/다른 접두·수명은 잔여다. [911 기록](docs/exe/cpp-form-attach-current-20261105.md).

- [x] **감시 쌍 현재 생성/번호·클래스 수명(910):** 두 현재 Create/WORD·VT/부착 부모와 장기 번호/실제 World 공장·독립 입력을 연결했다. 새 Native1,620입력/재관측 일치·고유32사례 최종 통과·23명령61실행/실패 단언0이다. 초기Graph2등록누락/첫29통과·클래스 경계 보완을 기록했다. Release2명령 경고0·기존2784/fixture964개 불변·새fixture1개·checking8 유지다. 공통 부착 자식/다른 클래스·번호는 잔여다. [910 기록](docs/exe/cpp-depend-create-current-20261105.md).
- [x] **CD 공간 Unpop 현재 진단·부분 쓰기/해시(909):** 선행 전수 거부를 제거하고 현재 TYPE/실제 종속 분리를 같은 부모로 연결했다. 새 CD CPU1,402행/기존 PATCH1,300행 등은 첫 검사부터 일치했고 고유40사례 최종 통과다. 최초 관측기 배치/3사례4단언 실패·입력/경로 교정과 첫 빌드 종료 코드 관측 누락을 보존했다. Release2명령 경고0·기존2783/fixture963개 불변·새fixture1개·checking8 유지다. 고정 Form 생성 편의 경로/다른 부모는 잔여다. [909 기록](docs/exe/cpp-cd-squid-unpop-current-20261105.md).
- [x] **양판본 Form Unpop 현재 진단/VT·종속 분리(908):** 현재 Factory·Process/Gump/Depend·Session ContentForm을 같은 World 분리에 연결했다. 새 Native1,728입력과 기존600입력은 첫 검사부터 일치했고 고유50사례 최종 통과다. 최초3사례/4단언 실패·입력/보호 계약 교정을 기록했으며 Release2명령 경고0·기존2782/fixture962개 불변·새fixture1개·checking8 유지다. CD 공간 Unpop과 다른 생성/부착 부모는 잔여다. [908 기록](docs/exe/cpp-form-unpop-current-20261105.md).
- [x] **Process/Gump 현재 Factory·World 대여(907):** Process 세생성/지정과Gump Take를현재공통부모/장기Form번호·현재World대여에연결했다. 새4/기존51 고유55사례 첫 실패0·Release1명령 경고0·기존2782/962fixture와 입력82파일의 단언2,387 불변·새 Native/fixture0·checking8 유지다. Form·CD Unpop/나머지부모는잔여다. [907 기록](docs/exe/cpp-process-form-current-factory-20261105.md).
- [x] **실제 Form3/6 현재 생성자·contained 번호(906):** 고정 VT를 공통 현재 Create/반환 VT+28로 바꾸고 실제 Session의 현재 contained/bridge를 연결했다. 새3/기존41 고유44사례 첫 실패0·Release3명령 최종 경고0·두 컴파일 오류 교정·기존2782/962fixture와 입력10파일의 단언370 불변·새 Native/fixture0·checking8 유지다. Process/Gump Take·하위 해제는 잔여다. [906 기록](docs/exe/cpp-form-creation-current-20261105.md).
- [x] **양판본 Take 현재 부모/반환 객체(905):** 현재 VT+48·생성자/반환 객체·판본별 상태/TYPE·저장 VT+2c를 실제 Factory에 연결했다. Native720행/재관측·새3/기존31 고유34사례 첫 실패0·Release1명령 경고0·기존2781/961fixture 불변·새1개→2782/962개·checking8 유지다. Form/contained 하위·고정 생성은 잔여다. [905 기록](docs/exe/cpp-squid-take-parent-current-20261105.md).
- [x] **양판본 공통 초기화/현재 진단·HP(904):** CD PostCreate/PostTake의 일괄 거부를 원문 진단 복귀/현재 TYPE·권한·HP WORD/부분 쓰기로 복원했다. Native576행/재관측·새2/기존21 고유23사례 첫 실패0·Release1명령 경고0·기존2780/960fixture 불변·새1개→2781/961개·checking8 유지다. CD Take 앞단/고정 Form은 잔여다. [904 기록](docs/exe/cpp-factory-initialization-current-20261105.md).
- [x] **회복 현재 TYPE/읽기 순서·Help HP(903):** 판본별 번호/HP/getter 순서·반복 증분/상한과 실제 Session/초기 Help를 공유 getter에 연결했다. Native864행/재관측·새4/기존19 고유23사례 첫 실패0·Release1명령 경고0·기존2779/959fixture 불변·새1개→2780/960개·checking8 유지다. [903 기록](docs/exe/cpp-priest-regen-help-hit-points-current-20261105.md).
- [x] **연소점 HP/현재 전역·정수 부모(902):** 사전 HP/선택 TYPE this·진단 뒤 현재 flags2와 실제 공유 getter/장기 사제·dais/약화를 연결했다. 양판본5652행/재관측·새3/기존14 고유17사례 첫 실패0·Release1명령 경고0·기존2778/958fixture 불변·새1개→2779/959개·checking8 유지다. 기존 입자/선택·제거/행/난수 관측도 불변이다. [902 기록](docs/exe/cpp-burning-source-count-current-20261105.md).

- [x] **선택 HP 막대/현재 표시·전송 getter(901):** 원문 PATCH3회/CD1~2회 조회와 HP/색 순서·저장 TYPE this를 실제 공장 getter에 연결했다. Native504행/재관측·새5/기존25 고유30사례 첫 실패0·Release1명령 경고0·기존2777/957fixture 불변·새1개→2778/958개·checking8 유지다. 전송은 정적 구현이며 신규 MP 검사는0이다. [901 기록](docs/exe/cpp-sprite-hit-point-bar-current-20261105.md).

- [x] **보상 HP 현재 진단/공유 getter(900):** 선택된 TYPE this/현재 표 기준·장기 전역과 실제 Session assertion을 연결했다. 기존 원문 어댑터1158행/직접 getter1160행·새3/기존17 고유20사례 첫 실패0·Release1명령 경고0·보호2777개/957fixture 불변·새 fixture0개·checking8 유지다. 다른 HP caller/전체 Factory는 잔여다. [900 기록](docs/exe/cpp-reward-hit-points-diagnostic-current-20261105.md).

- [x] **Create/현재 TYPE 생성자 부모(899):** 양판본진단/실제할당/signed SID/현재생성자/반환객체VT/TYPE/실제+28과현재rawTYPE생성자·Retype·장기fakeTYPE를연결했다. Native588행/재관측·고유33사례최종실패0·Release3명령경고0이다. 최초CD추적294단언은ASM순서로교정했고기존2776/956fixture불변·새1개→2777/957개·checking8유지다. 전체Factory/다른HP caller는잔여다. [899 기록](docs/exe/cpp-squid-creation-parent-current-20261105.md).

- [x] **TYPE HP/현재 Factory·장기사제번호(898):** 양판본getter의진단복귀/같은TYPE this/현재base·특별TYPE/부호있는HP/4와실제Factory·World/Session현재checking을연결했다. 사제번호를Client장기owner에두고기존이동/회복참조에연결했다. Native1160행/재관측·새5/기존27 고유32사례 첫 실패0·Release2회경고0·기존2775/955fixture불변·새1개→2776/956개·checking8유지다. 다른HP caller/생성부모는잔여다. [898 기록](docs/exe/cpp-rifttype-hit-points-current-20261105.md).

- [x] **간헐천 부분 표시/현재 기본 외형(897):** 004854b0/가상78과현재TYPE+128/defaultFrame·실제로더·공간/프레임연결을복원했다. Native432행/재관측2회 동일·새4/기존26 고유30사례 첫 검사 실패0이다. 최초미존재필드6오류메시지를실제defaultFrame으로교정한후제품/집중·최종집중빌드경고0이다. 기존2774/954fixture불변·새1개→2775/955개·표시81표지원/메타맵2표미연결·checking8유지다. [897 기록](docs/exe/cpp-geyser-partial-display-current-20261105.md).

- [x] **공통PostCreate 현재 진단 복귀/Pop 분배 감사(896):** 세 진단/현재 TYPE·서버·후단과 부분 효과를 같은 기존 Client/Session 대여에 연결했다. 새6/기존24, 고유30사례 첫 실패0·Release1회 경고0·보호2774/954fixture 불변이다. 감사에서 간헐천+8c 비멀티 누락을 확인했고 checking8/전체Pop 잔여를 유지한다. 새Native/fixture/MP0이다. [896 기록](docs/exe/cpp-factory-postcreate-pop-audit-20261105.md).

- [x] **checking 공유 owner와 후속 부모:** 779의 기존 41관문 공유와 별도로, 접근점(783)·다리 낙하(793)/붕괴(795)·Gump 표시/소멸(796)·연소 3소비자(797→798)·건설 Form(800)·커서 취소(801)·펜스 2타입(802)·Gunanim(803)·Squid 표시(804→805)·Gump 부착(806)·Gun 통지(808)·네 Gun PostPop(809)·CanonDecoder(810→811)·Graph 참조 감소/SID0조회(815)·Graph Add(818)·Graph sentinel/Flood의 마지막 Detach 소비자(819)·Boardcoord각도 세caller(820~821)를 실제 소비자에 연결하고 PaletteFlash 미도달 진단/현재 수명(824)·연소 갱신/회복 소비자(825)·마나 공급원 등록/카운터(826)·제단 해방 부모/현재 권한(828)·공통 Halt 진단/수명(832)·공통 도착/항상참 SID 진단(833)을 대조했다. [함수별 색인](docs/exe/cpp-checking-consumer-gaps-20261103.md), [808](docs/exe/cpp-gun-notify-checking-20261103.md). 순수부모 준비본을 실제 연결 완료로 중복 계산하지 않는다. 807의 Gun/바리케이드 서버 관문 4곳 owner 분리와 809의 네 PostPop 진단 위치·복귀까지 완료다.
- [x] **오벨리스크/스펠 전환 실제 진단 복귀(812):** 현재 Client 서비스와 실제 Factory/frame DWORD/QA WORD 바인더를 연결했다. 원문128+36행·집중6개(새3) 통과이며 공간/상위 입력은 명시 장치다. [812 기록](docs/exe/cpp-object-conversion-diagnostic-20261103.md). 전체 상태별 효과/GUI 완료와 구별한다.
- [x] **기본/Factory 레벨의 실제 진단 복귀(813):** 두 Session 연결점을 현재 Client 서비스로 바꾸고 원문 BYTE 저장→Factory 표시 순서를 유지했다. 양판본 원문96행·집중4개(새1) 최종 통과. 최초 검사 입력 오류1건 교정·제품/fixture 불변. [813 기록](docs/exe/cpp-squid-level-diagnostic-20261103.md). 전체 상태별 메뉴/GUI 완료는 아니다.
- [x] **ContentForm 부착 부모(814):** 현재 전투/checking·앞단 진단 복귀와 같은 실제 사제 참여/이름/음향/Tell·Client 기한을 연결했다. 일반 부착/기한/너겟 집중4개(새3) 통과. 포획 안내 본체와 실제 포획 재현은 미실행이며 원문/어셈블리·정적 연결/빌드 근거다. [814 기록](docs/exe/cpp-content-attach-parent-20261103.md).
- [x] **Graph 참조 감소/SID0 조회(815):** 현재 checking·진단 복귀 뒤 같은 행/후속 후보를 실제 영역 통지와 일반/지형/gameplay 앵커에 연결했다. 새 원문760입력/재관측760 일치·집중7개(새4) 최종 통과. 최초 검사 풀 입력 오류3건 교정·제품/fixture 불변. Graph5중2완료이며 나머지3은 잔여다. [815 기록](docs/exe/cpp-graph-reference-checking-20261103.md).
- [x] **Graph sentinel/실제 Free(816):** 현재 checking·두 번째 WORD 재조회와 진단 중 변경된 현재 행 쓰기를 연결했다. 새 원문1,080행·집중6개(새2) 통과, Release 빌드2회 종료0. CD 대응0045b560 교정. 나머지 중첩 caller는 미완료다. [816 기록](docs/exe/cpp-graph-sentinel-checking-20261103.md).
- [x] **현재 표면 이웃 flags8(817):** y/x·모서리/중복·현재 지도/후보/보관 원천의 부품을 연결했다. 기존250대조·새32행과 집중6개(새2) 최종 통과. 최초4단언 실패는 경계 후보 raw spot 조회로 제품 수정·fixture 불변. [817 기록](docs/exe/cpp-surface-neighbor-current-20261103.md).
- [x] **현재 Graph 부모/Client trace(818):** Add/Flood/Rebuild/Allocate·실제 postPop/postDestroy·World FullPool·현재 로그를 연결했다. 새 PE168행·C++집중34개(새3) 최종 통과, 실제 자산 Tutorial1 콘솔 초기 로드 종료0. 최초 입력/상태 관문/진단 listener 교정을 기록하고 기존 fixture 기대값을 유지했다. [818 기록](docs/exe/cpp-graph-live-checking-20261103.md).
- [x] **현재 Detach/영역 setter/시계(819):** 삭제 값 원천·일반 finder·현재 정책/Flood/sentinel·World 활성/분할 상태·현재 Client 시계를 연결했다. 새 PE440행·집중45개(새3) 실패0·최종 Release 경고/오류0이며 기존 fixture는 불변이다. [819 기록](docs/exe/cpp-graph-detach-live-20261103.md).
- [x] **Boardcoord 각도/방향 소비자(820):** 현재 CRT fmod/판본별 저장·진단 복귀를 네 방향 소비자와 썬 아처 종료각에 연결했다. 실제CPU1088행/재관측1088행·집중32개(새3) 최종 실패0. Unicorn112행 불일치와 최초 교정을 보존했다. [820 기록](docs/exe/cpp-board-angle-checking-20261104.md).
- [x] **Client 바람/현재 입자 owner(821):** 감쇠/공유 난수/각도·실제 Movie 가지·Storage0/1/0과 연소 현재 owner를 연결했다. 실제CPU1920행/재관측1920행·집중17개(새3)·현재owner의 기존 원문426행 통과. [821 기록](docs/exe/cpp-ambient-wind-20261104.md).
- [x] **주변음/날씨와 공유 State기한(822):** 현재 벽시각/120초·같은 난수·실제Client Wind 앞·음향뒤 현재 모달/flash·기존10초 재예약을 연결했다. 실제CPU1272행/재관측1272행·집중11개(새3) 최종 통과. State19는MP잔여다. [822 기록](docs/exe/cpp-ambient-sound-20261104.md).
- [x] **시간 알림/임시 Chat 서식(823):** Wind 뒤180초·현재 guard/옵션/기한/이름·같은 Config/일반 제출·front 정상 복원을 연결했다. 실제CPU1176+12행/재관측·집중11개(새4) 통과. MP 실제 전송 검사는 하지 않았다. [823 기록](docs/exe/cpp-monitor-time-20261104.md).
- [x] **팔레트 Frame의 미도달 진단/현재 수명(824):** 실제 기계어로 진단 미도달을 증명하고 기존 Frame/Client Kernel·FrameTime/Form을 CPU1670행/재관측·집중10개(새2)로 대조했다. CD 미초기화stack 차이는 별도다. [824 기록](docs/exe/cpp-palette-frame-checking-20261104.md).
- [x] **연소 갱신 진단/현재 회복 소비자(825):** 실제CPU1100행/진단216·재관측과 집중8개(새4) 최종 실패0으로 저장 HP/첫 검색·현재 TYPE/위치음·실제 Client/Session Form 생성/회복 삭제를 대조했다. [825 기록](docs/exe/cpp-damageable-burning-checking-20261104.md).
- [x] **마나 공급원 등록/현재 카운터(826):** CPU1084행/진단135·재관측·집중16개(새6) 실패0으로 중복 진단/저장 판단·signed 용량/전체 DWORD·실제 Client/두World/Session 공통postPop·현재TYPE/owner 통계를 대조했다. CD 첫 검색/do조건 순서도 별도 검사했다. 재접속004a7510은MP 미완료로 이월한다. [826 기록](docs/exe/cpp-mana-registration-checking-20261104.md).
- [x] **군도 첫 시드 조회 범위 정정(827):** 첫 영역이라는 오기를 raw+5/record[3] 시드로 고치고 두caller가 군도 분기임을 정적으로 확인했다. 제품/빌드/검사0이며 함수 복원 완료는 아니다(3절 4번 군도 항목). [827 기록](docs/exe/cpp-terrain-first-seed-scope-20261104.md).
- [x] **제단 해방 부모/현재 권한(828):** CPU 부모1596행/진단468·재관측·집중7개(새3) 실패0으로 현재 checking·저장 finder·진단 복귀/현재 범위·plain P/가상 공간과 실제 Client caller를 대조했다. 하위 세 진단/현재 권한은 정적 연결·빌드이며 실제 포획/희생/해방 사제 몸체는 실행하지 않았다. [828 기록](docs/exe/cpp-altar-release-parent-20261104.md).
- [x] **숫자 송금 확인/현재 대상 수명(829):** CPU 비멀티432행/진단112·재관측·집중9개(새4) 최종 실패0으로 현재 checking/진단·현재 대상 재조회/저장·실제 모달/Client SP·AI 지갑을 연결했다. [829 기록](docs/exe/cpp-money-confirm-checking-20261104.md).
- [x] **UserInput 고정 송금 checking/원문 위치(830):** 세 checking을 고정1000/700/300 AI 가지로 정정하고 실제 Userinput.cpp 세 줄/현재 Client 서비스·진단 복귀를 연결했다. 전체 입력/다른7진단·Console UI 완료가 아니다. [830 기록](docs/exe/cpp-userinput-money-checking-20261104.md).
- [x] **재접속 State caller 범위 정리(831):** 004b8cb0의 ForceServerDeath/StateReconnect 인자0/1·자동 서버 장애 caller와 실제 이름표를 정적으로 확인했다. 8번MP에 미완료 이월한다. [831 기록](docs/exe/cpp-state-reconnect-scope-20261104.md).
- [x] **공통 Halt 진단/현재 수명 복귀(832):** CPU2288행/진단258·재관측·집중8개(새3) 실패0으로 현재 checking·삭제 뒤 진단 정상 복귀·같은 Client/Host와 기존 실제 재지정을 연결했다. !deadFlag는 정적 미도달/CPU0행이며 원문stack 주소 자체는 별도 호스트 경계다. [832 기록](docs/exe/cpp-path-halt-checking-20261104.md).
- [x] **공통 도착/항상참 SID 진단 복귀(833):** CPU860행/진단462·재관측·집중12개(새4) 최종 실패0으로 현재 checking·원문0041be80의 항상1/범위 진단·같은 실제 경로/Host 삭제 뒤 현재 metadata 겹침 정리를 연결했다. isRobust false4곳은 정적 미도달/CPU0회다. [833 기록](docs/exe/cpp-path-arrival-checking-20261104.md).

- [x] **현재 Regular 조회/생성·PID 오류 경계(891):** 같은 장기3번호/실제 finder·공유 진단/현재 시각·예약15caller와 실제 Pop을 연결했다. Native1232/64행/재관측·null읽기·새7/고유40사례 최종 실패0·검사 경고 교정 후15.147초/경고0이다. 첫 장치1실패/필터 오집계/경고2와 재빌드를 보존했다. 보호2770개/950fixture 불변·새2개→2772개/952개·checking10 유지다. [891 기록](docs/exe/cpp-regular-current-20261105.md).
- [x] **군도 첫 시드/흩뿌리기 부품(892, 실제 상위는 미연결):** 양판본 raw+5·현재 진단/복귀·로컬 난수·중복/덮어쓰기를 복원했다. Native378행/재관측·새4/고유11사례 실패0·검사 경고 교정4재통과다. 보호2772개/952fixture 불변·새1개→2773개/953개·checking10 유지다. [892 기록](docs/exe/cpp-terrain-seed-20261105.md).
- [x] **군도 공유 성장/제거 부품(893, 실제 상위는 미연결):** 같은context 군도DWORD·두좌표X폭/caller시드·즉시3×3·성긴칸 가중/반복제거·배치영역 제거를 복원했다. Native1734행/전체재관측·새4/고유15사례 첫 실패0·두Release 빌드 경고/오류0·기존2773개/953fixture 불변·새1개 후2774개/954개·checking10 유지다. [893 기록](docs/exe/cpp-terrain-arch-helpers-20261105.md).
- [x] **실제 군도 생성 상위/장기 전역·caller 코드 연결(894):** 양판본 후보/옵션/크기 차이·시드/밀도/현재Player 배치·실제 도장/정규화/해제/군집 순서와공유span/Zone/Login owner를연결했다. 세Release경고/오류0·기존일반/공유16사례첫실패0·새MP/Native/fixture0·보호2774개/954fixture불변·checking10유지다. MP동작검증은제외했다. [894 기록](docs/exe/cpp-terrain-generation-root-20261105.md).
- [x] **CoreData 복원/현재checking·실제Client 코드 연결(895):** 4/4/2복원·local/remote/진단 복귀와CD차이·저장BYTE/Chat·두caller를 같은 장기 owner에 연결했다. Release1회 경고/오류0·기존 getter/장부8사례 첫 실패0·새 복원/MP/Native/fixture0·보호2774개/954fixture 불변이다. 892/894의004c0e20 완료 집계도 정정해 checking8/완료 부모39/부분4다. [895 기록](docs/exe/cpp-player-core-restore-current-20261105.md).

- [x] **주문 효과 예약/83VT 분배(890):** 세 효과의 중복/new28/공유 난수·float저장/실제 Host와 예약187의 같은+20을 연결했다. Native432×2/재관측·새4/고유24사례 첫 실패0·Release952.728초/경고0·교정/재빌드0이다. 보호2769개/949fixture 불변·새1개→2770개/950개·81표코드대응/MP메타맵2이월·checking10 유지다. [890 기록](docs/exe/cpp-bomb-postpop-dispatch-20261105.md).

- [x] **Pop 현재 세계 크기/ordered 비교(889):** 같은 Client 크기·float저장·NaN 보정과 늦은 시작점 조회를 연결했다. Native260행×4/재관측·새4/고유21논리사례 최종 실패0·옛 비용 기대1개를 원문캐시로 교정/비영400 추가·Release3회 경고0이다. 기존2768개/948fixture 불변·새1개→2769개/949개·checking10 유지다. [889 기록](docs/exe/cpp-pop-world-bounds-current-20261105.md).

- [x] **자산 bare cdecl 물리 분배(888):** recipe/NULL/진단 뒤 저장을 실제 Session/Teleport로 연결했다. 원문1036행/재관측·고유32개(새4) 최종 실패0·새 입력2곳 교정·제품/기대값 불변·Release3회 경고0이다. 기존2767개/947fixture 불변·새1개→2768개/948개다. 공중3종/Outpost 전용 실행은 제외·typed factory/전체Pop은 잔여·checking10 유지다. [888 기록](docs/exe/cpp-asset-constructor-current-20261105.md).

- [x] **UI50/51 cdecl 실제 분배(887):** 고정 함수 클래스·현재 표시 TYPE·marker/Host와 Teleport를 연결했다. wrapper32행/재관측·기존 UI642/593행·고유30개(새1) 첫 실패0·Release1회 경고0·보호2767개/947fixture 불변이다. 최초0~69 비영21행/고유20주소의 정적 분배는 연결됐으며 자산/전체Pop은 잔여·checking10 유지다. [887 기록](docs/exe/cpp-cursor-constructor-current-20261105.md).

- [x] **timer57 기본 cdecl(886):** 빈100칸/기한0/미작성flags와 실제 Session/Teleport 미부착 수명을 연결했다. 원문12행+NULL4/재관측·시각조회0·고유29개(새3) 첫 실패0·기존 수치Frame4416행·Release1회 경고0이다. 기존2766개/946fixture 불변·새1개→2767개/947개·checking10 유지다. [886 기록](docs/exe/cpp-bomb-timer-constructor-current-20261105.md).

- [x] **경로8 cdecl/미시작 수명(885):** 고정 함수 클래스·같은 실제 장치·미부착 소유권/Teleport 공통 복귀를 연결했다. 원문32행+NULL8/재관측·고유27개(새3) 최종 실패0·Release2회 경고0이다. 첫 미설치 역참조 뒤 관문/새 입력을 교정했고 기존2765개/945fixture 불변·새1개→2766개/946개·checking10 유지다. [885 기록](docs/exe/cpp-path-constructor-current-20261105.md).

- [x] **기본 Form 지정 슬롯 생성자(884):** 양판본 네 cdecl의 단일DWORD를 원문56행/재관측과 실제 Teleport로 연결했다. 고유24개(새2) 첫 실패0·Release1회 경고0·추가 교정/기존 기대값 변경0이다. 기존2764개/944fixture 불변·새1개→2765개/945개·checking10 유지다. [884 기록](docs/exe/cpp-form-constructor-current-20261105.md).

- [x] **EvilRay 실제 Host/현재 전역 분배(883):** 같은 실제 registry/Host·Kernel·TYPE/source owner·SHP/Client 진단·위치 음향/타원·Teleport 생성자를 연결했다. parent0 등록도 보관한다. 고유31개(새3) 첫 실패0·Release2회 경고0·기존 기대값/보호2764개/944fixture 불변이다. 게임/AI/전투 Kernel은 진행하지 않았으며 다른 생성자/전체Pop·checking10은 잔여다. [883 기록](docs/exe/cpp-evilray-current-host-20261105.md).

- [x] **실제 VFX 타원 래스터(882):** 원문270전체 화면/재관측·보호 바이트와 고유9개(새1) 첫 실패0으로 타원 윤곽/퇴화 선을 대조했다. 새 검사 BYTE 명시 뒤 Release 경고0·제품 추가 수정/기존 기대값 변경0이다. EvilRay 실제 Host/Session은 미연결이며 기존2763개/943fixture 불변·새1개→기준2764개/944개·checking10 유지다. [882 기록](docs/exe/cpp-vfx-ellipse-20261105.md).

- [x] **EvilRay 독립 Frame/Draw 요청(881):** 현재 source/형상·전체 delta/등호 조건·음향/꼬리/종료·dirty/clip/타원 요청을 원문36행/재관측·추가2행과 고유8개(새3) 첫 검사 실패0·Release 경고0으로 대조했다. 실제 래스터/Host/Session은 미연결이며 기존2762개/942fixture 불변·새1개→기준2763개/943개다. checking10 유지다. [881 기록](docs/exe/cpp-evilray-frame-20261105.md).

- [x] **EvilRay 독립 수명 부품(880):** 현재 TYPE/source 조회 위치·50입자/공유3색·Stop/소멸을 원문34행/재관측과 고유5개(새3) 첫 실패0·Release 경고0으로 대조했다. 실제 Session/Host/Frame/Draw 분배는 미연결이다. 기존2761개/941fixture 불변·새fixture1개→다음2762개/942개이며 checking10 유지다. [880 기록](docs/exe/cpp-evilray-lifetime-20261105.md).

- [x] **Teleport 원문 NULL 생성자(879):** 현재 TYPE13/22의 실제 진단/NULL 복귀와 같은 Client·공통 부모를 연결했다. 고유21개(새3) 첫 검사 실패0·새 검사 컴파일식 한 곳 교정 후 Release 경고0·기존 기대값/보호2761개 불변이다. 다른 process12항목/자산 생성자와 전체Pop은 잔여이며 checking10 유지다. [879 기록](docs/exe/cpp-teleport-null-constructors-20261105.md).

- [x] **내용물 부모 알림 현재 연결(878):** PATCH 선택 장치·효과 전 반환/현재 kind 뒤 위치 조회를 연결하고 CD 생성 계약을 유지했다. 고유29개(새2) 첫 실패0·Release1회 경고0·기존 기대값/보호2761개 불변이다. 실제 포획 알림 효과는 코드/정적이고 새 검사는 앞단 조회뿐, checking10 유지다. [878 기록](docs/exe/cpp-content-notice-current-20261105.md).

- [x] **의식 저장소/WORD 현재 연결(877):** 공통 WORD/Bridge 장치·PATCH 물리 쓰기/진단 복귀·판본별 Player 결과 주소를 연결했다. 고유27개(새3) 첫 실패0·Release2회 경고0·정적 TYPE 범위 추가·기존 기대값/보호2761개 불변이다. 의식 효과는 코드/정적이며 새 검사는 저장소/범위뿐, checking10 유지다. [877 기록](docs/exe/cpp-ritual-storage-current-20261105.md).

- [x] **다이스/제단 선택 장치(876):** PATCH 도달 장치/callable 수명·현재 사제 fallback을 연결하고 CD 생성 계약을 유지했다. 고유24개(새2) 첫 실패0·Release1회 경고0·기존 기대값/보호2761개 불변이다. 새 검사는 자격/무효과/위치 조회뿐이며 의식 효과는 코드/정적, checking10 유지다. [876 기록](docs/exe/cpp-ritual-devices-current-20261105.md).

- [x] **의식 helper 현재 소비자(875):** 현재 사제/권한·PATCH 선택 장치·Rune 물리 쓰기/진단 복귀를 연결했다. 고유22개(새2) 첫 실패0·Release1회 경고0·기존 기대값/보호2761개 불변이다. 새 검사는 일반 조회/효과 전 반환뿐이며 의식/생성/진단 본체는 코드/정적, checking10 유지다. [875 기록](docs/exe/cpp-temple-helper-current-20261105.md).

- [x] **Rune 번호/현재 미션 상태(874):** 같은 장기 번호와 현재 억제 getter·원문 Count 뒤 번호 조회를 연결했다. 고유24개(새4) 첫 실패0·Release1회 경고0·기존 Rune1920행/기대값·보호2761개 불변이다. 의식/생성/삭제 효과는 코드/정적이며 새 Session은 사전 검사만 조회, checking10 유지다. [874 기록](docs/exe/cpp-rune-shared-current-20261105.md).

- [x] **다이스/제단 현재 가상 진입(873):** 실제 PATCH VT와 선택 장치·현재 모드/권한/Count/판본별 Player 쓰기 주소를 연결했다. 고유18개(새4) 최종 실패0·기존 검사2곳의 관측 경계 교정·Release3회 경고0·보호2761개 불변이다. PreDestroy 효과는 코드/정적이며 실제 희생/포획/가상삭제0, checking10 유지다. [873 기록](docs/exe/cpp-dais-class-current-20261105.md).

- [x] **생산 정보/너겟 실제 가상 진입(872):** 현재 네 번호를 실제 메뉴 owner에 연결하고 TYPE154 선행 조건을 실제 VT 분배로 교정했다. 고유13개(새4) 최종 실패0·새 검사 입력1곳 교정·기존 수동1단언 원문 교정·Release2회 경고0·보호2761개 불변이다. 원문 메뉴3888/너겟9019행은 불변이며 checking10 유지다. [872 기록](docs/exe/cpp-inventory-nugget-current-20261105.md).

- [x] **공통 너겟 번호·참조 연결(871):** 일곱 상태와 Client의 AI/덱/경로/의식 요청/선택을 같은 현재 owner에 연결했다. 고유17개(새4) 첫 검사 실패0·최초 빌드 경로 오류1곳 교정·교정 빌드 경고0·보호2761개 불변이다. 운반/의식 효과는 정적·컴파일이며 실제 의식/패킷0, checking10 유지다. [871 기록](docs/exe/cpp-nugget-type-consumers-20261105.md).

- [x] **Rune/다이스 현재 번호·참조 연결(870):** 보상과 같은 두 owner·const 참조 getter를 finder/참여자/AI/저장/메뉴/수명 소비자에 연결했다. 고유30개(새4) 첫 실패0·제품빌드1회/경고0·새Native/fixture0·보호2761개 불변이다. 수명/의식 효과는 정적/컴파일이며 실제 의식은0, checking10 유지다. [870 기록](docs/exe/cpp-rune-type-numbers-20261105.md).

- [x] **보상/일반 삭제 현재 TYPE 번호(869):** 같은 장기 owner의 다섯 번호와 Count 전후 판본별 TYPE 저장을 연결했다. 고유33개(새8) 첫 실패0·제품빌드1회/경고0·새Native/fixture0·보호2761개 불변이며 checking10을 유지한다. Rune/의식·운반/내용물/채집/건설/덱의 다른 번호 사본은 잔여다. [869 기록](docs/exe/cpp-reward-type-numbers-20261105.md).

- [x] **비행 홈 현재 진단/TYPE 코드 연결(868):**0045c100의 현재 표/공유005410ec/checking·두 진단 정상 복귀와 실제 두AI caller를 연결했다. 제품빌드1회/경고0·보호2761개 불변이며 전용 신규 검사/Native/게임0이다. 기능 검증 완료와 구별하며 checking 구현 잔여10이다. [868 기록](docs/exe/cpp-flyer-home-checking-20261105.md).

- [x] **투사체 현재 checking/Host 진단 복귀(867):**00482340의 두 현재 관문·같은Client/0041be80과 저장 표시/후반 발사자 재조회를 연결했다. 고유28개(새8) 첫 실패0·실제Host 삭제2개·제품빌드1회/경고0·새Native/fixture0·보호2761개 불변이며 checking11이다. Client진단UI/공격플레이 검증으로 확대하지 않는다. [867 기록](docs/exe/cpp-missile-checking-20261104.md).

- [x] **신전 event4/Descend 정적 범위 정정(866):** 실제 curServer·fort territory·입력명령0x02000013과State19를 확인해 메타맵 부모와 함께8번에 미완료 이월했다. 생성입구004dbac0과CD 대응도 기록했으며 단독 전체 도달불가를 주장하지 않는다. 제품/빌드/검사0·보호2761개 불변. [866 기록](docs/exe/cpp-temple-descend-scope-20261104.md).

#### 2.1.3 Unpop/Take·가상80·공통 Pop 현재 파생 연결 (834~865)

- [x] **Unpop 일반 공간/현재 진단의 부분 연결(834):** CPU1300행/진단670·재관측·집중10개(새3) 실패0으로 저장 TYPE/spot·현재 checking/TYPE·void→해시→표시와 같은 Client/World/Session을 연결했다. 기존3618단계/VT151도 통과했다. contained 종단은 835에서 연결했다. [834 기록](docs/exe/cpp-squid-unpop-checking-20261104.md).
- [x] **공통 contained 종단/실제 입력큐(835):** 004ac6b0→004ad230의 실제 혼합링크·부모통지·UpdateContents39 예약과 World/Session weak대여를 연결했다. 집중19개(새11) 최종실패0·빌드2회 종료0·새CPU/fixture0·보호2759개 불변. 834 CPU contained60행 자체는 외부 요청 관측이며 835~836의 실제 부품 연결 검사와 구별한다. [835 기록](docs/exe/cpp-squid-contained-unpop-20261104.md).
- [x] **실제 Take Unpop/현재 TYPE·PostTake(836):** PATCH의 모든 !free/void 진단 호출과 현재 state·constructor→TYPE→void·zOrder/후단 진단 복귀를 같은 World/Session에 연결했다. 집중35개(새8) 첫실행 실패0·기존 fixture 불변으로 004afe50 완료다. Factory 자체 진단/CD 별도 경계는 남는다. [836 기록](docs/exe/cpp-factory-take-unpop-20261104.md).
- [x] **객체 메뉴 직접 가상80의 현재 VT 분배(837):** TYPE 생성자 VT쌍 강제를 제거해 실제 Summary/회수에서 현재 지원 VT를 선택한다. 새3/집중6 최종 실패0·빌드2회 종료0·보호 불변이다. [837 기록](docs/exe/cpp-object-count-vtable-20261104.md).
- [x] **제단 중첩 현재 가상80/Session 공유(838):** typedDais 현재 VT 분배·명시 virtual80 소비자 공유·activeSID RAII를 연결했다. 집중8개(새2) 첫실행 실패0·빌드1회 종료0·보호 불변이다. [838 기록](docs/exe/cpp-nested-count-vtable-20261104.md).
- [x] **가상80 getter 읽기 정의역(839):** Has의 현재 Slot/free·예약 SID 읽기와 DaisCount의 현재 TYPE/frame 범위를 원문으로 교정했다. Level3584/Count1920 명시 입력/새3·집중13 첫실행 실패0·기존 원문96행 불변이다. setter 쓰기 계약은 별도다. [839 기록](docs/exe/cpp-current-count-getter-20261104.md).
- [x] **다이스 checking/내부 현재 가상Count(840):** CanBridge00449710/BridgeCommand00449900의 선행 내용물 finder·진단 복귀 뒤 현재 TYPE/owner/kind/동맹·권한과 NULL Path/저장 depth·Stage/EscapeFrame/StartSacrifice/FinishSacrifice의 확인된 내부 가상80을 연결했다. 부속 MoveAwayDepth00427d30 포함. 새6/집중17 첫 실행 실패0·보호2759개 불변이며 두 부모 완료로 checking12다. 효과 함수는 정적 대조·컴파일만 확인했다. [840 기록](docs/exe/cpp-dais-checking-20261104.md).
- [x] **Take SID/TYPE 진단과 실제 상세 owner(841):** 원문1a7·상세00436d20·문자열0/1ba 정상 복귀와 현재 Unpop/생성자·Client/World2대여·비멀티 Session을 연결했다. 집중27개(새8) 첫 실행 실패0·Release2회 종료0·보호2759개 불변이다. Factory 전체/CD·Form 등은 잔여다. [841 기록](docs/exe/cpp-factory-take-diagnostic-20261104.md).
- [x] **공통 Pop 현재 조회/부분 효과(842, 부분 완료):** 현재TYPE·저장입력범위·spot/grid 진단 복귀·늦은SHP·해시/현재firstPop을 같은 World/Session/Frame에 연결했다. 집중40개(새15) 최종 실패0·보호 불변이다. 전체 Pop 완료/checking 감소로 세지 않는다. [842 기록](docs/exe/cpp-squid-pop-parent-20261104.md).
- [x] **Activate/현재 writer/같은 전송 물리(843):** 현재 서버·SID별 flags·전송 전후 상태·같은 깊이/392 정상 복귀와 83VT의 실제 35BYTE/압축 writer를 World/Session에 연결했다. 집중51개(새11) 첫실행 실패0·보호 불변이다. MP/패킷 전용 검사0이며 전체 Pop/checking 완료로 세지 않는다. [843 기록](docs/exe/cpp-squid-activate-20261104.md).
- [x] **Rift 모양/현재 TYPE 대여(844):** 00419850의 signed DWORD 두 배 감김·checking 무관 상세/e1 정상 복귀·선택 TYPE/저장 frame/현재 SHP를 실제 FrameSize/Client 상세와 World/비멀티 Session에 연결했다. postPop/Graph/이웃은 현재 TYPE 배열을 명시 대여한다. 집중97개(새15) 최종 실패0이며 최초 기존 검사6개(18단언)의 옛 무효과 가정만 원문 기준으로 교정했고 fixture 입력/기대값·제품 추가 수정0이다. [844 기록](docs/exe/cpp-rift-shape-current-types-20261104.md).
- [x] **공통 postPop 현재 부모·선택 재검사(845):** 004b0d30의 메타맵 선행/현재 그래프 활성·TYPE 재조회/80b 정상 복귀·저장 genus/비용→현재 공급→AI/owner·TYPE 통계→실제 선택 재검사와 004da650의 현재 mouseType/finder/Select(found,0,1)/원래 found의 현재 extra를 같은 그래프/장부에 연결했다. 실제 PATCH 공통 앞 전체 Validate는 제거했다. 집중130개(새17) 첫 실행 실패0·보호 불변이다. [845 기록](docs/exe/cpp-post-pop-current-parent-20261104.md).
- [x] **Carrier/Damageable 현재 부모(846):** 00427720/0044bf80의 선택 장치·현재 TYPE/로딩/간헐천/extra/flags1·최초004ad080 접근과 owner→localOwner 순서를 원문 위치로 연결하고 PATCH 공통 전수 Validate를 제거했다. 집중135개(새14) 첫 실행 실패0·기존 Carrier7344행 유지다. 중간 중단 이력은 [중단 인계](docs/exe/cpp-carrier-current-parent-wip-20261104.md). [846 기록](docs/exe/cpp-carrier-current-parent-20261104.md).
- [x] **Factory 현재 분배/선택 장치(847):** 선택 예약/생성·프레임·경고/삭제의 부분 효과와 두 현재 가상80→TYPE/저장 프레임·같은 실제 Session을 연결했다. 집중77개(새13) 첫 실패0·빌드1회 종료0·보호2759개 불변이다. [847 기록](docs/exe/cpp-factory-current-parent-20261104.md).
- [x] **Factory 현재 TYPE 전역(848):** 세 전체 DWORD의 장기 owner·현재 예약 비교/프레임 제외와 같은 Session/기존 소비자를 연결했다. 집중85개(새8) 첫 실패0·빌드1회 종료0·보호2759개 불변이며 두 Session 실제 예약 Form을 확인했다. [848 기록](docs/exe/cpp-factory-type-globals-20261104.md).
- [x] **Residence/Geyser 현재 부모 연결(849):** 선택 장치·부분 효과/물리 쓰기·signed 목록·실제 현재 finder·WORD 통지를 연결했다. 최종108개(새21) 첫 실패0·빌드2회 종료0·보호2759개 불변이다. [849 기록](docs/exe/cpp-asset-current-parents-20261104.md).
- [x] **NotifySurface 현재 TYPE/진단 복귀(850):** 004214a0의 현재 TYPE/checking·진단 정상 복귀/자체 연결 반납과 같은 실제 World/Session·Client 장치를 연결했다. 원문2264행/재관측·집중119개(새9) 첫 실패0·기존2759개 불변/새fixture1개다. CD의 버려지는 포인터 계산은 진단/raw 미접근을 유지했다. 기존51함수와 별도 부속 완료다. [850 기록](docs/exe/cpp-building-notify-current-20261104.md).
- [x] **Gun·제네레이터·장식의 선택 장치/현재 부모(851):** 실제 가상 진입의 도달한 장치·앞선 부분 효과·Sun WORD/현재 Bridge 통지와 같은 Session Graph 진단을 연결했다. 집중105개(새15) 첫 실패0·빌드1회 종료0·기존2760개 불변이다. [851 기록](docs/exe/cpp-asset-gun-current-20261104.md).
- [x] **바리케이드·두 펜스의 선택 후처리/직접 부모(852):** 상위 실제 가상 진입의 일괄 Validate를 제거하고 선택 장치/부분 효과·직접 Damageable와 같은 Session 예약을 확인했다. 집중120개(새15) 첫 실패0·빌드1회 종료0·기존2760개 불변이다. [852 기록](docs/exe/cpp-asset-fence-current-20261104.md).
- [x] **성장 아이스·썬 바리케이드·너겟 현재 후처리(853):** 선택 몸체/현재 물리 DWORD·프레임 읽기 순서와 같은 Session 예약을 연결했다. 집중138개(새15) 첫 실패0·빌드1회 종료0·기존2760개 불변이다. 명시 Validate/실제 본문을 분리했고 기존 guard 단언만 교정했다. [853 기록](docs/exe/cpp-asset-tail-current-20261104.md).
- [x] **Vortex 진단 복귀/현재 Player·목록 후처리(854):** 같은 실제 가상 진입·원문 진단/현재 Player·WORD/중복 목록·물리/선택 장치와 Session 기록 쓰기를 연결했다. Native936행/재관측·최종161개(새20) 통과·제품 추가 수정0·기존2760개 불변/새fixture1개다. 기존51함수 밖 부속 완료다. [854 기록](docs/exe/cpp-vortex-current-20261104.md).
- [x] **공중 생산 시설의 선택 후처리/중복 진단 코드 연결(855):** 알려진 RawAssetPostPop20부류의 일괄 사전 검사 제거를 마치고 세 중복 진단의 현재 Client 서비스 복귀를 정적/코드로 연결했다. 제품 빌드/기존 비공중161개 통과·새 전용 검사/Native0이며 공중3종 동작 검증은 하지 않았다. [855 기록](docs/exe/cpp-aviary-postpop-current-20261104.md).
- [x] **다리·보호막 현재 접두/선택 장치(856):** 실제 VT 분배·도달 장치·실패 앞 쓰기/확보/예약과 공통 부모·같은 Session 예약을 연결했다. 집중178개(새17) 첫 실패0·다리240/보호막 Prefix384행·기존2761개 불변이며 새 Native/fixture0이다. 보호막 Regular의 별도 진단/수명은 범위 밖이다. [856 기록](docs/exe/cpp-post-prefix-current-20261104.md).
- [x] **buried·Rune 현재 후처리(857):** 물리 QA/현재 Bridge 통지·실제 VT/선택 장치·현재 시간 전역과 같은 Session 예약을 연결했다. 집중52개(새19) 첫 실패0·기존 buried580/Rune1920행·보호2761개 불변이며 새 Native/fixture0이다. [857 기록](docs/exe/cpp-buried-rune-current-20261104.md).
- [x] **Player 현재 부모/선택 장치(858):** 실제 VT/물리9기록·예약/직접 프레임·현재 생성/진단/typed Find를 같은 Session에 연결했다. 집중67개(새16) 첫 실패0·기존 Player1680행/실제 Host·보호2761개 불변이며 새 Native/fixture0이다. Client 서비스는 expression만 전달하므로 원문 파일/행 표시 전체 일치로 확대하지 않는다. [858 기록](docs/exe/cpp-player-current-parent-20261104.md).
- [x] **Dude 현재 부모/생성(859):** 물리 QA/현재 Bridge 통지·signed population/현재 프레임과 생성 관문을 같은 Session에 연결했다. 집중95개(새19) 첫 실패0·기존 Dude288행·보호2761개 불변이며 새 Native/fixture0이다. [859 기록](docs/exe/cpp-dude-current-parent-20261104.md).
- [x] **Teleport 두 입자 생성 함수/현재 TYPE(860):** 실제 VT/현재 TYPE+1d0 선택과 Streak/Fiery 생성·표시 부착/도착 삭제의 같은 전역을 연결했다. 고유50개(새16) 최종 실패0·최초 도우미/좌표 준비 오류를 검사만 교정·제품 추가 수정0·기존 Streak8/Fiery62/도착306행·보호2761개 불변이다. 다른 생성 함수 분배는 잔여다. [860 기록](docs/exe/cpp-teleport-current-constructor-20261104.md).
- [x] **섬·표면 현재 접두/진단 복귀(861):** 물리 프레임/필수 검색·현재 좌표/TYPE·선택 장치와 실제 World 상세/진단/TypeWas 읽기를 연결했다. 집중 고유63개(새20) 최종 실패0·최초 기존3개/3단언은 검사2파일만 교정·기존 표면1656/연결10800행·보호2761개 불변이다. MP 수신 writer와 전체 Pop은 남는다. [861 기록](docs/exe/cpp-island-surface-current-20261104.md).
- [x] **사제 postPop 현재 접두/실제 가상 진입(862):** PATCH 일괄 Validate/할당 확인 제거·물리 슬롯·signed 목록·도달 장치와 현재 사제 TYPE DWORD(회복 분자/최대HP 1/4 비교)·VT만의 분배를 같은 Session에 연결했다. 고유90개/14묶음 최종 실패0·기존 접두7176/후반6144/Pop3840행·보호2761개 불변이며 새 Native/fixture0이다. 최초 새 검사 입력 오류와 기존 PATCH Pop 노후 guard는 검사 파일만 교정했다. [862 기록](docs/exe/cpp-priest-postpop-current-20261104.md).

- [x] **현재 TYPE 표의 보상 HP/사제 상태(863):** 실제 PATCH Session/사제 World가 같은188TYPE를 빌려 최대HP/HP플래그/다이스 부류/genus를 읽는다. 독립/CD 사본과 PATCH 별도 정수 비용 캐시는 유지한다. 고유49개/9묶음(새6) 첫 실패0·Release2회 종료0·보호2761개 불변. 다른 TYPE 번호/Regular/전체 Pop은 잔여다. [863 기록](docs/exe/cpp-reward-current-types-20261104.md).

- [x] **보호막 Regular 현재 접속/진단 복귀(864):** PATCH 도달 장치·현재 checking·진단 복귀/처리기 반납과 같은 사제 TYPE·실제 Session typed finder/VT 분배를 연결했다. 고유31개/6묶음(새10) 최종 실패0·Release2회 종료0·보호2761개 불변. 독립/CD 계약과 전체 Host 틱/다른 Regular는 구별한다. [864 기록](docs/exe/cpp-forcefield-regular-current-20261104.md).

- [x] **최초 정수 비용표/현재 TYPE 분리(865):** PATCH 공통 후처리와 보상이 같은 불변 GameAssets 최초 비용을 읽고 실제 World/Session 재대여 뒤에도 변한float를 캐시로 재생성하지 않는다. HP/플래그는 현재 표를 유지한다. 고유44개/8묶음(새5) 최종 실패0·Release2회 종료0·보호2761개 불변. [865 기록](docs/exe/cpp-loaded-cost-table-20261104.md).

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
| 옵션 저장 | 1224 대응은 완료. 공유 삭제 거부 시 이전 파일/changed/복구 파일을 보존하고 실패 안내. 외부 매핑이 남으면 성공 백업/교체 폴더도 남을 수 있음. 966 실제CRT부분전달/버퍼실패·1176/1177상태복구대조완료.물리디스크고장/OS자체반환은호스트미관측 |
| PlayerNoMake | 경고255DWORD/정리 `004328a0` 연결은453 완료. 수신 생성/검증 패킷 부모 `00433c30` writer는 후순위 MP(3절8번). CD `00487900`은 spot 정리이며 같은 함수가 아님 |
| Graph/글꼴/사용자 제보 | 대표 Graph와 캐시 수명은721~724, 비ASCII 글꼴 경계는774, 제보 2건은677/678 완료. 2.1절에 명시한 나머지 레이아웃/글꼴/플레이 한계만 남김 |

## 3. 앞으로 할 작업 (재개 후 권장 순서)

**사용자 작업순서(2026-11-06 재개):** 3절 앞부터 진행한다. 1번929/2번943/3번956 정리 뒤 현재4번 내부 수명을 진행한다. 957~961의 공통/부착 대조에 이어962에서 생산 접두/젬 중립·표면 분배,963에서 사제Owner 현재 진단/부분효과·실제 Client를 복원했다. 964에서 신전 missingIsland 상세/실제 Client 진단을 연결했다. 965에서42cd10을폐기CD확인으로분류했다.다음966도4번options부분쓰기/교체실패와Factory/파일잔여다. **4→7→8 순서를 유지한다.** 모든 자연 상태/조합·픽셀 동일성은5·6번 사용자 확인이며 단계 전환을 막지 않는다. 아래 `[ ]`가 AI 구현 잔여이고 완료 범위는2절이다.

### 1. 대상별 커서·오브젝트 우클릭·게임 UI — 비멀티 구현/부품 대조 완료(929)

- [x] **건물·특수 대상의 현재 메뉴/명령 연결:** 원본83VT의 작성13입구와 비멀티81VT의 열기5입구를 대조했다.913~919·923~924의 소유자/레벨/손상·생산/회수·특수 대상·선택/Form 반납 근거를 유지한다. 성공 회수/연구/업그레이드 요청은 실제 송신 부모에 연결되어 있으며 새 송수신 검사는 하지 않았다. 전체 자연 상태/포획·희생/공격·외관은5·6번 사용자 확인이다.
- [x] **운반체 커서/현재 상태 연결:** 전체8종의917·918·928 근거와 실제 후보/Graph·방향별 동맹·내용물/자격·강조/명령 연결을 확인했다. 현재Graph6조합·96폴링/48직접평가에서 지상 이력/벌룬·무효254 동등도 확인했다. 실제 집기/이동/포획/희생 요청의 추가 실행 완료로 세지 않는다. 일반전투 earlySacrifice/보상창은8번이다.
- [x] **게임 메뉴·HUD/Chat·Console/모달 구현:** 기존 단축키와920~922·925~928의 현재 메뉴/도킹/버퍼·3모달 삭제순서/부분 생성 실패·메인 생성 실패5접두/재구성 근거를 유지한다. 전체 조합·픽셀은5번, 물리 입력/장치·다른 모니터/DPI/실제PCM/CD는7번이다. 실제OS 메모리 고갈·반복 장치 실패·전체SEH/미정 메모리와10.72 전체UI는 이번10.78 구현 완료로 주장하지 않는다.
- **정리 근거:** [929 기록](docs/exe/cpp-ui-implementation-closure-20261105.md). 현재 비멀티 메뉴 진입 누락0이며 새 제보가 있으면 구체적인 AI 수정 항목으로 재개한다. 모든 원본 동일성이나 모든 미관측 예외의 검증 완료를 뜻하지 않는다.
- 활성 caller 없는 관리명령·grid/debug는 기본 UI에 노출하지 않는다. 퇴역명령의 대조한 무효과와 현재 special/개발자키 조건을 유지한다.
- RequestMenu 서버 전송·메타맵 열기2개(`004789b0`/`00479390`)·c6(Refresh Zones)/db(Declare a Draw)·원격Player/일반전투 의식 UI는8번이다. 특수 대상의 Dig Up·전환/다리/Meltdown/Detonate·미니맵/Send Money·c2(Test Battle/편집 복귀)/c5(Leave)의 기존 연결을 반복 미구현으로 세지 않는다.

### 2. Edit 메뉴·커스텀 맵/시나리오 — 원문 구현/대조 정리(943)

- [x] **시나리오 설정/UI writer·TARC/동적Config·비멀티 명령 연결을 대조했다(940).** 기존687/715의 이름/큐 근거와 실제 현재파일/동적치환/HTML메뉴/설정쓰기·읽기실패반납/재생성을 확인했다. 영문549문서의 치환980곳 중 비멀티11명령967곳은 기존 연결, MP/관리10명령11곳은8번 이월, 원본오자2곳은 보존한다. 임의 사용자 입력의 모든 조합/전체동일성·모든동적실행의 증거로 확대하지 않는다. [940 기록](docs/exe/cpp-scenario-dynamic-config-20261106.md).
- [x] **편집 도구·저장/재파싱·이름/목록·파일/전환 실패 수명을 대조했다(930/941~943).** 실제저장공유거부/선행snapshot/재저장,양판본객체적재/지연파일/공개guard·Guide반납,섬/그림/이동/손상파일반복·Clear실패와현재LeaveEditMode소비자를확인했다.716의연속왕복·726의owner저장·724의Graph근거를유지한다.현재확인된미연결비멀티Edit진입점은없으며모든조합/외관/OS부분I/O·SEH/Factory전체검증으로확대하지않는다. State3특이분기/연속미션전환은3번,내부전체는4번,시나리오UI/기본english자동생성은8번이다. [943 기록](docs/exe/cpp-editor-implementation-closure-20261106.md).
- [x] 원문 언어 UI/폰트 재생성·다른 언어 파일 실패 수명을 대조했다(931, [기록](docs/exe/cpp-language-closure-20261105.md)). 번호/Xlat/요청/기존 Chat 알림은 710~712·787 완료이며 `languageChanged` 소비자는 Chat 하나뿐이고 폰트 재생성은 `ChangeFontFace`에만 묶여 있음을 확인했다. 590의 명시 `.french`는 프랑스어 UI 완료가 아니고 한국어는 6차다.
- 원문 목록 첫50개 상한은634의당시기준이며5차934에서무제한/50행스크롤로이미변경했다. 실제 Fort 433개는 종류2가 432·종류0이 1개다. TEST01/TEST02 원본은 읽기만 하고 저장은 작업본에 한다. 10.78 기본 설명의 `{Text}` 리터럴(494)을10.72와 혼동하지 않는다.
- `.english` 부재 Test Battle 종료는 원본 복원 단계에서 유지하며 기본파일 자동 생성/시나리오 UI·부재 종료 개선은8차에서 한다.

### 3. 미션 수명·진행 저장·데모 반복

- [x] **공통교체/Retry·Next Tutorial·Next Mission·Abort의현재연결과연속수명을대조했다(944/945).** 원문286/831행·실제Client/Session3세대의참조/등록/네기한/Kernel/Template/반납·재설치와실제Early1새진입2+Replay2/Continue/Abort메인왕복을확인했다. Next의이름/큐·Begin/Fork기존근거를유지한다. 자연승리뒤진행저장·전체AI/다리재고·모든미션조합동일성은5·6번사용자확인이며정상State3/단독State7·LeaveBattle다른분기는아래셋째항목이다. [945 기록](docs/exe/cpp-mission-abort-replay-20261106.md).
- [x] **세 데모의 두 번째 전체360초/세 번째 월드/정리(946~948):** 각실제MCP한프로세스에서자연두만료·Start+360/새HUD/월드1→2→3·세번째비정지게임/벽시각10초초과·ExitDemo/메인/Quit종료0을확인했다. DemoStormRages/GuardMyBack의세안내약15초와DissolvedAlliance의Demo섹션/실제안내부재를대조했다. 처음실행/후처리실패는946기록에별도보존하고947/948은첫실행/동일적재도구로통과했다. 기한/시계/AI/승패조작없음·자료불변·제품939고정이다. [948 기록](docs/exe/cpp-demo-second-alliance-20261106.md). 949에서공통중단저장경로를교정했고950에서편집Guide조회/제목을교정했고951에서State3반환계약을교정했고952에서실제본문/CoreData를연결했고953에서기술/사제·정상확정을확인했고954에서Tick/실제State1→7을확인했고955에서LeaveBattle양수이유/CD·State6분류를대조했고956에서Native/State/cleanup과State2를대조해아래비멀티범위를정리했고다음957은4번이다.
- [x] **LeaveBattle 비멀티 양수 이유·CD/State6 분류(955):** 이유2의직접next3와이유3/9의선행/공통두삭제·부분효과를새PE22행/실제공용부모로대조했다. CD원문은차이기록이며전체10.72이관완료가아니다. 원격페널티/State6은8번으로구분하고검사하지않았다. [955 기록](docs/exe/cpp-battle-leave-reason-20261106.md).
- [x] **비멀티 Native3/State1·2/단독State7·cleanup Tick/연속전환(949~956):** 실제본문/기술·사제/정상확정·새예약지연/부분Fort공개와State2직접진입을연결해대조했다. 956은수정전거부→실제Session/파일/Deck/UI/확정통과·cleanup34case/현재owner감사·고유38검사/Release경고오류0이다. 자연특이State3/직접State2도달·모든조합/픽셀동일성은미확인이고5·6번사용자확인이다. 큰내부/진단·파일/Factory잔여는4번,일반전투/State6/재접속/Zone0/메타는8번이며신규MP검사없음이다. [956 기록](docs/exe/cpp-mission-state-scope-20261106.md).
- 정상 State7의 복사/로그·World 교체·7/8 확정(660), State1 공통 앞단(661), 현재 Guide 요청과 실제 Early1 전환(713·714)은 완료다. 일반 전투 저장/복구·StateReconnect·서버 이전·Zone0/메타 Poll·시계 패킷은 8번이며 기존 질문/삭제 UI를 유지한다.

### 4. 내부 수명·파일·checking 실제 연결

**현재 연결 기준:** 809~833·867~868의 실제 checking 소비자, 835~836의 공통 Unpop/Take, 840의 CanBridge00449710/BridgeCommand00449900, 842~865·869의 공통 Pop 현재 파생 부분 연결을 완료했다(2.1.2·2.1.3절). 잔여8=부모3+부분2+MP3이며 자세한 완료/미완료 범위는 [함수별 공백](docs/exe/cpp-checking-consumer-gaps-20261103.md)을 따른다.

- [x] **checking 현재 비멀티 부모/진단 연결 대조(957):** 실제공통Pop입구/현재TYPE·SHP·checking/부분효과·장기Client/World장치와현재83표를대조했고기존31검사가통과했다. **전체checking 잔여8(부모3/부분2/MP3)·완료부모39/부분4는불변**이며전체8함수구현완료가아니다. 8번에함수별잔여를보존했다. 895의0048fb30/004c0e20완료와897의간헐천부분표시를유지하며다른공통Pop파생/공유수명은아래계속진행한다. [957 기록](docs/exe/cpp-pop-current-scope-20261106.md).
- [ ] **공통 Pop 현재 파생 나머지:** **962에서 생산 갱신 접두/젬 중립·표면의 현재 가상 분배를 실제 World/Session에 연결했다. 새128행/고유13검사·실제4진입을 확인했다. 963에서 사제Owner 현재 진단/부분효과·실제 Client를 복원했다. 964에서 신전 missingIsland 상세/Client 진단을 연결했다. 965에서42cd10의폐기정책을정리했다.966에서옵션부분전달/복구상태를대조했다.967에서현재색표FILE/기존표시소비를연결했다.다음968도Factory/파일·비멀티수명과남은색표소비이며전체파생완료는아니다.** 사제 접두/가상 진입은862, 보상HP/사제상태의 실제 현재 TYPE 표는863에서 완료했다. PATCH 별도 비용 캐시와 실제 최초 표 대여는865 완료다. 보상/일반 삭제의 다섯 TYPE 번호는869 완료이며 Rune/다이스의 두 번호·참조 연결은870 완료이며 공통 너겟/덱·의식 요청 참조는871 완료이며 생산 정보/너겟 실제 가상 진입은872 완료이며 다이스/제단 현재 가상 진입은873 완료이며 Rune 번호/미션 상태는874 완료이며 의식 helper는875 완료이며 다이스/제단 선택 장치는876 완료이며 의식 저장소/WORD는877 완료이며 내용물 부모 알림은878, Teleport NULL 생성자는879, EvilRay 독립 수명은880·독립Frame/Draw 요청은881 완료이며 실제 타원 래스터는882·실제 Host/Session은883·기본Form은884·경로8 cdecl은885·timer57 기본은886·UI2는887·자산 bare 분배는888 완료다. 현재 세계 크기/ordered 비교는889, 누락효과3/예약187과83VT 분배/명시Validate 감사는890 완료다. Host/Regular의 현재 TYPE/상한·공유 진단과 실제 Pop 재진입은891 완료다. 군도 첫 시드/흩뿌리기 부품은892·공유 성장/제거 부품은893·실제 상위/root·caller 코드/정적 연결은894 완료이며 MP 실행 검증은 제외했다(0.4절). 896에서찾은간헐천+8c004854b0은897에서현재defaultFrame/실제로더와복원했다. 현재표시실효마스크3인81표/0인메타맵2표이며다른checking/전체Pop은잔여다. 비멀티 Missile의 두 관문은867, 비행 홈은868 코드/정적 완료다. 신전event4는866의정정에따라8번미완료이월이다. Teleport의 알려진 비영 생성 주소 분배는888까지 연결했으며(함수포인터0의base대체없음), 현재+20분배81표의코드와897의표시81표/MP메타맵2표미연결을구별하며공유TYPE/모드전역·다른Regular/전체Pop을대조한다. 보호막 Regular의 현재 접속/진단 복귀는864 완료이며 독립/CD 계약은 별도로 유지한다. 다른 공중 진단/행동은 코드/정적만 한다. [862 기록](docs/exe/cpp-priest-postpop-current-20261104.md).
- [ ] Factory의 CD Take/contained/Form과 고정 Form 편의 경로·나머지 부모/쓰기·진단 경계를 대조한다. 896의 PATCH PostCreate,898의 TYPE HP/현재 Factory,899의 Create/현재 raw TYPE 생성자,900의 보상 HP,901의 표시/전송 HP,902의 연소점 HP·정수 부모,903의 회복/Help HP,904의 양판본 공통 초기화,905의 양판본 Take 부모,906의 실제 Form3/6 생성/번호,907의 Process/Gump 현재 Factory·대여,908의 양판본 Form Unpop 현재 진단/VT·종속 분리,909의 CD 공간 Unpop/진단 경로,910의 감시 생성 부모/현재 번호·실제 클래스 수명은 완료다. 911의 공통 Form 부착+74 현재 TYPE/VT·진단/부분 링크도 완료다. 912의 공통 Form Pop 원문 진단·현재 권한/깊이·실제+20도 완료다. 958에서정렬Content를복원했고959에서Process파생접두/현재공통후단을복원했으며, 960에서상위부착/갱신·생성부분실패를복원했고961에서공통Owner현재값/진단을복원했다. 962에서생산접두/젬중립·표면현재분배를연결했고963에서사제Owner현재진단/부분효과를복원했다. 964에서신전missingIsland상세/Client진단을연결했다. 965에서42cd10의폐기정책을정리했다.966옵션/967색표FILE은완료이며다음968도같은4번Factory외부heap/다른파일·색표소비잔여다. 자산+74는Owner ret4로분리하며파생Owner/프로세스·외부 힙 반환·할당 실패 UI 등과 전체 Factory 완료를 구별한다.
- [ ] 비멀티 파생 프로세스·State/Session의 남은 수명과 전체 팔레트 전환·Chat 모드별 표시·시스템뉴스 UI를 연결한다. 원격/메타맵·desc는8번이다.
- [ ] 경로 고리/FileSpec·BaseFile의 **분류만 했거나 아직 연결하지 않은 소비자**, 최초 FILE/리소스 I/O·다른 쓰기/키/스트림·TAFF/보조 폴백·손상/할당/부분 I/O 실패 수명을 대조한다. 이미 실제 연결한 reader/FontStore/SHP·Edit 쓰기는 2.1절을 따른다. [785의 직접 74caller 분류](docs/exe/cpp-secondary-sound-root-20261103.md)는 모든 부모 구현 완료가 아니다. 등록 snapshot/무진단 optional·OS 직접 ReadFileBytes 계약을 보존한다. **965에서42cd10은폐기CD확인7f/80으로분류해일반비멀티FILE복원목록에서제외했다.** **967에서43c4e0/!color.dat의실제호출과CRT/장기9표·기존표시소비를연결했다.두8행/나머지색표실제소비전체와FILE/SEH한계는다음968의대조범위다.**
- [x] **options.cfg CRT/교체실패 구현·집중대조(966):** 실제파이프부분전달/버퍼실패·changed/반납과C/H의1176/1177상태별복구를확인했다.물리디스크고장/OS자체1176·1177/모든commit·close실패는호스트미관측이며구현미연결로세지않는다. [966기록](docs/exe/cpp-options-save-failures-20261106.md).
- [ ] 원문 FILE/FileSpec/SEH와 호스트 소유 경계의 남은 차이를 판정한다. `0044fa00`의 재접속 저장 소비자는 `004b54e0`과 함께8번이다. CD 일반 열기는 `004ba5d0`, 부분 파일은 `004ba630`이며 자동 정밀 후보의 반대 대응을 따르지 않는다(652). 실제 UNC 서버/다른 드라이브 상대CWD 등 미관측 경계도 보존한다. **다음968은남은색표/파일소비자/Factory·비멀티수명이며옵션966/색표967집중검사를조건없이반복하지않는다.**
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

- [ ] **3차 TCP/IP 로컬 멀티플레이·Outpust**를 원문 근거로 사전 구현한다. 신규 단위/게임/통신 검사는 하지 않는다. 공식 서버 복구·구 클라이언트 유효성 검사는 연결하지 않는다. **폐기CD확인7f/80(42cd10)은965의검증비활성화정책에따라복원하지않는다.**
- checking전체잔여8(957재대조,완료부모39/부분4불변): 부모004a7510·004b02d0의메타맵표시0050c9f0/0050cc08·004dc220(event4/Descend),부분004b54e0/004b8cb0,MP분류0042bc10/004b4df0/004b8b20. 기존분류이며직접inSingle관문부재를모든단독도달불가증명으로확대하지않는다. 신규MP검사없이원문구현을이어간다.
- MP 이월 목록: 공통+20 미연결VT0050c9f0(TYPE108/109)/0050cc08(TYPE161), RequestMenu 전송 `004d712e..004d717c`, 메타맵 열기 `004789b0/00479390`·번개 `00479650`·Challenge 이미지/텍스트 `00478d10`/참가자 삭제 `0042bc10`, c6/db·ChalSailPos 모드6·선행클릭/항해 메뉴·desc, 비미션 의식/랭크·조기 희생 안내, PlayerNoMake writer `00433c30`, frame22/주기64·timeout/UI·endpoint/server/boss·리슨/수락/22칸 송신, Activate의 별도 수신자/삭제 선택·프로세스 writer 전체/decoder·재동기화의 허용/owner 캐시 부모(843), 섬·표면 TypeWas의 MP 수신 writer(861).
- 신전 `004dc220` event4의fort territory0046f650/0046f5a0·curServer0054db4c·Descend0x02000013→004b3760/004b3620/State19와 생성입구004dbac0/0x030000a7을 포함한다(866). 기존 removeAnimation/regionGeneration 임시명은 실제 의미가 아니며 원문 부모 전체를 사전 구현할 잔여다. 직접inSingle 관문이 없으므로 단독 전체 미도달을 증명한 것은 아니다.
- 일반 전투 `!state.dat` 저장/복구·saveBattle/metamap/비mission phase14·StateReconnect/서버 이전`004b8cb0`(ForceServerDeath/StateReconnect·자동 장애 caller,831)·Zone0/메타 Poll/`004c1e70` 시계 패킷·`004b54e0` 파일명/count 진단과 `0044fa00` 소비자·`004a7510` 참가자 재접속/팀 재편·실패 진단·LeaveBattle 원격 페널티(004cbab0/004b4ac0/004b4b40/004bfb50)와State6의일반Fort/결과복귀를 포함한다(661/955의 분류). 기존 질문/삭제 UI는 유지한다. 정적 사전 구현과 검증 완료를 구별한다.
- 일반전투 State7 `004b6dd0`의 최소/최대밀도 옵션getter(`0041c9b0/0041c9e0`)·게임난수/강제밀도bb4 writer와중앙섬metadata/전체MP부모는잔여다.894는실제생성root/장기owner의코드연결이며이writer나MP동작검증완료를뜻하지않는다.
- [x] **4차 로컬 MCP·해상도(935~937 완료: MCP 서버 조작/상태/화면 캡처/소리 포함 녹화, 16:9 최대 1920×1080. 남은 것은 덱 검은 직사각형 원인과 실제 전체화면 확인):** 모든 게임 조작·캡처·소리 포함 녹화, 16:9 최대 1920×1080을 제공한다. 그래픽 크기 스케일 조정은 이 단계의 요구가 아니다.
- [x] **5차 사용자 저장 경로·무제한 맵 목록(932~934 완료: 설정/`!state.dat`/전체화면 표식/GIF·BMP/진단/Editor 작업본·장부/글꼴 캐시/PieceList 이관, 목록 무제한·50행 스크롤, 읽기 전용 설치 실행 검증. 4차 MCP 산출물 경로와 `CustomMaps` 배치 확인은 4차/사용자 확인으로 이월):** 모든 자동 생성 파일/디렉터리와 reader를 `%USERPROFILE%\Saved Games\Netstorm\` 아래로 옮긴다. 설정/진행은 Saves, 신규 맵은 CustomMaps. 원래/사용자 맵 병행 읽기·목록 스크롤/무제한을 구현한다. [733 조사](docs/exe/cpp-phase5-file-inventory-20261102.md)의 12파일군/50후보와 이후 추가 writer(HTML PieceList·옵션 임시/복구·!state/전체화면 표식·Edit 장부/잠금/pending·GIF/BMP·squids/procSumm·MCP 생성물)까지 포함한다. 이관 우선순위·두 설치 충돌/장부 identity·명시 export 정책을 정하고 일반 권한/읽기 전용 설치·다른 CWD/Unicode·저장/재적재/중단/교체/전체화면 재실행을 확인한다.
- [ ] **6차 한국어(938 1차 완료: 글꼴·번역표 502항목·Language 메뉴·`.korean`→`.english` 대체·배포 동봉. 남은 것: 미션 브리핑/팁/도움말 `.korean` 본문(AI/SP 구성은 english에서 복사), 한글 메인 버튼 그림, 번역 검수, 한글 입력):** 메뉴/문자열/시나리오·글꼴을 지원한다. 글꼴은 D2Coding이며 명칭은 AGENTS.md를 따른다.
- [x] **7차 AA/FSR1.0(939 완료: Direct3D 11 후처리 출력기·AMD 공식 EASU/RCAS·옵션 토글(기본 off)·FSR 해상도 2560×1440/3840×2160(논리 1920×1080)·MCP presented 캡처. 남은 것은 미션/전체화면/4K/입력 역변환의 사용자 확인과 최적화):** Options에서 각각 선택·기본 off. 지원 환경은 2560×1440/3840×2160을 추가하고 이 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 업스케일한다.
- [ ] **8차 Edit 시나리오 UI:** 초기 자본/테크·적·동맹 등을 별도 편집 UI로 수정하고 새 맵의 기본 english 자동 생성·english 부재 Test Battle 종료를 개선한다. 기존 수동 대본·저장/재적재를 보존한다.
- 배포 스크립트 자체는 완료다. 배포할 제품이 정해지면 README/BUILD_INFO를 갱신하고 그 제품의 ZIP·압축 해제 실행을 확인한다. 현재 검증 ZIP은644 제품이며 이후 제품의 배포 확인으로 쓰지 않는다.
- 60/120프레임·CD10.72 전체 복원/Windows 10 구동 불가 해결책은 별도 후순위다. Linux는dotnetpj 후순위이며cpppj는Windows 대상이다.

## 4. 운영 규칙과 유효한 사용자 결정

- **현재 진행 지시(2026-11-06 사용자 재개):** 3절을 앞부터 진행한다. 1번929/2번943/3번956 정리 뒤현재4번이며965완료·제품965·전체미완료다. 다음966도4번options부분쓰기/교체실패와Factory/파일잔여다.4→7→8순서·제외검사·단위인계/커밋·push금지를유지한다.961의사용자일시정지는이번재개로해제됐다.
- **단계 전환(2026-10-31):** 모든 자연 완주·전체 동일성은 사용자 확인이며 AI 잔여가 이것뿐이면 다음 단계로 간다. Windows 10 확인 뒤 Windows 11 실기 없이 넘어간다. 사용자가 오류/차이를 제보하면 해당 건을 AI 작업으로 올린다.
- **실행 빈도(2026-10-31):** 더 많은 기능 묶음 뒤 필요한 통합 확인을 한다. 소수 단위마다 전체 34·긴 Early 완주·전체 CTest/71대상/ZIP을 반복하지 않는다. 옛 3~5단위 기준은 자동 실행 조건이 아니다. 새 실제 실행은 변경과 직접 관련된 필요성을 먼저 판단한다.
- **검증 기록:** 정적 대조·명시 입력·컴파일·C++ 검사·GUI/물리 관측을 구별한다. 최초 실패/교정 원시를 보존하고 기대값을 제품에 맞춰 약화하지 않는다. 다른 제품의 성공·준비본을 현재 제품 통과 수로 합산하지 않는다. GUI/분석기 실행 720의 범위는 [720 기록](docs/exe/cpp-editor-graph-native-20261102.md)을 따른다.
- **CHEAT/테스트 조작:** CHEAT은 Edit 편집에서만 각 원문 조건으로 노출한다. `--test-bridge-patterns`·명시 로컬 AI/SP 등은 검사 조작임을 주석/기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다.
- **실행 파일/자료:** Windows 실행 파일은 `Netstorm.exe`다. CMake 대상명 NetstormCpp·OUTPUT_NAME Netstorm·ARCHIVE_OUTPUT_NAME NetstormCpp를 유지한다. 옛 `NetstormCpp.exe` 로그는 당시 이름으로 보존하고 원본/클론 프로세스는 전체 경로로 구별한다. 무인자 GUI·`--help/--run`·분석 CLI를 유지하며 EXE 옆 `game/netstorm.tarc`를 우선한다. cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다.
- **보호:** AGENTS.md·originals/originalCD 기존 파일은 수정/삭제하지 않는다(AGENTS의 명시 예외만 허용). 게임 실행 전후 원본 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild18.x를 쓴다. `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`로 빌드하며, 선행 CMake/MSBuild 자식이 끝난 뒤 한 번에 하나만 실행한다. 단일 빌드 핸들을 추적하고 빌드 중 소스를 바꾸지 않는다. exit_code0 확인 후 의존 검사를 한다. HJOW-X3D는 VS17(2022)/v143이며 BuildTools cmake의 전체 경로를 쓴다. [빌드 안내](docs/cpp-build.md) 참조.
- **UTF-8:** PowerShell 5.1에서 Python stdin으로 한국어를 보낼 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정한다. cp949 오류 시 `PYTHONUTF8=1/PYTHONIOENCODING=utf-8`을 설정한다. 원시 로그/Native fixture를 인코딩 정리로 덮어쓰지 않는다.
- **Git:** 이 문서/CPP 작업 파일은 기존 줄바꿈을 유지하고 확인/stage에 필요 시 `git -c core.autocrlf=false`·`-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정은 바꾸지 않고 최초 관측 TSV의 `-text`를 보존한다. DN/WB는 해당 문서·`.gitattributes` 규칙을 따른다. 단위별 인계 후 `YYYYMMDD-NN PJ 요약`으로 로컬 커밋한다. 당일 마지막 번호를 확인하며 cpppj=CP·dotnetpj=DN·webpj=WB·공통=CM이다. push는 금지다.

## 5. 참고 문서와 이관 기록

- [C++ 빌드](docs/cpp-build.md), [로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md), [모듈 맵](docs/exe/modules.md)
- [디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-11-03 양판본 디컴파일·컴파일](docs/exe/cpp-decompilation-20261103.md), [2026-10-23 양판본 디컴파일](docs/exe/cpp-decompilation-20261023.md), [analyzeManager](docs/analyze-manager.md)
- [준비본 보관](docs/exe/prepared/README.md): 443/444와 469의 일부는 후속 단위에 통합됐다. 보관본의 미적용/미빌드 문구를 현재 상태로 쓰지 않으며 base/patch/manifest/142파일 원시는 보존한다.
- 과거 전체 원문: [504까지](LEFT_JOBS.old.md#archive-20261027-through-504), [549의 0절](LEFT_JOBS.old.md#archive-20261029-section0-through-549), [561의 3절](LEFT_JOBS.old.md#archive-20261029-section3-through-561), [638까지](LEFT_JOBS.old.md#archive-20261031-through-638), [695까지](LEFT_JOBS.old.md#archive-20261102-before-cleanup), [808까지](LEFT_JOBS.old.md#archive-20261103-through-808), **[861까지·이번 정리 직전](LEFT_JOBS.old.md#archive-20261104-through-861)**.
- **이번 정리(2026-11-04, HJOW-Athlon, 사용자 요청):** 정리 직전 원문을 보관 파일 끝에 그대로 추가하고 기존 보관 바이트를 유지했다. 단위834~861의 단위별 인계 이력(옛 0.4~0.31절)·대체된 재개 지시·해소된 다음 작업 예고와 3절에 중복된 완료 항목을 보관본으로 이관했다. 0.32절의 최신 인계는 0.4절로 옮겼다. 흩어져 있던 완료 항목은 2.1절의 세 묶음으로 모았고, 완료됐지만 목록에 없던 844·845·846을 `[x]`로 추가했다. 단위마다 반복되던 검사 제외 문구는 0.3절 한 곳으로 모았고, 미해결 MP 이월(843·861)은 3절8번에 더했다. 제품861/11.01·EXE 해시·마지막 디컴파일 호스트·일시 정지는 유지했다. 문서/링크/보관 원문 해시만 확인했으며 빌드/게임/기능 검사/디컴파일은 하지 않았다.
- 이전 정리(2026-11-03)는 단위808까지를 같은 방식으로 이관했다([보관본 머리말](LEFT_JOBS.old.md#archive-20261103-through-808)).

## 인수인계 문서 분담

cpppj/공통/디컴파일은 이 문서, dotnetpj는 LEFT_JOBS.dotnetpj.md, webpj는 LEFT_JOBS.webpj.md에 기록한다. cpppj 1차·Windows 10/11 호환·MCP가 완료돼 dotnetpj 구현에 활용할 수 있게 되면 인수인계를 LEFT_JOBS.md로 다시 통합한다. 현재는 프로젝트별 문서를 유지한다.
