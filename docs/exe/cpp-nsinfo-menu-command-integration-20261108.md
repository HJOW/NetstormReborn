# 1161 — M4.2a NSInfo 메뉴 명령의 공통 입력 큐 접합

2026-11-08 / HJOW-Athlon. 표시·파일·제품 버전11.01. 이전1160은faf16ab7/20261108-132 CP다.

추방·옵션 정렬·값 설정·제외 추가·제외 제거 메뉴의 실제 다섯 명령은 작성기와 NativeNsInfoInput/NativeBattleInfoInput 소비자에 존재했지만, Uber의 공통 menuHooks.command 허용 목록에서 빠져 있었다. 실제 MenuGumpInput::Handle은 모든 선택을 이 callback으로 보내므로 큐에 넣기 전에 미연결 예외가 발생하는 경로였다.

같은 ClientNsInfoGlobals의 현재 kickoffCommand/sortOptionsCommand/setOptionCommand/excludeTypeCommand/restoreTypeCommand를 선택 시점에 읽어 기존 큐 관문에 포함했다. 초기 숫자를 별도 표로 복제하거나 새 known/권한/모드 관문을 더하지 않았다. RawMenuControls의 현재 명령 읽기→실제 메뉴→공통 클릭→InputFIFO→같은 NSInfo/BattleInfo Handle→기존1fh/67h·TCP11 writer 연결을 대조했다. 각 소비자의 원문 활성/권한·부분효과·재부착 조건을 유지한다.

원문47c8c0/47b8f0/47b180과 일반 입력4d62b0의 전체 C/PE를 확보하고 기존 설명 모달/옵션/참가/항해·HTML 연결을 살폈다. 그 과정에서 새 활성 잔여를 확인했다. 실제 tell.english의 QueryDecendToChalLevel Yes는 DecendToChalLevel 명령을 보내지만 현재 이름 등록/020000a4 소비자가 없다. 확인창e8만 구현돼 있으므로 M4 전체를 완료로 세지 않는다. 다음 M4.2b에서 원문4b8810/42bf40·같은 QA51기록/State19 동기 전환까지 연결한다. 7ch 수신 등 일반 LAN 전수 잔여는M6와 구별한다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 C++ 단위/MP 패킷·게임·통신/ZIP 실행0이며 사용자 제외를 유지한다. 실제 MP 메뉴 동작을 검증한 것은 아니다. 생성 디컴파일 소스 변경0, 마지막 클라이언트2026-11-03/HJOW-Athlon·R2026-11-07/같은 호스트다. 원시 근거는 extracted/cpp-resume-1161/(Git 제외), 보고서는 cpppj/nsinfo-menu-command-integration-20261108.json이다.

Netstorm.exe 13,169,664바이트/SHA-256 `d4788e7939c6563bf08e9961b00ab9cd39b4ccde17f5c0703f3708baf7166355` / R.exe 670,208바이트/SHA-256 `77bc3835f775113dfdf4384a93a8be500a95111351b815f6027cb2ed973a1270`
