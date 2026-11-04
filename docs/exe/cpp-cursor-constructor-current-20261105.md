# UI50/51 cdecl의 실제 마나·사정거리 표시 분배

단위887 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전886은 `9a4a01ea / 20261105-20 CP`, 현재 제품887/11.01이며 전체 목표/Pop은 미완료다.

PATCH004741d0은 new224→004737f0,00474900은 new512→00474520으로 첫 부모SID/둘째Form을 전달한다. 몸체는 표시 자식을 생성하고 현재00541138/0054113c의 부착 TYPE·flags10으로 Host를 호출하며, 그 뒤 현재 시각을 초기화한다. 이전 단위의 미시작 경로/타이머와 달리 실제 부착을 생략할 수 없다. 기존 CreateCursorManaProcess/CreateCursorRangeProcess를 같은 Session/Host로 호출하고 Form0을 전달했다. wrapper의 선택 TYPE 번호와 생성자 몸체가 읽는 현재 표시 TYPE는 별도다. 두 RawCursor logic은 Impl 생성자1119/1134에서 prefix보다 앞서 준비되므로 ConnectCursor/배치/UI를 중복 초기화하지 않는다. 초기 확보 실패는 기존 공장이 반환하는nullptr를 원문처럼 무시하고 공통 부모로 복귀한다.

[새 ABI 도구](../../tools/decomp_cursor_constructor_abi.py)는 실제 두 wrapper×부모0/5/50/65535×Form0/65535×할당 정상/NULL의32입력에서 new 크기·전체DWORD 인자·ECX·cdecl 스택/SEH 복원을 관찰하고 재관측했다. operator new와 **UI Construct 몸체는 명시 외부 경계**이며 이32행을 새 UI 전체 몸체 실행으로 세지 않는다. 몸체 자체는 기존 마나642행·사정거리593행의 동결 Native fixture를 C++에서 재생해 대조했다. 새 fixture/기존 기대값 변경/기존 전체 PE 몸체 재실행은0이다. CD 대응 wrapper 주소0048b950/0048ba20은 별도 정적 인계에 있으며 새 CD 실행/이식으로 합산하지 않는다.

새 실제 Session1사례는 TYPE61 하나의 함수 주소를 두 wrapper로 바꾸고 마나 현재 processType58/사정거리57로 부착하는지 확인한다. 입력 TYPE74는 group0/theme0/foot1/flags0과 Teleport VT를 명시한다. 실제 각 클래스/현재 Form/부모·VT·8개 marker103/104/숨김 비트·할당9개·Kernel+1·공통 TYPE74 장부/깊이를 확인했다. marker/프로세스 Frame과 게임/AI/전투 Kernel은 진행하지 않았다. 같은 소유권의 기존 소멸이 표시 자식을 정리하며 별도 UI/마우스/사제 행동을 추가하지 않는다.

고유30개/10묶음(새1) 첫 실패0이다. 기존 타이머/경로/기본Form/EvilRay/Teleport 실제 연결도 통과했다. Release1회 135.6356283초·종료0/경고·오류0, 최종4파일은 빌드 중 불변이며 추가 교정/기존 기대값 변경0이다. 보호 **2767개/947fixture 전부 불변**·새fixture0이다. EXE는10660864BYTE, SHA-256 `2e9d8f21077d1fe400220aed62fb3787c23f7b1a16310589a84361b366f672b7`다. [검증 JSON](../../cpppj/cursor-constructor-current-20261105.json)과 Git 제외 `extracted/cpp-resume-887/`에 ABI32행·정적 주소 커버리지·빌드/검사·SHA를 보존했다.

PATCH 최초 TYPE0~69의 비영21행(기본Form4+process17, 중복 포함)·고유20함수 주소의 분배가 정적으로 모두 연결됐다. 주소 존재 확인을 모든 입력/실행 몸체·현재 TYPE writer/장기 owner·전체Pop 완료로 확대하지 않는다. 새 부착 번호는 기존 Session 표시 상태의 현재 값을 읽으며 Native 전역 owner 전체 통합은 별도 잔여다. checking10(부모4/부분3/MP3)·완료부모38/부분3은 유지한다.

다음888은 자산 TYPE70 이상의 비영101행/고유84주소를 대조한다. 기존 PATCH recipe82개와 mog/lightning의 공통 NULL004429e0·daisExtraFrames169의 진단 뒤 물리VT0044a420 두 특수가 있다. CD recipe는71개다. 공중3종(현재 raw ctor0045fbb0/0045fbd0/0045fbf0)과 outpost00454de0의 새 전용 실행/검사는 제외하고 코드/정적만 한다. 원시 메모는 `next-asset-constructor-notes.json` 및 next-*.c.txt/entry-window.asm에 있다. 게임/GUI·사용자 직접 확인·포획/희생/공격/승리·공중3종 전용·신규MP/패킷/통신·전체71/CTest/34미션/ZIP 반복0이며 목표는 active다.
