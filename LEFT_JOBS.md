# LEFT_JOBS — cpppj 및 공통 인수인계

cpppj와 공통 사항의 **현재 제품, 완료 범위, 남은 구현, 사용자 확인, 재개 절차**를 정리한다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다.

**2026-11-07 진행:** 사용자 /goal 지시로 기존 일시중지를 해제했다. 실행 제품은 **993 / 11.01**이며987~992의 Fort/섬/일반 시작 세부 경로에 이어993 시간 패킷·Commander·직접 송신과 실제 게임 시계 보정을 연결·컴파일했다. 남은 AI 구현은 **LAN 멀티플레이 → 한국어 후속 → Edit 시나리오 UI**다. 지정된 공격/포획/희생/승리·사용자 확인·멀티플레이 테스트는 제외하고 단위별 문서/로컬 커밋을 계속한다. 986까지의 상세 이력은 [보관본](LEFT_JOBS.old.md#archive-20261106-through-986)에 있다.

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-11-07, 제품993)

| 항목 | 현행 기준 |
| --- | --- |
| 진행 상태 | **사용자 /goal로 구현 재개.** 993을 마쳤고 다음은 일반 State6·7 호스트와 roster·LAN UI다. 전체 목표는 미완료이며 지정된 테스트 제외를 유지한다. |
| 현재 제품 | **993 / 표시·파일·제품11.01.** 시간 패킷·70표본·Commander·직접 노드 송신과 같은 GameTicks 게임 시계를 연결했다. Multiplayer 메뉴와 일반 전투 전체는 아직 미완료다. |
| 실행·플레이 가능 여부 | 무인자 GUI 시작과 미션 플레이 경로가 있다. 일반31+데모3의 마지막 전체 진입 검증은 **645의644 제품**이며 현재 제품의 전체 회귀 결과가 아니다. |
| 원본과의 동일성 | 부품별 원문 대조와 집중 검사를 누적했다. **전체 동일성·모든 시나리오 자연 완주는 미확인**이며 사용자 확인 대상이다 |
| EXE SHA-256 | `71540224f4926cd5fda2ff9ee12de7e47fab7d16786aced609216ac09c207261` (993 / 11.01, 11,647,488바이트) |
| 최신 빌드·집중 검사 | **993 Release 제품 빌드 종료0·경고/오류0.** 신규 MP/단위/게임/통신 검사0이다. 기존 비멀티 파일 읽기9검사는987 결과다. [993 기록](docs/exe/cpp-clock-synchronization-20261107.md)·`cpppj/clock-synchronization-20261107.json`. |
| checking 잔여 | **8함수=부모3+부분2+MP분류3.** 완료부모39/부분4이며현재비멀티수명/FILE범위는980에서마감했다. 메타맵2표/재접속/Descend 등은3절8번 잔여다. |
| 마지막 GUI | **984 실제 단독MCP 1실행:** 격리1920×1080 tutorial1 진입·비정지100프레임 대기·WM_CLOSE 종료0, 입력5/관측7·PNG 확인·설치1351개/보호 불변. LAN/공격/포획/희생/승리/물리 입력 검증이 아니다. |
| 전체 검사 기준 | 활성 솔루션83프로젝트: Application78(검사 이름73/게임1/probe·dump4), 라이브러리2, Utility3. 현재 제품의 전체 검사 통과 기록은 없다. 과거 전체 결과는 보관본, 알려진 실패/기준선 판정은2.5절을 따른다. |
| 전체 34·ZIP | 645의644 제품, 최종34/34 진입·비정지 시각 증가·창DC RGB·Native Frame 확인. ZIP SHA-256 `6485febc36080dc90e9eec43965a842f85d39e81c82a003a96b83b34b3694b73`, 1376항목/게임 자산1358개, 한글😀 경로 압축 해제 실행 확인 |
| 보호 기준 | **987에서 새로 고정한 AGENTS.md·원본/CD/fixture2800파일을993 종료 때도 모두 불변으로 확인했다.** 새 fixture 없음. 다른 세션의 WB 파일은 보존하고 CP 커밋에서 제외한다. 원시 기준은 `extracted/cpp-resume-993/protected-before.json`이다. |
| 마지막 전체 정밀 디컴파일 | **HJOW-Athlon (192.168.0.94) / 2026-11-03**. 양판본 정밀 산출물과 컴파일 기록은 [2026-11-03 기록](docs/exe/cpp-decompilation-20261103.md)을 따른다 |

### 0.2 재개 절차

1. 사용자 재개 요청 뒤 AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 해당 인계도 읽는다. `git status`와 `git fetch`로 다른 PC/세션 변경을 확인하며 push는 하지 않는다.
2. 현재 호스트가 마지막 디컴파일 호스트와 다르면 cpppj 작업/디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 문서에 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 불완전하므로 쓰지 않는다.
3. 실제 게임·별도 창 실행은 AGENTS.md의 확인 규칙을 따른다. 예외는 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94) 및 사용자가 직접 요청한 원본 수동 컨트롤/녹화 분석의 해당 단계다. 다른 PC는 실행 목적·필요성 설명 후 명시적 확인이 필요하다.
4. 최신 제품993의 보고서는 `cpppj/clock-synchronization-20261107.json`이다. 986의 한국어/시나리오 후보·기준·해시는 [추적 인계](docs/exe/handoffs/cpp-pause-20261106/README.md)에 있다. `extracted/cpp-resume-993/` 등 원시는 Git 제외·작업 PC 전용이므로 다른 PC에 있다고 가정하지 않는다.
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

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **활성 비멀티 구현·대조 범위 마감.** UI929·Edit943·미션 수명956·내부 수명/FILE980. 34경로의 마지막 전체 진입 확인은645/제품644다. 자연 완주·전체 동일성은 사용자 확인이며, 새로운 제보는 AI 수정 항목으로 연다. 8차 신규 시나리오 UI와 구별한다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **구현·현재 확인 범위를 정리하고 후속 단계로 전환.** 호환 전체화면·화면비·가장자리 스크롤·저장 재시작을 확인했고 와이드 덱 결함은981에서 수정했다. 물리 입력/출력·다른 장치/DPI·Windows 11/ARM 실기는 미검증이며 사용자/환경 확인이다. 무오류 보증은 아니다. 10.72 호환성은 후순위다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | **사전 구현 진행 중.** 982 TCP·984 공유 전송/수명·985 일반 Login/SP·987 원격 Fort 수신·988 참가자별 동기화·989 전투섬·990 중앙섬·991 자기 Fort·992 시작 세부 경로·993 시간 동기화/Commander를 연결·컴파일했다. 일반 전투·전체 decoder·roster/LAN UI·Outpust는 미완료다. 신규 MP/통신 검사0이며 플레이 가능 상태가 아니다. |
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

아래의 완료는 **표에 적힌 구현·대조 범위**다. 원본 전체 동일성이나 최신 제품의 전체 회귀 완료를 뜻하지 않는다. 단위별 원문 관측·검사 수·최초 실패·교정 내역·제품 해시는 [정리 전 전체 원문](LEFT_JOBS.old.md#archive-20261106-through-986)과 각 근거 문서에 보존했다. 과거 검사 수와 미연결 예고를 반복 나열하지 않는다.

| 범위 | 완료한 구현·대조와 최신 근거 | 남은 범위의 위치 |
| --- | --- | --- |
| 비멀티 메뉴·입력/UI | 무인자 시작·메인/브리핑/Help/Credits·단축키·객체/화물 메뉴·선택/커서·HUD/Chat/Console/모달 수명. 697~761/913~929, [929 정리](docs/exe/cpp-ui-implementation-closure-20261105.md) | MP 전송/메타/의식은3절8번, 전체 외관·물리 입력은5·7번 |
| 원문 Edit·시나리오 실행 | Fort 읽기/쓰기·Save as·섬/그림/이동·owner/Graph·동적Config/TARC/HTML 명령·저장 실패/재시도·Test Battle/편집 복귀. [943 정리](docs/exe/cpp-editor-implementation-closure-20261106.md) | 신규 시나리오 UI/english 자동 생성은8차. 모든 조합/손상/OS 실패 관측 완료는 아님 |
| 미션 교체·State | Retry/Next/Abort·Guide 교체·실제 연속 왕복·State1/2/3/단독7·LeaveBattle·cleanup/Tick. [956 정리](docs/exe/cpp-mission-state-scope-20261106.md) | 일반 전투/State6·저장/재접속은3절8번. 자연 승리/진행 해제는 사용자 |
| 데모 반복 | 제품939에서 세 데모 모두360초 두 만료→세 번째 월드 진행→Exit Demo/메인/종료. [946](docs/exe/cpp-demo-second-storm-20261106.md)·[947](docs/exe/cpp-demo-second-guard-20261106.md)·[948](docs/exe/cpp-demo-second-alliance-20261106.md) | 모든 자연 전투/승패·최신 제품 회귀와 구별 |
| 월드·규칙·Renderer | 공통 AI·워크샾25종·의식/SP·간헐천·투사체/다리/지형·단일 표시 목록·카메라/미니맵·Graph/색표. [25종](docs/exe/cpp-unit-restoration-20261022.md)·[Renderer](docs/exe/cpp-renderer-frame-20261030.md)·[Graph](docs/exe/cpp-editor-graph-world-20261102.md) | 자연 플레이/전체TYPE 프레임·공중3종 신규 전용 검사는 제외. Graph 장부 차이8은 원본에도 있어 정규화하지 않음 |
| 현재TYPE·HP·생성/반납 | 834~912의 Unpop/Take/Pop·현재VT/TYPE·Form/Process/Factory·의식/너겟/Rune·HP/Regular·실제 Session 연결과957~979 후속. [Pop 범위](docs/exe/cpp-pop-current-scope-20261106.md)·[Factory](docs/exe/cpp-factory-parent-scope-20261106.md)·[980 정리](docs/exe/cpp-internal-lifetime-closure-20261106.md) | checking8·MP writer·CD전체/ABI 자체/미관측 실패는 분리. 효과 정적 연결을 실제 포획/희생 검사로 세지 않음 |
| checking 소비자·군도 | 779~833의 현재 진단 복귀/장기owner·Graph/연소/마나/도착·892~895 군도 root/CoreData. [함수 색인](docs/exe/cpp-checking-consumer-gaps-20261103.md)·[군도 root](docs/exe/cpp-terrain-generation-root-20261105.md) | 892/893의 당시 상위 미연결은894에서 연결. 일반 State7 밀도 writer는992·중앙섬은990 연결. 남은 MP 부모는3절8번 |
| FILE·자산/색표·저장 실패 | 파일명/경로·Native FILE/키/TAFF·Config/TYPE/GIF/SHP/글꼴·옵션 교체/복구·실제 색표Cache·Kernel/SID/new_handler/종료. [966](docs/exe/cpp-options-save-failures-20261106.md)·[968](docs/exe/cpp-colormap-consumers-startup-20261106.md)·[980](docs/exe/cpp-internal-lifetime-closure-20261106.md) | FILE 직접74호출의 현재54/MP·폐기인터넷18/폐기CD1/휴면Chat1 분류. 32비트ABI/실제OS 고갈/전체PE 간접호출 부재 증명과 구별 |
| 4차 MCP·와이드 | 935~937의 조작/상태/PNG/대기/소리 포함 녹화·1600×900/1920×1080. [MCP](docs/exe/cpp-mcp-server-20261106.md)·[해상도](docs/exe/cpp-wide-resolution-20261106.md). 덱 검은띠는[981](docs/exe/cpp-wide-deck-background-20261106.md) 수정 | 물리 입력/활성 전체화면 출력은 사용자/환경 |
| 5차 사용자 데이터 | 932~934의 Saves/CustomMaps/Editor/Captures/Diagnostics·reader·무제한/50행 스크롤·읽기 전용 설치/Unicode 실행. [5차 기록](docs/exe/cpp-user-data-root-20261105.md) | CustomMaps 배치/복사 방식은 사용자 확인(3절8번) |
| 6차 한국어1차/7차 후처리 | 938 D2Coding/표시 부호화·502항목·언어 메뉴/영어fallback, 939 D3D11 AA/공식FSR1·1440p/4K 선택/캡처. [한국어](docs/exe/cpp-korean-language-20261106.md)·[AA/FSR](docs/exe/cpp-aa-fsr-20261106.md) | 한국어 본문/그림/입력은 미적용 후보. 미션/활성 전체화면/실제4K/입력 역변환은 사용자 확인 |
| 호환·배포·제품 정책 | 전체화면/3화면비/가장자리·저장 재시작(746), `Netstorm.exe`/배포 스크립트/빌드 선택(562/637/739), 11.01/Credits685·외부접속 분류/검증 비활성684/965 | 최신제품 ZIP 검증 미실시. 구 CD검증7f/80 복원 금지. 1.3절·3절7·8번 |
| 최신 사전 구현·인계 | 982 TCP·984 공유 전송/수명·985 일반Login/SP·[987 원격Fort](docs/exe/cpp-remote-fort-image-20261106.md)·[988 동기화](docs/exe/cpp-peer-world-sync-20261106.md)·[989 전투섬](docs/exe/cpp-battle-island-20261107.md)·[990 중앙섬](docs/exe/cpp-central-island-20261107.md)·[991 자기Fort](docs/exe/cpp-local-fort-20261107.md)·[992 시작 세부](docs/exe/cpp-battle-start-details-20261107.md)·[993 시간/Commander](docs/exe/cpp-clock-synchronization-20261107.md). 986 후보 보존 | 일반State 호스트/LAN UI/전체 파생 writer·decoder 미완료·신규MP검사0. 986 후보는 기능 완료가 아님 |

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

checking8(부모3/부분2/MP3)은8번의 명시 잔여다. 미등록코드/NULL/범위 밖 포인터·임의외부heap→SID·32비트FILE/FileSpec/SEH·미작성 메모리 자체 일치, 실제OS 고갈/UNC/드라이브CWD/장치실패와CD전체dump/trace/Retype/파생은 호스트/후순위 경계다. 현재 등록된 활성 비멀티 caller의 미연결을 이 경계로 숨기지 않으며 새 근거/제보가 있으면 다시 연다.

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

- [ ] **3차 TCP/IP 로컬 멀티플레이·Outpust:** 982 TCP부품/984 공유 등록표·per-ID 복합 전송·실제 단독 Client/World 수명·985 일반 Login/SP·987 원격 Fort/manager/수신 이미지 수명·988 참가자별 동기화·989 전투섬·990 중앙섬·991 자기 Fort·992 시작 세부 경로·993 시간 동기화를 접합·컴파일했다. LAN UI/roster/일반 전투는 아직 미접합이다. 다음은 **일반 State6·7 호스트/roster→LAN UI→메타·저장/재접속/Outpost/checking8**이다. 원격 client buildFort는989에서 연결했다. 자기 ordinary localFort는991에서 연결했다. 신규 MP 단위/게임/통신 검사는 하지 않는다. 공식 서버 복구·구 클라이언트 유효성 검사는 연결하지 않는다. **폐기CD확인7f/80(42cd10)은965의검증비활성화정책에따라복원하지않는다.**
- checking전체잔여8(957재대조,완료부모39/부분4불변): 부모004a7510·004b02d0의메타맵표시0050c9f0/0050cc08·004dc220(event4/Descend),부분004b54e0/004b8cb0,MP분류0042bc10/004b4df0/004b8b20. 기존분류이며직접inSingle관문부재를모든단독도달불가증명으로확대하지않는다. 신규MP검사없이원문구현을이어간다.
- MP 이월 목록: 공통+20 미연결VT0050c9f0(TYPE108/109)/0050cc08(TYPE161), 시스템뉴스0042e470/0042e4d0/0042e4f0·endpoint/UI·설정저장, RequestMenu 전송 `004d712e..004d717c`, 메타맵 열기 `004789b0/00479390`·번개 `00479650`·Challenge 이미지/텍스트 `00478d10`/참가자 삭제 `0042bc10`, c6/db·ChalSailPos 모드6·선행클릭/항해 메뉴·desc, 비미션 의식/랭크·조기 희생 안내, PlayerNoMake writer `00433c30`, frame22/주기64·timeout/UI·endpoint/server/boss·리슨/수락/22칸 송신, Activate의 별도 수신자/삭제 선택·프로세스 writer 전체/decoder·재동기화의 허용/owner 캐시 부모(843), 섬·표면 TypeWas의 MP 수신 writer(861).
- 신전 `004dc220` event4의fort territory0046f650/0046f5a0·curServer0054db4c·Descend0x02000013→004b3760/004b3620/State19와 생성입구004dbac0/0x030000a7을 포함한다(866). 기존 removeAnimation/regionGeneration 임시명은 실제 의미가 아니며 원문 부모 전체를 사전 구현할 잔여다. 직접inSingle 관문이 없으므로 단독 전체 미도달을 증명한 것은 아니다.
- 일반 전투 `!state.dat` 저장/복구·saveBattle/metamap/비mission phase14·StateReconnect/서버 이전`004b8cb0`(ForceServerDeath/StateReconnect·자동 장애 caller,831)·Zone0/메타 Poll·`004b54e0` 파일명/count 진단과 `0044fa00` 소비자·`004a7510` 참가자 재접속/팀 재편·실패 진단·LeaveBattle 원격 페널티(004cbab0/004b4ac0/004b4b40/004bfb50)와State6의일반Fort/결과복귀를 포함한다(661/955의 분류). 기존 질문/삭제 UI는 유지한다. 시계 패킷/Commander는993에서 연결했다. 정적 사전 구현과 검증 완료를 구별한다.
- 일반전투 State7의 최소/최대밀도 옵션getter·게임난수/강제밀도bb4 writer는992, 중앙섬metadata 수신/송신 observer는990에서 연결했다. 일반 호스트 State7 진입/World 교체·전체MP부모는잔여다.894는실제생성root/장기owner의코드연결이며이writer나MP동작검증완료를뜻하지않는다.
- [x] **4차 로컬 MCP·와이드 완료(935~937), 덱 검은띠981 해결.** 제공 기능/검증과 물리 확인 경계는1.1절·3절7번을 따른다.
- [x] **5차 사용자 저장 경로·무제한 목록 완료(932~934).** 실제 이관/reader와 읽기 전용 설치·Unicode 실행을 확인했다. 사용자 확인: 작업 루트가 `CustomMaps`라 실제 맵은 `CustomMaps\d\NAME.fort`이고 첫 Edit 목록 때 원본fort433개를 복사한다. 스크롤은 이동 행 클릭 방식이며 마우스 휠은 미구현, 이동 행 문구는 영어 고정이다. 배치/복사 방식은 사용자 결정 후 조정하고 한국어 문구는6차에서 처리한다.
- [ ] **6차 한국어(938 1차 완료: 글꼴·번역표 502항목·Language 메뉴·`.korean`→`.english` 대체·배포 동봉. 남은 것: 미션 브리핑/팁/도움말 `.korean` 본문(AI/SP 구성은 english에서 복사), 한글 메인 버튼 그림, 번역 검수, 한글 입력):** 메뉴/문자열/시나리오·글꼴을 지원한다. 글꼴은 D2Coding이며 명칭은 AGENTS.md를 따른다. **986후보는 미적용이다.** `korean/FINAL_PLAN.md`의 최종 코드41파일과 별도44자산을 검토·병합하고 허용된 빌드/집중 검사로 확인한다. 표시 부호화/IME·클립보드 경계를 시나리오 통합 전에 완료한다.
- [x] **7차 AA/FSR1.0 구현 완료(939).** 기본off·1440p/4K 자동FSR/논리1080p는1.1절 기준이다. 미션/활성 전체화면/실제4K/입력 역변환은 사용자 확인이며 성능 최적화는 후속이다.
- [ ] **8차 Edit 시나리오 UI:** 초기 자본/테크·적·동맹 등을 별도 편집 UI로 수정하고 새 맵의 기본 english 자동 생성·english 부재 Test Battle 종료를 개선한다. 기존 수동 대본·저장/재적재를 보존한다. **986 통합 후보는 미적용이다.** 한국어 통합 뒤 `scenario/PLAN.md`·`CMake-additions.txt`를 따라 모델5파일/코드를 병합·등록하고 실제 사용자 홈 저장/Editor/Test Battle 진입을 확인한다. 0.4절의 준비 검사를 실제 통과로 세지 않는다.
- 배포 스크립트 자체는 완료다. 배포할 제품이 정해지면 README/BUILD_INFO를 갱신하고 그 제품의 ZIP·압축 해제 실행을 확인한다. 마지막 전체34경로와 묶인 검증 ZIP은644 제품이며 이후 제품의 전체 배포 회귀로 쓰지 않는다. 938에서는 별도1379항목 ZIP의 한국어 실행만 추가 확인했다. 현재993의 ZIP 검증은 없다.
- 60/120프레임·CD10.72 전체 복원/Windows 10 구동 불가 해결책은 별도 후순위다. Linux는dotnetpj 후순위이며cpppj는Windows 대상이다.

## 4. 운영 규칙과 유효한 사용자 결정

- **최신 진행 지시(2026-11-06 /goal):** cpppj의 이 문서상 AI 작업 완료까지 계속한다. 기존 일시중지는 해제됐다. 사제 포획/희생·특정 적 공격·승리 재현과 사용자 직접 확인 항목·멀티플레이 테스트를 제외한다. 단순 진입/대기/유닛 생산/건설로 끝나는 필요한 검증은 진행한다. 단위마다 인계/로컬 커밋하며 커밋 실패 때도 작업을 계속한다.
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
