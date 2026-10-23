# HUD의 NSInfo: 실제 생성과 일반 미션의 최소 경계

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 기존 `extracted/refined/originals/Netstorm.c`와 보호 `originals/Netstorm.exe`를 읽기만 했다. PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 새 게임/GUI 실행·디컴파일·기존 공유 소스 변경·빌드·커밋은 이 조사에서 하지 않았다.

## 창의 정체와 범위

HUD `+c0`, 전역 `00593150`은 **메타맵 Player List / Battle Status** 창이다. F6의 Netstorm Knowledge와 구별해야 한다. 원본은 `0047e9d0`에서 제목/도움말을 `Player List Window`/`playerListView` 또는 `Battle Status Window`/`battleStatusView`로 설정한다. 근거: [원문 102948행](../../extracted/refined/originals/Netstorm.c:102948), [103078행](../../extracted/refined/originals/Netstorm.c:103078). F6는 별도 `00492b60`이며 현재 [RawKnowledgeMenu.h](../../cpppj/src/client/RawKnowledgeMenu.h:6)의 실제 카드 작성기로 이미 분리되어 있다.

일반 캠페인/미션과 Edit에서는 메타맵 관문을 통과하지 않는 부착 경로를 복원할 수 있다. 그러나 메타맵이 아니어도 **생성자는 모든 자식을 항상 생성**한다. 숨긴 빈 기본 Gump 하나로 대체하거나, 사용하지 않는다는 이유로 PlayerInfo/BattleInfo 자식 생성 및 등록/serial 증가를 생략하면 원본과 다르다.

## 생성 0047ca50 → 0047c4b0

`0047ca50`은 `new(0xd8)` → `0047c4b0` → 반환된 현재 창을 `00593150`에 저장 → 비0이면 `flags &= ~1` 순서다. 정밀 C의 `void` 서명과 달리 PE는 생성한 포인터를 EAX에 유지하므로 HUD의 호출자가 그대로 받는다. 창이 숨겨지는 것은 실제 생성 뒤이며 생성 자체의 메타맵 조건은 없다. 근거: [101409행](../../extracted/refined/originals/Netstorm.c:101409), PE `0047ca66~0047caa8`.

`0047c4b0`은 기본 `004642c0(this,0,0,0,1)` 뒤 VT `0050d074`, `+20=movsx BYTE[00540421]`(실제 값 6)을 쓴다. 아래 순서로 전체 하위 트리를 생성하고 마지막에 자신의 `flags |= 0x800`을 쓴다. 근거: [101219행](../../extracted/refined/originals/Netstorm.c:101219).

| NSInfo 필드 | 생성 순서/실제 종류 | 생성 직후 효과와 부모 |
|---|---|---|
| `+a0` | 1. Background `new(0xb8)`, `004655d0(1.0f,1.0f,0x02000000,1,currentGenus,currentShapeFlag)` | `(0,0,this,0)`으로 부착한다. 다른 NSInfo 자식들의 부모다. |
| `+c4` | 2. ScrollingText `new(0xd8)`, `004bc060(0,0,0,0)` | visible을 끄고 `textFlags(+d0)|=8`, `004ba230(1)` → `004ba1b0(1)` 뒤 `+a0`에 `(0,0)` 부착한다. |
| `+c8` | 3. ScrollingText `new(0xd8)`, 같은 생성자 | 번역된 `~w~E%s\nPlayers in Zone %d\n`와 현재 서버명/zone을 서식화하여 파싱한다. visible을 끄고 `+a0`에 부착한다. |
| `+cc` | 4. Background `new(0xb8)`, 크기1×1, `0x02a00000` | visible을 끄고 `+a0`에 `(0,0)` 부착한다. |
| `+d0` | 5. Background, 위와 같은 생성 | visible을 끄고 `+a0`에 `(0,0)` 부착한다. |
| `+d4` | 6. BattleInfo `new(0x16c)`, `0047afa0` | visible을 끄고 `+a0`에 `(0,0)` 부착한다. |
| `+a4..+c0` | 7~14. PlayerInfo 8개, 각각 `new(0xbc)`, `00479d40` | 배열 순서대로 생성하고 각각 `+a0`에 `(0,0)` 부착한다. 생성자에 별도의 metaMode 관문이 없다. |

서버명 `0047c430`은 `0054db4c == -1`이면 현재 번역의 `LAN Server`를 정적 `00594970`에 복사한다. 그 밖에는 `00458f90(server,0,0,buffer,0x50,0)`를 호출한다. 이 현재 문자열/zone 포맷은 일반 미션의 생성에서도 실행되므로 임의 빈 문자열로 바꾸면 안 된다. 근거: [101163행](../../extracted/refined/originals/Netstorm.c:101163). 포맷은 `%s`와 `%d` 양쪽이 있으며 C에 인자가 적게 나온 부분은 PE 호출 스택으로 확정해야 한다.

`new` 실패 뒤 원문은 여러 위치에서 바로 포인터를 역참조한다. 안전한 제품 계약은 해당 할당 위치에서 명시 실패시키되, 생성에 성공한 앞부분을 없었던 것으로 처리하지 않는 것이다. 원문의 구조화 예외 처리 정리는 별도로 검증해야 한다.

## 항상 생성되는 PlayerInfo와 BattleInfo

PlayerInfo `00479d40`은 `+b0=0`, `+b8=0` 및 VT `0050cd64`, 종류 BYTE `00540440=37`을 쓴 뒤 다음 네 자식을 항상 만든다. 근거: [99754행](../../extracted/refined/originals/Netstorm.c:99754).

| 필드 | 항상 생성하는 실제 자식 | 후속 효과 |
|---|---|---|
| `+a0` | ScrollingText `004bc060(0,0,0,0)` | `004ba230(0)`으로 스크롤을 끈다. |
| `+a8` | 특수 버튼 `004256d0(16,15)`, 크기 `0xc8` | `004253e0` 기본 버튼 뒤 VT `005038e8`, `+c4=0`; 이후 버튼 `+ac |= 6`. 일반 Base/Stone 버튼만으로 이 VT를 대체할 수 없다. |
| `+a4` | TextGump `004bb170(0,0,0,0)`, 크기 `0xa4` | 현재 번역 `~r~EBATTLEMASTER`를 파싱하고 `004bb310(0,0)`으로 Fit한다. |
| `+ac` | StoneButton `004254b0(1,1,translate("Start Battle"),currentColor,0)`, 크기 `0x170` | `00424f90` 호출 뒤 실제 버튼과 자식을 생성한다. |

최종 부착 순서는 `+a0`, `+a8`, `+ac`, `+a4`이며 모두 해당 PlayerInfo가 부모다. PlayerInfo `00479fb0`은 **기본 Attach를 먼저 실행한 뒤** `00594fc8==0` 또는 `00595344==0`이면 즉시 반환한다. 따라서 일반 미션에서 이후 메타맵 문자열·선택·버튼 상태 계산은 생략되지만 앞서 생성한 네 자식은 남는다. 근거: [99855행](../../extracted/refined/originals/Netstorm.c:99855).

BattleInfo `0047afa0`은 VT `0050ce00`, 종류 BYTE `00540457`, `+ac/+b0=0`, `+164 BYTE=0`, `+168=2` 및 `+b4`부터 22개의 DWORD 쌍 `{0,1}`을 초기화한다. 이어 `+a0` ScrollingText, `+a4` TextGump, `+a8` ScrollingText를 항상 생성한다. ScrollingText 둘은 `SetScrollable(0)`→`SetWrap(0)`, Text는 Fit(0,0)이며 생성 순서대로 자신에게 부착한다. 마지막 `flags|=0x800`이다. 근거: [100398행](../../extracted/refined/originals/Netstorm.c:100398). `0047b480`도 기본 Attach 뒤 같은 metaMode/playerKnown 관문을 통과해야 나머지 전체 설정을 작성한다. 근거: [100635행](../../extracted/refined/originals/Netstorm.c:100635).

이 경로는 NSInfo root까지 최소 **50개의 상위 종류 노드**(root1+직접/배경 아래14+PlayerInfo 자식32+BattleInfo 자식3)를 갖는다. ScrollingText가 생성하는 Slider 및 그 화살표/손잡이, StoneButton의 배경/라벨은 여기에 추가된다. 실제 자식 우선 삭제와 등록표 정리도 함께 연결해야 한다.

## NSInfo 부착 0047e9d0: 일반 미션에서 실행되는 부분

정상 기본 부착 경로는 아래 순서다. PE `0047e9d0~0047ea6b`, [102849행](../../extracted/refined/originals/Netstorm.c:102849).

1. `00466750(this,x,y,parent,mode)`의 실제 공통 기본 Attach를 실행한다.
2. 현재 `this.+a0` 배경의 virtual `+18` Push를 호출한다.
3. 현재 NSInfo `baseWidth(+3c)`를 읽어 현재 배경 `+3c`에 저장하고 배경의 현재 scale을 곱해 `+44`에 저장한다.
4. 현재 `this.+a0`을 다시 읽고 NSInfo `baseHeight(+40)`를 현재 배경 `+40`에 저장한 뒤 현재 배경 scale을 곱해 `+48`에 저장한다.
5. 현재 `this.+a0` 배경을 `(0,0,this,0)`으로 virtual Attach한다.
6. **이후에** 현재 `00594fc8`을 읽는다. 비0이어야 현재 `00595344`를 읽는다. 어느 관문이든 0이면 `0047efdb`의 공통 epilogue로 이동하고 `ret 16`한다.

배경 Push나 Attach가 `+a0`/너비/높이를 바꾸는 경우 재조회 순서를 보존해야 한다. 여기서는 배경 확장 저장소를 미리 한 번 캡처하여 두 축에 재사용하지 않는다. 일반 미션 경로에서 PlayerInfo 표 정렬, Chat/메타맵 참가자 조회, 제목/도움말 변경은 실행되지 않는다.

관문 통과 뒤 로컬 메타 레코드의 `+a4`가 0이면 zone 전체 플레이어 목록을 만들고, 비0이면 battle의 최대8명 PlayerInfo와 BattleInfo를 배치한다. `0047e630`은 40×`0x1cc` 메타 레코드의 현재 alive/등급/이름을 정렬하여 `005930b0`의 표시번호→실제번호 표를 갱신한다. 이 전체 가지는 일반 캠페인·Edit 최소 연결의 완료 범위에 포함하지 않는다. 원문 C의 `unaff_EBP` 및 이상한 `fVar11` 카운터는 실제 PE 레지스터/스택과 대조해야 한다.

## 입력과 수명

VT `0050d074`의 실제 슬롯은 scalar소멸 `+0=0047c490`, Move `+10=00464d10`, Push `+18=00464560`, Attach `+20=0047e9d0`, Handle `+28=0047c8c0`, Broadcast `+2c=004665a0`다. `+24/+30`은 실제 빈 `004437f0`이고 별도 Idle 계산을 가정하지 않는다.

`0047c8c0`은 사건을 `&03ffffff`한 뒤 세 가지 메타맵 동작을 처리한다. `second`(세 번째 사건 인자)가 signed양수여야 한다. `02000026/02000025`는 현재 표시번호 표 `005930b0[second]`를 통해 실제 `0x1cc` 레코드를 찾아 `flags(+64)&1`일 때 각각 현재 커서 위치의 Player 메뉴 `004785f0` 또는 사제 탐색 `00433ba0(record.+b8)`을 실행한다. `020000ba`는 표시 표를 거치지 않은 **실제 번호**를 쓰고, alive관문 뒤 packet 초기화→현재 subscriber(+4)→값8→음향→송신 순서다. 나머지는 원래 사건 세 인자로 기본 `00464990`에 전달한다. 근거: [101343행](../../extracted/refined/originals/Netstorm.c:101343), PE `0047c8c0~0047ca4a`.

소멸 `0047c490`은 `0047c480`으로 파생 VT를 복구한 뒤 공통 `00464420`을 실행하고 scalar flags bit1이 있으면 자신의 저장소를 free한다. 자식 해제는 기존 GumpLifetime의 실제 자식 우선 트리를 사용해야 하며, NSInfo의 숨김을 삭제로 취급하면 안 된다.

## 현재 cpppj와 구체적인 연결점

| 원본/의존성 | 현재 재사용 가능 | 남은 연결 |
|---|---|---|
| 기본 생성/트리/수명 | `GumpCreate`, `GumpTree`, `GumpLifetime`, `RawMenuTree` | 현재 HUD 아래 동일 등록표·일련번호·직계 링크로 NSInfo 및 전체 자식을 등록한다. |
| 배경 `004655d0` | [GumpBackgroundCreate.h](../../cpppj/src/client/GumpBackgroundCreate.h:31), `RawMenuControls::BackgroundBackend` | `currentGenus/currentShapeFlag`, 같은 TYPE/SHP 및 파생 저장소를 공급한다. |
| ScrollingText 생성/스크롤/Slider | [ScrollingTextGumpWidgets.h](../../cpppj/src/client/ScrollingTextGumpWidgets.h:73), [ScrollingTextGumpMarkup.h](../../cpppj/src/client/ScrollingTextGumpMarkup.h:14) | 현재 RawMenuControls의 관련 객체/저장소는 내부 private다. API를 공개해 공유하거나 같은 실제 스타일/버퍼 처리기를 별도 어댑터에 연결해야 한다. |
| 한 줄 텍스트/버튼 | `TextGump`, `ButtonGump`, `StoneButtonGump` | `004256d0` 특수 버튼의 VT/생성 `+c4`와 가상 동작은 아직 별도 복원이 필요하다. |
| PlayerInfo/BattleInfo 부모 | 현재 해당 파생 모듈 없음 | 항상 실행되는 생성/하위 생성과 기본 Attach 뒤 mode관문을 먼저 복원한다. 메타맵 본문은 별도 명시 가지로 둔다. |
| metaMode / playerKnown | [RawCampaignSession.cpp](../../cpppj/src/client/RawCampaignSession.cpp:3288)의 같은 `core.ownerMode.challenge`, [PlayerKnown](../../cpppj/src/client/RawCampaignSession.cpp:2919) | 복사한 bool 대신 호출 위치마다 현재 값을 읽는다. `challenge`라는 호스트 이름이 원본 `00594fc8`과 연결되는 사실을 유지한다. |
| 플레이어/메타 레코드 | `RawPlayerIdentity`, 현재 [CampaignObjectMenuUi::chat/meta](../../cpppj/src/client/RawCampaignSession.h:58), Session `PlayerRecords()` | 일반 Session의 9×`0xb4` Player 저장소와 메타맵 40×`0x1cc` 기록/9×`0x124` battle 기록은 다른 자료다. PlayerRecords를 메타 레코드로 재해석하면 안 된다. |
| Netstorm Knowledge | `RawKnowledgeMenu`/`RawMenuControls::OpenKnowledge` | NSInfo와 통합하지 않는다. |
| Inventory | `InventoryGump`, `InventoryItemGump`, `RawProductionControls` | NSInfo 생성/normal Attach는 생산 격자 생성 `0043d790`/항목 `0043e120`을 호출하지 않는다. Inventory를 대체 패널로 만들 필요가 없다. 기존 생산 패널은 HUD `+b0`의 별도 자식이다. |

다음 일관된 단위는 **실제 NSInfo/PlayerInfo/BattleInfo 생성 전체와 일반 mode의 기본 부착/수명**이다. 기존 공통 생성·배경·텍스트·버튼을 재사용하고, 서버명/zone/번역 및 특수 버튼의 미복원 경계를 실제 자료로 연결한다. 이후 메타맵이 0임을 확인하는 일반 캠페인/미션/Edit에서 normal부착의 현재 포인터/크기/scale 재조회와 전체 자식 등록/해제를 검증할 수 있다. metaMode비0 가지까지 완료했다고 확장해서 보고하지 않는다.

후속 준비로 [HudNsInfo.h](../../cpppj/src/client/HudNsInfo.h)와 [구현](../../cpppj/src/client/HudNsInfo.cpp)에 위 일반 Attach만 복원했다. 전체 NSInfo 자식 생성은 별도 부모의 책임으로 남겼다. metaMode/playerKnown 양쪽이 켜지면 실제 전체 메타맵 후속 훅이 필요하며, 없으면 일반부착의 부분효과 뒤 명시 거부한다. 메타맵 본문을 빈 성공으로 대체하지 않는다.

[독립 도구](../../tools/decomp_hud_nsinfo_attach.py)는 기존 HUD의 격리장치만 재사용하고 원문 `0047e9d0~0047ea71` 및 `0047efdb`의 실제 합류 RET를 실행했다. [새 fixture](../../cpppj/tests/fixtures/hud-nsinfo-attach-x86.tsv)는 **155입력×x87 두 제어어=310행**, 정상302/명시중단8, 제어어간 관측차이0, 실제13,672명령(CRT0)이다. 최초 범위의 분기·두축 크기·현재배경교체·배율·callback뒤모드조회·부분쓰기를 관측한다. 양쪽 관문이 켜지면 `0047ea71` 이후는 명시 `metamapLayout` 후속효과로만 기록한다. fixture의 `operation=reflow`는 공통 격리 실행기의 입력 규약이며 주소는 NSInfo 진입으로 제한했다. 전체 메타맵 명령을 실행했다는 뜻이 아니다.

[새 검사](../../cpppj/tests/HudNsInfoTests.cpp)는 두 검사로310행 전체 raw/24전역/trace/중단을 비교하고 실제 메타맵 후속 누락을 별도로 거부하는지 확인하도록 작성했다. 매크로는 `NETSTORM_HUD_NSINFO_ATTACH_FIXTURE`다. 기존 CMake 변경·빌드·검사 실행·GUI 연결은 이 준비 작업에서 하지 않았다. 원문/도구/공통장치/fixture 해시는 `extracted/cpp-hud-nsinfo-attach-20261023-native.json`에 남겼다.

단위368의 별도 `HudDock`/입력 fixture 통과를 NSInfo 전체 생성·하위 PlayerInfo/BattleInfo 구현이나 전체 HUD GUI 완성으로 해석하지 않는다.
