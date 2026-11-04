# 공통 Pop의 현재 세계 크기와 ordered 좌표 비교

단위889 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치·산출물 변경0이다. 직전888은 `0d163f6e / 20261105-22 CP`, 현재 제품889/11.01이며 전체 목표/Pop은 미완료다.

공통 Pop004b02d0의 좌표 입구와 Point.isValid0040e800은 x>0을 ordered 비교한 뒤에만 현재 signedDWORD00531928을 읽는다. 각각004b0321/0040e810의 fild 뒤004b0327/0040e816에서 **float 한 번 저장**하고 x<limit/y>0/y<limit에 같은 값을 공유한다. x가 NaN/0/음수면 크기를 읽지 않으며 NaN/무한값의 실패도 원문 비교 결과다. Pop은 실패한 두 좌표를10/10으로 보정한다. 기존 C++ 부모는256 고정값과 선행 비유한 throw를 사용했다. 새 IsCurrentPopPointValid에 원문 순서를 모으고 입구 및 미니맵 뒤 시작점 검사가 각각 현재 값을 읽도록 연결했다. signed16777217이 float16777216으로 저장되는 경계도 보존한다.

실제 Session은 카메라·커서/마나·AI가 빌리는 **같은 ClientGameplayGlobals.WorldCells** getter를 Pop 진단 상태에 연결한다. callback이 장기 owner를 소유해 지역 주소/Session this를 남기지 않는다. 선택한 getter 사본은 자기 연결 해제 중에도 살아 있다. 독립 미연결 입력의 기존256만 유지한다. 논리 경계와 실제256×256 spot/hash 저장소는 구분하며 물리 범위를 넘어서는 메모리는 성공으로 보정하지 않는다. 건물군/픽셀 등 다른 전역 전체 연결 완료를 주장하지 않는다.

[격리 도구](../../tools/decomp_pop_world_bounds.py)는13 signed 크기×20좌표=260입력에서 **40e800 전체**와 **Pop의 SetPosition 호출 직전까지**를 각각 실제 PE로 실행한다. CW027f/037f에서4관측/입력,1040관측이며 다시1040을 실행해 같은 [260행 fixture](../../cpppj/tests/fixtures/pop-world-bounds-x86.tsv)를 얻었다. 크기 조회0/1회·함수 반환과 원문이 선택한 SetPosition 인자의 f32 비트를 보존했다. 로그는 메타맵 입력으로 억제했으며 SetPosition/Pop 나머지/게임/장치는 실행하지 않았다. 이를 전체 Pop Native 실행으로 세지 않는다.

새 순수2사례는260행 반환/비트/조회 횟수와 getter 자기 해제/중단/NaN 단락을 대조한다. 새 실제 부모1사례는 현재32의40/20→10/10, NaN x의 getter 생략, 미니맵 뒤64→16 변경에 따른 시작점6b7 진단과 정상 복귀/부분 spot·활성화를 확인한다. 새 실제 Session1사례는 Load 후 같은 WorldCells32/64를 바꿔 실제 일반 Pop·커서 참조·좌표/장부/할당·Kernel 불변과 유효한 유한 좌표 로그를 확인한다. 게임/AI/전투 Kernel은 진행하지 않는다. 기존 부모14개와 UI 실제 생성도 첫 통과했다.

추가로 선택한 옛 SHP 회귀1개가 실패했다. 관측은 진단1/level3/frame0/저장 bucket 일치/깊이0으로 모두 같고 비용만delta0, 현재float21, 최초로딩0이었다. 단위865의 원문004b0e72~004b0e97은 별도0059ab50 캐시를 읽으므로 옛 "현재TYPE 비용21 누적" 기대가 낡았다. 관측 출력으로 분리 확인한 뒤 이름을 InputFrameAndLoadedCost로 바꾸고 최초 Banner 비용0/현재float21을 구분했다. 비영 썬 캐논400을 확인하는 기존 실제 Session 회귀도 추가 통과하여 비용을 무조건0으로 바꾸는 잘못된 수정을 막았다. 제품/기존 fixture 수정은0이다.

고유 **21논리사례(새4)·8필터/23실행 최종 실패0**이다. 이름 변경 전후 SHP를 한 사례로 집계하고 최초19개 통과를 유지했으며 실패 관측/수정과 비영 비용2개만 추가 실행했다. 제품 첫 Release435.5978637초·관측 검사33.2878957초·기대 교정 검사33.5409148초 모두 종료0/경고·오류0이다. 제품은 첫 빌드 뒤 불변·검사 파일1개만 추가 교정했다. 최종10파일 SHA를 보존했다.

기존 보호2768개/948fixture 불변·새1개 후 **2769개/949개**다. EXE는10664448BYTE, SHA-256 `81f84b67df6f49a78cd0a821ece8a60b2ab59eb0d475f463a09a08c2fc463998`다. [검증 JSON](../../cpppj/pop-world-bounds-current-20261105.json)과 Git 제외 `extracted/cpp-resume-889/`에 기계어·Native/최초 실패 관측·빌드/검사·보호SHA를 보존했다.

정적 감사에서 기존 자산 recipe/base83 VT의 +30은 모두004ad470이다. 공통+20이004b0d30이지만 일반 Supports 밖인 두 VT0050c9f0(TYPE108 playerIsland/109 battleIsland),0050cc08(TYPE161 challengeIsland)는 MP 메타맵 잔여로 기록한다. 전체 파생+20/명시 Validate 경계·현재 TYPE writer·군도·Factory/State/파일/UI/Edit/Windows/무검사MP/4~8차는 계속 남는다. checking10(부모4/부분3/MP3)·완료부모38/부분3은 유지한다. 다음890에서 이83 VT/43개가량 실제 +20 family와 현재 분배/명시 Validate caller를 감사해 코드 공백을 이어간다. 실제 게임/GUI·포획/희생/공격/승리/사용자 확인·공중3종 전용/MP·전체71/CTest/34미션/ZIP 반복0이며 목표는 active다.
