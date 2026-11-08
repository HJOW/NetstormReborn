# LEFT_JOBS — cpppj 및 공통 인수인계

**현재는 AGENTS.md의 3차(TCP/IP 로컬 멀티플레이·Outpost) 복원 중이다. 다음은 M3.3b의 State19 준비·Descend/참가 후속 World/카메라/HTML·실제 진단 장치다.** 제품은 **1158 / 11.01**이다. M3.3a의 master 주소 읽기를 기존9 조기 재사용 뒤로 복원했다.

다음 단계인 **6차 한국어 후속**으로 넘어가려면 아래 **M3~M8의6개 필수 묶음**을 마쳐야 한다. M1은1150, M2는1156에서 완료했고4·5·7차도 이미 구현됐다.3차 뒤에는 **6차 잔여5묶음 → 8차 시나리오 UI4묶음** 순서다. 이 수는 작업 범위를 나눈 개수이며 커밋 수·소요일·완료율을 뜻하지 않는다. 일반 LAN/Outpost는 전수 대조에서 세부 작업이 더 발견될 수 있다.

상세 과거 이력과 만료된 “다음 작업” 예고는 [1147까지의 정리 직전 원문](LEFT_JOBS.old.md#archive-20261108-through-1147)에 보존했다. 현재 우선순위는 **이 문서의 계획 ID와 완료 조건**만 따른다. dotnetpj와 webpj는 각각 [별도 인계](LEFT_JOBS.dotnetpj.md), [별도 인계](LEFT_JOBS.webpj.md)를 따른다.

## 0. 재개 전 확인 사항

### 0.1 현재 제품과 검증

| 항목 | 현행 기준 |
| --- | --- |
| 진행 상태 | 사용자2026-11-08 `/goal`로 진행 중. 전체 목표는 미완료다. 다음은 **M3.3b**이며 3차 잔여는 **M3~M8**이다. |
| 현재 제품 | **1158 / 표시·파일·제품11.01**. State33 master의 같은44BYTE 주소를 TCP의 기존9 재사용 전에 훑지 않게 했다. 로컬 커밋은 Git 이력에서 확인한다. |
| 실행·플레이 | 무인자 GUI와 미션 플레이 경로가 있다. 마지막 전체34진입은 **645에서 검증한 제품644**이며 현 제품 전체 회귀 결과가 아니다. |
| 원본과의 동일성 | 부품별 원문 대조를 누적했다. 모든 시나리오 자연 완주·전체 동일성은 사용자 확인 대상이다. |
| 최신 빌드·정적 확인 | **1158 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0**. 원문 C/PE와 State33→실제 TCP의 주소/재사용 읽기 순서를 대조했다. [보고서](cpppj/meta-master-address-order-20261108.json)·[설명](docs/exe/cpp-meta-master-address-order-20261108.md). |
| 최근 완료 | **1157 / M3.1·M3.2:** 기존 Pop/신전 정기 Handle의 전체 부모·현재 checking 연결 대조. **1158 / M3.3a:** 실제 master 주소의 bounded view·TCP 조기 재사용 선행. [1157](docs/exe/cpp-checking-parent-closure-20261108.md)·[1158](docs/exe/cpp-meta-master-address-order-20261108.md). |
| checking 잔여 | **현재 목록0개.** `004b02d0`/`004dc220`은1157에서 기존 전체 본체/실제 같은 DWORD 연결을 확인했다. 모든 MP 경로 완료를 뜻하지 않으며 M3.3/M4~M8은 남는다. |
| 마지막 C++ 실행 검사 | **1052의 기존7개**. 이후 MP 작업에서는 새 단위/패킷/통신/게임 검사를 실행하지 않았다. |
| 마지막 GUI | **984:** 격리1920×1080 Tutorial1 진입·비정지100프레임 대기·WM_CLOSE 종료0. 포획/희생/공격/승리/MP 검증이 아니다. |
| 전체 검사·ZIP | 현재 전체 솔루션 통과 기록은 없다. 최신 ZIP은 **1092**이며 현재1158 바이너리의 ZIP이 아니다. 다음 패키지 정합성 작업은 **M8**이다. |
| 보호 기준 | AGENTS.md·원본/CD/fixture **2800파일 불변(1158)**. 기준 `extracted/cpp-resume-1158/protected-before.json`은 현재PC 전용이다. |
| EXE SHA-256 | Netstorm.exe 13,169,664바이트/SHA-256 `ab184250379d8db2a42f0b40d434c42ed1b3463313e25e312e188914f97314e0` / R.exe 670,208바이트/SHA-256 `c526c36cb3e2425f31a952304b614a9de0d5a9ba6dde0b9db2f10f6f3f03b729` |
| 마지막 정밀 디컴파일 | **클라이언트 양판본 2026-11-03 / HJOW-Athlon(192.168.0.94)**. R전용은 **2026-11-07 / 같은 호스트**. [기록](docs/exe/cpp-decompilation-20261103.md). |

### 0.2 재개 순서

1. AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 읽고 `git status`/`git fetch`로 다른 세션 변경을 확인한다. push는 하지 않는다.
2. 현재 호스트가 위 마지막 디컴파일 호스트와 다르면 CPP 작업/원문 분석 전에 `tools/ghidra/refine_all.ps1`을 다시 실행하고 호스트를 갱신한다. R원문은 필요 시 `tools/ghidra/refine_root_server.ps1`도 실행한다. HJOW-X3D의2026-10-27 중단 산출물은 완성본으로 쓰지 않는다.
3. **M3 → M4 → M5 → M6 → M7 → M8**을 기본 순서로 삼는다. 실제 의존 하위는 해당 묶음 안에서 먼저 복원한다. M1/M2와 이미 끝난 하위를 옛 예고만 보고 다시 만들지 않는다.
4. `extracted/cpp-resume-1158/`에는 State33/실제 TCP C/ASM·빌드/정적 대조 자료가 있다. 현재PC 보조 자료일 뿐 다른PC에 있다고 가정하지 않는다. 현재 소스/단위 문서가 기준이다.
5. 단위 종료마다 **0.1의 현행 값·해당 M/K/S 항목·checking 목록·최근 완료2건**을 갱신하고 단위 문서/보고서를 작성해 로컬 커밋한다. 과거 예고·완료 로그를 이 문서에 계속 누적하지 않는다.
6. 다른 세션의 DN/WB 파일·스테이징을 수정/정리/커밋하지 않는다. 문서 정리 다음에는 CPP 구현을 계속하며 목표를 완료/일시중지로 바꾸지 않는다.

### 0.3 유효한 실행 제외·보류

- **사용자 확인:** 전체 동일성, 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리 재현. 이들만 남으면 다음 단계로 간다. 사용자 제보는 해당 건을 AI 수정 항목으로 다시 연다.
- **멀티플레이:** 원문을 참고해 구현·컴파일·정적 대조하되 **신규 MP 단위/패킷/게임/통신 검사는 하지 않는다**. 이 제외는 구현을 생략하거나 동작 검증 완료로 쓰라는 뜻이 아니다.
- **공중3종:** 훨리기그·맨오워·더스트 데빌의 전용 생성/생산/행동 관측은 실행하지 않는다. 정적 분석/코드는 가능하다.
- **사용자 보류:** 스펠 효과 도움말 그림 추가 분석(530)과 연녹색 직사각형 추가 분석. `.english` 부재 Test Battle 문제는 보류가 아니라 **S2**의 구현 대상이다.
- **환경/사용자 확인:** 물리 입력/캡처는762의 새 부팅/RDP에서도 두 번 timeout이었다. 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·앱 전환·청취·활성 전체화면 물리 출력·실제 CD 삽입·Windows11/ARM 실기는 별도 확인이다.
- 단순 진입/대기/생산/건설로 끝나고 변경과 직접 관련된 필요한 **비MP** 검증은 수행한다. 전체34/전체CTest/ZIP을 작은 단위마다 반복하지 않는다. 실제 게임/별도 창의 실행 확인은 AGENTS.md의 현재 호스트 예외와 명시 허용 범위를 따른다.

## 1. 단계 지도와 전환 기준

<a id="agents-reflection-20261102"></a>

| AGENTS 단계 | 현재 상태 | 다음 전환을 막는 AI 작업 |
| --- | --- | --- |
| 1차 10.78 원본 복원 | 활성 **비멀티** UI929·Edit943·미션 수명956·내부 수명/FILE980 범위 마감. MP 전용은3차에서 복원 중이다. | 새 구체적 누락/사용자 제보는 다시 연다. 전체 자연 완주·전체 픽셀 동일성은 사용자 확인이다. |
| 2차 Win10/11 호환 | 호환 전체화면/재실행·화면비·가장자리 스크롤·저장 재시작을 구현/확인하고 다음 단계로 전환했다. 덱 결함981도 해결했다. | 다른 장치/Windows11/ARM 실기는 현재 전환 조건이 아니다. 무오류 보증과 구별한다. |
| **3차 TCP/IP 로컬 MP·Outpost** | **현재 진행 중. 제품1158. M1/M2 완료, 다음M3.3b.** | **M3~M8의6묶음.** 새 MP 실행 테스트는 종료 조건에서 제외한다. |
| 4차 로컬MCP·와이드 | 935~937 구현, 덱 보완981. 캡처/음향 녹화의936 후속 성공도 있다. | 물리/환경 확인은0.3절. 같은 기능을 다시 미구현으로 세지 않는다. |
| 5차 사용자 홈 저장·맵 목록 | 932~934 이관/reader/읽기 전용 설치·Unicode 실행·무제한50행 스크롤 구현. | 현재 `CustomMaps\d\NAME.fort`, 최초 Edit의 원본433맵 복사, 이동행 클릭/휠 미구현을 유지한다. 임의 배치 변경은 하지 않으며 이동행 한국어 문구는K4다. |
| **6차 한국어** | 938의1차: D2Coding·번역502·Language 메뉴·korean→english 대체·배포 동봉. **986 후보는 미적용**이다. | **K1~K5의5묶음.** 3차 M1~M8 뒤 착수한다. |
| 7차 AA/FSR1.0 | 939 구현. 기본off,1440p/4K 자동FSR·논리1080p를 유지한다. | 실제4K/미션/활성 전체화면의 물리 출력·입력 역변환은 사용자 확인, 성능 최적화는 후속이다. |
| **8차 Edit 시나리오 UI** | 원본 Edit943과 별개인 신규 기능. **986 통합 후보는 미적용**이다. | **S1~S4의4묶음.** 한국어 입력/표시 경계를 마친 뒤 착수한다. |

**3차 → 6차 조건:** M1~M7의 활성 원문 경로가 실제 동일 저장소/장치/호출자로 연결되고, M8의 구현 마감 감사·빌드·필요한 배포 정합성이 끝나야 한다. 빈 callback·가짜 ACK·성공 상수로 닫지 않는다. 사용자 제외 MP 테스트와 자연 완주는 기다리지 않는다.

**6차 → 8차 조건:** K1~K5를 반영하고 표시 부호화/IME/클립보드가 시나리오 편집에 쓰일 수 있어야 한다. 준비본 파일 수/초안 검사를 제품 통과로 세지 않는다.

요구 범위는 원본10.78의34경로(Early6·Training9·캠페인16·데모3), 기존 Edit/비멀티 메뉴·HUD/Chat·11.01/세 번째 Patch Credits·Windows 실행 파일 `Netstorm.exe`·외부 도메인/IP 문서화와 폐기 검증 비활성화다. 근거는 [TODO](TODO.md)·[11.01](docs/exe/cpp-product-1101-20261101.md)·[외부 접속 정책](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json)이다. 버전/이름/Credits 요구를 문서 정리로 바꾸지 않는다.

## 2. 3차 잔여 — 다음 단계까지6묶음

| ID | 남은 작업 | 종료를 판정할 근거 | 상태/규모 |
| --- | --- | --- | --- |
| **M1** | 메타 CS 수신20/21/22/23/24/25/26/88h·1dh와 해당 실제 하위. | [1149/22h](docs/exe/cpp-meta-participant-move-20261108.md)·[1150/25h](docs/exe/cpp-meta-sail-error-20261108.md) 및 선행 단위 원문/빌드 근거. | **완료(1150)**. 추가 Caller 감사는M8이며 이미 연결한 수신을 다시 만들지 않는다. |
| **M2** | 로그인/진단1ah·1bh 전체 수신, 실제 기록/Zone·State7/즉시8·인계 후 재진입. | 기록1151·복제/Zone1152·첫 State7/수명1153·1ah1154·1bh1155·같은 전체 전송 재진입1156. [마감 근거](docs/exe/cpp-meta-battle-reentry-20261108.md). | **완료(1156)**. 남은 별도 직접 Caller/전송 전수 대조는M6/M8이다. |
| **M3** | **Descend·메타전환 요청/참가 후속 World·카메라/HTML(M3.3b)**. | 기존 공통 Squid Pop/신전 정기 Handle의 전체 부모·현재 checking은1157 대조 완료. master 주소의 TCP 재사용 순서는1158에서 교정했다. 준비/전환·실제 진단·카메라/HTML 후속이 남는다. | **다음M3.3b / 부분 완료.** 신전 정기 Handle을 일반 마우스 입력과 혼동하지 않는다. |
| **M4** | 1095에서 작성한 메타 **송신 상태의 실제 Caller**와 남은 요청/HTML 후속. | 송신 부품만 존재하는 상태와 실제 UI/State 분배를 구별해 모든 활성 진입점을 연결한다. | 미완료 / 중간. M3의 전환 계약을 공유한다. |
| **M5** | R의 **세 `.server` 콘텐츠/배포**, LAN hostname·Config `registryValue` 환경 계약. | 원문이 지정한3파일의 이름/출처/내용·사용자 홈/배포 위치를 확정하고 실제 R 읽기와 연결. 숫자IPv4 밖의 로컬 이름 및 필요한 registryValue 경로를 원문 근거로 보완. | 미완료 / 중간.3파일+2계약이며 내용 조사가 필요하다. |
| **M6** | 일반 LAN·**Outpost(AGENTS의 Outpust)** 잔여 전수 대조/구현. | 일반 전투/합의/입력·저장/복구·선택적packet-record·나머지 송수신·Outpost 수명을 함수/Caller 단위로 목록화하고 실제 장치에 접합. | 미완료 / **큼·세부 수 미확정**. 기존982~1093 완성분을 되살려 중복 작업하지 않는다. |
| **M7** | 기존 독립 후속 **4건:** helpWord 전역 캐시 수명, 강제 활성 커서, 조기/원격 희생 안내, marker 진단 경계. | 현행 도달성/원문/테스트 전용 정책을 대조한다. 활성 미연결은 복원하고 의도된 경계라면 근거로 판정한다. 실제 미구현을 “한계”로 이름만 바꿔 닫지 않는다. | 미완료 / 조사 후 확정. 사용자 제외 희생 플레이 테스트는 수행하지 않는다. |
| **M8** | **3차 구현 마감 감사·배포 정합성**. | M1~M7/현재 checking 목록0/새 누락/활성 Caller·실제 owner/수명·외부 검증 비활성화 대조, Release 빌드, 필요한 최종 ZIP의 실제 Netstorm/R/에셋 바이트 검증. | 미완료 / 마지막 관문. MP 실행 테스트는 하지 않으며 ZIP/전체검사를 반복하지 않는다. |

**남은 양의 해석:** M1/M2와 M3의 두 checking 부모 대조는 완료했다. 다음은 M3.3b의 실제 전환 후속이며, 송신 Caller·3서버파일/2환경계약·LAN/Outpost 전수 목록·독립후속4건·마감 감사가 남는다. 잔여6묶음을6커밋 또는 짧은 몇 단위로 예상하지 않는다. M6은 원문 전수 목록을 작성해야 세부 수를 확정할 수 있다.

### M2 세부 계획 — 완료(1156), 다음M3

1. **M2.1 완료(1151):** 같은 Client의 실제 선택적 기록 FILE/요청DWORD/현재 프레임과 메타·일반·내부·편집 송수신 연결.435220의설정 해석은 요청만0으로 초기화하며 열린FILE/enabled는 유지한다.1ah/1bh의Open은1154/1155에서연결했다.
2. **M2.2a 완료(1152):** `004dece0` 전체(observer1/현재 WORD/할당·signed 용량/앞방향 DWORD+BYTE), Client 장기 Zone owner, 실제 접속 초기 복제 Caller와 기존 일반 LAN State7의 현재 token 설치/재조회/반납을 연결했다. 단독 미션86BYTE와 구별한다.
3. **M2.2b 완료(1153):** 메타 next7/PollStarting·current7/동기RunStarting과 즉시8, 같은 일반 World 본체·실제 TCP/UDP/등록표/현재Player·전체callback owner를 인계했다. 같은 메타 owner만 명시 pin한 수신 중 정리를 허용하고 다른 callback 보호는 유지한다.두수신등록은1154/1155에서연결했고인계 후 재진입은1156에서 연결했다.
4. **M2.3 완료(1154):** 전체1ah/004b4df0·40a1e0 가변observer, checking복귀→next 주소/7/현재포트→실제 Zone복제/예약증가→선택적 기록Open→10/11 정리를 실제 공유 등록표에 연결했다. [근거](docs/exe/cpp-meta-battle-zone-receive-20261108.md).
5. **M2.4 완료(1155):** 전체1bh/004b8b20·빈observer/38BYTE, server/예약조기반환→checking복귀→현재packet+4/Subscriber 기록Open→현재주소NUL포함BYTE복사/포트/current7→같은직접Run을등록했다. [근거](docs/exe/cpp-meta-battle-peer-receive-20261108.md).
6. **M2.5 완료(1156):** 두 수신의 실제 전체Source,1ah next7의 단일 외부Poll/1bh 동기Run, 같은 현재 World/TCP·UDP·두 표·전체Meta 확인, 내부 끝까지 같은 coordinator 재사용/현재 장치 재연결과 등록 반납→재대여를 연결했다. 다른 Session 콜백 보호는 유지한다. [근거](docs/exe/cpp-meta-battle-reentry-20261108.md).
7. 요청 DWORD의활성writer/다른원문전송·기록진입점은M6전수대조에도포함한다. 개발자이름제한test battle의4b8be0→1bh직접Caller도M6대상이다. 새MP실행검사는하지않는다.

### M3 세부 계획 — 현재 다음M3.3b

1. **M3.1 완료(1157 정적 감사):** 기존 `RunSquidPopParent`/004b02d0와 같은 실제 World/Pool/TYPE·현재 checking·부분 spot/side·SHP/표시/Activate Caller를 C/PE에 대조했다. 제품 변경/새 실행 없이 기존 연결의 오래된 미완료 표시를 제거했다.
2. **M3.2 완료(1157 정적 감사):** 004dc220은 신전 정기 Handle(event0~4/default)이다. 기존 `RawAssetRegular::Vortex`와 실제 Session의 상태/상승/이탈·Descend 장치, 현재 checking DWORD·네 가상표/풍 thunk를 대조했다. [근거](docs/exe/cpp-checking-parent-closure-20261108.md).
3. **M3.3a 완료(1158):** 참가 master의 현재 주소44BYTE 경계만 실제 TCP에 넘겨 기존9 준비 비영 재사용이 NUL/주소 해석보다 먼저 오도록 복원했다. [근거](docs/exe/cpp-meta-master-address-order-20261108.md).
4. **M3.3b 즉시 다음:** 실제 Descend/참가 요청→State19 준비·같은 World/카메라/HTML serial 후속을 마감한다. `UberMeta.inc`의 CS->listsInitialized/FortCoreDataLoadContext 진단 두 곳은 clog만 쓰므로 원문 식/파일/줄을 확인해 같은 Client AssertDisplay에 연결한다. 포획/희생/특정 공격/승리·MP 실행은 하지 않는다.
5. 이 실제 부모/Caller 연결 뒤 M4의 송신 UI/State Caller로 넘어간다. M6 전수 대조에서 새 범위가 발견되면 현재 항목에 구체적으로 추가한다.

### M2~M8 수행 시 유지할 원칙

- 원본 R을 복사/실행해 새 R복원을 대신하지 않는다. 새 R/실제TCP·UDP·같은 등록표/시계/Config/주소·기록 owner 연결은1062~1093의 완료 근거를 사용한다.
- 제공 R10.71의3BYTE 존과 클라이언트10.78의47BYTE 존 차이는1093의 명시 compact 호환 범위를 유지한다. 전체 프로토콜 차이가 없다고 확대하지 않는다.
- 폐기 CD확인7f/80(42cd10)·클라이언트 유효성 검사·PIRATE quota를 복구하지 않는다. IPX/SPX는 현재 명시 미지원이며 숫자 IPv4 TCP/UDP 범위를 유지한다. 원문에 없는 원격 관리 포트 Open/handler를 추가하지 않는다.
- 숨은 잔여가 발견되면 해당 M항목을 세분화하고 종료 근거를 추가한다. “소스가 있음”과 실제 부모 연결 완료를 구별한다.

## 3. 3차 뒤의 구현 계획

### 3.1 한국어 후속 — K1~K5

| ID | 작업 | 종료 조건 |
| --- | --- | --- |
| K1 | 986 한국어 후보의 기준/현행 차이 검토·선별 병합 | `korean/FINAL_PLAN.md`의 코드41파일·별도44자산을 현재 소스와 병합. 옛 patch 일괄 적용/덮어쓰기는 하지 않는다. |
| K2 | 표시 부호화·한글 입력/IME·클립보드 | `EncodeOriginalText`/표시용 부호화 경계를 유지하고 편집/입력/표시의 관련 빌드·허용 집중 확인을 마친다. 시나리오UI보다 먼저다. |
| K3 | 미션 브리핑·팁·도움말 `.korean` 본문 | 영어의 AI/SP/시나리오 선언을 보존하며 필요한 한국어 본문/대체 경로를 제공한다. 준비 자산만 있다고 전체 번역 완료로 세지 않는다. |
| K4 | 한글 메인 버튼 그림·목록 이동행/누락 UI 문구 | 기존 원본 스타일/크기·D2Coding·AGENTS 명칭 규칙을 유지하고 메뉴/해당 화면에서 확인한다. |
| K5 | 번역 검수·배포/회귀 마감 | 번역502의 기존1차와 새 본문/그림/입력을 구별해 검수하고 관련 집중 검사·단순 비MP 진입 확인·배포 정합성을 마친다. 자연 완주/공격/희생 테스트는 제외한다. |

### 3.2 신규 Edit 시나리오 UI — S1~S4

| ID | 작업 | 종료 조건 |
| --- | --- | --- |
| S1 | 986 시나리오 후보/모델 병합 | `scenario/PLAN.md`·`CMake-additions.txt`에 따라 모델5파일/코드·필요 자산을 현행 편집기와 병합/등록한다. |
| S2 | 기본 english/부재 Test Battle | 새 맵의 기본 `.english`를 자동 생성하고 english가 없어도 Test Battle 진입에서 종료되지 않게 한다. 기존 수동 대본을 보존한다. |
| S3 | 실제 전용 편집 UI | 초기 자본/테크·적·플레이어·동맹 등 시나리오 구성을 화면에서 수정하고 맵 배치 도구와 함께 동작하게 한다. |
| S4 | 사용자 홈 저장/재적재·마감 | 실제 사용자 홈·읽기 전용 설치에서 저장/불러오기·Editor/Test Battle 진입을 확인한다. 필요한 검증은 진입/대기/기본 조작으로 마치며 승리/포획/희생을 재현하지 않는다. |

### 3.3 준비본과 별도 후순위

- 준비본의 기준은 [986 안내](docs/exe/handoffs/cpp-pause-20261106/README.md)·[MANIFEST](docs/exe/handoffs/cpp-pause-20261106/MANIFEST.json)이다. **한국어/시나리오 후보는 제품 미적용·미컴파일·미실행**이며 초안 검사 수를 현재 통과로 세지 않는다. 이번 문서 정리에서237파일의 보관 해시를 다시 확인했다.
- [한국어 최종 계획](docs/exe/handoffs/cpp-pause-20261106/korean/FINAL_PLAN.md)은41코드/배포/검사파일·44별도자산·새 검사24개 초안이다. `final.patch`에44자산이 들어 있지 않다.
- [시나리오 계획](docs/exe/handoffs/cpp-pause-20261106/scenario/PLAN.md)은 전체24파일(변경23)·기준18·통합9/모델10 검사 초안이다. 모델5파일은 `scenario/base`에 보존돼 있고 제품 CMake에는 아직 없다. shadow 패치의 개행 정규화 일치를 실제 컴파일 통과로 보지 않는다.
- 986의 **원격 Fort 준비분은987에서 실제 제품으로 별도 구현**했다. 후보 보존을 이유로 다시 미적용으로 되돌리지 않는다.
- **기존 별도 후순위도 삭제하지 않는다:** 60/120프레임 지원·CD10.72 전체 복원/호환성. 현재3차 종료 관문과 분리하며1~8차 뒤 세부 계획을 확정할 미착수 후순위다.10.72 관련 분석 근거는 계속 문서화한다. dotnetpj/webpj/Linux 구현은 별도 프로젝트/범위다.

## 4. 완료 근거 색인

| 완료 범위 | 확인할 근거 |
| --- | --- |
| 비멀티 UI/원본 Edit/미션 수명/내부 수명 | [929](docs/exe/cpp-ui-implementation-closure-20261105.md)·[943](docs/exe/cpp-editor-implementation-closure-20261106.md)·[956](docs/exe/cpp-mission-state-scope-20261106.md)·[980](docs/exe/cpp-internal-lifetime-closure-20261106.md) |
| 디컴파일·소스 위치 | [2026-11-03](docs/exe/cpp-decompilation-20261103.md)·[신뢰도](docs/exe/decompile-reliability.md)·[SOURCE_MAP](cpppj/SOURCE_MAP.md)·[모듈](docs/exe/modules.md) |
| LAN 일반/서버 완료 상세 | 단위982~1147의 `docs/exe/` 문서/`cpppj/*.json`·Git 이력. 긴 순차 완료 목록은 [보관 원문2.1/3.8](LEFT_JOBS.old.md#archive-20261108-through-1147)에서 찾는다. |
| 현재 메타 수신 | [1142/88h](docs/exe/cpp-meta-participant-name-20261108.md)·[1143/26h](docs/exe/cpp-meta-geometry-receive-20261108.md)·[1146/20h](docs/exe/cpp-meta-participant-create-20261108.md)·[1147/21h](docs/exe/cpp-meta-participant-remove-20261108.md). 나머지 완료 근거는 최신 단위 보고서에서 역추적한다. |
| 전체34/미션별 과거 증거 | [대응표](docs/gameplay/demo-campaign-map.md)·[보관 원문2.2~2.4](LEFT_JOBS.old.md#archive-20261108-through-1147). 과거 진입/일부 사용자 완주를 현재 모든 자연 완주로 합산하지 않는다. |

## 5. 미해결 검사·환경·정밀도 경계

이 절은 만료 이력이 아니라 **현재 유지할 미해결/제한**이다. 관련 허용 집중 검사에서 재발하면 원인 분류·수정을 이어 가며 문서 정리로 통과 처리하지 않는다.

- 현재 전체 솔루션의 최신 통과 결과는 없다.938의68집중 대상 중48완전 통과/20실패는 당시 결과다. `guide_release`151개와 `startup_file_names`3개(`names.Index()`)는6차 전 `1f5b4177`에서도 같은 실패를 확인했다.
- 당시 **원인 분류 잔여:** `file_tests`5, `pop_context`29, `menu_session`3, `mission_entry`1, `guide_fort_request`1, `renderer_presentation`1, `startup_*`/`file_directory`의 경로 고리. `world_generation`은1024×768 건물 그림/장치 미연결 계열이며 기준선 개별 비교를 하지 않았다. 후속 개별 성공을 이 목록의 최신 전체 재검사로 쓰지 않는다.
- `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은 일반Battle/MP 이월이며 새 MP 검사는 제외한다. `CampaignForwardBattle_NaturalTempleAssault`549 실패는 사용자 자연 완주/별도 지원 범위이며 기존 전략/승리 단언을 약화하지 않는다. TEST02 로컬 AI/SP·자동 공략 지원은 별도 요청 전 하지 않는다. 원문 `owner!=myPlayerId || demo!=0`를 `gate.demo=1`로 우회하지 않는다.
- 해결된 실패를 되살리지 않는다: FortObjectLoad 입력/필수Pop 관측은942, SubscriberGlobals C2607은1052의 실제 파일 컴파일/기존7검사로 교정했다.
- 원문8비트/설정·경로의 `EncodeOriginalText`와 화면용 `EncodeDisplayText`/`DecodeDisplayText`를 구별한다. MakeAi owner0 이전/삭제·전략 차이, 설치 글꼴/다른ACP·구형무BOM 설정 자동 이관은 [글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md)를 따른다.
- 32비트 FILE/FileSpec/포인터·SEH/스택·미작성값 자체 일치, 임의 외부heap→SID, 실제 OS 고갈/물리 디스크 장애·UNC/드라이브CWD·CD 전체 dump/trace/Retype·x87 전체 입력/픽셀은 기존 호스트/후순위 경계다. 명시 부분I/O/버퍼 실패/복구966·new_handler/종료979를 모두 미착수로 되돌리지 않는다. 활성 Caller의 미연결은 이 경계로 숨기지 않는다.
- PCM은936의HJOW-Athlon에서 프로세스 WASAPI101653프레임/비무음99726과 영상/소리 합성을 확인했다. 청취/다른 장치/실제 CD/활성 전체화면까지 검증한 것은 아니다. 덱 검은띠981과 별도의 사용자 보류 연녹색 사각형을 혼동하지 않는다.

## 6. 운영 규칙과 유효한 사용자 결정

- **최신 지시:** CPP의 이 문서상 AI 작업을 계속한다. 사용자2026-11-08 추가 지시에 따라 이번에 만료 이력을 보관하고 단계/계획을 정리했다. 정리 후 CPP 구현을 이어 간다. 자연 완주·전체 동일성·사용자 직접 확인·MP 테스트는0.3절대로 제외한다.
- **검증 구분/빈도:** 정적 대조·컴파일·단위 실행·GUI·물리를 구별한다. 최초 실패/교정 원시를 보존하고 기대값을 제품에 맞춰 약화하지 않는다. 많은 기능 묶음 뒤 필요한 통합 확인을 하되 옛3~5단위 숫자를 자동 전체34/CTest/ZIP 실행 조건으로 쓰지 않는다.
- **빌드:** 현재 HJOW-Athlon은 VS18(2026)/MSBuild18.x다. `cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`로 빌드한다. 살아 있는 단일 핸들을 추적하고 종료0을 확인한 뒤 다음 작업을 한다. 빌드 중 제품 소스를 변경하지 않는다. HJOW-X3D는VS17(2022)/v143이다. [빌드 안내](docs/cpp-build.md).
- **실행 파일/자료:** 대상NetstormCpp·OUTPUT_NAME Netstorm·ARCHIVE_OUTPUT_NAME NetstormCpp를 유지한다. 원본/클론은 전체 경로로 구별하고 EXE 옆 `game/netstorm.tarc`를 우선한다. cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다. 무인자 GUI·`--help/--run`·분석CLI를 유지한다.
- **보호/실행:** AGENTS.md·원본/CD 기존 파일·루트 README 개요를 바꾸지 않는다. 원본 실행 전후 해시/허용 설정 복원을 지킨다. 시작한 프로세스만 종료하고 이름 기반 일괄 강제 종료를 하지 않는다. CHEAT/테스트 조작은 원문 Edit 조건과 명시 검사 옵션으로 구분해 기본 적AI/프로필에 섞지 않는다.
- **언어/편집:** 새 문서/코드는UTF-8, 응답/주석은한국어다. D2Coding/AGENTS 명칭·주석 규칙을 따른다. 이 두 인계 MD의 기존 CRLF를 보존한다. PowerShell5.1 stdin은 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`, 필요 시Python UTF-8 모드를 사용한다. 원시 로그/fixture는 인코딩 정리로 덮지 않는다.
- **Git:** 단위마다 인계 뒤 `YYYYMMDD-NN CP 요약`으로 로컬 커밋한다. 다른 세션과 당일 번호를 확인하고 필요한 경우 `git -c core.autocrlf=false`/`-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정을 바꾸지 않고 최초 TSV의 `-text`를 지킨다. 실패해도 일을 계속하며 **push는 하지 않는다**.
- **문서 권한:** 이 문서가 현행 CPP 상태/우선순위의 기준이다. [빌드/로드맵](docs/cpp-roadmap.md)·[플레이 계획](docs/cpp-playable-plan.md)·옛 보고서의 당시 “다음 작업”을 현행 미완료로 되살리지 않는다. 단위 보고서는 완료 범위의 근거로 사용한다.

<a id="인수인계-문서-임시-분할"></a>

## 7. 보관과 문서 분담

- cpppj/공통/디컴파일은 이 문서, DN/WB는 각 LEFT_JOBS 문서를 유지한다. 현재 다른 세션의 DN/WB 작업을 합치거나 정리하지 않는다.
- **2026-11-08 정리1148:** 이전 원문251150바이트를 [새 보관 앵커](LEFT_JOBS.old.md#archive-20261108-through-1147)에 그대로 추가했다. 기존 보관본의 모든 바이트를 접두부로 보존했다.0.5~0.165의 순차 완료 이력·만료된 예고/긴 완료목록을 옮기고 현재 단계/8개M·5개K·4개S 계획/전환 조건으로 대체했다.
- 1147에서 완료한 `0042bc10`을 checking 미완료에서 제외해 **5→4**로 정정했다. 미해결 검사/사용자 결정/준비본 미적용/후순위 범위는 남겼다. 구현/검증 완료 범위를 새로 부풀리지 않았다.
- **문서 정리 검증:** 원문/기존보관 바이트 보존·현재 링크/기존 외부앵커·237후보 해시·2800보호 파일·제품1147 EXE/R 해시를 확인한다. 새 빌드/단위/게임/MP/통신/디컴파일은 실행하지 않는다. 이후 CPP 구현을 계속하며 현재 다음 항목은M3.3b다.
