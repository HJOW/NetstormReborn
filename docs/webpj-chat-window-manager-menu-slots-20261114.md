# webpj ChatWindow·Chat manager 전역·객체 메뉴 슬롯 표 ChatWindow/ClientChatManagerGlobals/ObjectMenuSlots/RawObjectMenuSlots (WB04-E447, 2026-11-14)

RawMenuControls 선행 **ChatWindow 272줄·ClientChatManagerGlobals 123줄·ObjectMenuSlots.inc 88줄·RawObjectMenuSlots 26줄(기준선 `1f00a25a`의 .cpp/.h/.inc 줄 수, 합계 509줄, todo→port 4단위)**을 새 모듈 [`www/ui/chat-window.js`](../webpj/www/ui/chat-window.js)·[`www/core/client-chat-manager-globals.js`](../webpj/www/core/client-chat-manager-globals.js)·[`www/render/object-menu-slots.js`](../webpj/www/render/object-menu-slots.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약·손 계산 값·실제 기존 처리기(TextEntryGump/Buffer/Storage·GumpCreate·RawPlayerIdentity·ObjectHitVtables)로 한 것이며 원본 직접 관측 수치가 아니다. 이 작업은 직전 세션 중단 뒤 이어서 마감했다(코드·시험·대응표는 중단 전에 끝났고 문서/LEFT_JOBS/전체 검사/커밋을 이번에 마쳤다).

## 기준선 뒤 변경 (3.6절)

네 단위 모두 기준선 뒤 변경이 없다. 같은 묶음 후보였던 `ClientChatWindowGlobals`(기준선 뒤 2커밋: 서버별 뉴스 키 버퍼·동맹 요청 제한 상태, 온라인 몫)·`MessageGump`(6커밋)·`ClientChatRuntime`(4커밋)·`WarningMessages`(4커밋)·`RawChatRelay`(2커밋)는 판정 대기라 이번 묶음에서 제외했다.

## 구현

- **`ChatWindow`(0x13c 바이트 root)**: `construct`는 root 기본 생성(float 폭/높이)·가상 표·종류(signed BYTE 부호 확장) → helper(0x170, `>` 라벨, 갈색)·크기 맞춤·제목/도움말(`chathelper`) 설치 → root 크기(배율 곱은 배정도 뒤 float32)·helperVisible 1 → 글자 `M` 측정 → 배경(0xb8, 폭-20, 높이-측정 높이, flags|0x04000000) → Console(0x128, 초기 없음, 글꼴 5, 변경 통지 1, 최대 길이 80, 단일 줄, 표시 비트 끔) → 로그(0xd8) → 공유 버퍼 교체·콜백 1 → 네 자식 부착(helper/Console 은 y=높이-측정 높이) → Slider/스크롤 → root 플래그 0xe00·접두 비움·본문 비움·접두 정규화·자동완성 0x42e540 → **최초 한 번만** 완성 목록(색 `~[C.n]` 256·배경 `~[B.n]` 256·프레임 높이 8..16 인 `~[I<type>.<frame>]`)을 만든다. `create`는 clear/quit 명령을 처음 한 번만 등록하고 200×100 root 를 만들어 현재로 저장한 뒤 로그 버퍼에 append 콜백을 설치한다(할당 실패는 Error). `attach`는 이미 이식한 `HudChatGeometry` 본체에 위임한다.
- **입력줄 접두**: `normalizePrefix`(첫 `:` 앞+`:`/뒤 본문 재합성), `changeRecipient`(작업 0 추가=공백+이름+공백·1 제거=찾은 이름과 뒤 공백, 유효하지 않은 접두는 비움·본문은 유효할 때만 첫 `:` 뒤로 자름, 작업>1 은 진단 0x561 후 계속), `containsRecipient`(대소문자 구분 부분 문자열, 빈 이름은 항상 일치). 입력 복사는 128바이트(127 초과는 Error), 접두는 128바이트 미만만 허용한다.
- **`handle`(0042fab0)**: 일반 사건은 PageUp/PageDown/Home/End/방향키만 로그 스크롤로 처리하고(좌우 버튼·해제 제외) 나머지는 기본 입력에 위임한다. 명령 사건은 전역 사건 값(명령 비트 포함)과 같은 코드일 때만 처리한다: 삽입(문자열 필요, `copy`/`paste`는 버퍼에 위임)·helper 메뉴(helper serial 일치)·부모 전달(현재 root 있을 때 1)·입력줄 제출(Console serial 일치 → 개행 추가·길이 -2 가 아니면 전송·유효 접두만 남겨 재정규화)·수신자 추가/제거(subscriber 로 이름 조회, 이름은 40바이트까지, 실패는 진단 0x13)·재부착(Push 뒤 현재 좌표). `appendLog`는 원문대로 **자식** 문맥(8 DWORD)을 저장하되 지정/복원은 **부모(root)** 문맥에 쓰며(원문 그대로) 맨 아래였다면 추가 뒤 맨 아래 요청을 한다. `destruct`는 현재 root 를 0 으로 쓴다.
- **도우미 메뉴**: 메타맵/전투 모드일 때만 서식·효과(+전투는 채널/플레이어/채팅 설정, 체크 항목은 설정을 두 번 읽어 `=0`/`=1` 할당 문자열과 체크 값 생성) 항목이 붙고, 색/배경은 21개·아이콘은 23개 단위로 묶어 `Colors a to b` 제목을 만들며(마지막 원소는 항목에서 제외, 1개 목록의 `size-2` 는 부호 없는 감김 4294967295) 키워드·스타일 8개·미리 정의된 색 20개·명령 3개·Copy/Paste 순으로 마친다.
- **`ClientChatManagerGlobals`**: 현재 manager 토큰 상자(00548714)·서식 캡처 81바이트·manager(0xa98)/내부(0x80) 할당과 수신자 저장소(4의 배수) 할당·반납(재초기화가 이전 manager 를 해제하지 않음, manager 객체 반납은 RangeError)·`makeClientChatManagerHooks`/`SubmitHooks`/`PacketHooks` 공장이 같은 소유자의 memory/allocate/release/addRecipient 를 기존 제출/prefix 처리기에 연결한다.
- **객체 메뉴 슬롯 표**: 원본 `ObjectMenuSlots.inc` 의 83개 VT(+a8 작성/+ac 열기)를 기계적으로 옮겨 동결하고 `objectMenuSlots(vtable)`가 미등록 VT 를 임의 메뉴로 바꾸지 않고 RangeError 로 거부한다. `ObjectHitVtables` 와 같은 VT 집합·순서다.
- 웹 적응 표기: C++ `logic_error`/`runtime_error`→`Error`, `invalid_argument`→`TypeError`, `out_of_range`→`RangeError`, `std::uint32_t&`→`{ value }` 상자, `std::optional<string_view>`→`null`/문자열, 값 초기화 없는 할당은 0 으로 채운 `Uint8Array`(소비자가 미작성 바이트를 읽지 않는다).

## 검사와 발견

새 시험 파일 3개, **42건 통과**(`chat-window.test.mjs` 27건·`client-chat-manager-globals.test.mjs` 10건·`object-menu-slots.test.mjs` 5건). ChatWindow 는 실제 `TextEntryGump`/`TextEntryBuffer`/`TextEntryStorage`(E439 지원 틀)와 실제 `GumpCreate` 위에서 동작하고 배경/스크롤/메뉴/Chatmanager 는 기록 훅이다. 객체 메뉴 표는 원본 `.inc` 83개 항목 전체와 대조하는 시험을 포함한다(파일이 없으면 건너뜀).

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류):** 명령 사건 전역 값은 명령 비트(0x02000000)를 포함하므로 전역 표와 사건을 같은 값으로 맞췄고, `appendLog`가 자식이 아니라 부모 문맥에 복원하는 원문 효과, 빈 접두(`:x`)의 정규화 결과, 목록 1개의 `size-2` 부호 없는 감김, 모든 할당 실패(0)가 하위 생성자만 건너뛰고 부착 훅은 0 토큰을 받는 점, `0x80` 내부 할당도 수신자 저장소 종류라 반납 가능한 점(manager 만 거부), 호스트 거대 배열 할당은 실제로 성공할 수 있어 실패 시험을 가짜 생성자로 바꾼 점을 기대값에 반영했다.

**변형 확인:** ChatWindow 314개(동등 23)·manager 전역 20개(동등 2)·객체 메뉴 표 250+5개(동등 0), 합계 589개 중 564개를 잡았다. 처음 생존한 항목(ChatWindow 68개)은 표시 비트 초기값·입력줄 폭/글꼴/색/배경 플래그·Slider/스크롤 인자·TYPE 수 경계·128바이트 입력/접두 한도·빈 접두 유효 검사·이름 40바이트 절단·subscriber 4바이트·식별 지역 크기·둘째 묶음 제목 등을 시험해 잡았다. 남은 25개는 동등 변형이다: 기본 생성이 이미 켠 비트의 중복 설정(`0xe00`↔`0xe01`)·바로 뒤에서 덮어쓰이는 값(접두 첫 BYTE)·범위 끝 `undefined` 읽기 루프(`<=`)·NUL 뒤 바이트를 자르는 방어 코드·해제 비트의 하위 비트 변형·`stringOf` 의 길이 판정 변형(NUL 로 잘린 뒤 사용)·토큰 소진 경계(2^32 개 할당 필요).

## 남은 경계

- 실제 연결(배경/스크롤/메뉴 빌더/Chatmanager/번역/설정 훅)·`ClientChatWindowGlobals`·`ClientChatRuntime`·`WarningMessages`·`MessageGump`(기준선 뒤 변경 판정 대기)·RawMenuControls 허브가 이 부품들을 조립하는 부분은 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
