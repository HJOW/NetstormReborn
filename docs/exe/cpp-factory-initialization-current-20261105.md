# 양판본 공통 생성·수신 후처리의 현재 진단/HP (단위904)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 구현

공통 PostCreate PATCH004b0c60/CD004ae040은 Form 진단과 contained 진단 뒤 같은 현재 TYPE this/base를 저장한다. zOrder BYTE를 쓰고, 서버인 경우 owner BYTE를0으로 초기화한다. 이후 같은 슬롯의 현재 TYPE flags2/flags1과 abstract extra를 읽어 HP 필요성을 결정하며, 앞서 선택한 TYPE this의 공유 HP getter를 호출한다. 후단의 현재 contained 진단은 비서버에서도 실행한다. PATCH는 zOrder+35/owner+34/HP DWORD, CD는+33/+32/HP WORD다. CD의 서버 플래그 snapshot은 zOrder 쓰기 직전이며 PATCH는 그 뒤다.

공통 PostTake PATCH004ad4a0/CD004ae120은 Form 진단 복귀 뒤 현재 TYPE의 zOrder를 쓰고, 마지막에 현재 contained를 진단한다. free/dead는 별도 진입 조건이 아니다. 기존 CD의 LiveBytes 일괄 거부를 제거하고 양판본을 같은 실제 `SquidFactory` 부모에 연결했다. CD 진단 파일은 원문 `\ns\o\Squid.cpp`와 줄7a6/7a7/7b1·7c8/7cc를 사용한다. 선택된 진단을 사본으로 유지하므로 실행 중 연결 교체에도 호출 수명을 보존한다. 실제 메모리 범위 밖은 호스트 경계로 거부한다.

## 원문 관측과 검증

[격리 도구](../../tools/decomp_factory_initialization_current.py)는 두 부모와 실제 HasHitPoints/TYPE 플래그·HP getter/getNum·CD 직접 HP setter를 실행한다. PATCH HP 쓰기는 부모의 실제 DWORD 저장이다. 외부 진단 UI의 정상 복귀/명시 TYPE·최대HP·약화·사제번호·서버·contained 변경만 장치 경계다. 각288/총 **576행**을 최초 동결한 뒤 재관측 결과가 같았다. 두 연산·TYPE3/74·상태0/7/8/13·서버0/1·HP/abstract3종·복귀3종을 교차해 전체 슬롯과 각 진단 당시 전체 슬롯/현재 전역을 대조했다. 기대값을 C++ 모델로 생성하지 않았다.

도구 준비 첫 호출은 공용 Oracle API의 인자 이름이 잘못돼 실행 전에 실패했다. ecx를 실제 API의 this로 교정한 뒤 최초 동결했다. 그 실패 때 fixture 쓰기는0이었다.

Release 제품/집중 빌드1명령 종료0·경고/오류0이다. 정리 도우미 종료→2,864소스 snapshot→CMake를 같은 명령에서 순차 실행했고, 빌드 중 소스 편집/다른 빌드는 없었다. 최종 소스는 snapshot과 같다. 새2/기존21의 **고유23사례·6명령/23실행이 첫 실행부터 실패0**이다. 새576행/선택 장치·진단 자기 교체와 기존 PATCH 공통 생성6/상세·진단7/양판본 base·파생 생성2/anim1/공유HP5를 대조했다. 기존 순수 Take 부품 회귀와 실제 수신/패킷 검사를 구별한다.

기존 보호2,780개/960fixture는 불변이며 새fixture1개로 다음 기준은 **2,781개/961fixture**다. 기존 입력/기대값은 수정하지 않았다. 원시 로그/소스 snapshot/보호 해시/정밀 원문은 `extracted/cpp-resume-904/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/factory-initialization-current-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준을 유지한다.

제품은 **904 / 표시·파일·제품11.01**,10,732,544바이트, SHA-256 `7ec416e9a2a336d2dd1cf21a30c6606fb399835d8dda62dd04347503ec63a124`다.

## 잔여와 범위

CD Take는 진입에서 TYPE를 고정하고 contained를 거부하며 void Unpop을 건너뛴다. 원문004ab440은 현재 VT+48 뒤 현재 생성자를 조회하고 생성자 반환 객체에 void→TYPE를 쓴 뒤 그 객체의 VT+2c를 호출한다. 공통 후처리 복원만으로 이 앞단이 완료되지 않으며905에 남긴다. Factory의6/3 전용 생성과 SquidForms의 고정 Create/Take는 독립 입력 계약이 있어 일반 Create로 무조건 치환하지 않았다. 실제 caller의 현재 생성자·Form/contained 종단 연결을 이어서 대조한다.

전체 HP/Factory/Pop 완료로 세지 않는다. checking8(부모3/부분2/MP3), 완료부모39/부분4와 State/파일·객체UI/Edit·Windows·무검사MP 및4~8차의 전체 목표를 유지한다. 실제 게임/GUI/진단 UI·포획/희생/특정 적 공격/승리·신규 MP/패킷/통신·공중3종 전용 검사는0이다.
