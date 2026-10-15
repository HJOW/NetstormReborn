# buried 최초 Pop 복원과 실제 미션 시작 연결

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 일치한다. 분석 대상은 보호된 `originals/Netstorm.exe`의 10.78이다. 새 전체 디컴파일, 원본/클론 게임 GUI 실행과 원본 파일 수정은 없다.

## 원인과 원본 흐름

직전 중단 커밋 `463aad2`의 전체1,314개 중 유일한 실패는 `CampaignMissionStart_ProtectedEightMissions`였다. 실제 Load/Start 경계와 SID/TYPE/VT 진단을 추가해 Tutorial5의 **Load 중 SID15011, 타입112 `buried`, 가상 표0050a748**가 누락된 것을 확인했다. 해당 가상 표 +20의 실제 몸체는00462170이며 단순 공통 PostPop 주소 표에 추가해서는 안 된다. 원본 SubtleDefense.fort의 buried2개는 QA114를 저장한다.

실제00462170은 다음 순서다.

1. 현재 boss 권한 DAT00540bc4가 켜져 있고 flags의1이 있으며 loading005c89b8이0이면 `00461f00(5)`로 보상 타입을 추첨하고 `004ac220`으로 낮은 QA WORD만 쓴다. server00540bc0·전투 모드00594fbc와 구별한다.
2. flags의1이 있으면 boss 여부와 무관하게 각각 `new(0x28)`을 시도한다. 첫 성공은 Regular 사건0·지연0, 둘째 성공은 사건1·지연 float 비트3c23d70a(0.01)로 생성한다. 첫 할당 실패가 둘째 할당을 막지 않는다.
3. `004acd10`의 QA로 `0049a840`에서 타입 주소를 계산하되 반환 주소는 사용하지 않는다. 이 helper들은 여기서 관찰 가능한 쓰기나 추가 조회 효과를 만들지 않는다.
4. 같은 flags로 공통004b0d30을 호출한다.

새 `RawBuriedPostPop`은 현재 boss/로딩 주소를 빌리고, `SquidPostPop`의 파생 Validate/Activate에 같은 풀로 등록한다. 전체 몸체 안에서 같은 공통 장부/깊이를 한 번만 처리한다. Session은 첫 저장 객체 전에 실제 Regular 생성과 공통 ValidateBase/PostPopBase를 연결한다. 분기에서 선택되지 않은 효과를 요구하지 않으며 선택된 미연결 효과는 명시 오류를 유지한다.

## 독립 원본 대조

`tools/decomp_buried_postpop.py`는 실제00462170/004ac220/004acd10/0049a840 명령을 실행한다. 추첨·힙 할당·Regular 생성·공통 장부는 명시 입력/효과 경계이며 실제 하위 몸체까지 복원한 관찰로 세지 않는다. OS와 실제 게임을 실행하지 않는다.

고유 입력 **580개**는 boss2×로딩2×flags6×개별 할당4×QA3×추첨 반환2=576개와 효과 중 QA/현재 전역 변경4개다. 전체 객체50BYTE, 현재 boss/로딩, 개별 호출 인자/순서를 대조한다. 정상 반환 ESP, FS0의 SEH 복원, EBX/ESI/EDI/EBP와 허용 주소 밖 쓰기 guard를 확인했다. 원본 첫 시험은580개 정상 반환했고 `--verify`의 SHA/고유 입력 감사가 통과했다.

`BuriedPostPopTests`의 **2개 검사·실패0**으로 원본580행 전수 재생, 미선택 추첨 경계 생략·선택된 효과 실패의 부분 쓰기·다른 VT 거부를 확인했다. 공용 헤더의 초기 빌드 도중 boss 연결과 저장 QA 검사를 수정했으므로, 초기 실행 파일은 검사하지 않고 고정 소스로 다시 빌드하여 검증했다. 최종 빌드는 `extracted/goal-buried-start-final-build.log`, 집중 검사는 `extracted/goal-buried-start-focused.log`다.

미션 시작 집중3개 중 **2개 통과·1개 실패**다. 보호8미션 순회의 Tutorial5 Load는 앞선 buried를 지나 **SID15013·타입72 sunAviary·VT00509930**의 미연결 PostPop에서 중단했다. 따라서 이 단위로 Tutorial5 Load/Start가 완료됐다고 표시하지 않는다. 저장 QA114·두 Regular의 동일 부모/초기값 검사도 Load 뒤에 있으므로 아직 해당 실제 미션 입력에서 실행되지 않았다. 별도 실제1-1의 이름 인코딩/진단과 부분 쓰기 검사는 계속 통과했다.

최종 고정 소스 Release는 **종료0·경고0·오류0**, 전체 내부 **1,316개 중1,315개 통과·1개 실패**다. 유일한 실패는 위 sunAviary PostPop 미연결이다. CTest는 전체 실행 파일 하나를 등록하므로 **0/1 통과·종료8**, 검사205.15초/전체205.20초다. `extracted/goal-buried-start-{ctest,tests,audit}.log`를 보존하며 전체 회귀 통과로 표시하지 않는다. 새 native580과 기존 요구에너지2352의 SHA/고유 입력 감사·Python 구문·UTF-8·`git diff --check`는 통과했다. 요구에너지 제품/fixture는 그대로이며 CMake 등록 변경 SHA2개만 이력과 함께 갱신했다.

다음 누락 몸체는 실제 `sunAviary` PostPop **0045ec50**(CD0049b420)이다. 현재 server00540bc0와 flags1이면 기존 process 중복을 진단하고, new0x30→생성자0045d690의+20→0041bd20(type005410f0,SID,0,8)을 실행한 뒤 Damageable0044bf80을 호출한다. 비어 있는 생성 효과를 공급하거나 일반 Factory 후처리로 잘못 분배해서는 안 된다. 실제 생성/부착/Kernel 프로세스 몸체를 이어서 복원한다.

## 범위와 다음 연결

이 단위는 PATCH10.78만 복원했다. CD의 대응00416ac0은 문서에 기록하지만 검증 전 같은 코드로 지원한다고 주장하지 않는다. 비로딩 최초 생성의 실제 보상 추첨00461f00과 buried Regular 사건 몸체, 실제 GUI 로그인/Guide/Start와 자연 완주는 별도 작업이다.

AGENTS.md의 최신 규칙을 커밋 직전 다시 읽고 **`20261016-02 CP buried 최초 후처리 복원과 원본 580개 대조 및 미션 로드 누락 추적`**으로 로컬 커밋한다. 전체 목표는 active로 유지하며 단위75에서 실제 아비어리 프로세스와 남은 미션 연결을 이어간다. 이 문서와 JSON의 통합 소스/로그 SHA는 이 커밋 시점의 역사 스냅샷이다.

확장 목표의 원본 Demo 메뉴는 DemoStormRages, GuardMyBack, DissolvedAlliance 세 개다. StartDemo 분기는004d7ffb의00463b70으로 미션을 만든 다음004d8006에서 push1→00463a00으로 미션+68 데모 플래그를 쓴다. 원문 스크립트를 수정하거나 추가 `demo=1` 설정으로 대체하는 경로가 아니다. 데모 세 개의 실제 Session 시작 대조는 다음 연결 단위에서 수행한다.
