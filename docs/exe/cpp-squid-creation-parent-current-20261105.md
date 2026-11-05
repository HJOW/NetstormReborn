# Create 부모와 현재 TYPE 생성자 연결 (단위899)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 실제 연결

PATCH004af530은 현재005412e0과 전체DWORD TYPE 인자를 비교해 원문식/`\Ns\O\Squid.cpp`/189를 진단한다. 정상 복귀 뒤 실제 할당004af1d0을 호출하고, 반환 SID를 **signed32**로23001과 비교해 선택된191/194 진단을 수행한다. 그 뒤 현재 TYPE 표에서 생성자를 읽는다. CD004ab390은 fake TYPE 진단 없이 실제 할당004aae40을 먼저 호출하며 signed14000/188·18b와 `\ns\o\Squid.cpp`를 사용한다.

주소0은 원문의 base 생성자 VT 저장이고, 비영 cdecl은 자신이 돌려준 객체를 사용한다. 할당 SID의 슬롯을 무조건 결과로 고르지 않는다. 원문 ASM에서 양판본 모두 **반환 객체의 VT DWORD 읽기 → TYPE BYTE 저장 → 저장한 VT의+28 호출**이다. +28 함수 포인터 메모리 읽기까지 TYPE 저장 전으로 옮긴다는 뜻은 아니다. CD 디컴파일 C의 표시 순서와 달라 실제 ASM004ab426/428/42a/42d를 따른다.

새 `CreateRawSquid`가 이 부모 순서만 보관한다. 실제 `SquidFactory::Create`는 같은 실제 SidPool의 Allocate, 현재 TYPE, 복원된 bare 생성자, 반환 슬롯과 가상 후처리를 연결한다. 미복원 생성자/배열 밖 TYPE에서 멈추더라도 앞서 수행한 할당은 남고, 아직 도달하지 않은 TYPE BYTE 저장과 후처리는 실행하지 않는다. 선택되지 않은 장치를 미리 요구하거나 실패를 성공 SID로 바꾸지 않는다.

자산 base/recipe VT83개(PATCH)·68개(CD)의 실제+28은 각각004b0c60/004ae040이다. 네 내장 Form1/2/3/6은 실제 생성자 VT와 빈+28인004437f0/004b0470(PE 바이트c3)을 사용한다. 따라서 자산 번호의 TYPE라도 현재 생성자가 Form VT를 남기면 자산용 HP/깊이 초기화를 호출하지 않는다. 가상 분배는 원래 TYPE와 VT의 쌍 일치를 강제하지 않는다.

`ConstructCurrent`는004af720/CD004ab770처럼 현재 raw TYPE BYTE에서 현재 생성자를 조회해 bare 쓰기만 한다. free/dead/contained·예약 SID·TYPE<70를 선행 제외하지 않고, 할당/TYPE BYTE/가상 후처리를 추가하지 않는다. cdecl 반환 객체는 원문처럼 무시한다. Retype004b1620의 실제 꼬리가 이 경로를 사용한다. 기존 명시 인자 `Construct(type,sid)`는 독립 편의 API로 보존하며 원문 현재 TYPE 부모와 구별한다.

현재 fakeThreeByThreeSurface TYPE005412e0은 최초162인 `RawGameAuxState.FakeSurfaceType`에 장기 소유시켰다. 실제 World의 공장이 같은 주소를 빌리고 일반 월드 준비·World/Session 교체로162를 다시 쓰지 않는다. 임시 DWORD나 다른 owner 주소의 재연결은 거부한다.

## 관측과 검증

[격리 도구](../../tools/decomp_squid_creation_parent.py)는 양판본 Create 부모의 실제 명령/TYPE 멤버 읽기/진단 인자/객체 선택/VT 읽기/TYPE 저장/가상 호출을 실행한다. 각판본294행·합계588행의 최초 관측과 읽기 전용 재관측이 동일했다. 입력은 signed SID 경계·음수 비트, 전체DWORD TYPE330의 하위BYTE74 저장, flags 전체값, fake TYPE 진단, 할당 중 현재 생성자 변경, 다른 반환 객체, TYPE 저장 중 VT 교체, base fallback이다.

이 도구의 **할당 반환·cdecl 반환/초기VT·가상 자식 효과·외부 진단 UI·TYPE 저장 중 VT 교체는 명시 장치 경계**다. 실제 게임·창·OS·네트워크나 미복원 자식을 실행한 것으로 세지 않는다. 실제 Allocate/자산 생성자/공통 후처리의 구성 검증은 기존 동결 fixture를 사용하는 검사로 구분한다. 원문 부모의 기대값은 C++ 구현으로 만들지 않았다.

기존 CreationTests의 잘못된 선행 거부 가정 두 곳을 원문 순서에 맞게 수정했다. 일반Create의 Form1은 성공하고, 잘못된 TYPE/미연결·void 생성자 오류는 할당 후 보고된다. 별도 Construct/Take의 기존 경계는 유지한다. 기존 원문 fixture/입력/관측 기대값은 수정하지 않았다.

최종 집중 검사는 **33고유사례(새6/기존27)·실패0**이다. 최초33사례에서는 CD의 VT 읽기를 TYPE 저장 뒤로 옮긴 구현 때문에 원문 추적 한 사례에서294단언이 실패했다. 반환 슬롯의 바이트는 같았으나 독립 원문 추적이 차이를 발견했다. 원문 fixture는 보존하고 실제 ASM의 공통 순서로 C++와 null 반환 경계 검사를 수정했다. 기존 생성8/후처리6/전환5·Retype1/생성자표1/HP5/VT범위1을 함께 재실행했다. 총16검사명령·66사례 실행이며 최종 실패 단언0이다.

제품/집중 Release 빌드3명령은 각각 종료0·경고/오류0·컴파일 실패0이다. 첫 빌드는 줄바꿈 보존 도우미의 종료를 확인하기 전에 시작해 두 작업이 겹쳤다. 줄바꿈 외 의미 변경은 없었고, 둘의 종료 후2,851소스의 정규화 해시를 고정·대조해 두번째 빌드를 수행했다. 그 뒤 CD 순서를 수정하고 별도 snapshot을 고정해 세번째 빌드와 최종 검사를 완료했다. 최종 소스는 세번째 빌드 직전 snapshot과 같다. 첫 동시 진행을 소스 편집0으로 기록하지 않는다.

기존 보호2,776개/956fixture가 불변이며 새fixture1개로 다음 기준은 **2,777개/957fixture**다. 원시 로그/소스 snapshot/보호 해시/원문·ASM/VT 감사는 `extracted/cpp-resume-899/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/squid-creation-parent-current-20261105.json)이다. 활성74프로젝트와 과거 전체/GUI/34미션/ZIP 기준은 유지한다.

제품은 **899 / 표시·파일·제품11.01**,10,712,576바이트, SHA-256 `85979e0ce847c2c35c3f9e571c5dfbb623d7ac195b83ecc619cc8b722b9c82f1`다.

## 범위와 다음 작업

이번 단위는 두 Create 부모와 현재 TYPE 생성자/실제 공장 연결이다. 전체 Factory/Pop 완료로 세지 않으며 checking8(부모3/부분2/MP3), 완료부모39/부분4를 유지한다. CD Take/contained/Form의 진단 복귀와 남은 HP 호출점은 후속이다. 전용 CreateContained/CreateGumpForm은 기존 고정 Form 편의 경로를 유지하므로 현재 TYPE 표 변경의 일반Create와 동일 계약이라고 주장하지 않는다.

실제 공장의 결과는 같은 물리 SidPool에 표현 가능한 복원된 base/자산/Form 객체다. void NULL004429e0은 ConstructCurrent에서는 원문처럼 아무 쓰기 없이 복귀하지만, Create의 객체 반환으로 입력 SID를 위조하지 않고 선택된 반환 경계에서 오류를 보고한다. 미연결 Process/외부 힙 객체 생성자, 원본 메모리 밖 주소/NULL 역참조/재배치된 풀의 모든 주소 영역은 호스트 한계로 남긴다. 부모 장치 검사에서 다른 반환 객체를 검증했다고 이 자식들의 복원 완료로 세지 않는다.

다음900은 보상 쪽 TYPE HP 조회를 같은 getter/현재 checking·진단 장치에 연결하고 다른 HP 호출점을 추적한다. 이후 CD Take/contained/Form·State/파일·객체UI/Edit·Windows·무검사MP와4~8차를 계속한다. 실제 포획/희생/특정 적 공격/승리·사용자 직접 확인·MP 신규 검사·공중3종 전용·전체/GUI/34미션/ZIP·물리 입력/PCM/CD 재시도는 제외한다.
