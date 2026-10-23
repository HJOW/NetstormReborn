# NSInfo 전체 부모 생성과 필수 자식 조립

현재 PC와 마지막 전체 정밀 디컴파일 PC는 `VM-W11-Codex / 2026-10-23`이다. 보호 10.78 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 기존 `extracted/refined/originals/Netstorm.c`와 PE를 재사용했으며 새 디컴파일, 게임/GUI 실행, 공유 CMake 변경, 빌드와 커밋은 하지 않았다. 기존 `HudNsInfo` 일반 Attach 모듈도 수정하지 않았다.

새 `HudNsInfoCreate.h/.cpp`는 `0047ca50` wrapper, `0047c4b0` NSInfo, `00479d40` PlayerInfo, `0047afa0` BattleInfo, `004256d0` 특수 버튼의 전체 부모 생성 본체를 분리한다. NSInfo는 메타맵 Player List/Battle Status 창이며 일반 F6 Knowledge와 별개다. 일반 미션에서도 이 생성자는 자식을 생략하지 않는다.

## 실제 생성 순서와 저장소

NSInfo의 필수 자식14개는 **NSInfo 직계 배경1개와 그 배경 아래13개**다. 배경 아래에는 ScrollingText2, separator Background2, BattleInfo1, PlayerInfo8이 있다. PlayerInfo는 각각4개 자식, BattleInfo는3개 자식을 추가하므로 부모 단계에서 root를 포함하여 최소50개 노드를 만든다. ScrollingText가 실제로 만드는 Slider/화살표, StoneButton이 만드는 배경 등은 이50개에 추가된다.

| 함수/원문 행 | 복원한 순서 |
| --- | --- |
| `0047ca50`, C101409행 | new0xd8 → 실제 NSInfo 생성 → 반환토큰을 `00593150`에 저장 → 반환노드의 visible 비트만 끄기. 최상위 new가0이면 생성 없이 current0을 쓴다. PE는 EAX에 생성포인터를 유지한다. |
| `0047c4b0`, C101219행 | 기본 Gump(0,0,0,1) → VT0050d074 → 종류 BYTE00540421 부호확장 → Background+a0 → roster+c4 → title+c8와 현재 서버/zone 문자열 → divider+cc/+d0 → BattleInfo+d4 → PlayerInfo+a4..c0 순서 → root flags OR800. |
| `00479d40`, C99754행 | 기본Gump → VT0050cd64, +b8=0, 종류 BYTE00540440 부호확장, +b0=0 → Scrolling+a0/SetScrollable0 → 현재 DWORD005411c8로 TYPE 조회, +b4 저장, null이면 line1912 진단 → 특수버튼+a8, +ac OR6 → Text+a4, 실제 번역 `~r~EBATTLEMASTER` 파싱/Fit(0,0) → Stone+ac, 번역 `Start Battle`, 현재005bec3c/문맥0, Fit → a0,a8,ac,a4 순서 부착. |
| `0047afa0`, C100398행 | 기본Gump → VT0050ce00 → +b4부터22쌍 각각 뒤값1/앞값0 → +168=2 → 종류 BYTE00540457 부호확장 → +164 BYTE만0 → Scrolling+a0/scroll0/wrap0/Attach → Text+a4/Fit/Attach → Scrolling+a8/scroll0/wrap0/Attach → flags OR800 → +ac/+b0=0. +165..167은 보존한다. |
| `004256d0`, C30217행 | 실제 기본 Button(16,15,0) → VT005038e8 → +c4=0. +c0은 보존한다. 기본 Button/Stone으로 가상표를 바꾸어 대체하지 않는다. |

배경 new가 성공한 뒤 현재 frame `005ca924`를 먼저, 현재 TYPE `005ca920`을 다음에 읽는다. 첫 배경은 backgroundFlags02000000, 두 divider는02a00000이며 크기1×1, 기본flags1이다. roster는 visible을 끄고 textFlags OR8 → SetScrollable1 → SetSlider1 → 현재 배경에 부착한다. title/divider/BattleInfo의 visible은 생성/파싱 후 각 원문 위치에서 끈다. PlayerInfo 자체는 여기서 visible을 끄지 않는다.

모든 파생 저장소는 기존 공통 Gump 등록표에서 생성한 노드와 같은 주소여야 한다. `current`는 원문00593150의 장기 소유 DWORD 참조다. `base`, `node`, `player`, `battle`, `button`은 순수 현재 저장소 조회다. 함수 도중 고정this의 주소는 유지하며, 배경·title·roster 등 원문이 다시 읽는 자식 필드는 효과 뒤에 다시 조회한다. 따라서 `formatZone`이 title 포인터를 바꾸거나 생성/설정 콜백이 현재 종류/배경 자산을 바꾸어도 뒤쪽은 그 현재 값을 쓴다.

## 문자열·초기 갱신·Attach 경계

PE `0047c635..0047c677`의 정확한 순서는 현재 zone format 원문포인터 `00540bac` 조회 → 실제 번역 → 현재 zone DWORD `005467b8` 캡처 → **인자 없는** 서버명 `0047c430` → 캡처한 format/server/zone으로 형식화 → **현재 title**을 읽어 ParseScroll이다. 정밀 C가 서버명 함수에 zone 인자를 붙인 서명은 잘못됐다. 서버 조회 중 zone이 바뀌면 이미 캡처한 값으로 포맷한다. 번역 후 zone이 바뀌면 바뀐 값을 캡처한다.

문자열은32비트 토큰으로 전달한다. 번역 포인터를 무조건 소유 std::string으로 복사하여 뒤 콜백에서 바뀐 현재 문자열을 숨기지 않는다. 토큰에 대응하는 메모리는 해당 소비까지 살아 있어야 한다. `formatZone`은 원문의 `%s`/signed32 `%d` 인자를 받고 같은 파서가 읽을 출력 토큰을 반환한다. 서버명 훅은 실제 현재 서버/번역을 연결해야 한다. 빈 서버명이나 임의 라벨을 성공 대체값으로 공급하지 않는다.

ParseScroll/ParseText/FitText/FitStone와 SetScrollable/SetSlider/SetWrap은 기존 실제 본체에 연결한다. 이 부모 생성자는 그 호출을 생략하거나 임의 Refresh 카운터로 대체하지 않는다. 별도 root 전체 Refresh는 원문 생성자에 없으므로 추가하지 않았다. 각 기존 텍스트 생성자가 만드는 버퍼/Slider와 Parse가 수행하는 현재 버퍼 갱신은 그 실제 처리기가 소유한다.

PlayerInfo `00479fb0`와 BattleInfo `0047b480`의 일반 Attach는 공통00466750 호출 **뒤** 현재 metamap00594fc8, 비0일 때 현재 playerKnown00595344를 읽는다. 어느 값이0이면 정확한 원문 epilogue로 반환한다. 새 모듈의 `AttachPlayer/AttachBattle`은 이 일반 몸체를 제공한다. 둘 다 비0인 전체 메타맵 후속은 `metamapLayout(kind,token)` 필수 경계로 구별하고 없으면 실패한다. NSInfo 자체 일반 Attach는 기존 `HudNsInfo::Attach`를 그대로 사용한다.

자식 할당0 이후 원문이 바로 역참조하는 경로와 실제 SEH unwind는 이 준비 범위의 독립 검증 대상이 아니다. 부모 단계의 타입/포인터 접근에서 유효 저장소를 요구하며, 실패 시 이미 생성한 앞부분을 없었던 것으로 되돌리지 않는다. 실제 자식 우선 소멸/등록 해제는 기존 공통 Gump 수명에 연결해야 한다.

## 독립 원문 관측과 제품 검사

`tools/decomp_hud_nsinfo_create.py`는 네 부모 생성자/wrapper/특수 버튼과 `0047c430` 서버 getter의 실제 PE 명령을 실행한다. Player/Battle의 실제 일반 Attach 관문과 원문 합류 epilogue도 실행한다. 서버 getter의 LAN 번역 뒤 NUL 복사는 실제 본체다. 메타맵 본문 진입은 별도 `meta` 경계다.

85입력 × x87 제어어027f/037f = **170행**이며 정밀도차0, 실제 명령214,310이다. 정상120행, 명시 경계 중단50행이다. 전체 부모 생성 정상76행은 각각50개 노드를 만든다. 최상위 new 실패, 세 종류의 signedBYTE 경계, 원격/LAN 서버, zone signed32, 콜백 뒤 현재 title/종류/자산/버튼 flags/TYPE 진단 복귀·모드 변경, 빌린 format/서버 번역 바이트 변경, wrapper의 기존 current 덮어쓰기와 여러 단계의 부분 중단을 포함한다.

앞10열은 `name,operation,globals,fill,typeResult,format,server,mutation,stop,fail`, 뒤8열은 `completed,result,globalsout,nodesout,stringsout,trace,stackdelta,nativeinstructions`다. 각 노드는 요청폭과 별도로0x180BYTE 전체를 관측한다. 번역 버퍼는 각512BYTE, 실제 서버80BYTE와 뒤176BYTE guard를 함께 관측하고, 형식화 출력은 스택주소 대신 문자 내용을 관측한다. `mutation`은 `경계명+1기반번호/주소/DWORD`를 세미콜론으로 연결한다. 종류 전역3개는BYTE폭으로 쓴다. `stop`은 경계기록/입력변경 후 복귀 전에 중단한다. 미완료 EAX는 의미 반환값으로 비교하지 않는다.

공통Gump/배경/Scrolling/Text/ButtonBase/Stone 및 Parse/Fit/TYPE/번역/원격서버/형식화/가상Attach는 명시 하위 장치다. 이 oracle이 실제 Slider/Stone 내부 생성 명령이나 전체 공통 등록/트리 본체를 실행했다고 주장하지 않는다. 실제 GUI에서는 이 경계를 기존 검증된 모듈로 연결해야 한다.

새 `HudNsInfoCreateTests.cpp`의 검사2개는 앞10열만 장치로 공급하여170행 전체 현재 전역/노드/미작성·guard/번역·서버 바이트/효과순서/부분중단을 제품 부모와 대조하고, 실제 메타맵 후속 누락을 거부하도록 작성했다. CMake 매크로는 `NETSTORM_HUD_NSINFO_CREATE_FIXTURE`다. **아직 CMake 미등록·미빌드·미실행** 상태다. 이 준비를 실제 HUD/NSInfo 전체 입력·Draw·소멸 또는 메타맵/게임 완성으로 계산하지 않는다.

## 후속 실제 연결

루트는 같은 실제 등록표에서 파생50노드 및 기존 텍스트·Slider·Stone 내부 자식을 생성하고, 각 가상 Attach를 새 Player/Battle 일반관문과 기존 공통/Text/Scrolling/Stone 처리기에 분배해야 한다. 숨김 root라도 생성/serial/초기 파싱·Fit/등록은 모두 유지한다. 특수 버튼 VT005038e8의 메타맵 전용 Draw/선택·아이콘 내용, PlayerInfo/BattleInfo 전체 메타맵 배치/Handle, 원격 서버 이름 공급과 전체 NSInfo 수명 검증은 별도 후속이다. F6 Knowledge나 빈 배경으로 대체하지 않는다.
