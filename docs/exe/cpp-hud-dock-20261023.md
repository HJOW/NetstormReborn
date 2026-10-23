# 10.78 HUD divider와 지도 영역 본체 복원 — 2026-10-23

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 보호 입력은 `originals/Netstorm.exe` SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이며 기존 정밀 원문 `extracted/refined/originals/Netstorm.c`를 재사용했다. 보호 파일과 디컴파일 산출물은 변경하지 않았다.

## 이번 단위의 범위

`HudDock`은 원본 생성·슬레이브 등록·재배치·HUD 사건·지도 영역 계산 본체이며 `HudDividerInput`은 같은 파생 노드와 공통 Gump의 입력·프레임 드래그 본체다. 두 모듈을 제품 라이브러리에 등록하고 독립 PE 관측으로 직접 대조한다. 실제 Client의 고정80×75 미니맵/생산 패널 어댑터를 교체하는 작업은 다음 단위다. 이 결과를 실제 HUD 드래그·픽셀·게임 완주·원본 동일성 완료로 확대하지 않는다.

전체 HUD의 실제 조립에는 ChatWindow와 메타맵 Player List/Battle Status용 NSInfo의 자식 생성도 필요하다. ChatWindow는 배경, 숨은 Console/TextEntry, ScrollingText와 helper Button을 함께 생성한다. NSInfo는 일반 F6 지식 팝업과 다른 메타맵 모드4의 Player List/Battle Status 패널이다. Attach의 metaMode/playerKnown 관문은 일반 캠페인에서 자식 목록 꾸미기를 건너뛴다. 생성자가 항상 만드는 자식과 이 관문 안의 동작을 별도로 조사한다. 임의의 빈 노드로 생성 성공을 대체하지 않고 해당 의존을 별도 복원한다. 이 때문에 GameWorld의 실제 viewport 연결 준비 수정은 `extracted/cpp-hud-viewport-20261023-pending.patch`에 보관하고 이번 빌드/커밋에서는 제외했다.

## 원문과 공통 처리기

| 주소 | 복원한 역할 |
|---|---|
| 004c6c50 | 배경 생성 뒤 divider 가상 표·종류·축·10개 serial/관계 슬롯 초기화 |
| 004c4790 | 첫 빈 serial 슬롯에 등록, 중복 허용, 가득 찬 진단 뒤에도 자식40000h 표시 |
| 004c47f0 | 팝업 정리→기본 Attach→현재 부모 통지→관계1/2/4/8 순서 재배치 |
| 004c6530 | HUD 모드1/3/4, 채팅 표시/높이, divider serial 사건의 정수/float 지도 영역 |
| 004c49f0 | 선행 ScreenToLocal, 좌클릭 anchor·capture/release, hover 커서, 기본 Help |
| 004c4b00 | 현재 MenuGump 인스턴스0일 때 폴링·release 비트 정리·축/제한·Move |
| 004c7160 | 정상 HUD의 슬레이브10개 관계와 세 divider 제한의 근거 |
| 00464d10 | 기존 공통 Move: anchor 차감 뒤 가상 Push/Attach |
| 004650a0 / 00464cf0 / 00465250 | 기존 공통 로컬 변환 / anchor / 첫 capture |
| 00465c60 | divider 가상+24의 기존 배경 Draw. 새 전용 Draw가 아니다 |

실제 divider 가상 표는00514fd8이다. 드래그는 flags100h가 선택하는 공통 프레임 Broadcast이며 별도 Idle 등록이 아니다. 메뉴 관문00593024는 **MenuGumpState.instances**다. Pause나 Help 모달 수로 대신하지 않는다. release를 폴링한 프레임도 마지막 Move를 실행한다. 축2가 축1보다 먼저이며 제한은 signed min→max 순서로 적용한다.

슬레이브 재배치는 현재 serial과 관계 플래그를 매번 다시 읽는다. 한 자식에 여러 관계가 있으면 각 분기 사이 callback이 변경한 값도 다음 분기에 반영한다. 원문 `FST field→FMUL scale`는 저장한 float32를 다시 읽는 계산과 구별했다. 실제 일반 함수/CRT가 수행하는 중첩 Move·divider 재배치·HUD 지도 영역 갱신을 관측에 포함한다.

정상 기본 배치는 세로 구분선 x80/폭5, 하단 구분선 yH−80/높이5, 채팅 구분선 yH−96/높이5다. 지도 영역은 left85/top0/rightW/bottomH−96이며 하단 미니맵 경계와 다르다. 단순히 채팅 자식을 숨기는 것과 채팅 구분선을 H로 이동하는 것을 구별한다. 정상 제한은 세로 X60..130, 미니맵 YH−123..H, 채팅 Y25..H−25다.

## 독립 관측과 제품 검사

기하 도구 `tools/decomp_hud_dock_geometry.py`는 실제 PE의 생성/등록/재배치/HUD와 중첩 Move 및 실제 signed64 `_ftol` 명령을 실행한다. **1,075입력×x87 두 제어어 = 2,150행**이며 정상2,144행과 명시 callback 중단6행이다. 실제 명령683,920개 중 CRT65,524개이며 두 정밀도 차이는0이다. 노드8개의 전체0x11cBYTE와22DWORD 전역, callback 순서·인자·부분 쓰기를 기록한다. 앞6열만 실행 입력이고 뒤8열은 CPU 관측이다.

기하의 명시 경계는 배경 생성, 활성 등록, 팝업 정리, 공통 기본 Attach, Push, 일반 자식 Attach, serial 조회, 부모 통지 전달, dirty 요청, 화면 후처리, 기본 Help, 채팅 설정, 진단이다. HUD 안의 divider Attach/Move/다음 HUD 진입은 실제 원문으로 중첩 실행한다. 공통 트리 자체의 전체 링크/할당/Draw까지 이 관측으로 보증하지 않는다.

입력 도구 `tools/decomp_hud_divider_input.py`는 실제004c49f0/004c4b00과 공통 하위의 진입부터 RET까지 실행한다. **6,699입력×두 제어어 = 13,398행**이며 실제987,266명령, 정밀도 차이0, 정상RET/stackdelta0이다. Ctrl/Shift/Alt8조합·outside/pressedSince, 부모0/1/2단계, capture없음/자신/다른 대상, 정상·역전·극값 signed 제한을 포함한다.

입력의 명시 경계는 OS 폴링, 기본 Help, 커서, 최종 가상 Push/Attach다. 제품 검사는 실제 공통 Collection/Input/Tree를 재사용해 전체13,398행의 flags/anchor/focus·공개 효과·최종 부착 인자를 대조한다. 내부 local/anchor/capture/move 진입 표식은 공개 효과 trace에서 제외하고 실제 상태·최종 가상 인자로 대조한다. 공통 Broadcast의 visible/notify/비dead 관문까지 통과할 입력은2,186행이다. 전체13,398행을 실제 Route/GUI 통과로 세지 않는다.

## 검증과 남은 연결

Release 빌드1은 검사 로컬 `Split`과 공통 지원 헤더의 함수 이름 충돌2건으로 실패했다. 검사 함수를 `DockSplit`로 구별한 빌드2는 **종료0/경고0/오류0**이다. 이후 제품/검사 C++는 동결했다. 집중 검사2개는 각각 전체2,150행/13,398행을 소비했고 실패check0이다.

전체 CTest1은 **2,375검사·실패check0·종료0**, 단일 검사 컨테이너242.50초(전체242.57초)다. 내부 `LastTest.log`의 두 새 검사와 최종2375/0도 확인했다. GUI/game 실행은 이번 단위에서 하지 않았다. 아직 Client 연결을 바꾸지 않은 본체 복원이므로 실제 HUD 사용성 검증과 구별한다.

| 산출물 | SHA256 |
|---|---|
| `hud-dock-geometry-x86.tsv` | `a8bd529f1df42321d035882dcdc59efb1016b43b2ad08ea108a0fe879250830e` |
| `decomp_hud_dock_geometry.py` | `304139576bd0acc3332c3444630a12ca4f31bd28835da854d9018a07da8760b9` |
| `hud-divider-input-x86.tsv` | `d6551d404c129e9e389519ffc4cb11061f1c4769929a5f4dd0fc65f1100d6183` |
| `decomp_hud_divider_input.py` | `18cac565e6b6c042de194b65f688ef2a253f6e59e7c2b4c4f986fc606a14b4ff` |
| `NetstormCpp.exe` | `920fc93c1192869deb5a21b1c8b3f5f8b1086b69eb855cc6b59871faace26874` |
| `netstorm_tests.exe` | `cb2a7584fe1e9276c977b45436998ba4312193f5114f405606df06cc1f450926` |

로그/독립 범위 JSON/검사 보고는 `extracted/cpp-hud-core-20261023-*`, `extracted/cpp-hud-dock-geometry-20261023-native.json`, `extracted/cpp-hud-divider-input-20261023-native.json`이다. 두 도구 AST와 새 파일의 엄격한 UTF-8·주석 문자, 실제 보호PE/도구/fixture SHA, diff/보호 경로를 검사했다. 새 전체 디컴파일을 수행하거나 보호 원본을 변경하지 않았다.

다음 연결 작업은 아래와 같다.

- 실제 ChatWindow/NSInfo의 생성·수명·배경·텍스트·입력을 같은 공유 Gump로 연결한다.
- RawButtonControls에서 노드/가상 Attach/Handle/Broadcast/소멸/외부 Draw를 조립하고 현재 MenuGump 인스턴스를 읽는다.
- 생산·미니맵의 고정 예약 Resize를 실제 divider 관계로 교체한다. 실제 지도 영역을 GameWorld의 표시·커서·입자·위치 소리·미니맵에 전달한다.
- 실제 Route/프레임 드래그·최소/최대 제한·release·메뉴 관문·미션 이탈/재진입·두 화면비의 Draw를 검증한다.
- 미니맵 폭130처럼 bitmap128보다 큰 합법적 dock 크기의 원문 블릿 경계를 추가 분석한다. 현재 제품의 crop 범위 예외를 숨기거나 임의 크기 제한으로 대체하지 않는다.
- ESC 수평 메뉴/하위 메뉴 원문은 별도 새 모듈로 병렬 분석 중이며 이번 단위의 빌드·커밋에는 포함하지 않는다.

체크아웃/관측 SHA 재현성을 위해 새 본체·검사·도구·상세문서는 `.gitattributes`에서 LF로 고정했다. 기하 도구의 출력도 명시 LF로 고정해 같은2150행을 재생성하고 집중2검사를 다시 통과했다. 실제 관측 값과 제품/검사 C++ 및 실행 파일은 변경하지 않았다.
