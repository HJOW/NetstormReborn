# ChatWindow 생성·공장·입력 본체와 독립 원문 관측

2026-10-23, 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**으로 같다. AGENTS.md·공통/닷넷 인수인계와 reverse-engineer-anything의 독립 분담 절차를 적용했다. 기존 보호 PE와 refined C를 읽었고 새 디컴파일·빌드·GUI·게임·커밋은 하지 않았다. 기존 `HudChatGeometry`/`TextEntryBuffer`/`TextEntryGump` 및 그 검사는 변경하지 않았다. 이번 새 파일은 다음 HUD 조립을 위한 본체이며 실제 Client HUD 연결 완료가 아니다.

- 보호 PE SHA256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`.
- refined C SHA256: `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`.
- 새 제품: `client/ChatWindow.h/.cpp`. 같은 `HudChatNode`와 실제 `TextEntryGump`/편집 버퍼를 사용한다. 다른 자식 생성·공통 메뉴/Chatmanager/Identity는 실제 기존 모듈에 바인딩해야 하는 명시 훅이며 빈 성공 fallback은 없다.
- 새 관측: `tools/decomp_chat_window.py`, `fixtures/chat-window-x86.tsv`, `extracted/cpp-chat-window-20261023-native.json`.
- 새 제품 검사: `tests/ChatWindowTests.cpp`, 그 검사에만 쓰는 `ChatWindowTestSupport.h`. 미등록·미빌드·미실행 상태다.

## 원문과 실제 네 자식

원문 생성 `00431810`은 `Netstorm.c:40306`, 공장 `00431ee0`은40586, Attach `0042e6e0`은37767, Handle `0042fab0`은39167이다. 생성자의 스택 복구 경고는 보호 PE 명령과 독립 CPU 관측으로 보완했다. root의 Draw+24는 실제 빈 `004437f0`이며 자식들이 배경/글자를 그린다.

| 원문 필드 | 생성 순서와 같은 실제 저장소 |
| --- | --- |
| root | 기본4642c0(command0,width,height,flags1) → VT00504a94 → kind signed BYTE00540424. 실제 기본 폭/높이 및 배율 크기, helperVisible BYTE1 |
| +a8 helper | new170 → 실제4254b0(0,0,`>`,unused005acd24,null) →424f90 FitToText → 번역한 `Chat Helper` 제목 →`chathelper` help.005acd24를 임의 글자색으로 해석하지 않음 |
| +ac background | newb8 →4655d0(width−20,height−현재M높이,04000000\|현재005b5dc0,flags1,type0,frame0) |
| +a0 Console | new128 → 실제 동결 `TextEntryGump::ConstructConsole(width−20,M높이,null,font5,04000000\|005b5e60,005b5dd4)`. 실제 편집 버퍼/두 Slider도 생성. changeNotice1,maxLength80 |
| +a4 ScrollingText | newd8 → 실제4bc060(width−20,height−M높이,font5,005b5dd8). 이후4bc3b0으로 생성 중 소유 버퍼 반납→Client 수명00548758 버퍼를 owns0으로 빌림. callback DWORD+d4=1 |

root+b0=128,+b4=0이다. 최초 부착은 background `(leftPadding,0,root)`, Console `(leftPadding,height−M높이,root)`, helper도 **동일한 height−M높이**, ScrollingText `(0,0,background)` 순서다. 이후 Slider1/Scrollable1, textFlags&~10, Console.multiline0/visible0, root.flags|e00, 실제 접두 첫 NUL/Console.SetText/NormalizePrefix를 수행한다. Console.autocomplete에는 원문 callback 식별자0042e540을 설치한다.

재부착은 동결된 `HudChatGeometry::Attach`의 실제42e6e0 본체를 사용한다. 최초 생성의 width−20/height−M 수식을 재부착의 leftPadding+17/helper높이15와 합쳐 보정하지 않는다. geometry의 bottomIndex는 실제004bc2c0 조회를 요구하며 임의 AtBottom bool/전체 줄 수로 대체할 수 없다.

## 한 번 초기화·공장과 버퍼 수명

completionReady0054da91이0이면 두 벡터에256개씩 **`~[C.0]`..`~[C.255]`, `~[B.0]`..`~[B.255]`**를 원문 순서대로 추가한다. 현재 TYPE 수00541348을 바깥 반복마다 읽고 각 TYPE의+114 프레임 수를 한 번 읽는다. 실제419850 프레임 헤더의 signed WORD 높이가8..16일 때만 `~[Itype.frame]` 아이콘 문자열을 추가한다. 마지막에 ready BYTE1을 쓴다. 색을 RGB/이름 표로 재계산하지 않는다.

공장431ee0은 registered00548744가0일 때 `clear→0042f680`, `quit→0042e520`, flag1을 실제 Console 명령표43f660에 등록하고1을 쓴다. new13c→Construct(200,100) 결과를 현재 root00548740에 설치한 다음, **그 시점의 root.text.buffer+34**에0042f590을 설치한다. 새 모듈은 실제 StyleTextBuffer.onAppend로 이 callback을 바인딩하는 장치를 요구한다. 공유 로그는 root보다 길게 살고 Console의0x28 입력 버퍼와 구별한다. 소멸42e8d0은 VT 재설정→현재 root0→기본 자식 우선 수명이다.

## Handle과 helper 메뉴

일반 사건은 command02000000이 없고 좌/우 마우스가 아니며 release가 아닐 때만 Home→RequestScroll0, End→RequestBottom, PageUp/Down→현재 root 기본높이의 음/양 `_ftol`, Up/Down→−5/+5 상대 scroll을 처리해1을 반환한다. 나머지는 같은 기본 Help464990에 원시 사건을 위임한다.

command 분기에는 별도 release 관문을 추가하지 않았다. 현재00542b24 문자열 사건은 정확한 소문자 `copy`/`paste`를 같은 실제 편집 버퍼에 보내고 그 외 문자열은 AppendText한다. 이후 원문처럼 나머지 분기/기본 Help도 계속 수행한다. helper click은 현재 helper serial과 일치해야 실제 controlled 메뉴를 조립한다. 전체 형식/Chat Effects/채널/Player/Chat Controls와 Text Colors/Background Colors/Icons/Keywords/Styles/미리 정의된20색/Commands/Copy/Paste를 저수준 기존 Menu 함수 계약으로 구성하며 helper 전체를 빈 callback 하나로 넘기지 않는다.

색/BG 그룹 간격21, 아이콘 그룹 간격23, 마지막 원소 제외와 원문의 마지막 그룹 제목 수식도 보존한다. 아이콘 첫 그룹의 시작1과 색 첫 그룹 시작0을 구별한다. 설정 toggle은 첫 현재 config로 assignment문자열을 만든 뒤 번역→현재 명령 ID 저장→두 번째 config→checked 항목 순서다. 메뉴 제목은 번역 뒤 현재 메뉴를 pop하며 포인터/문자열의 실제 수명은 기존 메뉴 어댑터가 소유해야 한다.

changeNotice005429f0은 그 시점의 전역 root가 있으면 부모00542914(first0,second1)를 전달하고1을 반환한다. Enter005427d8은 현재 Console serial이 일치할 때 LF Append→실제 Chatmanager42da00 입력→128BYTE Copy→유효 수신 접두만 남김→SetText→NormalizePrefix 순서다. 일반 Enter 키를 ChatWindow가 임의 제출하지 않는다. 실제 Console→부모 사건 경로가 필요하다.

수신자 추가00542794/제거00542af8은 기존 RawPlayerIdentity.Initialize/Resolve 계약을 통해 subscriber(second DWORD)의 이름을 읽는다. 실패 진단은 원문처럼 first DWORD를 출력한다. 성공 시42ea60의 현재 접두 유효성/대소문자 구분 strstr/공백 처리·본문 보존을 복원했다. Normalize42ebc0은 현재 입력을 두 번 Copy하고 원문 접두+b8을 다시 작성한다. refresh0054298c는 root Push→현재x/y Attach(parent0,mode0) 후 기본 Help로 간다.

## 독립 CPU 관측

최종 **90입력×두 x87=180행**, 실제 **178470명령**, 정밀도 차이0이다. 생성20행/공장6행/Handle144행/Normalize8행/소멸2행이며 모두 유효 진입부터 정상 RET와 스택까지 관측했다. 실제 부모의 불연속5767BYTE Handle과 중첩 Console/TextEntry/0x28 버퍼/_ftol, 수신자 추가·제거/Normalize 본체를 실행했다. 초기화 ready0도4행으로 관측하며512색 문자열 및2TYPE×높이−1/7/8/9/16/17/32767의 실제 아이콘 판정을 출력과 대조할 수 있다. helper의21/23 경계가 있는45/45/25 원문 String 배열도 별도 입력이다.

먼저 완료한 **83입력/166행**은 `fixtures/chat-window-base-x86.tsv`에 기존30열 그대로 보존했다. 보존본 SHA256 **`a0ce2d0f46f2ef61ddda12e266d314254ce149957c9eac6a22da50f4279e4618`**은 확장 전 결과와 완전히 같다. 새31열 자료는 동일 관측을 유지하며 초기화/수신자 제거/그룹 경계를 더한다.

앞21열은 name/operation/크기/사건·인자/문자열·기존 입력/등록·모드·config·recipient·Identity 결과/세 String 벡터/Console 노드·버퍼·전체 할당의 **raw 입력**이다. 뒤10열은 root16DWORD/Console48DWORD/ScrollingText4DWORD/접두/현재입력/세완성벡터/전체 장치trace/반환/stackdelta/실제명령 수다. 새 검사 **`ChatWindow_IndependentNativeCreationInputHelperAndLifetime`**, 매크로 **`NETSTORM_CHAT_WINDOW_FIXTURE`**가180행의 실제 제품 출력을 직접 비교한다. 지원 헤더는 기존 새 Entry 검사의 장치 코드를 복사해 실제 TextEntry/버퍼를 구성하며 기존 파일은 수정하지 않았다. 이 검사는 아직 빌드·실행하지 않았다.

관측 장치 경계는 Root/Helper/Background/ScrollingText의 큰 공통 생성·StyleTextBuffer 교체, 글꼴·트리/가상/Slider, 저수준 Menu, Identity.Resolve, Chatmanager, 메모리/clipboard/시계다. String/strstr/벡터 append는 명시 순수 계약이다. 초기화 벡터 장치는 capacity=end를 공급하여 성장 append 경로를 선택하며 STL allocator/여유 capacity의 in-place 삽입 자체는 검사하지 않는다. 부모의 숫자/문자열/높이 분기는 실제 원문이다. 큰 하위 장치의 픽셀/전체 링크/소유 메모리 반납까지 관측했다고 확대하지 않는다.

도구 SHA256 **`5ab61c2658f805b06bedc89b26ceb2cb7bda77c7270a2d16ce1517c569aeac3d`**, 최종 TSV SHA256 **`3c02c531d119660cfc41d53bb921a65931e991a14fbbdacac85a5ac494c4a4ed`**. 정확한 실행 range/call/write 집계와 현재 호스트는 JSON 보고서에 있다.

## 다음 HUD 연결의 남은 경계

실제 Client 어댑터에서 네 자식/Slider/두 버퍼 저장소와 Gump 등록·가상 분배·자식 우선 삭제를 연결해야 한다. 생성 훅은 기존 StoneButtonGump/GumpBackgroundCreate/ScrollingTextGumpWidgets/StyleTextBufferStore를 실제 호출해야 하며 임의 빈 노드로 성공시켜서는 안 된다. Console은 새 모듈에서 실제 TextEntryGump를 직접 호출한다. 공유 로그 callback0042f590은 원본 파일 로그0054874c/현재 파일 열림/WriteBytes에 연결해야 한다. clear/quit/0042e540 자동완성 callback의 실제 명령 registry·순환 후보 처리와 Chatmanager 전달, 기존 미복원004bc2c0도 후속 어댑터의 명시 필수다.

이번 자료는 GUI/실제 사용자 입력 route·실제 글꼴/픽셀·통신·모든 allocator 실패/EH·128BYTE를 넘는 원본 stack overflow·전체 캠페인 완료를 증명하지 않는다. 실제 dock 연결 후 숨긴 Console의 표시 전환/Tab·Enter·clipboard·helper/로그 scroll·모달/미션 이탈·재진입을 검증해야 한다. 기존 HUD/viewport 미완료를 제거하지 않는다.
