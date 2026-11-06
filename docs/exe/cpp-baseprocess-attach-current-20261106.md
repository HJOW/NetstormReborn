# BaseProcess 상위 부착과 생성 실패의 부분 상태 — 단위960

2026-11-06, HJOW-Athlon. AGENTS와 공통·DN 인계를 다시 읽고 LEFT_JOBS 3절4번의 상위 Process/Factory 수명을 이어갔다. 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-11-03이며 원본과 정밀 산출물은 변경하지 않았다. 제품은960/11.01이다.

## 원문 부모와 복원

PATCH `0041bd20`/CD`0048f590`은 프로세스 타입을 signed 상한과 비교해 진단하고, 정상 복귀 뒤 먼저 this+8 부모와 this+4 Form을 쓴다. PATCH는 그 뒤 저장 부모의 free/dead에서 조기 반환한다. CD는 free와 dead를 각각 진단하며 정상 복귀하면 계속한다. 이후 Kernel 등록 → 현재 Form 필드 조회 → 신규 Create 또는 지정 free Take → 타입/PID 쓰기 → 실제 VT+74 → OnAttached 순서다. 지정 Form은 Take 전 객체를 저장하고, free 분기를 마친 뒤 현재 가상 부모 getter와 PID를 늦게 진단한다. live 분기는 타입/PID를 새로 쓰지 않는다.

기존 Host는 타입/SID·지정 슬롯 범위·genus/체인·전송 장치를 선행 거부했고, 신규 Create 실패 때 Kernel 등록과 타입 회전을 되돌렸다. 프로세스 필드/Host 연결도 생성 뒤로 미뤘다. 새 AttachBaseProcess는 원문 부모를 공용으로 추출해 Attach/Reattach가 같은 순서를 사용하게 한다. 진단이 반환하면 계속하며, 이미 수행한 등록·회전·프로세스 필드·슬롯 변경은 복구하지 않는다. PATCH 회전 helper는 실제 Kernel 검색 전에 수행한다. 미지원 현재 장치와 물리 범위 밖은 소비 위치의 명시 호스트 오류로 남긴다.

신규 Create가 돌아오면 Form 필드를 먼저 쓰고 실제 setProcessType의 두 진단(0≤type≤254·현재 processFormType과 TYPE 비교)을 실행한다. 타입 진단이 복귀한 뒤 원래 입력의 BYTE를 쓰고 PID DWORD 전체만 저장한다. 지정 free 경로는 원문처럼 별도 setProcessType 진단을 호출하지 않고 Take 뒤 BYTE/PID를 직접 쓴다. 현재 Form/부모 필드는 callback 뒤 다시 읽고, 지정 슬롯의 getter가 반환한 뒤 프로세스 부모 필드를 비교한다.

실제 Host의 부착은 raw TYPE를 클래스 선택에 사용하지 않는다. 내장 Form1/3/6의 VT+74는 PATCH thunk004af160/CD004b04e0의 공통 부착이며 Process2는004b1440/CD004af380의 별도 접두다(911 근거 및 현재 보호 PE 재조회). 다른 자산 VT의 특수 +74를 전부 복원했다고 주장하지 않으며 미분류 표를 공통 부모로 대체하지 않는다. PATCH의 현재 상한/번호를 빌리고 CD 상한은 기존 초기62 계약을 유지한다. 모든 CD 동적 전역/전체10.72 이관 완료가 아니다.

C++ API의 신규 unique_ptr·이미 소유한 포인터·같은 Kernel/Host 참조는 C++ 소유권 경계다. 미등록 참조를 새 소유권으로 합성하지 않는다. PATCH 비활성 부모의 미등록 신규 입력은 기존 API처럼 nullptr로 반환한다. 원문32비트 this/필드 전체 ABI·반환 레지스터 미정값·외부 heap/실제 메모리 고갈·파생 생성자의 전체 SEH 해제/해제 메모리까지 동일하다는 증거가 아니다. 이번 생성 중단은 **검토한 부모에서 자식 진입까지의 부분 상태**이며 전체 파생 생성 실패 처리로 확대하지 않는다.

갱신 부모 PATCH0041c010/CD0048f8d0도 복원했다. 첫 현재 VT getter의 부모를 저장하고 dead/free를 각각 진단한 뒤, 각 가상 진입 전에 현재 Form 필드를 다시 읽어 Unpop과 VT+74 부착을 실행한다. 전체 전송 장치를 미리 요구하지 않는다. 수정 전 로컬 갱신 검사1개는 양판본의 두 진단에 도달하지 못해2단언이 실패했고 수정 뒤 통과했다. 원문 파일/행은 PATCH Baseprocess.cpp b0/b1·CD BaseProcess.cpp ab/ac다.

## 독립 관측과 최초 실패

제품 구현 전에 새 fixture48행을 고정했다. 양판본 × 부모 상태0/1/2/3 × 프로세스 타입46/9/ffffffff × 정상/명시 Create 진입 중단이다. 모든 입력은 신규 Form0·로컬 flags50이며 지정 수신/MP는 실행하지 않는다. 실제 부모와 setProcessType만 실행하고 Kernel/Create/가상 부착/OnAttached는 명시 장치다. 호출 순서와 프로세스 두 필드·Form 슬롯50/36바이트를 비교한다. 정상 입력은 실제 부모 ret16/스택도 확인했고 재관측48행이 일치했다. Create 중단에서 Kernel 내부·실제 할당·힙 실패·SEH는 실행한 것이 아니다.

첫 Native 도구는 cdecl Create 자식의 두 인자를 callee에서 반납해 정상 반환 때 잘못된 주소로 진행했다. 원문 ASM의 caller add esp,8에 맞춰 자식 경계만 고친 뒤 최초 유효48행을 생성했다. 최초 실패 로그는 보존했으며 유효 fixture를 덮어쓰지 않았다.

새 실제 로컬 검사2개는 수정 전 제품에서5단언이 실패했다. 생성 실패 뒤 등록/필드/회전을 남겨야 하는 검사와 타입9 진단 정상 복귀 뒤 부착을 계속해야 하는 검사다. 최초 검사 빌드의 protected Pid 접근은 공개 IsRegistered 관측으로 교정했고, 제어 빌드의 int→float 경고1건은 명시 float 입력으로 고쳤다. 최초 실패/경고는 각각 원시에 남긴다.

## 최종 검증과 남은 범위

최종 Release 게임/집중 대상은 종료0·경고/오류0이다. 첫 제품 후보에서 새4검사는 통과했고 기존 ProcessCreationCurrent의 등록 rollback 기대7단언이 실패했다. 실제 현재 World/독립 Host/현재 공장 부재의 세 검사에서 등록 수를 +1로 바꾸고 명시 반납을 추가했다. 전역 DWORD·슬롯 할당·다른 공장 거부 검사는 유지하며 기존 Native fixture 바이트는 바꾸지 않았다. ProcessAttachCurrent_ 부분 이름은 새 BaseProcessAttachCurrent_에도 일치하므로 최종 회귀는 기존 세 이름을 각각 선택해 중복을 제거했다.

| 최종 필터 | 개수 | 결과 |
| --- | ---: | --- |
| BaseProcessAttachCurrent_ | 새5 | 통과, Native48행/네 실제 VT/생성 실패/타입 진단/갱신 진단 |
| ProcessCreationCurrent_ | 기존4 | 통과 |
| ProcessAttachCurrent_의 기존 세 정확한 이름 | 기존3 | 각각 통과 |
| FormAttachCurrent_ | 기존5 | 통과, 양판본864행 포함 |
| 기본 Regular 반복/종속 체인/현재 효과 pin/최종 반납 | 기존4 | 각각 통과 |

고유21검사=새5/기존16, 실패0이다. 전체 CTest/검사 전체의 통과로 확대하지 않는다. 지정 Form/수신·일반 전투와 섞인 기존 guard 검사는 이번에 실행하지 않았으며, 예전 선행 거부 기대가 현재 부모와 다른 경우8번 범위의 정적 이관 잔여로 구별한다.

실제 새960 EXE로 기존945 MCP 도구를 재사용해 Bridge the Gap 새 진입2회·Replay2회·Continue/Abort 메인 복귀를 확인했다. 한 프로세스의2주기/4진입·앱 입력35/관측24·종료0이며 설치 사본 파일이 모두 불변이다. 사용자 저장은 이 단위 출력의 NETSTORM_USER_DATA로 격리했다. 1024×768 미션/최종 메인 PNG를 직접 열어 표시를 확인했다. 도구 summary의 unit945는 기존 도구 태그이고 실제 EXE SHA는 새960 제품이다. 사제 포획/희생·특정 적 공격/승리·공중3종 전용·신규 MP/패킷/통신·물리 입력/PCM/CD 삽입은 실행하지 않았다. 전체34미션/ZIP도 반복하지 않았다.

다음961도 같은3절4번이다. 내장4종 밖의 현재 VT+74/파생 프로세스·Factory 외부 heap/실제 할당 실패·파일 수명을 계속 대조한다. 지정 Form 분기와 직렬화/MP는 코드 반영이며 새 동작 검증을 주장하지 않는다. checking8=부모3/부분2/MP3와 완료부모39/부분4는 불변이다.

EXE SHA-256은 `9851fae7dec11fa632870b8572db92ff2d0df9cfa90634dc0d3d6e5fa91f97a4`, 11,307,520바이트다. 기존 보호2791개/fixture971개 불변·새 fixture1개로 다음 기준은2792개/972개다. [완료 JSON](../../cpppj/baseprocess-attach-current-20261106.json), [Native 보고](../../cpppj/baseprocess-attach-current-native-20261106.json)와 `extracted/cpp-resume-960/`의 원시/다음 보호 기준을 따른다. 전체 목표와3절4번은 미완료다.
