# 공통 AI 생산 소비와 실제 건설 배치

2026-10-22, 시작 HEAD `2444b0a8 / 20261022-02 CP`. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 일치한다. 기존 `extracted/refined/originals/Netstorm.c`/`functions.tsv`와 보호10.78 PE 명령을 정적으로 읽었다. 새 디컴파일/원본 수정/게임 실행은 없으며 이전 검사/게임 중단 지시를 유지한다.

[`RawAiDeployment`](../../cpppj/src/o/RawAiDeployment.cpp)는 수집자·시설·건물 전략이 공유하는 `004106e0` 생산 소비/실제 배치와 `00410280/004102b0` 비용 비교·차감을 복원한다. 임시 공략 AI나 별도 유닛 생성 경로를 만들지 않는다.

## 원본 실행 순서

1. 같은 현재 본체 owner→Player 기술부모→mask8 생산 조각을 조회한다. 있으면 가상 삭제 flags0으로 소비하고 같은 현재 기술부모에 **flags108/attach10** 대기 조각을 생성한다. 원본 new(0x28) 성공 시에만 현재 본체224의 생산 대기시간으로 event0/TYPE46 Regular을 구성하고 같은 Kernel에 부착한다. 할당 실패해도 배치는 계속한다.
2. 생산 조각이 있으면 비용을 차감한다. 없으면 flags2의70000에서 원본 진단을 먼저 호출하고, 그 뒤 현재 flags2를 다시 읽어4200인 시설만 비용을 차감한다. 일반 비생산 타입에 임의 비용을 추가하지 않는다.
3. 비용 비교는 `ftol(cost)`의 signed 하위DWORD, 차감은 **부호를 먼저 뒤집은 `ftol(-cost)`**의 DWORD 감김 가산이다. 현재 데모 getter를 호출한 뒤 지갑을 다시 읽어1190 하한을 적용한다. SP 자체는 이 위치에서 쓰지 않으며 [경제 정리](cpp-ai-economy-20261022.md)의 원본 말미 Set에 맡긴다.
4. 현재 권한을 읽고 같은 건설 통지의`moveLocalBuilder`에1을 쓴다. 권한이 있으면 실제 `RawConstructionRequest::Request`에 **type,x,y,owner,argument,direction,flags0,time0,time0,quality0**을 전달하고 반환값을 무시한다. 비권한이면 같은 현재 기술부모에 별도 flags108 예측 조각을 만든 뒤 실제 `RawConstructionLocal::Place`의5인자를 전달한다.
5. 정상 하위 반환 뒤 이동 플래그를0으로 쓴다. 현재 flags2의800이면5b8,4200이면5c0에 각각 **현재 QWORD now+20초**를 기록한다. 하위가 예외를 던지면 앞선 실제 쓰기와 이동 플래그를 임의로 되돌리지 않는다.

실제 PE의 서버10인자와 로컬5인자를 직접 스택 추적했다. 정밀 디컴파일의 `undefined8 param_3` 표기와 빠진 인자를 그대로 옮기지 않았다. 생산 Regular의 타입은 기존 공통 Host의46이며 커서 반환이 쓰는 실제 예약/구성/Attach/ResetTask 방식과 같다. event0의 후속은 이미 연결된 `ContentFormHost::Damage` 가상5c가 처리한다.

## Session 연결

같은 Session의 생산 조회/ContentForm 생성·삭제·Player 기술부모·건설 Request·Kernel·실제 본체 지갑을 연결했다. 명시 `DeployAi(owner,type,x,y,argument,direction)`는 원본 전략이 선택한 인자를 전달하며 자동 후보 선택을 대신하지 않는다. 비권한 경로는 `RawCampaignCursor::PlaceLocal`로 기존4433b0 몸체를 직접 호출한다. 커서 Take/Drop의 추가 비용이나 사용자의 마우스 상태를 배치 입력으로 합성하지 않는다.

**00415a10 수집자 선택,00415920 무작위 후보 탐색,004108f0의 실제 로그/로컬 드롭 음향은 아직 후속이다.** 전체 전략/자동 프레임도 연결하지 않았다. 이번 실제 배치 몸체가 있어야 그 후보 선택이 생산·대기·지갑·건설까지 원본 순서로 이어질 수 있다.

## 확인 범위

새9검사는 서버 생산 소비/타이머/10인자·할당 실패·비생산 종류별 비용·진단 뒤 현재 TYPE·비권한 두 번째 예측 조각·signed 절삭/감김/현재 데모·누락 효과의 접두 보존·하위 뒤 현재 rate/비용/owner/시각, 실제 Session의 전체 MayPlace 후보/생산/Regular/지갑/건설 연결을 다룬다. **작성·컴파일만 하며 실행하지 않는다.** 합성 입력이나 작성된 통합 검사를 자연 플레이 관측으로 세지 않는다.

Release 전체 빌드는 **종료0·경고0·오류0**이다. 세션14923의 실제 종료까지 조회했고 로그를 내용 변경 없이 UTF-8로 변환했다. 빌드/실제 종료는 `extracted/cpp-ai-deployment-20261022-build1*`, 정적 PE/최종 컴파일 입력·링크 이름은 [`cpp_ai_deployment_audit.py`](../../tools/cpp_ai_deployment_audit.py)와 [신규 JSON](../../cpppj/ai-deployment-static-20261022.json)에 남겼다. **검사/CTest/게임/native 실행0**이다. 이전 전략/경제 정적 보고서는 당시 입력과 실행파일의 증거로 보존한다. 인수인계 뒤`20261022-03 CP`로 로컬 커밋하며 원격 push하지 않는다.

원본 호스트와 다른 x87 확장 정밀도/비정상 부동소수점 예외 전체, 원본 CRT/전역 진단 장치의 모든 효과와 CD10.72는 미검증이다. 물리 SID/TYPE·작은/해제 본체는 호스트 오류로 거부한다.15개 미션+Edit의 원본 동일성/자연 플레이 목표는 계속 미완료다.
