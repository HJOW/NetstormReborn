# 실제 내용물 통지와 같은 Session의 표시 수명

작업 단위146. 기준 `9b15487`, 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 새 디컴파일·원본 게임 실행·DirectDraw 수정은 없다. 원본/AGENTS/DN은 수정하지 않았다.

원본42977f는 내용물 통지에서426080을 호출한다. 기존 InstallContents.checkCarrier는426fc0 지면 검사에 잘못 연결되어 있었다. 같은426080 StartSelectionContentPreview로 교체해 로딩/extra 관문을 지키고 실제491 Regular를 예약한다. 생성/프레임 소비를 빈 성공으로 처리하지 않았다.

같은 Session의 Carrier42a750은 현재 내용물 검색, 실제 자산 생성, 원문 WORD 저장과 다리 표면 통지, 현재 파생 소유자, 실제 Pop/표시/삭제, 기존 RawAssetRegular.DamageableHandle에 연결한다. TYPE+118/defaultFrame과+1b8/casttime은 같은 보호 자산 정의에서 보충한다. 사제의 다섯 고유 사건259~25d는 기존 파생 분배에 남기고 나머지만 원래 기본 Carrier에 위임한다. 492의4225b0과5105의426ef0은 아직 명시 미복원 경계다. 이 하위들을 성공으로 대신하지 않는다.

내용물 표시 타이머는 현재 core.processState.now와 SoundProcessState.processType을 참조한다. 다른 시계/고정47 사본을 만들지 않는다. new18에 해당하는 C++ 객체를 먼저 확보한 뒤 그 같은 원문 필드의428700 생성과 Host 부착으로 소유권을 넘긴다. 부모 가상 삭제는 실제 World.Destroy를 거쳐 자신/자기 Form/공간/표시를 정리한다. Session 해제 전에 기존 World.ClearProcesses가 먼저 실행된다.

CreateContained는 기존 RawContainedCreate/ContentFormHost의 생성·정렬 부착·현재 부모 통지를 그대로 실행하는 명령 입구다. 설치 전에 거부하며 원본의 type/argument/flags/attachFlags/quality를 바꾸지 않는다. 별도 내용물 상태/검사용 부모 알림을 만들지 않는다.

첫 실제 통합 검사는 보호 TYPE에서 표시 genus8000을 가진 첫 실제 자산을 골랐고, 이는 **너겟154/VT50dc30**이었다. 최초 실패는 너겟 Pop 후처리가 미복원이라는 오류다. 입력을 다른 타입으로 바꾸거나 허용 표를 느슨하게 만들지 않고 전체484f90을 복원했다. flags bit0에서 현재 event1을 조회하고, 없으면 첫 독립 할당→게임 난수(0,0.3)→Regular1을 만든다. 두 번째 독립 할당→게임 난수(0,0.5)→Regular3은 항상 시도한다. 각 할당 실패는 뒤의 시도나 마지막44bf80을 막지 않는다. 미리보기 flags4는 원래 예약 분기에 들어가지 않는다.

RawNuggetPostPop을 기존 자산 후처리의 실제20번째 PATCH VT로 등록했다. 새 hook은 기존 aggregate 순서를 유지하도록 뒤에 추가했다. 실제 Session은 같은 GameRandom을 공유하고, 확보한 RegularProcess를 같은 주소에서 실제 event/payload로 생성해 두 번째 할당 없이 Host에 부착한다. 초기 time은 현재 시각이며 random float는 초기 time 지연이 아닌 payload다. 실제 너겟 일반 등록 검사에서 두 프로세스/Form/부모/현재 시각/payload 구간을 확인했다. 너겟 자체의 Regular485230 프레임은 이번 예약 생성과 구분한다.

보호 PE의50dc30은 +10=4af780/+14=484f80/+18=44b9e0/+20=484f90이다. 484f80의5바이트는44b4b0으로 가는 jmp다. 이 실제 삭제 대응만 Session의 너겟 전용 분배에 연결했으며 일반 Damageable 메뉴 범위는 확장하지 않았다. 표시/Unpop의 기존 공통 VT 근거를 재사용한다.

새 decomp_nugget_postpop.py는484f90 전체를 두 정밀도로 실행한다. 조회4afb40/할당/게임 난수455930/Regular496e80/기본44bf80은 명시 경계다. 부모 분기와 할당 뒤 난수 소비 순서, 정상ret4·SEH복구·보존GP/스택canary·x87 제어/빈 스택을 확인했다. **288입력·576정상반환·23,744명령·53/64차이0**이며 별도 VT/삭제thunk 바이트도 보존했다. 기존 fixture/원본 기대값은 바꾸지 않았다. 이번 도구가 실제455930이나44bf80 전체를 새로 실행했다고 주장하지 않는다.

최종 집중은 실제 내용물 추가→491/너겟 그림→Pop/소유자/WORD/기한→실제 만료 삭제, 일반 너겟의 두 실제 예약 부착,288행 원문 재생, 기존 같은 Session 선택의 **4개**가 성공했다. 첫 통합 실패 로그focus1과 너겟 복원 뒤 성공focus2를 모두 보존한다. 빌드1~3은 종료0이며 최종 빌드3은 경고0·오류0이다.

실제 통합 검사는 같은 Host의491 Regular.RunFrame과 타이머.RunFrame을 직접 호출한다. 다른 AI/사건까지 전체 Session 프레임을 실행한 것이 아니다. 실제 Client의 자연 내용물 추가 입력/선택/비정지 우클릭이나 대상11스테이지 자연 완주·승패·저장·다음 미션/전체 동일성으로 확대하지 않는다. 다음 우선순위는 너겟 Regular485230, Carrier492/5105 큰 하위, 실제 Client 선택/DWORD 소비자와 기존 생산/경제/AI/로그인된 로컬 전송이다.

484f90의 CD10.72 정밀 대응은 현재 없다. 새 너겟 모듈은 PATCH 전용으로 CD를 거부하며 CD 복원 완료로 기록하지 않는다. 로그 접두부는 `extracted/goal-carrier-preview-session-20261017-`, 새 원본 보고서는 `cpppj/nugget-postpop.json`, 연결 보고서는 `cpppj/campaign-carrier-preview-session.json`이다. 전체 회귀/클론 진입/최종 보호·인수인계 결과는 아래에 추가한다.

최종 전체 CTest는 **1,589개·실패0·종료0·204.96초**(CTest205.00초)다. 최종 빌드3/집중4개와 같은 바이너리다. 원본 감사64종은 **53성공·기존11실패·새실패0**이며 기존11종은 이전 단위의 SHA 불일치9종/로컬 정밀 산출물 부재2종이다. 기존41보고서110현재핀만 갱신하고 이전값/과거이력을 보존했다.

최종 클론 진입5회 모두PASS/originals_unchanged=true다. thewarbegins/masterofwhirligigs는 브리핑→Play Mission→월드, tutorial1/2/3은 브리핑 범위다. 창 실행은 예외 호스트HJOW-Athlon에서 수행했다. 자연 내용물 조작/완주 검증으로 확대하지 않는다. 81157/27979/35895/15986/36954/33881/86804는 모두 종료했고 실행 대기 명령은 없다.

보호EXE2/TARC의SHA3, 기존CMake접두/과거핀/UTF-8·LF/기존fixture·AGENTS·원본·DN불변을 확인한 뒤 인수인계·로컬 커밋한다. `20261017-46 CP 실제 내용물 통지와 너겟 표시 및 만료 삭제 연결`로 커밋하며 push하지 않는다. 전체 목표는 active다.
