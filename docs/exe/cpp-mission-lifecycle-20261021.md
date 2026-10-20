# 실제 Guide 단계 반복과 월드 진입 Pause

단위250의 시작 커밋은 `3485834e`다. 현재 PC/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 전체 지정11미션 목표는 진행 중이다.

## 실제 시작 순서의 차이와 연결

기존 BeginMission은 Script/월드/로그인/프레임을 모두 설치하고 Start를 직접 실행했다. 실제 Guide 순번은 생성 기본0에서 Start가1로 만들었고, 원본 Guide의0..6 반복은 Client에서 호출되지 않았다. 초기 안내 전에도 세계 시계가 진행될 수 있었다.

원본 `004b6dd0` State7의 `DAT_00594fbc=1`→음악 `00469fc0` 뒤 호출은 **`00463a40`→현재 미션 가상+28**이다. Tutorial 가상표 `005149ec`에서+28은 `004c2a40` **자기 정지**이며 Start는+10의 `004c2b20`이다. 기존 코드의 주석과 직접 Start 호출을 이 순서로 바로잡았다. 이는 원본 전체 State7을 복원했다는 뜻은 아니다.

현재 `BeginMission`은 Script/빈 Runtime만 만들고 Tutorial을 실제 `RawMissionGuide`에 맡긴다. Guide는 현재 상태와 UI 전환 큐의 Pending,Dialog/Briefing 부재를 직접 읽는다. `MissionRuntime`은 Session보다 먼저 살아 있으며 실제 Start 연결부는 단계1의 Session 생성 뒤에만 만든다. Guide 객체는 현재 미션 해제 중에도 호출 스택이 살아 있도록 UberGump 수명에 둔다.

- 단계0은 원본 First의 flags 요청을 사용한다.
- 단계1은 현재 Header/월드/로그인/커서/공통 프레임을 기존 실제 소비자에 설치한다. 음악 뒤 현재 미션의 **PauseForDialog**를 호출해 자기 정지 한 깊이를 잡고 다음 단계 flags를 요청한다.
- 단계2는 원본 `Tell("init",1)` 호출 경계와 마지막 flags 요청이다.
- 단계3에서만 기존 실제 `RawMissionStart::Start`를 호출한다. 같은 순번3을 원래4로 증가시키며 SP/기술/AI/시각 등 기존 실제 몸체를 실행한다.
- 단계4에서 공통 프레임을 실행한다. 창이 열렸거나 전환이 대기 중이면 Guide는 가상 Postpone만 호출한다.
- 단계5는 성공 bit4의 Success/Failure 조회→성공한 Tell의 Kill/Pause→마지막 flags 요청이다. 단계6은 원본 빈 `004437f0`이며 Guide가 현재 미션 해제를 호출한다.

`MissionLifecycle`은 얇은 단계와 진입 Pause를 분리하고 초기화/Start/프레임의 큰 몸체는 실제 기존 소비자로 보낸다. 단계1의 전체 `004c3290`과 원본 Init의 이름 정규화/+b0/전체 전역,비동기 State7/Guide 공장/가상 소멸까지 새로 명령 단위로 복원한 것은 아니다. 새 해제 관문도 기존 MainMenu/ClearWorld 어댑터를 사용한다. 지정11개 미션과 보호 tell 본문에는 원본 이름 그대로의 `init`/`Success`/`Failure` 섹션이 없어, 없는 본문을 새로 만들지 않는다. 사용자 맵의 init 본문/Tell 표시 인자1 전체 동작은 별도 검증 대상이다.

Header 조회는 Runtime 생성에서 단계1로 옮겼다. 단계0에서 아직 world가 없는 상태도 Report가 안전하게 읽는다. 기존 Battle 진입과 복원하지 않은 CD Guide는 이10.78 Tutorial 경로에 분배하지 않는다. 전체 비동기 State의 pending을 구현한 것은 아니며 현재 실제 단일 플레이 UI 전환 큐와 동기 로그인 경계를 공급한다.

## 제품 작성 전 실제 원문

`decomp_mission_lifecycle.py`는 실제 Guide `00463bb0`/Advance/First,단계2 `004c2b00`·5 `004c31a0`·빈6,Postpone,`00463a40→004c2a40`을 **831고유 입력·1262정상 반환·44306실제 명령**으로 관측했다. Guide1230회,진입 함수56회,Pause60회다. 현재/flags/순번/전환·창 차단/종료 요청/반복/중첩 정지/기한을 교차해 전체208바이트,현재/깊이,원본 재조회와 외부 호출 순서를 보존했다.

큰 Init/Start/Frame,Tell/UI·시계 효과·해제는 **명시 경계**다. Init 경계는 다음 flags와 실제 진입 Pause를 요청하고 Start 경계는 원래 Advance에 진입한다. 이 큰 몸체나 원본 월드 실행을 새 관측으로 합산하지 않는다. 실제 명령 쓰기는 flags/순번/기한/자기 정지와 한정 스택만 허용한다. 정상 반환/스택·비영 보존 레지스터·x87 제어어를 검사했고 게임/OS/창은 없다.

현재 전체 함수 표에서 누락된 First17바이트/Postpone는 실제 가상표와 capstone으로 확인한 범위를 허용했다. 디컴파일 파일을 고쳐 함수를 추가하지 않았다. 최초 성공 원문/도구/보고서는 제품 작성 전에 `extracted/mission-lifecycle-20261021-first-capture/`에 보존했다. 기존 Guide1550/공통328/시작/다른 원문은 덮어쓰지 않는다.

## 검사와 후속 검증

`MissionLifecycle_Native831`은 전체 원문 상태/호출 순서를 기존 실제 RawMissionGuide/FrameGate와 새 단계 분배로 대조한다. `MissionLifecycle_ActualPendingAndEmptyEntry`는 현재 UI 전환 큐의 Pending과 빈 현재의 무동작을 확인한다. 실제 `CampaignMissionLifecycle_ActualSessionEntryPauseBeforeStart`는 보호 캠페인 Session 설치→진입 Pause→순번3 Start→같은 공통 프레임/10호출 안내·창 관문·7호출 재개를 연결한다. 장치/UI/시간은 명시 입력이며 전체 Kernel/Client 창/자연 플레이 검사는 아니다. **C++검사는 작성·컴파일만 하며 사용자 지시대로 실행하지 않는다.**

빌드·현재 연결/보호 원문·최초 결과·현재 핀 역변환은 `cpppj/mission-lifecycle-session.json`과 `cpp_mission_lifecycle_audit.py --verify`에 남긴다. 직전 공통 미션 정적 감사는 현재 Header 조회가 단계1로 옮겨진 실제 경로를 확인하도록 갱신하며 과거 원본/기대값/실행 결과를 바꾸지 않는다. 기존 보고서의 현재 핀만 후속 이력으로 갱신하고 전체 JSON을 시작 커밋으로 역변환한다.

최종 전체 Release는 **경고0·오류0·종료0**이며 라이브러리·게임/검사 EXE를 생성했다. 원문 로그는 `extracted/cpp-mission-lifecycle-20261021-build.log`다. C++검사와 실제 게임 실행 통과를 주장하지 않는다.

현재 소스 핀 **102보고서·130항목**의 전체 JSON 역변환이 시작 커밋과 일치한다. 이번 원본/연결 감사와 현재 단계1 Header를 확인하도록 갱신한 공통 미션 감사,변경 검사가 모두 종료0이다. 과거 원본/기대값/실행·초기 시각 관측을 새 성공 결과로 바꾸지 않았다.

Guide Begin/Fork/Finish/후속 제어 명령,전체 Init/가상 소멸·State/네트워크 세부와 Review Knowledge/ShowTechnology·중첩 창/다른 키는 계속 필요하다. 이전보다 초기 안내 전 게임 시각과 순번이 달라지므로,예전 저장 플레이 관측을 새 실행 성공으로 쓰지 않는다. 실제11미션 자연 완주·진입 정지/F8/재시작/중단·전략AI·지형 효과/포획/희생/Dude 자연 대피와 원본 외관/음향 동일성은 후속 통합 실제 플레이에서 검증한다.
