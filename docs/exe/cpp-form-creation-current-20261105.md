# Form3/6의 현재 생성자와 contained 생성 전역 연결 (단위906)

2026-11-05 HJOW-Athlon에서 작업했다. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 전체 cpppj 목표는 진행 중이다.

## 원문과 구현

Factory의 CreateContained(6)/CreateGumpForm(3)이 고정 VT를 직접 쓰던 우회를 제거하고 둘 다 기존 원문 공통 Create004af530/CD004ab390에 연결했다. 호출할 때 현재 TYPE 생성자를 조회하고 그 반환 객체의 실제 VT+28을 선택한다. 정상 원문6/3은 각각 ContentForm/GumpForm 생성자와 빈+28이지만, 현재 표의 다른 cdecl이나 명시 주소0을 고정 Form 생성자로 대체하지 않는다. 실제 GumpFormHost::Create와 contained 생성 어댑터도 이 공통 부모를 사용한다.

RawContainedCreate004afa70/CD004ac4d0은 현재 contained 번호00541088/CD0051c978을 읽어 공통 Create에 넘긴다. 생성 뒤 TYPE/인자 WORD를 저장한 후 현재 bridge005411a0/CD0051ca8c와 새 슬롯 TYPE를 비교해 표면을 통지한다. CD의 비교/통지는 별도 pattern setter004abb00 안에 있다. 실제 Session은 같은 Client finder의 containedType과 같은 World links.bridgeType 주소를 빌린다. 어댑터의 type!=6 선행 거부를 제거했고, 독립 입력의6/82 fallback은 유지한다. 원문 판본별 품질/플래그 쓰기 순서와 실제 ContentForm 부착 콜백은 그대로 사용한다.

기존 독립 검사10파일/23 TYPE 입력에 네 내장 Form의 실제 생성자 메타데이터를 명시했다. 보호 PE와 대조한 TypeConstructorAddress 표의1/2/3/6 주소만 공급하며 출력/기대값은 계산하지 않는다. 실제 RiftTypeTable 로더도 원문 전체 constructorAddress를 처음부터 채운다. 누락된 입력을 보충해 공통 Create의 원문 전제조건을 맞춘 것이며, 잘못된 현재 메타데이터를 생산 코드에서 고정 VT로 덮는 호환 분기는 추가하지 않았다.

## 검증 범위

신규 Native 도구/fixture는0이다. 기존 공통 Create의 독립588행/실제 Form 생성자56행·원문 Form 수명/생성·반환 관측을 사용한다. 새 검사는 양판본6/3 전용 함수와 GumpFormHost가 변경한 현재 생성자/반환 VT를 사용하는지, 주소0이 실제 base와 판본별 진단/HP에 도달하는지, contained의 빌린 현재 번호7→8 및 생성 자식 복귀 뒤 변경된 bridge를 읽는지 확인한다. source의 실제 Session 연결은 같은 장기/World 소유 주소를 사용한다.

Release 제품/집중 빌드는3명령·종료1/1/0으로 최종 성공했다. 첫 빌드는 존재하지 않는 core.links 참조, 둘째는 새 검사의 SquidForms 정의 헤더 누락으로 각각 컴파일 오류1개였다. 실제 world.Storage()->links 주소와 명시 include로 교정했다. 경고는 세 명령 모두0이며 마지막 오류0이다. 각 명령의 실제 종료 뒤에만 교정하고 다음 snapshot/빌드를 시작했다. 최종2,869소스는 세 번째 snapshot과 같고 빌드 중 소스 편집/다른 빌드는 없었다.

새3/기존41의 **고유44사례·17명령/44실행이 첫 실행부터 실패0**이다. 현재 전용 함수/주소0·진단/빌린 번호3, 원문 생성자 표1·Create 부모6·bare Form1·Form Take4·수명3·반환6·현재 반환 옵션3·내려놓기5·건설 요청6·내용물 수명4·플레이어 기술2를 대조했다. 추가 링크한 Dais/AI 파일은 컴파일만 확인하며 새 전용 검사는 실행하지 않았다.

입력 보충10파일의 기존 단언370개와 검사 이름이 모두 같음을 별도 비교했다. 기존 보호2,782개/962fixture도 불변이며 다음 기준은 그대로 **2,782개/962fixture**다. 신규 Native/fixture는0이다. 원시 로그/세 소스 snapshot/보호 해시/정밀 원문·입력 단언 비교는 `extracted/cpp-resume-906/`(Git 제외·현재 PC 전용)에 보존한다. 커밋 증거는 [JSON](../../cpppj/form-creation-current-20261105.json)이다. 활성74프로젝트/과거 전체·GUI·34미션·ZIP 기준을 유지한다.

제품은 **906 / 표시·파일·제품11.01**,10,744,832바이트, SHA-256 `af991c475eb135b2d3bf11bb11bd8558fbd027d5a4c92f59f0db1837d7979c99`다.

## 잔여

SquidProcessHost의 두 생성/재수신 caller와 GumpFormHost::Take는 아직 SquidForms의 고정 편의 경로다. 현재 World의 Factory를 장기 Host의 짧은 대여에서 조회하고 반납 뒤 옛 World 포인터를 남기지 않도록 연결해야 한다. 이번3/6 전용 함수를 ProcessForm까지 완료한 것으로 세지 않는다. 원문0041bd20/CD0048f590의Create/Take는 현재00541078/CD0051c968을 사용한다. Form/contained 가상 해제의 현재 TYPE/진단·부모 알림, CD Unpop과 RawContainedCreate의 다른 실제 가상 부착/오류/수명 경계도 남는다.

전체 HP/Factory/Pop 완료로 세지 않는다. checking8(부모3/부분2/MP3), 완료부모39/부분4와 State/파일·객체UI/Edit·Windows·무검사MP 및4~8차의 전체 목표를 유지한다. 실제 게임/GUI/진단 UI·포획/희생/특정 적 공격/승리·신규 MP/패킷/통신·공중3종 전용 검사는 실행하지 않는다.
