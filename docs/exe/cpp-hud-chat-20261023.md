# 10.78 ChatWindow와 mode4 NSInfo: 다음 HUD 연결을 위한 경계

2026-10-23, 현재 호스트와 마지막 전체 정밀 디컴파일 호스트는 **VM-W11-Codex / 2026-10-23**으로 일치한다. AGENTS.md와 공통/닷넷 인수인계를 다시 읽었다. 기존 보호 PE와 refined C를 읽었으며 새 디컴파일·게임·GUI·빌드·커밋은 하지 않았다. 이 문서는 실제 정상 HUD 연결 완료 판정이 아니다. 단위368의 divider/dock 코어 검증과 후속 ChatWindow/NSInfo 조립을 구별한다.

- 보호 PE `originals/Netstorm.exe`: SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`.
- 정밀 원문 `extracted/refined/originals/Netstorm.c`: SHA256 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`.
- 아래 C 행 번호와 원시 가상 표는 이 정밀 원문 및 보호 PE를 기준으로 한다. 정적 관찰·격리 native 관측·아직 연결하지 않은 제품 경계를 구별한다.

## 우선 필요한 실제 경계

ChatWindow root의 Draw는 빈 함수다. 회색 사각형 하나를 그리거나 빈 노드에 chat 이름을 붙여도 원본 창을 구현한 것이 아니다. 원본은 **helper 돌 버튼, 평면 배경, Console/TextEntry, ScrollingText와 그 Slider/StyleTextBuffer**를 같은 공통 Gump에 생성하고 이 자식들이 그린다. 정상 본문 표시에는 기존 ScrollingText 계층을 재사용한다. 후속 새 `TextEntryBuffer`/`TextEntryGump`는 실제 생성·입력·Draw 본체를 분리했으나 이 문서 시점에는 미등록·미빌드이며 실제 ChatWindow 어댑터 연결이 남아 있다.

`0047c4b0/0047ca50`은 기존 `RawMenuControls::OpenKnowledge`의 기술 카드 popup과 다른 **mode4 NSInfo**다. 이 root도 Draw가 비어 있고, 배경·텍스트 두 개·가로 배경 두 개·22개 설정을 가진 특수 하위 패널·PlayerInfo 여덟 개가 표시를 맡는다. 일반 SID Inventory 격자를 이 특수 패널로 대신할 수 없다. NSInfo의 생성/정상 Attach 후속은 별도 독립 분석 및 새 모듈과 합쳐야 한다.

## ChatWindow 생성과 수명

원문 생성자 `00431810`은 `Netstorm.c:40306`, 공장 `00431ee0`은 `40586`부터다. 생성자 C는 스택 인자 복구가 흐트러져 있으므로 `00431838..00431bb9`의 보호 PE 명령을 함께 읽었다.

| 대상 | 생성/크기 | 원본 상태와 필수 연결 |
| --- | --- | --- |
| root | `new(0x13c)` → `00431810(200,100)` | `004642c0(command=0,width,height,flags=1)` 뒤 VT `00504a94`, +20은 signed BYTE `00540424`(초기9), 최종 flags에 `0xe00` OR |
| +a8 helper | `new(0x170)` → `004254b0(0,0,">",005acd24,null)` | `00424f90`으로 글자 크기에 맞춤. title은 번역한 `Chat Helper`, help key는 `chathelper`. 다섯 번째 `005acd24`는 기존 생성자의 **unused 인자**이므로 이를 실제 글자색으로 해석하지 않는다. 실제 기본 문맥과 버튼 세 상태의 색을 사용한다. |
| +ac background | `new(0xb8)` → `004655d0` | 초기 폭 `width−20`, 높이 `height−measure("M").height`, 배경 flags `0x04000000 | 005b5dc0`, Gump flags1, type/frame0 |
| +a0 Console | `new(0x128)` → `0043f820` → TextEntry `00452430` | 초기 폭 `width−20`, 높이는 현재 root 문맥으로 측정한 `M` 높이, 초기 문자열0, font5, 배경 `0x04000000 | 005b5e60`, 글자색 `005b5dd4`. +f8=1, +100=80, +f4=0을 쓰고 visible을 지운다. |
| +a4 ScrollingText | `new(0xd8)` → `004bc060(width−20,height−MHeight,font5,005b5dd8)` | `004bc3b0(&00548758,owns=0)`으로 생성 중 임시 소유 버퍼를 반납하고 **클라이언트 수명 공유 버퍼**를 빌린다. +d4=1, SetSlider(1), SetScrollable(1), textFlags의0x10 제거. |

root +b0=128, +b4=0, +b8..137은 Console 입력과 연결하는128바이트 접두 문자열, +138=1이다. `00451e00(Console,root+b8)`로 입력을 연결하고 `0042ebc0`으로 `:` 앞의 수신 대상 접두를 정리한다. Console +110에는 실제 autocomplete callback `0042e540`을 넣는다. 생성자 말미는 `0054da91`이0일 때 스타일/색/그림 autocomplete 목록을 한 번 준비하며 TYPE/SHP 프레임 높이도 읽는다. 이 초기화는 현재 기하 모듈의 범위가 아니다.

공장 `00431ee0`은 한 번만 `clear→0042f680`, `quit→0042e520`을 Console 명령표 `0043f660`에 등록한다. 최종 공유 StyleTextBuffer +34 callback을 `0042f590`으로 설치한다. 현재 공유 버퍼의 정적 초기 생성 위치/원래 maxLines는 이번에 확정하지 않았으므로 임의128줄 소유 버퍼로 대체하지 않는다. 원문 정적 종료부는 `Netstorm.c:222485`에서 `004bbf20(&00548758)`를 호출한다.

root scalar 소멸자는 `0042f6b0` → `0042e8d0` → 기본 `00464420`이다. 자식은 공통 자식 우선 수명으로 반납하고 공유 버퍼는 root보다 오래 살아야 한다. `ScrollingTextGumpWidgets::ReplaceBuffer(...,owns=0)`와 빌린 버퍼의 소멸 계약을 그대로 사용한다.

## 가상 표와 Draw

보호 PE VT `00504a94`의 +20은 `0042e6e0` Attach, +24는 빈 `004437f0`, +28은 `0042fab0` Handle, +2c는 기본 `004665a0`, +30은 빈 `004437f0`다. +1c `00419df0`는 Draw가 아니라 현재 x/y로 가상+20을 호출한다.

실제 자식 Draw는 다음 기존 경계로 이어야 한다.

| 원본 | 제품 재사용/잔여 |
| --- | --- |
| 배경 `00465c60/00465880` | `GumpBackgroundCreate`와 `GumpBackgroundDraw`, type0 평면 채우기. root 전체를 임의 배경으로 칠하지 않는다. |
| helper 돌 버튼 | `StoneButtonGump::Construct/FitToText/Attach/Draw`와 `ButtonGump::Handle/Broadcast/Idle`, 실제 내부 배경 자식도 생성 |
| ScrollingText `004ba6f0` | `ScrollingTextGump::Draw`, `ScrollingTextGumpWidgets::Construct/Attach/Handle/ReplaceBuffer`, 실제 Slider와 공통 StyleText/Markup/글꼴/클립 |
| Console VT `00506a6c`, Draw `00451f00`, Handle `0043f8a0` | 새 `TextEntryGump`/`TextEntryBuffer`가 실제 생성·Attach·편집·Draw·Console Tab/제출 본체를 분리한다. 현재 미등록·미빌드이며 아래 독립 관측 경계를 적용한다. platform/Console.cpp는 앱 stdout 경계라 대체물이 아니다. |

## Attach `0042e6e0`의 정확한 순서

원문 `Netstorm.c:37767`부터, 실제 보호 PE `0042e6e0..0042e8cf`다. root 폭/높이를 각각 W/H, signed +b4를 L로 쓴다. 아래는 계산 설명이며 부모/자식은 각 콜백 뒤 현재 포인터를 재조회한다.

1. 공통 `00466750(root,x,y,parent,mode)`.
2. 배경 Push. 배경 기본폭=`W−L`, 기본높이=`H−(Console visible ? Console.baseHeight : 0)`. 배율 크기도 갱신한다.
3. helper 기본높이와 배율높이는 **+138 비영이고 Console visible일 때만15**, 그 외0. helper visible flag를 대신 지우지 않는다.
4. 배경을 `(float(L),0,root,0)`에 Attach.
5. **변경 전 ScrollingText의 현재 표시/scroll**에 대해 `004bc2c0` 조회 → 빈 `004437f0` → ScrollingText Push.
6. ScrollingText 기본/배율 크기를 현재 배경 크기로 복사. `004ba3d0(text,조회값)`으로 scrollRequest=−2와 bottomPadding을 쓰고 Redraw(false). 텍스트를 `(0,0,background,0)`에 Attach.
7. helper Push → `(float(L),H−helper.baseHeight,root,0)`에 Attach.
8. Console Push. 기본/배율 폭만 현재 텍스트 폭으로 복사하고 높이는 유지한다. `(float(L+17),H−Console.baseHeight,root,0)`에 Attach.

`005037e0`은 **17.0**, `00504a90`은15.0이다. 초기 생성의 `width−20`을 재부착의 L+17/폭 복사와 합쳐 임의로 보정하면 원문이 달라진다. 배경 폭·높이 계산의 native FST는 float32 기본 크기를 저장한 뒤에도 저장 전 x87 중간값에 scale을 곱하므로 기본 크기를 읽어 다시 곱하는 구현과 구별한다.

`004bc2c0`은 EnsureLayout/ProcessRequest 뒤 soft-line 체인을 현재 scroll/높이로 순회하고 마지막 줄 여백 인덱스를 반환한다(`Netstorm.c:149817`). 단순 AtBottom bool이나 전체 줄 수가 아니다. 기존 `ScrollingTextGump`에는 이 함수의 공개 API가 아직 없다. `RequestBottom`의 extra 인자가 현재 ScrollBottom에서 무시되더라도 조회 자체의 배치/스크롤 효과와 호출 순서를 삭제해서는 안 된다.

## 표시와 숨김

- `0042f250`(`Netstorm.c:38453`)은 root +138에 `value!=0`만 쓴다. root visible/Console visible을 바꾸거나 즉시 Attach하지 않는다.
- `0042f270`(`38465`)은 root가 없으면 반환한다. 있으면 **root Push → 현재 Console Push → Console visible 결정 → Console 현재 x/y로 가상 Attach(parent0,mode0) → root 현재 x/y로 가상 Attach(parent0,mode0)** 순서다. value0이며 challenge `00594fc8`도0일 때만 Console을 숨긴다. challenge에서는 value0 요청도 Console을 표시한다.
- `0042e410/0042e440`은 부모 체인에 `00542914` 사건을 second1/0으로 보낸다. 실제 `HudDock::Handle(004c6530)`이 chat divider 위치를 저장/이동하고 `0042f250/270`을 호출한다. ChatWindow 자체의 visible bool 하나로 합치지 않는다.
- 실제 HUD 생성 `Netstorm.c:158520`은 Chat root에0x20000을 OR하고 title=`Chat Window`, help=`chatView`를 설치한다. 바깥 divider와 같은 부모/serial/bubble을 사용해야 한다.

## Handle `0042fab0`의 범위

`Netstorm.c:39167`부터 약5.7KB인 이 본체는 아직 새 제품 모듈로 옮기지 않았다. 일반 사건은 마우스 버튼/release를 구별하고 PageUp/PageDown/Home/End/스크롤 요청을 실제 ScrollingText에 전달하며 나머지는 기본 Help `00464990`으로 간다. command 사건은 helper click(serial 일치)→실제 controlled Chat Helper 메뉴, 수신 대상/채널·스타일 문자열, copy/paste, Console 입력 편집/실행, 설정 변경 등을 처리한다.

따라서 다음 어댑터가 `BaseHandle`만 돌려주거나 입력을 삼킨 뒤 완료로 보고해서는 안 된다. Multiplayer 통신은 현재 사용자 목표에서 제외하되, 싱글플레이에서도 노출되는 Console/TextEntry 편집·스크롤·helper·도움말·숨김은 분리해서 복원해야 한다. 기존 `RawChatChannels`는 실제100개 채널 기록 및 이름 정제일 뿐 ChatWindow Handle/Console 구현이 아니다.

## 실제 색과 자산

색 전역은 PE의 BSS0을 쓰지 않고 실제 파일 팔레트에서 `BuildPaletteColorTable`의 결과를 받는다. 원문 로더 `004a4850`, `Netstorm.c:130642/130667..670` 및 `PaletteColors.cpp`의 호출 순서와 일치한다.

| 원문 전역 | 목표 RGB | 10.78 제품 표 | 사용 |
| --- | --- | --- | --- |
| `005b5dc0` |84,80,81|named[32]|본문 배경, `0x04000000 | index` |
| `005b5e60` |114,106,103|named[30]|Console 배경, `0x04000000 | index` |
| `005b5dd4` |250,248,238|named[31]|Console 글자색 |
| `005b5dd8` |206,193,178|named[33]|ScrollingText 글자색 |
| `005acd24` |181,140,111|named[9]|helper 생성의 unused 인자로 전달됨. 실제 버튼 글자색으로 오인하지 않음 |

helper 돌 버튼과 NSInfo 질감 자식은 실제 `fortGump` TYPE/SHP를 사용한다. HUD가 설정하는 공유 Help 배경 `005ca920=TYPE[005411c8]`, `005ca924=0`과 돌 버튼 계층의 A01 frame1 문맥을 구별한다. 자식별 생성자가 요구하는 현재 type/frame을 같은 자산으로 공급한다.

## 기존 공통 모듈과 후속 어댑터 연결표

| ChatWindow의 필수 효과 | 현 제품 API 또는 필요한 후속 |
| --- | --- |
| root 등록/가상 분배/자식 수명 | `RawMenuTree.factory/collection/tree/lifetime/events/input`, `RawButtonControls`의 같은 안정된 노드 저장소 및 Attach/Handle/Broadcast/삭제 분배 |
| helper 생성/라벨/문맥/측정/누름 | `RawMenuTree.stones`, `StoneButtonGump::Construct/FitToText`, 공통 ButtonGump 분배 |
| M 측정 | `RawMenuControls::MeasureText`에 root의 실제32바이트 문맥, 문자열M, limit−1을 제공 |
| 배경 생성 | `RawMenuControls::BackgroundBackend` 또는 같은 자산을 바인딩한 `GumpBackgroundCreate` |
| 텍스트 생성/StyleTextBuffer 공유 | 기존 `ScrollingTextGumpWidgets`/`StyleTextBufferStore`를 외부 자식 저장소에 구성. 메뉴 내부 전용 저장소를 복사하지 않음 |
| 외부 표시 | `RawMenuControls::SetExternalGumps(slot,...)`의 owns/text/draw로 같은 픽셀/글자/배경 문맥에 등록. 생산slot0/미니맵slot1과 충돌하지 않는 별도 slot 필요 |
| 끝 줄 조회 | **미복원 `004bc2c0` 공개 본체 추가 필요**. `HudChatGeometryHooks.bottomIndex`는 이 실제 구현을 요구함 |
| 맨 아래 요청 | `ScrollingTextGump::RequestBottom(token,extra)` |
| Console/TextEntry | 새 `TextEntryGump`/`TextEntryBuffer`를 같은 저장소에 바인딩하는 실제 어댑터가 필요. 숨긴 Console도 실제 노드/버퍼/문맥으로 생성해야 ShowInput 이후 정상 동작함 |
| Chat Helper 메뉴/기본 Help | `MenuGumpWidgets/Controls/Input`와 `RawMenuControls::BaseHandle`, 실제 번역/문자열 수명/사건 큐 |
| 현재 root/공유 로그 | `00548740` 대응 포인터는 root Kill/삭제 시 정리. `00548758` 대응 버퍼는 root 교체보다 긴 Client 수명 |

## mode4 NSInfo 정적 생성

원문 `0047c4b0`은 `Netstorm.c:101215`, 공장 `0047ca50`은 `101405`부터다. root new0xd8, VT `0050d074`, kind signed BYTE `00540421` 초기6, 최종 flags0x800 OR, 공장 끝에서 visible을 지운다. VT +20=`0047e9d0`, +24=빈 `004437f0`, +28=`0047c8c0`, +2c=`004665a0`다.

| root 필드 | 실제 생성/초기 연결 |
| --- | --- |
|+a0|1×1 기본 질감 배경, flags0x02000000, type=`005ca920`,frame=`005ca924`, root 자식 |
|+c4|ScrollingText(0,0,0,0), 숨김, textFlags|8, scroll/slider1, 배경 자식 |
|+c8|ScrollingText(0,0,0,0), `~w~E%s\nPlayers in Zone %d\n`와 현재 zone 형식, 숨김, 배경 자식 |
|+cc/+d0|1×1 질감+상하 경계 배경0x02a00000 두 개, 숨김, 배경 자식 |
|+d4|`new(0x16c)`→`0047afa0`, 숨김, 배경 자식. ScrollingText 둘/TextGump 하나와22개의 값/활성 쌍을 가진 특수 패널이며 일반 SID InventoryGump가 아님 |
|+a4..c0|`new(0xbc)`→`00479d40` 여덟 개, 배경 자식. 각각 ScrollingText/TextGump/16×15 버튼/Start Battle 돌 버튼을 생성하고 현재 player binding을 초기0으로 설정 |

HUD는 NSInfo 기본 크기를 전체 화면으로 쓰고0x20000 OR/부착한 뒤 관계 등록에 포함한다(`Netstorm.c:158507`). mode4에서 vertical 위치170, 미니맵 숨김/production 숨김/NSInfo 표시로 바꾸는 것은 `HudDock::Handle`이며 NSInfo 내부 내용을 대신하지 않는다.

`0047e9d0`(`Netstorm.c:102841`)은 공통 부착 후 배경을 root 폭/높이로 맞춘다. 추가 내용 분기는 **challenge00594fc8 비영 및00595344 비영**일 때만 들어간다. 이 안에서 현재 Client/player/zone 기록(`005467d8`, stride0x1cc/0x124), 스크립트/번역, 현재 battle/PlayerInfo 상태를 읽으며 `0047e630`, 텍스트 FitHeight와 특수 패널 Attach를 호출한다. challenge0 경로에 가상의 Player List/임의 카드 내용물을 채우면 원문과 달라진다. 복잡한 분기 C는 unaff_EBP 등 경고가 있어 실제 명령과 현재 independent NSInfo 분석으로 확정해야 한다.

기존 기술 지식 카드 `RawMenuControls::OpenKnowledge`/`RawKnowledgeMenu`를 mode4 root 대신 붙이는 것도 의미가 다르다. NSInfo 하위 PlayerInfo/특수 패널의 실제 생성·Attach·Draw·Handle과 일반/챌린지 모드의 현재 데이터 경계가 후속 필수다.

## 이번 새 기하 모듈과 독립 관측

새 `client/HudChatGeometry.h/.cpp`는 **0042e6e0 Attach와0042f250/270 표시 전환만** 분리한다. 전체00431810 생성·Console/TextEntry·0042fab0 Handle·실제 Client 어댑터·HUD Draw를 완료한 모듈이 아니다. `HudChatNode`는 같은 기본 저장소에 실제 네 자식과 접두 버퍼를 보유하고, 기존 공통/텍스트 경계가 없으면 명시 오류로 거부한다. CMake/기존 파일을 바꾸지 않았으며 새 모듈/검사는 이 문서 작성 시점에는 빌드하지 않았다.

`tools/decomp_hud_chat_geometry.py`와 `fixtures/hud-chat-geometry-x86.tsv`는 **1,980입력×두 x87 정밀도=3,960관측, 실제548,716명령, 정밀도 차이0, 정상RET/stackdelta0**이다. attach3,888행/helper24행/input48행이다. Native 전체 범위는 `0042e6e0..0042e8cf`, `0042f250..0042f265`, `0042f270..0042f300`, `004ba3d0..004ba3f0`, 빈 `004437f0`다. 기본 Attach/자식 Push·Attach·Redraw는 실제 호출 인자 관측기로 대체하고 링크/좌표는 변경하지 않았다. `004bc2c0`은 명시 반환 입력으로 대체했으므로 soft-line 조회 본체의 보증은 아니다. `004ba3d0` 요청 본체는 실제 실행했다.

새 `HudChatGeometryTests.cpp`는 매크로 `NETSTORM_HUD_CHAT_GEOMETRY_FIXTURE`, 검사 `HudChatGeometry_Native3960SizesAndCallbacks`로 같은 입력의 다섯 노드 flags/기본·배율 크기, helper BYTE와 가상/외부 호출 순서를 대조한다. Native 내부 Redraw는 기하 모듈의 외부 requestBottom 경계 안쪽이므로 직접 trace 대조에서 제외한다. 실제 공유 트리 링크·실제 bottomIndex·전체 텍스트/Console·픽셀·사용자 GUI까지 통과했다고 확대하지 않는다.

도구 SHA256 `60a088770393cee7464c59f5e3e3eb03f53d4803f4cb6393dca5669e721ccdcb`, TSV SHA256 `9c3cb7eba22c800405b726861d102c4fa0deb4b911b5dbe4a77238984f22504a`. 범위/제외/정밀도별 명령 수는 `extracted/cpp-hud-chat-geometry-20261023-native.json`에 있다. 기존 보호 파일과 기존 소스/빌드 파일은 변경하지 않았다.

## 10.72 비교 기록

기존 정밀 메타데이터의 대응 후보는 Chat constructor `004b1a40`, factory `004b28f0`, Handle `004b2450`, Console constructor `0043f050`, NSInfo factory `004eebb0`, Attach `00470f00`이다. 이번 구현/격리 관측은10.78만 대상으로 했으며 CD판의 자식 크기·호출 순서·색 인덱스 동일성을 주장하지 않는다.
