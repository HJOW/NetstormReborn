# Factory 공통 부모 범위와 firstPop 직접 조회 — 단위975

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 원본/정밀 자료는 읽기만 했다. 우선순위4의 Factory 항목을 공통 부모와 그 밖의 소유권/가상 효과로 구분해 대조한다.

## 함수별 공통 부모 대조

| 함수군 | 원문 경계 | 현재 연결과 근거 |
| --- | --- | --- |
| Create | PATCH004af530/CD004ab390 | RawSquidCreation→같은 Factory의 할당/현재 생성자/반환 객체/실제 VT+28. 899의 Native588 및 실제 World 현재 fake 번호·대여 |
| 현재 TYPE 생성/Retype | 004af720/CD004ab770·PATCH004b1620 | 현재 raw TYPE를 cdecl에 전달한다. Retype은 현재 void/foot 진단 뒤 TYPE BYTE→현재 생성자 순이다. 제한된 독립 Construct 편의 API와 이 실제 부모를 구별한다 |
| Take | 004af610/CD004ab440 | RawSquidTake→현재 VT+48/현재 생성자 반환 객체/판본별 TYPE·상태 저장/실제 VT+2c. 905의 Native720·현재 callback/부분 효과 |
| PostCreate/PostTake | 004b0c60/CD004ae040·004ad4a0/CD004ae120 | 현재 TYPE/진단 복귀·owner/zOrder/HP 폭·CD 서버 snapshot. 904의 Native576 및896/898의 현재 진단·HP |
| 공통 firstPop | 004ad470/CD004ae0f0 | 두 실제 base VT+30의 직접 몸체. extra BYTE의1/8비트만 읽는다. 이번 Native512/직접 Factory 물리3072입력 |
| 고정 Form3/6 생성·수신 | 같은 Create/Take의 TYPE3/6 | CreateContained/CreateGumpForm/GumpFormHost/contained adapter가 실제 공통 부모를 사용한다. 906/907의 현재 생성자/반환 VT·실제 대여 |
| Form Attach/Pop/Unpop | 실제 +74/+20/+48 | 같은 Factory의 현재 TYPE/Content 번호·공유 깊이·종속 분리/현재 VT를 SquidForms에 연결한다. 908~912·958~972의 공통 부모/정렬 Content/현재 클래스·실제 World/Session |

이 표는 공통 부모와 고정 Form 편의 경로의 대조다. 모든 자산/프로세스 생성자의 가상 효과·메타맵2표·외부 heap/할당 실패·전체 Factory 완료를 뜻하지 않는다. CD Retype의 별도 전체 이관도 이번 범위가 아니다. 실제 원문 cdecl을 호출하지 않는 독립 검증용 Construct의 제한과, 실제 Create/Take/ConstructCurrent 부모를 혼동하지 않는다. 물리 범위 밖·미등록 주소·void NULL 생성자의 호스트 오류를 임의의 base 생성 성공으로 바꾸지 않는다.

## 직접 firstPop과 미사용 보조 코드

실제 SquidPop 경로는 이미 해당 원문 플래그를 사용한다. 반면 직접 SquidFactory.FirstPopFlags는 LiveBytes의 예약/free/contained 검사, 자산 TYPE 범위/생성자 지원, TYPE-현재 VT 쌍 일치까지 요구했다. 원본 두 함수는 이 필드를 읽지 않는다. 직접 API를 같은 물리 슬롯의 extra BYTE 조회로 바꾸고 전용 LiveBytes를 제거했다. 실제 게임 Pop 알고리즘을 새로 바꾼 것으로 쓰지 않는다.

GumpFormHost.SetPrevious/ValidatePop는 선언/정의 외 호출이 없고 실제 Attach/Unpop은 이미 같은 Factory/SquidForms로 연결된다. 사용하지 않는 TYPE기반 previous 쓰기와 옛 전송 거부 루틴·전용 상수를 제거했다. 실제 MP 전송 writer가 구현됐다는 의미가 아니며8번 잔여를 유지한다.

PATCH4ad470은 현재 canonical 함수 목록에 누락돼 있어 보호PE의 실제 base00501dd0+30/바이트를 직접 사용했다. CD도 실제 base005058d8+30이4ae0f0이다. Ghidra의 포인터 표 시작 주소를 실제 VT 시작과 동일시하지 않았다. Native 도구는 다음 함수 전까지0x30바이트만 실행하며, 매 호출 extra 한 BYTE 읽기·무쓰기·코드/입력 불변·정상 복귀를 검사했다. 두 판본256입력씩 총512입력을 새 fixture에 동결했다.

## 검사와 제품

수정 전 새2검사 중 직접 조회의6프로필×512입력에서 **2560단언 실패**, 물리 범위 검사는 통과했다. 최종 **고유39검사(새2/기존37), 실패0**이다. 원문512반환값/물리3072조회와 전체 풀 무쓰기를 대조했다. 기존 Create588/Take720/공통 초기화576/로컬 상위 부착48의 Native1932행, 실제 World의 고정 Form/현재 genus·번호/공유 깊이와 혼합 클래스·현재 Content/Process 수명도 통과했다. Form Attach/Pop의 전송 선택 fixture를 새로 실행하지 않고 현재 로컬 World 사례만 선택했다.

새 검사의 처음 작은 풀64는 정적 준비 단계에서 원문 free-list 최소 용량 조건에 맞춰24000으로 교정했고, 그 뒤 수정 전 결과를 수집했다. 기대값은 원본 CPU 반환에서 얻었으며 기존 fixture는 바꾸지 않았다. **Release3명령(검사 준비2/최종 게임·검사1) 모두 종료0·경고0·오류0**이다. 헤더 변경 후 실제 종료까지 기다린 다음 최종 검사를 실행했다.

제품975/표시·파일·제품11.01은 **11,373,056바이트**, SHA256 `b6232ba43945042e0a18025cd5e20842252ede2478c4e147992fadce1e8f863e`다. 기존 보호2797개/fixture977개 불변·새fixture1개로 다음 기준은 **2798개/fixture978개**, `extracted/cpp-resume-975/protected-final.json`이다. [단위 보고서](../../cpppj/factory-parent-scope-20261106.json)를 따른다. 솔루션83/checking잔여8은 불변이다. 새 전용 공중 생성·프레임·행동, GUI/진단UI·MP/패킷/통신·실제 포획/희생/특정 적 공격/승리·전체CTest/34미션/ZIP은 실행하지 않았다. 마지막 GUI는968이다.

## 후속 범위

Factory 공통 부모/CD Take/contained/Form 고정 편의 경로의 함수별 대조와 전체 Factory의 외부 소유권·가상 callee를 구별한다. 다음976은 외부 heap 반환/할당 실패의 실제 caller와 파일·비멀티 미연결 소비자를 확인한다. 공통 Pop의 다른 Regular/전체 팔레트·Chat/시스템뉴스와8번의 메타맵/MP도 남아 있다. 우선순위4 전체는 미완료다.
