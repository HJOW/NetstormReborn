# 실제 섬 추가 창 조립 — 단위480

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본/CD, AGENTS.md와 정밀 디컴파일은 수정하지 않았다.473~479에서 복원한 실제 섬 배치/Bitmap/Image/MetaImage/21칸 목록/배치·미리보기/내장 설명을 하나의 실제 창과 일반 입력 명령에 연결했다.

## 생성과 일반 입력

`RawMenuControls::ShowEditorIslandWindow`는00480090→0047f450의432×412 장식 창, 가운데 정렬과 전체 상대 깊이−80,004d7bb3 이후의 Attach(0,0,root,2)→00466600 모달 등록을 수행한다. `UberGump`의 AddIsland press는 이 공장을 호출하며 기존 미연결 예외를 제거했다. release는 기존처럼 소비만 한다. 일반 팝업 phase나 간략한 대체 창을 만들지 않는다.

생성자는 원본과 같은 TYPE127/프레임0 지도 배경, 현재 Image를 빌리는 MetaImage, 숨김 글자와 제목의 실제 ScrollingText,4열21칸 목록, Done 돌 버튼, 실제 PlacingIslands HtmlBody를 만든다. 원본의 기본 문맥·흰 제목/숨김 글자의 파란 그림자 값·정렬·생성 순서와 자식 부착을 유지한다. Done 호출의 원본005acd24 인자는 팔레트의181/140/111 값이다.

`EditorIslandWindowNode`는 현재 crop0 Image, 독립14×14 preview, 후보21×6BYTE와 드래그 record, 일곱 자식, dirty/ready와 현재 원본 참조 수를 같은 공통 저장소에 둔다. 실제 생성 전에 증가하지 않은 참조 수를 부분 생성 정리에서 감소시키지 않도록 호스트 추적을 추가했다. 성공/부분 생성 모두 실제 Gump 등록/수명에 참여한다.

## 갱신과 입력

0047f9b0 Refresh는 현재 manager를 먼저 확인하고, 기존 지도 Image를 Destroy한 다음 같은 Session으로 실제 지도/bitmap→crop0 Image를 만든다. 기존 후보는 원문처럼 Image Destroy 두 번→실제 목록 Kill/압축 순서로 반납한다. 각 후보는 현재 flag에 따라 공용 GameRandom으로 seed→새 Image→실제 crop1 shape→실제 목록 Add를 수행한다. 모든 seed를 먼저 실행하는 별도 배치로 바꾸지 않는다. 마지막 Push/현재 좌표 Attach는478의 실제 배치 몸체를 사용한다.

00480130 Idle은 dirty일 때만 Refresh하고 shape가 생긴 뒤 redraw/dirty0을 수행한다. ready일 때 원본 등록 ID005ca930의00419d50 관문을 호출한다. 이 관문은 현재+a4/+a0/+ac의 bit10을 차례로 확인한 뒤 실제 버튼 SetPulse(true)를 호출한다. 현재 ID의 BSS0과 실제 Uber 루트의 필수 자식 필드를 같은 의미로 읽는다. 원본 루트는+a4의 minimapDivider가 비영이므로 이 경우 점멸 요청을 수행하지 않는다. 유효 버튼과 원본 Uber 필드 이외의 부당한 대상은 호스트 진단으로 구별한다.

473의0047fb90 입력은 같은 현재 목록/ID/Image와 Session의 실제 Drop→지형/거주 구역·사제/Graph/Territory에 연결했다. 플래그는 실제 부모 Gump+4를 직접 빌리므로 가상 Kill/상태 변경을 이전 복사본으로 덮어쓰지 않는다. 선택 드래그 생성, 목록 위 취소, 배치 성공 후 목록/Image 반납·선택 해제·지도 변경 전역 증가와 재갱신을 수행한다. 미리보기 Broadcast는478의 실제 격자/색상/깊이를 사용한다.

이번 연결에서00465250이 트리 Push라는 이전 장치 이름의 해석을 정정했다. 실제 몸체는 비어 있는 focus에 현재 창을 등록한다. 기존 관측 API 이름 `pushParent`와40행 fixture는 유지하되 실제 장치는 이 focus 등록에 연결했다. 선택 시 창을 트리에서 분리하지 않는다.

## 수명과 설명 기본색 정정

창 소멸은0047f940/00480110의 현재 Image Destroy→참조 수 감소→중복 Destroy→기본 부모 반납을 유지한다. 후보 Image는 MetaImage/목록이 소유하지 않으며, 창 밖 루트 드래그가 빌린 Image도 부모 반납만으로 해제하지 않는다. 상태 이탈의 `CloseEditorIslandWindows`는 부모/부분 생성/루트 드래그를 먼저 공통 Kill/Cleanup하고 그 뒤 후보 소유권을 해제한다. `ClearWorld`도 이 대여를 Session보다 먼저 반납한다.

기존 공통 UI의 설명 기본색이 흰색으로 연결돼 있던 문제도 고쳤다. 원본 팔레트 초기화는005b5dc8에 RGB219/213/203을 저장하며 HtmlBody/QuickHelp의 실제 다중 줄 생성자가 이 값을 읽는다. 두 판본의 색 목록 인덱스 차이를 피하기 위해 같은 RGB 항목을 찾아 현재 인덱스를 공급한다. 새 실제 창 검사에서도 내장 글자의 기본 문맥을 이 원본 색과 대조했다.

## 검증과 범위

신규2개 C++ 검사는 보호 자산을 사용하는 공통 UI와 실제 기존/신규 Editor Session을 동기 fixture로 조립한다. 현재 Session의 같은 GameRandom·실제 월드/목록/Image를 사용하며, 카메라/점멸/음향 장치 경계는 요청을 관측한다. 기본 루트에 장치를 임의로 가장해 사용자 게임 실행을 했다고 주장하지 않는다.

기존/빈 새 맵에서 실제 창 생성/모달, 지도와21후보 생성, 같은8비트 표면의 전체 창 출력/행 padding, 실제 입력 라우팅의 선택/취소와 focus 유지, 실제 섬 배치/카메라112 요청/변경 전역, 후보 재갱신과 seed 재소비 없음, Done의 Kill과 지연 참조 수 감소를 확인했다. 별도 검사는 부모가 닫혀도 루트 드래그의 비소유 Image가 유효하고 상태 이탈에서 반납됨을 확인한다. 부분 중복 생성의 refCount 진단과 정리도 확인한다.

새 Native CPU fixture는 만들지 않았다. 기존 원본 드래그/배치/목록/이미지/bitmap/HtmlBody·설명/섬 배치의 C++ 원문 재생을 함께 실행하고, 새 조립은 실제 UI/Session 통합과 정적 원문으로 검증한다. 원본00419d50/00465250/00480130/004d7bb3 및 팔레트·타입 전역 대조는 `extracted/cpp-island-window-480/source-facts.log`에 보존한다.

최초 build1은 새 검사 include의 잘못된 Client.h 경로에서 실패했고 ClientMain.h로 정정했다. build2 이후 첫 신규 실행은 새 맵 준비에 기존 맵 전용 PumpLocalConnection 거부 전제를 가진 helper를 재사용해 기존 helper의 CHECK 하나가 실패했다. 신규 helper의 새 맵 분기만 이미 검증된 NewFort 설치 흐름으로 고쳤다. 기존 검사 본문/기대값은 유지했으며 build2→3 제품 소스와 게임 실행 파일은 동일하다. 두 최초 실패 원시는 덮어쓰지 않았다.

최종 명령/종료값/로그 SHA·집중/전체 결과·보호 바이트·소스 동결과 실행 파일 SHA는 [실행 보고서](../../cpppj/editor-island-window-runtime-20261027.json)에 기록한다. 게임/GUI/음향 장치는 실행하지 않았다.

- 최종 Release build3은 경고/오류0, 집중166고유/169관측·실패0, 같은 실행 파일의 전체 C++3,260개·실패0이다. CTest wrapper 대신 실제 검사 실행 파일 전체를 실행했다.
- 기존3,258등록을 모두 유지했다. 원본/CD/AGENTS·기존 검사/자료3,187파일은 바이트동일이며 기존 메뉴/Editor 검사46,494/117,193BYTE 전체 접두부를 보존하고 새 include만 추가했다.
- 최종 소스/검사2,839파일과 두 실행 파일은 마지막 빌드 이후 동일하다. build2→3의 변경은 새 준비 helper 하나뿐이며 제품 소스와 게임 실행 파일은 동일하다. 모든 빌드/검사 프로세스는 종료했다.
- 게임 SHA256은 `52abfb6ba5c9c335c1df4f2bd685b435e41ae38ebdac5f4dcce39e018bd37be9`, 검사 실행 파일은 `a161001120fdaeaab0f1e85423d983975de32a162f274cc2e8f21e3f5b950fb6`다.

## 다음 확인

전용 창 조립과 실제 AddIsland 명령은 연결됐지만, 실제 게임 창에서의 기존/신규 맵 선택·취소·거부·배치·닫기와 저장/재진입 검증은 남아 있다. 원본과의 전체 화면/음향/입력 동일성 완료로 합산하지 않는다. 사용자 지시대로 실제 구동과 개입 검증은 뒤로 미룬다. 다음 기능 묶음은 Test Battle/편집 복귀와 언어별 시나리오의 실제 경계를 확인한다. 전체 미션 자연 완주·메뉴/UI 동일성 목표도 계속 진행 중이다.
