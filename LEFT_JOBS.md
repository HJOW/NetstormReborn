# LEFT_JOBS — cpppj 및 공통 인수인계

cpppj와 공통 사항의 **현재 제품, 완료 범위, 남은 구현, 사용자 확인, 재개 절차**를 정리한다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다.

**2026-11-08 /goal 진행·제품1138:** 메타 ESC의 실제 선택 정리·커서 취소/메뉴 serial 토글과 공통 Game/View/Options 메뉴 작성기를 같은 Session/SP/현재 HUD에 연결했다. H/N/P/R/F4/F5/F6·생산 키·F9 예약/실제 카메라 경로와 일반 F6의 조기 소비 누락도 보완했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 다음은 메타 Refresh Zones(c6)·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. 기존 사용자 검사 제외와 우선순위를 유지하며 전체 목표는 진행 중이다.

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-11-08, 제품1138)

| 항목 | 현행 기준 |
| --- | --- |
| 진행 상태 | **사용자2026-11-08 /goal 지시로 진행 중.** 전체 목표는 미완료이며0.156/3절8번을 따른다. |
| 현재 제품 | **1138 / 표시·파일·제품11.01.** 메타 ESC의 실제 선택 정리·커서 취소/메뉴 serial 토글과 공통 Game/View/Options 메뉴 작성기를 같은 Session/SP/현재 HUD에 연결했다. H/N/P/R/F4/F5/F6·생산 키·F9 예약/실제 카메라 경로와 일반 F6의 조기 소비 누락도 보완했다. |
| 실행·플레이 가능 여부 | 무인자 GUI 시작과 미션 플레이 경로가 있다. 일반31+데모3의 마지막 전체 진입 검증은 **645의644 제품**이며 현재 제품의 전체 회귀 결과가 아니다. |
| 원본과의 동일성 | 부품별 원문 대조와 집중 검사를 누적했다. **전체 동일성·모든 시나리오 자연 완주는 미확인**이며 사용자 확인 대상이다 |
| EXE SHA-256 | Netstorm.exe 12,933,120바이트/SHA-256 `fabc5568b05ec2768ffc04a055ce08efd8bdad6f3518f36e2242fd73fb4d20c4` / R.exe 669,696바이트/SHA-256 `9078d237d386bfb38fc27218455cf8f343c9e45a91ce824885a52507c6b93b6e` /1138·11.01 |
| 최신 빌드·집중 검사 | **1138 Release/x64 종료0·경고/오류0.** 4cc210/4d9494~d960c/4da880/4950d0와 기존 선택·메뉴·카메라 몸체의 실제 getter·Select(0,0,0)·serial/Kill/0쓰기·지도 원점/검산·일반 F6 진행을 대조했다. 보호2800 불변·새 C++/MP/게임/통신실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1138 기록](docs/exe/cpp-meta-common-input-20261108.md). |
| checking 잔여 | **5함수=부모2+부분0+MP분류3.** 1014에서004b8cb0 전체 몸체/현재 진단·수동 입력을 연결했다. 완료부모40/부분6다. 메타맵2표/004b02d0·Descend/004b4df0/004b8b20 등은3절8번 잔여다. |
| 최신 정적 감사 | **1138: 4cc210/4d9494~d960c/4da880/4950d0와 기존 선택·메뉴·카메라 몸체의 실제 getter·Select(0,0,0)·serial/Kill/0쓰기·지도 원점/검산·일반 F6 진행을 대조했다.** 실제 메타 장면/MP 실행 검증과 구별한다. |
| 마지막 GUI | **984 실제 단독MCP 1실행:** 격리1920×1080 tutorial1 진입·비정지100프레임 대기·WM_CLOSE 종료0, 입력5/관측7·PNG 확인·설치1351개/보호 불변. LAN/공격/포획/희생/승리/물리 입력 검증이 아니다. |
| 전체 검사 기준 | 활성 솔루션84프로젝트: Application79(검사 이름73/게임1/서버1/probe·dump4),라이브러리2,Utility3. 디스크의잔류10프로젝트는제외한다. 현재 제품의 전체 검사 통과 기록은 없다. 과거 전체 결과는 보관본, 알려진 실패/기준선 판정은2.5절을 따른다. |
| 전체 34·ZIP | 마지막전체34진입은645의644제품이다. 최신ZIP만1092/1384항목·게임자료1358개·202,440,265바이트·SHA-256 `33b2459cca9dcbde8b10c7e2cca2e12cc071715244868e3a60908fb085089ae7`/전체CRC·두EXE동일바이트다. 1092 ZIP추출후게임/서버실행은0이며645의전체GUI검증과구분한다. |
| 보호 기준 | **987의 AGENTS.md·원본/CD/fixture2800파일을1138 종료 때도 모두 불변으로 확인했다.** 새fixture0, DN/WB 제외. 기준은 `extracted/cpp-resume-1138/protected-before.json`이다. |
| 마지막 전체 정밀 디컴파일 | **HJOW-Athlon (192.168.0.94) / 2026-11-03**. 양판본 정밀 산출물과 컴파일 기록은 [2026-11-03 기록](docs/exe/cpp-decompilation-20261103.md)을 따른다 |
| R.exe 전용 정밀 디컴파일 | **HJOW-Athlon / 2026-11-07**.5308함수/실패0·미정규약184. 클라이언트양판본전체날짜와구분하며 [1057 기록](docs/exe/cpp-local-server-discovery-20261107.md)의전용스크립트로재생성한다. |

### 0.2 재개 절차

1. 사용자 재개 요청 뒤 AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 해당 인계도 읽는다. `git status`와 `git fetch`로 다른 PC/세션 변경을 확인하며 push는 하지 않는다.
2. 현재 호스트가 마지막 디컴파일 호스트와 다르면 cpppj 작업/디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 문서에 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 불완전하므로 쓰지 않는다.
3. 실제 게임·별도 창 실행은 AGENTS.md의 확인 규칙을 따른다. 예외는 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94) 및 사용자가 직접 요청한 원본 수동 컨트롤/녹화 분석의 해당 단계다. 다른 PC는 실행 목적·필요성 설명 후 명시적 확인이 필요하다.
4. 최신 제품1138의 보고서는 `cpppj/meta-common-input-20261108.json`이다. R원문이필요한새PC에서는기존클라이언트전체정밀절차와별도로`tools/ghidra/refine_root_server.ps1`을실행하고호스트를갱신한다. 986의 한국어/시나리오 후보·기준·해시는 [추적 인계](docs/exe/handoffs/cpp-pause-20261106/README.md)에 있다. `extracted/cpp-resume-996/` 등 원시는 Git 제외·작업 PC 전용이므로 다른 PC에 있다고 가정하지 않는다.
5. 다른 세션의 dotnetpj/webpj·staged 파일·실행 출력은 포함하지 않는다. 한 단위를 마치면 0.1절·0.4절·2.1절 완료 항목·3절 잔여·단위 문서를 갱신하고 해당 파일만 로컬 커밋한다.
6. 완료 범위는2.1절, 잔여 우선순위는3절을 따른다. 단위986까지의 상세 인계·옛 검사 수·대체된 지시는 [정리 전 전체 원문](LEFT_JOBS.old.md#archive-20261106-through-986)과 각 단위 기록에서 확인한다. 과거의 다음 작업 예고를 현재 미완료 목록으로 되살리지 않는다.

### 0.3 유지되는 제한과 보류

- **사용자 직접 확인:** 모든 시나리오의 자연 완주·전체 동일성, 실제 사제 포획/희생·특정 적 공격·시나리오 승리 재현. AI는 별도 요청 없이 자동 공략/테스트 지원을 진행하지 않는다. 짧은 메뉴·진입·기능 검증과 부품 원문 대조는 가능하다.
- **공중 3종:** 훨리기그·맨오워·더스트 데빌은 정적 분석·코드 작업만 한다. 새 전용 생산·생성·우클릭·행동 관측/검사는 하지 않는다.
- **멀티플레이:** 비멀티·UI·Windows 10 호환성 뒤 원문 근거로 사전 구현한다. 신규 단위 검사·게임·실제 통신 검사는 하지 않는다. 사전 구현을 동작/안정성 검증 완료로 쓰지 않는다.
- **단위 작업의 상시 제외:** 각 단위에서 실제 게임/GUI·진단 UI, 포획/희생/특정 적 공격/승리, 신규 MP/패킷/통신, 공중3종 전용, 물리 입력/PCM/CD 재시도, 전체 검사/34미션/ZIP 반복은 실행하지 않는다(4절 실행 빈도). CP 단위에서 DN/WB 파일을 수정·커밋하지 않으며, 작업 트리의 다른 세션 미커밋 변경은 정리·stage하지 않고 보존한다.
- **사용자 보류:** 스펠 효과 도움말 그림 추가 분석(530, 임시 숨김 유지), 연녹색 직사각형 추가 분석(원본에도 관찰, RDP 원인 미확정). `.english` 부재 Test Battle 개선은 영구 보류가 아니라 **8차 요구**다.
- **환경 한계:** 물리 입력/캡처는762의 새 부팅·RDP세션1에서도 두 번 timeout이었다. 실제 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·실제 앱 전환·청취·사용자 활성 전체화면의 물리 출력·실제 CD 삽입은 미검증이다. 실제 PCM 캡처는936에서후속성공했다(3절7번).
- **원문/호스트 경계:** 32비트 FILE/FileSpec/SEH·스택 배치/미정값 자체 일치, 실제 OS 고갈·물리 디스크 고장과 전체 진단 UI는 별도 한계다. 명시 부분 I/O/버퍼 실패/복구 상태는966, new_handler·종료 수명은979에서 대조했으므로 모두 미착수로 쓰지 않는다. 612의 반사 입력, VFX signed WORD/정상 표본·양수/회전0/원색의 근거를 전체 입력·픽셀로 확대하지 않는다.

### 0.4 준비본 인계 — 986 보존, 원격 Fort는987에 별도 반영

986 인계는 `c68942cf / 20261106-57 CP`로 커밋됐다. [인계 안내](docs/exe/handoffs/cpp-pause-20261106/README.md)·[986 기록](docs/exe/cpp-pause-handoff-20261106.md)과 `MANIFEST.json`이 기준이다. 이번 검증에서 보관237파일의 바이트 해시가 모두 일치했다.

| 준비분 | 보존 범위 | 실제 제품 상태 |
| --- | --- | --- |
| [원격 Fort](docs/exe/handoffs/cpp-pause-20261106/remote-fort/PLAN.md) | 실제 이름·manager·MarkStream·이미지 수명의 설계/함수표 | **987에서 실제 제품에 별도 구현·컴파일 완료.** 스케치를 덮어쓰지 않고 원문 재대조·공통 장치 재사용·Session 접합을 했다. MP 동작 검증은 제외 |
| [한국어 후속](docs/exe/handoffs/cpp-pause-20261106/korean/FINAL_PLAN.md) | 코드/배포/검사41파일·자산44개·새 검사24개 초안 | **미적용·미컴파일·미실행.** 실제 제품은938의 한국어1차까지만 제공 |
| [시나리오 UI](docs/exe/handoffs/cpp-pause-20261106/scenario/PLAN.md) | 전체24파일(변경23)·기준18·통합9/모델10 검사 초안 | **미적용·미컴파일·미실행.** 미등록 모델5파일은 `scenario/base`로 이동했으며 제품 CMake에는 없음 |

전체 사본이 바이트 기준이고 패치는 검토/병합용이다. 시나리오 shadow 패치는 개행 정규화 후24/24 동일을 확인했을 뿐 바이트 동일 재구성이나 C++ 통과가 아니다. 한국어 `final.patch`에는 자산44개가 없으므로 별도 병합한다. 후보를 현재 소스 위에 일괄 덮어쓰거나 옛 패치를 중복 적용하지 않는다. 재개 순서는3절8번과 인계 안내를 따른다.

### 0.5 최근 단위 — 987 원격 Fort·988 동기화·989 전투섬

987은 `a328ac1f / 20261106-62 CP`로 커밋됐다. 988은 `RawPeerWorldSync`·대상별 Squid 전송·ProcessForm 강제 writer/실제 버퍼 조회·공유 Regular writer·같은 Session/World를 연결했고 Release 제품 첫/최종 빌드가 각각 종료0이다. 최종 검토에서 Path fallback 선택 표식을 갱신한 뒤 다시 빌드했다. 신규 MP/단위/게임/통신 검사는 사용자 지시대로0이다. [988 기록](docs/exe/cpp-peer-world-sync-20261106.md). 마지막 순회는 process 송신이 아닌 walker Unpop/재등록으로 원문·PE에서 확인했다. 미연결 파생 process writer/decoder 전체와 일반 전투섬/중앙섬·일반 State/roster/UI는 후속 잔여다.

988은 `1cd2b05a / 20261106-63 CP`로 커밋됐다. 989는 `RawBattleIsland`의 첫 flag4 Territory 선택→실제 Player 청크 배치→섬/지형→현재 boss의 선택 영역 Objects→Graph0을 서버 Login/client2a에 연결했다. 첫 제품 빌드는 종료0/경고1(C4458)이었고 매개변수명을 고친 최종 빌드는 종료0·경고/오류0이다. 신규 MP/단위/게임/통신 검사는0이다. [989 기록](docs/exe/cpp-battle-island-20261107.md). 원격 buildFort와 일반 makeBattleIsland는 이제 연결됐으며 중앙섬/ordinary 자기 Fort/일반 State·roster/UI·전체 파생 writer/decoder는 남는다.

### 0.6 최근 단위 — 990 중앙섬과 metadata

989는 `dc6d1933 / 20261107-02 CP`로 커밋됐다. 990은 일반 중앙섬(`RawCentralIsland`)·실제 지면 군도 분리(`RawIslandPartition`)·현재 지역 getter·metadata 수신/송신 observer·layout2 지역 SURROUNDED Template/원시 Objects·같은 Pool 예측 카운터를 연결했다. Release 제품 빌드는 종료0·경고/오류0이며 보호2800파일은 불변이다. 원문 PE에서00540a30은127(owner0과 별개),005424b0은 Pool Reset의1임을 재확인했다. 신규 MP/단위/게임/통신 검사0이며 다른 세션의 별도 oracle 빌드를 포함하지 않는다. [990 기록](docs/exe/cpp-central-island-20261107.md). ordinary 자기 Fort/일반 State/roster/LAN UI 및 나머지 MP는 잔여다.

### 0.7 최근 단위 — 991 자기 일반 Fort

990은 `9cb0547f / 20261107-03 CP`로 커밋됐다. 991은 client2a 자기 ordinary Fort를 같은 Reload→실제 Badges→현재 Player 생성→CoreData 인자1 최소 병합→Technology→Deck에 연결했다. 첫 빌드의 폭 인자 오류6개를 교정한 최종 Release 빌드는 종료0·경고/오류0이고 보호2800파일은 불변이다. 신규 MP/단위/게임/통신 검사0이다. [991 기록](docs/exe/cpp-local-fort-20261107.md). 다음은 일반 State6·7/Commander/roster/LAN UI다.

### 0.8 최근 단위 — 992 일반 시작 세부 경로

991은 `a4f57cda / 20261107-04 CP`로 커밋됐다. 992는 일반 State7의 현재 밀도 옵션/두 게임 난수·채널40/80 정리·simulconnect listener, State8 연결 안내, State9 송신 뒤 안내를 기존 상태 몸체/실제 Session·Client HTML에 연결했다. Release 제품 빌드 종료0·경고/오류0, 보호2800파일 불변이며 신규 MP/단위/게임/통신 검사0이다. [992 기록](docs/exe/cpp-battle-start-details-20261107.md). 일반 전투의 호스트 State7 진입/World 교체 부모와 State6/Commander/roster/LAN UI는 남는다.

### 0.9 최근 단위 — 993 시간 동기화와 Commander

992는 `7f1ae00e / 20261107-06 CP`로 커밋됐다. 993은 원문5번 시간 패킷·70표본·빠른 절반의 보정 평균·서버/Commander 설치, 같은 실제 GameTicks를 쓰는 게임 시계와 flush→동기화→receive 순서를 연결했다. 가상 this의 직접 송신에서 기존 Player/ID 재검사를 제거하고 현재 노드 수명을 유지했다. Release 빌드 종료0·경고/오류0, 보호2800파일 불변이며 신규 MP/단위/게임/통신 검사0이다. [993 기록](docs/exe/cpp-clock-synchronization-20261107.md). 일반 State6·7 호스트/roster/LAN UI 등은 남는다.

### 0.10 최근 단위 — 994 일반 복귀 State6 부모

993은 `39251709 / 20261107-07 CP`로 커밋됐다. 994는 State6의 정리 전 결과/출처 캡처·승패 배경/음악·이전 Battle14 통지·공통 Fort 연속 실행과 후반 옵션/결과 안내 부모를 추가했다. 기존 상태 분배기6번 위임과 공통 HUD/음악 뒤6번 자연 반환을 연결했다. Release 빌드 종료0·경고/오류0, 보호2800파일 불변이며 신규 MP/단위/게임/통신 검사0이다. [994 기록](docs/exe/cpp-battle-return-parent-20261107.md). **실제 Uber State6 선택/일반 Fort의 완료 계약 확장·Multiplayer Options/LAN 화면은 아직 남는다.**

### 0.11 최근 단위 — 995 공유 시계 주소 수명

994는 `14369776 / 20261107-08 CP`로 커밋됐다. 995는 공유 GameClock이 같은 GameTicks 주소의 실제 LocalState owner도 유지하도록 보완했다. 제품/집중 검사 빌드 각각 종료0·경고/오류0이며 현재 제품 라이브러리에 기존 비멀티 GameClock Reset4검사를 링크해 모두 통과했다. 신규 검사/fixture·시간패킷/MP/통신/게임 실행0, 보호2800파일 불변이다. [995 기록](docs/exe/cpp-clock-owner-lifetime-20261107.md). 실제 State6/Fort 계약·Multiplayer Options/일반 State7 호스트/LAN 통합 잔여는 유지한다.

### 0.12 최근 단위 — 996 실제 State6/Fort 통합

995는 `3e9dc2c7 / 20261107-09 CP`로 커밋됐다. 996은 실제 Uber State6 선택과994 앞뒤 부모를 같은 일반 Fort 파일/Player/월드/덱·UI 설치에 연결했다. 명시 비편집6만 기존 prepared 계약에 허용하고 다음0/공통144BYTE 확정을 검사한 뒤 loaded를 공개한다. 첫 빌드의 미사용 참조 경고1을 제거한 최종 Release 빌드는 종료0·경고/오류0이고 보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [996 기록](docs/exe/cpp-battle-return-integration-20261107.md). **결과0의 Multiplayer Options 공장 callback은 아직 미연결이다.** 일반 State7 호스트/roster/LAN 게임 전체도 남는다.

### 0.13 최근 단위 — 997 Multiplayer Options 창

996은 `49cac5a0 / 20261107-10 CP`로 커밋됐다. 997은 원문250×145 창·같은 Gump/문자 편집/돌 버튼·하단 정렬/serial/모달 수명을 작성하고 실제 State6 옵션 공장에 연결했다. 현재 Fort 선택/Subscriber 이름 변경/명시 로컬 주소 입력과 원문 일반 입력 큐를 재사용한다. 첫 빌드 C2664 한 건을 교정한 최종 Release는 종료0·경고/오류0이며 보호2800파일은 불변이다. 신규 MP/단위/게임/통신 검사0이다. [997 기록](docs/exe/cpp-multiplayer-options-20261107.md). **Connect의 실제 일반 State7/LAN 시작 소비자와 메인 메뉴 Multiplayer 진입은 남는다.**

### 0.14 최근 단위 — 998 일반 LAN State7 후반

997은 `3ca2516b / 20261107-11 CP`로 커밋됐다. 998은 일반 LAN의 별도 GeneralBattle 설치/요청과 실제 정리 뒤 같은 Client/Player/Core/표면 대여를 작성했다. 실제 가변 호스트 Zone과 client의 Zone 없음·simulconnect/밀도/채널/안내·팔레트를 구별하고 단독 미션86BYTE fallback을 쓰지 않는다. 게임 모듈 준비와 실제 phase14/Player 참여 완료를 구별한다. 첫/보완 후 Release 빌드 각각 종료0·경고/오류0·보호2800파일 불변이며 신규 MP/단위/게임/통신 검사0이다. [998 기록](docs/exe/cpp-general-battle-login-20261107.md). **Uber Connect/World 교체·대기실/실제 명단과 재접속 이름 선택은 후속 연결 대상이다.**

### 0.15 최근 단위 — 999 로컬 LAN 대기실 명단/시작 합의

998은 `7ce6f129 / 20261107-12 CP`로 커밋됐다. 999는 폐기 공식 매칭 대신 별도 사설 TCP 대기실의 실제 subscriber/이름/최대8명 명단·가변 Zone1a/71BYTE를 합의하는 부품을 제품 라이브러리에 등록·컴파일했다. Native 게임 Player/Ready/권한은 쓰지 않고 실제 시작 ACK/송신 drain 뒤 요청만 외부에 인계한다. simulconnect의 기존 TCP/내부 listener 재사용과 pending partial 송신 전진도 보완했다. 첫/최종 Release 각각 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [999 기록](docs/exe/cpp-lan-battle-lobby-20261107.md). **실제 Uber 호출자/대기실 화면·Connect/World 교체/메인 메뉴 진입과 일반 Session 이름 인계는 아직 미연결이다.**

### 0.16 최근 단위 — 1000 실제 LAN 화면과 일반 State7 앞단

999는 `f98d50d5 / 20261107-13 CP`로 커밋됐다. 1000은 메인 Multiplayer의 마지막 Fort/next6 진입, 같은 HTML의 로컬 host/join/주소·명단/Start/Cancel, 실제 대기실 폴링과 다음 Tick의 일반 State7 World 교체를 연결했다. 정리 전 실제 이름을 일반 Session에 인계하고 Player 재할당 전에 대기실을 닫으며 실제 phase14/Player 참여 뒤 입력/표시를 공개한다. 설정 API2곳 원인의 첫 빌드 오류8건을 교정한 빌드와 두 보완 빌드 각각 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [1000 기록](docs/exe/cpp-lan-world-integration-20261107.md). **다음은 일반 로그인 타이머 초기화/팀/초기 간헐천/진행 안내·저장과 나머지 전체 decoder·메타/재접속/Outpost/checking8이다. UI 연결을 LAN 플레이 완료로 보고하지 않는다.**

### 0.17 최근 단위 — 1001 일반 로그인 완료 초기화

1000은 `aa562029 / 20261107-14 CP`로 커밋됐다. 1001은 원문 같은 Core 재조회/32700 상한·미도착 Player 정리·비관전자 수+3/180초 스펠 예약·팀 동맹 일반 입력과 실제78h 수신/MessageBoard 종류7·9를 연결했다. 실제 중단/카메라·초기 세계 간헐천 부모도 타이머에 연결했다. 첫/보완 Release 각각 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [1001 기록](docs/exe/cpp-ordinary-battle-start-20261107.md). **다음은 일반 대기 진행 UI/ServerForceBattleStart·재접속 저장/복구와 일반 프레임/스펠 예약 소비, 전체 decoder·메타/Outpost/checking8이다.**

### 0.18 최근 단위 — 1002 일반 대기 화면/강제 시작

1001은 `a60d547f / 20261107-15 CP`로 커밋됐다.1002는 같은 현재 Player 명단/상태·프레임 남은 시간과 지연/서버 조건부 Start를 실제 cf570/HTML·일반 입력에 연결했다. 실제 반환 serial005c85b0은 transient와 별도로 보관하고 완료/Abort에서 해당 serial만 Kill/0으로 만든다. ServerForceBattleStart는 Begin만1을 쓰고 다음 타이머가 실제6e를 보낸다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다.[1002 기록](docs/exe/cpp-ordinary-login-progress-20261107.md). **다음은004b54e0 재접속 저장/복구·일반 프레임/스펠 예약 소비와 전체 decoder·메타/Outpost/checking8이다.**

### 0.19 최근 단위 — 1003 실제 저장상태 writer

1002는 `25e32b7a / 20261107-16 CP`로 커밋됐다.1003은004b54e0의 실제 SP 진단·976BYTE 개별 FILE 쓰기와 같은 복구 owner/subscriber/이전handle을 로그인 완료 타이머에 연결했다. 원문 벽시각/SP의 x87 정수 변환과 실제 사용자 홈 VFS/Unicode CRT를 사용한다. 두 checking 진단 뒤 계속하는 몸체를 연결해 checking잔여7(부모3/부분1/MP3)로 줄였다.Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다.[1003 기록](docs/exe/cpp-saved-state-write-20261107.md). **다음은 실제 파일 reader/주기 writer·일반 프레임/스펠 소비와 전체 decoder·메타/재접속/Outpost/checking7이다.**

### 0.20 최근 단위 — 1004 실제 주기 저장 caller

1003은 `5844bfb5 / 20261107-17 CP`로 커밋됐다.1004는004b6730의 실제 현재전투/벽시각 만료→같은writer→새wall+10을 Client439b39 프레임 위치에 연결했다. 영상 갱신 잠금/독립 검사 프레임에는 새 caller를 추가하지 않았다.1003의 로그인 완료 표식 설명은 서버Begin이 먼저LoginEnd0을 쓴다는 원문에 맞게 정정했으며 writer 코드는 이미 현재값을 사용했다.Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다.[1004 기록](docs/exe/cpp-saved-state-periodic-20261107.md). **다음은 실제 파일 reader/복구·일반 프레임486bc0/스펠 소비와 전체 decoder·메타/재접속/Outpost/checking7이다.**

### 0.21 최근 단위 — 1005 실제 startup reader/질문 표시

1004는 `4558729b / 20261107-18 CP`로 커밋됐다.1005는 실제 사용자 파일 reader를 같은 Player/Core metadata/복구 슬롯에 연결하고, 원문 startup의 번역 주소/버튼256BYTE/질문 서식과cf550 실제 HTML을 공급했다. 부재 관문/짧은 읽기/부분 복원은 기존 RawSavedStateRead를 사용한다.Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다.[1005 기록](docs/exe/cpp-saved-state-read-20261107.md). **다음은 StateReconnect/DelStateFile 입력·004b8cb0 서버 이전/복구State7 인계와 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking7이다.**질문 표시를 버튼 실행/재접속 완료로 쓰지 않는다.

### 0.22 최근 단위 — 1006 실제 일반 전투 종료 관문

1005는 `caa70291 / 20261107-19 CP`로 커밋됐다.1006은00449200/448f50/448fd0/4490d0의 실제 참가자/동맹/사제/flags 관문을 작성하고 기존 Player 패킷 소비자의 HasMenus 대체를 교정했다.CD 기존 어댑터는 유지한다.첫/보완 Release 각각 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다.[1006 기록](docs/exe/cpp-battle-completion-20261107.md). **다음은 StateReconnect/DelStateFile 입력·004b8cb0 서버 이전/복구State7 인계와 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking7이다.**

### 0.23 최근 단위 — 1007 질문No/DelStateFile 입력

1006은 `46d64b2a / 20261107-20 CP`로 커밋됐다. 1007은 DelStateFile의 실제 이름표/020000ac·HTML 일반 FIFO와 원문4b5880의 현재 경로 삭제를 연결했다. 삭제 반환을 무시하고 새 상태 전환/월드 반납을 추가하지 않는다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사·실제 파일 삭제0이다. [1007 기록](docs/exe/cpp-saved-state-delete-command-20261107.md). **다음은 StateReconnect/ForceServerDeath·004b8cb0의 서버 선택/State15/복구State7과 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking7이다.**

### 0.24 최근 단위 — 1008 같은 LAN owner의 서버 역할 전환

1007은 `3bd0b5bd / 20261107-21 CP`로 커밋됐다. 1008은 실제 server 권한을 받은 LanClient의 Serve가 같은 TCP/Player/Registry/등록표와 장기 owner를 유지해 host 내부 backend를 늦게 만들도록 연결했다. callback 중 전환은 거부하고 TCP 성공은 뒤 실패에서 되돌리지 않는다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [1008 기록](docs/exe/cpp-lan-server-promotion-20261107.md). **다음은 State15 부모/63h·StateReconnect/ForceServerDeath/004b8cb0·복구State7과 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking7이다.**

### 0.25 최근 단위 — 1009 실제63h 수신/참가 순서·객체 정리

1008은 `049ef793 / 20261107-22 CP`로 커밋됐다. 1009는004a7510/63h의 실제 가입/응답/metadata·참가 순서 복구·client SID 환불/가상count/삭제와Ready/Begin·checking 미발견 꼬리를 연결했다. PE의 owner번호/+5c 비교와 진단0/1b2를 대조했다. 첫 빌드의 log 선언 한 원인/오류2를 교정한 최종 Release는 종료0·경고/오류0이며 보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. checking잔여6(부모2/부분1/MP3)이다. [1009 기록](docs/exe/cpp-reconnect-player-20261107.md). **다음은 State15 부모/63h 송신·StateReconnect/ForceServerDeath/004b8cb0·복구State7과 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking6이다.**

### 0.26 최근 단위 — 1010 실제 State15 서버 인수

1009는 `12e76ceb / 20261107-23 CP`로 커밋됐다. 1010은 같은 실제 Login/World의 State15와63h 송신·flags702/서버 free목록/로그인타이머·시계3DWORD reset/시각 복구와15→16 공통확정을 연결했다. callback 중 실행은 거부하며 실제 일반 입력/Poll의 진입 API를 마련했다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [1010 기록](docs/exe/cpp-server-takeover-20261107.md). **다음은 StateReconnect/ForceServerDeath/004b8cb0 caller·복구State7/전역handle 인계와 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking6이다.**

### 0.27 최근 단위 — 1011 현재 handle의 Client 수명

1010은 `9986097a / 20261107-24 CP`로 커밋됐다. 1011은PTR00540ca8의 현재 이름64BYTE를 실제 ClientSubscriberGlobals가 소유하고 기존 Session의 모든 handle 소비자가 같은 참조를 빌리도록 보완했다. Client를 쓰지 않는 독립 Session만 기존 지역 버퍼를 유지한다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [1011 기록](docs/exe/cpp-current-handle-owner-20261107.md). **다음은 StateReconnect/ForceServerDeath/004b8cb0 caller·복구State7과 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking6이다.**

### 0.28 최근 단위 — 1012 원문 저장 주소 문자열

1011은 `bed549ff / 20261107-25 CP`로 커밋됐다. 1012는 원문4e2620이 입력 문자열을 해석한 뒤 별도 포트를 적용한다는 점과 상태+54의40BYTE 영역을 대조하고, 현재 재접속 표식에서 문자열을 같은 실제TCP/내부 backend에 전달하도록 연결했다. 기존 typed LAN 시작은 유지한다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사0이다. [1012 기록](docs/exe/cpp-reconnect-address-text-20261107.md). **다음은 StateReconnect/ForceServerDeath/004b8cb0 caller·복구State7/현재Fort 이름 인계와 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking6이다.**

### 0.29 최근 단위 — 1013 복구State7의 Fort 이름 선택

1012는 `f7210282 / 20261107-26 CP`로 커밋됐다. 1013은 현재 복구7의 pending 이름을 기존00460980/같은 TemplateStorage·snapshot/Notify·제목/dirty와 사용자 파일 namespace에 연결했다. 기본 State7 hook도 같은 장치를 사용하고 현재 GeneralBattle의 실제 모듈 준비 뒤 수명 관문을 유지한다. Release 빌드 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사·실제 이름 변경0이다. [1013 기록](docs/exe/cpp-reconnect-fort-selection-20261107.md). **다음은 StateReconnect/ForceServerDeath/004b8cb0 caller·복구State7 요청과 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking6이다.**

### 0.30 최근 단위 — 1014 실제 서버 선택/복구 일반 입력

1013은 `e064f175 / 20261107-27 CP`로 커밋됐다. 1014는004b8cb0 전체 reader/서버 후보 순환·현재15/다음7·3·실패 Tell/삭제와 StateReconnect/ForceServerDeath 일반 FIFO를 연결했다. 같은 실제 상태/handle을 다음 일반 World 설치에 인계하고 저장 복구의 빈 이름 계약을 보완했다. NotRestart 공장을 원문 일반Tell로 정적 교정한 첫/보완 Release 각각 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사·실제 삭제0이다. checking잔여5(부모2/부분0/MP3)다. [1014 기록](docs/exe/cpp-battle-reconnect-selection-20261107.md). **다음은 자동004b9010 heartbeat/timeout·서버 실패 caller와 일반 프레임486bc0/스펠·전체 decoder·메타/Outpost/checking5다.**

### 0.31 최근 단위 — 1015 실제 연결 감시/자동 장애 caller

1014는 `fb2caed4 / 20261107-28 CP`로 커밋됐다.1015는439ad8/004b9010의 실제64h·현재 기한/활동표·지연kind1/2·서버 장애 flags/저장과 자동004b8cb0을 연결했다. 실제Client 타이머는439ae3로 옮기고 독립 Session은 기존 Pump를 유지한다.Release 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신 검사·실제 장애 저장0이다.[1015 기록](docs/exe/cpp-connection-health-frame-20261107.md). **다음은 일반486bc0/스펠·비미션 너겟 배치와 전체 decoder·메타/Outpost/checking5다.**

### 0.32 최근 단위 — 1016 실제 비미션 보충 배치

1015는 `37dcdfc2 / 20261107-29 CP`로 커밋됐다.1016은00485e40의 미연결 하위0040e050을 현재모드/지역0 seed·오벨리스크 TYPE112와 같은 Factory/MayPlace/Pop에 연결했다. 실제 boss/!inSingle 진단 복귀를 사용한다.Release 종료0·경고/오류0·보호2800파일 불변·신규 MP/단위/게임/통신/실제 배치 검사0이다.[1016 기록](docs/exe/cpp-battle-nugget-placement-20261107.md). **다음은 일반486bc0/스펠 소비·SP 주기/간헐천과 전체 decoder·메타/Outpost/checking5다.**

### 0.33 최근 단위 — 1017 실제 주기 SP 수신/안내

1016은 `e6e448f7 / 20261107-30 CP`로 커밋됐다.1017은6ch 종류2의 현재 간격/금액 검증과 실제 SP 가산·Message4/18/현재 이름·금액·10초 수명을 연결했다. 원문 표3개를 공유하고 효과 뒤 packet/owner 재조회를 보완했다. 첫 Release 종료0/경고2를 지역명 교정한 최종 빌드는 종료0·경고/오류0이다.1015 첫 빌드의 같은 경고1도 기록을 보완했다.보호2800파일 불변·신규 MP/단위/게임/통신/SP전송 검사0이다.[1017 기록](docs/exe/cpp-battle-sp-notice-20261107.md). **다음은 일반486bc0/스펠·SP주기/간헐천,6ch 부족 자금·건설Reject의 실제00442b50 제재/Tell와 전체 decoder·메타/Outpost/checking5다.**

### 0.34 최근 단위 — 1018 실제 일반 자원 프레임

1017은 `53df9e61 / 20261107-31 CP`로 커밋됐다.1018은486bc0 전체 스펠 소비/최초예약·주기SP6ch·구역별/전체간헐천과같은실제Pool/옵션/자원/목록을Kernel뒤caller에연결했다. PE의ordered비교·count/기한store순서·구역target/후보/난수소비와00595124장기owner를보존한다.Release 종료0·경고/오류0·보호2800파일불변·신규 MP/단위/게임/통신·NativeCPU/fixture생성0이다.[1018 기록](docs/exe/cpp-battle-resource-frame-20261107.md). **다음은6ch부족자금·건설Reject의00442b50제재/Tell와전체decoder·메타/Outpost/checking5다.**

### 0.35 최근 단위 — 1019 실제 원격 Tell70h

1018은 `d913ec6a / 20261107-32 CP`로커밋됐다.1019는70h쓰기/현재tellLength송신관찰기·현재Tell수신/대상송신과Session등록수명을연결했다. 기존ChargeShortfall계산본체를재확인했으며그실제비용안내/caller는후속이다. 첫빌드의OriginalText헤더누락한원인/오류2를교정한최종Release는종료0·경고/오류0·보호2800파일불변·신규MP/단위/게임/통신/실제Tell검사0이다.[1019 기록](docs/exe/cpp-remote-tell-packet-20261107.md). **다음은기존ChargeShortfall의비용안내/6chcaller·SP/취소송신장치와전체decoder·메타/Outpost/checking5다.**

### 0.36 최근 단위 — 1020 실제 비용 부족·취소수신

1019는 `8ddcfc1d / 20261107-33 CP`로커밋됐다.1020은기존ChargeShortfall의실제70h비용안내·6ch환불/2fh취소·송금/운반caller와같은Session의2fh전체예측정리수신을연결했다.Reject는442af0,PlayerNoMake의433c30은별도전체객체decoder임을구분한다. 첫Release종료0/경고1을교정한최종빌드는종료0·경고/오류0·보호2800파일불변·신규MP/단위/게임/통신/실제실패검사0이다.[1020 기록](docs/exe/cpp-construction-failure-flow-20261107.md). **다음은433c30전체객체/검증·PlayerNoMake와전체파생writer/decoder·메타/Outpost/checking5다.**

### 0.37 최근 단위 — 1021 실제 전체 객체 수신·검증

1020은 `e3517f06 / 20261107-34 CP`로 커밋됐다. 1021은433c30의 전체 객체 필드·좌표16/11 변환·첫 배치/현재 기술 장부·PlayerNoMake/Player400 표식·종속 부착과2ch 등록/해제 수명을 연결했다. 검증 DWORD와 다섯 불일치 로그도 같은 장기 owner에 연결했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1021 기록](docs/exe/cpp-squid-full-receive-20261107.md). **다음은7ah 압축 decoder/수신 owner 캐시·검증 시작 부모와 파생 writer/decoder·메타/Outpost/checking5다.**

### 0.38 최근 단위 — 1022 실제 압축7ah 객체 수신

1021은 `8ee7666b / 20261107-35 CP`로 커밋됐다. 1022는 생성자 기본값·0..15 필드 복원/상충 진단·현재 TYPE 최대HP와 zOrder·공유 owner 캐시를 연결했다. 실제 길이 관찰기/7ah 등록과 같은 공통 수신 직접 호출·해제 수명도 연결했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1022 기록](docs/exe/cpp-squid-compressed-receive-20261107.md). **다음은4ah/4bh 검증 시작·이전 SID 순회와 파생 writer/decoder·재동기화 부모·메타/Outpost/checking5다.**

### 0.39 최근 단위 — 1023 실제 검증 패킷 진입/간격

1022는 `96904de5 / 20261107-36 CP`로 커밋됐다. 1023은4ah/4bh의 free/type 비교·현재 sid/별도 이전 위치·signed65535/15000 순회·두 간격 불일치 로그와 실제 등록/반납 수명을 연결했다. 다음2ch/7ah가 같은 검증 SID를 읽는다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1023 기록](docs/exe/cpp-squid-verification-entry-20261107.md). **다음은2dh 내용물/2eh 삭제·파생 writer/decoder와 재동기화 부모·메타/Outpost/checking5다.**

### 0.40 최근 단위 — 1024 실제 내용물/삭제 수신

1023은 `147d4a39 / 20261107-37 CP`로 커밋됐다. 1024는2dh의 현재 Take/필드/가상 부착과2eh의 boss 진단·현재 TYPE 로그/선택 환불·가상 삭제/같은 SP 추적 꼬리를 연결했다. 실제 등록/해제 수명도 포함한다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1024 기록](docs/exe/cpp-squid-mutation-receive-20261107.md). **다음은 프로세스/감시 decoder와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.41 최근 단위 — 1025 실제 감시/공유 예약 수신

1024는 `6adc6e88 / 20261107-38 CP`로 커밋됐다. 1025는65h의 현재 Take/감시 가상74 부착과7eh의 저장 시각·사건·payload·횟수/현재 공유 TYPE·동일 Kernel 부착을 연결했다. 일반 공유 생성자의 현재 시각 쓰기를 넣지 않았다. 실제 등록/반납 수명도 포함한다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1025 기록](docs/exe/cpp-form-regular-receive-20261107.md). **다음은 나머지 프로세스 decoder와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.42 최근 단위 — 1026 실제 총 전체 수신

1025는 `6fefcff5 / 20261107-39 CP`로 커밋됐다. 1026은3ah의 수신용 생성자/dirty decoder·before/after observer·신규 명시 Form 부착/기존 PID 갱신과 같은 장기 총 TYPE를 연결했다. 실제 등록/반납 수명도 포함한다. 첫Release 종료0/경고2건을 교정한 최종 빌드 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1026 기록](docs/exe/cpp-gun-full-receive-20261107.md). **다음은 아비어리/비행/애니메이션 decoder와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.43 최근 단위 — 1027 실제 아비어리 전체 수신

1026은 `ceb59671 / 20261107-40 CP`로 커밋됐다. 1027은38h/73h/74h의 신규 생성자·공통/기존 decoder·같은 PID/명시 Form/장기 TYPE와 실제 등록·반납 수명을 연결했다. NULL PID의 미선택 객체 필드 무동작도 원문에 맞췄다. 첫/보완 Release 각각 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/공중3종 전용 검사0이다. [1027 기록](docs/exe/cpp-aviary-full-receive-20261107.md). **다음은 비행/애니메이션 decoder와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.44 최근 단위 — 1028 실제 비행 전체 수신

1027은 `8b15b357 / 20261107-41 CP`로 커밋됐다. 1028은39h의 임시 Sun 해석/현재 부모 테마별 기본 생성·임시 소멸·재해석과 같은 PID/명시 Form 수신을 연결했다. 실제 observer/소멸 쓰기와 기본0.0을 PE로 대조하고 writer/reader 필드 정의를 공유했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/공중3종 전용 검사0이다. [1028 기록](docs/exe/cpp-flyer-full-receive-20261107.md). **다음은 애니메이션 decoder와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.45 최근 단위 — 1029 실제 발사·회전 전체 수신

1028은 `bf74b6dd / 20261107-42 CP`로 커밋됐다. 1029는3bh/3ch의 네 실제 기본 생성자/공통 decoder와 같은 PID/명시 Form 부착·등록 수명을 연결했다. 기본 생성자의 미작성 필드와 NULL PID의 미선택 저장 무동작도 보존했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1029 기록](docs/exe/cpp-animation-full-receive-20261107.md). **다음은 건설37h/PlayerKnown·나머지 decoder 감사와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.46 최근 단위 — 1030 애니메이션 송신/같은 TYPE

1029는 `c193d582 / 20261107-43 CP`로 커밋됐다. 1030은 실제 발사7/회전4 VT 선택과 일반/전체 writer·공통2048BYTE 버퍼/전송을 연결하고 생성/조회/수신의35/36·부모71/128/140을 같은 장기 주소에 통합했다. 부모0 길이와 현재 헤더도 보존했다. CD의 대응 wrapper/필드 폭·헤더 차이를 정적으로 기록했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1030 기록](docs/exe/cpp-animation-transmission-20261107.md). **다음은 건설37h/PlayerKnown·나머지 decoder 감사와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.47 최근 단위 — 1031 실제 건설37h/PlayerKnown

1030은 `02b00b12 / 20261107-44 CP`로 커밋됐다. 1031은 건설37h의 실제 Session 생성/등록·일반/전체 writer/공통 버퍼와 같은 TYPE/헤더를 연결했다. 두 진단의 정상 복귀/decoder 부분 쓰기와 부모0을 원문에 맞추고 내용물 통지의00595344를 현재PlayerKnown에 교정했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신 검사0이다. [1031 기록](docs/exe/cpp-construction-data-integration-20261107.md). **다음은 Path 등 나머지 decoder 감사와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.48 최근 단위 — 1032 실제 Path 전체 수신

1031은 `a942a75a / 20261107-45 CP`로 커밋됐다. 1032는30h의 전체 압축/직접 좌표 decoder·실제 cdecl 생성자/명시 Form 부착·의식 끝점/Prepare/5105 삭제와 송신 길이 observer를 연결했다. 일반/전체 전송 분배와 이동의네 stale boss 읽기도 교정했다. 첫/보완 Release 각각 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/공중3종 전용 검사0이다. [1032 기록](docs/exe/cpp-path-data-integration-20261107.md). **다음은 나머지 수신/전송 감사와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.49 최근 단위 — 1033 공간 세 수신과 실제 전역

1032는 `5318fdd6 / 20261107-46 CP`로 커밋됐다. 1033은33h 위치/34h 카메라 목표/35h 섬 정리 수신과 동일 TYPE·별도 억제 전역·9/1/1 진단을 실제 Session에 연결·교정했다. PE의 FILD32×2^-16→FSTP32를 확인했고 CD의 예약 범위/frame BYTE 차이도 문서화했다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/공중3종 전용 검사0이다. [1033 기록](docs/exe/cpp-spatial-packet-integration-20261107.md). **다음은31h/32h 등 나머지 수신/전송 감사와 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.50 최근 단위 — 1034 현재32h 설정과 소비자

1033은 `4717dff5 / 20261107-47 CP`로 커밋됐다. 1034는32h의7DWORD를 Login 지역 사본에서 Client 장기 owner로 옮기고 두 배치 파이프라인·사제/일반 피해·표적/총·배리어/지상 사격·Wind/Rain/애시드·생산 타이머가 같은 현재값을 읽도록 연결했다. 원문32h에 없는 즉시 UI/에너지 효과는 추가하지 않았다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/공중3종 전용 검사0이다. [1034 기록](docs/exe/cpp-current-mission-settings-20261107.md). **다음은31h 해석기/명령 효과·비Login/Editor32h 등록 연결 등 나머지 수신/전송과 재동기화·Activate 부모·메타/Outpost/checking5다.**

### 0.51 최근 단위 — 1035 개발자 핵심 명령과 등록 수명

1034는 `a84509c9 / 20261107-48 CP`로 커밋됐다. 1035는31h 길이 관찰/현재 문자열과 핵심16명령·quickBuild/quickVortex·실제CRT/생성·삭제/피해/abstract를 연결했다. 비Login/Editor32h 등록 잔여도 해소했다. **31h 나머지17명령과cr65h UI 소비자는 미완료다.** 첫/보완 Release 각각종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/공중3종 전용 검사0이다. [1035 기록](docs/exe/cpp-developer-command-integration-20261107.md). 다음은31h의화면 강조·덤프/진단/나머지효과·입력과재동기화·Activate 부모·메타/Outpost/checking5다.

### 0.52 최근 단위 — 1036 화면 강조·진단과 사용자 요청 일시 정지

1035는 `52eae6bc / 20261107-49 CP`로 커밋됐다. 1036은4345e0의현재카메라/노랑3겹사각형/즉시출력과기존44dab0 trace,help32문자열·소유자/HP·Player/동맹dump6명령을접합했다. 31h는22개접합/11개미완료다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/진단GUI/공중3종전용검사0이다. [1036 기록](docs/exe/cpp-developer-diagnostics-integration-20261107.md). **사용자 명시 요청에 따라 문서/로컬 커밋 마감 후 목표 일시 정지.** 재개시31h 나머지11명령/입력·cr65h→나머지MP/재동기화·Activate·메타/Outpost/checking5→한국어후속→시나리오UI 순서다.

### 0.53 최근 단위 — 1037 재개·개발자 설정 명령

1036은 `269682b7 / 20261107-50 CP`로 커밋됐다. 이번 /goal로 재개해 setsubid/sbkp/stl을 기존 현재 Subscriber/스캔/Config 몸체에 연결했다. 31h25개 접합/8개 미완료이며 실제 입력·cr65h는 잔여다. Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신검사0. [1037 기록](docs/exe/cpp-developer-state-commands-20261107.md). 다음은31h 잔여8명령→입력·cr65h→LAN 나머지→한국어 후속→시나리오UI다.

### 0.54 최근 단위 — 1038 현재 TYPE/그림/graph와 프레임 진단

1037은 `162ddc94 / 20261107-51 CP`로 커밋됐다. dumptypes/dumptype/dad/dg/ht를 현재 TYPE/shape/graph와 실제 Frame에 연결해31h30개접합/3개잔여다.437f0 단일RET/두 BSS0의 PE를 확인해 새 힙 집계를 합성하지 않았다. 제품 Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신검사0. [1038 기록](docs/exe/cpp-developer-type-diagnostics-20261107.md). 다음은qdd/addLabel/getPriest 및 실제 입력·cr65h→LAN 나머지→한국어후속→시나리오UI다.

### 0.55 최근 단위 — 1039 qdd 시스템 버전과 사제 명령

1038은 `fcc4cc15 / 20261107-52 CP`로 커밋됐다. qdd/gp를 실제 Windows 버전 리소스와 현재 Finder/생성/WORD·owner/발자국/배치·가상피해에 연결해31h32개접합/1개잔여다. gp의 원래 사제SID 인자를 기계어로 확인했다. 첫빌드 경고1은명시WORD변환으로수정했고보완제품 Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신검사0. [1039 기록](docs/exe/cpp-developer-ddraw-priest-20261107.md). 다음은addLabel/기록부모·실제입력/cr65h감사→LAN나머지→한국어후속→시나리오UI다.

### 0.56 최근 단위 — 1040 addLabel과 실제 건설 기록

1039는 `20557f32 / 20261107-53 CP`로 커밋됐다. addLabel/실제Ctrl+F6 시작·종료/473e0 배치writer·장기FILE/시각과 fresh/반납시 두 기록DWORD 보존을 연결했다. 사용자홈Captures/BuildRecords/d에번호.rec를생성하며개발자플래그는켜지않는다.31h33개분기를모두접합했지만입력/송신·기록재생/cr65h감사는남는다.제품 Release 종료0·경고/오류0·보호2800파일 불변·신규MP/단위/게임/통신/실제기록검사0. [1040 기록](docs/exe/cpp-developer-build-record-20261107.md). 다음은해당감사→LAN나머지→한국어후속→시나리오UI다.

### 0.57 최근 단위 — 1041 직접 입력/송신·ChangeCR 감사

1040은 `b234f02e / 20261107-54 CP`로 커밋됐다.1041은31h 헤더의직접참조/데이터포인터0·해석기직접caller44dba0 하나와ChangeCR의생산자이름표/행·열참조를대조했다.새로만들로컬sender/UI크기소비자가있다는가정을해소했다.제품소스/EXE불변·새빌드/검사/게임/통신0·보호2800불변. [1041 기록](docs/exe/cpp-developer-input-audit-20261107.md).다음은기록재생4957a0(gaps180바이트)/reader·입력·Client/Kernel부모→LAN나머지→한국어후속→시나리오UI다.

### 0.58 최근 단위 — 1042 기록 재생 판독/라벨·Run 부품

1041은 `33d45df7 / 20261107-55 CP`로커밋됐다.1042는기존gaps의4957a0/180바이트를PE로분석해200×40BYTE 판독/부분쓰기·한프레임한이벤트/strict시각·품질BYTE보존/라벨몸체를CMake에등록·컴파일했다.실제Client/Kernel/홈reader·Ctrl+F7/건설Confirm부모는미접합이다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0. [1042 기록](docs/exe/cpp-build-replay-body-20261107.md).다음은해당실제부모접합→LAN나머지→한국어후속→시나리오UI다.

### 0.59 최근 단위 — 1043 기록 재생 실제 부모

1042는 `ef2a7ffb / 20261107-56 CP`로 커밋됐다.1043은 같은 Client 전역/실제 Kernel·홈 최신rec·일반Ctrl+F7/Shift NextStage·건설 확정 후 현재 라벨 통지와 실제 가상 선택/표시를 연결했다. 기본/자산 생성자83VT의7c/88을 PE로 대조해 기존 실행기를 재사용한다. 첫 Release 종료0/경고3을 수명 토큰 폭 명시로 교정한 최종 빌드는 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1043 기록](docs/exe/cpp-build-replay-integration-20261107.md). 다음은 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.60 최근 단위 — 1044 삭제 선택자·공통 전송 부모

1043은 `44e53bdc / 20261107-58 CP`로 커밋됐다.1044는 현재+58/삭제 선택자(-2)와 같은 버퍼·대상별 송신 부모를 실제 Session 삭제에 연결했다.87VT의 공통004ac340과 헤더→진단→표식 소비 순서를 PE로 대조했다. 재동기화 허용/owner 캐시 부모는988 연결을 재확인해 이월 잔여를 해소했다. Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1044 기록](docs/exe/cpp-squid-deletion-transmission-20261107.md). 다음은 나머지 LAN writer/decoder·일반 프레임·메타/Outpost/checking5→한국어 후속→시나리오 UI다.

### 0.61 최근 단위 — 1045 실제2번 종료 통지·선택 활동표

1044는 `b8cadde1 / 20261107-59 CP`로 커밋됐다.1045는 누락됐던004b6660/2번 통지와 실제 현재 World/Registry·Session pin·서버 실패/Player flags·상태 방송 뒤 선택frame0을 연결했다. 전체22DWORD clear와64h/timeout UI는 기존 연결임을 재대조했다. 첫 C2660 한 오류/종료1을 교정한 최종 Release는 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1045 기록](docs/exe/cpp-connection-ended-20261107.md). 다음은1번004e2f00/접속 전 별도22송신버퍼와 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.62 최근 단위 — 1046 실제1번 접속 완료·별도 송신버퍼

1045는 `269de895 / 20261107-60 CP`로 커밋됐다.1046은 실제 TCP/내부/Editor의1번 통지와 같은 Registry의 별도22송신버퍼 복제/현재 가상 송신·현재 칸 재조회/free/0을 연결했다. 실제 Connect 성공 뒤 이전 버퍼 반납과 observer1/현재 길이 복제를 사용하며 원문 직접4호출의 NULL 인자를 PE로 대조했다. Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1046 기록](docs/exe/cpp-connection-established-20261107.md). 다음은 endpoint/서버·기존 연결 재사용과 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.63 최근 단위 — 1047 기존 연결 재사용·실제 socket 준비

1046은 `621efab7 / 20261107-61 CP`로 커밋됐다.1047은 Registry의 실제 TCP/내부/Editor 준비 장치와 원문 비영(-1포함) 재사용 guard·주소 해석 순서/새 실패false·성공 뒤 직접 칸 공개/초기 버퍼 처리를 연결했다. 상태는 캐시 대신 실제 select를 사용하고 조회 자체와 Poll의 통지를 구분한다. listener 기존 핸들 관문도 맞췄다. Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1047 기록](docs/exe/cpp-connection-reuse-20261107.md). 다음은 endpoint/서버·전체 수락/전송과 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.64 최근 단위 — 1048 실제 수락 순서·상한/진단

1047은 `31f08f76 / 20261107-62 CP`로 커밋됐다.1048은 실제Poll의zlocal→TCP 순서와 signed31 개수 상한,select준비/accept음수의 진단 뒤 반환·listener 보존을 교정했다. 부모 socket 속성 복제에 맞춰 추가ioctl을 제거했다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1048 기록](docs/exe/cpp-connection-accept-20261107.md). 다음은 제외 방송 진단/나머지 송신과 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.65 최근 단위 — 1049 제외 방송 실제 진단/정상 복귀

1048은 `6b700418 / 20261107-64 CP`로 커밋됐다.1049는4e2e80의 server 진단을 같은 실제 Client 장치에 연결하고 정상 복귀 뒤 로컬 번호 저장/현재 접속·연결 순회를 보존했다. 직접 caller는 채팅이 아닌004dc050 신전 섬 상승 수신이며 아직 미연결임을 확인했다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1049 기록](docs/exe/cpp-broadcast-diagnostic-20261107.md). 다음은004dc050/004dbce0·메타/Descend와 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.66 최근 단위 — 1050 실제 신전51h와 상승 완료

1049는 `167e7b79 / 20261107-65 CP`로 커밋됐다.1050은 실제51h 수신/서버 제외 중계·Regular 예약과004dbce0의현재 Territory/지형 기록·가상owner0·동적 표면/자산 정리·마지막 자기 섬 처리를 연결했다. 기존 상승 Tick과event3 방송/직접 수신·현재 상태/TYPE 통지도 연결했다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1050 기록](docs/exe/cpp-vortex-departure-20261107.md). 다음은004dbac0 command/Descend·event4/메타와 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.67 최근 단위 — 1051 신전명령·ViewMeta 저장과 State19 요청

1050은 `11fbb988 / 20261107-66 CP`로 커밋됐다.1051은 네 신전VT의 실제 명령/확인WORD·같은 섬 사제 검사/51h 요청과Descend event4의 실제Fort지역→현재서버/ViewMeta 큐를 연결했다. 일반 입력의 서버 검증/무효설정과 현재Territory/전체Fort 저장 뒤State19 요청을 연결했다. 이름표상02000013은ViewMeta이며 State19/CS 실제부모는 아직 미연결이다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1051 기록](docs/exe/cpp-vortex-command-descend-20261107.md). 다음은State19/CS·메타와 LAN 나머지→한국어 후속→시나리오 UI다.

### 0.68 최근 단위 — 1052 사용자 제보 SubscriberGlobalsTests C2607

1051은 `da2bf5b8 / 20261107-68 CP`로 커밋됐다.1052는 사용자 제보의40행/50열 C2607을 같은 실제 파일로 재현하고, Handle64BYTE 추가 뒤 남은 전체클래스8BYTE 단언을 실제 필드 폭/참조·이름 버퍼 계약 검사로 교정했다. 제품1051/EXE는 불변이다. 해당 파일 격리Release 종료0·경고/오류0·기존7검사 통과·보호2800불변이며 전체테스트/게임/통신 검사는 하지 않았다. [1052 기록](docs/exe/cpp-subscriber-globals-compile-fix-20261107.md). 제보 건 해결, 다음은State19/CS·메타와 기존 LAN 나머지다.

### 0.69 최근 단위 — 1053 CS 기록/생성·재사용과 같은 Client 수명

1052는 `aebc8d24 / 20261107-69 CP`로 커밋됐다.1053은0042cec0/42ba10의 실제5350h/10전투·40참가자 기록과 최초 좌표 초기화, 같은 Client 포인터/할당 수명을 작성·컴파일했다. 로그인 outpost는 같은 현재CS 슬롯을 빌리며 독립 입력만 기존NULL fallback이다. **실제State19/28 전체 호출자·메타 입력/표시·후속 상태는 아직 미연결**이다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1053 기록](docs/exe/cpp-challenge-owner-20261107.md). 다음은 실제State19/CS 부모와 기존 LAN 나머지다.

### 0.70 최근 단위 — 1054 State19 부모 부품과 실제 뉴스 시각/수신 HTML

1053은 `6c50644e / 20261107-70 CP`로 커밋됐다.1054는State19의 실제 순서/CS/현재 요청25·34·21을 제품 라이브러리 부품으로 복원하고 별도CD005c85bc/보조005c85e8 저장소를 추가했다. 같은 Client 뉴스 키/시각과 실제 채팅 수신의 설정 저장/HTML400×30을 연결했다. **State19 실제 Uber/Fort/Core/채널 장치·즉시 다음 상태/메타 표시는 아직 미접합**이다.Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1054 기록](docs/exe/cpp-meta-state-news-20261107.md). 다음은 실제State19/즉시 다음 상태와 메타/LAN 잔여다.

### 0.71 최근 단위 — 1055 실제 State19/Fort/Core/채널과 Client 수명

1054는 `8efc76eb / 20261107-71 CP`로 커밋됐다.1055는 실제 Uber/큰 부모19와 같은 Client의 Fort/Core/CS/뉴스/채널을 접합했다. 기존 Session도 공통 Core 읽기와 같은 중첩깊이/현재 표본6을 사용한다. **즉시21/25/34 부모는 미연결이므로 현재 메타 진입은 해당 invalidState 경계에서 중단한다.** 구형 전체4BYTE 검사 단언도 필드 폭으로 교정해 해당 파일 컴파일0(실행0)을 확인했다. Release 종료0·경고/오류0·보호2800불변·신규MP/단위/게임/통신0이다. [1055 기록](docs/exe/cpp-meta-state-client-20261107.md). 다음은 즉시21/25/34 실제 로컬 부모와State30/메타/LAN 잔여다.

### 0.72 최근 단위 — 1056 실제 메타20~27와 endpoint/TCP10·11 요청

1055는 `c1c8bd0a / 20261107-72 CP`로 커밋됐다.1056은 실제20~27/현재 서버 행·포트/TCP10·11과16h/1ch 요청·관찰 길이/현재 Client 수명을 접합했다. 폐기 도메인/공개주소는 기존 LAN 정책으로 거부한다. CS 다른reset 번호는State28로 정정했다. **검색34~38/로컬 서버·로그인 응답0Fh/1dh·State28~33/메타는 아직 미연결**이다. 첫 C1075/다음 C4244 두 건을 교정한 최종Release 종료0·경고/오류0·보호2800불변·새MP/단위/게임/통신 검사0이다. [1056 기록](docs/exe/cpp-meta-connection-20261107.md). 다음은 검색/응답과 메타/LAN 잔여다.

### 0.73 최근 단위 — 1057 R 정밀 디컴파일과 로컬 검색 부품

1056은 `6bff48b8 / 20261107-78 CP`로커밋됐다.1057은원본R.exe를별도프로젝트에서정밀디컴파일해5308함수/실패0·미정규약184를기록했고,선택적thunk호출추적/자동인자해제와읽기전용6함수내보내기를추가했다.클라이언트양판본전체정밀은2026-11-03/HJOW-Athlon그대로이며R전용은2026-11-07/HJOW-Athlon이다.66h/공유CRT토큰/State34·35의두기한/현재36발견수신·R의같은서버응답세번송신을제품라이브러리에준비했다. **실제UDP/장기전역/큰State34~38/새로컬서버실행대상은아직미연결**이다. Release최종종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신검사0이다. [1057 기록](docs/exe/cpp-local-server-discovery-20261107.md).

### 0.74 최근 단위 — 1058 실제 UDP 검색과 메타34·35·36·38

1057은 `6272726c / 20261107-82 CP`로 커밋됐다.1058은 실제UDP12/같은연결·패킷표/현재송신자주소·원문길이/종료·깊이반납,같은Client검색전역·현재BroadcastPort/CRT와메타34·35·36·38을연결했다.66h발견은현재36을쓴뒤같은Run을동기실행한다. **37의새R서버실행/Root·CS관리자·004b9230수동선택/재시도UI·0Fh/1dh응답/메타는남는다.** 만료후37은미복원부모에서명시오류이며전체검색완료가아니다. 첫/최종Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신검사0이다. [1058 기록](docs/exe/cpp-meta-udp-discovery-20261107.md).

### 0.75 최근 단위 — 1059 State37 경로 부모와 R 연결/모드

1058은 `eb680d67 / 20261107-84 CP`로 커밋됐다.1059는37의실제ReadString/파일명고리·VFS/여섯후보·실패34/선택lan·wait0/뒤38부모를연결했다. R전용256일반/32대기/51Challenge표·개수초기1/일반·대기·복합종료와정확한lan인자/후반모드비트를컴파일했다. **새R.exe/OS실행장치·Root/CS관리자는미구현**이다. 후보존재시선택실행장치미연결오류다. R응답10.71/클라이언트메타·게임검사차이·서로다른CS물리와LAN등록메시지누락을기록했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신검사0이다. [1059 기록](docs/exe/cpp-root-routing-launch-20261107.md).

### 0.76 최근 단위 — 1060 실제 등록 메시지/설정 재적재

1059는 `dcf0013e / 20261107-86 CP`로 커밋됐다.1060은 NETSTORMOOGABOOGA 등록/현재 두 DWORD/실제 WndProc,100h 빈 명령줄 재적재·명시 해석·등록ID 두 번 조회,200h/300h 표식과500h 현재 전체화면 창 복원을 연결했다. getter/400h 동기 송신 장치는 추가했으나 일반 명령 호출자는 후속이다. 현재 Session의 Fort 동기화는 기존 writer/Changed를 공유하고, manager/fort가 살아 있는 비Session 경계는 명시 오류로 남긴다. 첫 빌드 멤버 오기/오류2 교정 뒤 최종 Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신 검사0이다. [1060 기록](docs/exe/cpp-registered-messages-20261107.md).

### 0.77 최근 단위 — 1061 비Session 등록 ID 저장과 도구 명령 분류

1060은 `9b2a0580 / 20261107-89 CP`로 커밋됐다.1061은 World 없는 현재 Fort의 등록 ID 변경→실제 Subscriber 생성/쓰기·등록 설정/CRT→공유 Notify→같은 사용자 작업 파일 저장을 연결했다. 현재 manager/fort·isMine/preventSave·현재 미션/저장/숨김 관문을 유지한다. 기존 Session도 같은 Notify를 쓰고 현재 callback은 선택 시 조회한다. AccountSetup/0200007a와 setup.exe register/Tell Setup을 확인해 구 외부 등록 실행은 연결하지 않는 것으로 분류했다. 첫/최종 Release종료0·최종 경고/오류0·보호2800불변·새 단위/MP/게임/통신/파일 저장 검사0이다. [1061 기록](docs/exe/cpp-registered-fort-subscriber-20261107.md).

### 0.78 최근 단위 — 1062 R 메인 통신 부모

1061은 `f261f530 / 20261107-91 CP`로 커밋됐다.1062는 R00476b80의 일반256/대기32/Challenge51×40·64칸 소켓 집합과 signed timeout/현재 순회·select 뒤 수신/flush·LAN 유휴 종료를 정적 라이브러리에 컴파일했다. 지역 시각/직전 참가자수 PE초기1·집합 중복·가득 찬 추가 생략·64번째 진단·콜백 뒤 현재 슬롯 재조회를 유지한다. **새R.exe/OS 장치·분배/Root·CS/호출은 미연결**이다. Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신 검사0이다. [1062 기록](docs/exe/cpp-root-network-pump-20261107.md).

### 0.79 최근 단위 — 1063 R 실제 IPv4 소켓과 공통 프레임

1062는 `150b6019 / 20261107-92 CP`로 커밋됐다.1063은 실제 R IPv4 UDP/TCP listen4·accept/select/수신·일시 nonblocking 송신/20BYTE sendto·핸들·공유WSA 수명을 작성했다. 기존 Client는 Player/22칸 Registry 없는 같은 공통 프레임 몸체에 연결했다. R/10.78 최대137/길이138개 동일을 PE로 확인했다. **R 전송 노드/버퍼/번호·깊이·NetworkPump 장치 접합/새R.exe·Root/CS는 미연결**이다. 첫/최종 Release종료0·최종 경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기 검사0이다. [1063 기록](docs/exe/cpp-root-socket-device-20261107.md).

### 0.80 최근 단위 — 1064 R 실제 전송 노드/수신·송신·깊이

1063은 `6c4a981c / 20261107-93 CP`로 커밋됐다.1064는 실제 R 노드의UDP512/listener8·accepted4096·부분 수신/관찰/공통 분배·직접/집합 송신/부분 flush·번호 토큰/깊이 닫힘과 실제 SocketDevice/NetworkPump를 연결했다. SETL 대기개수<31/매 송신observer/no-op·현재BYTE0/소멸flush/버퍼 모드 포인터를 대조했다. **새R.exe/프로세스·Root/CS/보조 객체·시계/기록/formatter와 실제 호출은 미연결**이다. 첫/최종 Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기 검사0이다. [1064 기록](docs/exe/cpp-root-transceiver-transport-20261107.md).

### 0.81 최근 단위 — 1065 R Root16h→0Fh 응답

1064는 `d1e53e49 / 20261107-99 CP`로 커밋됐다.1065는 실제 CCh2b80BYTE 뷰·현재 토큰/옵션과 Root16h의 추천 존/인원·평균/친구 최대16건·0Fh 관찰기/버전·실제 전송/음수 guard 접합 부품을 작성했다. **옛 Root 응답17h는 PE/기계어 근거로0Fh로 정정**하며17h는 별도 변경 통지다. 제공 R3BYTE/10.78 클라이언트47BYTE 존 항목 차이도 확인했다. 새R.exe/실제 초기화·CCh 관리자/Root·CS나머지·호출과0F/1d 클라이언트 수신은 남는다. Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기 검사0이다. 게임 EXE 해시는 이번 빌드 값으로 갱신했다. [1065 기록](docs/exe/cpp-root-directory-20261107.md).

### 0.82 최근 단위 — 1066 R 시작 설정/재해석 옵션

1065는 `c9b2eec8 / 20261107-100 CP`로 커밋됐다.1066은 R의 여섯 탐색/환경·파일 순서와 기존 Config Load·현재 callback/재해석26·최초33옵션/PE기본값·문자열을 같은 NetworkState/ConnectionState/Root 옵션 참조에 연결했다. 포트/개수/모드는 재해석에서 다시 읽지 않으며 폐기 클라이언트 검증 소비자를 연결하지 않는다. **새R.exe/실제 초기화 호출·moderation/기록 장치·CCh 관리자/Root·CS나머지와0F/1d 수신은 남는다.** Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기 검사0이다. [1066 기록](docs/exe/cpp-root-configuration-20261107.md).

### 0.83 최근 단위 — 1067 R CCh 물리/생성·현재 통신

1066은 `9e65b735 / 20261107-101 CP`로 커밋됐다.1067은 R CCh2b80BYTE/미작성 끝DWORD·별도 최초 좌표·실제 CRT/숫자 토큰과 Root 단일 프로세스 배치/index/flags·현재40토큰 수신/flush·명시 소멸/표0을 연결했다. RootDirectory도 같은 실제 바이트를 빌릴 수 있다. **새R.exe/실제 전체 초기화/호출·Root/CS나머지·참가자 로그인/종료·시계/기록/보조 객체는 남는다.** Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기 검사0이다. [1067 기록](docs/exe/cpp-root-challenge-store-20261107.md).

### 0.84 최근 단위 — 1068 Root1Ch 관문/실제 소켓 이전

1067은 `958526d9 / 20261107-103 CP`로 커밋됐다.1068은 Root1Ch의 주소/존/현재 CCh 관문·원문 로그와 실제 pending socket 이전/옛 노드 닫힘·64비트 핸들/WSA 유지·새 CCh sender0/4096/flags444→별도 peer/flags54를 연결했다. 내부 DWORD 토큰은 실제 CCh만 한 번 소비한다. **0044e940 로그인 본체/0044e0d0 종료·응답과 localAddress/새R.exe 실제 호출은 남는다.** 원문 hostname 해석 경계를 외부 서비스 문서에 추가했다. Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기/DNS 검사0이다. [1068 기록](docs/exe/cpp-root-login-request-20261107.md).

### 0.85 최근 단위 — 1069 실제 CCh 로그인/참가자 종료

1068은 `26b21de6 / 20261107-106 CP`로 커밋됐다.1069는 실제 로그인/종료 전체 부모와 참가자·전투 수명/명단·주소/메시지·길드 이름/관찰기를 같은 저장소·전송에 연결했다. 성공 상태1/DWORD subscriber, 공유scratch 본문 보존·OR,0번subscriber/source 길드 특이동작을 대조했다. 두 채팅 표는 실제guild/guildreq였다. **상위 표/파일·주소/Root 라우터·나머지 수신/입력·새R.exe/실제 호출은 남는다.** 첫/문자열 보완/길드 정정 뒤 Release3회 종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/소켓 열기 검사0이다. [1069 기록](docs/exe/cpp-root-challenge-runtime-20261107.md).

### 0.86 최근 단위 — 1070 Root 내부 라우팅과 원격 요약

1069는 `5d41eca5 / 20261107-113 CP`로 커밋됐다.1070은 같은 현재 패킷표의sender0/NULL 직접 호출과Root17h 원격 요약/로컬과 같은 참가자 생성자를 연결했다. Root후처리는 실제 모드 진단 후 반환이며 임의 방송을 추가하지 않는다. 첫/relay 보완Release2회 종료0·마지막 등록 보완은 링크 완료/최신 EXE/프로세스 종료 확인(종료코드 관측 유실), 세 로그 경고/오류0·보호2800불변·새 단위/MP/게임/통신 검사0이다. **공유 표/파일·주소·나머지 수신/입력·새R.exe 전체 초기화/최종 owner·실제 호출은 남는다.** [1070 기록](docs/exe/cpp-root-routing-20261107.md).

### 0.87 최근 단위 — 1071 Root 공유 파일과 사용자 홈 저장

1070은 `41e91d11 / 20261107-119 CP`로 커밋됐다.1071은 공유 관리6로더/길드·요청/mtime 뉴스·blacklist 감지와 실제 사용자 홈 blacklist 저장·같은 Config/CCh hook을 작성했다. 숫자 일반append/변경감지clear·IP 실제 앞부분 비교/이름 대소문자 조건/mtime checksum을 보존했다. 미발견 길드 파일은 빈 현재 표, 불완전 필수 행·버퍼초과는 호스트 경계다. Release 종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/새 런타임 파일 실행 검사0이다. **주소·나머지 수신·시계/기록/보조 객체·새R.exe 전체 초기화/최종 owner/실제 호출·watcher 주기는 남는다.** [1071 기록](docs/exe/cpp-root-shared-files-20261107.md).

### 0.88 최근 단위 — 1072 Root 주소와 실제 로컬 장치

1071은 `052b2278 / 20261107-121 CP`로 커밋됐다.1072는 주소5*30고리/80BYTE 문법·IPX dot포인터/같은 로그인·CCh hook과 실제 DNS 없는 로컬 IPv4 장치를 작성했다. 숫자/localhost/현재 컴퓨터 이름만 해석하며 다른LAN/공개 hostname은 DNS 없이주소0 실패다. 현재 LAN은 숫자 IPv4를 사용하며 다른 LAN hostname의 외부 DNS 없는 해석은 후속이다. 다중 어댑터의 구형resolver 순서 동일성은 미확인이다. 첫Release 종료0/C4996경고1·최종종료0/경고·오류0·보호2800불변·새 단위/MP/게임/통신/OS 조회 실행 검사0이다. **시계/기록/보조 객체·나머지 수신·새R.exe 전체 초기화/최종 owner/실제 호출은 남는다.** [1072 기록](docs/exe/cpp-root-address-codec-20261107.md).

### 0.89 최근 단위 — 1073 R 시계와 실제 기록

1072는 `ddf2ff64 / 20261107-124 CP`로 커밋됐다.1073은 R의 실제 FullTime/중첩 정지·보정/프레임 통계·epoch 초기화와 같은 현재 시계/사용자 Diagnostics 패킷 기록·Config/전송 hook을 작성했다. 기록은21BYTE 접두/현재 헤더 본문·6fwrite/flush이며 송신의5번째length는 원문 writer처럼 미사용이다. Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/시계·우선순위/기록 실행 검사0이다. **보조 객체·나머지 수신/입력·CCh주기/메인루프·새R.exe 전체 초기화/최종 owner/실제 호출은 남는다.** [1073 기록](docs/exe/cpp-root-clock-record-20261107.md).

### 0.90 최근 단위 — 1074 CCh 주기와 R 전역 난수

1073은 `51ba77f0 / 20261107-126 CP`로 커밋됐다.1074는 실제 CCh 주기/전투 재생성·가상 참가자/배치와 같은 현재 시계/R process 난수·기존23h/20h를 연결했다. 난수는CRT TLS가 아닌R 전역초기1이며 위치의stale 참가자0/40회 참조를 기계어로 대조했다. Release종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/주기 실행 검사0이다. **메인루프의 중복Tick·파일watcher/설정/예약리셋·종료·보조객체·나머지수신·새R.exe 전체초기화/최종owner/실제호출은 남는다.** [1074 기록](docs/exe/cpp-root-challenge-periodic-20261107.md).

### 0.91 최근 단위 — 1075 메인 루프와 예약 리셋

1074는 `7087ed4d / 20261107-127 CP`로 커밋됐다.1075는 실제 cycle/현재51CCh·중복Tick/통신·파일 감시/예약리셋과Root41h 안내·같은 현재 receiver를 연결했다. 중간471f20은 Config설정이 아닌rootservers.dat 목록 재로드였다. 첫/직접include 보완Release2회 종료0·경고/오류0·보호2800불변·새 단위/MP/게임/통신/메인·예약/창/파일 실행 검사0이다. **서버목록 본체·보조객체/나머지수신·41h/전체초기화·정리·새R.exe 최종owner/실제호출은 남는다.** [1075 기록](docs/exe/cpp-root-main-loop-20261107.md).

### 0.92 최근 단위 — 1076 서버 목록과 현재 체크섬

1075는 `b62f8f9a / 20261107-130 CP`로 커밋됐다.1076은 실제 rootservers 표/체크섬·행 파싱/갱신·상대경로 파일 재로드/사용자 홈 저장·같은 메인hook을 연결했다. flags7/체크섬byte+1·부분검색의저장행만소문자·기존행교체비재계산·개행없는EOF끝글자추가를원문으로대조했다. 첫Release종료0/C4244관측·최종종료0/경고·오류0·보호2800불변·새 단위/MP/게임/통신/파일 실행 검사0이다. **보조객체/나머지수신·41h/전체초기화·정리·새R.exe 최종owner/실제호출은 남는다.** [1076 기록](docs/exe/cpp-root-server-list-20261107.md).

### 0.93 최근 단위 — 1077 CCh 채팅 수신

1076은 `3451978b / 20261107-137 CP`로 커밋됐다.1077은00476880→004512b0의전체41h 수신/특별명령·줄/빈도 제한·같은입력패킷의전체/개별/감시자 전달을기존등록부모에연결했다. 등록버전1/작성3·정수초/idiv1000·동등경계reset·recipient제자리수정을원문/PE/기계어로대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새채팅실행검사0이다. **원문수신등록18중구현4/폐기80제외·나머지13수신과보조객체/전체초기화·정리/새R.exe최종owner·실제호출은남는다.** [1077 기록](docs/exe/cpp-root-challenge-chat-20261107.md).

### 0.94 최근 단위 — 1078 CCh 전투 제어 수신

1077은 `7c9f4f92 / 20261107-140 CP`로 커밋됐다.1078은10h 카운터 관문·67h 옵션/68h 조회/71h 설명의 전체 몸체와 현재CCh·등록/전송을 연결했다. bit2 해제·길이초과후통지·고정160조회·관리자의자기소속쓰기/설명길이0·공유scratch별칭과전체CCh물리접근을 원문/PE/기계어로 대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새수신 실행검사0이다. **원문18수신중8구현/폐기80제외·나머지9수신과보조객체/전체초기화·정리/새R.exe최종owner·실제호출은남는다.** [1078 기록](docs/exe/cpp-root-battle-control-20261107.md).

### 0.95 최근 단위 — 1079 CCh 이동과 전투 입장

1078은 `7560ed5d / 20261107-142 CP`로커밋됐다.1079는1Eh 전체이동/입장검사·기존소속탈퇴/master·명단과보관BYTE길이옵션전송을현재CCh·등록에연결했다. 25h미작성4/5BYTE·금지표역순/0비교·signed버전·실패소속보존/자기22h·탈퇴순서와공유입력을원문/PE/기계어로대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새수신실행검사0이다. **원문18수신중9구현/폐기80제외·나머지8수신과1f/83의실제부모·보조객체/전체초기화·정리/새R.exe최종owner·실제호출은남는다.** [1079 기록](docs/exe/cpp-root-challenge-move-20261107.md).

### 0.96 최근 단위 — 1080 CCh 준비와 전투 시작

1079는 `dcce0d0a / 20261107-144 CP`로커밋됐다.1080은1Fh준비/실제전투시작·현재주소/난수4·40연결flush/현재명단종료·24h전투삭제와전체/지정재배치를현재CCh/장치에연결했다. 폐기PIRATE quota는기존기록전용정책대로비활성유지한다. 첫/최종Release종료0·각경고/오류0·보호2800불변·새단위/MP/게임/통신/새수신실행검사0이다. **원문18수신중10구현/폐기80제외·나머지7수신과83예약종료/Root문구·87/보조객체·전체초기화/정리·새R.exe최종owner·실제호출은남는다.** [1080 기록](docs/exe/cpp-root-battle-launch-20261107.md).

### 0.97 최근 단위 — 1081 CCh 관리와 현재 예약

1080은 `455bf786 / 20261107-145 CP`로커밋됐다.1081은83h관리명령·현재하위socket닫기/실제예약·로그인차단·일반/확장Root안내와같은전역Bind를연결했다. 초지연/분로그·고정122BYTE/후미·원문초기버전0·전처리무변경과명령인자를원문/PE/기계어로대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새수신실행검사0이다. **원문18수신중11구현/폐기80제외·나머지6수신과87/진단·보조객체/전체초기화·정리·새R.exe최종owner·실제Bind/호출은남는다.** [1081 기록](docs/exe/cpp-root-admin-command-20261107.md).

### 0.98 최근 단위 — 1082 R 인사와 LAN 제어

1081은 `47d5d1e9 / 20261107-147 CP`로커밋됐다.1082는0Bh/0Ch인사와66h현재LAN공지·일반번호/실제pending sender guard를같은R표/장치에연결했다. wire10.71/상위음수관문·실제연결동일성·같은공지packet3회/현재포트·IPX주소와실제장치한계를대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새수신실행검사0이다. **원문18수신중14구현/폐기80제외·나머지3수신(11/12/87)과보조객체/전체패킷·초기화/정리·새R.exe최종owner·실제UDP번호발행/호출은남는다.** [1082 기록](docs/exe/cpp-root-control-20261107.md).

### 0.99 최근 단위 — 1083 R 진단과 송신 TCP

1082는 `c43cb95c / 20261107-148 CP`로 커밋됐다.1083은11h/12h 진단 수신과 실제 역방향LAN TCP·현재번호/게임시각 연결 부모를 같은 R 장치에 연결했다. 하위pending/상위조회·미작성응답/현재sender닫힘·실패 뒤 선택번호반환·실제Connect/CaptureGame 순서를 원문/PE·기계어로 대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/소켓열기·새진단검사0이다. **원문18수신 중16구현/폐기80제외·나머지87h와 보조객체/전체패킷·초기화/정리·새R.exe최종owner·실제UDP번호발행/호출은 남는다.** [1083 기록](docs/exe/cpp-root-diagnostic-20261107.md).

### 0.100 최근 단위 — 1084 R 명령 페이지 부품

1083은 `344cf339 / 20261107-150 CP`로 커밋됐다.1084는87h의 실제 템플릿/섹션·공통 치환과 확장 CCh 응답 부품을 연결했다. 매 요청3파일/get30·시작marker 누락 계산·원문 사본 치환/한계·실제OS시각·현재51존·미작성8BYTE 객체를 대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새페이지·패킷 실행검사0이다. **87h 전체·길드/관리자 분기와 실제 등록은 남아 원문18중16구현/폐기80제외 상태다.** 세 .server 원본 콘텐츠가 현재 원본/CD에 없어 최종배포 확보도 남는다. [1084 기록](docs/exe/cpp-root-command-pages-20261107.md).

### 0.101 최근 단위 — 1085 R 길드 변경과 실제 저장

1084는 `2113a51e / 20261107-151 CP`로 커밋됐다.1085는 현재 길드/요청 조회·변경/제거·사용자 홈 저장과 같은 CCh owner를 연결했다. Guild의 모집상태/비밀번호·요청자명/승인 필드 의미를 바로잡고 빈리터럴 포인터 비교·로그인/중복요청·승인·0번누락 제거와 실제 저장 공백/두줄을 대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새길드·파일왕복 실행검사0이다. **87h 전체 부모/목록HTML·관리제재·관찰기/실제등록은 남아 원문18중16구현/폐기80제외 상태다.** [1085 기록](docs/exe/cpp-root-guild-management-20261107.md).

### 0.102 최근 단위 — 1086 R87h 전체 명령 수신

1085는 `46ca7472 / 20261107-152 CP`로 커밋됐다.1086은87h의15상위/6관리자·길드/명단HTML·현재통지/제재/파일·하위소켓과 실제수신을 연결했다. 원문 비연속8555BYTE/274·입력사본/현재조회·실제효과순서를 대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새명령·파일왕복 실행검사0이다. **원문18수신중17몸체 작성/컴파일 완료·폐기80비활성이다. 전체전역패킷·보조객체/초기화/정리·새R.exe 최종owner/실제호출은 남는다.** [1086 기록](docs/exe/cpp-root-command-receiver-20261107.md).

### 0.103 최근 단위 — 1087 R 원격명령 보조 장치

1086은 `43488d47 / 20261107-154 CP`로 커밋됐다.1087은004d23f8의실제원격명령 TCP/4세션·집합/Poll/종료와같은OS/메인hook을연결했다.479bb0은생성자가아닌활성조회이며기본생성7ee90/현재R전역14참조에는Open/handler등록이없다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/소켓·세션실행검사0이다. **다음은전체전역패킷/초기화·정리·새R.exe최종owner/실제호출이다. 기본원문에없는원격포트Open/handler등록은추가하지않는다.** [1087 기록](docs/exe/cpp-root-remote-command-20261107.md).

### 0.104 최근 단위 — 1088 R 전역 패킷 초기화

1087은 `2817e527 / 20261107-156 CP`로 커밋됐다.1088은00482b60/442950의 실제 전역함수/버전 초기화·17수신/102관찰 등록과 같은 동적32몸체/38필드 검사를 연결했다.138길이는R PE/기존표가 같고 NULL9·빈함수61·83버전/관찰기0을 유지한다. 폐기7f/80비활성이다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/소켓실행검사0이다. **다음은477e10시작/478af0정리·새R.exe최종owner/entry에서 실제번호발행/InitializePackets/Bind/Run이다.** [1088 기록](docs/exe/cpp-root-packet-registration-20261107.md).

### 0.105 최근 단위 — 1089 R 시작·정리와 실제 노드/메인 수명

1088은 `27e5a762 / 20261107-159 CP`로 커밋됐다.1089는477e10/478af0의 실제 시작/정리와 같은 노드생성/현재번호공개·메인hook을 연결했다.12전역0/256표2회·pending은시작에서만닫기/카운터보존·현재일반→CCh→보조정리를 유지한다.실제IPv4장치이며 IPX/SPX는명시미지원이다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/소켓실행검사0이다. **다음은새R.exe최종owner/entry에서 같은장기환경/VFS/로그·진단과 실제Lifecycle.Bind/MainLoop.Run·클라이언트OSlaunch/배포다.** [1089 기록](docs/exe/cpp-root-lifecycle-20261107.md).

### 0.106 최근 단위 — 1090 R 실제 로그와 복귀형 진단

1089는 `9971c3cd / 20261107-161 CP`로 커밋됐다.1090은최종R owner에필요한 ProcessTrace의 실제로그/OS시각·사용자홈HTML/호스트스택·복귀형ZAssertion을작성했다. 원문485cf0/85e42는throw옵션을사용하지않으며정상대화상자뒤복귀한다. 해당옵션 직접.text참조2개도읽기인자주소push뿐이다.Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/로그·진단/대화상자실행검사0이다. **다음은같은장기owner/새R.exe entry의 실제LogDevice/AssertionDevice·Lifecycle.Bind/MainLoop.Run·OSlaunch/배포다.** [1090 기록](docs/exe/cpp-root-process-trace-20261107.md).

### 0.107 최근 단위 — 1091 최종 서버 owner와 새 R.exe 진입점

1090은 `ec7dee19 / 20261107-162 CP`로커밋됐다.1091은같은장기서버전역/모든실제장치·Bind/수신·소켓이전·메인/수명과 새R.exe진입점Run호출·CMake대상을연결했다. 실제R PE는AMD64/CUI·11.01/UTF-8/asInvoker이며19핵심심볼/인라인256정리·활성84프로젝트를정적대조했다. Release종료0·경고/오류0·보호2800불변·새단위/MP/게임/통신/새R·원본R실행0이다. **다음은클라이언트NativeLocalServerLaunch에서이새R.exe의실제OSlaunch/배포다.** [1091 기록](docs/exe/cpp-root-server-entry-20261107.md).

### 0.108 최근 단위 — 1092 새 R.exe 실제 실행 장치와 배포

1091은 `dc59e44d / 20261107-164 CP`로커밋됐다.1092는클라이언트37의실제새R CreateProcessW lan/wait0/CWD·콘솔/핸들장치와배포동봉을연결했다. Native부모여섯순서/34·38을보존하고클론모듈기준/새11.01 R파일식별을사용한다. 두Release종료0·경고/오류0·PowerShell5.1 계획/한ZIP검증종료0·1384항목/두EXE정확바이트·보호2800불변·새C++/MP/게임/통신/새R·원본R실행0이다. **다음은제공R3BYTE존/클라이언트0F47BYTE차이·0F/1d수신/메타실제연결이다.** [1092 기록](docs/exe/cpp-local-server-process-package-20261107.md).

### 0.109 최근 단위 — 1093 Root0Fh 수신과 목록 저장

1092는 `d677b74e / 20261107-172 CP`로커밋됐다.1093은0Fh 실제수신/관찰기·같은Client 메타기록/State·목록 갱신·사용자Saves 저장/다음시작 우선읽기를연결했다. 원문47BYTE/친구3BYTE는보존하고제공R10.71/정확한3BYTE길이에만명시compact호환을적용하며없는이름/플래그는만들지않는다. 최종두Release종료0·경고/오류0·보호2800불변·새C++/MP/게임/통신실행0이다. **다음은1dh수신·State28..33/CS·실제메타표시다.** [1093 기록](docs/exe/cpp-root-login-reply-20261107.md).

### 0.110 최근 단위 — 1094 Challenge 응답/메타 상태 본체

1093은 `d97301dc / 20261107-178 CP`로커밋됐다.1094는1dh/State28~33·CS master/캐시감시본체를라이브러리컴파일하고Root요청실제목적지인자/별도005c85c4 owner·직접read/write를복원했다. **새1dh등록/28~33분배는아직0이며필수실제장치완성뒤접합한다.** 두Release종료0·경고/오류0·보호2800불변·새C++/MP/게임/통신실행0이다. 다음은0047dd60/479760/47cab0·CS/실제장치→1dh/28~33등록이다. [1094 기록](docs/exe/cpp-meta-connected-bodies-20261107.md).

### 0.111 사용자 요청 일시 중지 인계 — 1094 마감

- 현재1094/11.01의1dh/28~33 본체는라이브러리컴파일이며실제새수신등록/상태분배는0이다. 최종Root요청목적지인자/별도005c85c4 owner·직접read/write와현재005318e8 const참조를복원했다. 마지막제품은0.1절의두EXE 해시다.
- **재개순서:** 0047dd60/479760/47cab0메타표시/Poll·현재CS수신/가상표·같은오브젝트/카메라/HTMLserial·004b2c00/master통지·004b62b0/Login/동일전송장치완성→MetaConnectionRuntime의1dh등록·기존분배기28~33접합. 필수장치를빈callback/가짜ACK/가짜화면으로대체하지않는다.
- 다음은세.server콘텐츠/배포·LAN hostname/Config registryValue경계·LAN/Outpost/checking5→한국어후속→Edit시나리오UI다.986 후보는미적용이며기존인계파일을유지했다.
- 최종두Release종료0·경고/오류0·보호2800불변·새fixture0·새C++/MP/게임/통신/새R·원본R실행0·전체34/ZIP반복0이다. 마지막C++실행검사는1052 기존7개,마지막전체34진입은645의644제품,최신ZIP은1092이며현재1094와구별한다. 사용자지정포획/희생/공격/승리·사용자확인·MP검사는계속제외한다.
- Git공유주의: 검토중staging된1094중간13파일이병행 `d5c033e1 / 20261107-181 DN`에함께포함됐다. 그커밋은재작성하지않았고현재포트참조교정/최종제품해시·완료/중지인계를별도CP커밋으로마감한다. 다음재개시에도타세션변경/스테이징을포함하지않고자기파일만검증뒤즉시stage/commit한다. dotnetpj 변경은수정/커밋하지않는다.
- 마지막전체정밀디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon이다. 다른호스트재개는AGENTS의재디컴파일규칙을따른다. 원시/빌드/정적감사는Git제외 `extracted/cpp-resume-1094/`,현행보고서는 `cpppj/meta-connected-bodies-20261107.json`이다.


### 0.112 최근 단위 — 1095 메타 master 통지·게임 서버 로그인

새 `/goal`로1094의 일시 중지를 해제했다. 1095는004b2c00/004b62b0과두생성자·빈관찰기를대조하여같은실제CS/TCP에1fh12BYTE→11/7ch28BYTE→9 송신메서드와등록표를추가했다. master flags2000h 선행저장·음수master진단뒤현재CS 재조회·동일자기+a4/+a8 필드순서를보존한다. **State33 실제호출/1dh등록/28~33분배는미적용**이며표시/CS·실제장치완성뒤접합한다. 두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다. [1095 기록](docs/exe/cpp-meta-peer-requests-20261107.md). 다음은0047dd60/479760/47cab0·CS수신/실제장치→1dh/28~33등록이며나머지우선순위와사용자검사제외를유지한다.


### 0.113 최근 단위 — 1096 메타 원근 투영·클리핑과 Client 수명

1095는 `4619ffaa / 20261107-184 CP`로커밋됐다.1096은AngleCamera 생성/높이·배경분모·별도viewport/화면중심·원근/ordered평면클리핑·첫보정/60저장·객체raw+8 WORD/+1a DWORD와후행UI범위판정을같은Client장기owner에복원·컴파일했다. 두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다. [1096 기록](docs/exe/cpp-meta-projection-20261108.md). **전체0047dd60/479760/47cab0·CS수신/실제객체·가상표/HTML장치와1dh/28~33등록은미적용**이며완성뒤1095송신/1096투영을접합한다. x87초월함수/전체비트·픽셀대조는없고나머지우선순위/사용자검사제외를유지한다.


### 0.114 최근 단위 — 1097 메타 참가자·전투섬 갱신과 이름표

1096은 `67bbd8f3 / 20261108-02 CP`로커밋됐다.1097은0047d8c0 참가자전체/479c40 전투섬/479560의51존정리·4248a0 문맥/42b050 관문과같은1096투영을라이브러리컴파일했다. 같은CS/실제raw/Gump문맥·mask1/4/6·8자리배치/기존float좌표·visible/attach 순서를보존한다. 두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다. [1097 기록](docs/exe/cpp-meta-display-updates-20261108.md). **실제Pool/가상객체/Gump하위장치·전체0047dd60/479760/47cab0·CS수신·1dh/28~33등록은미적용**이며다음단위에서이어간다. 필수장치를빈callback/가짜노드로대체하지않고나머지우선순위와사용자검사제외를유지한다.


### 0.115 최근 단위 — 1098 전체 메타 Poll·참가자 목록 갱신

1097은 `c065c9e9 / 20261108-03 CP`로커밋됐다.1098은00479760 전체/47cab0·종속finder·같은Client 목록슬롯을컴파일했다.39참가자/9전투/50존·별도owner카운터/방금존clip·next선저장/전체DWORD제외TYPE·현재목록재조회를보존한다. 두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다. [1098 기록](docs/exe/cpp-meta-display-poll-20261108.md). **실제Pool/가상객체/Gump·CS수신/0047dd60 전체장치와1dh/28~33등록은미적용**이다. 다음은0047dd60/CS실제장치→1095~1098본체와1dh/28~33접합이다. 다른우선순위/사용자검사제외를유지하며새MP실행은하지않는다.


### 0.116 최근 단위 — 1099 실제 메타 Gump·Push/NSInfo 공용 슬롯

1098은 `39c89889 / 20261108-04 CP`로커밋됐다.1099는기존공통Gump의Push/다중줄·문자/스크롤/가상Attach/등록ID장치와같은메타글자공장·수명을연결했다.00593150은기존ClientNsInfoGlobals.current이며1098의별도CS목록슬롯을제거했다.가상+18은Redraw가아닌00464560 Push로정정하고실제metaThird에연결했다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1099 기록](docs/exe/cpp-meta-gump-devices-20261108.md).전체0047dd60·CS수신/Pool·가상객체/Renderer·HTML/NSInfo메타배치·전용정리의상태이탈부모와1dh/28~33등록은남는다.실제장치완성뒤접합하며기존우선순위/사용자검사제외를유지한다.


### 0.117 최근 단위 — 1100 전체 메타 존 장면·실제 글자/SHP 하위

1099는 `030bae94 / 20261108-06 CP`로커밋됐다.1100은0047dd60 전체·FPU asin/PE상수·두링/세이름표·카메라예약과실제TextGump/SHP/frameCheck·serial/Reference를작성·컴파일했다.글자문맥slot1글꼴/slot2색을구별해기존색전역이름을정정했고geometryMode/존그림TYPE를같은Client수명에더했다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1100 기록](docs/exe/cpp-meta-scene-20261108.md).다음은실제World/Pool·CS수신/Renderer·HTML/NSInfo메타배치·상태이탈수명→1095~1100실제접합/1dh·28~33등록이다.전체메타화면성공으로쓰지않으며나머지우선순위/사용자검사제외를유지한다.


### 0.118 최근 단위 — 1101 실제 메타 Session·Form/이탈 수명

1100은 `ecb1c779 / 20261108-10 CP`로커밋됐다.1101은State19의기존Fort/Core캐시를보존하는MetaMap Session과같은Pool/Player/Kernel·공통Form/투영/삭제를연결했다.004795a0전체·0046d8a0두free/현재CS재조회·가변배열owner와Native증빙뒤실제메타수명반납도접합했다.게임Load/Login/loaded공개는하지않는다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1101 기록](docs/exe/cpp-meta-world-lifetime-20261108.md).다음은같은Session의CS수신/생성·전체Renderer/카메라·HTML/NSInfo메타배치→1095~1100접합/1dh·28~33등록이다.State19/HUD/전체메타화면성공으로쓰지않으며나머지우선순위/사용자검사제외를유지한다.


### 0.119 최근 단위 — 1102 실제 메타 World·Poll/Renderer 접합

1101은 `6317535a / 20261108-16 CP`로커밋됐다.1102는같은메타Session의실제QA WORD/Owner·공장/Pool·Renderer/카메라Reset·공통SpriteDraw/팔레트와479760/47dd60 Poll을접합했다.004ac220은QA WORD,004ad6c0은TYPE+114프레임개수로정정했고현재Root/Capacity·Process상한/팔레트참조도연결했다.최종두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1102 기록](docs/exe/cpp-meta-renderer-integration-20261108.md).다음은CS수신/참가자·전투섬생성·카메라Rig·HTML/NSInfo메타배치→1dh·28~33등록/1095송신상태호출이다.전체메타화면성공으로쓰지않으며나머지우선순위/사용자검사제외를유지한다.


### 0.120 최근 단위 — 1103 NSInfo 메타 루트/참가자 목록

1102는`2e5e1c67 / 20261108-20 CP`로 커밋됐다.1103은0047e630 전체 정렬/행과0047e9d0 앞단/관문 뒤 전체 메타 루트를 같은 실제 CS/공통 UI에 연결했다.정렬 행 metadata·같은 Pool 소유자/TYPE 기술합계·현재 팔레트·master/nonmaster ftol과 FST32 저장 시점을 대조했다.첫/최종 두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1103 기록](docs/exe/cpp-nsinfo-meta-root-20261108.md). **PlayerInfo/BattleInfo 전체 배치·메타 입력/버튼 Draw는 후속**이며 실제 자식 호출을 우회하지 않으므로 전체 NSInfo/State19 화면 성공은 아니다.다음은 이 자식/입력→CS수신/생성·카메라Rig/HTML→1dh/28~33등록/1095송신 상태호출이다.나머지 우선순위/사용자 검사 제외를 유지한다.


### 0.121 최근 단위 — 1104 PlayerInfo 전체 메타 배치

1103은`78ad430e / 20261108-24 CP`로 커밋됐다.1104는00479fb0 전체 메타 본체·마스터/준비/시작/빈 슬롯·태그/문맥/아이콘/이름·최종 높이를 같은 실제 CS/공통 UI/Button에 접합했다.옵션 포인터와 캡처 여백은 전체 기계어로 보완했고 기존 ChatPrefixIcons/현재 TYPE·팔레트를 사용한다.첫/후속/최종 두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1104 기록](docs/exe/cpp-nsinfo-player-layout-20261108.md).BattleInfo 전체 배치·메타 입력/버튼 Draw는 후속으로 전체 화면 성공이 아니다.나머지 우선순위와 사용자 검사 제외를 유지한다.


### 0.122 최근 단위 — 1105 BattleInfo 전체 메타 배치/옵션 문자열

1104는`a321566f / 20261108-26 CP`로 커밋됐다.1105는0047b480 전체 메타 본체·47b1c0 설정 준비·47b430 범주 그림·41c3c0/3d0/440 표시 getter를 같은 실제 CS/공통 UI/Config에 접합했다.설정82대입/분기와 PE22옵션/76선택/번역 플래그를 정적 대조했다.두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1105 기록](docs/exe/cpp-nsinfo-battle-layout-20261108.md).메타 입력/특수 버튼 Draw와 CS/카메라/HTML·상태 접합은 후속이며 전체 화면 성공이 아니다.나머지 우선순위와 사용자 검사 제외를 유지한다.


### 0.123 최근 단위 — 1106 PlayerInfo 특수 버튼 전체 Draw

1105는`88b1cd81 / 20261108-29 CP`로 커밋됐다.1106은004252b0 전체 Draw·활성 캡처/배경/안쪽 단색/현재 TYPE/L+6/7·signed 중앙 나눗셈을 같은 실제 공통 클립/픽셀/SHP 분배에 접합했다.null SHP 반환도 현재 loaded로 보존한다.첫/최종 두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1106 기록](docs/exe/cpp-nsinfo-player-button-draw-20261108.md).메타 입력과 CS/카메라/HTML·상태 접합은 후속이며 전체 화면 성공이 아니다.나머지 우선순위와 사용자 검사 제외를 유지한다.


### 0.124 최근 단위 — 1107 슬롯 변경/공통 랭크

1106은`41b2b735 / 20261108-30 CP`로 커밋됐다.1107은색/랭크/특수 슬롯5몸체와 공통24DWORD 랭크 getter를 복원해 현재 PlayerInfo 표시 getter를 공유했다.원문 DL/EAX 폭·관문/상한/부호/두 저장·setne를 정적 대조했다.두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1107 기록](docs/exe/cpp-nsinfo-slot-options-20261108.md).writer UI 호출/세 Handle·메타 메뉴/송신은 후속이며 전체 입력/화면 성공이 아니다.나머지 우선순위와 사용자 검사 제외를 유지한다.


### 0.125 최근 단위 — 1108 실제 메타 상태/자료 송신

1107은`cd5dc8f8 / 20261108-32 CP`로 커밋됐다.1108은1fh 상태 writer와2bfb0 자료 송신/40ae80 관찰기를 같은 TCP11에 접합하고 기존 NotifyMetaMaster의헤더/CS쓰기 순서를 유지했다.PE descriptor·전체 정상 복사/길이 확정을 정적 대조했다.두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1108 기록](docs/exe/cpp-nsinfo-meta-senders-20261108.md).NSInfo 세 Handle/메타 메뉴·카메라/새 UI 송신 호출과 CS/카메라Rig/HTML·상태 접합은 후속이다.나머지 우선순위와 사용자 검사 제외를 유지한다.


### 0.126 최근 단위 — 1109 실제 메타 PlayerFinder 연결

1108은`6683d4fa / 20261108-34 CP`로 커밋됐다.1109는 PlayerFinder의 현재CS40기록 getter를 실제 메타 분기/메타메뉴 조회 때만 대여하고 Chat에 연결했다.기존고정span/자연관문·부분완료/Chat후속은 유지한다.원문/배선을 정적 대조했다.두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1109 기록](docs/exe/cpp-meta-player-identity-20261108.md).NSInfo 세 Handle/메타 메뉴·카메라/1107슬롯·1108송신 UI 호출과 CS/카메라Rig/HTML·상태 접합은 후속이다.나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.127 최근 단위 — 1110 참가자 메뉴/신뢰도

1110은004785f0 참가자 메뉴 전체/00478310 신뢰도를 복원하고 기존Player메뉴의신뢰도소비자를 실제CS에 연결했다.1109는`9f7c0604 / 20261108-37 CP`로 커밋됐다.두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1110 기록](docs/exe/cpp-meta-player-menu-20261108.md).World없는 메타메뉴공장·NSInfo 세Handle/카메라·1107슬롯/1108송신 UI 호출은후속이다.전체메타화면성공이아니며나머지우선순위/사용자검사제외를유지한다.

### 0.128 최근 단위 — 1111 실제 메타 Player메뉴 공장

1111은World없는전체메타 Player메뉴공장을실제 Client/CS/Chat·Session·controlled/객체Form에접합했다.공유서식본문은정적동일이고주소/실제신뢰도/수명도연결했다.1110은`a487fc19 / 20261108-40 CP`로커밋됐다.첫/최종두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1111 기록](docs/exe/cpp-meta-player-factory-20261108.md).NSInfo 세Handle/카메라·1107슬롯/1108송신UI호출은후속이다.전체메타화면성공이아니며나머지우선순위/사용자검사제외를유지한다.

### 0.129 최근 단위 — 1112 NSInfo 루트 전체 입력

1111은`6f25a98e / 20261108-44 CP`로커밋됐다.1112는0047c8c0 전체 루트입력과실제현재정렬표/CS·1111메뉴/원시좌표·같은Session카메라·1fh헤더/우선순위1추방음향/TCP11·원래BaseHelp를연결했다.별도mode/known관문을추가하지않고추방은직접참가자번호를쓴다.첫const호출오류1건/후속 경고1건교정뒤두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1112 기록](docs/exe/cpp-nsinfo-root-input-20261108.md).두자식입력/1107슬롯·1108자료송신 UI호출과나머지메타연결은후속이다.전체화면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.130 최근 단위 — 1113 PlayerInfo 전체 입력

1112는`4d408c76 / 20261108-47 CP`로커밋됐다.1113은0047a900 전체 PlayerInfo입력과현재버튼serial/value·CS·준비/시작·다섯슬롯writer/자기594fc4·실제1fh/87BYTE67h·추방확인메뉴를연결했다.원문헤더전후/실제쓰기·부분효과·태그소비1/기타0을보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1113 기록](docs/exe/cpp-nsinfo-player-input-20261108.md).BattleInfo전체입력/옵션·제외writer와나머지메타연결은후속이다.전체화면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.131 최근 단위 — 1114 BattleOptions 전체 변경/제외 목록

1113은`1c565aec / 20261108-49 CP`로커밋됐다.1114는BattleOptions 일곱전체getter/writer·현재포함최댓값/실제BYTE·제외목록이동과음향전후를제품에복원·컴파일했다.최초/현재count·전체DWORD비교/BYTE저장·끝잔존BYTE를보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1114 기록](docs/exe/cpp-battle-option-mutation-20261108.md).BattleInfo전체입력/실제메뉴의1114호출과나머지메타연결은후속이다.전체화면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.132 최근 단위 — 1115 BattleInfo 입력 분기 부모

1114는`77bc93f6 / 20261108-52 CP`로커밋됐다.1115는BattleInfo입력분기부모와0047b180 전체기본값적용을제품에복원·컴파일했다.권한/signed태그·독립명령·실제writer/송신·현재위치재부착/반환값을보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1115 기록](docs/exe/cpp-nsinfo-battle-input-parent-20261108.md).다섯메뉴자식/실제UI·부모분배와나머지메타연결은후속이다.실제입력/전체화면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.133 최근 단위 — 1116 BattleInfo 다섯 메뉴 자식

1115는`f18a7e1c / 20261108-53 CP`로커밋됐다.1116은BattleInfo다섯메뉴자식과선택문자열정체성비교/현재TYPE·전역제외/12별칭·여섯CRT정렬/다섯출력을복원·제품에컴파일했다.실제getter/메뉴장치와1115입력분배는후속이다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1116 기록](docs/exe/cpp-nsinfo-battle-menus-20261108.md).실제입력/전체화면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.134 최근 단위 — 1117 선택 문자열/Xlat 정체성

1116은`78de23a7 / 20261108-54 CP`로커밋됐다.1117은지정선택getter/보호PE76포인터·현재Client실제Xlat값정체성을제품에복원·컴파일했다.임시변환버퍼주소를배제하고원문폴백주소를보존한다.기존두Translate본문은정적동일이다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1117 기록](docs/exe/cpp-battle-choice-identity-20261108.md).실제메뉴/현재별칭·492960·1115/1116분배는후속이다.실제입력/전체화면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.135 최근 단위 — 1118 실제 BattleInfo와 세 입력 분배

1117은`cd63f421 / 20261108-56 CP`로커밋됐다.1118은BattleInfo입력/다섯메뉴와실제현재TYPE별칭/Deck·전역옵션/Xlat정체성·67h/가상재부착을연결하여NSInfo세Handle을공통분배에접합했다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1118 기록](docs/exe/cpp-nsinfo-battle-integration-20261108.md).CS수신/생성·카메라Rig/HTML→1dh/28~33등록/1095송신상태호출은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.136 최근 단위 — 1119 CS 조회와 버전 범위

1118은`080c977d / 20261108-59 CP`로커밋됐다.1119는CS명단첫일치/실제0번·이름뷰/진단·빈8슬롯/전투버전WORD집계를같은실제owner에복원·컴파일했다.초기max-1/min256·active필터없음·최종min/max순서를보존한다.최초/최종두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1119 기록](docs/exe/cpp-challenge-record-queries-20261108.md).수신/생성부모호출과나머지메타연결은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.137 최근 단위 — 1120 실제67h 전투옵션 수신

1119는`4d19ab9a / 20261108-62 CP`로커밋됐다.1120은0042c920 전체67h옵션수신을실제공유등록표/현재CS·guy.wav/NSInfo에접합했다.160포함상한/자기master관문·음향후현재길이복사/현재BYTE저장/조건부UI를보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1120 기록](docs/exe/cpp-meta-battle-options-receive-20261108.md).나머지CS수신/생성·Rig/HTML·상태접합은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.138 최근 단위 — 1121 메타 전투 생성/23h 부모

1120은`d20f510c / 20261108-64 CP`로커밋됐다.1121은메타전투생성/23h부모와호출부식별자구조체/값전달을복원·제품에컴파일했다.ret18h는스택DWORD6칸이며식별자구조체가2칸을쓰는논리인자5개다.마지막기존표시가Rig관문이다.이전1120설명도정정했다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1121 기록](docs/exe/cpp-meta-battle-create-parent-20261108.md).실제물리/Rig장치/23h등록과나머지메타연결은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

### 0.139 최근 단위 — 1122 실제 물리 바인더와 배율 정정

1121은 `f40e1625 / 20261108-66 CP`로 커밋됐다. 1122는004adca0을 배율 DWORD 쓰기로 정정하고 현재 Client/Session의 실제 팩터리·Pool·QA·Pop 바인더를 추가·컴파일했다. 바인더 소비자/Rig/23h 등록은 후속이다. 두 Release 종료0·경고/오류0·보호2800 불변·새fixture/C++/MP/게임/통신 실행0이다. [1122 기록](docs/exe/cpp-meta-battle-object-binding-20261108.md). 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.140 최근 단위 — 1123 Spring3d 공통 계산

1122는 `04947470 / 20261108-68 CP`로 커밋됐다. 1123은 Spring3d 생성/목표/시간 누적 계산을 같은 raw 필드 뷰로 복원·컴파일했다. 원본 double 1/30초·완료 비트·패딩 보존·F32 저장을 대조했고 음수 높이 속도의 도착 비교를 정정 후 재빌드했다. 첫/최종 두 Release 종료0·경고/오류0·보호2800 불변·새fixture/C++/MP/게임/통신 실행0이다. [1123 기록](docs/exe/cpp-meta-spring3d-20261108.md). 실제 Rig/카메라 프로세스·23h 등록은 후속이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.141 최근 단위 — 1124 실제 IslandRig와 CS Image 복사

1123는 `4664c37b / 20261108-70 CP`로 커밋됐다. 1124은 CS Image의 깊은 복사·현재 Animator TYPE와 실제 Kernel/ProcessForm IslandRig의 생성/부착/프레임/반납 본체를 컴파일했다. 현재 전역 getter를 사용하며 실제 Client 장치 바인더·소비자/23h 등록은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1124 기록](docs/exe/cpp-meta-island-rig-20261108.md). 후속은 실제 Client Rig 장치/생성 소비자·23h 등록·카메라 Rig·메타 객체 그림/나머지 CS 수신/참가자 생성/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.142 최근 단위 — 1125 실제 Client Rig와23h/24h 전투 수신

1124는 `4e666780 / 20261108-73 CP`로 커밋됐다. 1125은 실제 Client/현재 Session·Kernel·Gump·시각·CS Image owner를 IslandRig에 연결하고 원래 State wrapper를 포함한23h 추가/24h 삭제를 같은 TCP/UDP 표에 등록·컴파일했다. 전체 메타 장면 실행 검증은 아니다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1125 기록](docs/exe/cpp-meta-battle-receivers-20261108.md). 후속은 67h 전체 State 부모/일반 수신의 CS hook·카메라 Rig·메타 객체 그림/나머지 CS 수신/참가자 생성/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.143 최근 단위 — 1126 실제 카메라 Rig와 공유 메타 갱신

1125는 `739fc118 / 20261108-76 CP`로 커밋됐다. 1126은 카메라 Rig 생성/재사용/프레임/반납·intro wrapper와 실제 Client/CS/Kernel/Host/Renderer 바인더를 컴파일했다. 카메라 개수 DWORD와 기존 메타 갱신 예약을 공유하며 참가자 생성 caller 연결은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1126 기록](docs/exe/cpp-meta-camera-rig-20261108.md). 후속은 67h 전체 State 부모/일반 CS hook·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.144 최근 단위 — 1127 67h 공통 State 부모와 일반 CS 연결

1126는 `aebe0f12 / 20261108-79 CP`로 커밋됐다. 1127은 67h 전체 State 부모를 공통화해 실제 일반 수신에 접합하고 누락된 일반 Session의 CS 자식을 현재 Client CS/음향/NSInfo에 연결·컴파일했다. 메타 등록표의 전체 부모와 공유 파생 전역·HUD 재생성은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1127 기록](docs/exe/cpp-state-battle-options-20261108.md). 후속은 메타67h 전체 부모 등록/공유 반경·파생 전역/004c7880 HUD 재생성·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.145 최근 단위 — 1128 삭제 선행 전체 HUD 재생성과67h 연결

1127는 `3f942573 / 20261108-80 CP`로 커밋됐다. 1128은 004c7880의 기존 HUD Kill/Cleanup 선행·전체 새 자식 생성/임시등록 반납·루트 공개를 실제 공통 Gump 장치로 복원하고 일반67h의 선택 UI 훅에 연결·컴파일했다. 메타67h 전체 부모/공유 파생 전역은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1128 기록](docs/exe/cpp-hud-recreate-20261108.md). 후속은 메타67h 전체 부모 등록/공유 반경·파생 전역·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.146 최근 단위 — 1129 공유 공급 반경과 메타67h 전체 옵션 부모 연결

1128는 `52cfee23 / 20261108-82 CP`로 커밋됐다. 1129은 공급 반경/4float 선택표를 실제 Client 전역에서 공유하고 메타67h도 전체 State 부모를 통해 현재 옵션 복사·CS 통지·범위/TYPE 비용/환급·선택 HUD 재생성을 수행하도록 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1129 기록](docs/exe/cpp-meta-battle-options-integration-20261108.md). 후속은 메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.147 최근 단위 — 1130 메타 섬 전체 추가Draw와 실제 화면 연결

1129는 `76e7e04a / 20261108-84 CP`로 커밋됐다. 1130은 00478d10 전체 추가Draw의 참가자 Image/죽는 Rig Image·전투 TYPE/버전/설명·8자리 표시를 실제 CS·Pool/ProcessForm·동적 할당·SHP·현재 메뉴 글꼴과 같은 통합 Renderer 화면에 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1130 기록](docs/exe/cpp-meta-sprite-draw-20261108.md). 후속은 메타 파생 표시 갱신/메타 객체 입력/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.148 최근 단위 — 1131 메타 파생 표시 갱신과 실제 World·Gump 연결

1130는 `be336598 / 20261108-88 CP`로 커밋됐다. 1131은 004787c0 전체의 기본 영역/큰 배율 보정·실제 dirty·현재 Form 이름표 갱신을 복원하고 보호 PE의88/8c 분배와 실제 메타 World 생성 직후 Client/Renderer/Gump 장치에 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1131 기록](docs/exe/cpp-meta-sprite-refresh-20261108.md). 후속은 메타 객체 입력/일반 선택 캡처 접합/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.149 최근 단위 — 1132 일반·메타71h 전체 설명 수신과 실제 표시 연결

1131는 `739b04e5 / 20261108-94 CP`로 커밋됐다. 1132은 71h의 전체 observer/CS 비영 State 부모·전투 설명 복사/정제 자식을 일반·메타 등록표에 공통 연결하고 실제 객체 가상88·특정 전투 표시를 원문 순서로 수행하도록 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1132 기록](docs/exe/cpp-meta-battle-description-20261108.md). 후속은 메타 객체 입력/일반 선택 캡처 접합/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.150 최근 단위 — 1133 메타 선택 캡처의 실제 Image·글꼴 문맥 연결

1132는 `3abed63a / 20261108-97 CP`로 커밋됐다. 1133은 498000의 같은200×200 선택 버퍼에서 메타+a4가 실제 CS/Image·TYPE/SHP·기존 글꼴/색을 쓰도록 연결했다. 임시 글자 문맥은 정상/예외 모두 복원하며 메타 마우스 입력/선택 caller는 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1133 기록](docs/exe/cpp-meta-selection-pixels-20261108.md). 후속은 메타 객체 입력/메타 선택 caller/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.151 최근 단위 — 1134 메타 객체 전체 입력과 실제 메뉴·조회·큐 연결

1133는 `a0342fae / 20261108-100 CP`로 커밋됐다. 1134은 4789b0 전체의 배율 관문·참가자/전투 메뉴·3×3 좌석/이동 조회·입력 예약을 현재 실제 controlled·CS·TYPE/SHP·카메라·설정·입력 큐에 연결했다. 기존 객체 메뉴의 메타+ac도 같은 본체를 호출한다. 메타 월드 마우스 선택 caller와 f3/6b 상태 소비자는 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1134 기록](docs/exe/cpp-meta-object-input-20261108.md). 후속은 메타 월드 객체 입력/선택 caller·f3/6b 소비자/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.


### 0.152 사용자 요청 일시 중지 인계 — 1134 마감

- 사용자2026-11-08 후속 요청에 따라 현재 단위1134의 구현·빌드 확인·문서 반영·로컬 커밋까지만 수행한 뒤 목표를 일시 중지한다. 전체 목표 완료가 아니다. 현재 제품·두 EXE 해시는0.1절과 [1134 기록](docs/exe/cpp-meta-object-input-20261108.md)·`cpppj/meta-object-input-20261108.json`이 기준이다.
- **다음 구현:** 메타 월드의 실제 마우스 선택/picking 호출부와f3/6b 상태 소비자→나머지 CS 수신(참가자 생성/삭제/변경 포함)·현재 이미지/이름 Gump·카메라 caller→HTML→1dh·28~33 실제 등록/분배·1095 송신 상태 호출이다.4789b0 전체와 실제 메뉴/CS/SHP/입력 큐 연결은1134에서 컴파일됐지만 전체 메타 입력/항해 실행 완료가 아니다.
- 다음은 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다.986 한국어/시나리오 준비본은 미적용이며 옛 후보를 현재 제품에 덮어쓰지 않는다. 상세 우선순위와 미완료 범위는3절8번이다.
- 두 Release 종료0·경고/오류0·보호2800파일 불변이다. 새 C++/MP/게임/통신/전체 검사/34미션/ZIP 실행0이며 마지막 C++ 실행1052 기존7개·전체34진입645의644제품·ZIP1092는 이번 제품과 구별한다. 사용자 확인·포획/희생/특정 적 공격/승리·MP 검사는 계속 제외한다.
- 마지막 전체 정밀 디컴파일2026-11-03/HJOW-Athlon·R전용2026-11-07/HJOW-Athlon이다. 이번 생성 원문 변경0이며 재개 호스트가 다르면 AGENTS의 재디컴파일 규칙을 따른다. 원시/빌드/정적 감사는 Git 제외 `extracted/cpp-resume-1134/`이며 다른 PC에 있다고 가정하지 않는다.
- 재개 시 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md·Git 상태를 다시 읽는다. 병행 DN/WB의 변경/스테이징을 포함하지 않고 자기 파일만 로컬 커밋하며 push하지 않는다.1134의 실제 커밋 식별자는 Git 로그에서 `CP 메타 객체 전체 입력과 실제 메뉴 조회 큐 연결` 메시지로 확인한다.

### 0.153 최근 단위 — 1135 메타 설명 변경 모달과 실제 권한·전송 연결

1134는 `be0ad21c / 20261108-103 CP`로 커밋됐다. 메타 설명 변경 f3를 실제 CS 문자열·기존 문자/클립보드·공통 모달과 71h/TCP11 확정에 연결했다. 권한 재조회·40BYTE/NUL·원문 후행 공백과 거부 안내를 보존했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1135 기록](docs/exe/cpp-meta-description-input-20261108.md). 사용자 /goal 재개로 진행 중이며 다음은 메타 월드 객체 입력/선택 caller·6b/6c 항해 소비자·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.154 최근 단위 — 1136 메타 좌석 참가·항해 입력과 실제 전송/Rig 연결

1135는 `a949d135 / 20261108-104 CP`로 커밋됐다. 메타6b/6c·직접/경고억제 확인 e3/fe의 좌석 계산·실제1eh/TCP11·현재 참가자 좌표/Rig와 경고 Tell·설정을 연결했다. 실제 메뉴→큐의 f3/항해 허용 관문과 원문 항해 이름4개도 보완했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1136 기록](docs/exe/cpp-meta-sail-input-20261108.md). 다음은 메타 월드 마우스 객체 입력/선택 caller·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. BattleDesc HTML 문자열 고리/CS 주소 구별도 후속이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.155 최근 단위 — 1137 메타 마우스 선택·영역과 항해 메뉴 호출 연결

1136은 `44626991 / 20261108-105 CP`로 커밋됐다. 같은 메타 Session/Pool의 픽셀 선택·좌우 클릭·모드6 위치 항해·빈 공간 항해 메뉴·영역 glyph 메뉴/e8 확인을 실제 입력 큐와 공통 Gump에 연결했다. 이전 메인 메뉴 버튼 재생성을 막는 호스트 지도 화면 전환도 보완했다. 최종 NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새 C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이며1136제품의 읽기 전용 TYPE자료 조회1회는 별도 기록한다. 최초 컴파일10진단·후속API4진단과 교정 로그도 보존했다. [1137 기록](docs/exe/cpp-meta-pointer-input-20261108.md). 다음은 메타 ESC/공통 키보드·메뉴 caller·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. BattleDesc HTML 주소 구별·helpWord 캐시 수명도 후속이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

### 0.156 최근 단위 — 1138 메타 ESC·공통 키와 실제 게임 메뉴 호출 연결

1137은 `bcf8a663 / 20261108-106 CP`로 커밋됐다. 메타 ESC의 실제 선택 정리·커서 취소/메뉴 serial 토글과 공통 Game/View/Options 메뉴 작성기를 같은 Session/SP/현재 HUD에 연결했다. H/N/P/R/F4/F5/F6·생산 키·F9 예약/실제 카메라 경로와 일반 F6의 조기 소비 누락도 보완했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/자료조회/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1138 기록](docs/exe/cpp-meta-common-input-20261108.md). 다음은 메타 Refresh Zones(c6)·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. BattleDesc HTML 주소 구별·helpWord 캐시 수명도 후속이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **활성 비멀티 구현·대조 범위 마감.** UI929·Edit943·미션 수명956·내부 수명/FILE980. 34경로의 마지막 전체 진입 확인은645/제품644다. 자연 완주·전체 동일성은 사용자 확인이며, 새로운 제보는 AI 수정 항목으로 연다. 8차 신규 시나리오 UI와 구별한다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **구현·현재 확인 범위를 정리하고 후속 단계로 전환.** 호환 전체화면·화면비·가장자리 스크롤·저장 재시작을 확인했고 와이드 덱 결함은981에서 수정했다. 물리 입력/출력·다른 장치/DPI·Windows 11/ARM 실기는 미검증이며 사용자/환경 확인이다. 무오류 보증은 아니다. 10.72 호환성은 후순위다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | **사전 구현 진행 중.** 982~1015 TCP/실제 State·LAN UI/명단·일반 Login/저장·재접속·자동 장애,1016~1018 일반 자원 프레임,1019~1040 Tell/비용·객체/압축/검증·Form/공유 예약/총·아비어리·비행·애니메이션 수신과 애니메이션 송신/TYPE·건설37h/PlayerKnown·Path30h·공간33h/34h/35h·현재32h 설정·개발자31h33명령분기/화면 강조·건설기록writer를 연결·컴파일했다. 나머지 수신/전송·재동기화/Activate·제재/메타/Outpust는 미완료다. 신규MP/통신 검사0이며 LAN 플레이 검증 완료가 아니다. |
| 4차 | **로컬MCP**로 dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화. **16:9 최대 1920×1080 지원**이며 이 단계에서 그래픽 크기 스케일 조정은 불필요하다. | **구현 완료(935~937).** 로컬 MCP 서버(`Netstorm.exe --mcp`: 조작/상태/보고서/화면 PNG/프레임 대기/소리 포함 녹화/종료)와 16:9 최대 1920×1080을 구현하고 실제 실행으로 확인했다([MCP](docs/exe/cpp-mcp-server-20261106.md), [해상도](docs/exe/cpp-wide-resolution-20261106.md)). 와이드 덱의80×25 검은 띠는981에서 원인 확정·수정/검증 완료. 실제 활성 전체화면/큰 창 물리 확인은 사용자 대상이다. |
| 5차 | **일반 권한·읽기 전용 설치 정상 동작.** 설정/진행은`%USERPROFILE%\Saved Games\Netstorm\Saves\`, 신규 맵은`CustomMaps\`다. 모든 자동 생성 파일/디렉터리(임시/백업/잠금/장부/캡처/진단 포함)와 대응 reader도 같은Netstorm하위로 옮긴다. 원래/사용자 맵 병행 읽기와 Edit 스크롤/무제한도 포함한다. | **구현·해당 검증 완료(932~934).** 설정/체크포인트/표식/캡처/진단·Editor 작업본/장부·글꼴 캐시·PieceList 이관, Edit 목록 무제한/50행 스크롤, 읽기 전용 설치·유니코드 경로 실행 검증 완료([기록](docs/exe/cpp-user-data-root-20261105.md)). 4차 MCP 녹화 산출물도 사용자 `Captures`에 저장한다(936). 남음: `CustomMaps` 배치/복사 방식 사용자 확인. |
| 6차 | 한국어 지원 | **진행 중.** 938 1차 완료(글꼴·502항목 번역표·Language 메뉴·배포). 본문/버튼 그림·번역 검수·한글 입력은 잔여다. 986 후속 후보는 보존만 완료했다(0.4절) |
| 7차 | Options의AA/FSR1.0 선택은 기본 off. FSR 지원 환경에는2560×1440/3840×2160을 추가하고 해당 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 목표로 업스케일한다. [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)를 참고한다. | **구현·집중 검증 완료(939).** 미션/활성 전체화면/실제4K·입력 역변환과 성능은 사용자 확인/후속 최적화다. 전체 환경 검증 완료는 아니다. |
| 8차 | Edit 전용 시나리오 구성 UI(초기 자본/테크·적·동맹 등), 새 맵의 기본 english 자동 생성, english 부재 Test Battle 종료 개선 | **제품 구현 미착수·통합 후보 보존 완료(986).** 초기 자본/테크·적·동맹 편집, 기본english 자동 생성, 부재 Test Battle 개선은 아직 제품에서 쓸 수 없다. LAN·한국어 후속 뒤 통합한다. |

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

### 1.3 AGENTS.md 요구 반영 현황 (2026-11-06 대조)

AGENTS.md는 수정하지 않는다. 단계별 요구는 1.1절이 기준이며, 즉시 적용 요구의 cpppj 상태는 다음과 같다. DN/WB의 구현 완료 여부는 각 프로젝트 인계에서 확인한다.

| 요구 | cpppj 상태·근거 |
| --- | --- |
| Windows 실행 파일 `Netstorm.exe` | **완료(580·581).** CMake 대상명 NetstormCpp와 import library 이름은 유지·분리한다 |
| 11.01 표기·Credits 세 번째 항목 | **완료(685).** `Netstorm 11.01 Patch Credits`, 창 제목 `NetStorm 11.01 Patch Credits`, HJOW 개발·AI 사용 안내, 기존과 같은 Back/Cancel. [기록](docs/exe/cpp-product-1101-20261101.md) |
| 외부 도메인/IP 접속 문서화·클라이언트 유효성 검사 비활성화 | **완료(684).** [문서](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json). 구 서버 검증/security.dat writer·검증 getter·Not Validated 안내는 연결하지 않고 성공1도 합성하지 않는다. 사용자 선택 Help 링크와 로컬 LAN은 별도다 |
| Windows 10/11 AMD64·Windows 11 ARM 고려 | 현재 EXE AMD64(PE0x8664). ARM 빌드/실기 미검증이며 Windows 11 실기는 단계 전환 조건이 아니다 |
| 4~8차 구현 상태 | **4·5·7차 구현, 6차1차 구현, 8차는 후보만 보존.** 5차는733 조사에 머물지 않고932~934에서 실제 이관·reader·읽기 전용 설치 검증까지 마쳤다. 세부 상태와 잔여는1.1절·3절8번을 따른다 |
| 한국어·UTF-8·주석·용어 | 응답·새 코드 주석은 한국어, 새 문서/코드는 UTF-8. 한국어 글꼴은 `fonts/`의 D2Coding. 새 네임스페이스·상수·함수·구조체·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다. 명칭은 AGENTS.md의 번역/발음 표기를 따른다 |

## 2. 현재까지 완료한 범위와 검증 한계

### 2.1 완료 범위 색인

1133는 `a0342fae / 20261108-100 CP`로 커밋됐다. 1134은 4789b0 전체의 배율 관문·참가자/전투 메뉴·3×3 좌석/이동 조회·입력 예약을 현재 실제 controlled·CS·TYPE/SHP·카메라·설정·입력 큐에 연결했다. 기존 객체 메뉴의 메타+ac도 같은 본체를 호출한다. 메타 월드 마우스 선택 caller와 f3/6b 상태 소비자는 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1134 기록](docs/exe/cpp-meta-object-input-20261108.md). 후속은 메타 월드 객체 입력/선택 caller·f3/6b 소비자/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1132는 `3abed63a / 20261108-97 CP`로 커밋됐다. 1133은 498000의 같은200×200 선택 버퍼에서 메타+a4가 실제 CS/Image·TYPE/SHP·기존 글꼴/색을 쓰도록 연결했다. 임시 글자 문맥은 정상/예외 모두 복원하며 메타 마우스 입력/선택 caller는 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1133 기록](docs/exe/cpp-meta-selection-pixels-20261108.md). 후속은 메타 객체 입력/메타 선택 caller/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1131는 `739b04e5 / 20261108-94 CP`로 커밋됐다. 1132은 71h의 전체 observer/CS 비영 State 부모·전투 설명 복사/정제 자식을 일반·메타 등록표에 공통 연결하고 실제 객체 가상88·특정 전투 표시를 원문 순서로 수행하도록 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1132 기록](docs/exe/cpp-meta-battle-description-20261108.md). 후속은 메타 객체 입력/일반 선택 캡처 접합/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1130는 `be336598 / 20261108-88 CP`로 커밋됐다. 1131은 004787c0 전체의 기본 영역/큰 배율 보정·실제 dirty·현재 Form 이름표 갱신을 복원하고 보호 PE의88/8c 분배와 실제 메타 World 생성 직후 Client/Renderer/Gump 장치에 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1131 기록](docs/exe/cpp-meta-sprite-refresh-20261108.md). 후속은 메타 객체 입력/일반 선택 캡처 접합/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1129는 `76e7e04a / 20261108-84 CP`로 커밋됐다. 1130은 00478d10 전체 추가Draw의 참가자 Image/죽는 Rig Image·전투 TYPE/버전/설명·8자리 표시를 실제 CS·Pool/ProcessForm·동적 할당·SHP·현재 메뉴 글꼴과 같은 통합 Renderer 화면에 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1130 기록](docs/exe/cpp-meta-sprite-draw-20261108.md). 후속은 메타 파생 표시 갱신/메타 객체 입력/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1128는 `52cfee23 / 20261108-82 CP`로 커밋됐다. 1129은 공급 반경/4float 선택표를 실제 Client 전역에서 공유하고 메타67h도 전체 State 부모를 통해 현재 옵션 복사·CS 통지·범위/TYPE 비용/환급·선택 HUD 재생성을 수행하도록 연결·컴파일했다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1129 기록](docs/exe/cpp-meta-battle-options-integration-20261108.md). 후속은 메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1127는 `3f942573 / 20261108-80 CP`로 커밋됐다. 1128은 004c7880의 기존 HUD Kill/Cleanup 선행·전체 새 자식 생성/임시등록 반납·루트 공개를 실제 공통 Gump 장치로 복원하고 일반67h의 선택 UI 훅에 연결·컴파일했다. 메타67h 전체 부모/공유 파생 전역은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1128 기록](docs/exe/cpp-hud-recreate-20261108.md). 후속은 메타67h 전체 부모 등록/공유 반경·파생 전역·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1126는 `aebe0f12 / 20261108-79 CP`로 커밋됐다. 1127은 67h 전체 State 부모를 공통화해 실제 일반 수신에 접합하고 누락된 일반 Session의 CS 자식을 현재 Client CS/음향/NSInfo에 연결·컴파일했다. 메타 등록표의 전체 부모와 공유 파생 전역·HUD 재생성은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1127 기록](docs/exe/cpp-state-battle-options-20261108.md). 후속은 메타67h 전체 부모 등록/공유 반경·파생 전역/004c7880 HUD 재생성·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1125는 `739fc118 / 20261108-76 CP`로 커밋됐다. 1126은 카메라 Rig 생성/재사용/프레임/반납·intro wrapper와 실제 Client/CS/Kernel/Host/Renderer 바인더를 컴파일했다. 카메라 개수 DWORD와 기존 메타 갱신 예약을 공유하며 참가자 생성 caller 연결은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1126 기록](docs/exe/cpp-meta-camera-rig-20261108.md). 후속은 67h 전체 State 부모/일반 CS hook·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1124는 `4e666780 / 20261108-73 CP`로 커밋됐다. 1125은 실제 Client/현재 Session·Kernel·Gump·시각·CS Image owner를 IslandRig에 연결하고 원래 State wrapper를 포함한23h 추가/24h 삭제를 같은 TCP/UDP 표에 등록·컴파일했다. 전체 메타 장면 실행 검증은 아니다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1125 기록](docs/exe/cpp-meta-battle-receivers-20261108.md). 후속은 67h 전체 State 부모/일반 수신의 CS hook·카메라 Rig·메타 객체 그림/나머지 CS 수신/참가자 생성/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1123는 `4664c37b / 20261108-70 CP`로 커밋됐다. 1124은 CS Image의 깊은 복사·현재 Animator TYPE와 실제 Kernel/ProcessForm IslandRig의 생성/부착/프레임/반납 본체를 컴파일했다. 현재 전역 getter를 사용하며 실제 Client 장치 바인더·소비자/23h 등록은 후속이다. NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. [1124 기록](docs/exe/cpp-meta-island-rig-20261108.md). 후속은 실제 Client Rig 장치/생성 소비자·23h 등록·카메라 Rig·메타 객체 그림/나머지 CS 수신/참가자 생성/HTML→1dh/28~33 등록/1095 송신 상태 호출이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1122는 `04947470 / 20261108-68 CP`로 커밋됐다. 1123은 Spring3d 생성/목표/시간 누적 계산을 같은 raw 필드 뷰로 복원·컴파일했다. 원본 double 1/30초·완료 비트·패딩 보존·F32 저장을 대조했고 음수 높이 속도의 도착 비교를 정정 후 재빌드했다. 첫/최종 두 Release 종료0·경고/오류0·보호2800 불변·새fixture/C++/MP/게임/통신 실행0이다. [1123 기록](docs/exe/cpp-meta-spring3d-20261108.md). 실제 Rig/카메라 프로세스·23h 등록은 후속이며 나머지 우선순위와 사용자 검사 제외를 유지한다.

1121은 `f40e1625 / 20261108-66 CP`로 커밋됐다. 1122는004adca0을 배율 DWORD 쓰기로 정정하고 현재 Client/Session의 실제 팩터리·Pool·QA·Pop 바인더를 추가·컴파일했다. 바인더 소비자/Rig/23h 등록은 후속이다. 두 Release 종료0·경고/오류0·보호2800 불변·새fixture/C++/MP/게임/통신 실행0이다. [1122 기록](docs/exe/cpp-meta-battle-object-binding-20261108.md). 나머지 우선순위와 사용자 검사 제외를 유지한다.

1120은`d20f510c / 20261108-64 CP`로커밋됐다.1121은메타전투생성/23h부모와호출부식별자구조체/값전달을복원·제품에컴파일했다.ret18h는스택DWORD6칸이며식별자구조체가2칸을쓰는논리인자5개다.마지막기존표시가Rig관문이다.이전1120설명도정정했다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1121 기록](docs/exe/cpp-meta-battle-create-parent-20261108.md).실제물리/Rig장치/23h등록과나머지메타연결은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

1119는`4d19ab9a / 20261108-62 CP`로커밋됐다.1120은0042c920 전체67h옵션수신을실제공유등록표/현재CS·guy.wav/NSInfo에접합했다.160포함상한/자기master관문·음향후현재길이복사/현재BYTE저장/조건부UI를보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1120 기록](docs/exe/cpp-meta-battle-options-receive-20261108.md).나머지CS수신/생성·Rig/HTML·상태접합은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

1118은`080c977d / 20261108-59 CP`로커밋됐다.1119는CS명단첫일치/실제0번·이름뷰/진단·빈8슬롯/전투버전WORD집계를같은실제owner에복원·컴파일했다.초기max-1/min256·active필터없음·최종min/max순서를보존한다.최초/최종두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1119 기록](docs/exe/cpp-challenge-record-queries-20261108.md).수신/생성부모호출과나머지메타연결은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

1117은`cd63f421 / 20261108-56 CP`로커밋됐다.1118은BattleInfo입력/다섯메뉴와실제현재TYPE별칭/Deck·전역옵션/Xlat정체성·67h/가상재부착을연결하여NSInfo세Handle을공통분배에접합했다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1118 기록](docs/exe/cpp-nsinfo-battle-integration-20261108.md).CS수신/생성·카메라Rig/HTML→1dh/28~33등록/1095송신상태호출은후속이다.전체메타장면성공이아니며우선순위/사용자검사제외를유지한다.

1116은`78de23a7 / 20261108-54 CP`로커밋됐다.1117은지정선택getter/보호PE76포인터·현재Client실제Xlat값정체성을제품에복원·컴파일했다.임시변환버퍼주소를배제하고원문폴백주소를보존한다.기존두Translate본문은정적동일이다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1117 기록](docs/exe/cpp-battle-choice-identity-20261108.md).실제메뉴/현재별칭·492960·1115/1116분배는후속이다.실제입력/전체화면성공이아니며우선순위/사용자검사제외를유지한다.

1115는`f18a7e1c / 20261108-53 CP`로커밋됐다.1116은BattleInfo다섯메뉴자식과선택문자열정체성비교/현재TYPE·전역제외/12별칭·여섯CRT정렬/다섯출력을복원·제품에컴파일했다.실제getter/메뉴장치와1115입력분배는후속이다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1116 기록](docs/exe/cpp-nsinfo-battle-menus-20261108.md).실제입력/전체화면성공이아니며우선순위/사용자검사제외를유지한다.

1114는`77bc93f6 / 20261108-52 CP`로커밋됐다.1115는BattleInfo입력분기부모와0047b180 전체기본값적용을제품에복원·컴파일했다.권한/signed태그·독립명령·실제writer/송신·현재위치재부착/반환값을보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1115 기록](docs/exe/cpp-nsinfo-battle-input-parent-20261108.md).다섯메뉴자식/실제UI·부모분배와나머지메타연결은후속이다.실제입력/전체화면성공이아니며우선순위/사용자검사제외를유지한다.

1113은`1c565aec / 20261108-49 CP`로커밋됐다.1114는BattleOptions 일곱전체getter/writer·현재포함최댓값/실제BYTE·제외목록이동과음향전후를제품에복원·컴파일했다.최초/현재count·전체DWORD비교/BYTE저장·끝잔존BYTE를보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1114 기록](docs/exe/cpp-battle-option-mutation-20261108.md).BattleInfo전체입력/실제메뉴의1114호출과나머지메타연결은후속이다.전체화면성공이아니며우선순위/사용자검사제외를유지한다.

1112는`4d408c76 / 20261108-47 CP`로커밋됐다.1113은0047a900 전체 PlayerInfo입력과현재버튼serial/value·CS·준비/시작·다섯슬롯writer/자기594fc4·실제1fh/87BYTE67h·추방확인메뉴를연결했다.원문헤더전후/실제쓰기·부분효과·태그소비1/기타0을보존한다.두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1113 기록](docs/exe/cpp-nsinfo-player-input-20261108.md).BattleInfo전체입력/옵션·제외writer와나머지메타연결은후속이다.전체화면성공이아니며우선순위/사용자검사제외를유지한다.

1111은`6f25a98e / 20261108-44 CP`로커밋됐다.1112는0047c8c0 전체 루트입력과실제현재정렬표/CS·1111메뉴/원시좌표·같은Session카메라·1fh헤더/우선순위1추방음향/TCP11·원래BaseHelp를연결했다.별도mode/known관문을추가하지않고추방은직접참가자번호를쓴다.첫const호출오류1건/후속 경고1건교정뒤두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1112 기록](docs/exe/cpp-nsinfo-root-input-20261108.md).두자식입력/1107슬롯·1108자료송신 UI호출과나머지메타연결은후속이다.전체화면성공이아니며우선순위/사용자검사제외를유지한다.

1111은World없는전체메타 Player메뉴공장을실제 Client/CS/Chat·Session·controlled/객체Form에접합했다.공유서식본문은정적동일이고주소/실제신뢰도/수명도연결했다.1110은`a487fc19 / 20261108-40 CP`로커밋됐다.첫/최종두Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1111 기록](docs/exe/cpp-meta-player-factory-20261108.md).NSInfo 세Handle/카메라·1107슬롯/1108송신UI호출은후속이다.전체메타화면성공이아니며나머지우선순위/사용자검사제외를유지한다.

1110은004785f0 참가자 메뉴 전체/00478310 신뢰도를 복원하고 기존Player메뉴의신뢰도소비자를 실제CS에 연결했다.1109는`9f7c0604 / 20261108-37 CP`로 커밋됐다.두 Release 종료0·경고/오류0·보호2800불변·새fixture/C++/MP/게임/통신실행0이다.[1110 기록](docs/exe/cpp-meta-player-menu-20261108.md).World없는 메타메뉴공장·NSInfo 세Handle/카메라·1107슬롯/1108송신 UI 호출은후속이다.전체메타화면성공이아니며나머지우선순위/사용자검사제외를유지한다.

1109는PlayerFinder 실제 현재CS 지연 getter를 Chat에 접합했다.NSInfo Handle/메뉴/카메라·UI 호출은 후속이다([1109 기록](docs/exe/cpp-meta-player-identity-20261108.md)).

1108은실제1fh 상태 writer/67h 자료 송신·관찰기를 같은TCP11에 접합했다.NSInfo Handle/메뉴/카메라·UI 호출은 후속이다([1108 기록](docs/exe/cpp-nsinfo-meta-senders-20261108.md)).

1107은메타 슬롯5몸체/공통 랭크를 복원하고 PlayerInfo 표시 getter를 공유했다.실제 writer 입력 호출/메타 Handle·메뉴/송신은 후속이다([1107 기록](docs/exe/cpp-nsinfo-slot-options-20261108.md)).

1106은PlayerInfo 특수 버튼 전체 Draw를 실제 공통 픽셀에 접합했다.메타 입력은 후속이다([1106 기록](docs/exe/cpp-nsinfo-player-button-draw-20261108.md)).

1105는BattleInfo 전체 메타 배치/설정과 실제 옵션 문자열을 접합했다.메타 입력/특수 버튼 Draw는 후속이다([1105 기록](docs/exe/cpp-nsinfo-battle-layout-20261108.md)).

1104는PlayerInfo 전체 메타 배치를 접합했다.BattleInfo 전체 배치·메타 입력/버튼 Draw는 후속이다([1104 기록](docs/exe/cpp-nsinfo-player-layout-20261108.md)).

1103은NSInfo 전체 메타 루트/참가자 목록을 접합했다.두 자식 전체 배치·메타 입력/버튼 Draw는 후속이며 전체 화면 성공이 아니다([1103 기록](docs/exe/cpp-nsinfo-meta-root-20261108.md)).

1102에서실제메타World/UI/Renderer·Poll/전체배치·공통Draw/카메라Reset을접합했다.CS수신/생성·카메라Rig·HTML/NSInfo메타배치와1dh·28~33등록은후속이다([1102 기록](docs/exe/cpp-meta-renderer-integration-20261108.md)).

1101에서실제MetaMap Session·공통Form과전체이탈/Native반납수명을연결했다.CS수신/생성·전체Renderer/카메라·HTML/NSInfo메타배치/1dh·28~33등록은후속이다([1101 기록](docs/exe/cpp-meta-world-lifetime-20261108.md)).

1100에서0047dd60 전체와실제글자/SHP하위를컴파일했다.실제World/Pool·CS/Renderer/NSInfo메타배치·수명과1dh/28~33등록은후속이다([1100 기록](docs/exe/cpp-meta-scene-20261108.md)).

1099에서실제Gump장치/글자공장·Push와기존NSInfo/current를연결했다.1098의중복CS슬롯/+18해석을정정했고전체장면/CS·객체/Renderer·NSInfo메타배치/1dh등록은후속이다([1099 기록](docs/exe/cpp-meta-gump-devices-20261108.md)).

1098에서전체메타Poll·목록갱신/종속finder·같은Client 목록슬롯을컴파일했다. 실제0047dd60/CS·객체/Gump장치와1dh/28~33등록은후속이다([1098 기록](docs/exe/cpp-meta-display-poll-20261108.md)).

1097에서참가자/전투섬·이름표/8자리·51존정리와1096투영을컴파일했다. 실제Pool/가상객체/Gump장치접합·전체표시/CS·1dh/28~33등록은후속이다([1097 기록](docs/exe/cpp-meta-display-updates-20261108.md)).

1096에서메타원근카메라·투영/기준보정·객체배율/클리핑과같은Client owner를컴파일했다. 전체표시/CS실제장치와1dh/28~33등록은후속이다([1096 기록](docs/exe/cpp-meta-projection-20261108.md)).

1095에서같은실제CS/TCP의master1fh통지와게임로그인7ch전송장치를컴파일했다. 실제State33호출/1dh/28~33등록은메타표시/CS장치완성뒤접합한다([1095 기록](docs/exe/cpp-meta-peer-requests-20261107.md)).

1094에서1dh/28~33본체·CS master/캐시감시를라이브러리컴파일하고실제Root요청목적지인자/별도연결DWORD를복원했다. 실제1dh등록/28~33분배는미적용이며표시/CS·실제장치완성후접합한다([1094 기록](docs/exe/cpp-meta-connected-bodies-20261107.md)).

1093에서Root0Fh 실제수신/관찰·동일Client기록/State·서버목록갱신·사용자홈저장/시작읽기를연결했다. 원문47/친구3과제공10.71 compact호환을구분했으며1dh/State28..33/CS·실제메타표시는후속이다([1093 기록](docs/exe/cpp-root-login-reply-20261107.md)).

1092에서클라이언트실제새R실행/동일자료CWD·콘솔/핸들장치와배포동봉·한ZIP바이트검증을연결했다. 게임/서버·통신실행0이며0F/1d·메타와나머지LAN은후속이다([1092 기록](docs/exe/cpp-local-server-process-package-20261107.md)).

1091에서 같은 최종Server owner/실제모듈·수명/메인·새R.exe entry의Run호출·CMake빌드를연결했다. 새R실행/MP검사는0이며 클라이언트OSlaunch/배포는후속이다([1091 기록](docs/exe/cpp-root-server-entry-20261107.md)).

1090에서 R실제 로그/OS시각·사용자홈HTML/호스트스택·복귀형ZAssertion을 작성했다. 새R.exe최종owner/실제장치·Bind/Run 연결은 후속이다([1090 기록](docs/exe/cpp-root-process-trace-20261107.md)).

1089에서 R시작/정리·실제노드생성/현재번호공개와 같은 메인hook을 연결했다. 새R.exe최종owner/실제Bind·Run·OSlaunch/배포는 후속이다([1089 기록](docs/exe/cpp-root-lifecycle-20261107.md)).

1088에서 R전역 함수/버전 초기화·17수신/102관찰 등록·동적32몸체를 같은 표에 연결했다. 전체 시작/정리·최종R owner/실제 InitializePackets/Run 호출은 후속이다([1088 기록](docs/exe/cpp-root-packet-registration-20261107.md)).

1087에서 실제 원격명령 TCP/4세션·현재집합/Poll/종료와 같은OS/메인hook을 연결했다. 기본원문에없는 Open/handler등록은 추가하지 않으며 최종R owner/실제Bind·초기화/Run은 후속이다([1087 기록](docs/exe/cpp-root-remote-command-20261107.md)).

1086에서87h 전체/15상위·6관리자·목록HTML/현재통지·제재/파일·실제수신을 연결했다. 원문18중17몸체 작성/컴파일과 폐기80비활성 상태이며 최종서버/전체초기화·호출은 후속이다([1086 기록](docs/exe/cpp-root-command-receiver-20261107.md)).

1085에서 같은 현재 길드/가입요청의 조회·생성/변경/제거·실제 사용자 홈 저장과 CCh owner 참조를 연결했다. 87h 전체/목록HTML·관리제재/관찰기·실제등록/최종호출은 후속이다([1085 기록](docs/exe/cpp-root-guild-management-20261107.md)).

1084에서87h의 실제 템플릿/섹션·OS시각/현재존 치환과 확장 CCh 응답 부품을 연결했다. 87h 전체 수신/길드·관리자 분기·최종 owner/실제 호출은 후속이다([1084 기록](docs/exe/cpp-root-command-pages-20261107.md)).

1083에서11/12h 진단·실제 역방향LAN TCP와 현재번호/게임시각 부모를 연결했다. 최종R owner/실제Install/Connect/Run 호출은 후속이다([1083 기록](docs/exe/cpp-root-diagnostic-20261107.md)).

1082에서0b/0c인사·66h/LAN공지와현재일반번호·하위pending guard를연결했다. 최종R owner/실제번호발행·Install/Announce/Run호출은후속이다([1082 기록](docs/exe/cpp-root-control-20261107.md)).

1081에서83h관리명령·실제하위socket/예약·Root안내/차단과같은상태Bind를연결했다. 최종R owner/실제Bind·Install/Run호출은후속이다([1081 기록](docs/exe/cpp-root-admin-command-20261107.md)).

1080에서1fh준비·실제전투시작/난수·현재주소/flush·종료·삭제와재배치를연결했다. 폐기quota는비활성유지하며최종R owner/실제Install/Run호출은후속이다([1080 기록](docs/exe/cpp-root-battle-launch-20261107.md)).

1079에서1eh 이동전체수신·실제입장/탈퇴·master/명단·옵션전송을연결했다. 최종R owner/실제Install/Run호출은후속이다([1079 기록](docs/exe/cpp-root-challenge-move-20261107.md)).

1078에서10h 관문·67h 옵션/68h 조회/71h 설명 전체 수신과 같은CCh·실제전송을 연결했다. 최종R owner/실제Install/Run호출은후속이다([1078 기록](docs/exe/cpp-root-battle-control-20261107.md)).

1077에서41h전체채팅수신·명령/줄/빈도제한·같은mutable패킷전달을연결했다. 최종R owner/실제Install/Run호출은후속이다([1077 기록](docs/exe/cpp-root-challenge-chat-20261107.md)).

1076에서 실제 rootservers 표/체크섬·행/파일 재로드/사용자 홈 저장과 같은 메인hook을 연결했다. 최종 R owner/실제 Reload·Save·Run 호출은 후속이다([1076 기록](docs/exe/cpp-root-server-list-20261107.md)).

1075에서 실제 메인 cycle/중복CCh Tick·파일감시/예약리셋·Root41h 안내 경로를 같은 현재 부모에 연결했다. 실제 서버목록·보조장치/41h 수신/전체초기화·정리/최종owner·호출은 후속이다([1075 기록](docs/exe/cpp-root-main-loop-20261107.md)).

1074에서 실제 CCh 주기/전투 재생성·가상 참가자/배치·R 전역 난수와 현재 시계/기존 전송을 연결했다. 메인루프의 중복Tick/최종 R owner·실제 호출은 후속이다([1074 기록](docs/exe/cpp-root-challenge-periodic-20261107.md)).

1073에서 R의 실제 FullTime/프레임 통계·같은 현재 시계의 사용자 홈 패킷 기록과 Config/전송 hook을 작성했다. 최종 owner/시계 초기화·주기/실제 기록 호출은 후속이다([1073 기록](docs/exe/cpp-root-clock-record-20261107.md)).

1072에서 Root 주소 문법/서식 고리·실제 DNS 없는 로컬 주소와 같은 로그인/CCh hook을 작성했다. 최종 R owner/실제 호출은 후속이다([1072 기록](docs/exe/cpp-root-address-codec-20261107.md)).

1071에서 Root 공유 관리6로더/길드·뉴스와 파일 시각 감지·사용자 홈 blacklist 저장·같은 Config/CCh 장치 Bind를 작성했다. 최종 R owner/실제 초기화·watcher 주기와 호출은 후속이다([1071 기록](docs/exe/cpp-root-shared-files-20261107.md)).

1070에서 Root 현재 등록표의 NULL 직접 호출·17h 원격 요약/공유 참가자 생성자를 연결했다. 파일/표·주소·최종 R owner/실제 호출은 후속이다([1070 기록](docs/exe/cpp-root-routing-20261107.md)).

1069에서 CCh 로그인/종료 전체 부모·현재 참가자/전투 수명과 실제 패킷 작성/관찰기를 연결했다. 상위 표·파일/주소·Root 라우터/새R.exe 실제 호출은 후속이다([1069 기록](docs/exe/cpp-root-challenge-runtime-20261107.md)).

1068에서 Root1Ch 수신 관문/실제 대기 소켓 이전과 CCh 새 stream 노드 구성 API를 연결했다. 실제 CCh 로그인·참가자 종료/응답·localAddress/새R.exe 호출은 후속이다([1068 기록](docs/exe/cpp-root-login-request-20261107.md)).

1067에서 실제 R CCh 물리/최초 표·단일 프로세스 생성/토큰·현재40개 전송과 명시 정리를 연결했다. 실제 Root 전체 초기화/호출·CS 로그인/종료·보조 장치는 후속이다([1067 기록](docs/exe/cpp-root-challenge-store-20261107.md)).

1066에서 R 시작 설정/재해석·현재 옵션/PE초기값·문자열과 실제 Config/통신 전역/Root 참조를 접합했다. 실제 호출·moderation/기록 장치·CCh 관리자는 후속이다([1066 기록](docs/exe/cpp-root-configuration-20261107.md)).

1065에서 R16h→0Fh 실제 추천/존·친구 응답·관찰기와 전송 접합 API를 작성했다. 실제 초기화/호출·CCh 관리자·0F/1d 클라이언트 수신은 남는다. 옛 인계의 응답17h는0Fh로 정정한다([1065 기록](docs/exe/cpp-root-directory-20261107.md)).

1064에서 실제 R 전송 노드/버퍼·번호·닫힘 수명과 소켓·메인 부모를 연결했다. 새R.exe/실제 프로세스·Root/CS·보조 객체/시계/기록/formatter·호출은 후속이다([1064 기록](docs/exe/cpp-root-transceiver-transport-20261107.md)).

1063에서 실제 R IPv4 소켓 장치와 Player 없는 공통 프레임 분배를 추가하고 기존 Client를 같은 몸체에 연결했다. R 버퍼/전송 노드·부모/새R.exe·Root/CS 접합은 후속이다([1063 기록](docs/exe/cpp-root-socket-device-20261107.md)).

1062에서 R 메인 통신 부모를 정적 라이브러리에 컴파일했다. 실제 소켓/분배·Root/CS·새R.exe/호출은 후속이다([1062 기록](docs/exe/cpp-root-network-pump-20261107.md)).

1061에서 비Session 등록 ID의 실제 저장/공유 Notify를 연결해1060의 Session 부재 오류를 해소했다. 도구 소비자 AccountSetup은 폐기 외부 등록 경계로 분류하고 실행을 연결하지 않는다([1061 기록](docs/exe/cpp-registered-fort-subscriber-20261107.md)).

1060에서 실제 등록 메시지/설정 재적재·현재 도구 표식/전체화면 복원을 연결했다. 비Session Fort 동기화와 일반 명령 소비자는 후속이다([1060 기록](docs/exe/cpp-registered-messages-20261107.md)).

1059에서실제37파일탐색부모/R전용번호·종료·모드부품을추가했다. 새R실행장치/서버런타임은미구현이다([1059 기록](docs/exe/cpp-root-routing-launch-20261107.md)).

1058에서실제UDP12/공유표·원문송수신/Client검색전역·CRT/메타34·35·36·38/66h동기Run을연결했다.37/새R서버·응답·메타전체는남는다([1058 기록](docs/exe/cpp-meta-udp-discovery-20261107.md)).

1057에서R전용정밀디컴파일/66h·토큰/State34·35검색·발견수신·서버세번응답의원문부품을준비했다. 실제UDP/장기전역/큰상태/새서버실행은미연결이다([1057 기록](docs/exe/cpp-local-server-discovery-20261107.md)).

1056에서실제 메타20~27/서버 행·endpoint/TCP10·11/16h·1ch 요청을 접합했다. 검색34~38/로컬 서버·로그인 응답0Fh/1dh·28~33/메타 표시는 남는다([1056 기록](docs/exe/cpp-meta-connection-20261107.md)).

1055에서State19 실제 Uber/Fort/Core/CS/뉴스/채널과 Client 깊이/표본 수명을 연결했다. 즉시21/25/34 부모가 미연결이므로 메타 진입/표시는 아직 완료되지 않는다([1055 기록](docs/exe/cpp-meta-state-client-20261107.md)).

1054에서State19 부모 부품/별도CD 저장소와 같은 Client 뉴스 키·시각/실제 수신 설정·HTML을 작성·컴파일했다. 실제State19 Uber/Fort/Core/채널·즉시25/34/21 부모와 메타 표시는 남는다([1054 기록](docs/exe/cpp-meta-state-news-20261107.md)).

1053에서 CS의 실제 기록/생성·재사용/최초 좌표와 같은 Client/로그인 포인터 수명을 작성·컴파일했다. 실제State19/28 전체 호출자·메타 입력/표시는 남는다([1053 기록](docs/exe/cpp-challenge-owner-20261107.md)).

1052에서 사용자 제보 SubscriberGlobalsTests.cpp의 구형 크기 단언/C2607을 수정하고 같은 파일의 기존7검사 통과를 확인했다. 게임 실행 제품은1051 그대로다([1052 기록](docs/exe/cpp-subscriber-globals-compile-fix-20261107.md)).

1049의 제외 방송 진단에 이어1050에서 실제 caller004dc050/51h·004dbce0의 Territory/표면·자산 정리·마지막 섬과 기존 Tick/event3를 연결했다. 1051에서 신전command/event4·ViewMeta 저장/State19 요청까지 연결했다. State19/CS·메타의 실제 부모는 남는다([1051 기록](docs/exe/cpp-vortex-command-descend-20261107.md)).

아래의 완료는 **표에 적힌 구현·대조 범위**다. 원본 전체 동일성이나 최신 제품의 전체 회귀 완료를 뜻하지 않는다. 단위별 원문 관측·검사 수·최초 실패·교정 내역·제품 해시는 [정리 전 전체 원문](LEFT_JOBS.old.md#archive-20261106-through-986)과 각 근거 문서에 보존했다. 과거 검사 수와 미연결 예고를 반복 나열하지 않는다.

| 범위 | 완료한 구현·대조와 최신 근거 | 남은 범위의 위치 |
| --- | --- | --- |
| 비멀티 메뉴·입력/UI | 무인자 시작·메인/브리핑/Help/Credits·단축키·객체/화물 메뉴·선택/커서·HUD/Chat/Console/모달 수명. 697~761/913~929, [929 정리](docs/exe/cpp-ui-implementation-closure-20261105.md) | MP 전송/메타/의식은3절8번, 전체 외관·물리 입력은5·7번 |
| 원문 Edit·시나리오 실행 | Fort 읽기/쓰기·Save as·섬/그림/이동·owner/Graph·동적Config/TARC/HTML 명령·저장 실패/재시도·Test Battle/편집 복귀. [943 정리](docs/exe/cpp-editor-implementation-closure-20261106.md) | 신규 시나리오 UI/english 자동 생성은8차. 모든 조합/손상/OS 실패 관측 완료는 아님 |
| 미션 교체·State | Retry/Next/Abort·Guide 교체·실제 연속 왕복·State1/2/3/단독7·LeaveBattle·cleanup/Tick. [956 정리](docs/exe/cpp-mission-state-scope-20261106.md) | 일반 전투/State6·저장/재접속은3절8번. 자연 승리/진행 해제는 사용자 |
| 데모 반복 | 제품939에서 세 데모 모두360초 두 만료→세 번째 월드 진행→Exit Demo/메인/종료. [946](docs/exe/cpp-demo-second-storm-20261106.md)·[947](docs/exe/cpp-demo-second-guard-20261106.md)·[948](docs/exe/cpp-demo-second-alliance-20261106.md) | 모든 자연 전투/승패·최신 제품 회귀와 구별 |
| 월드·규칙·Renderer | 공통 AI·워크샾25종·의식/SP·간헐천·투사체/다리/지형·단일 표시 목록·카메라/미니맵·Graph/색표. [25종](docs/exe/cpp-unit-restoration-20261022.md)·[Renderer](docs/exe/cpp-renderer-frame-20261030.md)·[Graph](docs/exe/cpp-editor-graph-world-20261102.md) | 자연 플레이/전체TYPE 프레임·공중3종 신규 전용 검사는 제외. Graph 장부 차이8은 원본에도 있어 정규화하지 않음 |
| 현재TYPE·HP·생성/반납 | 834~912의 Unpop/Take/Pop·현재VT/TYPE·Form/Process/Factory·의식/너겟/Rune·HP/Regular·실제 Session 연결과957~979 후속. [Pop 범위](docs/exe/cpp-pop-current-scope-20261106.md)·[Factory](docs/exe/cpp-factory-parent-scope-20261106.md)·[980 정리](docs/exe/cpp-internal-lifetime-closure-20261106.md) | checking5·미완료MP writer/decoder·CD전체/ABI 자체/미관측 실패는 분리. 효과 정적 연결을 실제 포획/희생 검사로 세지 않음 |
| checking 소비자·군도 | 779~833의 현재 진단 복귀/장기owner·Graph/연소/마나/도착·892~895 군도 root/CoreData. [함수 색인](docs/exe/cpp-checking-consumer-gaps-20261103.md)·[군도 root](docs/exe/cpp-terrain-generation-root-20261105.md) | 892/893의 당시 상위 미연결은894에서 연결. 일반 State7 밀도 writer는992·중앙섬은990 연결. 남은 MP 부모는3절8번 |
| FILE·자산/색표·저장 실패 | 파일명/경로·Native FILE/키/TAFF·Config/TYPE/GIF/SHP/글꼴·옵션 교체/복구·실제 색표Cache·Kernel/SID/new_handler/종료. [966](docs/exe/cpp-options-save-failures-20261106.md)·[968](docs/exe/cpp-colormap-consumers-startup-20261106.md)·[980](docs/exe/cpp-internal-lifetime-closure-20261106.md) | FILE 직접74호출의 현재54/MP·폐기인터넷18/폐기CD1/휴면Chat1 분류. 32비트ABI/실제OS 고갈/전체PE 간접호출 부재 증명과 구별 |
| 4차 MCP·와이드 | 935~937의 조작/상태/PNG/대기/소리 포함 녹화·1600×900/1920×1080. [MCP](docs/exe/cpp-mcp-server-20261106.md)·[해상도](docs/exe/cpp-wide-resolution-20261106.md). 덱 검은띠는[981](docs/exe/cpp-wide-deck-background-20261106.md) 수정 | 물리 입력/활성 전체화면 출력은 사용자/환경 |
| 5차 사용자 데이터 | 932~934의 Saves/CustomMaps/Editor/Captures/Diagnostics·reader·무제한/50행 스크롤·읽기 전용 설치/Unicode 실행. [5차 기록](docs/exe/cpp-user-data-root-20261105.md) | CustomMaps 배치/복사 방식은 사용자 확인(3절8번) |
| 6차 한국어1차/7차 후처리 | 938 D2Coding/표시 부호화·502항목·언어 메뉴/영어fallback, 939 D3D11 AA/공식FSR1·1440p/4K 선택/캡처. [한국어](docs/exe/cpp-korean-language-20261106.md)·[AA/FSR](docs/exe/cpp-aa-fsr-20261106.md) | 한국어 본문/그림/입력은 미적용 후보. 미션/활성 전체화면/실제4K/입력 역변환은 사용자 확인 |
| 호환·배포·제품 정책 | 전체화면/3화면비/가장자리·저장 재시작(746), `Netstorm.exe`/배포 스크립트/빌드 선택(562/637/739), 11.01/Credits685·외부접속 분류/검증 비활성684/965 | 최신제품 ZIP 검증 미실시. 구 CD검증7f/80 복원 금지. 1.3절·3절7·8번 |
| 최신 사전 구현·인계 | 982~1015의 실제 TCP/State·LAN UI/명단·Login/저장/복구·서버 인수와 자동 장애,1016~1018 일반 자원 프레임,1019~1040 Tell·비용/취소·전체 객체/압축/검증·내용물/삭제·감시/공유 예약·총/아비어리/비행/애니메이션 수신과 애니메이션 송신/TYPE·건설37h/PlayerKnown·Path30h·공간33h/34h/35h·현재32h 설정·개발자31h33명령분기/화면 강조·건설기록writer를 연결·컴파일했다. [1040](docs/exe/cpp-developer-build-record-20261107.md). 986 후보 보존 | 나머지 수신/전송·재동기화/Activate·제재/메타/Outpost가 잔여다. checking5·신규MP검사0이며986 후보는 미적용 |
| 1135 메타 설명 입력 | f3 실제 CS 문자열·공통41BYTE 모달·CopyText/LF 확정·현재 권한·71h/TCP11 연결/컴파일. [1135](docs/exe/cpp-meta-description-input-20261108.md) | 메타 실제 마우스/항해·나머지 상태/수신은3절8번. 신규MP검사0 |
| 1136 메타 참가·항해 입력 | 6b/6c/e3/fe·실제 좌석/SHP/현재 CS·1eh/TCP11·Rig·경고/확인 이름4개·메뉴→큐 허용 연결/컴파일. [1136](docs/exe/cpp-meta-sail-input-20261108.md) | 메타 마우스 caller·나머지 상태/수신·BattleDesc HTML은 잔여. 신규MP검사0 |
| 1137 메타 마우스·영역·항해 메뉴 | 같은 실제 Session/픽셀 picker/선택·좌우 클릭·모드6·glyph479390/e8·fancy 항해 메뉴·공통 Gump/호스트 지도 화면 연결/컴파일. [1137](docs/exe/cpp-meta-pointer-input-20261108.md) | 메타 키보드/공통 메뉴·나머지 상태/수신·BattleDesc HTML은 잔여. 신규MP검사0 |
| 1138 메타 ESC·공통 키/메뉴 | 실제 Session/SP·선택/커서/serial·공통 가로 메뉴·생산/선택/View/카메라·F9와 일반F6 caller 연결/컴파일. [1138](docs/exe/cpp-meta-common-input-20261108.md) | c6·나머지 상태/수신·BattleDesc HTML은 잔여. 신규MP검사0 |

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
| 데모 3개 | 645 실제 메뉴 진입. 946~948에서 제품939의 각360초 두 만료→세 번째 월드/비정지 진행→Exit Demo/메인/종료0 확인 | 최신 제품 회귀·모든 자연 전투/승패·전체 동일성 |

### 2.3 음향·영상·캡처의 확인 범위

현재 위치 음향 조회/장치 순서(672), 곡 만료의 현재 battle/정리 순서(673), MovieGump/스킵·끝·CD 경로/캐시(594·595·654·655·670), 연속8비트BMP 수집(693~695)은 연결했다. [위치 음향](docs/exe/cpp-sound-current-view-20261101.md), [영상 owner](docs/exe/cpp-movie-override-20261101.md), [화면 수집](docs/exe/cpp-capture-client-20261102.md)을 따른다.

**936에서 소리 포함 녹화 후속 성공:** HJOW-Athlon HD Audio의 프로세스WASAPI 101,653프레임 중 비무음99,726프레임과 영상/소리 합성을 확인했다. 따라서 실제 PCM·소리 녹화 전체를 미검증으로 쓰지 않는다. 청취·다른 장치·실제CD 삽입·활성 전체화면 물리 출력·전체 영상 동일성·수집 중 모드 변경/1000프레임 GUI는 미검증이다. 672의 전체audio 실패 원시는 당시 증거로 보존하며 후속 성공으로 지우지 않는다. 세 데모 반복은946~948의 별도 증거이고 녹화 장기 안정성의 증거는 아니다.

### 2.4 현재 이슈의 처리 위치

| 항목 | 현재 처리 |
| --- | --- |
| 물리 캡처/입력 | 환경 대기.762의 FrameArrived/window capture timeout 뒤 실제 환경 변화 전 반복 금지(3절7번) |
| Early1 남동 경로 | 자연 플레이에서의 미확정 실패. 사용자 확인/제보 후 후속 작업(3절6번) |
| 옵션 저장 | 1224 대응은 완료. 공유 삭제 거부 시 이전 파일/changed/복구 파일을 보존하고 실패 안내. 외부 매핑이 남으면 성공 백업/교체 폴더도 남을 수 있음. 966 실제CRT부분전달/버퍼실패·1176/1177상태복구대조완료.물리디스크고장/OS자체반환은호스트미관측 |
| PlayerNoMake | 경고255DWORD/정리 `004328a0` 연결은453 완료. 수신 생성/검증 패킷 부모 `00433c30` writer는 후순위 MP(3절8번). CD `00487900`은 spot 정리이며 같은 함수가 아님 |
| Graph/글꼴/사용자 제보 | 대표 Graph와 캐시 수명은721~724, 비ASCII 글꼴 경계는774, 제보 2건은677/678 완료. 글꼴/플레이 한계는2.5절과3절5~7번을 따름 |

### 2.5 이월한 검사 실패·표시/환경 한계

- **사용자 제보 C2607 해결(1052):** SubscriberGlobalsTests.cpp(40,50)의 전체클래스8BYTE 단언은1011의Handle64BYTE 추가와 불일치했다. 실제 필드/참조 계약으로 교정해 해당 파일 Release컴파일/기존7검사를 통과했다. 다른 테스트 파일/전체 솔루션 통과로 확대하지 않는다.

- **전체 검사 최신 통과 아님:** 938의68집중 대상 중48완전 통과/20실패 관측과 그 뒤의 개별 통과를 구별한다. `guide_release`151개와 `startup_file_names`3개(`names.Index()`)는6차 이전 HEAD `1f5b4177`에서도 같은 실패를 확인했다. `world_generation`은 같은 `CampaignSessionTests`1028의1024×768 건물 스프라이트 비교/장치 미연결 계열이며 기준선 개별 비교는 하지 않았다.
- **분류·원인 조사 잔여:** 당시 `file_tests`5, `pop_context`29, `menu_session`3, `mission_entry`1, `guide_fort_request`1, `renderer_presentation`1 및 `startup_*`/`file_directory`의 경로 고리 실패가 있었다. 후속 개별 단위 통과를 이 이름들의 최신 전체 재검사로 쓰지 않는다. 허용된 집중 검사에서 해당 실패를 다시 만나면 현재 제품으로 원인을 가리고 수정하며, 문서 정리만으로 통과 처리하지 않는다. 전체 CTest 반복은 사용자 실행 빈도 정책을 따른다.
- `FortObjectLoad_ActualFactoryOwnerHpPopAndContent`의 알려진 실패는942에서 필수Pop 관측자/TYPE6 입력을 교정해 통과했다. `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은 일반Battle/MP 이월이며 신규MP 검사 제외다. `CampaignForwardBattle_NaturalTempleAssault`의549 실패는3절6번 사용자/별도 요청 범위다.
- 글꼴/경로의 원문 부호화(`EncodeOriginalText`의1252밖 문자 거부)와 표시용 부호화(`EncodeDisplayText`/`DecodeDisplayText`)를 섞지 않는다. 938에서 혼합 시 설정/유니코드 경로12검사가 깨져 분리 후 해결했다. 612의 스택/미정값·VFX signed WORD와 정상 표본/양수/회전0/원색의 확인을 전체 입력/픽셀 동등으로 확대하지 않는다.
- MakeAi의owner0 이전/삭제·전략 차이, 실제 설치 글꼴/다른ACP/구형무BOM 설정 자동 이관은 별도 한계다([글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md)).
- 덱 검은띠는981에서 해결했다. 별도의 연녹색 직사각형과 스펠 효과 도움말 그림은 사용자 보류이며, 전체 외관/물리 입력/장치 제약은0.3절과3절5·7번을 따른다.

## 3. 앞으로 할 작업 (재개 후 권장 순서)

**진행 지점은8번이다.** 1번929·2번943·3번956·4번980·7번표시결함981을 마감했다. 사용자 /goal로 재개하여987 원격Fort·988 동기화·989 전투섬·990 중앙섬·991 자기 Fort·992 시작 세부 경로·993 시간 동기화를 반영했으며 **일반 State 등 LAN 잔여 → 한국어 후속 → 시나리오UI** 순서로 진행한다. 아래1~4번은 완료 범위/재조사 조건이며,5~7번의 사용자·환경 확인은 단계 전환을 막지 않는다. `[ ]`만 AI 구현 잔여다.

### 1. 대상별 커서·오브젝트 우클릭·게임 UI — 비멀티 범위 완료(929)

83VT의 작성13입구/비멀티81VT의 열기5입구, 건물/특수 대상·운반체8종의 메뉴·현재Graph/동맹/화물·선택/강조/명령, HUD/Chat/Console/모달과 부분 생성 실패 수명을 대조했다. [929 정리](docs/exe/cpp-ui-implementation-closure-20261105.md)가 완료 범위 기준이다. 성공 회수/연구/업그레이드는 실제 송신 부모에 연결돼 있으며 신규 송수신 검증 완료는 아니다.

Dig Up·전환/다리/Meltdown/Detonate·미니맵/Send Money·c2(Test Battle/편집 복귀)/c5(Leave)를 반복 미구현으로 세지 않는다. 활성 caller 없는 관리명령/grid/debug는 기본 UI에 노출하지 않는다. RequestMenu·메타맵2입구·c6/db·원격Player/일반 전투 의식 UI는8번에 남긴다. 새 구체적 비멀티 누락/사용자 제보가 생기면 해당 입력으로 다시 연다.

### 2. Edit 메뉴·커스텀 맵/시나리오 — 원문 범위 완료(943)

현재 편집 도구·저장/재파싱·이름/목록·owner/Graph·LeaveEditMode, TARC/동적Config/HTML 명령과 파일/전환 실패 수명을 [940](docs/exe/cpp-scenario-dynamic-config-20261106.md)·[943](docs/exe/cpp-editor-implementation-closure-20261106.md)에서 정리했다. 영문549문서의 치환980곳은 비멀티11명령967곳/MP·관리10명령11곳/원본오자2곳으로 분류했다. MP/관리 명령은8번이다.

원문 언어/폰트 재생성은[931](docs/exe/cpp-language-closure-20261105.md)에서 정리했다. `languageChanged` 소비자는Chat이며 폰트 재생성은 `ChangeFontFace`다. 명시 `.french` 입력을 프랑스어 UI 완료로 쓰지 않는다. 한국어 확장은6차다.

목록50개 상한은934에서 무제한/50행 스크롤로 바꿨다. Fort433개는 종류2가432/종류0이1개다. TEST01/TEST02 원본은 읽기만 하고 작업본에 저장한다. 10.78 기본 설명의 `{Text}` 리터럴과10.72를 혼동하지 않는다. 기존 수동 시나리오 실행과 **8차 신규 편집 UI/기본english 자동 생성/english 부재 Test Battle 개선**은 구별한다.

### 3. 미션 수명·진행 저장·데모 반복 — 비멀티 범위 완료(956)

Retry/Next Tutorial/Next Mission/Abort·Guide 교체·Client/Session3세대 반납·State1/2/3/단독7·LeaveBattle와cleanup/Tick을 대조했다([945](docs/exe/cpp-mission-abort-replay-20261106.md)·[955](docs/exe/cpp-battle-leave-reason-20261106.md)·[956 정리](docs/exe/cpp-mission-state-scope-20261106.md)). Early1 새진입2+Replay2/Continue/Abort/메인 연속 왕복과 정상종료를 실제 확인했다. 세 데모의 두360초 주기/세 번째 월드도946~948에서 확인했다.

자연 특이State3/직접State2 도달·모든 미션 조합·자연 승리/신규 진행 저장은 사용자 확인이다. 일반 전투 저장/복구·State6/재접속·Zone0/메타Poll·서버 이전/시계 패킷은8번이며 기존 질문/삭제 UI를 유지한다. 단독State7의 복사/로그/World교체/7·8확정과Guide Fort 요청을 다시 미구현으로 세지 않는다.

### 4. 내부 수명·파일·checking 연결 — 활성 비멀티 범위 완료(980)

[980 함수표](docs/exe/cpp-internal-lifetime-closure-20261106.md)는957~979의 공통Pop/현재 파생·Factory/Form/Process·Kernel/SID 회수·외부Kernel 자기소유 반납·new_handler/종료, FILE/색표/Chat/State의 실제 Client/World/Session 연결을 정리한 기준이다. 당시 제품은979이며 현재 제품은0.1절을 따른다.

FILE 직접74호출/23caller를 현재54·MP/폐기인터넷18·폐기CD1·휴면Chat1로 분류했다. 등록snapshot/무진단optional/OS직접mapping/현재FILE의 계약을 구별하며 전체PE 간접호출 부재를 증명한 것은 아니다. 시스템뉴스·일반 전투saveBattle/metamap/재접속FILE은8번이다.

checking5(부모2/부분0/MP3)은8번의 명시 잔여다.1014에서004b8cb0 전체 몸체/수동 입력을 연결했다. 1009에서004a7510 전체 수신/진단을 연결했다.1003에서004b54e0 두 진단/전체 writer를 연결했으며 reader/주기 저장은8번에 남는다. 미등록코드/NULL/범위 밖 포인터·임의외부heap→SID·32비트FILE/FileSpec/SEH·미작성 메모리 자체 일치, 실제OS 고갈/UNC/드라이브CWD/장치실패와CD전체dump/trace/Retype/파생은 호스트/후순위 경계다. 현재 등록된 활성 비멀티 caller의 미연결을 이 경계로 숨기지 않으며 새 근거/제보가 있으면 다시 연다.

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

- [x] **와이드 덱 검은띠(981):** A02최초774행뒤선두검정25행재타일이원인이었다. 확장부본문749행반복으로수정했고새3/기존12검사·실제1920crop검정2000→0/17색을확인했다. 사용자보류연녹색직사각형과다른문제다. [981기록](docs/exe/cpp-wide-deck-background-20261106.md).
- **환경/사용자 확인(이번AI검사 제외):** 물리입력QWASZX/ESC/Home/포커스는762의두timeout 이후 환경변화전반복하지 않는다. 다른모니터/DPI·실제앱전환/장치실패·사용자활성전체화면물리출력·청취·실제CD삽입도사용자/환경대상이다. 앱내입력을물리성공으로세지않는다.
- **PCM 후속 성공 정정:** 594/762는당시미확인이며936의HJOW-Athlon HD Audio에서프로세스WASAPI101653프레임/비무음99726·영상/소리합성을확인했다. 청취/다른장치/실제CD/활성전체화면까지확인한것은아니다. [936기록](docs/exe/cpp-mcp-server-20261106.md).
- 전체화면종료/재실행·세화면비·가장자리스크롤과746의DD조정/복구/저장재시작은기존확인범위를유지한다.979의새종료부모는실제tutorial1진입/대기/WM_CLOSE종료0으로확인했다. 관련변경이있을때필요한회귀만하고전체34/CTest/ZIP을무조건반복하지않는다.
- 표시결함981해소뒤환경/사용자확인만남았으므로Windows11/ARM실기검증없이8번을진행한다. 전체호환성무오류보증이나실기완료로표현하지않는다.

### 8. 멀티플레이·배포·후속 단계

- **1138 최신 잔여:** 메타 ESC의 실제 선택 정리·커서 취소/메뉴 serial 토글과 공통 Game/View/Options 메뉴 작성기를 같은 Session/SP/현재 HUD에 연결했다. H/N/P/R/F4/F5/F6·생산 키·F9 예약/실제 카메라 경로와 일반 F6의 조기 소비 누락도 보완했다. 다음은 메타 Refresh Zones(c6)·나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오UI다. BattleDesc HTML 문자열 고리/CS 주소 구별과 helpWord 캐시 수명도 후속이다. 이전 단위 예고는 이력이며 현재 우선순위는 이 항목을 따른다. 새 MP 검사는 제외한다.

- [ ] **3차 TCP/IP 로컬 멀티플레이·Outpust:** 982 TCP부품/984 공유 등록표·per-ID 복합 전송·실제 단독 Client/World 수명·985 일반 Login/SP·987 원격 Fort/manager/수신 이미지 수명·988 참가자별 동기화·989 전투섬·990 중앙섬·991 자기 Fort·992 시작 세부 경로·993 시간 동기화를 접합·컴파일했다. 1000에서 실제 LAN UI/roster·일반 State7 앞단을 접합했다. 일반 타이머 완료/대기 UI와 로그인 완료 writer는1001~1003에서 연결했으며 주기 writer는1004,파일 reader/복구 일부는1005~1014,일반 자원 프레임은1018에서 연결했다. 전체 일반 전투/메타 등 잔여는 아래 목록을 따른다. 997에서 Multiplayer Options 공장을 연결했다. 998에서 일반 Login 후반/준비 대여를 작성했다. 999에서 LAN 대기실 명단/시작 합의 부품을 작성했다. 1000에서 화면/실제 호출자·World 교체/현재 이름을 연결했다. 1001에서 일반 완료 초기화/팀/간헐천/스펠 예약을 연결했다. 1002에서 대기 명단/시간·강제 시작과 별도 창 serial 수명을 연결했다. 1003에서 일반 완료의 실제 재접속 writer/두 checking을 연결했다. 1004에서 실제 Client 주기 writer를 연결했다. 1005에서 실제 startup 파일 reader/질문 표시를 연결했다. 1006에서 실제00449200 종료 선행 관문과 현재 Player 소비자를 교정했다.1007에서 질문No/DelStateFile 입력을 연결했다.1008에서 서버 이전의 동일 owner Serve backend를 보완했다. 1009에서 실제63h 수신/참가 순서·객체 정리/checking을 연결했다. 1010에서 실제 State15/63h 송신과15→16 확정을 연결했다. 1011에서 현재 handle의 Client 수명을 보완했다. 1012에서 저장 주소 문자열의 실제 backend 연결을 보완했다. 1013에서 복구State7의 실제 pending Fort 선택을 연결했다. 1014에서 실제 서버 후보 선택/두 수동 입력·State15/복구State7 요청을 연결했다. 1015에서 실제 프레임의 자동004b9010/서버 장애 저장과 타이머 위치를 연결했다. 1016에서 비미션 보충의 실제0040e050 배치를 연결했다. 1017에서6ch 주기 지급 검증/실제Message4·18을 연결했다. 1018에서 실제486bc0 전체 자원 프레임/Kernel 뒤 caller를 연결했다. 1019에서 실제70h 원격Tell송수신을연결했다. 00442b50 계산본체는기존ChargeShortfall이며Reject는00442af0임을재대조했다. 1020에서기존ChargeShortfall의실제비용안내/송금·운반caller/6ch환불·2fh취소송수신을연결했다. 1021에서433c30 전체 객체 수신/검증·PlayerNoMake와 실제2ch 수명을 연결했다. 1022에서7ah 압축 decoder/현재 owner 캐시·길이 관찰기를 연결했다. 1023에서4ah/4bh 검증 시작·이전 SID 간격/같은 두 전역을 연결했다. 1024에서2dh 내용물/2eh 삭제와 같은 부착·환불/삭제·SP 추적을 연결했다. 1025에서감시65h/공유 Regular7eh 전체 decoder와 같은 Form/Kernel을 연결했다. 1026에서3ah 총 전체 decoder/수신 생성자·같은 PID/장기 TYPE를 연결했다. 1027에서38h/73h/74h 아비어리 전체 decoder·같은 생성자/PID·장기 TYPE를 연결했다. 1028에서39h 비행 전체 decoder·임시 해석/테마별 재해석·같은 PID/명시 Form을 연결했다. 1029에서3bh/3ch 발사·회전 전체 decoder/기본 생성 공장·같은 PID/명시 Form을 연결했다. 1030에서 애니메이션 writer/현재 헤더·공통 버퍼/전송과 일반 TYPE 별칭을 통합했다. 1031에서 건설37h 실제 수신/송신·진단 복귀/현재 TYPE와 PlayerKnown binding을 연결·교정했다. 1032에서 Path30h 전체 decoder/실제 생성·부착과 송신 길이 observer·현재 boss 조회를 연결·교정했다. 1033에서33h 위치/34h 카메라 목표/35h 섬 정리와 같은 전역/진단을 연결·교정했다. 1034에서32h 현재 설정의 Client 수명과 배치/피해/친군 사격/생산 소비자를 연결·교정했다. 1035에서31h 수신/관찰·핵심16명령/현재quick 플래그·비Login/Editor32h를 연결했다. 1036에서화면 강조와6진단명령을추가해22개를접합했다. 1037에서Subscriber/스캔주기/로그수준을추가해1038에서진단5개추가해1039에서qdd/gp추가해1040에서addLabel/기록writer를추가해33개분기접합이며 1041에서직접입력/송신·cr UI 가정은해소했고1042~1043에서 기록 재생 몸체/실제 부모까지 연결했으며 1044에서 삭제 선택자/공통 송신과 재동기화 부모 잔여를 해소했으며 1045에서2번 종료 통지/선택 활동표와 기존 전체표·64h/timeout UI를 대조했으며 1046에서1번 접속 통지/별도 송신버퍼까지 연결했으며 1047에서 기존 연결 재사용/실제 준비·새 공개/실패 보존과 listener 관문을 연결했으며 1048에서 실제 수락 순서/signed31/준비·진단 반환을 교정했으며 다음은 **1050 신전51h/004dbce0·Tick/event3 연결, 1051 command/event4·ViewMeta저장/State19요청 연결, 1058 UDP/검색34·35·36·38 접합 뒤 남은37/새R서버·응답0Fh/1dh·State28~33/메타·나머지 송신→endpoint/서버·나머지 writer/decoder·메타/Outpost/checking5**이다. 원격 client buildFort는989에서 연결했다. 자기 ordinary localFort는991에서 연결했다. 신규 MP 단위/게임/통신 검사는 하지 않는다. 공식 서버 복구·구 클라이언트 유효성 검사는 연결하지 않는다. **폐기CD확인7f/80(42cd10)은965의검증비활성화정책에따라복원하지않는다.**
- checking 전체 잔여5(1014에서004b8cb0 부모/진단 연결, 완료부모40/부분6): 부모004b02d0의 메타맵 표시0050c9f0/0050cc08·004dc220(event4/Descend), MP분류0042bc10/004b4df0/004b8b20. 기존분류이며직접inSingle관문부재를모든단독도달불가증명으로확대하지않는다. 신규MP검사없이원문구현을이어간다.
- [x] 내용물 통지의 PlayerKnown00595344 binding은1031에서 원문4d55e0/490750을 대조해 현재 `playerState.playerKnown`으로 교정했다. 삭제2eh도 같은 현재 값을 읽으며 별도 alias 잔여는 해소됐다.
- MP 이월 목록: 공통+20 미연결VT0050c9f0(TYPE108/109)/0050cc08(TYPE161), 시스템뉴스의 키/읽기/쓰기·실제 수신UI/설정은1054 연결(검사 제외), endpoint(1055에서State19 뉴스 읽기 호출자 연결), RequestMenu 전송 `004d712e..004d717c`, 메타맵 열기 `004789b0/00479390`·번개 `00479650`·Challenge 이미지/텍스트 `00478d10`/참가자 삭제 `0042bc10`, c6/db·ChalSailPos 모드6·선행클릭/항해 메뉴·desc, 비미션 의식/랭크·조기 희생 안내, PlayerNoMake/TypeWas의433c30은1021 연결, 압축7ah는1022/검증 시작은1023/내용물·삭제는1024 연결, 감시/공유 Regular는1025/총3ah는1026/아비어리3종은1027/비행39h는1028/애니메이션3bh·3ch는1029 연결, 애니메이션 writer/일반 TYPE 별칭은1030 연결, 건설37h의 실제 수신/송신·진단 복귀/현재 TYPE와 PlayerKnown은1031 연결·교정, Path30h는1032/공간33h·34h·35h는1033 연결, 현재32h 설정/소비자는1034 연결, 31h 수신/관찰·핵심16명령/비Login32h는1035 연결, 화면 강조·6진단명령은1036 연결(총22개), 1037에서설정3명령추가(총25개), 1038에서진단5명령추가(총30개), 1039에서qdd/gp추가(총32개), 1040에서addLabel/기록writer추가(33개분기접합), 1041에서직접입력/송신·cr UI 가정해소, 1042의 판독/라벨·Run에 이어1043에서 실제Client/Kernel/홈reader·입력/Confirm부모 연결, 나머지 수신/전송 감사, 1045에서 활동frame22 전체/선택 정리와 기존1015의64h·timeout/UI 재대조, 1046에서1번004e2f00/별도22송신버퍼 연결, 1047에서 기존 연결 재사용/실제 준비·직접 공개/실패 보존과 listener 관문 연결, 1048에서 실제 수락 순서/signed31/준비·진단 반환 교정, 1050 실제 신전51h/004dbce0·Tick/event3 연결, 1051 command/event4·ViewMeta저장/State19요청 연결, 1058 UDP/검색34·35·36·38 접합 뒤 남은37/새R서버·응답0Fh/1dh·State28~33/메타·전체 송신·endpoint/server/boss 감사, 1044에서 Activate의 별도 수신자/삭제 선택과 재동기화 허용/owner 캐시 부모 잔여 해소(988 연결 재대조), 나머지 프로세스 writer/decoder 감사, 섬·표면 TypeWas의 MP 수신 writer는1021에서 연결(861 잔여 해소).
- **1094에서1dh/State28~33의원문본체·CS master/캐시감시를라이브러리컴파일하고Root요청실제목적지인자/별도005c85c4 owner·직접read/write를복원했다. 최신잔여는이항목이우선한다.** 실제1dh등록/28~33분배는아직미적용이다. 다음은0047dd60/479760/47cab0메타표시/Poll·현재CS수신/가상표·같은오브젝트/카메라/HTMLserial·004b2c00/master통지·004b62b0/Login/실제동일전송장치를완성한뒤MetaConnectionRuntime의1dh등록과28~33분배를연결한다. 빈callback/가짜ACK/가짜화면으로우회하지않는다. 세.server콘텐츠/배포·다른LAN hostname/기존Config registryValue경계·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를이어간다. 새C++/MP/게임/통신실행0이며전체34/ZIP을반복하지않는다. 현재IPv4 TCP/UDP/IPX·SPX명시미지원·폐기7f/80/PIRATE quota비활성과원문에없는원격포트Open/handler등록금지를유지한다.
- **1093에서Root0Fh 실제수신/관찰기·같은Client 메타기록/State·현재서버목록 갱신·사용자Saves 저장/다음시작 우선읽기를연결했다. 최신잔여는이항목이우선한다.** 원문47BYTE 존/친구3BYTE 위치를보존하고제공R10.71·정확한3BYTE길이/물리에만별도compact호환을적용하며없는이름/+84값은정리결과를유지한다. 원문47BYTE 친구위치이상은고쳤다고주장하지않는다. 다음은1dh수신·State28..33/CS·실제메타표시와나머지수신이다. 세.server콘텐츠/배포·다른LAN hostname/기존Config registryValue경계의R필요경로대조/접합·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를이어간다. 새C++/MP/게임/통신실행0이며전체34/ZIP을반복하지않는다. 현재IPv4 TCP/UDP/IPX·SPX명시미지원·폐기7f/80/PIRATE quota비활성과원문에없는원격포트Open/handler등록금지를유지한다.
- **1092에서클라이언트37의실제새R Windows실행/동일자료CWD·부모와분리된콘솔/핸들장치와배포동봉·한ZIP의바이트검증을연결했다. 최신잔여는이항목이우선한다.** 여섯검색/34·38/lan0부모는유지하고현대분리배치의클론모듈폴더/11.01 R식별을사용한다. 새R의500사전통지를준비ACK로만들지않으며실제OS실행/게임·서버/MP·통신검사는0이다. 다음은제공R3BYTE존과클라이언트0F47BYTE차이·0F/1d수신/메타실제연결을우선대조한다. 세.server콘텐츠/배포·다른LAN hostname/기존Config registryValue경계의R필요경로대조/접합·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를이어간다. 기존동일Server owner/진단·파일·주소·라우팅/CCh/전송·메인수명과현재IPv4 TCP/UDP/IPX·SPX명시미지원·폐기7f/80/PIRATE quota비활성을유지한다. 배포ZIP은실제새R/런타임합집합을동봉했지만원본R실행/원문에없는원격포트Open·handler등록은추가하지않는다. 패키지변경없는단위마다ZIP/전체34검사를반복하지않는다.
- **1091에서같은최종Server owner/모든실제모듈·수명/메인과 새R.exe entry의Run호출·CMake빌드를연결했다. 최신잔여는 이 항목이 우선한다.** NetstormCpp가새NetstormRoot/R.exe도빌드하며19핵심링크심볼·Release인라인256정리·AMD64/CUI/11.01·UTF-8/asInvoker를정적대조했다. 실제새R/서버·MP/통신실행은0이다. 다음은클라이언트 NativeLocalServerLaunch의 새R.exe 실제OSlaunch/배포이며 원본R을대신실행/복사하면안된다. 실제500사전통지를준비성공ACK로해석하지않는다. 원문에없는원격포트Open/handler등록은추가하지않는다. 세.server콘텐츠/배포·LAN hostname/클라이언트0F존47/서버3BYTE·0F/1d/메타와기존Config registryValue미연결계약의R필요경로대조/접합·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를이어간다. 현재실제IPv4 TCP/UDP/IPX·SPX명시미지원과폐기검증/PIRATE quota비활성·신규MP검사제외를유지한다.
- **1090에서최종R owner가사용할 실제ProcessTrace 로그/OS시각·사용자홈HTML/호스트스택·복귀형ZAssertion을 작성했다. 최신잔여는 이 항목이 우선한다.** 원문485cf0/85e42에서throwOnAssert옵션을소비하지않으므로정상진단을일률적throw로대체하지않는다. 다음은새R.exe최종owner/entry·CMake R.exe대상과 실제LogDevice/AssertionDevice·Lifecycle.Bind/MainLoop.Run·클라이언트OSlaunch/배포다. ProcessTrace에는같은Configuration.Values() 참조를넘긴다. Configuration생성은IO가없으므로필요한forward capture는실행전같은장치구성을보장하고별도Options사본/빈fallback장치를만들지않는다. 같은Configuration/Clock/Random/기록·파일/목록·주소/RootControl·Routing/BindSchedule/CommandPages.Bind·SharedFiles.Bind·LoginRequest/CCh/전송과LifecycleModules/State를장기owner에묶는다. 원문에없는원격포트Open/handler등록은추가하지않는다. IPv4 TCP/UDP범위/IPX·SPX명시미지원·세.server콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를이어간다. 폐기검증/PIRATE quota비활성과신규MP검사제외를유지한다.
- **1089에서477e10시작/478af0정리·실제노드생성/번호공개/같은메인hook을 연결했다. 최신잔여는 이 항목이 우선한다.** 다음은새R.exe최종owner/entry·CMake R.exe대상과 실제Lifecycle.Bind/MainLoop.Run·클라이언트OSlaunch/배포다. 같은 장기 Configuration/Clock/Random/기록·파일/목록·주소/RootControl·Routing/BindSchedule/CommandPages.Bind·SharedFiles.Bind·LoginRequest/CCh/전송을 프로세스 환경/VFS/로그·진단과 묶는다. LifecycleModules/LifecycleState를 같은 owner에 만들고 실제Bind를 메인생성전에 호출한다. 원문에없는 원격포트Open/handler등록은 하지 않는다. Start의IPX/SPX주소선택은보존하지만 실제장치는IPv4 TCP/UDP이며 선택시명시미지원이다. 세.server콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를 이어간다. 폐기검증/PIRATE quota비활성과 신규MP검사 제외를 유지한다.
- **1088에서 R전역 패킷 초기화/17수신·102관찰 등록과 공통 동적32몸체/38필드를 연결했다. 최신잔여는 이 항목이 우선한다.** ResetCallbacks는 같은 표/길이를 유지하고 Session대여·콜백/전송실행 중 초기화를 거부한다. NULL9·빈함수61·83버전/관찰기0·폐기7f/80비활성을 유지한다. 다음은477e10시작/478af0정리·새R.exe최종owner/entry에서 실제UDP번호발행/Announce/InitializePackets/Bind/Run이다. 원문에없는 원격포트Open/handler등록은 하지 않는다. 같은 장기수명의 Configuration/Clock/Random/기록·파일/목록·주소·Routing/BindSchedule/CommandPages.Bind·SharedFiles.Bind·CCh/전송을 묶는다. 세.server콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타/OSlaunch·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를 이어간다. 실제IPX미지원·폐기검증/PIRATE quota비활성과 신규MP검사 제외를 유지한다.
- **1087에서 Root보조객체의 실제원격명령4세션/집합·Poll/정리·OS와 메인hook을 연결했다. 최신잔여는 이 항목이 우선한다.** 479bb0은생성자가아닌활성조회이고실제기본생성은7ee90다. 기본전역R의14참조/477e10에는Open/handler등록이없으므로최종owner에서 rootTelnetPort만보고포트를열거나새관리callback을추가하면안된다. 기본 RemoteCommandServer를같은장기수명으로만들고 실제Bind/Shutdown을연결한다. 다음은전체전역패킷/477e10초기화·478af0정리·새R.exe최종owner/entry·실제UDP번호발행/Announce/Bind/Install/Run이다. 기존 Configuration/Clock/Random/기록·파일/목록·주소·Routing/BindSchedule/CommandPages.Bind·SharedFiles.Bind·CCh/전송을같은수명에묶는다. 원문18수신중17몸체작성/컴파일·폐기80비활성은완료범위다. 세.server콘텐츠/배포·LAN hostname/0F·1d/메타/OSlaunch·LAN/Outpost/checking5를이어간다. 실제IPX 미지원/폐기검증·PIRATE quota 비활성과 신규MP검사 제외를 유지한다.
- **1086에서87h 전체를 현재 실제 CCh/파일·전송/등록에 연결했다. 최신 잔여는 이 항목이 우선한다.** 원문18수신중17몸체 작성/컴파일 완료·폐기80비활성으로 수신몸체 잔여0이다. 최종R서버/실제호출 완료가 아니다. 다음은004d23f8의 보조객체/집합·Poll(479bb0/47f2b0/47f510/47f140 등)과 전체전역패킷/477e10초기화·478af0정리·새R.exe 최종owner/entry다. 기존 실제 Configuration/Clock/Random/기록·파일/목록·주소·Routing/BindSchedule/CommandPages.Bind·SharedFiles.Bind·CCh/전송을 같은 수명에서 연결하고 실제UDP번호발행/Announce/Install/Run을 호출해야 한다. 세 .server 콘텐츠/배포·LAN hostname/0F존47·3BYTE/0F·1d/메타/OSlaunch·나머지LAN/Outpost/checking5도 이어간다. 실제IPX 미지원/폐기검증·PIRATE quota 비활성과 신규MP검사 제외를 유지한다.
- **1085에서 현재 길드/요청 변경과 실제 저장·같은 CCh owner를 연결했다. 최신 잔여는 이 항목이 우선한다.** SharedFiles.Tables/GuildFor/GuildIndex/생성·변경·로그인/가입요청·저장 부품이 준비됐다. Guild.id/contact와 요청contact/value는 recruiting/password·playerName/approved로 의미를 바로잡았다. 다음은 기존 CommandPages/SharedFiles를 사용한 길드 목록HTML·87h 전체 부모와 관리자6분기/명단·제재변경·관찰기/실제등록이다. 원문18중16구현·폐기80제외·87h 미등록 상태다. 모집/로그인·빈리터럴 포인터/0번누락 제거를 원문대로 유지하며 승인만으로 회원추가를 합성하지 않는다. 세 .server 템플릿 콘텐츠/배포·보조객체/전체패킷·477e10초기화/478af0정리·새R.exe 최종owner/entry·실제UDP번호발행/Announce/Bind/Install/Run·LAN hostname/0F·1d/메타/OSlaunch를 이어간다. 실제IPX 미지원/폐기검증·PIRATE quota 비활성과 신규MP검사 제외를 유지한다.
- **1084에서87h 명령의 실제 페이지 부품을 연결했다. 최신 잔여는 이 항목이 우선한다.** 다음은 길드 상태/회원·가입요청 변경·저장/목록 HTML과 관리자6분기·명단을 복원해87h 전체 부모/관찰기·실제 등록에 연결하는 작업이다. 원문15상위명령/관리자6개와 비연속8555BYTE를 분류했지만 87h는 아직 미등록이다. 원문18중16구현·폐기80제외 상태를 유지한다. CommandPages.Bind는 같은 장기파일/현재51존과 실제 OS시각을 쓰며, 최종 owner에서 실제 연결해야 한다. 현재 세 .server 템플릿은 원본/CD에 없어 콘텐츠/최종배포 확보가 남는다. Guild.id로 이름 붙인 첫DWORD는 원문 모집 ON/OFF 상태로 쓰이므로 길드 구현 때 의미/이름을 고친다. 이후 보조객체/전체패킷·477e10초기화/478af0정리·새R.exe 최종owner/entry·실제UDP번호발행/Announce/Bind/Install/Run·LAN hostname/0F·1d/메타/OSlaunch를 이어간다. 실제IPX 미지원/폐기검증·PIRATE quota 비활성과 신규MP검사 제외를 유지한다.
- **1083에서11/12h 실제 진단 수신과 역방향LAN TCP/번호·시각 부모를 연결했다. 최신 잔여는 이 항목이 우선한다.** 원문등록18중16구현·폐기80비활성유지·나머지는87h 하나다. 다음은87명령 전체몸체·보조객체/전체패킷등록·477e10초기화/478af0정리·새R.exe최종owner/entry·실제호출이다. RootControl 생성에는 같은 장기Clock/실제로그가 추가로 필요하며 Connect는 현재IPv4/client TCP 범위다. 기존 UDP번호발행/Announce·Routing.Bind/BindSchedule/AddressCodec.Bind·Clock/Random/Config/표/파일/기록/전송·0F/1d/메타/OSlaunch를 이어 연결한다. 실제IPX미지원/폐기검증·PIRATE quota 비활성과 신규MP검사 제외를 유지한다.
- **1082에서0b/0c/66h 실제수신·LAN재공지/하위pending guard를연결했다. 최신잔여는이항목이우선한다.** 원문등록18중14구현·폐기80비활성유지·나머지3수신(11/12/87)이다. 다음은진단연결테스트11/12·87명령·보조객체/전체패킷등록·477e10초기화/478af0정리·새R.exe최종owner·실제호출이다. 시작부모는udpBroadcastId/ipxBroadcastId초기화와실제OpenDatagram반환일반번호발행후RootControl.Announce를호출해야한다. 현재IPX통신미지원을가짜성공으로대체하지않는다. 같은Routing.Bind/BindSchedule/AddressCodec.Bind·Clock/Random·Config/표/파일/기록/전송·0F/1d/메타/OSlaunch도유지하며신규MP검사는제외한다.
- **1081에서83h 관리명령/현재예약·하위socket/Root일반·확장안내·차단을연결했다. 최신잔여는이항목이우선한다.** 원문등록18중11구현·폐기80비활성유지·나머지6수신(0b/0c/11/12/66/87)이다. 다음은87명령·진단/LAN수신·보조객체/전체477e10/478af0·새R.exe최종owner·실제Bind/호출이다. Routing.Bind/BindSchedule와AddressCodec.Bind를CCh생성전에같은장기상태/장치로연결한다. Clock/Random·Config/표/파일/주소/기록·전송·0F/1d/메타/OSlaunch도유지한다. 폐기검증/PIRATE quota는비활성유지하며신규MP검사는제외한다.
- **1080에서1fh 준비/실제StartBattle·450240/4503b0 재배치를 연결했다. 최신 잔여는 이 항목이 우선한다.** 원문등록18중10구현·폐기80비활성유지·나머지7수신(0b/0c/11/12/66/83/87)이다. 다음은83관리명령의예약종료/Root문구장치와87명령·진단/LAN수신·보조객체/전체477e10/478af0·새R.exe최종owner·실제호출이다. AddressCodec.Bind의CCh localAddress도부모생성전에같은장치로연결한다. 같은Clock/Random·Config/표/파일/주소/기록·전송·0F/1d/메타/OSlaunch도유지한다. 폐기PIRATE quota는기존기록전용정책대로비활성유지하며신규MP검사는제외한다.
- **1079에서1eh 전체 이동 수신을 연결했다. 최신 잔여는 이 항목이 우선한다.** 원문등록18중9구현·폐기80비활성유지·나머지8수신(0b/0c/11/12/1f/66/83/87)이다. 다음은1f준비/83관리명령의실제4570c0전투시작·450240/4503b0재배치/제거·예약종료/Root문구장치다. 몸체없이빈callback으로수신을등록하지않는다. 이후87/진단·LAN수신·보조객체와전체477e10/478af0/새R.exe최종owner·실제호출이다. InstallPacketObservers에현재1e도등록됐으며같은CCh/입력·공유scratch를유지한다. 같은Clock/Random·Config/표/파일/주소/기록·전송·0F/1d/메타/OSlaunch도연결해야하며신규MP검사는제외한다.
- **1078에서10h/67h/68h/71h 전체 수신을 연결했다. 최신 잔여는 이 항목이 우선한다.** 원문등록18중10/16/17/1c/41/67/68/71구현·폐기80비활성유지·나머지9수신이다. 다음은83/1e/1f/87의CCh부모,0b/0c/11/12/66진단·LAN수신·보조객체와전체477e10/478af0/새R.exe최종owner·실제호출이다. 기존InstallPacketObservers가다섯CCh수신을등록하며원래mutable버퍼를필수로한다. 같은Clock/Random·Config/표/파일/주소/기록·전송을연결하고0F/1d·메타/OSlaunch를계속한다. 신규MP검사는제외한다.
- **1077에서 실제41h 채팅전체수신을연결했다. 최신잔여는이항목이우선한다.** 원문등록18중16/17/1c/41구현·폐기80비활성유지·나머지13수신이다. 다음은67/68/71전투옵션·조회/설명,10/83/1e/1f/87등CCh부모,0b/0c/11/12/66진단·LAN수신·보조객체와전체477e10/478af0/새R.exe최종owner·실제호출이다. 기존InstallPacketObservers가41수신도등록하며원래mutable버퍼를필수로한다. 같은Clock/Random·Config/표/파일/주소/기록·전송을연결하고0F/1d·메타/OSlaunch를계속한다. 신규MP검사는제외한다.
- **1076에서 실제 rootservers.dat 검색·문자열 표/체크섬/행갱신·사용자 홈 저장과 같은 메인 감시 hook을 연결했다.** 다음은 보조객체/집합·Poll·나머지Root/CS·41h 수신·전체477e10초기화/478af0정리·새R.exe 최종owner/entry·실제Run호출이다. ServerList에Options.rootServersChecksum의같은참조/RawFileName·현재파일장치를넘기고초기Reload/메인Bind를실제호출해야한다.현재Clock/Random·설정/표·뉴스/주소/기록·라우팅을같은owner에묶는다.다른LAN hostname/0F·1d수신·메타/OS launch는후속이며MP검사는제외한다.
- **1075에서 실제 메인 cycle/중복CCh Tick·파일 감시·예약리셋/종료·Root41h 안내를 같은 현재 시계/CCh/전송에 연결했다.** 다음은471ec0/471710의rootservers.dat 실제 검색·목록/체크섬,보조객체/집합·Poll·나머지Root/CS·41h 수신·전체477e10초기화/478af0정리·새R.exe 최종owner/entry·실제Run호출이다. 감시중간471f20은Config재로드가아니라는정정을따른다. 현재Clock/Random·설정/표·뉴스/파일/주소/기록·라우팅을같은owner에묶는다.다른LAN hostname/0F·1d수신·메타/OS launch는후속이며MP검사는제외한다.
- **1074에서 실제 CCh 주기/전투 재생성·가상 참가자·배치/R process 난수·현재 시계/기존 명단 전송을 연결했다.** 다음은 메인루프의 현재CCh 최소대기 선택/조건부 중복Tick,파일watcher·설정 갱신/예약리셋·종료·보조객체/나머지 Root·CS 수신·새R.exe 전체초기화/최종owner·실제호출이다. 같은 ClockState/Random을 CCh 생성자에 넘기고 기존 Config/표·파일/주소/시계/기록·전송을 유지한다. 다른 LAN hostname/0F·1d 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1073에서 R FullTime/중첩 정지·현재 프레임 통계와 실제 사용자 홈 기록·Config/전송 hook을 작성했다.** 다음은 보조 객체·나머지 Root/CS 수신/입력·실제 CCh주기/메인루프·리셋/종료·새R.exe 전체 초기화/최종 owner·실제 호출이다. Clock.Bind/PacketRecord.Bind와 기존 주소/파일 Bind를 각 부모 생성 전에 적용하고 같은 상태·뉴스·표/파일 watcher·현재 시계를 유지한다. 다른 LAN hostname의 외부 DNS 없는 해석/0F·1d 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1072에서 Root 주소 문법/5칸 서식·실제 DNS 없는 로컬 IPv4와 같은 로그인/CCh hook을 작성했다.** 다음은 시계/기록/보조 객체·나머지 Root/CS 수신/입력·새R.exe 전체 초기화/최종 owner·실제 호출이다. AddressCodec.Bind/SharedFiles.Bind를 각 부모 생성 전에 적용하고 Config/CCh/Router/현재 표·뉴스·파일/DataPrefix와 watcher 주기를 같은 owner에 묶는다. 다른 LAN hostname의 외부 DNS 없는 실제 해석은 후속이며 현재 LAN은 숫자 IPv4를 사용한다. 공개 DNS는 복구하지 않는다.0F/1d 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1071에서 실제 공유 blacklist/marked/IP/filter·guild/guildreq/news 로더·mtime 감지와 사용자 홈 blacklist 저장/Config·CCh hook을 작성했다.** 다음은 주소 서식/해석·시계/기록/보조 객체·나머지 Root/CS 수신/입력·새R.exe 전체 초기화/최종 owner·실제 호출이다. SharedFiles.Bind를 Config/CCh 생성 전에 적용하고 같은 ChallengeMessages/표·파일 장치를 유지하며 DataPrefix 확정/주기 watcher도 원문 위치에 묶어야 한다. Routing/로그인·claim/종료 hook과 함께 최종 owner에 묶는다. 0F/1d 클라이언트 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1070에서 Root00474b00 현재 receiver/NULL 직접 호출과17h 원격 요약·공유 참가자 생성자를 연결했다.** 다음은 공유 blacklist/marked/IP/filter·guild/guildreq/news 로더와 사용자 홈 저장, 주소 서식/해석·시계/기록/보조 객체·나머지 Root/CS 수신/입력·새R.exe 전체 초기화/최종 owner·실제 호출이다. Routing.Install/Relay와 ChallengeRuntime relayRoot·LoginRequest login/claim·종료hook을 같은 최종 owner에 묶어야 한다. 0F/1d 클라이언트 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1069에서 실제 CCh0044e940 로그인/0044e0d0 종료 전체 부모·참가자/전투 수명/명단·주소·메시지·길드 이름을 같은 저장소/전송에 연결했다.** 다음은 공유 blacklist/marked/IP/filter·guild/guildreq/news 로더/사용자 홈 저장, Root 라우터·주소 서식/해석·나머지 Root/CS 수신/입력·시계/기록/보조 객체·새R.exe 전체 초기화/호출이다. LoginRequest의실제login/소켓claim·종료hook과이parent를최종Rowner에묶어야한다. 0F/1d 클라이언트 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1068에서 Root1Ch 관문/현재 CCh 선택·대기 소켓 이전/64비트 핸들·WSA/새 stream 구성 API를 연결했다.** 다음은 CCh0044e940의 실제 참가자 준비/데이터·목록·응답과0044e0d0 종료, 나머지 Root/CS·localAddress/시계·기록/주소 formatter·moderation/보조 객체·새R.exe 전체 초기화/호출이다. 실제 CCh가 내부 소켓 토큰을 소비하도록 연결해야 한다. 0F/1d 클라이언트 수신·메타/OS launch와 사용자 홈 장치는 후속이며 MP검사는 제외한다.
- **1067에서 실제 R CCh/최초 좌표·단일 프로세스 생성/소유 토큰과 현재40개 수신·flush·명시 정리를 연결했다.** 다음은 Root/CS 나머지 수신·참가자 로그인/종료·시계/기록/주소 formatter·moderation/보조 객체·새R.exe 전체 초기화/정리와 실제 호출이다. RootDirectory/Network도 이 실제 저장소를 빌리도록 최종 부모에 연결해야 한다. 사용자 홈/0F·1d 수신·메타/OS launch는 후속이며 MP검사는 제외한다.
- **1066에서 R 시작 설정/재해석·기본값/문자열과 같은 Config/현재 통신·Root 옵션 참조를 연결했다.** 다음은 CCh 생성·관리/Root·CS나머지·moderation/기록·시계/보조 객체·새R.exe 실제 초기화/호출이다. 설정 사용자 홈 읽기/쓰기와0F/1d 수신·메타/OS launch도 실제 장치에 연결해야 한다. 빈 moderation/기록 callback이나 원본 R 실행으로 대신하지 않으며 MP검사는 제외한다.
- **1065에서 Root16h→0Fh 응답/추천·현재 참가자/친구·관찰기와 실제 전송·pending guard 접합 부품을 작성했다.** 다음은 실제 R 초기화/옵션·CCh 생성/관리·Root/CS나머지·시계/기록/보조 객체·새R.exe/호출이다. 제공 R의 존3BYTE/클라이언트47BYTE 차이와0F/1d 수신·메타/OS launch는 후속이다. 과거17h 응답 표기는0Fh 오류였으며17h는 별도 변경 통지다. MP검사는 제외한다.
- **1064에서 실제 R 전송 노드/UDP512·listener8·accepted4096·부분 수신/집합·부분 송신/번호·깊이 수명과 실제 SocketDevice/NetworkPump를 연결했다.** 다음은 새R.exe/실제 초기화·옵션·Root/CS 수신·보조 객체·시계/기록/formatter와 실행 호출이다. 지연 호출 큐/세부 packet trace/응답·메타는 남으며 원본 R 실행으로 대신하지 않는다. MP검사는 제외한다.
- **1063에서 실제 R IPv4 WinSock 장치와 공통 프레임 분배/Client 접합을 작성했다.** 다음은 R transceiver512/4096 버퍼/부분 프레임·집합 송신/숫자 토큰/깊이 반납과 실제 SocketDevice/NetworkPump 연결이다. 새R.exe/Root·CS/응답은 남으며 원본 R 실행으로 대신하지 않는다. MP검사는 제외한다.
- **1062에서 R00476b80의256/32/51×40 통신 부모/64칸 집합·timeout/현재 순회/유휴 종료를 컴파일했다.** 다음은 실제 R 소켓/프레임·패킷 분배/보조 객체·Root/CS 관리자/실행 대상과 부모 호출 연결이다. 클라이언트22칸 장치나 원본 R 실행으로 대신하지 않으며 MP검사는 제외한다.
- **1061에서1060의 비Session 등록 동기화 장기 writer/Notify·실제 파일 저장을 연결했다.** 도구 일반 소비자는 AccountSetup/setup.exe register이며 폐기된 외부 등록 실행을 복구하지 않는다. 다음은 새R.exe/OS실행·실제256칸 전송/Root·CS/0Fh·1dh·수동 검색/메타 나머지다. 전체435220/MP 동작 완료를 뜻하지 않으며 MP검사는 제외한다.
- **1059에서37의설정/여섯파일탐색부모와R전용번호·종료·모드부품을추가했다.** 다음은새R.exe/OS실행장치·실제256칸전송/Root·CS/NETSTORMOOGABOOGA등록메시지다. 제공R응답10.71과CS서버물리를클라이언트10.78과섞지않는다. 004b9230수동UI/0Fh·1dh/메타·선택적record는남으며MP검사는제외한다.
- **1058에서실제UDP12/공유표·Client검색전역/CRT/34·35·36·38/66h동기Run을연결했다.** 다음은37/새R서버실행·Root/CS관리자/004b9230수동선택·재시도UI→0Fh/1dh응답→28~33/메타다. 선택적packet-record writer도남는다. 원본R실행으로새서버복원을대체하지않으며MP검사는계속제외한다.
- **1050은51h/004dc050·004dbce0·Tick/event3,1051은004dbac0 명령/실제 확인WORD·event4의fort territory0046f650/0046f5a0·curServer0054db4c→ViewMeta02000013→004b3760/004b3620/State19 요청을 연결했다.** 이름표의Descend는030000a7이며 기존 removeAnimation/regionGeneration은 독립 입력 호환명이다. **1053~1058에서CS/State19·20~27/endpoint·TCP/UDP·검색34·35·36·38을 접합했으나37/새R서버·로그인 응답0Fh/1dh·State28~33/메타 입력·표시는 미연결**이며 checking5를 그대로 유지한다. 직접inSingle 관문이 없으므로 단독 전체 미도달 증명으로 확대하지 않는다.
- 일반 전투 `!state.dat` 저장/복구·saveBattle/metamap/비mission phase14·StateReconnect/서버 이전`004b8cb0`(ForceServerDeath/StateReconnect·자동 장애 caller,831)·Zone0/메타 Poll·1003의`004b54e0` writer와1004 주기 caller/1005 파일 reader·1009의`004a7510` 수신과1010 State15/63h 송신은 연결됐다. 나머지 `0044fa00` 소비자·LeaveBattle 원격 페널티(004cbab0/004b4ac0/004b4b40/004bfb50)와State6의일반Fort/결과복귀를 포함한다(661/955의 분류). 기존 질문/삭제 UI는 유지한다. 시계 패킷/Commander는993에서 연결했다. 정적 사전 구현과 검증 완료를 구별한다.
- 일반전투 State7의 최소/최대밀도 옵션getter·게임난수/강제밀도bb4 writer는992, 중앙섬metadata 수신/송신 observer는990에서 연결했다. 일반 호스트 State7 진입/World 교체는1000에서 연결했고 전체MP부모는잔여다.894는실제생성root/장기owner의코드연결이며이writer나MP동작검증완료를뜻하지않는다.
- [x] **4차 로컬 MCP·와이드 완료(935~937), 덱 검은띠981 해결.** 제공 기능/검증과 물리 확인 경계는1.1절·3절7번을 따른다.
- [x] **5차 사용자 저장 경로·무제한 목록 완료(932~934).** 실제 이관/reader와 읽기 전용 설치·Unicode 실행을 확인했다. 사용자 확인: 작업 루트가 `CustomMaps`라 실제 맵은 `CustomMaps\d\NAME.fort`이고 첫 Edit 목록 때 원본fort433개를 복사한다. 스크롤은 이동 행 클릭 방식이며 마우스 휠은 미구현, 이동 행 문구는 영어 고정이다. 배치/복사 방식은 사용자 결정 후 조정하고 한국어 문구는6차에서 처리한다.
- [ ] **6차 한국어(938 1차 완료: 글꼴·번역표 502항목·Language 메뉴·`.korean`→`.english` 대체·배포 동봉. 남은 것: 미션 브리핑/팁/도움말 `.korean` 본문(AI/SP 구성은 english에서 복사), 한글 메인 버튼 그림, 번역 검수, 한글 입력):** 메뉴/문자열/시나리오·글꼴을 지원한다. 글꼴은 D2Coding이며 명칭은 AGENTS.md를 따른다. **986후보는 미적용이다.** `korean/FINAL_PLAN.md`의 최종 코드41파일과 별도44자산을 검토·병합하고 허용된 빌드/집중 검사로 확인한다. 표시 부호화/IME·클립보드 경계를 시나리오 통합 전에 완료한다.
- [x] **7차 AA/FSR1.0 구현 완료(939).** 기본off·1440p/4K 자동FSR/논리1080p는1.1절 기준이다. 미션/활성 전체화면/실제4K/입력 역변환은 사용자 확인이며 성능 최적화는 후속이다.
- [ ] **8차 Edit 시나리오 UI:** 초기 자본/테크·적·동맹 등을 별도 편집 UI로 수정하고 새 맵의 기본 english 자동 생성·english 부재 Test Battle 종료를 개선한다. 기존 수동 대본·저장/재적재를 보존한다. **986 통합 후보는 미적용이다.** 한국어 통합 뒤 `scenario/PLAN.md`·`CMake-additions.txt`를 따라 모델5파일/코드를 병합·등록하고 실제 사용자 홈 저장/Editor/Test Battle 진입을 확인한다. 0.4절의 준비 검사를 실제 통과로 세지 않는다.
- 배포 스크립트 자체는 완료다. 배포할 제품이 정해지면 README/BUILD_INFO를 갱신하고 그 제품의 ZIP·압축 해제 실행을 확인한다. 마지막 전체34경로와 묶인 검증 ZIP은644 제품이며 이후 제품의 전체 배포 회귀로 쓰지 않는다. 938에서는 별도1379항목 ZIP의 한국어 실행만 추가 확인했다. 현재1056의 ZIP 검증은 없다.
- 60/120프레임·CD10.72 전체 복원/Windows 10 구동 불가 해결책은 별도 후순위다. Linux는dotnetpj 후순위이며cpppj는Windows 대상이다.

## 4. 운영 규칙과 유효한 사용자 결정

- **최신 진행 지시(2026-11-08 /goal):** cpppj의 이 문서상 AI 작업 완료까지 계속한다. 기존 일시중지는 해제됐다. 사제 포획/희생·특정 적 공격·승리 재현과 사용자 직접 확인 항목·멀티플레이 테스트를 제외한다. 단순 진입/대기/유닛 생산/건설로 끝나는 필요한 검증은 진행한다. 단위마다 인계/로컬 커밋하며 커밋 실패 때도 작업을 계속한다.
- **단계 전환(2026-10-31):** 모든 자연 완주·전체 동일성은 사용자 확인이며 AI 잔여가 이것뿐이면 다음 단계로 간다. Windows 10 확인 뒤 Windows 11 실기 없이 넘어간다. 사용자가 오류/차이를 제보하면 해당 건을 AI 작업으로 올린다.
- **실행 빈도(2026-10-31):** 더 많은 기능 묶음 뒤 필요한 통합 확인을 한다. 소수 단위마다 전체 34·긴 Early 완주·전체 CTest/ZIP을 반복하지 않는다. 옛 3~5단위 기준은 자동 실행 조건이 아니다. 새 실제 실행은 변경과 직접 관련된 필요성을 먼저 판단한다.
- **검증 기록:** 정적 대조·명시 입력·컴파일·C++ 검사·GUI/물리 관측을 구별한다. 최초 실패/교정 원시를 보존하고 기대값을 제품에 맞춰 약화하지 않는다. 다른 제품의 성공·준비본을 현재 제품 통과 수로 합산하지 않는다. GUI/분석기 실행 720의 범위는 [720 기록](docs/exe/cpp-editor-graph-native-20261102.md)을 따른다.
- **CHEAT/테스트 조작:** CHEAT은 Edit 편집에서만 각 원문 조건으로 노출한다. `--test-bridge-patterns`·명시 로컬 AI/SP 등은 검사 조작임을 주석/기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다.
- **실행 파일/자료:** Windows 실행 파일은 `Netstorm.exe`다. CMake 대상명 NetstormCpp·OUTPUT_NAME Netstorm·ARCHIVE_OUTPUT_NAME NetstormCpp를 유지한다. 옛 `NetstormCpp.exe` 로그는 당시 이름으로 보존하고 원본/클론 프로세스는 전체 경로로 구별한다. 무인자 GUI·`--help/--run`·분석 CLI를 유지하며 EXE 옆 `game/netstorm.tarc`를 우선한다. cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다.
- **보호:** AGENTS.md·originals/originalCD 기존 파일은 수정/삭제하지 않는다(AGENTS의 명시 예외만 허용). 게임 실행 전후 원본 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild18.x를 쓴다. `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`로 빌드하며, 선행 CMake/MSBuild 자식이 끝난 뒤 한 번에 하나만 실행한다. 단일 빌드 핸들을 추적하고 빌드 중 소스를 바꾸지 않는다. exit_code0 확인 후 의존 검사를 한다. HJOW-X3D는 VS17(2022)/v143이며 BuildTools cmake의 전체 경로를 쓴다. [빌드 안내](docs/cpp-build.md) 참조.
- **편집·도구:** 이 문서는 기존 CRLF를 보존한다. UI-script TSV는 BOM 없이 쓰며 게임 스모크에는 EXE/데이터의 절대 경로를 넘긴다. 시작한 작업/프로세스만 종료하고 이름 기반 일괄 강제 종료를 하지 않는다. Git Bash의 `/p:` 경로 변환을 피하려고 MSBuild는 PowerShell에서 실행한다.
- **UTF-8:** PowerShell 5.1에서 Python stdin으로 한국어를 보낼 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정한다. cp949 오류 시 `PYTHONUTF8=1/PYTHONIOENCODING=utf-8`을 설정한다. 원시 로그/Native fixture를 인코딩 정리로 덮어쓰지 않는다.
- **Git:** 이 문서/CPP 작업 파일은 기존 줄바꿈을 유지하고 확인/stage에 필요 시 `git -c core.autocrlf=false`·`-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정은 바꾸지 않고 최초 관측 TSV의 `-text`를 보존한다. DN/WB는 해당 문서·`.gitattributes` 규칙을 따른다. 단위별 인계 후 `YYYYMMDD-NN PJ 요약`으로 로컬 커밋한다. 당일 마지막 번호를 확인하며 cpppj=CP·dotnetpj=DN·webpj=WB·공통=CM이다. push는 금지다.

## 5. 참고 문서와 이관 기록

- [C++ 빌드](docs/cpp-build.md)·[로드맵](docs/cpp-roadmap.md)·[플레이 가능 계획](docs/cpp-playable-plan.md)·[소스 맵](cpppj/SOURCE_MAP.md)·[모듈 맵](docs/exe/modules.md). 이들 문서의 날짜별 수치보다 이 문서0.1절과 최신 단위 보고서를 우선한다.
- [디컴파일 신뢰도](docs/exe/decompile-reliability.md)·[2026-11-03 양판본 디컴파일·컴파일](docs/exe/cpp-decompilation-20261103.md)·[analyzeManager](docs/analyze-manager.md).
- [986 추적 인계](docs/exe/handoffs/cpp-pause-20261106/README.md)·[과거 준비본](docs/exe/prepared/README.md). 443/444와469의 일부는 후속 통합됐으므로 보관본의 미적용 문구를 현재에 재적용하지 않는다.
- **2026-11-06 이번 정리(HJOW-Athlon):** 정리 직전 전체 원문170,929바이트를 [보관본 끝](LEFT_JOBS.old.md#archive-20261106-through-986)에 그대로 추가하고 기존 보관 바이트를 유지했다. 본문에서 반복되던 단위별 완료 로그/검사 수/해소된 다음 작업/옛 중지·재개 지시를 이관하고 완료 범위 색인으로 대체했다. 해소되지 않은 검사 실패·사용자 결정·MP 이월·환경/호스트 경계는 본문에 남겼다.
- **현재 상태 대조와 정정:** Git 이력(`2349afd8` 기준), `cpppj/src/CMakeLists.txt`, 실제Login/배경그리기 소스,985/986 보고서와237파일 보관 해시, 로컬EXE 크기/SHA를 대조했다. ① 1차Edit/수명 잔여→943/956/980 마감 ② 3차미착수→982~985 사전 구현 ③ 와이드 결함→981 해결 ④ 5차조사만→932~934 구현/검증 ⑤ 데모 둘째 주기 잔여→946~948 완료 ⑥ PCM/소리녹화 미검증→936 성공 ⑦ 8차미착수→제품미착수/후보보존 ⑧ 986커밋 예정→`c68942cf` 완료로 고쳤다. 한국어 후보를938 제품 구현과 혼동하지 않는다.
- **이번 검증 범위:** 문서 링크/앵커·보관 전후 바이트·237파일 해시·EXE 해시/크기·Git diff를 확인했다. 빌드/C++ 검사/게임/MP/디컴파일은 하지 않았다. 보호 파일과AGENTS.md·루트README.md는 수정하지 않았다. `cpppj/README.md`의961/다음962 머리말도985/986 기준으로 동기화하고, 뒤의 날짜별 내용은 과거 기록으로 명시했다. 문서 정리 완료이며 제품 기능 완료/개발 목표 재개가 아니다.
- 이전 전체 원문: [504](LEFT_JOBS.old.md#archive-20261027-through-504)·[549의0절](LEFT_JOBS.old.md#archive-20261029-section0-through-549)·[561의3절](LEFT_JOBS.old.md#archive-20261029-section3-through-561)·[638](LEFT_JOBS.old.md#archive-20261031-through-638)·[695](LEFT_JOBS.old.md#archive-20261102-before-cleanup)·[808](LEFT_JOBS.old.md#archive-20261103-through-808)·[861](LEFT_JOBS.old.md#archive-20261104-through-861). 과거 이관 설명은 이번 보관 원문의5절에도 있다.

## 인수인계 문서 분담

cpppj/공통/디컴파일은 이 문서, dotnetpj는 LEFT_JOBS.dotnetpj.md, webpj는 LEFT_JOBS.webpj.md에 기록한다. cpppj 1차·Windows 10/11 호환·MCP가 완료돼 dotnetpj 구현에 활용할 수 있게 되면 인수인계를 LEFT_JOBS.md로 다시 통합한다. 현재는 프로젝트별 문서를 유지한다.
