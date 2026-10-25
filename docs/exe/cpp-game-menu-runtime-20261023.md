# 실제 ESC 가로 메뉴와 사건 연결 — 2026-10-23

기준은 보호10.78 PE와 VM-W11-Codex에서 2026-10-23 마지막 전체 정밀 디컴파일한 `extracted/refined/originals/Netstorm.c`다. 새 전체 디컴파일이나 기존 보호 원문 변경은 하지 않았다.

## 2026-10-25 후속: 임시 Game 돌 버튼 제거 완료

사용자 요청으로 평상시 왼쪽 위 임시 Game 버튼의 생성·클릭 영역을 제거했다. 이는 초기 호스트 UI(`00c04b75`)가 실제 생산 패널 옆으로 이동(`3447c500`)한 뒤 남은 것으로, 원본 ESC 가로 메뉴(`5adcf532`)의 Game 열과 다르다. 아래10/23 당시 남은 범위 중 이 임시 버튼 항목은 이번에 완료했다.

현재PC/마지막 전체 정밀PC는 HJOW-Athlon/2026-10-23이다. 첫 증분 빌드 경고·오류0, 기존 집중14개 통과와 같은 제품의1024×768/1280×720 GUI를 확인했다. 평상시 클릭영역 부재와 ESC 토글·원본Game/Options하위가 통과했고 세 실제 캡처를 육안확인했다. 원본/허용설정 복원·보호1821/1소스/4실행 파일 해시도 같다. [실행 보고서](../../cpppj/game-button-removal-runtime-20261025.json)가 실제 증거와 범위를 기록한다. 전체CTest·긴플레이·아래다른미완료항목을이번완료로확대하지않는다.

## 제품 연결

`RawGameMenuBar`와 `RawGameMenus`는 같은 공통 Gump 등록표/controlled 스택에 실제 가로 부모와 Game·Edit·View·Options·Players·About 하위를 생성한다. root 위치는 현재 월드 viewport.left와 화면폭을 사용한다. 메뉴 serial은 +2c를 사용하고 등록 id와 혼동하지 않는다. SP 준비는 실제 Rotate, 검산은 같은 사제 재생 sequence 참조다. 호스트의 선행 Close/Cleanup을 생략한 채 생성한다. 실제 루트 Attach의 replace 비트가 이전 팝업을 지연 Kill하므로, 열기 뒤 이전 팝업이 계속 보인다고 해석하지 않는다. ESC 토글은 현재 serial 검색과 지연 Kill을 사용한다.

미션 사건은 Guide 뒤 같은 공통 Gump에서 한 번 소비를 판정한다. 소비되지 않은 바깥 좌우 누름은 팝업을 닫고 같은 사건으로 월드에 계속 전달한다. 열기 자체는 Pause를 하지 않는다. 가로 메뉴 자동 입력점은 ItemPosition의 하위 부착 Y 대신 실제 열 너비/행 높이를 사용한다.

메뉴 명령은 현재 원본 정수 ID와 동일 큐/80칸 문자열 고리로 전달한다. Tell의 param2는 event.y이며, TryTell은 실제 인자 파서를 통해 로컬 param 설정을 설치한 채 HTML을 해석한다. RequestQuitApp의 확인창, 음량, Pause, ViewHome/ViewYourPriest, HideBuildings, ReloadTheams를 실제 소유자에 연결한다. Pause11은 기존20칸 MessageBoard와 실제 TextGump를 공유하고 만료/idle/새경고 음향을 추가하지 않는다. Pause 명령이 상태 안내→현재 깊이 판단→커서 취소/시계/음소거 순서를 소유한다.

카메라의 433ba0은 실제 TYPE 전체 발 기준점과 float 저장을 유지하며, 화면 적용 뒤 현재 flags2를 재조회하여 기존426080 내용물 표시를 요청한다. Reset은 같은 world loadingDepth를 사용한다. 명시 소유자 사제 조회는 기존494f10 참여 조회를 사용한다. ReloadTheams는 현재 outposts→factories 두 목록을 다시 읽으며 기존455380 지역 투표/도장을 실행한다. 건물 숨김은 실제 SpriteDraw.specialMode와 Pick의 해당 후보 관문을 공유한다.

Config6d는 같은 parser/현재 내부 인코딩 경계, revision 증가, redraw, 원본의 두 번째 Gump 전달을 유지한다. 실제 복원된 효과음·음악 장치 옵션을 반영한다. 전체 InterpretOptions·원본 화면 장치의 모든 재해석까지 완료한 것으로 확대하지 않는다. MenuJustClosed의 대화 부모 Kill은 현재 페이지 수명 정리 어댑터를 사용한다.

## 검증

독립 관측은 가로 부모510행·Game/Edit/View/Options918행·Players530행·Tell 인자1070행·일반 메뉴 명령610행·Pause 메시지648행의 총4286행이다. 각 상세에 실제 실행 범위/명시 외부 경계/정밀도 차이0을 기록했다. 전체 OS 입력/Gump 그래프/전체 GUI의 독립 관측이라는 뜻은 아니다.

- 빌드2 종료0/제품 오류0, 메시지 검사 바이트 상수 경고1을 발견했다. 가로 공통트리 검사의 기존 팝업 보존 기대1도 실제 Attach replace/지연 Kill에 맞춰 수정했다. 제품 원본 Attach는 바꾸지 않았다.
- 최종 Release 빌드3 종료0·경고/오류0, 관련40검사 실패check0. 전체 CTest1은 **2399검사·실패check0·종료0/196.17초**이며 LastTest.log 내부2399/0도 확인했다. 최종 검사 뒤 제품/검사 C++ 변경은 없다.
- GUI1은 미션 진입 안내의 정지 해제 전 너무 이른 관측 때문에 실패하여 검사 시점을 수정했다. GUI2의1024 core는 실제 ESC 열기/닫기·목표 재열기·Leave 취소·View/Version·재시작을 통과했다. GUI2의1280 probe는 options.cfg 일시 쓰기 실패로 종료1이며 원인은 미확정이다. 같은 제품 GUI3의1280 재실행은 core와 음량2/4·해상도 SETSIZE 확인의No·QWASZX 여섯 슬롯·바깥 클릭 월드 선택까지 통과했다. SETSIZE 캡처에서800 by600 문구도 직접 확인했다. 인자 원문 토큰 자체의 추가 보고는 없다.
- 별도 Pause GUI는 실제 Options 행→영구 안내·정지→정지 중 ESC/Options 재열기→같은 Pause 행 재개·안내 제거·음소거 깊이1 복구를 통과했다. 기존 미니맵 crop/capture/카메라/미션 이탈과 생산 단축키/회전/배치/재공급 회귀도 통과했다.
- 모든 GUI 실행은 PID/바이너리 SHA/종료/시간 제한과 전체 originals 스냅샷·허용 설정 복원을 보존했다. 각 실행 종료 후 originals 원문 해시는 동일하다. 실제 원본 게임은 실행하지 않았다.

주요 자료는 `extracted/cpp-game-menu-20261023-{build3.log,ctest1.log,LastTest.log,scope.json}`과 `gui1/gui2/gui3`, `pause-gui1`, `minimap-regression1`, `shortcuts-regression1`이다. 제품 실행파일 SHA256은 `0891896326e0331d1196f2ebad960e20dbc875656c38008970bb13045f499688`, 검사 실행파일은 `535931b36cb5fbd6c83fcf632d9c5d46d25854fccf5ab56c537fad43274e46e0`이다.

## 계속 남은 범위

현재 전체 HUD/viewport는 고정 기본 배치 어댑터이며 native dock left85를 실제 ChatWindow/NSInfo 생성자와 연결하는 작업이 남는다. 실제 캡처에서 구형 위쪽 Game 돌 버튼과 ESC 막대가 겹치므로, dock 연결에서 이 임시 버튼을 원본 수명으로 교체/제거하고 다시 검사해야 한다. Chat/Player List 토글, Editor Session 및 여러 편집/장치 명령은 아직 미연결일 때 명시 오류를 낸다. 새 Chat/NSInfo/TextEntry 복원 파일은 별도 후속 단위의 미등록 자료다. 해상도 Yes가 쓰는 OnExit QuitApp99는 현재 페이지 수명에 아직 연결하지 않았으므로 종료까지 완료한 것으로 표시하지 않는다. 별도 DialogExitCommand 원문408행/제품/검사/문서는 단위370용으로 준비했으며 미등록·미빌드다. 기존 Pick 전체는 근사 선택이며 이번에는 건물 숨김 후보 관문만 추가했다. 키보드 모든 단축키, edgeScroll, 전체 사용자의 Early Missions/캠페인/데모 완주와 원본 전체 동일성은 계속 목표다.
