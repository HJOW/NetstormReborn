# Add Island 469 동결 인수인계

2026-10-27 / HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon /2026-10-23 유지. **제품 미적용·미빌드·C++ 검사 미실행·GUI/게임 미실행** 준비본이다. AGENTS/원본/정밀 export/기존 Native fixture/LEFT/CMake는 수정하지 않았다. root가 이번 현재 작업을 마친 뒤 인수인계·커밋·목표 일시중지를 담당한다. 후속 child에는 착수하지 않았다.

## 산출물과 상태

- `base/`: Source3에서 읽은 Session.h/.cpp·RawMenuControls.h/.cpp·UberGump.cpp·GameMenuCommands.cpp 여섯 파일. base-manifest.json의 SHA와 적용 당시 실제 제품 SHA를 반드시 대조한다.
- `work/`: root 적용 검토용 새 소스/검사/도구/fixture/문서. 기존 제품 네 파일만 바꿨으며 RawMenuControls 두 파일은 기준 사본 그대로다. 전체 폴더를 덮어쓰지 않는다.
- `add-island-469.patch`: base와 달라진 부분 및 새 파일의 git apply 형식 초안. CMake와 기존 Checkpoint 검사 TU는 포함하지 않았다.
- `manifest.json`: 전체 work SHA·baseline 현재 일치 여부·검증/미검증·Native 명령 상한·남은 필수 child.
- `work/docs/exe/cpp-add-island-runtime-prepared-20261027.md`: 주소·순서·Session API·원문 좌표 정정·후속 범위의 상세 근거.

새 o 모듈 RawIslandPopulation/RawIslandAdd는468 staging에서 복사했다. Native fixture118행은 **바이트 그대로**다. PopulationTests 초안의 입력 전치만469 사본에서 고쳤다. Native 도구는 물리 `(y*16+x)`에 명시 입력을 썼지만 C++는 원래 `At(x,y)`를 써서 실제 `(x*16+y)`로 전치되고 있었다. 이제 `Entries[y*16+x]`와 파괴콜백의 같은 물리offset으로 준비한다. Native 기대값이나 원본 기계어를 고친 것이 아니다. 원래468 준비본과 원시는 보존했다.

새 client 모듈 EditorIslandDrag는 실제0047fb90 입력 부모다. 드롭·시작 Attach 기준은 **event x/y−drag 반폭/반높이**이며 별도 커서 전역이 아니다. 실제PE `FILD(event.x)→FLD(drag+3c)→FMUL0.5→FSUBP→__ftol`로 확정했다. 이전 연구 설명의 커서 전역 해석은 폐기한다. 성공 후 카메라는 preview 전체 중심112,112다.

새 Session API 네 개는 `BuildEditorIslandMap`(지도 부분), `SeedEditorIslandChoices`(21행), `BuildEditorIslandChoiceMap`(단일 지도 부분), `DropEditorIsland`(실제World)이다. 기존 loaded/ExistingEditor/Editor/Fort/비Battle guard를 유지하고 State1 prefix Scope를 완화하지 않았다. Drop은 같은 실제 placement/factory/HP/ownership/Pop·normal terrain·population·Graph(false)·현재 Territory snapshot을 사용한다.

**메뉴/GUI 완료가 아니다.** 실제02000022 AddIsland dispatch/release는 준비했지만 UberGump의 staging 분기는00480090/0047f450·MetaImage·목록·Bitmap 미연결 경계에서 명시 중단한다. 새로운 성공 stub이나 가짜 창을 만들지 않았다. NativeNewFort는 기존 Editor Add Island의 선행조건이 아니다.

## 실행한 검증

1. `python -X utf8 work/tools/decomp_editor_island_drag.py` → `native1.log`:20고유/40Native 관측 정상반환, 최대398명령.
2. 같은 명령 `--verify` → `native2-verify.log`: 새 fixture40행 바이트 일치.
3. `python -X utf8 work/tools/decomp_island_population.py --verify` → `population-native1-verify.log`: 기존59고유/118Native 관측 바이트 일치.
4. `git apply --check --ignore-space-change add-island-469.patch` → `patch-check1.log`: 종료0. 읽기 전용 적용 검사이며 실제 제품에는 적용하지 않았다.

새 fixture SHA256:`883f1a6c1af9e79af2b6485beee69859a96be10d76c4cb7f0fc81825195038d1`. 기존 population fixture:`f63a926ccb280a96078a97dc8f025f9fcae9c22fffe947bf9f53cc27006e0f13`. 새 Native는 실제0047fb90 및 helper9개의 CODE SHA·진입 횟수·보호PE SHA를 JSON에 기록했다. FPCW027f/037f·정상ret12·x87 stack/control·CODE 불변을 검사했다. 큰GUI/World/CanPlace 결과는 명시장치이며 제품GUI 전체 동일성을 뜻하지 않는다. 이번 새 Native 관측 실패는 없었다. C++ 코드와 검사 초안은 빌드하지 않았다.

## 재개 절차

1. AGENTS/LEFT를 다시 읽고 현재 PC/마지막 디컴파일 PC를 확인한다. actual 제품 네 파일의 현재 SHA가 base와 같지 않으면 hunk만 재검토해 현재 source4 이후 변경을 보존한다. protected 원본/기존 fixture는 유지한다.
2. patch를 검토한 뒤 root가 적용한다. `cpppj/src/o/CMakeLists.txt`의 netstorm target에 `RawIslandPopulation.cpp/.h`, `RawIslandAdd.cpp/.h`를 등록한다. client target에는 `EditorIslandDialog.cpp/.h`, `EditorIslandDrag.cpp/.h`를 등록한다. `RawCampaignEditorIsland.inc`는 Session.cpp에서 include하므로 별도 컴파일하지 않는다.
3. tests target에 `IslandPopulationTests.cpp`, `EditorIslandDragTests.cpp`를 등록하고 각각 `NETSTORM_ISLAND_POPULATION_FIXTURE`, `NETSTORM_EDITOR_ISLAND_DRAG_FIXTURE`를 새 fixture 절대경로로 정의한다. `CampaignExistingEditorCheckpointTests.cpp` 끝에 `#include "CampaignEditorIslandTests.inc"`를 한 번 추가한다. 기존 검사 본문/등록/기대값을 삭제하지 않는다.
4. 먼저 빌드와 새6사례 집중 검사를 실행한다. 여기에는 Native118/40행 재생 각1사례·부분배치 실패1·메뉴dispatch1·실제 Client2가 포함된다. 실패하면 새 원시를 보존하고 원문과 actual child를 복원한다. 실제 Client 성공검사는 아직 통과 근거가 없으며 컴파일/API/자연 child 경계를 발견할 수 있다.
5. 실제 GUI를 하려면 **다음 필수 child**를 먼저 완료한다:0046e410→004c1cc0 bitmap/그 하위,0047f2a0 MetaImageGump Draw/크기/소멸,0047ccb0/47f360 등 실제 목록,0047f450/47f9b0/480130 생성/Refresh/Idle/소멸·단일창 refcount·공통 Gump 등록. 기존 ImageGump/미니맵 대체나 callback 성공 반환으로 완료 처리하지 않는다. 현재사용자중지요청에따라이번에는이범위를착수하지않았다.
6. 필수 child를 같은 RawMenuControls actual storage/lifetime/surface에 바인딩한 뒤 메뉴→실제 preview→거부/성공 drop→목록 위 취소→닫기→저장/재진입을 확인한다. 현재 Event의 명시 중단을 이 실제 창 factory로 교체한다. LEFT 갱신/단위 커밋은 root가 한다.
