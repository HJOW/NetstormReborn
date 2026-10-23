# State 정리의 점유 Chat 기록 반납

2026-10-24 단위389 기록이다. 기준은 **c1b8fbf4 / 20261024-11 CP**이며 현재 호스트와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. 단위388의 전체2652검사·자연Early1/Early2·34진입·두 비율 메뉴는 당시 제품의 결과로 유지한다.

## 원문과 같은 저장소

현재 정밀 `Netstorm.c`의 `FUN_00445140`은00558ed0부터0055a18f까지100개 기록을48BYTE씩 순회한다. 각 행 +2c의 DWORD Subscriber가 비0일 때만 그 행의12DWORD 전체를0으로 쓴다. 빈 행의 stale 이름/flags/padding을 지우지 않는다. My00540c74와 Login0054265c의 직접 쓰기나 읽기, 다른 함수 호출은 없다.

`RawChatChannels`는 이미 같은00558ed0·100×48BYTE와 이름40/flags+28/Subscriber+2c를 명시한다. `Client::ChatState::records`가 실제 단일 저장소이며 모든 AI 서비스가 같은 `client.ChatChannels()`를 사용한다. 새 저장소를 만들지 않고 기존 객체에 `ResetOccupied()`를 추가했다. 기존 `Clear(mask)`는 점유 행의 flags 한 BYTE만 바꾸므로 이 전체 정리를 대신하지 못한다. 두 기능을 합치거나 기존 Clear 동작을 바꾸지 않았다.

SourceMap의 CD10.72 후보는 **004d7db0**이며 당시 대응 신뢰도는 낮은0.31이다. 후보를 확정 대응으로 쓰지 않았고 CD를 새로 실행하거나 PATCH 몸체로 대체하지 않았다. 새 디컴파일/보호 원문 변경도 없다.

## 제품 전 독립 관측

새 [Native 도구](../../tools/decomp_chat_state_clear.py)는 제품 작성 전에 실제00445140의 **37BYTE**만 허용해 PE 격리 실행으로 관측했다. [최초 보고서](../../cpppj/chat-state-clear-native-20261024.json)와 [fixture](../../cpppj/tests/fixtures/chat-state-clear-x86.tsv)에 **108입력×027f/037f 두 정밀도=216관측**을 고정했다. 보고서의 `productWritten=false`는 그 최초 시점의 기록이며 이후 구현 결과로 덮어쓰지 않는다.

입력은 전체 빈 행·전체 점유·짝수/희소·첫/마지막·중복 ID·각 DWORD의 높은 BYTE·80000000/ffffffff와 모든100위치의 단독 점유다. 각 입력의 나머지 BYTE는 비영 패턴이다. 전체4800BYTE 출력, 양쪽16BYTE guard, 정상 반환/ESP+4, 보존 GP와 현재 x87 제어어, My/Login 불변을 확인했다. 게임/GUI/OS 진입과 외부 성공 대체는0이다. 원문이 점유 행마다12DWORD를 쓰는 실제 횟수와 명령 수도 따로 보존했다. C++ 결과로 원문 기대를 생성하지 않았다.

도구의 `--observe`는 다른 호스트와 기존 파일 덮어쓰기를 거부한다. `--verify`는 최초 입력/도구/fixture SHA와216행을 읽기 전용으로 확인하며 게임이나 PE 명령도 실행하지 않는다.

## 실제 구현과 검증

신규3검사를 공용 등록했다. 첫 검사는216관측의 전체 BYTE를 각각 Native 기대에 대조하고 원문에 없는 현재 My 조회나 이름/Refresh 효과가 발생하면 실패한다. 둘째는 mask 정리와 전체 정리의 차이, 빈 행/padding·양쪽 guard·반복 호출을 검사한다. 셋째는 Run 없는 실제 Client의 단일 기록 주소·AI 두 소비자의 반납 후 첫 행 재사용과 My/Login 불변을 확인한다.

configure1과 Release build1은 종료0·관측 경고오류0이다. 신규3·기존 Chat5·Subscriber9, 합계 **17검사/실패0**을 통과했다. 전체 CTest1도 **2655검사/실패0·종료0**이다. 검사316.59초/전체316.65초, wrapper의 CTest 실행 측정은316.85초다. 원본 전체 SHA 읽기는 그 실행 측정 전후에 별도로 수행했다. 내부 전체 출력은 다음 CTest가 바꾸지 못하도록 별도 보존했다. 원시는 `extracted/cpp-chat-state-clear-389-*`에 있다.

최종 제품SHA는 `b51ee2d9c796894f62de7d32ea602bb4d94a23333923d077e43f2c571869db82`, 검사SHA는 `8f337dc10f6ba20a6d6f18a98fcfa4598a9cc7ea1044861c61cb588be999dff8`이며 전체 검사 전후 같다. 원본 **1394/1394 동일**과 AGENTS·원본/CD·닷넷/웹의 추적 변경0을 확인했다. [새 제품 보고서](../../cpppj/chat-state-clear-runtime-20261024.json)에 소스9개와 실제 증거6개의 SHA, Native108쌍의 두 정밀도 출력 동일성을 기록했다. 최초 Native 보고서/fixture는 변경하지 않았다.

엄격 UTF-8·AST·로컬 링크·원시 SHA와 선택 stage/공백을 확인해 `20261024-12 CP`로 커밋한다. 원격 push는 하지 않는다.

최초 Native 보고서/fixture/도구3경로는 `.gitattributes`의 `-text`로 체크아웃 줄끝 변환을 막는다. 작업본과 실제 Git 인덱스의 원시 바이트를 각각 대조하며 Native 내용 자체를 다시 작성하지 않는다.

이번 변경은 기존 클래스의 비가상 메서드 추가이며 기존 Add/Clear/현재 Subscriber 경로 본문을 수정하지 않았다. State 정리 부모의 호출 배선은 아직 없고 이번 제품의 새 GUI 실행도0이다. 이전388 제품의 GUI 결과를 새 실행으로 세지 않는다.

## 계속 남은 일

[State 정리 재사용 표](cpp-state-world-cleanup-reuse-20261024.md)의00445140은 이번에 같은 실제 저장소의 자식 몸체를 확보했다. 전체34개 자식과 추가 hook의 부모 접합이 완료됐다는 뜻은 아니다. 전송 표/정지/커서/UI/Kernel/Core0/Pool/Player 재구성·SP 순서와 실제 Client 수명을 이어야 한다.0054d4e8의 `PlayerNoMake` 중복 경고 표 owner도 여전히 미확인이다. 최종 Edit 및 전체 Early/캠페인/Demo 사용자 완주·완전 동일성 목표는 계속 활성 상태다.
