# 채팅 대상 이름 검사 — 단위570

원본10.78의 `0042dc70→0042d910` 검사 부모를 [ChatRecipients](../../cpppj/src/client/ChatRecipients.h)로 복원했다. **독립 원문350관측·실제 원본 Regexp 보강350관측 모두 차이0, Release 검사 빌드 경고·오류0, 관련15개 검사 실패0**이다. [원문 관측](../../cpppj/chat-recipients-native-20261029.json)·[Regexp 보강](../../cpppj/chat-recipients-regex-native-20261029.json)·[검증 집계](../../cpppj/chat-recipients-runtime-20261029.json)를 따른다.

**실제 Client/Console/ChatWindow의 대상 callback과 현재 PlayerFinder 연결은 아직 하지 않았다.** 재사용할 검사 처리기의 복원이며 게임·송수신·재접속을 검증한 결과가 아니다. 게임 실행파일은568제품/SHA `d45745a61e448a77674eec4248ee566070592afd60da5627c4d1ad2e33b9206e` 그대로다.

## 원문 동작과 구현

`00548714`가0이면 입력을 읽지 않고false다. 비영이라도 NULL 입력은false지만 빈 문자열과 CR/LF/TAB/SPACE뿐인 문자열은true다. CRT 생성자의 패턴 포인터 `005049e8`은 `^[CR LF TAB SPACE]*([^CR LF TAB SPACE]+)(.*)`이며 정확한 네 구분자만 사용한다. VT·FF·NBSP를 공백으로 확대하지 않는다.

최초 `00484190`은60BYTE PlayerFinder의 owner/subscriber/첫 이름BYTE/주소/mode0만 쓴다. 매 토큰의 `004841b0`은 같은 선택 필드만 지우고 mode와 이름 뒤 잔여를 보존한다. `00495a10(...,1,...,40,0)`은 최대40BYTE 복사 뒤NUL을 쓴다. 40글자일 때NUL은 인접 주소DWORD의 첫BYTE다. 지역을 늘리거나39글자로 바꾸지 않았다. 다음 검색은 절단된 이름 끝이 아니라 원래 캡처2 시작점에서 이어진다.

먼저 현재 Identity `004841c0`을 조회하고 실패 때만 채널표 `00531768`의 첫NULL 앞8포인터를 순서대로 번역/비교한다. 순서는 `빈 키, 빈 키, 빈 키, BATTLE, 빈 키, EVERYONE, ALLIES, OBSERVER`다. 빈 키를 건너뛰지 않고 C locale ASCII 대소문자 비교를 한다. 번역 callback 뒤 같은 지역 이름을 다시 읽으므로 Identity의 이름 변경·하위 중단·부분쓰기가 남는다.

`CheckChatRecipients(manager,text,scratch,hooks)`는 명시 초기60BYTE와 현재 Identity/번역 장치를 빌린다. 고정 표현식만 직역했으며 원본 공유 Regexp 객체 전체·재귀 검색 상태·범용 API를 구현한 것은 아니다. 다른 호출자와의 공유는 실제 Client 접합 때 함께 검토한다.

## 독립 관측

[최초 도구](../../tools/decomp_chat_recipients.py)는 제품 작성 전에350행을 고정했다. 정상327·번역 중단19·두 번째 Identity 중단4, 실제34,244명령이다. 상위 두 부모·PlayerFinder 초기화/재초기화·캡처/오프셋은 실제 CPU다. 검색은 보호 PE의 고정 패턴을 대조한 명시 Python 장치이며 Identity 응답/이름 변경·번역·stricmp도 외부 장치다. 정상반환의 스택과 비영 EBX/ESI/EDI/EBP 보존도 확인했다. `--verify`의350행 재관측 차이0이다.

[보강 도구](../../tools/decomp_chat_recipients_regex.py)는 실제 `00496c90→00496b20` 컴파일러와 `004964e0` 이하 검색 엔진을 실행했다. **같은350행의 반환·60BYTE·전체 Identity/번역 순서가 모두 같았다.** 이 실행에는 Python regex 대체가 없다. malloc/CRT strchr·isalnum·strncmp·stricmp, Identity·번역만 명시 장치다. 원문 `00495ba0`의 dot 반복도NUL까지 읽는다. 같은 입력의 재검증이므로 고유700입력으로 합산하지 않는다.

최초 도구의 raw 문자열 표기와 실제 CR/LF/TAB 바이트 비교 오류, 교정 뒤 `Path` import 누락은 fixture 작성 전에 발생했다. `native-first.log`·`native-second.log`를 보존했고 제품 결과로 기대값을 바꾸지 않았다. 보강 도구 첫 실행은350행/차이0이었다.

## C++ 검증과 인계

| 필터 | 결과 |
| --- | --- |
| ChatRecipients | 신규2개/실패0. 최초350행의 결과·지역·순서·중단, 미선택 장치·구분자 |
| ChatWindow | 기존9개/실패0. 부모와 실제 공통 UI 공장·수명·callback |
| PlayerIdentity | 기존4개/실패0. 현재 검색·부분완료·순서 |

고유15개, 등록3,437개다. 최초 C++ 대조도 통과했고 재초기화의mode 보존을 명시 helper로 정밀화한 최종 빌드/집중 검사도 통과했다. Release 검사 target 두 빌드 경고·오류0이다. 기존fixture723개 불변/신규1개로 현재724개이며 보호2,583파일(원본/CD·AGENTS.md·정밀 산출물·기존fixture)의 해시 차이는0이다. 원시는 `extracted/cpp-chat-recipient-570-20261029/`다.

마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재도 같다. 디컴파일 산출물과 원본/CD/AGENTS.md 변경은 없다. 게임/GUI/전체C++/34경로/음향/Windows11 실기, 포획·희생·특정 공격·승리는 실행하지 않았다. 신규MP/통신 검사도 없다.

다음은 Console Draw/ChatWindow에 같은 현재 PlayerFinder를 연결하고 객체 메뉴의 별도 빈 문자열을 실제 편집 입력으로 교체하는 것이다. 40BYTE 토큰과 현재 PlayerFinder의40BYTE 문자열 물리 검사도 원문에 맞춰 검토해야 한다. 자동완성·제출·Chat manager 송수신, 저장상태 재접속/MP와 일반 UI/Edit·시나리오·수명·호환성은 잔여다. 전체 목표는 진행 중이다.
