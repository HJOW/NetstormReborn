# Tell % HTML 목록의 정상 경로·두 번 치환 — 2026-10-25

단위410은10.78 원본 `4cdd40`과 공유 꼬리 `4ce14d`의 **정상 경로**를 복원한다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 게임 직접 실행·이번 단위의 실제 클론 GUI 실행은 없다. 전체 `/goal`은 활성/미완료다.

## 본체와 연결

[TellFileMenus](../../cpppj/src/client/TellFileMenus.cpp)의 `Inline`은 @/#와 같은 실제 Config 등록/재사용·첫 경로/index·고유 prefix·원문 비교교환을 사용한다. 공통 `Collect`를 분리했지만 @/#의40개 배열 제한은 유지하며 %는 원문의80개 배열을 사용한다. 기존 @/# 독립 관측384행도 변경하지 않고 재생한다.

각 행에서 실제 지역 `cur` Config를 등록하고 현재 제목을 치환한다. 빈 제목은 파일명으로 바꾸고 `--` 제목은 링크/인자 조회 없이 건너뛴다. 일반 행의 인자는 **두 번 치환**하며 첫 반환값만 버린다. `{@미션.키}`처럼 파일을 읽는 치환은 두 조회의 외부 효과를 모두 수행하고 두 번째 반환값을 사용한다. Config·지역 cur는 이 두 조회와 현재 링크 작성이 끝날 때까지 같은 수명이다. % 자체는 ScriptStringRing 항목을 만들지 않는다.

출력은 항목마다 LF로 시작하는 원문 `\n<a herf="cmd:명령,인자">제목</a>`다. `herf` 철자, 명령 대소문자·인용·LF 위치를 그대로 유지하며 별도 HTML escape나 마지막 LF를 추가하지 않는다. String 반환 후 기존 메타가 수행하는 **replacement 길이만 원래 버퍼 앞에 복사하는 동작**도 유지한다. 이를 정상적인 전체 문자열 삽입으로 바꾸지 않았다.

[RawMenuHtml](../../cpppj/src/client/RawMenuHtml.inc)의 실제 % 메타가 같은 파일 목록 소유자의 `Inline`을 호출한다. 명시 하위 `inlineList` 콜백은 기존대로 우선하고, 둘 다 없으면 선택된 분기에서 실패한다. 기존 Tell 부모의 구형 % 표시 어댑터는 이번 연결과 별개이며 전체 부모 이행 때 교체해야 한다.

## 독립 원문과 결합 검사

[관측 도구](../../tools/decomp_tell_inline_list.py)는 실제 `4cdd40..4ce1af` 정상 본체와 공유 꼬리를 **144입력×두 정밀도=288행**으로 실행했다. 입력에는 파일 순열/중복·기존 Config·빈 제목/구분선·일반/숫자 인자와 매번 다른 값을 반환하는 동적 조회를 포함한다. 파일 적재/두 번 조회 순서와 최종 HTML 전체 바이트가 C++와 같다. 두 정밀도 차이0이고 `--check`도 자료 변경0이다. FileSpec·Config·할당·CRT 비교·String 생성/추가/복사/소멸은 명시 장치다.

[목록 검사](../../cpppj/tests/TellFileMenusTests.cpp)는 실제 ConfigInterface의 `{@probe.value}`→현재 missionSpec→실제 Reader를 연결하여 첫 값1/두 번째 값2를 구별한다. Native288행의 Q 효과와 반환 HTML을 이 실제 Config로 대조한다.80개 수집/공유 Config 재사용과81개 범위 거부도 검사한다. 기존 @/#384행과 등록·고리·반납 검사는 그대로 통과해야 한다.

[실제 메타 결합 검사](../../cpppj/tests/HtmlWindowMetadataClientTests.cpp)는 실제 전체 부모/controlled 메뉴/고리와 실제 % 본체를 연결한다. 긴 prefix 뒤 % 지시문에서 두 VFS 조회를 관측하고, 원문 부분 복사 때문에 현재 본문이 `prefix+TAIL`로 남는 물리 결과와 NUL 뒤 경계·현재 cur 반납·실제 미부착 메뉴 정리를 확인한다. HTML 링크가 화면 전체에 정상 삽입됐다고 바꾸어 판정하지 않는다.

최종 Release 빌드 종료0, 집중 **Tell20+실제 메타9=29개/실패0**, 전체 CTest **2781개/실패0·303.03초**다. 초기 빌드는 신규 검사의 `OriginalText.h` 포함 누락을 보완했고 최종 재빌드가 성공했다. 보호 감사에서 **originals1394/CD425/AGENTS/정밀 소스=1821개**가 시작 시점과 같다. 제품 SHA256은 `481bb6150467c39fdfa6d8f9a874b85c864e48d27e606e572b3dfd046f31dee4`, 검사 SHA256은 `25c7c8991098f8a01c1d04b9711826c0073844d60a107e586e1ce6e08dacb46f`이며 집중/전체에서 같다. [제품 보고서](../../cpppj/tell-inline-list-client-runtime-20261025.json)에 소스/참조·원시 SHA와 경계를 기록했다. 원시는 `extracted/cpp-tell-inline-410-*`다.

## 미완료 범위

원문의 행별 C++ 예외와 Catch funclet의 `ERROR` 링크 복구는 이번 정상 경로 관측에 포함하지 않았고, 선택 장치 예외는 현재 C++ 호출자에 전달된다. 이 부분을 원본과 같은 예외 동작으로 완료 처리하지 않는다. 기존 Tell 부모의 전체 HTML/구형 % 표시·기술/Input/전송·배경/Checked 전역·Guide/실제 창 개수·full Uber가 남는다. Production/Minimap·State/Core/Pool/Win32와 전체 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오·완전 동일성 및393 Wide SaveOptions도 미완료다. 10.72는 별도 비교 대상이다.
