# 현재 객체·청크의 Fort 저장 복원 — 2026-10-23

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 기존 `extracted/refined/originals/Netstorm.c`와 보호 PE의 실제 명령을 사용했다. 새 디컴파일, 게임/GUI 실행, AGENTS.md 및 originals/originalCD 변경은 없다. 원본 10.78 실행 파일 SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.

이번 범위는 단일 객체 `004bd940`와 청크 `004be020`, 그 둘이 사용하는 실제 Session 연결이다. 전체 지역 부모 `004be430` 및 Editor UI의 전체 저장을 완료한 것으로 계산하지 않는다. 앞선 TypeNames·Deck·Technology·Territory writer와 같은 명시 `TemplateWriteStream`에 쓴다.

## 객체 저장 계약

`RawFortObjectSave`는 첫 raw 좌표의 float32를 실제 CRT `__ftol`과 같이 signed64로 절삭한 뒤 하위 DWORD를 보존한다. NaN·무한대·signed64 범위 밖 불확정값의 하위 DWORD는0이다. 초기 좌표는 고정하지만 저장 과정의 raw/TYPE/전역은 원본 위치마다 다시 읽는다. 상단 경계만 먼저 검사하며, 아래 경계를 임의로 추가하지 않는다. raw+40의9, Battle/Mission/Editor와 현재 flags2 마스크, Scope1/2·가상60·현재 좌표의 섬 판정, 로컬 다이스 소유자 순서다. Scope는 가상 호출의 효과 뒤 다시 읽으므로 처음1이던 값이2로 바뀌는 경로도 보존한다.

좌표 오프셋 진단667/668 뒤에도 복귀하면 위치 BYTE를 쓴다. y를4비트로 자르는 보정을 추가하지 않는다. 타입은 위치 쓰기 뒤 다시 읽으며 다리는 현재 owner의 보수를 낮은 BYTE로 쓴다. flags1의 frame20/QA1000/QB2000/내용물20000, flags2의 bridge4/level4000/ownerMask는 필드마다 현재 getter를 호출한다. Battle·Owner 마스크는 해당 getter의 효과 뒤 읽도록 문장 순서를 고정했다. version의 `>0`, `>=2`는 signed 비교다. 버전0의 Has→Level, 버전2 이상의 가상80, owner 진단707 뒤의 현재 owner 재조회와 내용물 저장을 보존한다.

Session은 표시용 `World.Objects()`를 저장 입력으로 사용하지 않는다. 같은 Pool/Hash/TYPE의 `RawSquidFinder`, `RawBuildingSpace::HasIsland`, `RawSquidLevel`, 제단·다이스의 실제 Count, 벌룬 몸체, 같은 내용물 저장을 사용한다. `FortObjectSettings()`는 새 복사본이 아니라 기존 로더가 가진 `FortObjectLoadState`를 제공한다. Battle의 Mission은 `00594fa0`의 같은 `core.bridgeState.mission`이며 `00594fa4`의 missionPost와 혼동하지 않는다. 새 battleSavedMask는 `0054263c`를 나타내며 값이 같은 fortKeepMask `00542640`와 별도다.

가상 분배는 지원 생성자의 실제 최종83개 VT를 정적 감사했다. +60은 공통72개·항상0인9개·벌룬2개, +80은 공통81개·제단/다이스2개다. `SquidFactory::SupportsConstructedVtable`는 기존 생성자 레시피를 재사용하며83개 목록을 복제하지 않는다. 현재 VT를 선택하고 TYPE와 생성자 VT가 반드시 일치해야 한다는 새 조건은 넣지 않는다. 미등록 VT는 성공값으로 대체하지 않는다. 항상0인 VT는5070c0/508cc8/5092d0/5096a0/509758/509810/50a8e8/50db78/50ee08이다. 벌룬5040c0/504198은 signed frame DWORD를 먼저 캡처한 뒤 현재 TYPE의 frame 수와 비교한다. 제단5076b8/다이스507950은 기존 실제 의식 Count를 사용한다. 공통 Has의 OR 조건이 정상 TYPE에서 항상 참인 원본 동작도 유지한다.

근거는 `Netstorm.c`의 SaveObject151206, flags getter136747/136761, Has137580, 벌룬 Robust34219, 제단·다이스 Count58078/60141행과 실제 VT/생성자 레시피다. 이83개 분배의 근거는 정적 감사이며 이번 외부 장치 oracle 전체를83개 원본 가상 몸체 실행으로 잘못 설명하지 않는다.

## 청크와 쓰기 수명

청크 인덱스의 `int<<4`는 DWORD로 감긴 뒤 signed→float32로 저장한다. Begin의 양끝은 감긴 시작과+15이고 flags0이다. 실제 Begin 뒤 marker99를 기존 `WriteMemStream`으로 쓰고 countWORD 자리를 예약한다. 예약 시0을 쓰지 않는다. 따라서 이후 중단되면 기존 두 BYTE가 그대로 남는다. count는 최초 data+offset에 고정하며 후보의 현재 SID와 poolLimit을 매번 확인한다. 각 SaveObject의 전체 DWORD 반환값을16비트로 누적하고 Next를 실행한다. 정상 탈출 때만 고정 주소에 countWORD를 쓰며, 도중 변경된 stream cursor/data로 위치를 재계산하지 않는다.

`RawCampaignSession::WriteFortObjectChunk`는 같은 저장소에서 발급한 참조를 받는다. 호출자가 판본/범위를 정하고 버전 BYTE 및 부모 섹션을 준비한다. 함수는 한 청크만 쓰며 Temp 노드·전체 Terr/Chaff·Notify·파일 이름 변경·UI 상태 전이를 앞당기지 않는다. 기술과 객체 내용물은 `Impl::ContentWriter`의 같은 Form6 연결을 공유하며, 반환 hooks는 stream을 값으로 보관한다.

최종 PE 감사에서 float Finder 래퍼0041def0의 상단 변환을 확인해 연결을 보정했다. lower는0041d770의 절삭→signed DWORD→float32 저장, upper는0041d7a0의 `double(v)+float bias0.9999899864196777`→절삭→signed DWORD→float32 저장을 거친다. 그 뒤 네 float를 다시 CRT 절삭해 정수 Begin에 넘긴다. 기존 `RawCursorPosition::TruncatePoint`와 `RawCarrierDropPoint::Snap`을 재사용하며 각각 독립 원본623/256입력 검사에 포함되어 있다. 정상 청크0..15에서는 직접 절삭과 같지만 chunkX=-1의 상단-1은0이 되어, -1을 전체 보드 sentinel로 잘못 넘기는 차이를 없앴다. 큰 음수에서도 중간 float32 재저장을 생략하지 않는다. 이번 청크에서 만들어지는 정수 float 범위에서는 두 x87 정밀도의 최종 변환이 같다.

## 독립 관측과 검사

`tools/decomp_fort_object_save.py`는 제품 C++를 읽지 않고 보호 PE의 전체 두 본체와 실제 CRT를 Unicorn으로 실행한다. 단일 객체의 하위 flags/가상/섬/Write/내용물, 청크의 Begin/Next/SaveObject만 입력에 명시한 장치다. 청크 marker는 실제 MemStream 명령을 실행하며 순수 진입 관측 `write:1:63`만 남긴다. 대상 본체의 분기·좌표 변환·count 예약/최종 쓰기는 실행 자체다.

객체 **1647입력·두 제어어3294행**, 청크 **336입력·두 제어어672행**, 합계 **3966관측**이다. x87 제어어027f/037f 차이는0이다. 본체3,595,564명령에 실제 CRT188,206명령이 포함되며 정상3950행의 stackdelta0/보존 레지스터/x87 TOP를 확인했다. 명시 중단16행도 부분 쓰기와 현재 상태를 기록했다. 반환1인 후보65536개를 실제 반복한 두 실행은 `630000`과 pos=end3으로 countWORD 감김을 확인한다. 보고서 `extracted/cpp-fort-object-save-20261023-native.json`은 PE/도구/두TSV SHA 및 출력 버퍼7932개의 SHA를 담는다.

`FortObjectSaveTests`의 두 검사는 각각 앞25열/10열만 외부 입력으로 쓴다. 정상 객체 반환·현재 raw50BYTE·stream4DWORD·두 버퍼·현재 전역10개/장치6개·호출 순서와 부분 중단을 원문 출력과 대조한다. 중단 EAX와 void 청크의 EAX는 의미 반환값으로 비교하지 않는다. C++에서 내부 함수 진입을 관찰하는 별도 게임 hook을 만들지 않았으므로 청크 trace에서 실제 marker 진입 관측 하나만 제거한다. 나머지 호출, 용량 진단, marker를 포함한 전체 출력, 미초기화 count와 고정 쓰기는 모두 비교한다.

별도 검사는 실제 생성자 VT 범위/미등록 생성자 조회의 예외와 풀 불변, 벌룬 signed frame·현재 타입·getter의 frame 변경 뒤 최초 값 보존을 확인한다. 실제 Session 검사는 TheWarBegins를 읽어 같은 현재 사제의 owner BYTE를 두 번 바꾼다. 실제 섬 판정에 맞는 Scope1/Terr00 또는 Scope2/Chaff에서 원본 Finder로 선택한 한 청크를 저장하고 실제 파일 저장 부모·재파싱으로 현재 위치/타입/owner와 두 파일의 차이를 확인한다. 작업본은 `cpppj/build/test-data`에만 생성한다. 표시 객체 목록으로 저장 순서를 대신하지 않는다. 다른 지역은 기존 파일에 남으므로 전체 지도 snapshot/사용자 Editor 저장 완료를 주장하지 않는다.

첫 Release 빌드1은 경고/오류 없이 종료0이다. 독립3966행·83VT·벌룬의4검사는 통과했지만 실제 월드 검사에서 saveFrame 플래그/복수 프레임을 동시에 갖춘 대상이 없어 선택 실패했다. 원본 writer를 바꾸지 않고 현재 미션의 실제 사제 owner를 입력으로 사용하는 검사로 수정했다. 최종 감사에서 찾은 Finder upper 변환도 기존 실제 좌표 몸체에 연결하고 빌드2를 진행한다. 최종 Release 빌드2는 종료0·경고/오류0이며, 새5검사(원본3966행/현재 사제의 파일 저장·재읽기/83VT/벌룬), 기존 내용물3검사, 좌표3검사 합계11검사가 실패check0이다. 이 검증 뒤 제품/검사 C++는 동결했다. 최종 전체 CTest1은 **2,369검사·실패check0·종료0/195.16초**이며 LastTest.log 내부2369/0도 확인했다. 로그는 `extracted/cpp-fort-object-save-20261023-*`다. UTF-8/한국어 주석·원본/도구/두 TSV SHA·diff/보호 경로를 확인하고 인수인계를 마감해 **20261023-33 CP**로 로컬 커밋한다. 검증 뒤 제품/검사 C++ 수정과 원격 push는 없다. 별도 dotnet 세션의 파일은 포함하지 않는다.

## 다음 연결과 게임 UI

전체 Objects 부모는 Terr00..19/Chaff, 정확한 이름 `Temp`의 동적 노드·제거/rename, `005c89c0`의 Storage/Session 수명 공유가 남는다. Money의 공용 난수, 실제 Mission/Badges와 마지막 Notify·BindFileSave/빈 filename, generic Editor 설치·State1/6·새 맵·시나리오도 후속이다. 사용자 전체 Early Missions·캠페인·데모3 완주와 원본 동일성, Multiplayer 외 메인 메뉴·GUI·음향·배포 검증 목표는 계속 미완료다.

요청에 명시된 진행 중 UI 네 요소를 읽기 전용 감사했다. 좌측 덱은 `UberGump::Compose→EnableProduction→공유 Gump Idle/Draw/Event`에 연결되어 있으나 원본 드래그/지도 영역 배치와 사용자 조작 검증이 더 필요하다. QWASZX는 키를 생산 명령 큐에 넣는 경계와 그 명령을 공통 Gump 트리에 전달하는 경계가 둘 다 없다. 이번 단위 커밋 뒤 **QWASZX 두 경계를 Temp 지원보다 우선** 복원한다.

원본004d62b0의 매핑은 Q/W/A/S/Z/X→slot0/1/2/3/4/5, Y→4이며 대소문자 모두 해당한다. E는0..5를 순서대로 예약한다. 공통004d9bb3에서00542ac4의 명령020000f4와 slot,0을452fe0에 예약한다. Guide463e40→Gump466200이 먼저 소비하고 release는 그 뒤 거부한다. Paused460de0은 키 예약을 막지만 이미 예약된f4 소비보다 뒤에 있다. 별도 modifier/활성 커서 거부는 없다. 현재 하위 `RawProductionControls::Handle`은 flags40/slot의 실제 현재 Form을 찾아 Cursor.Take와 getPiece 음향까지 구현되어 있다. 그러나 `UserInput::Event`의 매핑과 `UberGump::Event`의 비마우스 조기 반환을 고쳐야 도달한다.

좌측 하단 미니맵은 dirty 큐만 있으며 제품 생성·Draw·입력·Refresh가 빠져 있다. 원본 생성4801b0/표시480810/Draw480660/이동4803f0/드래그Idle480440/셀그림4808d0/프레임Refresh481320이 후속 근거다. ESC는 현재 가운데 Leave Battle 한 행이며 원본4cc210의 Game·조건부Edit·View·Options·조건부Players·About 막대 구성과 다르다. 덱·미니맵·단축키·ESC 모두 실제 프레임에서 배치·소비·카메라·팝업까지 검증한 뒤 완료로 계산한다.
