# Checked의 실제 Client 전역과 설정·공급 수명 — 2026-10-25

단위413은 Checked 항목이 읽는 원본 `5cab30`/`55a480`을 실제 Client 설정과 Session의 공급 상태에 연결한다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 직접 실행·클론 GUI 실행은 없다. 전체 목표는 활성/미완료다.

## 원문과 변경

원본 `4353ea..43541a`는 `55a480`과 인접 `55a484`를0으로 쓴 뒤 현재 `5cab30` 주소로 `ReadInt("userCheat")`를 호출한다. 누락 값은 현재값을 보존하지만, 이후 비교에서 `0x845fed` 이외의 값은0으로 돌아간다. 이 구간의 EBX0은 원문 `43523b`의 XOR와 이후 선택 구간까지 EBX 대입이 없음을 확인했다.

[Client](../../cpppj/src/client/ClientMain.cpp)의 최초 설정 해석과 즉시 설정 변경 알림에 이 처리를 연결했다. [ClientGameplayGlobals](../../cpppj/src/client/ClientGameplayGlobals.h)가 공급 상태를 소유하고 [실제 Session](../../cpppj/src/client/RawCampaignSession.cpp)이 같은 주소를 참조한다. 따라서 설정·메뉴·공급 판정이 서로 다른 `userCheat` 사본을 읽지 않는다. Runtime의 생성·반납에서 이 값과 특수 모드를 유지한다. 나머지 공급 필드는 현재 Session 초기화 의미를 유지하며, 원본 NativeState 전체 초기화 순서 복원은 별도 미완료 범위다.

전체 HTML과 기존 Tell 표시 어댑터의 Checked가 현재 Client 전역을 읽는다. 명시 enabled가 참이면 전역을 읽지 않고, 거짓일 때 `5cab30`을 먼저 읽고 필요할 때만 `55a480`을 읽는다. 이 두 전역을 읽기 위해 가짜 월드를 만들거나 누락된 장치를 상수0으로 대체하지 않는다.

기존 `PlayerWorldState::special`의 표현은 bool이다. 이 단위는 현재0/비0 조건과 설정의0 쓰기를 연결하지만 임의 비영 DWORD의 전체 비트 표현까지 복원했다고 주장하지 않는다. 인접 `55a484`는 원문0 쓰기를 위한 실제 장기 DWORD로 보관하며 별도 기능 해석은 후속이다.

## 독립 관측과 검사

[관측 도구](../../tools/decomp_html_checked_options.py)는 보호 PE의 실제 선택 구간을 **144행·1410명령** 실행했다. 이전 값·누락/허용/인접 거부값·특수/인접 전역의 초기값을 조합하고, 현재 세 전역과 continuation/스택 이동56/ReadInt 횟수를 읽었다. 원문 ReadInt는 현재 DWORD 또는 누락을 공급하는 명시 장치다. `435220` 전체 설정 함수 실행이나 원본 ReadInt 본체 검증과 구별한다. 정수 경로로 두 x87 정밀도 비교도 별도로 수행하지 않았다. `--check`는 자료를 바꾸지 않는다.

[실제 설정 검사](../../cpppj/tests/ClientScriptMenuOptionsTests.cpp)는 같은 Client의 Config/변경 알림으로144행을 재생하고, 부호·앞 공백·decimal 접미사·32비트 감김·인접 거부값을 실제 ConfigParseLong으로 처리한다. [실제 메타 결합](../../cpppj/tests/HtmlWindowMetadataClientTests.cpp)은 같은 장기 전역에 따른 disabled 비트와 현재 읽기 순서를 확인한다. [두 실제 Session 결합](../../cpppj/tests/CampaignSessionTests.cpp)은 설정 전역의 생성 전 존재·공급 상태 주소 일치·다음 Runtime까지의 보존을 확인한다.

빌드1/2 종료0·관측 경고오류0, 최종 집중 **설정2+메타11+실제 Session2+기존 옵션5=20개/실패0**,전체 CTest **2790개/실패0·327.48초**다. 첫 신규 메타 검사의 비활성 기대 비트를 원문 `0x200`으로 바로잡았고 제품 메뉴 로직은 바꾸지 않았다. 이전 `0x4` 기대는 checked 비트와 혼동한 검사 오류이며 첫 실패 로그를 보존했다.

보호 **originals1394/CD425/AGENTS/정밀 소스=1821개**가 시작 시점과 같다. 제품 SHA256은 `7019c9b89666408fb9ec64d43e24837be37638d1704e98ea1f0957aaff2af55f`, 검사 SHA256은 `14fd53be428698677a18caecd8e192024808f7c5f6b740ee27140d97b5ca7e5c`다. [제품 보고서](../../cpppj/html-checked-client-runtime-20261025.json)에 소스/참조/원시 SHA와 범위를 기록했다. 원시는 `extracted/cpp-html-checked-413-*`다.

## 다음 범위

기존 Tell 전체 부모·구형 `%` 표시와 Guide의 실제 HTML 개수 통합·full Uber 배경 GIF 생성/교체가 남는다. Production/Minimap·State/Core/Pool/Win32와 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오·완전 동일성 및393 Wide SaveOptions도 계속 미완료다. 10.72의 별도 옵션/전역 비교는 후속이다.
