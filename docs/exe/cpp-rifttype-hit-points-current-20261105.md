# TYPE HP getter의 현재 진단 복귀와 공장/장기 번호 연결 (단위898)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이며 이번 단위는 양판본 HP getter 몸체와 실제 Factory caller의 연결이다.

## 원문과 실제 연결

PATCH0049fce0은 checking005e4794가 비영이고 TYPE flags1의HP비트가 없을 때 그 시점의 TYPE base로 getNum을 구해 현재 dais005412ec와 비교한다. 다르면 원문식/`\Ns\O\Rifttype.cpp`/1e3을 진단하며, 정상 복귀 뒤 **현재 약화005ca8f0·현재 base·현재 priest005412d0**를 다시 읽는다. 처음 선택한 TYPE this의 현재HP를 반환하며 특별 TYPE이면 signed HP/4(음수도0방향 절삭)다. 진단이 raw SID의 TYPE나 표 기준을 바꿔도 새 TYPE의HP를 다시 선택하지 않는다. CD00444720은 checking/dais 진단 없이 현재 약화00511c90/번호/0051cbbc만 비교한다.

새 `ReadRiftHitPoints`가 이 몸체를 공유한다. 같은 물리 Record 배열의 유효한 겹치는 뷰로 표 기준이 이동한 경우 `CurrentRiftTypeNumber`가 상대 번호를 다시 계산한다. -1 같은 음수도 DWORD 비트로 비교한다. 호스트 Record 한 칸을 원문의500/468바이트 한 칸에 대응시키며, 서로 다른 할당 영역이나 물리 범위 밖을 임의 원본 주소로 해석하지 않고 명시적 호스트 오류로 남긴다. 프로그램의 TYPE 배열 크기를 늘려 오류를 숨기지 않았다.

Factory의 PATCH/CD PostCreate는 처음 선택한 TYPE와 물리 표 기준을 저장해 같은 getter로 전달한다. 기존 World/Session 진단 장치에 현재 checking을 추가하고 장기 사제/dais TYPE와 약화 주소를 빌린다. 부모의 zOrder/owner 쓰기 뒤 선택된 HP 장치가 실패해도 앞의 실제 쓰기는 남는다. 현재표 바뀜/정상진단복귀/자기callback교체를 사전 거부나 성공 상수로 바꾸지 않았다.

사제 TYPE005412d0/CD0051cbbc는 최초158인 `RawGameAuxState.PriestType`에 장기 소유시켰다. 실제 Session의 `RawPriestWorldState.movement.priest`가 같은 주소를 참조하므로 기존 이동/회복/보상 참조와 Factory가 같은값을 읽는다. 일반 월드 준비나 세션 교체로 이 번호를158로 다시 쓰지 않는다. 독립 기본 입력은 기존 자체158을 가지며, 값복사는 독립 저장소를 만들고 명시 대입은 목적지 참조를 유지한다. 일반 World의 기존 약화0 초기화는 유지했고 검사에서 그 뒤 명시 현재 옵션을 공급했다.

## 원본 관측과 검증

[격리 도구](../../tools/decomp_rifttype_hit_points.py)는 양판본 getter/getNum의 실제 정수 명령을 실행한다. 외부 진단 UI만 원문식/소스/줄과 명시 상태 변경/정상 복귀 경계로 둔다. 실제 게임/OS API/창/MP/의식/공중3종을 실행하지 않는다. 프로그램 쓰기는 스택만 허용하고 ret/ESP, getNum의 실제 base 읽기 횟수와 반환/진단/HP·전역 후상태를 기록했다.

각판본580입력(일반 교차576+진단복귀3+음수번호1), 합계 **1160행**의 최초 관측과 재관측이 동일했다. HP의INT_MIN/음수/0/양수/INT_MAX, 현재 번호0/74/158, HP비트, DWORD 상위비트 비영, 특별 TYPE의 전체DWORD 비교, 진단 뒤 같은 this의HP/약화/base±1 변경을 구별한다. CD에서는 진단 프로필이 발동하지 않는 것도 독립 결과다. 기대값은 C++ 구현으로 만들지 않았다.

새5사례는1160행/두판본 전체 대조, 실제 Factory의 raw TYPE/표기준 변경 뒤 저장this HP, 누락 진단의 부분 쓰기, 실제 World 두대여/장기 번호/현재checking, 이동 상태의 복사/대입 참조 수명이다. 기존 부모PostCreate6/생성8/이동종류1/현재HP표5/보상번호7, 총 **32고유사례(새5/기존27)·6명령/32실행·첫 실패0·실패 단언0**이다. 모든 기존 입력/fixture/관측 기대값은 불변이다.

제품/집중 Release 빌드2회는 각각 종료0·경고/오류0이다. 최초 빌드도 실패가 없었으며, 완료 뒤 정적 읽기순서 감사에서 getNum 호출과 현재 비교값 로드를 별도 문장으로 명시해 최종 빌드를 수행했다. 빌드가 진행 중일 때 소스를 편집하거나 다른 빌드를 시작하지 않았다. 최종 소스는 두번째 빌드 직전 snapshot과 같다.

## 범위·인계

원문 C에는 PATCH19호출점/11부모, CD19호출점/12부모가 있다. 이번에는 getter 몸체와 Factory PostCreate를 연결했으며 **다른 호출점 전체가 같은 helper에 통합됐다고 세지 않는다**. SquidReward/전송/정보 등의 나머지 HP 조회·독립계약/현재진단은 별도 후속이다. Create/Construct·CD Take/contained/Form과 Factory 전체도 미완료다. checking8(부모3/부분2/MP3), 완료부모39/부분4를 유지한다. GUI/전체검사/34미션/ZIP·실제 포획/희생/공격/승리·사용자 직접 확인·MP/패킷/통신·공중3종 전용·물리입력/PCM/CD재시도는 실행하지 않았다.

보호된 기존2775개/955fixture가 불변이며 새fixture1개로 다음 기준은 **2776개/956fixture**다. 새 fixture는107,011바이트다. 원시 로그/소스 snapshot/보호해시/원문·기계어/호출자 감사는 `extracted/cpp-resume-898/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/rifttype-hit-points-current-20261105.json)이다. 활성74프로젝트 구성과 과거 전체/GUI/34/ZIP 기준은 유지한다.

제품은 **898 / 표시·파일·제품11.01**,10,702,336바이트, SHA-256 `2c3d4d79eaba6f61a529404c4b705d6a54509b8830564cea75bdce1aec41f7a2`다. 다음899는 Create004af530의 금지TYPE 진단→할당→현재TYPE 조회/생성자/가상초기화와 Construct004af720의 현재 raw TYPE 분배다. 이후 다른HP caller·수명/UI/Edit/Windows·무검사MP와4~8차를 계속한다.
