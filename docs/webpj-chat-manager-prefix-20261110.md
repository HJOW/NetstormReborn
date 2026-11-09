# webpj Chat manager 랭크 prefix(RawChatManagerPrefix) 이식 — WB04-E229

2026-11-10, HJOW-Athlon. 채팅 시스템의 manager 생성/초기화(Chat.cpp 0042e000·0042e050)와 채팅창 랭크 아이콘 prefix(Chatgump.cpp 0042d620 setter·0042f480 formatter·0042f540 wrapper)를 같은 현재 전역(현재 manager 슬롯·subscriber·CoreData 활성)에 적용하는 부분을 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E228 State 세계 정리에 이어 옮겼다. 전체 Chat 전송/창 생성과는 구별한다.

- cpppj 기준선 `1f00a25a`. 소스(`RawChatManagerPrefix.cpp/.h`)·시험(`ChatManagerPrefixTests`)·fixture(`chat-manager-prefix-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `2ede4027`(2026-10-24).
- 원본 fixture: `chat-manager-prefix-x86.tsv` 644행(두 x87 관측, 10.78 전용; 정상 524·CPU fault 94·명시 중단 26). 입력 13칸(진입점 0 생성·1 초기화·2 setter·3 formatter·4 wrapper, seed, 현재 manager 0/1/2, 활성, 랭크, subscriber, 할당 실패 프로필, 아이콘 개수, 최대 랭크, 반복, Config 길이 둘, 변이)과 출력(상태, 외부 출력 256 BYTE, 두 manager 0xa98 BYTE 전체, 두 내부 0x80 BYTE 전체, 하위 호출 trace)이다.
- 공유 장치: 없음(순수 바이트·순서 규칙).

## 1. 옮긴 것

`www/core/raw-chat-manager-prefix.js`(DOM 없음, 10.78 전용)

- 상수 `CHAT_MANAGER_BYTES`(0xa98)·`CHAT_MANAGER_BUFFER_BYTES`(0x80, 별도 내부 할당이며 초기화하지 않음)·`CHAT_MANAGER_RANK_PREFIX_OFFSET`(0x244)·`CHAT_RANK_OUTPUT_BYTES`(256).
- `constructChatManager(object, hooks)`(0042e000): 내부 0x80 할당 → object+8 에 토큰 쓰기 → 할당이 0 이면 진단 220 → 지정한 필드(+0·+0xc·+0x14·+0x50·+0x244·+4)만 쓰고 같은 토큰 반환.
- `initializeChatManager(globals, hooks)`(0042e050): 새 0xa98 할당 → 현재 manager 에 대입(생성자 호출) → 명령 3개 등록(`format`·`back`·`front`) → 현재 subscriber(signed)로 만든 설정 키 `<n>chatFormat`·`<n>chatFormat2` 읽기 → manager 가 없으면 진단 898 → **현재 manager 를 매번 다시 읽어** strncpy(+0x14, 60 / +0x50, 500)와 NUL 쓰기(+0x4f / +0x243).
- `setChatManagerRankPrefix(manager, source)`(0042d620): 같은 source 에서 NUL 포함 순방향 복사(+0x244). 겹치는 별칭 입력도 매 BYTE 현재 값을 읽는다. 의미상 256 절단이나 입력창 접두 변경은 없다.
- `chatPrefixIcons(output, temporary, rank, hooks)`(0042f480): temporary[0] = 0 → 랭크 ≥ 0 이면 최대값으로 clamp(비교마다 getter 를 다시 읽음) → 아이콘 개수 getter 의 결과가 0 이하면 진단 2474 뒤 계속하되 0 이거나 INT_MIN/−1 이면 CPU 나눗셈 fault(`ChatManagerDivideError`) → 몫 > 0 이면 `C`(몫−1)·항상 `B`(나머지) 아이콘 서식을 덧붙임 → temporary 256 BYTE 를 DWORD 단위로 output 에 복사. 나눗셈 인자는 진단이 getter 를 바꿔도 다시 읽지 않는다.
- `refreshChatManagerRankPrefix(globals, hooks, output, temporary)`(0042f540): 현재 manager 와 CoreData 활성이 모두 비영일 때만 현재 Core+4 BYTE(unsigned)로 formatter 를 돌리고, 그 사이 바뀔 수 있는 현재 manager 를 다시 읽어 setter 를 적용한다.
- `copyChatManagerFormat`(strncpy 의 NUL padding·지역 C문자열의 첫 NUL 절단·대상 초과 거부)·`appendChatRankIcon`(`~[Iicon.<kind><value>]`을 NUL 뒤 끝에 덧붙임, NUL 없음/물리 부족 거부)·`bindChatManagerMemoryCopy`(누락된 CRT 경계만 같은 memory 에 연결하고 사용자 callback 은 보존).
- `globals = { current: { value }, subscriber: () => n, coreDataActive: () => n }`, `hooks = { allocate, memory, assertion, registerCommand, readConfig, copyFormat, maximumRank, minorCount, appendIcon, core }`. 도달한 장치 누락은 TypeError, 물리 범위 밖은 RangeError(C++ `out_of_range`)다.

## 2. 검증

`tests/chat-manager-prefix.test.mjs`(5건)

- 원본 관측 재생: 644행 전부. 행마다 새 장치(두 manager·두 내부 버퍼 전 BYTE 에 독립 패턴, 출력/임시/wrapper 출력, Core 10 BYTE)를 만들어 반복 실행하고 상태 문자열·외부 출력·두 manager 전체·두 내부 버퍼 전체·trace 를 대조한다. 정상/fault/중단 수(524/94/26)도 센다.
- cpppj 계약 포팅 2건: 현재 manager 나 활성이 0 인 자연 관문은 Core·formatter·버퍼 장치가 없어도 복귀, 실사용 CRT 어댑터는 같은 실제 바이트에 쓰고 NUL padding 밖은 보존하며 사용자 callback 은 덮어쓰지 않고 잘못된 토큰은 도달 위치에서 거부.
- 웹 계약 2건(원본 fixture 에 없는 경계): formatter(정상 C/B 서식, 마지막 DWORD 복사, 음수 랭크, 아이콘 개수 0 과 INT_MIN/−1 의 fault 후 아이콘 없음, 중간에 음수가 된 랭크의 `B-1`, 짧은 temporary/output 거부), setter·append·strncpy·wrapper 의 물리 범위(NUL 없는 source 의 앞선 쓰기 보존, NUL 없는/꽉 찬 append 대상, 첫 NUL 절단, 4 BYTE 짜리 Core 거부).

최초 실패와 교정: 모듈은 처음부터 644행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다. 처음 변형 57건 중 14건이 살아남았고 그중 10건은 위 웹 계약 2건을 더해 잡았으며(typed array 는 범위 밖 쓰기를 조용히 무시하므로 fixture 가 범위 거부를 확인하지 못했다), append 의 NUL 없음 검사는 대상 크기를 32 BYTE 로 키워 구별했다.

- 실패한 변형: 크기/위치 상수, 범위 검사 제거(store·strncpy·append·setter·formatter·wrapper), signed 키 변환, NUL padding/절단/NUL 쓰기 제거, 서식 문자열, 내부 할당 크기·진단 줄·각 sparse 쓰기 제거, 생성자 호출 조건, 명령 등록 순서·인자, 설정 키, manager 재조회(첫/둘째), strncpy 길이, 랭크 관문 `≥ 0`, getter 재읽기 제거, 진단 조건, 0/INT_MIN fault 제거, 몫 조건·`C` 인자·`B` 인자, 복사 범위(마지막 DWORD), wrapper 관문·Core 인덱스·Core 길이·캡처 divisor 재조회(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): ① 설정값 존재 판정을 `!= null` 대신 truthy 로 — 빈 문자열일 때도 결과가 같은 빈 복사다. ② wrapper 의 setter 인자 `memory(current ? current : 0)` — 위 관문에서 `current` 가 비영임이 보장돼 같다. ③ 나눗셈 몫에 `Math.trunc` 대신 `Math.floor` — 몫은 `> 0` 일 때만 쓰이고 양수 몫은 같다(`Math.floor` 가 달라지는 음수는 `C` 를 붙이지 않는다).

## 3. 경계와 남은 것

- 실제 채팅창/전송/Config 에 연결하지 않았다. 호출 지점(Chat 시스템 초기화·Subscriber 변경)과 실제 할당·명령 등록·Config 읽기·아이콘 개수 getter·CoreData 조회(E3 `readPlayerCoreData`)의 하위 연결은 순서4·5 몫이다(PORT_MAP session 0).
- 같은 소스를 쓰는 cpppj `ChatFormatTests`·`SubscriberGlobalsTests`·`ClientSubscriberGlobalsTests`는 별도 단위(`RawChatFormat`·Subscriber 전역)에서 다룬다.

## 4. 다음 시작점

WB04-E230 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`SquidSpatial`·`RawTerrainGrowth`·`RawTotalMade`·`RawObjectTraceFields`·`RawMissionFrameGate`)를 권한다. `RawManaBoltRegular` 는 시험이 카메라 투영(`client/RawCursorPosition`·`Sound` 의 SoundToScreen)을 쓰므로 그 두 도구를 옮기거나 정할 때 함께 판단한다.
