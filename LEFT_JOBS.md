# LEFT_JOBS — cpppj 및 공통 인수인계

**현재 제품: 1197 / 버전 11.01. 목표는 진행 중이며 전체 미완료다.** 사용자 2026-11-09 `/goal`의 순서는 **M6 → M7 → M8 → K1~K5 → S1~S4 → B1~B4**다. 1196의 인계 정리 구조를 유지한다.1197에서 장기 건설 본체를 연결했고 다음은 큰 NoWorld/AI/Carrier·삭제다.

이 문서는 현재 상태와 남은 작업의 기준이다. 완료 과정·만료된 중지/재개 안내·옛 다음 작업은 [1195까지의 정리 전 원문](LEFT_JOBS.old.md#archive-20261109-through-1195)에 보존했다. dotnetpj와 webpj의 작업 목록은 각각 [DN 인계](LEFT_JOBS.dotnetpj.md), [WB 인계](LEFT_JOBS.webpj.md)를 따른다.

## 0. 현재 어느 정도인가

AGENTS.md의 최근 변경 `551d27ae`에 따라 진행 요약은 세 프로젝트를 **실행·플레이 가능 여부**와 **원본 동일성**으로 나눠 평가한다. 아래 DN/WB는 각 인계 문서의 현재 설명을 읽은 요약이며 이번에 새로 실행한 결과가 아니다.

| 프로젝트 | 실행·플레이 가능 여부 | 원본과의 동일성 |
| --- | --- | --- |
| **cpppj** | 무인자 GUI·메뉴·미션 플레이 경로가 있다. 최신 제품 1197에서 Tutorial4 생산·건설 요청, 실제 TYPE75/owner1 생성, 600프레임 대기와 정상 종료0을 확인했다. | 부품별 C/PE·fixture 대조와 제한된 실제 플레이 확인을 누적했다. 모든 시나리오 자연 완주·전체 화면/음향 동일성은 사용자 확인이다. 전체 복원 완료로 판정하지 않는다. |
| **dotnetpj** | 기존 `Netstorm.Game`/`Netstorm.exe`는 1-1·1-2 근사 플레이가 가능하다. 새 `Netstorm.Client.Host`는 메뉴 일부까지이며 미션 플레이가 불가능하다. | 새 경로의 실제 메뉴·게임 캡처 비교는 아직0건이다. R3 진행/R4 일부 이식이며 부품 fixture 통과를 전체 게임 동일성으로 합산하지 않는다. |
| **webpj** | 클릭하여 시작·메뉴/목록·도움말/옵션·Credits·개발용 지형 미리보기까지다. 실제 미션 플레이는 불가능하다. | 메뉴 일부와 부품을 대조했다. cpppj 오라클 M1 8장면은 기록한 원인 밖 픽셀 차이0이며 구름 등 차이는 별도로 남았다. 같은 미션 L2/L3·전체 화면/음향·자연 완주는 미검증이다. |

### 0.1 cpppj 최신 근거

| 항목 | 현행 값 |
| --- | --- |
| 제품 구현 커밋 | **1197 /11.01**. 이번 단위의 커밋 번호는`extracted/cpp-resume-1197/commit.json`에 기록한다. 직전 제품1195는`0317d9ea /20261109-63 CP`다. |
| 최근 완료 | **1195:** 실제 삭제 bool·Native Runtime의 장기 Storage 연결. **1197:** 전체 Native body를 Client에 보유하고 실제 장기 SP/HP/Canon/지도/목록/현재Player·25개 큰 효과의 weak 대여/선택 pin·Host 반납 후 효과 수명을 연결했다. 큰 NoWorld 전체와M6는 미완료다. |
| 빌드·정적 확인 | 1197 NetstormCpp/NetstormRoot/세 집중 대상 Release/x64 종료0·경고오류0, 고정8소스 불변. 정적18조건/C·PE10함수를 대조했다. 제품7파일은 최초 빌드 뒤 불변이며 새 검사 준비만 보완했다. |
| 집중 실행 검사 | **16+35+명시17=68검사 실패0/신규3.** World/Player 할당 전 본체·같은 backing/주소, 현재 효과 pin 중 외부 반납/예외·재대여, 최종 무순환 해제를 확인했다. 최초 새 검사3개의 현재 카메라/view 누락을 검사 준비만 수정하고 최초 로그를 보존했다. 결합 Notice/패킷2개 제외·신규MP/패킷/통신 실행0이다. |
| 최신 GUI | **1197 최종 제품:** Tutorial4 정상 생산·건설 요청→실제TYPE75/owner1 생성→600frame 대기, 같은Client800×600 Edit TEST01→Main 왕복2회, 모두 정상 종료0. 건물 완성·전체 NoWorld 삭제/Test Battle/MP/포획·희생/특정 공격/승리/공중3종 검증은 아니다. |
| 전체 회귀·배포 | 마지막 전체34경로 진입은 **645에서 검증한 제품644**다. 현재 제품의 전체 솔루션 실행 통과 기록은 없다.1166은 기존 검사 컴파일만 완료했다. 최신 ZIP은 **1092**이며1197 배포물이 아니다. 최종 패키지 정합성은M8이다. |
| checking·송신 상태 | checking 잔여 목록0개, M6 태그25/활성88/무태그63F·큐OFF. checking0은 전체 MP 완료를 뜻하지 않는다. 중앙 원문 진단창은M7.5다. |
| 실행 중인 작업 | 없음.1197 빌드·68비MP 집중 검사·Tutorial4/같은Client Edit왕복2회는 모두 종료0이다. Source/태그25·활성88·무태그63F·큐OFF와ZIP1092를 유지한다. |
| 보호 기준 | 원본/CD/fixture2799와 개발자 커밋551d27ae의 AGENTS를 포함한 **2800파일 불변(1197)**. 원시 빌드/검사/GUI·소스 고정/보호 해시는`extracted/cpp-resume-1197/`에 있다. |
| 실행 파일 | Netstorm.exe 13701120바이트/SHA-256 `677d310919e08be152a7ab652f87151248abda23edc55110adbacf8c93c6250a`. R.exe 677376바이트/SHA-256 `3d81af3c2342bfe276fc8c3eee7934526f4c89d3543bec0b475f3b14edab329f`. |
| 마지막 정밀 디컴파일 | **Client 양판본: 2026-11-03 / HJOW-Athlon(192.168.0.94). R 전용: 2026-11-07 / 같은 호스트.** 현재 호스트도 같다. [기록](docs/exe/cpp-decompilation-20261103.md). |

1197 상세는 [본체 owner 설명](docs/exe/cpp-construction-body-owner-20261109.md)·[검증 보고서](cpppj/construction-body-owner-20261109.json)를 본다.1195 삭제와1194 상태 근거는 해당 설명에서 연결한다. 전체 동일성이나 완료율을 이 집중 검사 수로 계산하지 않는다.

## 1. 남은 양과 진행 순서

**다음 한국어 단계 전에는 큰 묶음3개(M6~M8)가 남았다.** M6 안에는 적어도4개 작업 갈래가 있고 M7은 독립5건이다. 그 뒤 한국어5묶음·시나리오UI4묶음·버그 수정4묶음, 즉 **후속13묶음**이 남는다. 전체는 **16개의 상위 계획 ID**로 관리한다. 각 묶음은 여러 구현/검증/커밋을 포함하므로 커밋 수·소요일·완료율과 같지 않다. 아직 큰 구현 작업이 남아 있으며 날짜/백분율 추정 근거는 없다.

<a id="agents-reflection-20261102"></a>

| 순서 / AGENTS 단계 | 현재 상태 | 남은 작업 규모·전환 조건 |
| --- | --- | --- |
| **현재: 3차 M6** | M6-a~k는1173까지 완료, l/m/n은 부분 접합 중이다.1197에서 전체 장기 건설 본체와 선택 효과 pin을 연결했다. | **큼.** 큰 NoWorld/AI/Carrier/다른 Form/일반 삭제, Source/등록표/Poll/잔여F·지연큐·JoinTestBattle가 남는다(2절). |
| **다음: 3차 M7** | 원문/차이 준비만 있으며 구현 미완료다. | 독립5건: helpWord, 메타커서, 희생 안내, marker, 중앙 assertion 창/파일(3절). |
| **3차 M8** | 미착수 마감 관문이다. | M1~M7의 활성 Caller/실제 owner·수명과 외부 검증 비활성화 전수 감사, Release 빌드, 필요한 최종 ZIP 바이트 검증. 새 MP 실행 검사는 하지 않는다. |
| **6차 K1~K5** | 938의 번역502·D2Coding·Language 메뉴·korean→english 대체·배포 동봉은 구현됐다.986 후보는 미적용이다. | **큼 / 5묶음.** 후보 선별 병합→부호화/IME/클립보드→미션 본문→메뉴 그림/문구→검수·배포(4절). |
| **8차 S1~S4** | 원본 Edit943과 별개인 신규 시나리오 편집 UI는 미구현이다.986 후보도 미적용이다. | **큼 / 4묶음.** 모델→기본english/부재Test Battle→실제 UI→홈 저장/재적재(4절). |
| **9차 B1~B4** | AGENTS `b5197d48`의 알려진 버그34건을 계획에 포함했다. 구현은 미착수다. | **큼 / 4묶음·34건 전수.** 원인/활성 Caller 분류→공통·비MP 수정→MP 정적 수정→허용 회귀/배포(4절). |

**이미 마감한 단계:** 1차 활성 비MP UI929·Edit943·미션956·내부 수명/FILE980,2차 호환 전체화면/재실행·화면비·가장자리 스크롤·저장과 덱 결함981,4차 로컬MCP935~937·와이드,5차 사용자 홈 저장/맵 목록932~934,7차 AA/FSR1.0 939는 구현 범위를 마감했다. 새 구체적 누락/사용자 제보가 생기면 해당 건만 다시 연다.

- 5차는 현재 `CustomMaps\d\NAME.fort`, 최초 Edit의 원본433맵 복사, 항목 수 무제한·한 화면50행 스크롤을 유지한다. 이동행 클릭/휠 미구현과 현재 배치는 임의 변경하지 않으며 이동행 한국어 문구는K4다.
- 7차는 AA/FSR 기본off,1440p/4K 자동FSR·논리1080p를 유지한다. 실제4K/활성 전체화면 물리 출력·입력 역변환과 성능 최적화는 별도 확인/후속이다.
- 3차→6차는 M1~M7 실제 같은 저장소/장치/Caller 연결과M8 마감이 조건이다.6차→8차는 표시 부호화/IME/클립보드를 시나리오 편집에 사용할 수 있어야 한다. 빈 callback·가짜 ACK·성공 상수로 닫지 않는다.
- 1~9차 뒤의 **별도 후순위**도 남아 있다:60/120프레임 지원, CD10.72 전체 복원/호환성. 그 뒤 세부 계획을 확정한다. dotnetpj/webpj/Linux 구현은 별도 범위다.

## 2. 바로 이어갈 M6

### 2.1 남은4갈래

| 우선순위 / 계획 ID | 해야 할 일 | 종료 근거 |
| --- | --- | --- |
| **1. M6-n: 큰 NoWorld·삭제** | Session 없는 실제 Client 설정/표시/진단 장치와 큰 AI/CarrierPreDestroy→ReplenishNuggets/실제 너겟 배치, 다른 Form/일반 삭제·전용 Regular 수명을 연결한다. | 같은 실제 Pool/목록/TYPE/현재 전역과 가상 부모를 사용한다.1195 bool/1197 장기 본체 연결만으로 전체 NoWorld 삭제 완료라 하지 않는다. |
| **2. M6-n: 전수 Caller·owner** | Source/listener/두 등록표/Poll/송수신·선택적 기록·25태그 전scope와 무태그63F·Snapshot/debugBurst를 대조한다. l/m 구현 뒤 최종 감사를 다시 닫는다. | 원문 등록92행/모든 실제 process VT와 현재 receiver/observer/version/길이·파일/해제 수명을 연결한다. 현재 없는 큰 장치나 빈 효과를 숨기지 않는다. |
| **3. M6-l: 실제 지연큐** | Client 장기 분포·OS시계/CRT rand·각 노드queue·현재20BYTE 주소 복원·실제 전송/등록 수명을 연결한다. | 원문4def40/4df010/4dee60/4def20/4df5e0/4dfc00. FIFO·엄격한deadline<now·원문 난수 소비/observer/기록·닫힘/해제를 보존한다. 기본queue를false로 닫지 않는다. |
| **4. M6-m: JoinTestBattle** | 개발자4이름 제한·현재 주소·server0/current29 뒤 기존1bh 전체 수신을 직접 호출한다. | 원문4b8be0→4b8b20, 입력0200002b의 선행4b3620을 보존한다. Edit TestBattle(c2)와 구별하며 새 수신 본문을 복제하지 않는다. |

원문 주소와 세부 근거는 [M6 감사](docs/exe/cpp-lan-remaining-audit-20261108.md)를 본다. **그 문서 본문의 초기 “없음” 판정은 당시 조사다.** 현행 첫머리·이 문서·단위 완료 근거를 함께 읽고 M6-a~k나 이미 연결한 수신을 다시 만들지 않는다.

### 2.2 다음 구현을 시작할 파일·수명 조건

- **1197 완료:** `ClientConstructionGlobals::Body`가 World/Player 구성 없이 실제 장기 Runtime/SP/HP/Canon/Chunks/최근·전체 목록/현재Player/Bounds 모드·방향·map/piece를 보유한다. Native Session은 본체를 빌리고 typed는 기존 지역 본체다. 현재 큰 효과25개는 weak 대여이며 기록기2개는 장기 Replay를 직접 조회한다. 다음은 `RawSurfaceStorage`/Session의 큰 NoWorld/AI/Carrier/너겟 배치·다른 Form/일반 삭제와 전용 Regular다.
- **1197 수명 계약:** body는 State/backing보다 먼저 반납하고 Storage→Gameplay alias의 참조 순환을 피한다. 현재 효과는 lock→실제 전체 Runtime pin→함수 사본→호출 순서이며 생성/직접 로드와 공개 후 호출을 구별한다. 실제 Host 반납이 정상 복귀한 뒤 짧은 효과 대여만 끝낸다. Source의 세 공개 경로는 같은 Session pin 장치를 사용한다. 큰 NoWorld 본문을 복원할 때 강한 Session 역참조·새 World/reset·임시 Player/성공 fallback을 넣지 않는다.
- M1~M5는 완료다: [메타22/25h](docs/exe/cpp-meta-sail-error-20261108.md)(1150), [로그인/기록/같은 전송 재진입](docs/exe/cpp-meta-battle-reentry-20261108.md)(1156), [Pop/전환](docs/exe/cpp-meta-transition-closure-20261108.md)(1159), [State33/영역 후속](docs/exe/cpp-meta-zone-descend-20261108.md)(1162), [두 환경/서버 페이지32섹션/reader](docs/exe/cpp-server-pages-integration-20261108.md)(1164). 원본 서버 본문 미확보의 한계와 새 호환 본문 계약을 유지한다.
- 기존 Pop/저장복구/78h/선택적 기록은 재작성하지 않는다. checking의004b02d0/004dc220은1157에서 기존 본체/실제 같은DWORD 연결을 확인했다. 중앙 assertion의 새 누락은M7.5에 남긴다.
- 실제 새R/TCP·UDP/같은 등록표/시계/Config/주소·기록 owner는1062~1093 근거를 유지한다. 제공R10.71의3BYTE 존과Client10.78의47BYTE 존은1093 compact 호환 범위다. 원본R 복사/실행으로 복원을 대신하지 않는다.
- 요청DWORD0054dbac는 reset0/1ah·1bh read 근거만 있다. 새Config/CLI/writer를 발명하지 않는다. 폐기CD7f/80·온라인 유효성검증·PIRATE quota는 복구하지 않는다. IPX/SPX는 명시 미지원,1163 hostname OS조회/첫LAN IPv4와 숫자주소 TCP/UDP를 유지한다. 원문에 없는 원격 관리 포트를 추가하지 않는다.

## 3. M7의 독립5건과 M8 마감

| ID | 남은 구현 |
| --- | --- |
| M7.1 | helpWord 캐시: 빈Config일 때 기존cache 유지·현재 정체성. |
| M7.2 | 메타커서 실제 본문/장치 설치·상태Lock/Unlock·장치 선행 순서. |
| M7.3 | 조기/원격 희생 안내. 구현·허용 정적 대조만 하고 사용자 제외 희생/MP 실행 검사는 하지 않는다. |
| M7.4 | marker의 실제 표시와 갱신. |
| M7.5 | 공통 assertion 창/파일 중앙화. 초기 registry 진단도 같은 중앙 장치에 통합한다. |

[M7.1/2 차이 준비](docs/exe/cpp-help-cursor-remaining-audit-20261108.md)는 구현/실행 통과가 아니다. M8은 M1~M7의 실제 활성 Caller·같은 owner·수명·현재 checking 목록0·새 누락·외부 검증 비활성화를 감사하고 Release 빌드와 필요한 최종ZIP의 실제 Netstorm/R/에셋 바이트를 검증한다. 작은 단위마다 전체 검사/ZIP을 반복하지 않는다. 사용자 제외 MP 실행이나 자연 완주만 남으면 다음 단계로 간다.

## 4. 3차 뒤의13묶음

### 4.1 한국어 K1~K5

| ID | 작업과 종료 조건 |
| --- | --- |
| K1 | 986 한국어 후보의 기준/현행 차이를 검토해41코드·배포·검사파일과44별도자산을 선별 병합한다. 옛 patch 일괄 적용/덮어쓰기는 하지 않는다. |
| K2 | `EncodeOriginalText`와 표시 부호화 경계를 유지하고 한글 입력/IME/클립보드를 구현·집중 검증한다. 시나리오UI보다 먼저다. |
| K3 | 미션 브리핑·팁·도움말 `.korean` 본문과 대체 경로를 제공한다. 영어의 AI/SP/시나리오 선언은 보존한다. 준비 자산을 전체 번역 완료로 세지 않는다. |
| K4 | 원본 스타일/크기·D2Coding·AGENTS 명칭으로 한글 메인 버튼 그림, 맵 목록 이동행, 누락 UI 문구를 적용하고 해당 화면에서 확인한다. |
| K5 | 기존 번역502와 새 본문/그림/입력을 구별해 검수한다. 관련 집중 검사·단순 비MP 진입·배포 정합성을 마친다. 자연 완주/공격/희생 검사는 제외한다. |

### 4.2 시나리오 편집 S1~S4

| ID | 작업과 종료 조건 |
| --- | --- |
| S1 | 986 시나리오 모델5파일·코드·필요 자산을 현재 편집기와 병합/등록한다. `scenario/PLAN.md`와`CMake-additions.txt`를 따른다. |
| S2 | 새 맵의 기본 `.english`를 자동 생성하고 english가 없어도 Test Battle 진입에서 종료되지 않게 한다. 기존 수동 대본을 보존한다. **이 항목은 보류가 아니다.** |
| S3 | 편집 모드 전용 UI에서 초기 자본/테크·적·플레이어·동맹 등 시나리오 구성을 수정하고 기존 맵 배치 도구와 함께 동작하게 한다. |
| S4 | 실제 사용자 홈·읽기 전용 설치에서 저장/불러오기·Editor/Test Battle 진입을 확인한다. 진입/대기/기본 조작으로 마치며 승리/포획/희생은 재현하지 않는다. |

준비본은 [986 안내](docs/exe/handoffs/cpp-pause-20261106/README.md)·[MANIFEST](docs/exe/handoffs/cpp-pause-20261106/MANIFEST.json)와 [한국어 FINAL_PLAN](docs/exe/handoffs/cpp-pause-20261106/korean/FINAL_PLAN.md)·[시나리오 PLAN](docs/exe/handoffs/cpp-pause-20261106/scenario/PLAN.md)을 본다.986 보관본은237파일이며 그중 **한국어/시나리오 후보는 제품 미적용·미컴파일·미실행**이다. 한국어 final.patch에는44자산이 없고 새 검사24개는 초안이다. 시나리오는24파일(변경23)/기준18/통합9·모델10검사 초안이며 모델5파일은scenario/base에 있다. shadow 패치 개행 일치를 컴파일 통과로 세지 않는다.986 원격Fort 준비분만은987에서 제품으로 별도 구현했으므로 다시 미적용으로 되돌리지 않는다.

### 4.3 알려진 버그 B1~B4

AGENTS `b5197d48`의 **34건 전체**가 대상이다. S4 뒤에 시작하며 이미 해결된 듯한 건도 원문·현행 근거 없이 닫지 않는다.

| ID | 작업과 종료 조건 |
| --- | --- |
| B1 | 34건마다ID·원본 주소/근거·현행 활성 Caller·필요 수정·허용/사용자 제외 검증을 연결한다. |
| B2 | 수명/버퍼/입력/에디터/시나리오/AI/전투/간헐천 등 공통·비MP 원인을 수정하고 필요한 허용 집중 검증을 한다. 포획/희생·특정 공격·승리는 사용자 확인으로 남긴다. |
| B3 | MP/클라이언트 동기화·상태·저장 수명을 디컴파일/필요 비교 판본으로 수정하고 컴파일·정적 대조한다. MP 단위/패킷/통신/게임 검사는 하지 않는다. |
| B4 | 34행의 구현·근거·검증·사용자 확인 범위를 마감하고 관련 빌드/비MP 허용 회귀/필요 배포 정합성을 확인한다. 제외된 직접 플레이 검사만 남으면 후순위로 간다. |

## 5. 실행 제외·미해결 검사와 정밀도 경계

### 5.1 현재도 유효한 제외/보류

- **사용자 확인:** 전체 동일성, 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리 재현. 이들만 남으면 다음 단계로 간다. 사용자 제보는 해당 건을 AI 수정 대상으로 다시 연다.
- **MP:** 원문을 참고해 구현·컴파일·정적 대조하되 신규 MP 단위/패킷/게임/통신/기록파일 실행 검사는 하지 않는다. 구현 생략이나 동작 검증 완료를 뜻하지 않는다.
- **공중3종:** 훨리기그·맨오워·더스트 데빌의 전용 생성/생산/행동 관측은 실행하지 않는다. 정적 분석/코드는 가능하다.
- **사용자 보류:** 스펠 효과 도움말 그림 추가 분석(530), 연녹색 직사각형 추가 분석. english 부재 Test Battle은 이 보류에 포함하지 않으며S2 대상이다.
- **환경/사용자 확인:** 물리 입력/캡처는762의 새 부팅/RDP에서도 두 번timeout이었다. 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·앱 전환·청취·활성 전체화면 물리 출력·실제CD 삽입·Windows11/ARM 실기는 별도 확인이다.
- 변경과 직접 관련된 단순 진입/대기/생산/건설로 끝나는 필요한 **비MP** 검증은 수행한다. 원본/복사본/클론·analyzeManager·별도 창 실행은 AGENTS의 현재 호스트 예외/명시 허용 범위를 따른다.

### 5.2 해결했다고 처리하지 않을 기존 한계

- 최신 전체 솔루션 실행 통과 결과는 없다.938의68집중 대상 중48완전 통과/20실패는 당시 결과다. `guide_release`151개와 `startup_file_names`3개(`names.Index()`)는6차 전`1f5b4177`에서도 같은 실패를 확인했다.
- 당시 원인 분류 잔여는 `file_tests`5,`pop_context`29,`menu_session`3,`mission_entry`1,`guide_fort_request`1,`renderer_presentation`1,`startup_*`/`file_directory` 경로 고리다. `world_generation`은1024×768 건물 그림/장치 미연결 계열이며 기준선 개별 비교를 하지 않았다. 후속 집중 성공을 최신 전체 재검사로 세지 않는다.
- `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은 일반Battle/MP 이월이며 새MP 검사는 제외한다. `CampaignForwardBattle_NaturalTempleAssault`549 실패는 사용자 자연 완주/별도 지원 범위다. 기존 전략/승리 단언을 약화하지 않는다. TEST02 로컬AI/SP·자동 공략 지원은 별도 요청 전 하지 않는다. `owner!=myPlayerId || demo!=0`를`gate.demo=1`로 우회하지 않는다.
- 해결된 FortObjectLoad 입력/필수Pop 관측942, SubscriberGlobals C2607의 실제 컴파일/기존7검사1052는 다시 실패 잔여로 올리지 않는다.
- 원문8비트/설정·경로의 `EncodeOriginalText`와 화면용 `EncodeDisplayText`/`DecodeDisplayText`를 구별한다. MakeAi owner0 이전/삭제·전략 차이, 설치 글꼴/다른ACP·구형무BOM 설정 자동 이관은 [글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md)를 따른다.
- 32비트 FILE/FileSpec/포인터·SEH/스택·미작성값 자체 일치, 임의 외부heap→SID, 실제OS 고갈/물리 디스크 장애·UNC/드라이브CWD·CD 전체dump/trace/Retype·x87 전체 입력/픽셀은 기존 호스트/후순위 경계다. 부분I/O/버퍼 실패/복구966·new_handler/종료979를 미착수로 되돌리지 않는다. 활성 Caller 미연결을 이 경계로 숨기지 않는다.
- PCM936은 HJOW-Athlon의 프로세스WASAPI101653프레임/비무음99726과 영상/소리 합성을 확인했다. 청취/다른 장치/실제CD/활성 전체화면까지 검증한 것은 아니다. 덱 검은띠981과 사용자 보류 연녹색 사각형을 혼동하지 않는다.

<a id="0-재개-전-확인-사항"></a>

## 6. 다음 작업의 운영 규칙

1. **착수:** AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 읽고 git status/fetch로 다른 세션 변경을 확인한다. 현재 호스트가 마지막 디컴파일 호스트와 다르면 CPP 작업/원문 분석 전에 `tools/ghidra/refine_all.ps1`을 다시 실행한다. R원문은 필요 시`tools/ghidra/refine_root_server.ps1`도 실행하고 문서/인계의 호스트를 갱신한다. HJOW-X3D의2026-10-27 중단 산출물은 완성본으로 쓰지 않는다.
2. **단위 종료:** 0.1 현행 값·해당 M/K/S/B 완료 조건·checking/최근 완료2건을 갱신한다. 상세는 단위 문서/보고서에 쓰고 현행 인계에 완료 로그를 계속 쌓지 않는다. 실제 완료와 준비본/정적 대조/컴파일/단위 실행/GUI/물리를 구별한다.
3. **Git:** 문서 작성 뒤 자기 파일만 `YYYYMMDD-NN CP 요약`으로 로컬 커밋한다. 당일 번호/다른 세션의 stage를 확인하고 DN/WB 파일·스테이징을 수정/정리/커밋하지 않는다. 커밋 실패해도 작업은 계속한다. **push하지 않는다.** 저장소/전역 설정과 최초TSV의`-text`를 바꾸지 않는다.
4. **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild18.x다. 아래4병렬 명령을 쓰고 단일 실행 핸들의 종료0을 확인한다. 빌드 중 제품 소스를 변경하지 않는다. HJOW-X3D는VS17(2022)/v143이다. [빌드 안내](docs/cpp-build.md).
5. **검증:** 최초 실패/교정 원시를 보존하고 기대값을 제품에 맞춰 약화하지 않는다. 변경 관련 집중 확인을 마친 뒤 다음 작업으로 간다. 옛3~5단위 수를 자동 전체34/CTest/ZIP 반복 조건으로 쓰지 않는다. 필요한 통합/최종 배포 확인은 해당 관문에서 수행한다.
6. **보호:** AGENTS.md·원본/CD 기존 파일·루트README 개요를 수정하지 않는다. 원본 실행 전후 해시/허용 설정 복원을 지킨다. 시작한 프로세스만 종료하고 이름 기반 일괄 종료하지 않는다. CHEAT/테스트 조작은 원문Edit 조건과 명시 검사 옵션으로 구분하고 기본AI/프로필에 섞지 않는다.
7. **제품 계약:** 대상NetstormCpp·OUTPUT_NAME Netstorm·ARCHIVE_OUTPUT_NAME NetstormCpp, Windows실행 파일`Netstorm.exe`,11.01/세 번째Patch Credits(HJOW·AI),무인자GUI/--help/--run/분석CLI를 유지한다.34경로(Early6·Training9·캠페인16·데모3) 범위를 유지한다. EXE옆`game/netstorm.tarc`를 우선하고 cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다.
8. **외부 접속:** 로컬TCP/IP를 제외한 외부 도메인/IP 기능을 문서화하고 폐기 온라인 유효성검증은 비활성화한다. [외부 정책](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json)을 기준으로 새 활성 Caller도 확인한다.
9. **언어:** 새 문서/코드는UTF-8, 응답/새 코드의 네임스페이스·상수·함수·클래스/구조체·멤버·반복문 설명은 한국어다. D2Coding/AGENTS 명칭을 따른다. 이 두 인계MD의 기존CRLF와 원시 로그/fixture 인코딩을 보존한다. PowerShell5.1 stdin은`$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`, 필요 시Python UTF-8 모드를 쓴다.
10. **요약/권한:** 진행 요약 시 cpppj·dotnetpj·webpj 각각의 플레이 가능성과 원본 동일성을 제시한다(AGENTS551d27ae).9차34건은B1~B4에 유지한다(AGENTSb5197d48). 이 문서가 현행 CPP 우선순위 기준이며 옛 로드맵/단위 보고서의 “다음”을 다시 미완료로 되살리지 않는다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

## 7. 근거와 보관

| 확인할 범위 | 문서 |
| --- | --- |
| 비MP UI·Edit·미션/내부 수명 | [929](docs/exe/cpp-ui-implementation-closure-20261105.md)·[943](docs/exe/cpp-editor-implementation-closure-20261106.md)·[956](docs/exe/cpp-mission-state-scope-20261106.md)·[980](docs/exe/cpp-internal-lifetime-closure-20261106.md) |
| 디컴파일/소스 신뢰도 | [디컴파일](docs/exe/cpp-decompilation-20261103.md)·[신뢰도](docs/exe/decompile-reliability.md)·[SOURCE_MAP](cpppj/SOURCE_MAP.md)·[모듈](docs/exe/modules.md) |
| 11.01·전체34경로 | [제품 계약](docs/exe/cpp-product-1101-20261101.md)·[TODO](TODO.md)·[미션 대응표](docs/gameplay/demo-campaign-map.md) |
| 과거 LAN 일반/서버/메타 완료 | [1147까지의 원문](LEFT_JOBS.old.md#archive-20261108-through-1147)과 각 단위의docs/exe·cpppj/JSON·Git 이력.1150~1195의 인계 요약/중지 안내는 아래1195 보관본을 본다. |

<a id="인수인계-문서-임시-분할"></a>

**2026-11-09 문서 정리1196:** 정리 전 현행 문서 전체를 [1195 보관본](LEFT_JOBS.old.md#archive-20261109-through-1195)에 바이트 그대로 추가하고 기존 LEFT_JOBS.old.md의 모든 바이트를접두부로 보존했다. 만료된 M2/M3/M4 세부 “다음” 예고·완료 나열과 중지 안내를 현행 작업 목록에서 제거했다.1195 제품·미해결 검사·실행 제외·준비본 미적용·후순위는 유지했다. AGENTS551d27ae/b5197d48 요구를 현행 요약·계획·운영 규칙에 반영했다. 제품 소스/EXE·원본/CD/fixture·AGENTS/README·DN/WB 인계·986준비본은 이 정리에서 변경하지 않았으며 새 빌드/게임/검사/MP/통신/디컴파일은 실행하지 않았다. [정리 검증 보고서](docs/exe/cpp-handoff-cleanup-20261109.json)를 참조한다. 정리 검증 보고서는1196 당시 제품1195/문서 기준이다. 후속 제품/우선순위는0.1절과2절을 따른다.
