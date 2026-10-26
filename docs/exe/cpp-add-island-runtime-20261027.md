# 섬 추가의 지도·입력·실제 배치 연결 — 단위473

보관된 Add Island 준비본을 현재 제품의 변경점과 대조하여 실제 Session에 채택했다. 전용 창의 목록 이미지 생성과 공통 Gump 조립은 후속이며, 이번 범위는 미리보기 지도·21행 후보·드래그 입력 부모와 실제 월드 배치다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행일2026-10-23**이며, 디컴파일 산출물은 변경하지 않았다.

## 제품 연결

`RawCampaignEditorIsland.inc`는 설치된 같은 Editor의 Pool·Hash·spot·ChunkMap·IslandRuntime·게임 난수·현재 TYPE를 빌린다. 기존 설치 완료/Editor/Fort/비Battle 관문과 State1 한정 UI 범위를 유지한다. `BuildEditorIslandMap`과 `BuildEditorIslandChoiceMap`은 별도14×14 지도에 기존 Builder를 실행하고 실제 World의 섬 목록을 바꾸지 않는다. Bitmap 생성까지 완료한 API와 구별한다.

`SeedEditorIslandChoices`는 저장용 Territory20행과 별개인 창 전용21행에서 비활성 레코드만 같은 Client GameRandom으로 채운다. GUI Refresh 전체의 이미지 반납·생성·목록 순서를 이 API 호출만으로 완료 처리하지 않는다.

`DropEditorIsland`는 실제 CanPlace→record flag 정리→기본 bounds/owner1→World 배치→정상 지형→초기 거주 구역/객체 교체→Graph(false)→현재 Territory snapshot을 연결한다. 기존 단일 배치·HP·소유자·Pop 몸체를 사용한다. 중간 하위가 실패하면 앞선 월드 변경을 보존하고 예외를 전달한다. 전체 Fort 파일 저장/Notify는 이 드롭 부모의 범위가 아니므로 앞당기지 않는다.

초기 객체의 단일 배치 검사는 `MayPlaceGameplay`를 호출해 현재 Editor/Player/동맹/섬·Graph를 읽는다. Session 생성 시 고정한 지형 생성용 pipeline의 상태를 편집기 현재값으로 간주하지 않는다. 기존 실제 전체 형상·지형·충돌·권한 검사에 현재 전역을 공급하는 연결이며, Editor에서 판정을 임의 성공으로 반환하지 않는다.

`RawIslandPopulation`은 원문 `0046df80/0046e000/0040e2b0`의 실제 record 조회·지역 bounds·로컬 난수·거주 구역 배치·중앙 객체 교체·frame/HP 직접 쓰기·현재 소유자·Pop·record bit8/생산 통지를 복원한다. 기존 `RawFortFinishOwner`와 실제 World의 단일 배치 훅을 조립하며 별도 월드를 만들지 않는다.

`EditorIslandDrag`는 원문 `0047fb90`을 재생한다. 좌표는 event.x/y에서 현재 이미지 반크기를 뺀 __ftol 결과이며 드롭 시 mapBounds를 빼고 signed /16한다. 충돌 거부 시 새 record 좌표와 drag를 유지하고, 목록 위 클릭은 취소한다. 성공 뒤 카메라는 미리보기 전체 중심112,112로 이동한다. 목록 선택 재조회·이미지 반납·행 제거·dirty/revision 순서도 유지한다.

`AddIsland` 스크립트와 실제 메뉴 명령22를 연결했다. release는 소비하고 press는 실제 전용 창 factory를 요구한다. 호스트의 생성 지점은 아직 `00480090→0047f450`/MetaImage/목록/Bitmap 필수 미연결을 명시한다. 일반 ImageGump나 빈 성공 창으로 대체하지 않았다.

## 증거와 후속 범위

새 파일은 [준비본](prepared/add-island-469-20261027/README.md)의 manifest SHA를 확인하고 채택했다. 기존 네 제품 파일은 해당 hunk만 반영해 단위469~472의 변경을 보존했다. 준비본142파일은 덮어쓰지 않았다.

원본 CPU의 드래그40행과 population118행을 `--verify`로 다시 실행하여 보관 fixture158행의 바이트 일치를 확인했다. 기존 기대 자료와 원본 기계어는 변경하지 않았다. 처음 두 도구 실행은 staging 전용 루트 탐색 때문에 StopIteration으로 멈췄다. 새 제품 사본의 탐색에 자신의 부모 디렉터리를 포함해 수정했고 첫 실패 로그도 보존했다. 당시 Native 보고서의 sourceSha는 준비본 도구를 뜻하며, 이번 수정 도구 SHA와 재검증 로그는 adoption473으로 별도 기록한다.

빌드·최종 C++ 실행 결과는 [보고서](../../cpppj/add-island-runtime-20261027.json)와 아래 최종 결과에 기록한다. 원시 로그는 `extracted/cpp-add-island-473/`에 둔다. 독립 원본 관측의 큰 GUI/World/CanPlace 결과는 명시 장치이며, 실제 Session 배치 검사와 구별한다.

최초 C++ 실행에서는 보관 검사 초안의 Pool16000이 현재 제품의 고정 예측 영역23001을 수용하지 못했고, 신규 Session 검사가 helper가 공급한 월드 물리 대신 초기화하지 않은 Client getter를 조회했다. 새 검사에서 유효 Pool24000과 실제 공급된 같은 surfaceStorage를 사용하도록 정정했다. 기존 Pool 제약/설치 관문/기존 검사 기대값은 바꾸지 않았으며 첫 실패 원시를 보존했다.

두 번째 실행에서는 Native 장치가 고정한SID15001/15002를 준비하려면 실제 서버 첫15000도 먼저 확보해야 함을 확인해 새 입력을 보완했다. 또한 Native trace의 중앙 배치 좌표가 달랐다. 실제 PE의 `00500fc8`은10.0이고 `0040e2bf/0040e307` 등은 이 값을 차감한다. 준비본의4.0 해석을10.0으로 고쳤으며 짧은 변의 음수 대체식과 x의 float32/y의 x87 저장 순서를 보존했다. 기존 Native 기대 자료를 바꾸지 않았다. 원시 상수/명령 기록도 보고서에 연결한다.

세 번째 실행에서는 Native population118행과 배치 중 실패 보존이 통과했으나 실제 Session의 초기 객체 배치가 원문 진단4(MissingPlacement)로 멈췄다. 위의 현재 전역을 읽는 전체 MayPlace 연결로 수정했고 이 최초 진단도 원시 로그에 남겼다.

네 번째 실행은 그 배치를 지나 공통 삭제의 unitLost 장치를 요구했다. 실제 호스트는 기존 `CampaignClientServices`에서 `unitLost.wav` 재생을 공급하며, 새 검사 helper에는 이 장치가 없었다. 신규 검사에 음향 요청 관측을 공급하고 정확한1회 요청을 확인하도록 보완했다. 실제 오디오 재생이나 삭제 본체를 대체하지 않았다.

그 뒤 첫 통과와 달리 반복 실행에서 보상 정수 변환 오류가 나타났다. 신규 검사의 helper는 Native 초기 준비를 생략하고 SP의 미정 할당 비트를 보유하므로, 실제 보상 전에 명시 시작 SP0을 공급했다. 실제 삭제 뒤에는 원문 Fort/Editor 보상 고정 모드194162도 단언한다. 성공/반복 실패 로그를 모두 보존했으며 기존 검사 본문이나 보상 검증을 완화하지 않았다.

## 최종 결과

- Release 빌드6회 모두 종료0·경고/오류0이다. 최종 build6 이후 제품/검사 소스를 변경하지 않았다. 게임 SHA256은 `000c7889fd9858ebb754549706d7774632b3bb65274de51b194c51640c2554fc`, 검사 실행 파일은 `2dbcfcd11093e435dee36b75596b8157fe7523c64d57e756abfecef39f15759d`다.
- 최종 집중 **94고유/94관측·실패0**이다. IslandPopulation1 + IslandAdd1 + EditorIsland4 + ExistingEditor58 + NewFort12 + IslandPlacement2 + FortFinishOwner6 + FortTerritory7 + NormalTerrain3이다. 처음 선택한 FortRandom은 실제 등록 이름에 없어0사례/종료1이었으며, 등록표를 확인해 FortFinishOwner와 NormalTerrain으로 바로잡았다. 이 검사기 선택 실패도 보존했다.
- 같은 최종 제품의 전체 C++ **3,239개·실패0**이다. CTest wrapper 대신 `netstorm_tests.exe` 전체를 직접 실행했다. 기존3,233개 등록을 모두 보존했고 신규6개가 추가됐다. 앞선 메뉴 토글과 신규 Fort 검사도 이 전체 실행에 포함된다.
- 신규 검사는 독립 미리보기의 Pool/섬/공유 난수 불변,21행 seed, 충돌 거부, 실제 섬·거주 구역·사제 생성/Graph/Territory, 공통 삭제의 unitLost1회와 편집 모드 SP194162를 확인한다. 별도 실제 파일 재로드나 GUI/음향 출력 검사까지 확대하지 않는다.
- 보호 원본/CD/AGENTS와 기존 검사·fixture3,161파일은 바이트동일이다. 기존 Checkpoint 검사 파일은 원래116,632BYTE를 온전히 접두부로 보존하고 새 include만 추가했으며, 검사 CMake는 새 파일/fixture 등록만 더했다. 준비본142파일/32,009,649BYTE도 읽기 전용 검증을 다시 통과했다.
- 최종 소스 manifest는2,800파일이며 `extracted/cpp-add-island-473/source-final.json`에 있다. 원시 실행·실패·최종 로그의 SHA는 커밋된 보고서에 기록했다.

실제 게임/GUI는 실행하지 않았다. 다음은 `0046e410→004c1cc0` Bitmap, MetaImageGump, 전용 목록, 생성/Refresh/Idle/소멸·단일 창 참조 수와 공통 트리 등록이다. 이어 새 맵/기존 맵의 실제 미리보기·배치 거부/성공·취소·닫기·저장/재진입을 확인한다. Test Battle/시나리오와 전체 미션 자연 완주·화면/음향 동일성도 계속 남아 있다.

**474 후속:** Bitmap/전체 부모를 실제 연결했다. 기존 섬 배치 검사는 known1 금액0을 명시했어도 수신자0의 미정 SP 비트에서 다시 변환 실패를 보였다. 실제 가산 두 부모/ftol504행을 추가 관측해 SP 가산을 원문대로 고쳤으며, 이 문서의 기존 검사 본문·입력·기대값은 유지했다. [474 검증과 현재 재개점](cpp-island-bitmap-runtime-20261027.md)을 본다.
