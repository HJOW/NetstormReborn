# 메뉴 출력·선택·하위 메뉴 입력·객체 부착

후속 [controlled 메뉴/제목/머리/CD 문맥](cpp-menu-gump-controls.md)에서 제목/머리 포맷·항목 래퍼·CD 자식 문맥 전달을 추가했다. 이 문서와 근거 해시는 출력/입력 구현 당시 상태를 나타낸다.

2026-10-13, **마지막 디컴파일 PC: HJOW-Athlon**. 현재 호스트/IP는 **HJOW-Athlon / 192.168.0.94**다. AGENTS.md와 두 인수인계를 읽고 같은 PC의 정밀 프로젝트에서 두 PE를 읽기 전용으로 분석했다.

[메뉴 생성/장식/수명/부착](cpp-menu-gump-widgets.md)에 실제 메뉴 Draw/Handle/Broadcast/AttachObject를 추가했다. **Client/UberGump는 기존 메뉴 어댑터를 사용하며 자동 raw UI·건설·미션 완주는 미완료다.** 이번 단계는 정적 분석과 빌드로 진행했다.

## 원본 근거

계획은 `tools/ghidra/menuinteraction-functions.json`, 내보내기는 `extracted/menuinteraction/{originals,originalCD}/creation.c`, 범위는 `functions.tsv`, 기계어는 `assembly.txt`다. 패치 **12개**, CD **13개** 함수를 내보냈다. CD 자식 문맥 전달 함수 00452a00은 참고 자료이며 이번 구현에 포함하지 않았다.

`tools/analyze_menuinteraction.py`는 두 보호 PE의 실제 슬롯/식별 BYTE/전역 주소/상수·Form stride·현재 소스/빌드 로그 해시를 `cpppj/recovery-menuinteraction-evidence.json`에 기록한다. 실행 관찰/독립 C++ 비교 수는 증가시키지 않는다.

| 기능 | 10.78 | CD 10.72 |
| --- | --- | --- |
| Draw | 00476080 | 004523b0 |
| Handle | 00476820 | 00452b50 |
| Broadcast | 004772d0 | 00452f00 |
| 하위 메뉴 닫기 | 00476670 | 00452920 |
| 자식 메뉴의 커서 가림 | 00476b00 | 00452e40 |
| 선택 행 변경 영역 | 00476e00 | 004532c0 |
| AttachObject | 00476dd0 | 00453290 |
| 기본 객체 연결 helper | 004644e0 | 00492f80 |
| 로컬 rect→화면 rect | 00464930 | 004936c0 |
| 루트 팝업 Kill | 00466710 | 00492c20 |
| 항목 측정 | 00475d70 | 00451430 |
| 항목 출력 | Draw에 인라인 | 00451490 |

앞 단계 생성자의 오류를 수정했다. 원본 메뉴/장식 식별값 **26/57**은 **+20 command**에 저장된다. 복원 코드가 이를 **+8 alignFlags**에 쓰던 부분을 두 생성자에서 고쳤다. 정렬 필드를 이 식별값으로 덮어쓰면 자식 재배치와 메뉴 부모/자식 판정이 달라진다. `alignment`라는 기존 상태 멤버 이름은 호스트 API 호환을 위해 유지하고 주석에 실제 필드를 명시했다.

## 선택과 입력

`MenuGumpInput`은 같은 MenuGump/기본 객체, 등록표, GumpCollection/GumpEvents/GumpLifetime/GumpTree를 사용한다.

- Broadcast는 실제 ScreenToLocal을 호출한 뒤 자체 경계와 메뉴 식별 자식들의 가림을 검사한다. 메뉴 아닌 자식은 하위 탐색도 생략하고, 숨긴 메뉴의 표시 중 하위 메뉴는 계속 검사한다.
- 패치의 ordered 0 하한과 CD의 unsigned float 비트 `<=0x80000000` 하한을 각각 표현한다. 둘 다 -0을 허용하고 strict 폭/높이 상한에서 NaN을 제외한다.
- 가로 선택은 localX/itemWidth를 CRT 방식으로 절삭한다. 세로는 offset/height를 찾고 일치하지 않으면 마지막 행을 선택하며 compact는 0번이다. 비활성 항목은 선택하지 않는다.
- old/new 강조 비트 변경과 행 요청 순서를 유지한다. 세로는 해당 행만, compact는 전체 크기, 가로는 가상 Redraw(false)를 요청한다. 부모의 visible만 확인하고 크기/offset에 배율을 다시 곱하지 않는다.
- Handle은 확장 중·release·좌우 누름 외 사건을 제외한다. 실제 가상 Broadcast 후 현재 선택을 읽고 명령 큐에 command/등록 ID/인자를 보낸다. 명시 인자 비트가 없으면 flags 전체 DWORD가 인자다.
- 기존 자식을 재귀 닫고 Redraw 후 숨긴다. 동적 자식은 숨김 여부와 무관하게 Kill하고 childId/0x20을 지운다. 강조 초기화는 선택 인덱스를 -1로 바꾸지 않는다.
- 동적 자식은 현재 요청 사건/serial/인자로 가상 Handle을 호출한다. 반환 자식의 등록 ID를 저장하고 숨김/Attach 후 현재 선택 항목에 0x20을 세운다.
- 자식 열기는 Push→visible→부모 항목 위치에서 가상 Attach→자식 우선 깊이 -1→실제 StartExpand→openGump.wav→부모 Redraw다. 명령 음향은 openSubGump.wav다.
- 메뉴 부모 경계의 가지 닫기, 명령 뒤 자신의 Kill/부모 Bubble, 루트 sticky 팝업 Kill을 원본 순서로 연결했다. 루트 팝업의 next는 Kill 전에 저장한다.

가상 입력/통지/부착/재그리기, 명령 큐, 실제 사건 전역과 변경 영역 공급은 명시 경계다. 필요한 경계가 없으면 도달한 시점에 예외로 거부한다. `BindMenuGumpInput`은 기존 이벤트/캡처/프레임 통지 분배를 감싸고, `BindMenuGumpInputSound`는 실제 SoundPlayer에 원본 인자 모두 0으로 연결한다. 동적 하위 메뉴를 공급하는 상위 파생 Handle은 요청 사건을 별도로 처리해야 한다.

## 실제 출력

`MenuGumpDraw`는 같은 GumpBackgroundDraw/GumpTextDraw/StyleText/MenuGumpLayout을 직접 호출한다.

1. 자신의 Bounds로 임시 클립을 만든 뒤 기본 배경과 Position을 조회한다.
2. 항목마다 현재 문맥을 복사하고 font=0/현재 흰색, 굵게/비활성 plain 문맥을 적용한다.
3. 선택 항목은 구분선을 제외하고 현재 메뉴 type/frame의 `0x2100000` 배경으로 강조한다. 가로/세로/compact의 안쪽 2픽셀 영역을 사용한다.
4. 세로 구분선은 행 중간의 두 1픽셀 배경이다. 첫 flags `0x2100000`, 둘째 `0x2080000`이며 각 pass마다 현재 전역 배경 type/frame과 임시 클립을 사용한다.
5. 체크 그림은 현재 장식 타입의 **+154 = I 구간 count**에 2를 더한 물리 프레임이다. +150은 I.first이며 +154를 J.first로 해석하지 않는다. 기존 GumpDecorationPixelTarget의 실제 SHP 해제 캐시/원색/투명도/클립을 공유한다.
6. plain은 항목 60바이트/NUL과 임시 문맥으로 출력한다. styled는 조각의 저장 문맥을 사용하며 override는 null이다. 하위 메뉴/동적 항목은 실제 측정 폭 뒤에 같은 글꼴의 ` >`를 출력한다.
7. 각 내부 클립과 전체 클립은 정상/예외 반환에서 복구한다. 원본처럼 글자 색 표는 별도로 복구하지 않는다.

목표 크기와 현재 크기 차이를 글자/체크/구분선 위치에 반영한다. 로컬 rect 인자는 float32로 저장한다. 화면 rect helper는 X/top Y의 정수 확장값을 유지하고 **패치만 bottom Y를 float32로 저장해 재사용**한다. CD는 bottom에서도 원래 정수를 유지한다. 극단 x87 80비트의 독립 대조는 남았다.

## 객체 부착과 연결 순서

`MenuGumpWidgets::AttachObject`는 기본 객체 helper와 현재 가상 Attach를 복원한다. object=0은 이전 object/Form/flags를 지우지 않는다.

비영 object는 실제 Form(type,2) 생성 경계→Form +12에 현재 Gump 등록 ID→가상 +74(object,0)→기본 flags 0x40/object/Form 풀 인덱스→가상 Attach 순서다. 패치는 flags 뒤 object, CD는 object 뒤 flags를 쓴다. 0x40은 이 helper의 연결 비트이며 화면 투영 비트 0x2000을 추가하지 않는다. Form 풀 stride는 패치 **0x32**, CD **0x24**이며 호스트 공급기가 실제 토큰/인덱스를 제공한다. **전체 Form 풀/소유/객체 연결 구현은 후속이다.**

`BindMenuGumpDraw`, `BindMenuGumpInput`, `BindMenuGumpObjectAttach`는 현재 메뉴 토큰만 분배하고 다른 파생 처리기를 보존한다. 처리기가 훅을 복사하기 전에 연결하고 늦게 만드는 처리기는 resolver로 조회한다. 모든 참조/저장소/표면/음향은 연결한 처리기보다 오래 살아야 한다.

## 빌드와 다음 작업

`netstorm`/`NetstormCpp` **Release 최종 빌드 경고 0·오류 0**. SOURCE_MAP 생성기로 존재 상태를 갱신했다. **새 테스트 추가/실행·전체 CTest·독립 원본/C++ 입력/콜백/픽셀 대조·게임/GUI 실행은 미수행**이다. 앞 단계 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 이번 추가 코드의 검증 결과가 아니다.

메뉴 제목/머리·controlled 항목·CD 자식 문맥은 위 후속에서 구현했다. 남은 작업은 전체 Form 풀/연결과 실제 전역/명령 큐/변경 영역 공급, 자동 raw Client UI/OS 큐/폴링/overlay다. 신규 UI와 기존 UI의 독립 동작/픽셀 대조·회전 VFX/그림 색 표/flag 2 재사용·건설/미션 완주도 남아 있다.

두 보호 PE SHA-256은 불변이다. AGENTS.md/보호 원본/dotnetpj/테스트를 편집하지 않았고 커밋/푸시를 수행하지 않았다.
