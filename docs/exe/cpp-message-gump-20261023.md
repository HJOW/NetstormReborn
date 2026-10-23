# 10.78 Pause 메시지 생성 경로

마지막 정밀 디컴파일은 `VM-W11-Codex / 2026-10-23`이다. 같은 호스트의 기존 `extracted/refined/originals/Netstorm.c`와 보호 `originals/Netstorm.exe`를 재사용했다. 새 디컴파일, 게임, GUI, 설치, 원본 파일 수정은 수행하지 않았다.

## 원문 경계

`004d3090`은 기존 `MessageBoard::Show`에 복원된 20칸 serial 표, 실제 루트 직계 자식 조회 `00464060`, 중복 검색, 삭제, 최초 빈 칸 선택, 오른쪽 위 배치를 사용한다. 이 작업에서는 그 본체를 다시 만들지 않는다. 종류11은 `00594fa4`의 미션 억제 상태에서도 허용되는 예외다.

파생 생성자 `004d0600`의 종류11 경로는 다음 순서를 가진다.

1. `004bb170(this,0,0,0,0)`으로 같은 TextGump를 생성한다. 기본 문맥은 `{0,0,0,0,0,0,1,1}`이며 종류 BYTE `00540448`의 기본값45와 공통 생성/serial/Clip 처리는 TextGump가 담당한다.
2. `+0`에 `00517b60`, `+b0`에 double0, `+a4/+a8/+b8`에 종류/플레이어/매개변수를 기록한다. `+ac/+bc`는 쓰지 않는다.
3. 현재 `00594fc8`을 읽는다. 영점이면 `0048f480(player)`, 비영이면 `0042bbb0(005467d8,player)`를 호출한다. Pause에서는 그 반환 이름을 쓰지 않아도 호출 자체는 생략하지 않는다. `0048f480`은 signed1..8에서 현재 Player180바이트의 `+0xc` 이름을 반환하고 나머지는 빈 문자열을 반환한다. Challenge는 원문의 별도 범위/실제 manager 조회가 필요하다.
4. `004d2c9c`에서 원문 `00516010`의 `~E~wGame Paused!`를 `004de9a0`에 전달한다. `004d2cab`에서 번역 반환을 `004bc800`의 현재 StyleText 문맥으로 파싱하고 마지막 `004bb310(this,0,0)`으로 측정한다. 원문의 `~E/~w` 처리와 색상은 기존 StyleText/팔레트 바인더를 사용한다.

이 경로는 시계, 음향, `00465330` idle 등록, `00418070`의 10초 기한 설정을 호출하지 않는다. 기한0은 영구 메시지이고, Pause 해제는 기존 `MessageBoard::Show(11,0,false,0,...)`의 같은 검색/지연 삭제를 사용한다. 가상표의 Draw `+24`는 기존 `004bb380` TextGump 본체이며 새 Draw를 만들 필요가 없다. TextGump의 실제 head/클립/위치/StyleText 출력 및 두 번 Clear 뒤 scalar 삭제를 재사용한다.

## 새 제품 계약

`MessageGumpNode`는 기존 TextGumpNode에 검색키/기한을 추가한다. `MessageGump(create,hooks).Construct(token,11,player,parameter)`를 기존 `WarningMessages::Impl::boardHooks.create`의 종류11 분기에서 호출한다. 같은 관리자의 WarningNode를 이 노드에서 파생시키면 기존 종류17의 필드 접근과 20칸 표를 함께 보존할 수 있다.

경계 `constructText`, `challenge`, `playerName`, `challengePlayerName`, `translate`, `style`, `fit`는 실제 현재 저장소/공통 모듈에 연결해야 한다. 선택된 Challenge 이름 경계 누락과 종류11 이외의 생성은 명시 거부한다. 다른 메시지의 소리/만료/서식 분기를 Pause 구현으로 대체하지 않는다. 관리자는 종류11에 새 idle 등록을 추가하지 않아야 한다.

## 독립 검증

`tools/decomp_message_gump.py`는 먼저 원본 `004d0600`의 선택 경로를 실행해 `message-gump-x86.tsv`를 동결했다. 324개 입력을 x87 `027f/037f`에서 각각 실행해 648회 정상 반환과 39,312명령을 관찰했다. 현재 challenge0/1/최상위비트, signed 플레이어 검색키, 매개변수, 세 번역 결과, 이름/번역 경계의 필드 변경을 교차했다. 192바이트 파생 상태와 호출 순서가 두 정밀도에서 일치했다. 정상 반환의 스택, SEH, 비휘발 레지스터, x87 제어/스택 복구도 확인했다.

TextGump 생성, 이름 조회, 번역, StyleText 파싱, 측정, CRT 큰 스택 확보는 명시 외부 경계다. 따라서 이 관측만으로 실제 폰트/픽셀, 공통 등록표 id와 serial 연결, 루트20칸/Draw/Idle 수명을 검증했다고 말할 수 없다. 이 부분은 기존 실제 모듈/공유 관리자의 통합 검사 대상이다.

격리 MSVC 컴파일/링크 `message-build1.log`는 경고0/오류0/종료0이며, 공유 빌드 산출물을 덮어쓰지 않고 이전에 복사한 `netstorm.lib`를 사용했다. 새 `MessageGump_PauseNative324`, 미선택/미복원 분기 거부 검사 총2개가 `message-focused1.log`에서 실패0이었다. CMake 등록과 실제 WarningMessages 연결은 부모 작업 단위의 범위로 남긴다. 검사 fixture 매크로는 `NETSTORM_MESSAGE_GUMP_FIXTURE`다.

이 결과는 Pause 메시지 생성 분기의 복원이며 전체 ESC 메뉴/HUD 통합, 메뉴의 모든 명령 효과, Early Missions/전체 캠페인/데모3스테이지 완주, Edit 전체 기능의 완료 증명이 아니다.
