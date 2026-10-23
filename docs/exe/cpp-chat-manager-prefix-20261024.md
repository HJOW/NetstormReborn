# Chatmanager 생성과 랭크 prefix 독립 복원

2026-10-24 후속 **단위385 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 신규 모듈·검사만 작성했으며 기존 공유 소스/CMake/LEFT/git·공용 라이브러리/빌드·게임/GUI에는 변경이나 실행이 없다. 실제 Client 연결은 아래 잔여로 구별한다.

## 최초 원문 관측과 검증 결과

제품 파일이 없는 상태를 도구에서 확인한 뒤 보호10.78 PE의 다음5몸체를 Unicorn으로 실행했다. 하위 장치는 명시 응답하며 게임/OS로 진입하지 않는다.

| 주소 | 실제 역할 | 원문 폭 |
|---|---|---:|
|0042e050|manager new0xa98·현재 슬롯 대입·명령등록·설정 포맷|393BYTE|
|0042e000|내부 new0x80과 sparse 생성자 쓰기|78BYTE|
|0042d620|manager+244의 NUL 포함 순방향 setter|29BYTE|
|0042f480|signed rank clamp·그림 서식·256BYTE 복사|179BYTE|
|0042f540|현재 Chat/Core 관문·현재 랭크 prefix 갱신|66BYTE|

최초 자료는 [관측 도구](../../tools/decomp_chat_manager_prefix.py), [고정 TSV](../../cpppj/tests/fixtures/chat-manager-prefix-x86.tsv), [Native 보고서](../../extracted/cpp-chat-manager-prefix-20261024-native.json)다. **322고유입력×x87 두 모드=644관측**, 정상524·fault94·하위 명시 중단26이며 **220442명령**이다. 이 다섯 정수/문자열 몸체의 양 모드 결과는 동일하다. 각 manager **2712BYTE 전체**, 별도 내부 할당 각128BYTE 전체, 직접 formatter 출력256BYTE 전체, 현재 manager/Subscriber/할당 횟수·외부 호출 순서를 원시로 보존했다. 비0 패턴은 미작성 메모리를 구별하는 명시 입력이며 실제 미초기화 메모리의 값을 단정하는 자료가 아니다.

제품 `RawChatManagerPrefix.h/.cpp`와 `ChatManagerPrefixTests.cpp` 작성 뒤 공용 lib 없이 세 파일만 별도 MSVC 경로에서 컴파일했다. build3에서3검사/실패0을 확인하고 설명 주석을 보강한 같은 최종 소스를 build4로 다시 컴파일했다. 최종 **build4 종료0·컴파일 경고/오류0**, **test3의3검사/실패0**이다. 신규 검사1개가 원문644행 전체를 대조하고 나머지2개는 미선택 장치가 없는 자연 관문과 실제 BYTE뷰를 빌리는 순수 CRT 어댑터를 확인한다. 전체 CTest/공용 등록·제품 실행은 여기서 수행하지 않았다.

과정의 실패를 숨기지 않았다. 최초 원문 준비 두 번은 CPU 나눗셈 예외 뒤 같은 Unicorn 엔진을 재사용해 다음 입력에 예외 상태가 남는 문제였으며 제품 작성·fixture 생성 전에 중단했다. 두 실패 로그를 별도 보존하고 최종 도구는 fault/명시 중단 뒤 새 엔진을 생성한다. 고정 후 원문 fixture/report는 다시 쓰지 않았다.

첫 C++ 대조는 **22check 실패**였다. wrapper는 자기 stack에 formatter 출력을 받는데 검사 장치가 직접 formatter 호출의 외부 OUTPUT과 같은 주소로 대응했다. 검사 장치의 `wrapperOutput`만 분리한 뒤644행/모든 칸이 통과했다. 제품 몸체/원문 기대는 교정하지 않았다. 첫 두 build 스크립트는 LF UTF8의 한국어 `rem`을 cmd가 잘못 분리한 메시지 뒤 실제 cl은 성공했다. build3 스크립트는 CRLF로 작성해 이 메시지도 없앴다. 원시 `extracted/cpp-chat-manager-prefix-20261024-isolated/build1.log`~`build4.log`·`test1.log`~`test3.log`와 최초 Native 준비 실패 로그 두 개를 보존한다. [최종 검증 manifest](../../extracted/cpp-chat-manager-prefix-20261024-validation.json)에 소스/fixture/로그/격리 실행파일SHA를 고정했다. 다른 작업자의 읽기 검토에서도 명백한 순서/수명 오류를 찾지 못했으며 추가 실행이나 파일 변경은 없었다.

## 실제 API와 원문 효과

[신규 공개 API](../../cpppj/src/o/RawChatManagerPrefix.h)는 실제 current00548714 참조, 현재 Subscriber00540c74 읽기 참조, CoreData활성00595348 읽기 참조를 `ChatManagerPrefixGlobals`로 빌린다. 실제 토큰의 같은 할당을 반환하는 `memory(token)`와 할당 장치는 외부 소유자다. host 포인터를32비트로 잘라 토큰으로 쓰지 않는다.0/해제 토큰은 실제 메모리 장치가 거부한다.

`ConstructChatManager`는 new128→this+8→실패 진단220→DWORD+0/+c=0·BYTE+14/+50/+244=0·DWORD+4=32만 쓴다. 남은 manager2712BYTE와 내부128BYTE는 보존한다. `InitializeChatManager`는 new2712/생성자 뒤 현재 슬롯을 대입하고 format/back/front 명령을 flag1로 등록한다. 각 signed Subscriber 서식 시점의 현재값으로 chatFormat/chatFormat2를 읽는다. 진단898 뒤 **현재 manager**의+14에strncpy60/같은 첫 manager+4f=0, 다시 현재 manager의+50에strncpy500/같은 둘째 manager+243=0 순서다. copy callback이 current를 바꾸더라도 강제 NUL은 캡처한 주소에 쓴다. 재초기화에서 이전 manager를 자동 해제하지 않는다.

`copyFormat(token,offset,text,count)`는 토큰 인자를 받는다. 진단 복귀 뒤 current0이면 **CRT 진입을 먼저 하고 그 뒤 실제 NULL 대상 접근이 실패**하는 순서이므로 span 선조회로 경계 자체를 생략하지 않는다. `BindChatManagerMemoryCopy`는 이 callback이 없을 때만 같은 실제 `memory`와 실제 작은 `CopyChatManagerFormat`을 연결하며 기존 callback은 보존한다.

`ChatPrefixIcons`의 최대/마이너 getter는 매 원문 호출 위치에서 읽고, minor는 진단 전에 한 번 캡처한다. 진단 callback이 현재 minor를 바꿔도 캡처 divisor를 사용한다. rank<0은 그림 추가를 생략하고, 그 외에는 원문의 반복 maximum 조회·signed clamp·idiv·C/B append 순서를 따른다. idiv0/overflow는 안전한 C++ 예외로 표면화하지만 앞선 진단/부분 효과는 보존한다. formatter의256BYTE 지역 입력은 첫BYTE만0으로 바꾸며 나머지는 caller가 제공한 실제 초기 물리다. NUL 뒤도256BYTE 전체를 순방향 DWORD 단위로 복사한다.

`SetChatManagerRankPrefix`는 NUL까지 한 BYTE씩 순방향 복사한다.256자 의미상 절단을 추가하지 않으며 같은 manager 안의 앞/뒤 겹침과 물리 끝의 부분 쓰기 실패를 대조했다. `RefreshChatManagerRankPrefix`는 실제 current0/active0이면 Core/formatter/버퍼 없이 자연 반환한다. 활성에서는 현재 CoreData+4의 unsigned BYTE를 읽고 formatter 뒤 **다시 현재 manager**에 쓴다. 이 모듈은255/256BYTE 임의 prefix 사본을 별도 현재 전역이라고 부르지 않는다.

## 실제 Client 접합과 남은 범위

0042f540은 선택정보Gump/생산 패널 갱신이 아니라 **Chatmanager의 랭크 그림 prefix**다. ChatWindow00548740의HudChatNode+b8 입력 접두128BYTE, 기존4800BYTE 채널표와 구별한다. [최초 조사](cpp-editor-startup-chat-boundaries-20261024.md)에 실제 공통 시작00439840와 다른 초기화 호출 지점을 기록했다. 정상 Client.Run의42e050을 생략하거나 current를 상수0으로 두어 전체 Editor 성공을 만들면 안 된다.

본 모듈은 **할당을 직접 소유하지 않는다**. 후속 Client 한 개가 current 슬롯과 실제2712/128BYTE 할당을 소유하고, 공통 시작 시점에 실제 명령등록·Config 읽기·초기화를 공급해야 한다. 현재 ChatWindow/Chat 채널 owner를 다른 manager 포인터로 가장하지 않는다. 아직 이 모듈을 실제 Client.Run/Session의fortTechnologyUi에 연결하지 않았다.

`core()`의 실제0048f0d0 진단/현재 포인터, `maximumRank/minorCount`의004c6c20/004c6c00 현재 icon.C/B 타입 조회, 실제 Config/명령등록·할당기는 이번 Native 관측에서도 **명시 하위 경계**다. 기존 `RawGameMenus::rankPicture` hook은 이 formatter를 재사용할 수 있으나 실제 바인딩은 별도다. Client 공유 CoreData의 현재 포인터·활성 DWORD·동일 캐시/Player+6c 수명이 필요하며 세션별 임의10BYTE 복사본을 실제 현재 포인터로 가장하지 않는다. 전체 Chat 전송/로그/창/패킷, 실제 Editor 마지막 loaded/UI, startup reader는 이5몸체 통과에 포함하지 않는다.

공용 등록은 제품 소스1개 `src/o/RawChatManagerPrefix.cpp`, 검사1개 `tests/ChatManagerPrefixTests.cpp`, 검사 compile definition `NETSTORM_CHAT_MANAGER_PREFIX_FIXTURE`에 위 고정TSV의 절대 경로를 공급하면 된다. 추가 라이브러리 의존은 없으며 이 문서의 격리 검증에서는 공용 netstorm.lib를 읽거나 링크하지 않았다.
