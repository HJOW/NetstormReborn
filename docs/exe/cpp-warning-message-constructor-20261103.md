# 759 — kind17 공통 생성자와 NULL·실패 수명

2026-11-03 HJOW-Athlon. 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23을 유지한다. 단위759 완료, 전체 목표는 미완료다.

## 제품 변경과 원문 순서

004d0600의 공통 Text 생성→메시지 필드→현재 모드/이름 조회 뒤 kind17을 분배한다. newWarnMessage.wav(인자 전부0)→번역키 %s→현재 입력 서식→Style→float10+현재double벽시각→Idle→Fit(0,0)의 원문 순서를 같은 MessageGump와 WarningMessages 공장에 연결했다. 본문 자체를 번역하지 않는다. 기존 kind11/16과 현재 missionPost 억제도 유지한다.

한 문자열의 %s·고정 숫자 폭/정밀도·왼쪽정렬·%%를 처리한다. VS2003 CRT의 NULL %s 결과 (null), 유효한 빈 문자열, 첫NUL 뒤 바이트를 구별한다. 음향/번역이 입력을 바꾸면 서식 위치의 현재 바이트를 읽는다. 미공급 추가인자/가변폭/넓은문자/원문256BYTE 초과는 명시 오류이며 원문 메모리 훼손을 재현하지 않는다.

독립 검토에서 두 실제 접합 누락을 발견해 함께 고쳤다. 원문004d88cd는 y=0도4d3090에 그대로 전달한다. 현재 Uber의 고리 Resolve(0) 예외를 우회해 AddNull→MessageBoard::ShowText→createText→같은 생성자의 nullopt로 전달한다. 기존 Show/create/string_view와 유효한 빈 문자열 계약은 보존했다.

원문 unwind4fbf80/4fbfa0은 Text소멸004bc790→할당free 순서다. 현재 실제 공장은 생성 실패 때 등록/조각/저장소를 남겼다. 호스트 전용 textConstructed 표식으로 실제 Text생성 전/후를 구별해, 전에는 저장소만 반납하고 후에는 공통 Destroy→Text Clear2회→기본 수명표/idle→free를 실행한다. serial 누계는 되돌리지 않는다. **원문SEH는 등록표/count를 직접 지우지 않는다.** 호스트의 복구 가능한 장치/서식 예외 뒤 사용 후 해제를 방지하도록 등록표도 함께 해제하는 명시 호환 보완이다. 32비트SEH바이트 동일성으로 기록하지 않는다.

## 검증과 최초 기록

독립 원문50입력을 두x87정밀도와 반복150회 관측했다. root의 별도150회 fixture 전체 바이트도 일치했다. 생성자·실제 문자열CRT·기한 몸체를 실행했고 Text/이름/음향/번역/Style/Idle/Fit/벽시각·isdigit 분류만 명시 장치다. TLS/실제OS는 실행하지 않았다.

제품 Release 및 기존 netstorm_selection_key_input_tests 빌드 경고/오류0. MessageGump6·Warnings18·GameTimeDisplay3, **집중27개/새7개/실패0**이다. 실제 공장 NULL/빈/NUL 구별, 이름/음향/번역/서식초과/Style완성뒤벽시각/Style prefix뒤80BYTE그림표현식 실패의 등록·조각 반납, 같은20칸 재사용을 확인했다. 기존11/16·억제·트리·Draw/Idle·재진입도 포함한다.

최초 준비 Native의 isdigit TLS 거부와 소형CMake의754 fixture 정의 누락은 준비 README에 보존한다. 실제 제품 최초 빌드는 성공했다. root의 tests-time-v1.log는 잘못된 GameTime_ 필터로0개/실패1이며 v2의 올바른 GameTimeDisplay_3개 통과와 구별한다. 기대값을 약화하지 않았다. 준비본 FormatWarningMessage.inc는 함수 작성 중간 파일이며 실제 MessageGump.cpp에 함수가 포함되어 별도 복사하지 않았다.

보호 2669개 모두 불변, 새fixture1개를 더해 2670개/fixture850다. 다음 기준은 extracted/cpp-resume-759/protected-after.json이다. 제품 SHA-256은 `8e19ca20a6c5cb7dc8b8f7027994ade4f84e5b665ed9ee35e8b902b2a748fa44`다. 원시 빌드/검사/Native는 같은 extracted 경로, 커밋 요약은 [보고서](../../cpppj/warning-message-constructor-20261103.json)에 있다.

## 남은 경계

Uber NULL 분기는 실제 컴파일/정적 연결이며 이번 새 GUI/게임/CLI는0이다. 실제 PCM/물리 입력·다른 메시지종류/Challenge 이름·포획/희생·특정적공격·승리·자연완주·MP·전체C++/34/ZIP도0이다. 마지막GUI746·전체34/ZIP645·644제품 근거는 별도다. 다음760~765와 남은 비멀티/호환성을 이어 간다.
