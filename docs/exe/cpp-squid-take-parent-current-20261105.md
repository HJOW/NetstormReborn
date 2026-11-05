# 공통 Take의 현재 가상 호출·생성자 반환 객체 (단위905)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 실제 연결

PATCH004af610/CD004ab440은 signed SID의 서버 영역 진단 뒤 원래 객체를 선택한다. !free이면 현재 TYPE가 다를 때 두 이름의 상세를 복사하고 진단0을 호출한다. 정상 복귀 뒤 같은 객체의 현재 VT+48을 인자0으로 호출한다. void도 이 가상 호출을 건너뛰지 않는다. 그 뒤 현재 상태에서 free/dead를 지우고 현재 전체 DWORD TYPE의 생성자를 조회한다. CD는 같은 원래 BYTE에서 free만 지운 저장과 free/dead를 지운 저장을 차례로 수행한다.

생성자가 반환한 객체를 원래 SID로 바꾸지 않는다. PATCH는 반환 객체의 상태를 먼저 저장한 뒤 VT를 읽고 TYPE BYTE→저장한 상태|4를 쓴다. CD는 VT→현재 상태를 읽고 상태|4→TYPE BYTE 순서다. 두 판본 모두 저장한 VT의+2c를 호출한다. TYPE 쓰기 중 외부 변경이 있어도 저장한 VT/상태를 다른 값으로 바꾸지 않는다. 주소0 생성자는 실제 base VT 쓰기 경로다.

새 `TakeRawSquid`가 이 부모를 보관하고 같은 실제 `SquidFactory::Take`가 물리 슬롯/현재 TYPE·상세/진단·bare 생성자/가상 후처리를 제공한다. CD 진입의 선행 TYPE 고정/contained 거부·void 해제 생략을 부모에서 제거했다. 현재 알려진 Form 생성자도 bare 분배를 사용하고 반환 VT가 네 Form이면 빈+2c를 실행한다. 진단 callback과 상세 writer는 사본으로 유지해 자기 연결 교체에도 호출 중 수명을 보존한다. 풀 밖/미복원 생성자·가상 몸체/실제 자식의 선택된 장치 부재는 해당 위치에서 명시 오류로 남긴다. 할당/자유 목록/카운터는 변경하지 않는다.

RawConstructionPlace의 실제 MakeConstructionPlaceHooks도 이 같은 Factory.Take를 사용한다. 새 관측은 순수 공통 객체 부모이며 수신 패킷/통신 검사가 아니다.

## 원문 관측

[격리 도구](../../tools/decomp_squid_take_parent.py)는 두 보호 PE의 실제 Take 부모와 GetType 산술만 실행한다. 상세 formatter/writer·진단 UI·VT+48·cdecl 반환·VT+2c와 TYPE 쓰기 중 외부 VT/상태 변경은 명시 장치다. 새 관측으로 실제 가상 자식/생성자·전체 게임을 검증했다고 세지 않는다.

각360/총720행을 최초 동결하고 재관측 결과가 같았다. TYPE74/330·SID5/6000/15000/32768/0xffffffff·상태0/1/4/7/8/13·외부 변경3종·base/cdecl2종을 교차했다. 원문 읽기/쓰기 trace와 원래 객체/반환 객체 전체 바이트를 대조한다. cdecl이 다른 객체를 반환하거나 해제 중 생성자가 바뀌는 경우, TYPE 저장 중 VT/상태가 바뀌는 경우를 구별한다. 기대값은 C++ 식으로 생성하지 않았다. 초기 Native 준비 실패는0이다. 각 SID의 실제 주소 산술이 유효 합성 객체에 대응하도록 입력 pool base를 조정했다. 실제 C++ Factory의 풀 밖 접근을 허용했다는 뜻은 아니다.

Release 제품/집중 빌드1명령 종료0·경고/오류0이다. 정리 도우미 종료→2,867소스 snapshot→CMake를 같은 명령에서 순차 실행했고, 빌드 중 소스 편집/다른 빌드는 없었다. 최종 소스는 snapshot과 같다. 새3/기존31의 **고유34사례·8명령/34실행이 첫 실행부터 실패0**이다. 새720행과 실제 CD 상세/진단→현재 anim 생성자→HP WORD·owner 보존·현재 zOrder, 선택 장치 부재의 부분 상태 쓰기를 검사했다. 기존 상세/진단7·Factory8·Create부모6·초기화2·PATCH PostCreate6·건설 배치2도 통과했다.

기존 CD Take guard 검사의 성공2호출에는 같은 물리 SquidUnpop 장치를 연결했다. 원문은 void도 가상 해제에 진입하므로 해당 실제 장치가 필요하다. 기존 기대값과 관측 fixture는 변경하지 않았다.

기존 보호2,781개/961fixture는 불변이며 새fixture1개로 다음 기준은 **2,782개/962fixture**다. 원시 로그/소스 snapshot/보호 해시/정밀 원문은 `extracted/cpp-resume-905/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/squid-take-parent-current-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준을 유지한다.

제품은 **905 / 표시·파일·제품11.01**,10,745,344바이트, SHA-256 `c7d16499fa316e22d853152c7f41d411aeb971714cf59a68037a76ba6afd15e5`다. 실제 게임/GUI/진단 UI·신규 MP/패킷/통신·공중3종 전용 검사는0이다.

## 잔여

CD 공통 Unpop의 contained/TYPE 일괄 거부와 Form 내부의 기존 TYPE/VT·void 검사/부모 제거 알림 종단은 각각 실제 자식 경계에 남는다. 이번 부모 복원을 전체 Form/contained 해제 완료로 세지 않는다. 실제 알려진 bare 생성자는 같은 SID를 반환하며 미복원 외부 힙/NULL의 반환값을 정상 객체로 합성하지 않는다. 부모 토큰 모델의 다른 반환 객체 검증을 실제 외부 힙 연결 완료로 세지 않는다.

Factory의6/3 전용 Create와 SquidForms의 고정 Create/Take, 실제 ProcessForm caller의 현재 생성자/진단 연결을 이어서 대조한다. 독립 편의 계약을 실제 현재 생성자 경로와 혼동하지 않는다. 전체 HP/Factory/Pop 완료로 세지 않으며 checking8(부모3/부분2/MP3), 완료부모39/부분4와 State/파일·객체UI/Edit·Windows·무검사MP 및4~8차의 목표를 유지한다.
