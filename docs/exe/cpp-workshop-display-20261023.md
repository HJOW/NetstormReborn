# 워크샾25종의 실제 본체 표시 연결

단위344의 `f5a13900`부터 진행한다. 현재·마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-22**로 일치한다. 원본/CD/AGENTS 및 기존 fixture를 수정하지 않으며, 게임 GUI를 실행하지 않는다.

## 표시 경로 차이와 복원

기존 `RawCampaignFactoryDisplay`는 특수 Factory/제네레이터/신전/일부 발사기·훨리베이스 등을 원본 SpriteDraw로 처리했다. 나머지 일반 워크샾 자산은 `GameWorld.Sprites`의 단순 SHP 출력으로 넘어갔다. 그 경로는 TYPE 기준점 차감과 소유자 색 대상이 일부 타입에 한정되어 있었고, 건설 중/추상 색 분기도 원본 공통 Draw에 연결되지 않았다. 이전 파편 캡처 통과는 실제 장면 표시의 증거가 아니었다.

`RawPlainAssetDraw`가 기존 독립 캡처의 정확한 일반 유닛/워크샾 타입·VT와 블로커/데빌메이커/아크 스파이어/성장형의 공통 +78/+a4를 확인한다. 이를 같은 실제 표시기에 연결한다. 일반 자산은 TYPE 기본 프레임과 빈+a4를 쓰며, 기존 특수 발사기·훨리베이스·썬더 캐논은 전용 그림을 보존한다. GameWorld는 현재 Handles 결과를 사용하므로 같은 본체를 단순 출력으로 중복 제출하지 않고 클릭 영역도 원본 기준점을 따른다.

일반12종/전용5종의 원본 소유자 색표 대상도 실제 표시 전역에 연결했다. 일반 표는 기존 `BuildStandardOwnerColorMaps`, 명도 기반 전용 표는 현재 파일 팔레트의 `BuildSpecialOwnerColorMaps`를 사용하고, 그릴 때 현재 `bridgeState.ownerColors[owner]`를 읽는다. `BuildSpriteColorMaps`의 건설 중/추상/그림자 색표도 기존 원본 SpriteDraw 분기가 선택한다. 새 색 계산이나 임의 팔레트를 만들지 않는다.

원본 수집기의 `entry.tail&4` 그림자 표식은 외부 `RenderSprite.shadow`에도 보존한다. 실제 출력은 원래 entry를 그대로 사용한다. 일반 경로에 새로 포함되는 사제와 비행체의 선택 표시에 필요한 현재 event `0x25d` 조회와 Form31의 같은 FlyerProcess 기한도 연결한다. Native Draw는 inner에서 outer로 `-0x30` 보정한 뒤 `+0x48/+0x68/+0x28`의 double을 읽는다. TYPE의 `casttime/praytime/effecttime`은 실제 `+0x1b8/+0x1bc/+0x1cc` 속성으로 표시 뷰에 공급한다. 기존 resource `0x493`과 선택 중 상태 프로세스 목록의 미연결 경계는 계속 남긴다.

## 근거와 검증 범위

`tools/audit_unit_draw_bindings.py`는 [정적 근거](../../cpppj/unit-draw-bindings-20261023.json)에 **25종 중 일반19/기존 특수6**, 비행·성장 파생4종의 실제 +78/+a0/+a4를 기록한다. 이 표만으로 전체 수명이나 GUI 완료를 주장하지 않는다. 원본 SpriteDraw의 기존 독립 `spritedraw-x86.tsv`는 **1,389입력·두 x87정밀도2,778정상반환**이며 이번에도 읽기 전용 검증을 통과했다. 기존 원문을 새 C++ 출력으로 바꾸지 않는다.

`CampaignWorkshopDisplay_All25ActualBodyAndColors` 검사는25개 독립 GameWorld에서 실제 로그인/입력 선택 서비스, 제품 생성자/받침 확정/저장 객체 Pop(flags9)/첫 Kernel 프레임과 카메라를 사용한다. 이는 명시 시작 상태이고 사용자 생산·건설 완료를 대신 증명하지 않는다. 일반19종의 owner1/8과 일반/건설 중/추상 표시 입력은 원래 SHP·기준점·원본 색표 출력과 전체 픽셀을 비교한다. 상태·소유자 색은 명시 표시 입력이며 HP/물리/프레임 진행 결과를 검사에서 대신 쓰지 않는다. 각25종의 실제 선택/해제와 괄호·HP 출력 경로도 확인한다.

최종 Release 빌드9는 종료0·경고/오류0이고 새 표시 검사와 기존 골렘 검사는 각각 집중6/골렘2에서 통과했다. 전체 CTest2는 **2,312개·실패0 / 종료0·283.63초**다. 기존 건설·전투 통합 검사의 사제 제외 조건도 새 일반 표시 범위에 맞춰 포함으로 갱신했다. 비교 이미지의 양 끝 포함 VFX pane은 좌표에 pane 원점을 더하므로 절대 좌표에서 원점을 차감한다. 첫 프레임의 실제 보행/회피 후 현재 좌표를 읽고, 보행 타입은 원본 수집기의 정렬 좌표 `Y+0.5`로 본체를 찾는다. 이 보정은 정렬용이며 실제 픽셀 기준점은 raw의 원래 위치다. 기존 골렘 검사는 단순 block 번호 대신 같은 실제 본체 procedure를 고르되 원본64프레임/두 카메라의 **81,600픽셀 기대값**을 그대로 보존했다. 최종 로그는 `extracted/cpp-workshop-display-20261023-build9.log`, `focused6.log`, `golem2.log`, `ctest2.log`(같은 접두사)와 CTest LastTest.log에 남겼다. 최종 검사 뒤 제품/검사 C++를 변경하지 않았으며 스테이징한12개 소스 SHA도 정적 근거와 일치한다.

전체25종의 공급/이동/공격/피격/삭제·표시 상호작용과 파생의 전체 수명, 주문 원주 배치, 사용자 GUI 플레이 및 전체 Early Missions/캠페인/데모3단계·Edit/커스텀 시나리오·배포 목표는 계속 진행한다.
