# TextEntry/Console 본체와 ChatWindow 연결 경계

2026-10-23, 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**으로 일치한다. AGENTS.md·공통/닷넷 인수인계와 reverse-engineer-anything의 독립 질문 분담 절차를 적용했다. 보호 PE와 기존 refined C를 읽었고 새 디컴파일·빌드·게임·GUI·커밋은 하지 않았다. 새 파일은 후속 HUD 연결용이며 실제 ChatWindow 완료 판정이 아니다.

- 원본 `originals/Netstorm.exe` SHA256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`.
- refined C `extracted/refined/originals/Netstorm.c` SHA256: `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`.
- 새 `client/TextEntryGump.h/.cpp`, `tests/TextEntryGumpTests.cpp`, `tools/decomp_text_entry_gump.py`, `fixtures/text-entry-gump-x86.tsv`만 이 조사에서 작성한다. 하위 실제0x28 버퍼는 독립 담당자가 작성한 `TextEntryBuffer`와 같은 상태/소유 메모리를 공유한다. 기존 파일/CMake/공유 코드에는 등록하지 않았다.

## 원문 본체와 공개 API

| 10.78 주소 / refined C 행 | 새 API와 실제 상태 |
| --- | --- |
| `00452430` / 66343 | `Construct`: 실제 배경 생성 → 편집 버퍼 new0x28/생성 → 현재 시각+0.5 → 문맥/크기/idle → Reset/Redraw → 두 Slider new0xd8/생성·부착 → scroll0 → 초기 문자열 전체 선택. TextEntry VT00508e10, kind0054042a |
| `0043f820` / 51627 | `ConstructConsole`: 위 생성 뒤 VT00506a6c, kind00540425, autocomplete/submit0, wordStart/wordLength0, resetWord1, recipientHighlight1 |
| `004527b0` / 66511 | `Attach`: 공통 Attach → V/H Push → 같은 자식 기본/배율 크기 → visible 비트 → V/H Attach → 현재 scroll 재적용 |
| `00451c90` / 65992 | `SetScroll`: 줄 clamp → **쓰기 전 scrollLine**의 길이로 열 clamp → M 측정/실제 `_ftol` page → H range/page/position/현재 position 조회 → V range/page/position/현재 position 조회 → Redraw |
| `004528b0` / 66561 | `RelativeScroll`: 현재 signed DWORD에 delta를 감김 덧셈한 뒤 위 본체 |
| `004528e0` / 66573 | `Handle`: 문자/수정키/Enter/선택/clipboard·현재 줄 scroll·부모 통지·Slider 사건 |
| `0043f8a0` / 51662 | `HandleConsole`: 실제 Tab 후보 callback/Return 제출 후 기본 Handle. Shift 자체는 resetWord를 초기화하지 않음 |
| `00451f00` / 66126 | `Draw`: 실제 배경 → M 측정 → 현재 raw 줄/선택 복사 → 수평 문자열 삭제 → LF 표시 → 일반/수신 대상/선택 글자 및 caret |
| `00451e90` / 66100 | `Idle`: !dead && !readOnly이며 현재 시각≥blinkAt일 때 phase 증가/Redraw 후 **두 번째 현재 시각**+0.5 |
| `00451e00/00451e20/00451e80` / 66049/66062/66088 | `SetText/AppendText/CopyText`: 같은 버퍼 Reset/끝 삽입/선택 복구/전체 복사. AppendText의 원본 EAX는 내부 삽입 결과이며 이번 void API/관측의 반환 contract에서는 제외함 |
| `00451b80/00451c60` / 65919/65975 | `SetExcluded/SetHorizontal`: 이전 제외문자 free 후 새 raw 문자열 복사 및 **LF 반복 제거**. horizontal 설정 후 !void이면 Push/current x,y Attach |
| `00452730` / 66481 | `Destruct`: 소유+buffer비영일 때 버퍼 Destroy → 저장해 둔 객체 토큰 delete → buffer0 → 기본 소멸. 빌린 버퍼는 보존 |

TextEntry의 +b8/+bc는 실제 두 Slider, +c0은 버퍼 소유, +c8은 double blinkAt, +d0/+d4는 위상/읽기전용, +d8/+dc는 shift 기준/끝, +e0/+e4는 현재 줄/열, +e8..fc는 V/H 표시·원시 입력 전달·multiline·변경 통지·recipient 색, +100은 최대길이, +104는 별도 제외문자 소유 주소, +108은 버퍼다. Console은 +110/+114 callback, +118 resetWord, +11c/+120 단어 시작/길이를 추가한다. +c4/+10c/+124는 생성자가 쓰지 않는 필드를 그대로 보존한다.

## 입력의 정확한 관문과 통지

기본 Handle의 문자 관문은 !command02000000 → 마스킹 code가 좌/우 마우스 아님 → readOnly0 → !release80000000 순서다. 이 관문에 들어온 모든 키는 처리 성공 여부와 별개로 Redraw(false)를 한 번 요청하며 forwardInput이 켜져 있으면 원시 사건을 부모로 보낸다. 최대길이 양수일 때는 현재 length+1<maxLength만 허용한다. 선택 삭제 후 길이로 보정하지 않는다. Enter는 multiline일 때 실제 전달 사건을 LF10으로 바꾸며, Enter 부모 사건02000038은 항상 먼저 전달한다. changeNotice020000bf → Redraw → forward 순서다.

Console Tab/Return의 파생 관문은 readOnly를 먼저 검사하지 않는다. 따라서 readOnly1에서도 Tab callback/제출 callback이 먼저 실행될 수 있고, 뒤의 기본 Handle에서 편집만 차단된다. Tab은 선택이 비어 있고 callback이 있을 때만 실제 후보를 조회한다. resetWord1이면 ASCII 공백/콜론 뒤까지 역방향으로 찾은 현재 문자열 포인터와 단어 길이/shift를 보내고, resetWord0이면 **null 포인터**와 이전 길이/shift를 보내 후보를 순환한다. null 반환과 빈 문자열 반환을 구별한다. Return은 현재 전체 입력의 제출 callback 이후 resetWord1 및 기본 Handle을 수행한다.

Slider 사건02000036/0200002e는 위 문자 관문 뒤의 별도 분기라 readOnly/release 여부와 분리된다. change는 실제 V/H serial을 읽고 불일치이면 원본 진단 줄0x246을 호출한다. step은 serial 일치를 보기 전에 M을 측정하고 실제 H serial 및 second1/3이면 +M폭,2/4이면 −M폭을 열 delta로 쓴다. Paused/Guide/메뉴/선택 route 관문은 공통 입력 계층의 별도 범위다.

## Draw·메모리 수명 주의점

Draw의 표시 줄 수는 ceil(height/M높이)가 아니다. `_ftol(height)%M높이`가 비0인지 더한 `height/M높이`를 다시 `_ftol`한다. 수평 열 삭제도 표준 std::string erase의 끝 clamp와 다르다. 원본 String은 요청 수가 현재 길이를 넘으면 삭제하지 않으며, 길이/선택 쌍을 다시 보정하지 않는다. LF는 최초 복사 길이를 기준으로 공백으로 치환한다. 선택 글자는 현재 backgroundFlags&ff, 선택 사각형/caret는 현재 기본 textContext[2], 유효 수신 대상 접두는0059afd8의 실제 yellow를 사용한다.

`TextEntryBuffer`의 state/data/줄offset 메모리는 노드보다 짧게 살면 안 된다. 0x28 버퍼 생성 직후에는 data[0]이 아직 NUL이 아니므로 즉시 원문 Reset이 필요하다. 원문의 exact-capacity NUL 쓰기는 physical nominal+1 이상 가드로 수용하되 요청 size와 state.capacity는 바꾸지 않는다. 배경·Slider·TextEntry 파생 주소와 공통 기본 Gump 주소는 같아야 한다. 기본 소멸은 자식 우선 실제 공통 수명에 맡기며 TextEntry는 소유 버퍼만 반납한다. +104 제외문자의 소멸 free가 원문에 없으므로 임의 추가하지 않았다.

## 독립 PE 관측과 제품 검사 계약

도구는 원본 분기/루프/메모리 복사 및 중첩 `TextEntryBuffer` 본체를 실제 실행한다. 생성/Console 생성/Attach/SetScroll/Handle/Console Handle/Draw/Idle/문자 교체/Append/제외문자/horizontal/소멸을 **1,146입력×두 x87=2,292행**, 실제 **295,786명령**으로 관측했으며 정상 RET·스택이 모두 일치하고 정밀도 차이0이다. Handle1724행,Console240행,Draw140행,scroll100행,Idle24행,제외문자16행,두 생성각12행,Attach/horizontal각6행,SetText/Append/소멸각4행이다.

앞13열은 `name,operation,node,buffer,blocks,arguments,text,excluded,clipboard,clock,completion,recipient,fill`의 raw 입력이다. 뒤10열은 `nodeout,verticalout,horizontalout,bufferout,blocksout,clipboardout,trace,result,stackdelta,nativeinstructions` 관측이다. 새 검사 `TextEntryGump_IndependentNativeConstructorAttachInputDrawLifetime`, 매크로 `NETSTORM_TEXT_ENTRY_GUMP_FIXTURE`는 실제 노드48DWORD·두 자식각11DWORD·버퍼10DWORD·반납 블록/가드까지 전체 저장소·장치 호출/순서·반환을 직접 대조한다. 아직 빌드/실행하지 않았으므로 제품 통과 판정을 하지 않는다.

명시 외부 경계는 공통 Gump 가상/기본 생성·Attach/Push/Redraw/Bubble/소멸, Slider 생성/range/page/position, 메모리/clipboard, 시계, 글꼴 측정/출력/화면위치/픽셀 fill, recipient/autocomplete/submit callback이다. Slider 장치는 전달 position을 그대로 저장하며 자체 clamp/화살표/손잡이는 기존 별도 Slider 검사가 맡는다. 글꼴은 BYTE마다4+BYTE%5폭/높이14인 독립 장치이며 실제 폰트/픽셀 보증이 아니다. Draw 임시 String 및 기본 ASCII isspace는 명시 순수 계약으로 분리했다. 전체 ChatWindow Handle0042fab0·실제 공통 route·자산/픽셀·GUI·실제 Clipboard/타이머는 이 관측이 검증하지 않는다. 실패 할당/콜백 중 노드 삭제와 원본128BYTE 선택 임시버퍼를 넘는 stack overflow 입력도 미검증이다. CopyText는 별도 버퍼 CopyAll 본체 검사를 재사용하며 이번 자료에서 반환 출력까지 다시 대조하지 않는다.

도구 SHA256 `dad2f8d4eef5ae77a8333db50e7468bfdf67b1d5039f65a802a4bd83ceb2de84`, TSV SHA256 `8b0aa5420c85f4cc20d97be777334bb99467b45d96cf16c869a8824f79f0e8c5`. 세부 실제 range/call/write/SHA는 `extracted/cpp-text-entry-gump-20261023-native.json`에 있다.

## 다음 실제 ChatWindow 어댑터의 최소 연결

`RawMenuTree`의 같은 안정된 저장소에 TextEntry/Console/Slider/버퍼 object를 등록하고 `GumpBackgroundCreate`, `ScrollGumpWidgets`, `ScrollGump`, `GumpTree/Lifetime/Events`의 실제 기존 API를 hooks에 바인딩한다. Draw는 `RawMenuControls::BackgroundBackend`·같은 TextBackend와 clip·팔레트, Idle은 실제 GumpFrame 등록과 현재 시계, 입력은 실제 가상 Handle route로 연결한다. 원시 버퍼는 `StyleTextBuffer`와 별도 저장소다. Chat 로그는 Client 수명의 공유 StyleTextBuffer00548758을 빌리며 Console은 자기0x28 입력 버퍼를 소유한다.

이후 실제00431810 생성·00431ee0 공유 callback/registry·0042fab0 Handle, ScrollingText의004bc2c0 공개 조회가 남아 있다. 현재 HudChatGeometry의 bottomIndex 훅을 AtBottom bool/전체 줄 수로 대신하지 않는다. 생성/Attach/수명까지 연결한 뒤 실제 HUD에서 chat ShowInput/Enter/Tab/선택/스크롤/helper/모달/삭제/재생성을 검증해야 한다. NSInfo는 별도 `HudNsInfo` 조사·모듈을 참조한다.
