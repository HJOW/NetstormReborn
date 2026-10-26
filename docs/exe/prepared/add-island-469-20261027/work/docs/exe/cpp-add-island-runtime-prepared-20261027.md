# Add Island 469 준비본과 실제 연결 경계

2026-10-27, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 현재 PC와 일치한다. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 원본 PE/정밀 export/기존 fixture/제품 src·tests·CMake를 수정하지 않았다. 빌드·C++ 검사·실제 GUI·게임 실행도 하지 않았다. 이 문서는 `extracted/cpp-add-island-469-staging/work`에 있는 **미적용 초안**의 범위다. 원본 10.72 비교는 기존 CanPlace/Builder 증거만 재사용하며 이번 GUI·population 관측은10.78이다.

## 완료한 준비와 아직 안 된 부분

- 기존468 준비본의 RawIslandAdd/RawIslandPopulation과118행 Native fixture를 그대로 옮기고 재관측 `--verify`로 바이트 일치를 확인했다.
- 기존 PopulationTests 초안의 입력은469 사본에서 Native와 같은 물리 Entries[y*16+x]로 정정했다. 원래 At(x,y)는 실제x*16+y이므로 입력을 전치하고 있었다. 파괴콜백도 Native와 같은 물리offset에 맞췄고 원본 기대 fixture/468 파일은 보존했다.
- 현재468 Source3 제품 여섯 파일의 SHA를 base-manifest에 고정했다. 이 중 Session.h/.cpp·GameMenuCommands.cpp·UberGump.cpp 네 파일만 staging에서 수정했다. RawMenuControls.h/.cpp는 참조 기준이며 변경하지 않았다.
- 실제 Session의 같은 Pool·Hash·spot·ChunkMap·IslandRuntime·공유 난수·현재 TYPE·owner를 사용하는 지도/21행 seed/드롭 접합 초안을 작성했다. 전체 지형·population·Graph(false)·Territory 저장까지 실제 기존 몸체를 부른다.
- 실제0047fb90 입력 부모의 C++ 초안과 독립 PE 관측40회, 새 비교 검사 초안을 작성했다.
- **전용 창 전체는 미완료다.** 메뉴02000022의 release/dispatch는 고정했으나 실제00480090→0047f450 생성·MetaImageGump·전용 목록·bitmap·Idle/수명 조립은 아직 필수 실패 경계다. staging Event는 이 경계를 명시적으로 중단하며 성공 창이나 일반 ImageGump로 대체하지 않는다. 이 패치를 적용해도 사용자가 Add Island GUI를 끝까지 진행할 수 있다고 선언할 수 없다.

## 원본 부모와 정확한 입력

실제 명령은00542780의02000022이다. `extracted/refined/originals/Netstorm.c`의004d7bab 부근은 공통 release 관문 뒤00480090을 호출하고, 실제 root00563e38에 virtual+20(0,0,root,2)로 Attach한 뒤00466600(this,1)을 수행한다. 이 경로에 NewFort는 없다. 따라서 기존 Editor 지도의 Add Island를 NewFort 복원 뒤로 미룰 필요는 없다.

00480090은0xd98을 할당하고0047f450(432,412)로 생성한다.0047f450은 전용 meta image·목록0047ccb0/0047f360·돌 버튼/Help PlacingIslands를 만들고 실제 단일표시 refCount00593154를 쓴다. 창+d00의21×6BYTE는0으로 시작하며 기존 Territory120BYTE를 이 배열에 복사하지 않는다. 전용 preview는14×14이고 drag+d88은 별도6BYTE다.

0047fb90(`Netstorm.c:103485` 시작, 기계어0047fb90~0048008b)의 규칙은 다음과 같다.

1. 코드&03ffffff.0200002c/2f는 first와 현재 닫기 버튼 serial+2c가 일치하면 parent Kill하고 두 경우 모두1 반환한다.
2.02000004의 second 비영이면 ItemImage(first)를 읽는다. image가 없으면1만 반환한다. 있으면 MetaImageGump를 만들고 **현재 List.selection의 원문 ID**로21행 record를 선택한다. first를 record index로 직접 쓰지 않는다.
3. 시작 Attach와 드롭의 기준점은 **event.x−drag.width/2, event.y−drag.height/2를 x87로 계산해 __ftol한 값**이다. 드롭에서는 여기에 현재 mapBounds.left/top을 빼고 signed /16(0 방향 절삭)한다. 디컴파일의 생략된 x87 인자를 별도 커서 전역으로 해석했던 이전 준비 설명은 이 문서로 정정한다. 확정 명령은0047fcd0 부근/0047fe46 FILD(event.x), FLD(drag+3c), FMUL00500ee4, FSUBP, CALL004e49c0 및 y의 같은 흐름이다.
4. drag Attach 뒤 depth−100, parent flags|100→Push→flags|200이다. 시작의 parent Bounds 조회도 생략하지 않았다.
5. 왼쪽 버튼00010000의 release가 아니고 drag가 있을 때만 배치/취소한다. 목록 경계 안이면 flags&~300→Kill drag→drag0→selection−1→virtual8(0),1 반환. ESC를 별도 취소 사건으로 발명하지 않았다.
6. mapBounds 밖은0/drag 유지. 기준 청크가0..13 밖이면1/drag 유지. 범위 안이면 record[2]=(y<<4)|x를 먼저 쓰고 CanPlace를 실행한다. 충돌 false는0이며 **새 좌표 record와 drag가 남는다.**
7. 성공이면 record[1]&fd→Builder.SetMap(world)의 기본 bounds(1,1,15,15)/owner1→현재 IslandCount→0046e0d0이다. templateOwner를 실제 배치 owner로 계속 쓰지 않는다.
8. 성공 뒤 카메라는 drop 청크 중심이 아니라 미리보기 전체 rect(0,0,14,14)를16배한 중심, 즉112,112이다. 실제0040d900→0041cc20→00433aa0 관측을 그대로 보존한다.
9. flags&~300→Kill drag→drag0→**현재 selection 재조회**→그 image0046d8a0/free→현재 selection 재조회/행 제거→selection−1/update0→dirty1→지도 revision005949d4++이다. 실패한 World 하위 앞의 부분 효과를 rollback하지 않는다.

## Session 접합과 원본 미리보기의 제한

`RawCampaignEditorIsland.inc`는 새 별도 Session을 만들지 않는다. 기존 loaded·ExistingEditor·editor·Fort·비Battle 관문을 모두 요구하며 State1 설치 중 제한 Scope를 열지 않는다. BuildEditorIslandMap은 현재 templateOwner와 헤더 첫DWORD를 조회하고0이면 false다. 실제 현재 ReadTerritory→독립14×14 지도→기존 RawIslandBuilder.PlaceAll(0,20)만 수행한다. 같은 world 지도 주소를 preview로 주면 거부한다. BuildEditorIslandChoiceMap은 owner0/ignorePosition=true의 단일 Place다.

SeedEditorIslandChoices는 동일 현재 헤더 관문 뒤21행의 현재 flag&1이0인 행만 기존 SeedFortTerritoryRecord를 호출한다. 이 부모의 Next(254)+1은 Client 공유 GameRandom을 소비한다. 이미 활성인 행의 seed는 바꾸지 않는다. GUI의 dirty Refresh에서 map image/이전 image 반납/행 seed/개별 image/목록 순서로 이 API를 배치해야 한다. API 두 개를 임의로 먼저 호출하고 GUI Refresh 전체와 같다고 할 수 없다.

원문0046e4a0은 로컬14×14 ChunkMap을 만든 뒤0046ddd0의 현재 Territory PlaceAll→0046e410 bitmap→3084BYTE 전체 지도 복사를 수행한다. 이번 지도 API는 **bitmap이 없는 지도 부분**이다. 원본 임시 스택의 비활성 물리 tail까지 초기0으로 관측했다고 주장하지 않는다. ChunkMap의 실제 저장 순서는x×16+y다. 전체 World의 IslandList를 건드리는 Place로 preview를 대체하지 않는다.

DropEditorIsland는 기존 MakeNormalTerrainFarmWorldHooks·RawFortRandomPlacement를 현재 실제 pipeline/factory/HP/월드 가상 공간에 연결한다. 기존 미연결 `finish` 객체의 실패 훅을 성공으로 바꾸지 않고, 같은 Pool/현재 FinishOwnerState에 실제 단일배치 훅을 가진 짧은 수명의 RawFortFinishOwner를 별도로 조립한다. 이 객체는 원본 PlaceRandom 몸체를 공유하고 별도 월드를 소유하지 않는다.

이어서 RawIslandPopulation의 residence·초기 객체 교체를 실제 Center→Destroy→Create→현재 TYPE 기본frame/HP→현재 지역 owner 가상분배→Pop에 연결한다. boss는 같은 Client Authority, Fort는 같은 OwnerMode bool 참조다. residence/primary/fallback/replacement는 기존 실제 Session의 residenceType/tutorialArcherType/templeProduct/rewardState.types.priest를 참조한다. 기존 Session 내부의 다른 용도 TYPE 별칭 전체를 이번에 통합한 것은 아니며 새 상수 사본을 추가하지 않았다. 실제 production refresh는 기존 Client ownership.refreshProduction을 쓴다. 원본 실제 시간 seed는 services.sound.wallSeconds이며 고정 시각으로 바꾸지 않는다.

마지막 실제 정상 지형 Generate→population→core.graph.Rebuild(false)→WriteFortTerritory의 순서다. 전체 SaveCurrentMap/파일 Notify를 앞당기지 않는다. 원본0046e0d0은 새 섬의 레코드로 현재 전체 Territory를 snapshot할 뿐이며 FullFort 저장과 구별한다. 성공한 뒤 GUI dirty Refresh는 별도다.

## 독립 원본 증거

보호 PE SHA256은`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 새 `tools/decomp_editor_island_drag.py`는 실제0047fb90, Contains004239c0, __ftol004e49c0, Builder0046d9f0/46da20/431fc0, Count0046ee50, owner004bd300, rect16배0040d900, 중심0041cc20을 실행했다. 실제 함수별 CODE SHA와 진입 횟수는 새 JSON에 기록했다. GUI Bounds/목록/MetaImage 생성·가상 Attach/Push/Kill/이미지 반납과 큰 World Add는 명시장치다. CanPlace는 이 부모 관측에서 명시 true/false이며 실제 몸체의 기존 독립fixture를 변경하지 않았다.

- 새 fixture `editor-island-drag-x86.tsv`:20고유 입력×FPCW027f/037f=40회, 최대398명령, 모든 호출 정상 ret12 및 x87 control/stack 보존. timeout500000µs/상한200000명령 안에 완료했다.
- SHA256:`883f1a6c1af9e79af2b6485beee69859a96be10d76c4cb7f0fc81825195038d1`.
- native1.log 생성 성공, native2-verify.log 바이트 재관측 성공. 이번 신규 관측의 실패는 없었다.
- TSV입력 열은 name/code/x/y/dragIn/canPlace/imageIn/width/height/control, 출력은 result/flags/dirty/dragOut/source/record/revision/trace/instructions다. dragOut은 Python bool True/False이며 주소 일치를 주장하지 않는다. source는+d90 값이고 selection 자체가 아니다. trace의 add는 실제 입력record6·기존IslandCount4·resetBuilder(1,1,15,15,1)를 기록한다.
- 기존 population118행 SHA는`f63a926ccb280a96078a97dc8f025f9fcae9c22fffe947bf9f53cc27006e0f13` 그대로다. population-native1-verify.log에서59고유/118관측 재현했다. 초기468 원시는 해당 staging에 계속 남아 있다.

## 제품 적용 뒤 필요한 검사

새 C++ 검사는 **아직 빌드/실행하지 않았다.** EditorIslandDragTests의 Native40행/메뉴release2사례, 기존 PopulationTests2사례, CampaignEditorIslandTests.inc의 실제 Client2사례를 준비했다. 마지막 inc는 기존 CampaignExistingEditorCheckpointTests.cpp의 끝에 include하여 같은 private helper를 쓴다. 기존 본문을 수정하거나 등록을 줄이지 않는다. 실제 성공 사례는 합법 후보를 현재 지도에서 선택한 다음 실제 섬count 증가·새 지역의 Residence/사제·현재 Territory 레코드와 Graph 실행 경로를 검증하는 초안이다. 컴파일/API/자연 진단 실패가 드러나면 새 원시를 보존하고 실제 미연결 child를 복원해야 하며 성공 더미로 통과시킬 수 없다.

root 적용 순서는 기준 SHA/hunk 확인→새 o/client 모듈 CMake 등록→두 새 .cpp 검사와 두 fixture macro 등록→기존 Checkpoint TU끝 inc include→집중 빌드/검사다. root가 source4 이후를 수정했으면 이 준비본 전체파일로 덮지 말고 patch/hunk만 적용한다. README와 manifest에 정확한 등록 목록을 적었다.

실제 창 완료의 다음 필수 범위는0046e410→004c1cc0 bitmap 및 하위004c0390/4c09a0/4c10a0/4c1860, MetaImageGump0047f2a0의 Draw/크기/소멸, 목록0047ccb0/47f360/47d170/47d1c0/47d1f0,0047f450 생성/0047f9b0 Refresh/00480130 Idle/소멸과 공통 Gump 등록이다. 이 자식들이 실제 같은 RawMenuControls 수명/표면에 연결되면 현재 EditorIslandDrag의 hooks를 그 실제 장치로 바인딩한다. 그때 기존 맵 Editor에서 메뉴→실제 커서 preview→거부/성공 drop→목록 위 취소→닫기→SaveGoMain/재진입을 GUI로 검증한다.
