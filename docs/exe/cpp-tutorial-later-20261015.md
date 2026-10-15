# Early Missions3~6 단계 판정 복원

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 같다. 원본10.78의 `004c3f00`/`004c40f0`/`004c42b0`/`004c45e0`을 다시 읽고 실제 기계어로 대조했다. 원본 게임/GUI를 실행하거나 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

`RawTutorialLater`는 기존 `RawMissionFrameGate`와 같은 stage/timer/정지 소유권, `RawMissionStart`가 초기화한 같은+c0/+c4 경고 표식, 같은 `RawTotalMade`와 raw Pool/TYPE/Player9를 빌린다. 사건은 실제3DWORD를 받고 명령 비교에만 `0x03ffffff` 마스크를 적용한다. SID/기술 타입 인자는 modifier를 제거하지 않는다.

- Tutorial3 Capture the Priest: 사거리 사건, 적 사제→내 사제→내 Graph→적 Graph 순서, 현재 생산 목록·기술 알림, 실제 배치 객체의 지역/주변 검색, 적 사제의 부재·포획, 현재 내용물 부류/수량을 판정한다. 지역 조회는 실제 stdcall8 규약이며 타입 검사보다 먼저 실행한다.
- Tutorial4 Tactical Combat: 다이스 허용표와 옵션BYTE2, 현재 전역/첫 로컬 제작수, 내용물 사건의 원본 두SID와 타입, 실제 포획/상승 사건을 판정한다.
- Tutorial5 Subtle Defense: switch 전에 VortexDestroyed/FactoryDestroyed를 각각 현재 상태로 검사한다. Tell 성공이면 실제 임시창 정리→게이트의 PauseForDialog를 실행하고 성공 여부와 무관하게 일회성 DWORD를1로 쓴다. 선택은 첫 조회가 비제로일 때 다시 읽는다. 예약은 원본 ROM의 **5.0초 double**이며 현재 게임 시각을 float로 축소하지 않는다.
- Tutorial6 Raw Power: 현재 전역 제작수, 사거리 사건, 기술 알림/생산 및 적 자산 제거 조건을 판정한다.

전역/로컬 수 비교는 원본 `sub`→`test`의 감긴 DWORD 결과 부호를 사용한다. 수학적 정수 비교로 바꾸면 `0x80000000`/`0x7fffffff` 경계에서 결과가 달라진다. 같은 타입 전역도 두 getter 앞에서 각각 다시 읽으며 실제 메모리 읽기 순서를 독립 fixture에 보존했다. Advance 거부 뒤의 허용표/옵션/선택 효과도 원본대로 유지한다.

[`decomp_tutorial_later.py`](../../tools/decomp_tutorial_later.py)는 현재 전체 정밀 함수표에서 검토한 범위만 실행한다. 네 판정기, 공통 Advance/Pause, TotalMade의 Global/Local/Allow, Genus와 TYPE getter는 실제 PE 바이트다. TYPE188/선택 raw 객체/제작 장부/Player9는 명시 입력이며 실제 생성자/게임 플레이를 실행한 것으로 세지 않는다. 생산·기술 알림·사제·Graph·지역·주변·포획·선택·Tell/팝업·시계 정지 깊이는 명시 외부 경계다. 알 수 없는 함수·쓰기·OS 진입은 거부한다.

새 **533고유입력/533정상 반환**을 보존했다. 모든 stage 기본분기, 현재 생산/사제/Graph, signed DWORD 차이, modifier, 예약 전/동일/후, 잠금/정지와 실제 효과의 입력 변경을 포함한다. 실제 판정기 진입은3번134/4번132/5번135/6번132회다. 반복 하위 함수 진입을 별도 고유입력으로 합산하지 않는다. cdecl/thiscall 스택과 보존 레지스터4개, x87 제어어/스택을 확인했다. 원본 공통 게이트의14필드, 옵션8BYTE, 다이스 허용DWORD, 현재 선택/owner, 일회성 경고 및 모든 현재 TYPE 읽기/외부 효과의 순서를 비교한다.

[`TutorialLaterTests.cpp`](../../cpppj/tests/TutorialLaterTests.cpp)는 독립533행 전수 재생과 필수 경계/3DWORD 계약의2개 검사다. 첫 C++ 대조에서 모두 통과했고 검사 마스크 시프트의 C4334 경고는 의미를 유지하는 명시64비트 입력 마스크로 교정했다. 최종 Release 경고0·오류0, 이2개를 포함한 관련 집중 **60개 실패0·전체 CTest1/1·내부1,283개 실패0·216.33초**다. 로그는 `extracted/goal-mission-entry-final-{build,focused}.log`, `goal-mission-entry-{ctest,tests}.log`다. 현재 근거/소스 SHA는 [`tutorial-later.json`](../../cpppj/tutorial-later.json)에 보존한다.

이 판정기는 실제 생산 Deck/기술 알림/Graph/지역/포획/UI의 현재 소비자를 연결해야 한다. 현재 UberGump의 자동 분배·일반 Construct·생산/수확/의식/자연 승패와 저장은 후속이며 이 단위의 독립 대조를 Early Missions 자연 완주 근거로 사용하지 않는다. CD10.72의 이 네 전체 판정기를 검증한 단위도 아니다.
