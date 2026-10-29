# 채팅 대상 검사와 실제 PlayerFinder의40BYTE 경계 — 단위571

단위570의 독립 검사기를 실제 `RawPlayerIdentity`와 합성하자40글자 토큰에서 `Player 식별자 NUL 없는 문자열` 예외가 발생했다. 원본은 같은 입력에서 정상 판정을 계속한다. **최초 실패를 보존하고 원본 Regexp·부모·실제 PlayerFinder를 함께 실행한48행을 근거로 수정했다. 최종 고유집중37개/실패0·등록3438, Release 검사/게임 빌드 경고·오류0**이다. [원문](../../cpppj/chat-recipient-identity-native-20261029.json)·[검증 집계](../../cpppj/chat-recipient-identity-runtime-20261029.json)를 따른다.

## 원인과 변경

원본 `00495a10`은 이름을40BYTE까지 복사하고 그 뒤NUL을 쓴다.60BYTE PlayerFinder의 이름은+8이므로40글자 뒤의NUL은+48 주소DWORD의 첫BYTE다. 기존 C++ `RawPlayerIdentity::Text(...,40)`은 이 종결자를 읽기 전에 예외를 냈다. 포인터/배열의 실제 할당 크기는 충분했고 데이터 손상 때문에 실패한 것이 아니었다.

[RawPlayerIdentity](../../cpppj/src/o/RawPlayerIdentity.cpp)는 조회 레코드의 이름을 비교할 때 인접한 한BYTE까지, 즉41BYTE 안에서NUL을 찾는다.100개 Chat행도40글자 뒤flags 첫BYTE가NUL인 원문 물리를 같은 방식으로 읽는다. `SetName`은 정확히40글자와 인접NUL을 쓰는 것을 허용하고 이름 getter도 같은 종결자를 읽는다. Player9/Chat100/레코드60BYTE의 할당 크기·순회 상한은 바꾸지 않았다. 더 긴 물리 계약 위반은 계속 명시 예외다.

조회 레코드와 Chat행의 이 한BYTE는 원문이 실제로 읽거나 쓰는 위치다. 임의 배열 확장이나 잘못된 후보 무조건 무시로 예외를 숨기지 않았다. owner/subscriber/주소·로컬/Player/Chat 검색 우선순위와 앞선 부분효과도 유지한다. 10.72의 기존 회귀를 함께 통과했지만 신규40BYTE 관측은10.78만 대상으로 하며 CD의 전체 동일성 확인으로 확대하지 않는다.

## 원문 접합 관측과 검사

[새 도구](../../tools/decomp_chat_recipient_identity.py)는 제품 수정 전에48행을 고정했다. `0042dc70/0042d910`, 실제 Regexp 컴파일/검색/캡처, `00484190/004841b0`, 실제 `004841c0`을 모두 실행한다. 명시 외부 장치는 malloc/CRT 바이트 문자열 함수와 영어 번역뿐이다. 로컬·일반 전투·편집 모드에 같은 Player9/Chat100 입력을 주며 메타맵·통신은 실행하지 않는다.

빈 이름·로컬 이름·Player Alice·Chat Bob·채널,39/40/41글자 토큰·40글자 Chat 이름·복수 토큰·CR/LF/VT·중간NUL을 포함한다. 전체60BYTE와 Identity 진입 이름/번역 키 순서도 고정했다. 같은 원문 재관측 차이는0이다. 이48행과570의350행은 서로 다른 하위 장치를 사용하는 별도 근거이며570의fixture를 덮어쓰지 않았다.

수정 전 새 접합 검사는1사례/1실패·종료1이고 `tests-baseline.log`에 보존했다. 이후 같은48행과 다음 집중 묶음을 통과했다.

| 필터 | 실행 사례 / 실패 |
| --- | --- |
| ChatRecipients | 3 / 0 |
| ChatWindow | 9 / 0 |
| PlayerIdentity | 4 / 0 |
| PlayerMenu | 3 / 0 |
| ObjectMenu | 20 / 0 |

중복2개를 제외한 고유37개, 총39실행이다. 등록3,438개이며 전체 C++를 실행한 결과는 아니다. 검사 target의 수정 전/후 두 빌드와 최종 게임 target 빌드는 모두 경고·오류0이다. 새 게임 SHA는 `e1997146d2e1daf6cee326c6cb08a1ee8614a7659aa319ddfcc952b29e5cba02`다. **이 제품을 실제 게임으로 실행하지 않았다.**

보호2,584파일(원본/CD·AGENTS.md·정밀 산출물·기존fixture724) 해시 차이0이며 현재fixture725개다. 최초 실패와 수정 전 소스는 `extracted/cpp-chat-recipient-571-20261029/`에 보존했다. 마지막 전체 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재도 같고 정밀 산출물 변경은 없다.

## 다음 작업

실제 Client/Console/ChatWindow의 recipient/resolve callback 연결, 현재 PlayerFinder의 장기 이름/주소와 객체 메뉴의 실제 편집 버퍼 연결은 여전히 남는다.570 당시의40BYTE 충돌은 이번 단위에서 해결됐으므로 재개 시 다시 미해결로 취급하지 않는다. 자동완성·제출·송수신/재접속과 MP는 별도 범위다.

게임/GUI·34경로·자연 완주·포획/희생·특정 공격·승리·신규MP/통신·Windows11 실기·원본 픽셀/음향은 실행하지 않았다. 마지막34경로는560제품/561, 실제 데모3개의 첫360초는565/568제품의567/569 기록을 유지한다. 일반 UI/Edit·시나리오·수명·호환성과 전체 목표는 계속 진행 중이다.
