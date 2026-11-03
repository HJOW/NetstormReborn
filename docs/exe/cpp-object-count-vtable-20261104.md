# 객체 메뉴의 현재 가상80 분배와 TYPE 생성자 독립성

단위837 기록이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**으로 현재 호스트와 같다. 직전836은 `98651032 / 20261104-18 CP`다.

원문004ada70 Summary는 owner/TYPE를 저장한 뒤 현재 인스턴스 VT+80을 호출하고 저장한 입력으로 설명을 작성한다. TYPE의 constructorAddress는 이후 생성/Retype의 정보이며 기존 객체의 VT를 결정하지 않는다. Session의 object.count가 두 VT의 쌍 일치를 강제하던 차이를 제거했다. 현재 지원 생성 VT만 확인하고 실제 공통 레벨/제단/다이스 몸체를 선택한다. 알 수 없는 VT는 임의 기본값으로 대체하지 않는다. 회수·정보가 같은 actualCount를 공유한다.

보호 PE의 PATCH 자산83개 VT+80은 공통004ad0f0 81개, Altar005076b8→0044a480, Dais00507950→004478f0이다. 이 정적 확인은 새 독립 CPU 입력 관측이 아니다.

실제 같은 Session/UI에서 windFactory의 TYPE 생성자만0/미지원 주소로 바꾸고 rawVT를 보존한 Summary가 Level III/IV를 출력하는지 검사했다. 제단/다이스는 실제 factory의 void 객체와 Allocate 초기 frame0만 사용했다. 실제 일반 finder에 다이스를 명시 배치해 정상 Dais VT의 Count1과 공통 state레벨7을 구별했다. Pop/Frame/의식/희생/포획/전투 함수는 실행하지 않았다. 미확인 VT는 완성된 메뉴에서 Summary의 명시 실패·객체 전체 바이트/기존 출력 불변과 정상 닫기·재개를 검사했다.

새3개와 기존 회수/설치관문·GumpForm 닫기·현재World 메뉴3개를 합해 **집중6개 최종 실패0**이다. 최초 새3개 중 두 핵심은 통과했으나, 제가 미확인 VT를 메뉴 builder callback 안에서 발생시켜 별개 controlled stack 예외 복구까지 요구한 검사에서2단언이 실패했다. 첫 소스/로그를 보존하고 완성된 메뉴의 count실패 경계로 검사 계약을 분리했다. 제품 코드는 바꾸지 않았다. 최종 새3개가 모두 통과했으며 기존3개 로그도 같은 제품 결과다. Release2회 종료0·경고/오류0이다.

**전체 가상80/메뉴 완료가 아니다.** Altar0044a480은 찾은 다이스의 현재 VT+80에 다시 위임하지만 현재 RawAltarSacrifice::Count는 temple_.Count를 직접 호출한다. 다음838에서 실제 Session의 여러 가상80 소비자와 이 중첩 경계를 공통 분배기로 연결한다. RawSquidLevel.Has의 free guard, RawTempleRitual.Count의 TYPE165 guard도 별도 현행 경계다. checking14=부모8+부분3+MP3·완료부모34/부분3은 유지한다.

제품837/11.01 EXE SHA-256은 `ec2360c404074c509f441381318a999e07979c2c707bc764726897420ec6eb55`다. 보호2759개/fixture939개 변경0·새fixture/CPU0이다. 원시는 `extracted/cpp-resume-837/`, 상세 해시는 [837 JSON](../../cpppj/object-count-vtable-20261104.json)에 있다. 실제 게임/GUI·물리/PCM/CD삽입·포획/희생/특정 적 공격/승리·신규MP/통신·공중3종 전용 검사0이며 전체71/CTest/34미션/ZIP을 반복하지 않았다. push/DN/WB 변경0이다.
