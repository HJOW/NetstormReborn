# 1013 — 복구 State7의 실제 pending Fort 이름 선택

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의00460980과004b6dd0 case7을 대조했다. 현재 이름 부모의 CD 대응은004ca6a0(strong0.659)이며 구현 판본은10.78이다.

실제 현재7/재접속 표식+84를 확인하는 Uber 진입을 만들고 ClientFortGlobals.Pending의 같은 이름을 기존SelectCurrentFortName으로 전달한다. 서비스 selectReconnectFort와 일반 State7 기본 몸체의 loadReconnectFort가 이 같은 장치를 빌리도록 연결했다. 실제 GeneralBattle는 기존처럼 모듈 준비/loaded1 뒤 Login.BeginGeneral에서 호출하므로 현재 Session의 snapshot/Notify 수명 관문을 앞당겨 우회하지 않는다. GameWorld connecting/실제phase14/Player 참여 완료 관문은 유지한다.

기존 RawFortCurrentName/FortCurrentNameDevices/같은 TemplateStorage와 실제 Session을 재사용한다. 현재 owner/manager/readonly에 따라 원문 snapshot→현재 이름→manager 파일명/Notify→snapshot/선택 old 삭제→제목/dirty/선택Config를 수행한다. 별도 파일Reload나 이름 사본 저장소를 만들지 않는다. 기존 사용자 홈/보호 작업 namespace resolver와 현재 파일 writer를 사용하며 원본/설치 폴더에 새 writer를 추가하지 않는다.

기존 명령/서비스의 이름에 있던 파일 읽기라는 주석을 실제 이름/manager 처리로 바로잡았다.1012에서 추가한 string_view 서비스의 명시 표준 헤더도 포함했다. 현재 이름/전송 API의 동작을 바꾸거나 그 parser를 다시 구현하지 않았다.

**StateReconnect/ForceServerDeath/004b8cb0의 실제 caller와 복구State7 요청/선택 인계는 아직 남는다.** 이 하위 연결을 실제 재접속/파일 이름 변경 검증 완료로 쓰지 않는다. 일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking6·한국어 후속·Edit 시나리오 UI도 이어간다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,864,576바이트, SHA-256 `447dc6606d13814be2a59ba5708d4869a163ec025059f829e2cbd392e6415e5e`다. 보고서는 `cpppj/reconnect-fort-selection-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성/실제 파일 이름 변경0이며 보호2800파일은 모두 불변이다.
