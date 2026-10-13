# 메뉴 생성·장식/제목 자식·소멸·부착 보정

후속 [메뉴 출력/입력/객체 부착](cpp-menu-gump-interaction.md)에서 메뉴·장식 생성자의 식별값 쓰기를 실제 +20 command로 수정하고 Draw/Handle/Broadcast/AttachObject를 추가했다. 당시 소스 해시는 앞 단계 상태를 나타낸다.

2026-10-13, **마지막 디컴파일 PC: HJOW-Athlon**. 현재 호스트/IP는 **HJOW-Athlon / 192.168.0.94**이며 같은 PC의 정밀 프로젝트를 사용했다. AGENTS.md와 두 LEFT_JOBS를 확인했다. 사용자 창/게임 실행 허용을 반영했으며 이번 단계는 정적 분석과 빌드로 진행했다.

앞선 [메뉴 크기/확장/controlled 스택](cpp-menu-gump-frame.md), [메뉴 항목/레이아웃](cpp-menu-gump-layout.md), [다중 줄 제목 생성/수명](cpp-scrolling-text-gump-widgets.md)에 정상 메뉴 생성과 실제 장식 자식·수명을 연결한다. **Client/UberGump의 기존 메뉴 어댑터와 raw 자동 GUI·건설/미션 완주는 미완료다.**

## 정적 근거

계획은 `tools/ghidra/menuwidgets-functions.json`, 읽기 전용 내보내기는 `extracted/menuwidgets/{originals,originalCD}/creation.c`, 함수 범위는 `functions.tsv`, 실제 PE 명령은 `assembly.txt`다. `tools/analyze_menuwidgets.py`가 `cpppj/menuwidgets.json`에 가상 표/정렬/여백/전역 주소와 현재 구현 해시를 기록한다. 패치 **13개**, CD **18개** 함수를 내보냈다. CD에는 정상 소멸 경로의 작은 보조 함수가 여러 개 있다.

| 기능 | 10.78 | CD 10.72 |
| --- | --- | --- |
| MenuGump 정상 생성 | 004775f0 | 00451a70 |
| 메뉴 소멸 몸체/scalar | 00476ec0 / 00476f40 | 00451650 / 004533a0 |
| 메뉴 항목의 StyleText 소멸 | 00475f00 | 004533c0 |
| 메뉴 가상 Attach | 00476be0 | 004530d0 |
| 가로/장식/세로 controlled 시작 | 00477a10 / 00477ae0 / 00477bb0 | 00450e00 / 00450ed0 / 00450f90 |
| 장식 배경 생성/Draw/Redraw | 00465d10 / 00465db0 / 00465e60 | 00494ec0 / 00494f80 / 00495050 |
| 장식 기본 scalar 경로 | 004d4c40 | 00495200 → 00495259 → 00405d00 → 00405d41 |
| 양수 크기 변경 영역 요청 | 00499370 | 00456e20 |

메뉴 가상 표는 `0050c6f0`/`00501fa0`, 장식 표는 `0050ae7c`/`00504928`이다. 두 PE의 signed +20 식별 BYTE 초기값은 메뉴 **26**, 장식 **57**이다. 배경/모서리 타입·프레임·팔레트 번호는 실제 초기화 전역을 별도로 공급한다. PE의 미초기화 0을 정상 타입 값으로 사용하지 않는다.

CD 메뉴 소멸 몸체는 scalar 표의 실제 call을 따라 `00451650`으로 확인했다. 해당 몸체는 메뉴 개수 감소/선택 통지 후 `004516bc`의 전체 항목 소멸과 `004516de`의 기본 소멸 경로를 수행한다. Ghidra의 일부 인자/float10 복원은 불완전하므로 실제 call/스택/FPU 명령을 함께 판독했다.

## 정상 메뉴 생성

`MenuGumpWidgets`는 기존 메뉴/레이아웃/배경/장식/다중 줄 제목/StyleText/수명/수집/트리와 같은 저장소를 사용한다. `MenuGumpNode`에는 원본 **+cc 현재 선택**, **+21c 선택 소멸 통지 토큰**을 추가하고 `MenuGumpState.instances`로 원본 생성/소멸 개수를 보관한다.

생성 순서는 다음과 같다.

1. 기존 실제 배경 생성자에 크기 0/0, 배경 `0x03e00000`, 기본 flags `0x201`과 현재 타입/프레임을 전달한다.
2. 메뉴 표/+20 식별값·기본 문맥 font=0/현재 흰색/현재 그림자 색·shadow 비트/offset 1,0을 쓴다. 원본 문맥의 다른 슬롯은 보존한다.
3. 메뉴 개수를 증가하고 expanding/itemWidth/menuFlags/itemCount·장식/제목/선택 통지·현재 선택 -1·깊이 -60을 쓴다. heights 40개와 항목 40개 전체를 0으로 만든다. **offsets/목표 폭·높이는 이 단계에서 쓰지 않는다.**
4. 순서 있는 생성 입력을 처리하고 마지막 flags를 메뉴에 저장한다. 장식 모드이면 실제 장식/제목 자식을 만든다.
5. 기존 실제 Reflow를 한 번 호출하고 기본 notify bit `0x100`을 켠다.

원본 varargs는 `MenuGumpConstructionEntry`의 문자열/자식 토큰 variant와 flags/command/parameter/condition 값 목록으로 표현한다. 마지막 null 뒤 메뉴 flags는 별도 인자다. 원본 포인터/varargs ABI를 호스트 주소로 재해석하지 않는다.

- flags bit 1의 command와 bit `0x80`의 parameter는 **condition 검사 전에** 현재 항목 칸에 쓴다. bit `0x100`이 없으면 condition을 사용하지 않는다.
- 문자열 항목은 bit `0x10000`이 필요하고 child bit `0x20000`을 동시에 사용할 수 없다. 허용 조건이면 flags 저장→현재 문맥 복사→기존 SetText→현재 itemCount 증가 순서다. 생성자의 원본 한도는 **numItems < 40**이며 기존 Append의 `<=40`과 다르다.
- 자식 입력은 숨김→가상 Attach(0,0,parent 메뉴,mode 0)를 수행한다. 그 뒤 현재 항목 개수가 양수이면 **이전 항목**에 child bit `0x20`과 실제 자식 등록 ID를 연결한다. 선두 자식도 부착은 하되 연결할 이전 항목이 없을 수 있다.
- 생성 도중 항목마다 Reflow를 호출하지 않는다. 조건 false에서도 앞서 쓴 command/parameter를 복구하지 않는다.

## 장식·다중 줄 제목과 controlled 시작

장식 메뉴는 가로 bit `0x40000`을 거부하고 외부 new **0xc4**의 `GumpDecoration(1,1)`을 생성한다. 장식은 메뉴에 0,0으로 가상 부착하고 secondaryDepth=1을 쓴다. 이어 new **0xd8**의 실제 다중 줄 제목(크기 0/0,font 0,현재 흰색)을 만든다. 제목의 128칸 버퍼/Slider/overlay는 앞서 복원한 실제 생성자가 담당한다. 제목은 현재 장식에 **16,16**, secondaryDepth=-1로 부착하고 메뉴 배경 flags에 `0x40000`을 더한다.

`BindMenuTitleLayout`은 기존 제목 Reflow의 두 경계를 실제 **RequestScroll(0)**과 **FitContent(0,0)**에 연결한다. 기존 크기 처리기의 장식 Push/폭·높이 조정/제목 높이를 포함한 장식 위치 재부착을 그대로 사용한다.

| 시작 API | 원본 flags/후속 |
| --- | --- |
| BeginHorizontal | 스택 index 증가/빈 칸 확인→new 0xb90→0x140000 생성→실제 idle 등록 |
| BeginFancy | 같은 칸/할당→0xa80000 생성. 별도 idle 등록 없음 |
| BeginVertical | 같은 칸/할당→0x280000 생성→idle→현재 메뉴 배경 flags에 인자 OR |

예약 0번/20칸 한도와 이미 찬 칸을 검사한다. index 증가는 원본처럼 검사보다 먼저 수행한다. 할당/생성/idle 콜백 후의 현재 index를 다시 읽어 저장/반환한다. 실패 뒤 index를 자동 복구하지 않는다.

## 장식 배경 실제 출력·변경 영역

`GumpDecorationNode`는 기존 배경에 캐시 SHP/장식 타입/첫 모서리 프레임을 추가한다. 생성 시 원본 타입 **+150**, 즉 **I 글자 첫 프레임**을 실제 GameAssets에서 읽는다. 캐시 SHP와 배경 타입/프레임은 독립 필드다.

Draw는 실제 배경 출력 뒤 현재 Position을 한 번 저장하고 **왼쪽 위→오른쪽 위→오른쪽 아래→왼쪽 아래** 프레임 0/1/2/3을 원색으로 그린다. 오른쪽/아래 위치는 현재 배율 적용 width/height를 각각 CRT 절삭한 DWORD를 더한다. 각 모서리마다 현재 캐시 SHP/첫 프레임을 다시 읽는다. 모서리 그림 자체에는 배율/색 치환을 적용하지 않는다.

`GumpDecorationPixelTarget`은 같은 실제 SHP의 물리 프레임/상대 VFX 픽셀 좌표와 투명도를 기존 `GumpPixelTarget`에 공급한다. 현재 VFX 클립을 사용하고 해제 이미지만 캐시한다. 원본처럼 장식 전체 크기로 모서리 이미지를 확대하지 않는다.

Redraw는 erase를 사용하지 않으며 자신과 모든 부모의 visible만 확인한다. 실제 위치에서 **x-1/y-1**, 크기 **절삭(width+4)/절삭(height+4)**를 요청한다. 두 PE의 ROM 여백은 모두 float **4.0**이다. 크기가 양수가 아닌 요청은 원본 `00499370`/CD 대응처럼 생략한다. 호스트 invalidate 훅은 같은 half-open 변경 영역을 실제 화면 갱신 계층에 공급해야 한다.

## 화면 밖 부착 보정·소멸

메뉴 Attach는 parent=0이면 기존 부모를 사용하고 부모가 root이면 기본 bit `0x4000`을 켠다. 해당 비트는 후속 비 root 부착에서도 여기서 지우지 않는다. mode bit 1 또는 기본 bit `0x20000`이면 보정을 생략하고 기존 기본 Attach로 전달한다.

일반 메뉴는 부모 절대 위치+입력을 전체 화면 폭/높이에서 현재 메뉴의 배율 적용 크기를 뺀 상한으로 제한한다. 장식 메뉴는 장식의 로컬 위치와 전체 장식 크기를 포함해 먼저 정수 좌표/0 하한/화면 상한을 구하고 장식 위치를 다시 뺀다. 마지막 부모 위치 차이는 signed DWORD로 감긴 뒤 float32 로컬 좌표로 전달한다.

**10.78은 min 비교 요청값을 float32로 저장하고 CD는 저장하지 않는다.** 두 비교는 unordered에서 요청값을 선택한다. 장식 Y 첫 합도 패치는 부모Y+입력Y+장식Y, CD는 부모Y+장식Y+입력Y다. 기존 기본 Attach의 화면 제한/깊이/형제/현재 자식 후속 처리를 이어서 수행한다. 극단 x87 80비트 값의 독립 대조는 후속이다.

메뉴 scalar 소멸은 파생 표 복원→현재 메뉴 개수 감소→현재 선택 소멸 통지→**40개 전체 항목을 39부터 0까지 StyleText Clear**→기존 기본 scalar 순서다. 현재 itemCount/스타일 flags로 삭제 대상을 줄이지 않는다. 장식/제목 자식은 기존 GumpLifetime::Destroy가 먼저 삭제한다. 장식은 SHP를 소유하지 않으며 기본 scalar 경로만 사용한다.

`BindMenuGumpWidgets`와 `BindGumpDecoration`은 기존 가상 Attach/Draw/Redraw/scalar 분배를 보존하며 해당 소속 토큰만 실제 처리기로 전달한다. 훅을 복사하는 상위 처리기 생성 전에 연결하고, 늦게 생성되는 처리기는 resolver로 조회한다.

## 빌드와 남은 작업

라이브러리 `netstorm`과 실행 파일 `NetstormCpp`의 **Release 빌드 경고 0·오류 0**. SOURCE_MAP을 생성했으며 내용 차이는 없다. **새 테스트 추가/실행·전체 CTest·독립 원본/C++ 생성/조건/부착/소멸/모서리/변경 영역/픽셀 대조·게임/GUI 실행은 미수행**이다. 이번 실행 관찰과 비교 추가는 각각 **0개**다. 기존 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계 기록이다.

메뉴 실제 Draw/Handle/Broadcast/AttachObject는 위 후속에서 구현했다. 남은 작업은 제목/머리 포맷과 controlled 항목 래퍼, 전체 Form 객체·그림 색 표/회전 VFX/flag 2 재사용·OS 큐/폴링/overlay·raw Client UI 조립이다. 실제 전역/allocator/invalidate 공급·생성 EH 정리·포인터/varargs alias·극단 x87과 기존 전체 UI/픽셀의 독립 대조도 남았다.

보호 두 PE SHA-256은 불변이며 보호 원본/dotnetpj/테스트를 수정하지 않았다. AGENTS.md는 직접 편집하지 않았고 작업 중 들어온 외부 변경을 보존했다. 커밋/푸시는 수행하지 않았다.
