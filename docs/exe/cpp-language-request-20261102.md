# SetLanguage 요청과 기존 Chat 갱신 알림 — 단위712

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 언어 번호/용어표710과 최초 다중 Xlat711에 이어, 원문의 언어 변경 이름·일반 입력·설정·예약 알림을 같은 Client에 연결했다.

## 원문과 구현

보호 PE의`005430b0`은`SetLanguage`→`00542988`(020000a5),`005430b8`은`LanguageChanged`→`0054298c`(020000a6)다. 공통 GameMenu 이름표/번호 조회와 HTML·시나리오 입력 큐에서 같은 이름을 사용한다. 원문 일반 입력`004d7e37..004d7e48`은 SetLanguage 선택 시 두 번째 사건 DWORD를 넘긴다. 앞의 일반 release 거부 뒤에 실행하며, Control/Shift 표시를 별도 실행 거부로 넣지 않는다.

`004dea70`은 요청 번호가 현재`005435b4`와 같으면 아무 효과 없이 반환한다. 다르면 다음 순서다.

1. 원문의 일곱 이름표에서 문자열을 골라`currentLanguage`를 실제 설정에 쓴다.
2. 동기 설정 callback이 끝난 뒤 현재`0054298c`를 읽고`00452fe0(command,0,0)`에 예약한다.
3. 예약이 반환한 뒤 현재`005c85d8`를1 증가시킨다. DWORD 순환과 callback이 바꾼 현재 값을 보존한다.

`LanguageRequest`와`Client::SetLanguage`는 같은 실제 Config/언어 전역·InputQueue·ClientLocalStateGlobals의 공유 카운터를 사용한다. 요청 함수가 현재 번호를 임의로 직접 바꾸지 않으며, 설정 해석은710의 같은 동기 callback이 수행한다. 언어 변경 때 Xlat 파일을 다시 읽지 않는다. 원문 영어0/1 선택 규칙도 유지하므로 요청 번호 자체가 그대로 최종 언어 번호가 된다고 가정하지 않는다.

`LanguageChanged`는 기존 공통 Gump 경로에 전달한다. 앞서 같은 사건을 전달한 경우 다시 전달하지 않는다. 기존 Chat 부모`0042fab0`의 갱신 분기는 Push 뒤 현재 x/y를 다시 읽고 Attach(parent0,mode0)를 수행한 다음 기본 Help 입력으로 간다. 이 알림에는 별도 release 거부를 추가하지 않았다. 전체 화면이나 미션을 새로 만드는 명령으로 해석하지 않는다. 메타맵 갱신 카운터의 MP 소비자는 기존 후순위 경계에 남긴다.

## 독립 원문과 검사

[격리 도구](../../tools/decomp_language_request.py)는 원문`004dea70`과 **실제40칸 큐 writer**`00452fe0`/`00452f90`을 실행한다. `004414d0`의 동기 Config callback만 명시 장치로 둔다. 큐 진입 시점의 카운터 변경도 독립 입력이며, 큐 본체의 명령 비트 정리/12BYTE 쓰기는 실제 CPU가 수행한다.

[원문 보고서](../../cpppj/language-request-native-20261102.json)는337개 입력이다. 초기 번호·요청0..6·카운터0/ffffffff·설정/큐 callback 효과와, 같은 미정 번호ffffffff의 자연 반환을 포함한다. fixture SHA-256은`fe51de8de8718752ac24d391cf9c700739a71d2e77da9f593413822124dc0c41`이다. 최초 관측을 덮어쓰지 않았다.

새 집중4개는337행 대조, 실패 전 선행 효과/같은 번호 자연 반환, 실제 이름→Script 큐→입력→Client/Config/번역/공유 카운터, 중복 없는 알림→기존 Chat 부모 재부착이다. Chat 검사는 실제 C++ 부모와 명시 Push/Attach 장치를 실행하며 전체 Gump 필터·픽셀·물리 입력을 검증했다는 뜻이 아니다. 기존 언어3개와 Xlat4개도 함께 선택해 **총11개 실패0**이다.

Release build-v1은 경고/오류0이다. 새 Native/집중 검사는 최초 실행에서 통과했다. 원시는`extracted/cpp-language-request-712/`이며 제품·보호 해시는 [최종 보고서](../../cpppj/language-request-20261102.json)를 따른다.

## 남은 범위

새 게임/GUI/통신/MP 검사는0이며 포획/희생/특정 적 공격/승리 재현·공중3종 전용 검사도 하지 않았다. 전체34/전체 C++/CTest/ZIP을 반복하지 않았다. 실제 GUI704와 전체34/ZIP645의 제품 구분을 유지한다. 전체 번역 UI/폰트·6차 한국어, 원문 범위 밖 언어 번호의 물리 접근/전체 예외 ABI, 나머지 Edit/시나리오 writer·파일/미션 전환 수명은 남는다. 전체 목표는 진행 중이다.
