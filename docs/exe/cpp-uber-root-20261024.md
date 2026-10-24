# Uber 루트의 원문 부모·현재 필드 뷰 복원 — 2026-10-24

이번 단위395는 **004c7160 전체 생성/004c77c0 루트 교체 부모와 공통 도킹 뷰를 복원·검증한 컴포넌트 작업**이다. **현재 Client의 활성 루트는 여전히 RawButtonControls의 기존 기본 루트이며 새 부모를 호출하지 않는다.** 실제 미니맵·생산·NSInfo·ChatWindow·HTML/전체 State/Core/Editor 연결이 완료되었다고 주장하지 않는다. 새 게임/GUI 실행은0이다.

시작 HEAD는805b2717이며 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 PE/디컴파일 산출물을 수정하지 않았다.10.72의 기존 대응은4c77c0↔4cd000이며 이번 C++/Native 범위는10.78 전용이다.

생성자는 같은 실제 Gump 기본주소에0xc8 Uber 필드를 두고 세0x11c 구분선→미니맵→생산→전체 NSInfo→ChatWindow→10회 도킹관계/제한→4회 정리/3회 Move→조건부0xb0 메시지를 원문 순서대로 요청한다. 필수 하위가 없으면 선택된 위치에서 실패한다. 이전 객체 재사용/빈 노드/종류만 맞춘 기본 Gump를 실제 공장 성공으로 대체하지 않는다. 메시지 비교·9인자 조건 표현식 작성·검사·방문·사건·Overlay·깊이도 별도 원문 위치를 보존한다.

교체는 새 전체 생성 뒤 임시 등록칸만0으로 만들고 **count를 감소시키지 않는다**. 새 id는0이며 원문 등록0칸의 호스트별칭도 같은 값으로 반영한다. 이전 직계자식의 next를 Push 전 보관하고 Push 뒤의 현재x/y로 새 루트에 가상Attach한 뒤 기존 공통 ReplaceRoot를 호출한다. 새루트의 기본flags800/종류BYTE540451과 미작성 확장 필드를 보존한다.

기존 HudDockNodes의 값사본을 만들지 않는 `DockView`/`HudDockNodeView`를 제공한다. 등록0이 아직 이전루트인 생성 중에도 해당 **this**의 현재 확장필드를 읽는다. 도킹 생성 뒤 vertical/minimap 필드를 바꾼 검사에서 실제 새필드로 viewport가 갱신되고 전역등록0으로 치환되지 않는 것을 확인했다. 기존 HudDock 호출자는 같은 기존 상태를 참조하는 getter로 연결되며 원문 기하/입력 몸체는 유지한다.

NSInfo 크기는 원문 FILD→FST→FMUL의FST가 레지스터를 float로 좁히지 않는 것을 반영했다.16777217/16777219와 배율1.25 입력도 실제 x87 결과와 일치한다. 기본 저장 float를 다시 읽어 곱하는 대체식으로 바꾸지 않았다.

4437f0은 실제 PE의 **C3 한 바이트 빈RET**다. 과거 MapMode라는 이름은 지도/모드 계산을 뜻하지 않는다. 새 부모는 실제 빈 본문으로 복귀하며 게임 하위 구현을 요구하지 않는다. Native 자료의 map 라벨은 최초 관측 표기를 보존한 것이며 선택 관측만 연결한다. Root/Divider vtable의 scalar 항목은 모두4d4c40이다. PE의 실제 종류는 Uber54/divider55다. Native fixture의11은 명시 입력이며 실제 기본값으로 쓰면 안 된다.

최초 Native1/2는 도움말 cdecl 경계를 잘못 명시해 GP보존 guard에서 실패했다. 두 로그를 보존했고 교정 Native3은 **80입력×027f/037f=160관측·각33322명령·정밀도차이0**이다. 정상RET의 GP/ESP/SEH/x87과 전체 원시 노드/등록표/전역/호출 및 부분 효과를 확인했다. 특수 메시지의 별도 정상 epilogue4c77a0..4c77b3도 실행 범위에 포함한다. 최초 fixture/보고서/원문·도구 SHA를 보존했고 verify1도160동일이다. 하위 공장/가상함수는 명시장치이며 전체 하위생성자/게임/EH unwind를 증명하지 않는다.

build1/2는 검사 Scene 이름충돌과 iostream 누락으로 종료1이었다. build3~7은 관측 경고오류0·종료0이다. 최초 focused1의118check 실패는0xb0 메시지 할당종류 m/b 검사 라벨 차이다. 최초 실패를 보존하고 라벨 교정 후 전체 Native160이 일치했다. 최종 집중 검사는 **신규6+기존27=33/실패0**이다. 실제 공통 GumpCreate/Tree/Kill/Cleanup의 이전3/새7 직계자식 보존·이전루트free/유일등록·최종count0, 필수공장누락 부분효과, 이전루트없음 실패/별칭, this현재뷰, 빈RET 무장치 경로를 확인했다.

전체 CTest는 **2683검사/실패0·종료0·299.02초**다. 제품 `48ee8913a25557dc3068839d3a80df6a119a52a9318fac5fe245392f558fea73`/검사 `9e38f088ddf38ee5c3f5438f109d53c2ff5e60c608d66e38e249e06757ee1df9`는 전후 같다. 원본1394파일 바이트/AGENTS/보호추적변경은0이다. [제품 보고서](../../cpppj/uber-root-runtime-20261024.json)에 소스9·Native fixture·28원시 증거 SHA와 현재 Client 미연결을 기록한다. 원시는 `extracted/cpp-uber-root-395-*`다.394의 실제 Early/안내 결과를 새 실행으로 합산하지 않는다.

다음은 같은 실제 Client Gump/Quad생성자/HTML/current 전역과 저장소 수명을 연결하는 작업이다. RawProduction의inline root/budget는 글로벌 Release에서 token이 남는 반면 미니맵 Release는 token을0으로 만든다. 글로벌 Kill/Cleanup 뒤 provider Close/교체의 중복 반납 여부를 먼저 실제로 확인해야 한다. 두 getter의0토큰 거부는 이미 존재하므로 없는0토큰 버그를 새로 고쳤다고 주장하지 않는다. 또한 Production/Minimap의 Session 종속성과 공통 World/Player 수명, State/Core/Pool/SP 순서를 실제 소유자에 연결해야 한다. 전체 Editor·모든 사용자 자연완주/동일성과393의 원본 경로 Wide 옵션 오류는 미해결이며 목표는 활성 상태다.
