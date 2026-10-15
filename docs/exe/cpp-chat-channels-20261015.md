# 실제 Chat 채널 기록과 AI 동맹 표시

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 같다. AGENTS와 양 LEFT의 최신 절을 다시 읽고 같은 PC의 전체 정밀 함수표·보호 PATCH PE를 사용했다. 새 디컴파일·게임/GUI·OS 진입은 없으며 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

## 실제 몸체와 API

`client/RawChatChannels.{h,cpp}`는 `00444ee0` Add, `00445080` Clear 및 Add가 호출하는 이름 정제 `0042b0e0`와 빈 몸체 `004437f0`을 복원한다. 현재 클라이언트가 소유할 **100×0x30=4,800바이트** 저장소와 **현재 mySubscriber getter**를 빌린다. 생성자는 기록을 지우거나 subscriber를 고정하지 않는다. 원본 정적 기록 시작은 `00558ed0`이다.

| 기록 offset | 실제 폭/의미 |
| --- | --- |
| `00..27` | 이름 40바이트. Add는 strncpy40 뒤 마지막 `27`을 NUL로 쓴다. 최대 표시 이름은 39바이트다. |
| `28` | 채널/상태 BYTE. 기존 항목은 들어온 BYTE와 OR하고 신규는 들어온 BYTE로 바꾼다. |
| `29..2b` | 이 두 몸체가 건드리지 않는 stale 바이트다. |
| `2c` | subscriber DWORD. 0은 빈 기록이고 음수 비트도 보존한다. |

실제 Player `00490ea0`의 capstone 명령은 `Add(sub,name,0,0,-1,channel,-1,0)`의8개 DWORD를 push한다. Add 본체는 인자 **1·2·6만 읽는다**. 보존 레지스터4개 push 뒤 `[esp+14]`가 subscriber, `[esp+18]`가 이름, `[esp+28]`가 flags다. 나머지5인자를 미확정 기록 필드로 저장하지 않는다. `AddToChannel(sub,name,channel)`은 이 실사용 세 인자를 같은 `Add`로 전달한다. `Add`의 `name=nullopt`는 NULL, 빈 string_view는 비NULL 빈 문자열이다.

1. 현재 `00540c74`와 subscriber가 같으면 **이름 조회/표 검색/하위 호출 없이 -1**이다. getter는 매 Add에서 다시 읽는다.
2. 일반 subscriber는 기록 순서대로 첫 빈 칸을 보관하면서 기존 같은 식별자를 찾는다. 첫 기존 항목을 만나면 앞의 빈 칸보다 기존 항목을 우선한다. subscriber0도 같은 일반 검색이어서 첫0기록의 **기존 항목 분기**를 탄다.
3. subscriber `ffffffff`는 기존 `ffffffff`를 갱신하지 않는다. 첫0기록만 선택하고 입력 flags의 bit10을 제거한다. 빈 칸이 없으면 실패한다.
4. 신규는 식별자를 쓰고, NULL 이름은 `Unknown%08x`를 NUL까지 쓰며 나머지 이름 tail을 지우지 않는다. 비NULL 이름은 원본 strncpy40처럼 NUL 뒤를0으로 채운다.
5. 기존 항목은 이름 첫 바이트를0으로 먼저 쓴다. 비NULL이면 그 뒤 strncpy40, NULL이면 이0쓰기만 한다. 기존 flags와 들어온 flags의 하위 BYTE를 OR한다.
6. 이름 마지막 BYTE0 → flags BYTE → 실제 이름 정제 → `004437f0` → `004437f0`의 순서다. 정제 후 반환은 선택 기록의0기반 index다.

`Clear(mask)`는100기록의 현재 subscriber가 비제로일 때만 `flags &= ~BYTE(mask)`를 수행한다. 이름·식별자·주변 stale 바이트와 subscriber0 기록의 flags를 바꾸지 않으며 refresh를 호출하지 않는다.

## 실제 이름 정제와 호출 관찰

`0042b0e0`은 UI 통지가 아니라 작은 이름 정제 본체다. 실제 원문 표의 첫 두 바이트를 `a0`·`%`로 고친 뒤 해당 두 바이트, 마침표·물음표·등호·소괄호·대괄호·물결표·큰따옴표·백틱·세미콜론·콜론·공백을 삭제한다. 원본 memmove처럼 NUL까지 왼쪽으로 옮기고 NUL 뒤의 stale 바이트는 지우지 않는다. 결과가 비면 `Cheat%d`를 쓰며 서식 직전 명령 `and eax,ff`에 따라 subscriber의 **하위 unsigned BYTE**를 사용한다. 이 모듈은 UTF-8을 원본 바이트로 오인하지 않는다. 실제 서비스가 Player의 현재 원본 좁은 문자열을 넘겨야 한다.

`004437f0`은 실제 단일 `ret`이다. 기본 구현도 이 원문처럼 추가 UI 효과가 없으며, Add의 두 호출 위치를 그대로 유지한다. 별도 미복원 UI 경계를 성공하는 noop로 대체한 것이 아니다. 선택 `ChatChannelCall` observer는 실제 이름 정제 직전과 두 refresh 진입을 검사에서 관찰할 뿐 계산을 대체하지 않는다. Client 연결에 observer를 공급할 필요는 없다.

## 독립 native 관찰과 남은 검증

`tools/decomp_chat_channels.py`는 같은 PC의 전체 정밀 함수표에서 위4개 몸체의 범위만 허용한다. 실제 보호 PE의 Add/Clear/이름 정제/빈 refresh를 실행하고, 기록4,800바이트·정제 문자표 첫2바이트·함수 스택 밖의 실제 명령 쓰기와 미검토 명령 진입을 거부한다. 호출 규약의 보존 레지스터4개, 정상 반환 주소·cdecl 스택을 확인한다.

새 **2,448개 고유 입력 = Add2,400 + Clear48**을 보존했다. Add 중 실제 이름 정제1,360회·refresh2,720회다. 자기 반환·빈칸 없음은 해당 하위를 자연 생략한다. 입력은0/1/77/-1/높은 비트 식별자, 자기 subscriber1/77, 기존 항목/앞 빈칸/마지막 빈칸/중복/-1 기록, NULL/빈 이름/39·40·41바이트/금지문자, BYTE를 넘는 flags/mask를 교차한다.

CRT는 명시 외부 대체다. `strncpy40` 1,190회, `sprintf(Unknown%08x)` 115회, `strchr` 27,225회, `memmove` 4,590회, `004a89c0(Cheat%d)` 395회를 실제 인자로 대체했다. 이 표본을 원본 CRT 내부나 임의 서식/로케일/포인터 겹침/OOM까지 관찰한 것으로 확대하지 않는다. 문자열 입력 공급과 기록 초기화도 관찰 입력이지 원본 생성자 실행이 아니다.

fixture는 `cpppj/tests/fixtures/chat-channels-x86.tsv`다. 매 입력의 **전체4,800바이트를 읽은 뒤 변경 offset:byte만 손실 없이 직렬화**한다. 검사는 동일 초기 입력에 해당 독립 관찰을 적용해 전체 배열을 비교하므로 선택된 기록만 검사하거나 체크섬만 비교하지 않는다. 보고서는 `cpppj/chat-channels.json`이며 보호 PE·현재 정밀 범위·도구·제품·검사·fixture SHA를 보존한다. `python tools/decomp_chat_channels.py --verify`가 고유입력/보존 SHA 감사를 통과했다.

`ChatChannelsTests.cpp`는 native2,448개 재생, 현재 subscriber/실제 이름정제→두refresh 순서와 AI wrapper, 판본/저장소/getter 계약의 **3개 몸체 검사**를 제공한다. 후속 단위67에서 실제 Client 수명/현재 설정 검사2개를 더해 현재5개다. CMake 등록 뒤 Release 경고0·오류0, 이5개를 포함한 집중33개 실패0·전체 CTest1/1·내부1,264개 실패0·210.62초가 통과했다. 보고서의 현재 상태는 `release-focused-and-full-ctest-validated`이며 원본 관찰과 제품 검증을 구분한다.

## 실제 Client 연결

실제 클라이언트가100기록 저장소와 `RawChatChannels`를 장기 소유하고 getter를 현재 Session/로그인 subscriber에 연결해야 한다. 같은 Client가 여러 미션을 거치는 동안 기록을 미션별 사본으로 복제하지 않는다. `PlayerParticipationHooks.clearChat`은 `Clear(channel)`에, `addChat(sub,name,channel)`은 `AddToChannel(sub,name,channel)`에 연결한다. 자기 subscriber 등록은 이 실제 Add의 자연 반환을 사용한다. AI 동맹 갱신 전체를 생략하거나 성공하는 가짜 Chat callback을 설치하지 않는다.

최초 몸체 단위는 Client/Session/CampaignClientServices를 수정하지 않았다. 후속 [단위67](cpp-client-ai-chat-20261015.md)이 실제 Client pimpl의 단일4800바이트와 현재 Session subscriber getter, 실제 Chat/기본이름 서비스를 연결했다. 로그인 부모, Stage3/AI 전략/미션 완주는 여전히 후속이다. 이 기존 native 보고서의 현재 검사 SHA 갱신은 새로운 native 입력 관찰을 뜻하지 않는다.
