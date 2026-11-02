# 761 — outside 입력과 실제 정지·모달 관문

2026-11-03 HJOW-Athlon. 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위761 완료, 전체 목표 미완료다.

## 원문과 제품 변경

원문4d6e61의Guide→Gump→4d6eca release,4d8c60 모달,4d96b6 정지,4d9bbb 활성커서/4d9c33 세계마우스 분류에는 큐 사건의 공통outside 거부가 없다. UserInput/RawCampaignCursor의 해당 거부와 C/c·Tab의 초기거부를 없앴다. D/Q/H/F4/F6 등 기존 명령큐·우클릭/방향키 회전은 같은 소비자를 사용한다.

실제부모의 추가누락도 고쳤다. UserInput::Event 입구에서 release/Client 정지 때 ReadCursorOptions·활성커서·세계마우스보다 먼저 반환한다. Uber의 마지막 world/userInput/Mission 꼬리는 현재 모달이 비영이면 하위Gump반환0에도 차단한다. 선행 일반명령과 정지중ESC 선택정리는 해당관문앞에 유지했다. 이 모달 접합은 컴파일/정적 근거이며 새GUI검증은 아니다.

숫자는 초기outside거부만 제거했다. 분류의mask와 달리 원문슬롯 계산은 plain=code&~Shift로 나머지비트를 보존한다. outside 숫자16행은 Pause→Language 조회 뒤 슬롯/소리/캡처/중심 효과가0이며80BYTE가 불변이다. 안쪽숫자/Shift기호의 원래효과도 유지했다. 언어조회 호출인자ffffffff(-1)와 반환언어0을 구별한다.

OS 큐의문자/키는원래outside를붙이지않으며 Client Poll·GumpPointerBoundary·Gump자체outside·활성화/화면경계와 UserInput::UpdateCursor의현재좌표저장→outside배치갱신생략 정책은 유지했다. 물리포커스결함의 수정으로 쓰지 않는다.

## 검사 입력 교정과 최초 기록

새 include가 기존 TEST_CASE 안에 들어 있던 준비 오류를 전역EOF로 옮겼다. 첫 C++ 실행의 숫자8단언 실패는 trace장치가조회인자-1을반환언어0으로기록한 오류다. 실제원문fixture는 그대로 유지했다.

새 actualoutside 및 기존CursorActions 두검사는 준비75공장부품 부재로 입력처리전에 실패했다. 옛 ReadyConstructionInput은 parsed territories/chaff만 바꿨고,664 이후현재로더는CampaignTemplateStorage의실제 fort.sections 스트림을 읽는다. 제품로더/성공기대는바꾸지않고 검사사본의Chaff4/Terr15..34에서 대상공장Contents count와끝offset만스캔해75항목을삽입하도록 고쳤다. 저장타입은TypeNames역변환,부모124의목록10이면별도flagsBYTE를넣지않는다. 다른객체·청크·섹션바이트는보존한다. 모든계획검증→사본역순splice→RefreshObjectRecords 성공뒤에만입력을교체한다. 손상/상한/할당/재파싱실패는원래검사Fort를유지한다. 보호원본/기존fixture는수정하지않았다.

새검사scanner의unsigned/signed비교경고 C4389 하나를폭일치로고쳤다. 최초v1/v2빌드·검사로그는보존하고최종v3과구별한다. Native 첫root반복은관측루프뒤출력변수 shadow의TypeError로파일생성전에실패했다. 경로변수를분리하고출력필수/미존재guard를유지해새repeat-v2에기록했다. 실패한출력시도를성공재관측으로합산하지않는다.

## 최종 검증과 보호

원문684행/1356실행과root별도재관측의fixture전체바이트가일치했다. outside안/밖효과차이는원래숫자슬롯효과가사라지는4쌍뿐이다. 일반명령/개발자사슬은생략하고세계좌/우효과는첫분기표식에서중단한다. 실제GUI/OS/두x87정밀도관측이아니다.

최종Release경고/오류0. 회전기존3+신규C/Tab/숫자3,이력3,숫자3,actualoutside1,기존CursorActions1,빈후보/폴링1의 **집중15개/새4개/실패0**이다. 실제활성커서의inside/outside×우/좌우키/왼쪽배치8정지조합도확인했다. 새실행대상은없다.

기존보호 2672개 모두불변, 새fixture1개포함 2673개/fixture853. 다음기준은 extracted/cpp-resume-761/protected-after.json이다. 제품SHA-256 `2be37b422e18c4aa16b50800c10625c7c564a0167d798dcfaff75b4f7dac1276`. 원시build-v1/v2/v3·tests-*-v1/v2/v3·native/repeat-v2는같은extracted경로, [보고서](../../cpppj/outside-input-routing-20261103.json)는최종집중범위와최초실패를구별한다.

새게임/GUI/CLI/MP·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 760의분류전MP명령관측은그문서에별도로보존한다. 다음은762의설정명령과메인루프priority,763~768의독립준비분이다.
