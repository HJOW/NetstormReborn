# 실제 Sky Overview 미니맵 연결 — 2026-10-23

마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이며 현재 호스트와 일치한다. AGENTS.md·공통/닷넷 인수인계를 다시 읽고 기존 `extracted/refined/originals/Netstorm.c`와 보호 PE를 사용했다. 새 디컴파일이나 보호 원문 변경은 없다. 실제 창을 사용하는 검증은 이 호스트의 실행 확인 예외에 따라 클론에서 수행한다.

## 복원한 원문과 실제 연결

| 역할 | 10.78 원문 | 제품 |
| --- | --- | --- |
| 생성·기본 수명 | 004801b0 / 004815b0 / 004d4c40 | RawMinimapControls, 같은 GumpCreate/GumpLifetime |
| crop 재생성 | 00480270 | RebuildMinimap |
| 카메라 목표·바깥 드래그 | 004803f0 / 00480440 | MinimapTarget / DragMinimap → 실제497f20 |
| 부착 전 crop 보정 | 004805e0 | ClampMinimapAttachment → 같은 GumpTree |
| crop 블릿·현재 시야 outline | 00480660 / 004a24b0 | PlanMinimapDraw → 같은8BIT Gump 출력/정렬 |
| 셀 그림 | 004808d0 | RawMinimapRaster → 현재 Session Pool/Hash/TYPE |
| 클릭·캡처·뗌 | 00481620 | 공유 GumpInput/Idle 및 실제 Carrier.Commit |
| 프레임 갱신 | 00481320 | Client의 Kernel 앞 RefreshMinimap → 기존 RawMetadisplayDirty |

미니맵을 별도 화면 오버레이나 독립 입력 사각형으로 만들지 않았다. RawButtonControls의 같은 노드 조회/가상 부착/입력/Idle/삭제/등록표에 연결한다. RawMenuControls의 외부 파생 출력은 슬롯0 생산/슬롯1 미니맵이 같은 수집·정렬·클립을 공유하며 각각 참조를 반납한다. 기본 우클릭은 기존 GumpHelp를 사용한다. 정적 제목/Help 토큰은 같은 문자열 해석 경계에서 처리한다.

원본 생성 타입은 signed BYTE29, VT는0050d51c다. 기본flags301, 부모 HUD의20000, 명목128×128와 crop 두 signed DWORD·scrolling DWORD를 보존한다. 현재 정상 HUD mode3 기본 분할은 세로분할x80, 하단분할H-80, 두께5여서 **(0,H-75)의80×75 crop**이 된다. 128×128을 축소해서 그리지 않는다. 생성 중간의(0,0)과 독립 토글480810의root(100,100)를 기본 HUD 위치로 오해하지 않는다.

카메라 중심 계산은 `center - ftol((창크기-visible)/2)`다. 차 전체를 먼저 절삭하면 정상80폭에서도1픽셀 달라진다. 이동 목표는 정수crop+local을 확장 정밀도로 두 배한 뒤 마지막float 저장을 한다. 좌/상단은local<=0, 우측은local>=width, 하단은local>=height-1이다. 첫 집중 검사에서 끝점 포함을 빠뜨린66관측·140check 차이를 발견해 실제 x87비교의<=조건으로 수정했다. Draw는min(128,크기)의 crop을 복사하고 오른쪽/아래 제외 사각형을004a24b0처럼각각-1하여그린다. 일반 카메라 중심과 같은 CameraHome::CenterPoint를 사용하며 이동 입력이 없는 UserInput::Tick이 Scroll(0,0)으로 원본 중심을 다시clamp하지 않게 했다.

셀은2×2 검정 초기화→지면4×4(Y/X)→Hash2 한 버킷→Hash1의2×2(X/Y)→Hash0의4×4(X/Y) 순서다. 지면 WORD는 별도 지형 사본이 아니라 Hash0의 실제 alias다. isle97/noIsland157만 지면색을 쓰고, 일반 해시에서는flags2&6을 검사한 뒤flags2&4인 다리만 한 점을 덮는다. 일반 건물/유닛 아이콘을 임의 추가하지 않는다. Challenge109/108은level1/0에서signed좌표절삭·/2·각축&~1 후2×2를 그린다.

소유자1..8은named11/basic1/basic7/named10/basic4/basic5/named12/basic6이며 중립isle는named18, 중립noIsland는named19다. 무소유 다리는literal246이다. 색 억제 시 검정 초기화를 제외한 색은 증가한 미니맵프레임의 하위BYTE다. 실제 callback이dirty.frames를 읽는다.

Pop/Unpop은 현재 같은 SID 풀의 dirty로 연결한다. 이 큐의 기존 공간 통지 시점과 활성/포화/중복 조건을 변경하지 않는다. bitmap·큐·프레임은 개별 노드/미션 교체보다 긴 RawButtonControls 수명에 두고, 새 월드의 전체 그림을 요청한다. Close는 같은Kill/Cleanup으로캡처·Idle·등록을정리하며Session해제전Pop/Unpop과외부Draw참조를끊는다. 일반 눌림은center→capture→scrolling/Idle, 뗌은scrolling/Idle해제/반환1이다. 모드2/5는카메라드래그없이현재선택의Carrier.Commit→Highlight0을실행한다.

## 독립 원본 관측과 검사

`tools/decomp_minimap-cell.py`는 셀 전체/Hash4b2b00/flags4ac200/TYPE49a840/CRT4e49c0을 실제 실행한다. 장치 반환 대체 없이 **2,361입력·두 정밀도4,722관측**, 실제6,172,554명령, 정밀도 차이0이다. 정상RET·스택·보존레지스터와 전체bitmap을 관측한다. MinimapRasterTests는 같은 raw입력/해시를 공급하고 전체128×128출력과 비교한다. native쓰기는별도TSV에순서대로보존하며제품검사는최종전체bitmap을비교한다.

`tools/decomp_minimap_geometry.py`는 생성/전체dock과 구별하여 재생성/이동/Idle/부착/Draw 다섯 기하 몸체와 실제CRT를 실행한다. **527입력·두 정밀도1,054관측**, 전체157,622명령 중CRT68,128, 정밀도차이0이다. Bounds/폴링·로컬변환/clip/blit/frame/center/기본Attach/Dirty/assert는명시외부경계다. 정상1050행과명시중단4행을구별한다.

MinimapGeometryTests는callback변경없는**1,028관측**의상태전체·불변view·이동float비트·전체블릿인자·exclusive사각형을제품과비교한다. callback변경/부분중단26행은순수API의분배범위가아니므로제외수를명시하며성공으로계산하지않는다. 두새검사의직접제품대조는합계**5,750행**이며원문모든5,776관측과구별한다.

최종 Release 빌드4는 종료0/경고·오류0이다. 빌드1의 지역훅 이름 가림 경고2건은 빌드2에서 없앴고, 최초 집중 검사에서 드래그 끝점 포함의66관측·140check 차이를 수정했다. 최종 집중2검사/실패check0이며 제품과 검사 C++는 이 시점 뒤 동결했다.

GUI1은 표시·드래그를 관측한 뒤 종료 시3221226505로 실패했다. unique_ptr::reset이 공통 가상 조회 포인터를 먼저 비워 소멸자의Close가 노드를 찾지 못했다. DisableMinimap에서 처리기가 아직 조회되는 동안 Close를 먼저 실행하도록 수정한 뒤 빌드4로 다시 검증했다. 이 실패를 성공으로 세지 않는다. Leave Battle은 즉시 이탈이 아니라 원본 확인Tell을 열므로 실제 Leave Honorably 버튼까지 누르게 입력 계획도 보완했다.

최종GUI2(1024×768, PID17700,3.469초)와wide1(1280×720,PID3932,3.515초)은클릭·우측/상단드래그·release/실제capture·Idle정리·브리핑입력차단·ESC확인/미션이탈/MainMenu·정상종료/원본불변을통과했다. 우측crop48의1024화면카메라는3904/782, 상단crop0의카메라는2304/-494다. 가장자리밖중심을무입력Scroll로덮지않음을확인했다. 실제초기/드래그/메뉴스크린샷을직접열어80×75crop의원본색/시야outline을확인했다.

같은최종빌드의기존단축키GUI(PID17612,50.047초)도여섯실제Form취득/회전복귀/합법배치/소비·보충/원본불변/종료0을통과했다. 합법다리SID15201 생성뒤관측의미니맵dirty count2/full0과다음소비count0, bitmap해시680181961→4172994885를관측했다. 다른자연월드변화도포함하므로그해시전체변화를단일픽셀만의효과라고주장하지않는다.

세성공GUI의실제게임실행파일SHA256은 `1d335ced718e124e143ffa893b5792475168a1436369413160c90f5e1896b0eb`다. 원본PE는 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`로불변이다. 실제입력은같은큐를쓰며검사용폴링ffff도마지막스크립트마우스좌표를읽는다. 모드2/5 이동은원문/제품수식과정적공통연결을확인했으나이번GUI계획에는포함하지않는다. 최종전체CTest1은 **2,373검사·실패check0·종료0/182.32초**이고 LastTest.log 내부2373/0도 확인했다. 전체검사뒤에도 originals 전체파일목록/해시가 불변이다. UTF-8/한국어새주석·도구AST·원본/도구/두fixture SHA 및 diff를 확인했다. 기존ClientMain의CRLF는보존했으며cr-at-eol 기준의공백검사도통과했다. 최종검증뒤제품/검사C++는변경하지않았고 `20261023-35 CP` 로컬커밋으로마감한다. 원격push는없다. 분석 산출물은 `extracted/cpp-minimap-cell-20261023-native.json`, `extracted/cpp-minimap-geometry-20261023-native.json`, 두fixture 및 `extracted/cpp-minimap-20261023-*`다.

## 계속 남는 전체 목표와 경계

기본80×75 배치는 정상mode3 원문 결과지만 전체 divider/dock 드래그·폭/높이 제한·HUD1/3 전환과 원본 지도viewport배치는 아직 미복원이다. 현재 미니맵은 기존 GameWorld가 실제로 표시하는viewport를 그대로 읽으므로 원본분할의left85/하단chat분할과 차이가 남는다. 색 억제 전역의 UI 토글도 미연결이며 기본0이다. 독립480810토글·노드재생성의 전체원문경로도별도다. 이 경계를 원본HUD 전체 동일성 완료로 세지 않는다.

전체 Early Missions·전체캠페인·데모3 사용자 완주, Edit/커스텀맵·시나리오, Multiplayer외메인메뉴, 좌측덱/ESC전체메뉴, snapshot부모와Editor저장의남은범위는계속활성목표다. 좁은native대조나미니맵GUI검증을전체게임완성으로확대하지않는다.
