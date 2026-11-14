# dotnetpj 0.530 — cpppj1312/1313 영향 분류와 생성 전 AI 동맹 판정

2026-11-15, HJOW-Athlon. 직전 단위0.529는 f1c8e0459다. cpppj 신규 커밋382ec33fe(1312)·979d65b5d(1313)의 src 변경15파일을 검토했다. 이 단위는 기존 구현에 즉시 닿는 생성 API/동맹 판정을 옮기고 아직 없는 상위 모듈의 변경을 구체적으로 이월한다. 전체 cpppj 소스 이식이나 전체 영향 floor 갱신이 아니다.

## 15파일 처분

| 소스 | 이번 처리 | 남은 연결 시점 |
| --- | --- | --- |
| o/AiAllianceSide.h | 새 C# 계산 모듈을 이식한다. 설정+23c의 로컬 동맹 선언도 생성 전 판정에 반영한다 | 실제 AI Host 설치에서 같은 판정기를 사용해야 한다 |
| o/RawAiCreate.h/.cpp(2) | Difficulty(owner,settings) 현재 API를 이식한다. NB10 정리 후·난이도 배율 전의 설정을 전달한다 | 원본 기본 경로 대조와 새 계약 검사, 실제 Host AI 호출은 R4 잔여 |
| client/CMakeLists.txt, o/CMakeLists.txt(2) | C++ 빌드 목록이다. C# SDK의 자동 파일 포함을 쓰므로 줄별 복사는 불필요하다 | 빌드/배포 구성은 각 언어 방식 유지 |
| client/CampaignTestPlay.h/.cpp(2), app/main.cpp | 명령줄 전용 테스트 옵션6종, 프리셋/현재 프로세스 저장, 시작SP/로컬AI 설정 준비가 새로 들어왔다. 일반 실행·메뉴·설정 파일에는 없다 | R6 AI/미션 Start 및 R7 실행 도구 조립에서 옮긴다. 아직 구현 완료로 세지 않는다 |
| client/CampaignTestSpectator.h/.cpp(2) | 최근 HP 감소/피해 후 소멸을 군집화하고 의식 위치를 우선하는 관전 계산이다. 게임 입력/난수는 바꾸지 않는다 | R5 카메라·R6 실제 월드 프레임/위치 음향 조립 뒤 옮긴다 |
| client/RawCampaignMissionStartBindings.h/.cpp(2), client/UberGump.cpp | 테스트 적격 미션 판정·로컬AI 부착·동맹별 난이도/SP·관전 서비스의 상위 결선이다. 무설정일 때 기존 흐름을 유지한다 | 해당 실제 미션 Start/AI/소리/카메라 설치에 함께 옮긴다 |
| client/RawCampaignSession.cpp | 난이도 콜백에서 현재 설정 동맹을 읽는 IsAiEnemyOfLocal 호출과 데모/소유자 범위 관문이다 | 실제 Host AI 생성 조립 때 적용한다. 현재 Host는 프레임 AI/brain 자동 생성 호출이 아직 없다 |
| client/RawCampaignHitBindings.cpp | 제단/다이스 가상98의 투명/희생 방향11·12 무시와 일반 피해 전달을 반환형/총기 양쪽에 공유한 보정이다 | 미이식 RawCampaignHitBindings 전체를 R6 피격 조립에서 현행 HEAD로 옮긴다. 특정 적 공격 재현은 사용자 검사로 둔다 |

3파일 이식·2빌드 파일 분류·10상위 파일 이월이며 누락된 파일은 없다. 이월은 변경 내용과 후속 위치의 분류 완료를 뜻하며 구현 완료를 뜻하지 않는다. 기존 0.379 전수 검토·0.499의1304~1310 검토에 이번1312/1313 분류를 더한다. fixture reflected-commit/floor는 그대로 둔다.

## 구현과 검사

AiAllianceSide는 같은 소유자0~8, 현재 로컬 동맹 마스크, AI 설정의 비정렬 little endian 동맹DWORD를 읽는다. 자기 자신·이미 기록된 동맹·AI가 로컬을 동맹으로 선언한 경우는 적이 아니다. 검사 순서는 설정 최소576BYTE→owner 범위→자기/로컬 동맹→설정 동맹이다. 일반 생성 API는 기존대로 정확히600BYTE를 받는다.

RawAiCreate는 강제 적 번호를 정리한 실제 active 설정을 Difficulty의 둘째 인자로 전달한다. 그 뒤에만 배율을 적용하고 같은 결과를 생성자/초기 갱신에 넘긴다. 원 입력은 보존한다. 기존 두 검사 호출자도 현재 API로 바꿨으며 이전 인자 수를 유지하는 우회 API는 만들지 않았다.

새14검사는 비대칭 동맹·0/8/상위 비트·물리 경계/진단 순서, 실제 Player 참여/AI 생성 경계에서 정리 전후 설정·적 쉬움/동맹 보통 주기/채취 수·입력 보존을 검증한다. 초기 검사 namespace의 참여 상태 이름 중복은 기존 Simulation 형식 별칭으로 교정했다. 최초 관련Core42/42와 기존 cpppj 캠페인 테스트16/16이 통과했다(각 종료0). C++ 검사는 창 없는 단위 검사이며 실제 자동 플레이/승리 실행이 아니다.

설정 동맹을0으로 고정한 변이는14중4실패, 정리 전 원본 설정을 콜백에 넘긴 변이는14중3실패로 검출했다. 두 소스를 바이트/SHA까지 복원했다. 자료는 extracted/dotnet-ai-alliance-0530-mutations/다(Git 제외). 새 x86 fixture나 golden을 만들지 않았다. AiAllianceSide는 클론 추가 기능이라 원본 L1로 세지 않으며 PORT_MAP은 port다. RawAiCreate의 기존 원본 기본 경로 L1은 유지하고 현행 소스를 재대조한 두 행의 base만382ec33fe로 올린다.

원복 뒤 최종 전체 비증분 Release 빌드는 경고0·오류0이다. 관련 Core46/46(새14·기존 AI/기술 초기화·메타데이터), Client24/24(미션 플레이어 적재/AI 공급자·메타데이터)가 실패0·건너뜀0·각 종료0으로 통과했다. 결과는 extracted/dotnet-ai-alliance-0530-final/에 보존했다. 이번 단위에서는 게임 창을 실행하지 않았다. 전체 회귀 기준은 직전0.529를 유지한다. PORT_MAP811행(L1 387+port424), fixture278개 유지. 새 외부 접속은 없다. AGENTS.md·보호 원본/CD·README 개요·cpppj·공통/다른 프로젝트 인계는 변경하지 않았다.

## 다음 작업

다음0.531에서 실제 사제 삭제/해방에 필요한 RawDamageableRelease의 현행 Native·현재 allowedGenus·판본 경계를 옮긴다. 그 뒤 일반 지역 정책/배치와 피해·삭제 전체 결선을 진행한다. AI Host 및 위 표의 상위 모듈은 해당 R4/R5/R6 작업에서 함께 잇는다. 전체 목표는 active이며 M1전체0/34, 후속 단계는 잔여다. 사용자 제외 조건인 실제 사제 포획/희생·특정 적 공격·시나리오 승리와 MP 재현은 하지 않았다.
