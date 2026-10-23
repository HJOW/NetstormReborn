# 원시 Fort Contents 읽기004bd130

2026-10-24 신규 독립 복원이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 기존 공유 소스·CMake·LEFT·공유 build·GUI·git은 변경하지 않았다. 보호 PE의 최초 관측을 먼저 고정하고 신규 제품 본문과 비교했다.

## 원문 순서와 현재 상태

각 재귀 진입에서 같은 stream의 현재 data/end가 모두 비영인지 검사한다. 부모 raw 주소는 그때의 SID×50+PoolBase로 고정한다. count BYTE를 읽고 pos를 증가한 뒤 각 항목의 type BYTE를 읽는다. count255도255항목이며 직접 BYTE 읽기는 논리 end로 제한하지 않는다.

type<246에서만 현재 conversion을 읽는다. 비영이면 relaxed0/checking비영/변환BYTE0 진단 `Template.cpp:0x79` 뒤 **현재 conversion을 재조회**해 변환한다. 진단이 pointer를0으로 바꾸면 원문은0주소+type의 BYTE 접근에 실제 실패한다. 이 경우를 정상 false나 무변환 성공으로 바꾸지 않는다. 변환 뒤 type0은 앞선 소비/생성을 보존한 채 즉시 false다.

첫 TYPE 조회의 `flags1 +0xe8`을 QA/QB/container용으로 캡처한다. QA는 BYTE, QB는 signed WORD이며 둘 다 있으면 QB가 수량을 덮는다. QB는 실제 WORD 읽기→현재 end 캡처→pos+=2→signed pos>end 진단 `memstream:0x2e` 순서다. 진단 이후에도 이미 읽은 WORD를 유지한다.

두 번째 TYPE 조회의 `contentListFlags +0x100`이0이면 **진입 때 고정한 부모 raw 주소**에004ac550을 호출한다. 이 helper는 그 주소의 **현재 +10 BYTE 실제 타입**을 읽어 TYPE를 조회하고 `containerListFlags +0xfc`를 반환한다. 저장된 contained.type WORD나 파일의 이전 타입으로 대체하지 않는다. 이것도0이면 listFlags BYTE를 직접 읽는다.

listFlags의0x100을 제거하고,0x10이면 해당 호출의 attachFlags를 `(flags&~8)|0x10`으로 바꾼다. 같은 부모의 다음 형제도 이 flags를 이어받는다. 즉시 실제004afa70에 **parent/type/quantity/listFlags/attachFlags/quality0**을 전달하고 첫 flags1의container0x20000이면 반환 SID로 같은 stream에 재귀한다. 자식의 false는 전파하지만 자식이 바꾼 지역 attachFlags를 부모 형제에 역전파하지 않는다. 반환SID0도 이 reader에서 조용히 제거하지 않는다.

이422BYTE 본문은 loadingDepth를 쓰지 않는다. 현재 depth의 변경은 외부 장치의 명시 입력이며 TypeNames/Technology/Deck 부모가 각자 자기 깊이를 소유한다. 중단/물리 실패에서도 앞선 cursor/생성을 되돌리지 않는다.

## 모듈과 빌림

[RawFortContentsLoad.h](../../cpppj/src/o/RawFortContentsLoad.h)와 [본문](../../cpppj/src/o/RawFortContentsLoad.cpp)의 `LoadFortContents(parent,stream,flags,relaxed,globals,hooks)`는 현재 conversion 참조와 checking 순수 getter를 빌린다. 동기 `access` 한 번 안에서 현재 fields16BYTE/버퍼로 전체 재귀를 실행한다. 원문 호출별 부모/flags/남은count는 별도 프레임으로 유지하며 미리 parsed 목록을 만들지 않는다.

`type`/`parentListFlags`/`create`/진단은 명시 하위 경계다. conversionBuffer와 memory.buffer는 해당 토큰의 실제 물리 할당을 순수 조회한다. 물리 범위/장치 누락은 선택한 위치에서 예외로 구별하며 fields/버퍼를 access 밖으로 탈출시키지 않는다. 이 신규 모듈만으로 실제 ContainedCreate/Pool/Editor 외곽의 전체 동작을 완료했다고 하지 않는다.

## 최초 원문과 격리 검증

[관측 도구](../../tools/decomp_fort_contents_load.py)는 실제 **004bd130..004bd2d5,422BYTE**와 같은 실제 재귀를 실행한다. [fixture](../../cpppj/tests/fixtures/fort-contents-load-x86.tsv)는334입력×2정밀도=**668행**,정상622/명시중단44/실제 null변환접근실패2,514,232명령,정밀도차이0이다. TYPE getter0049a840/부모TYPE getter004ac550/생성004afa70/진단004e0620만 명시 장치다.

현재conversion0/11/22·type0/245/246/254/255·count255·QA/QB/둘다·중첩·형제flags·signed end·count직접읽기·TYPE/부모/Create/진단 뒤 현재 conversion/data/pos/end/flags/depth 변경·부분중단을 포함한다. 최초 fixture SHA256은 `22851fa72e8905ac9465aadb64cb5669695dfd7d2847435fb0dcb25d4048948e`다. [최초 보고서](../../cpppj/fort-contents-load-native.json)는 제품 작성 전 상태를 보존하며 도구 `--verify`로 원본/도구/fixture SHA를 재확인했다.

[FortContentsLoadTests.cpp](../../cpppj/tests/FortContentsLoadTests.cpp)는668행 전체의 반환/중단/fault·현재 전역·두 stream의data/pos/end/capacity·TYPE/부모/Create/진단 trace를 비교한다. 별도 검사에서 빈 입력의 미선택 장치 누락 허용,실제Create 누락 직전pos3 보존,논리end1과별개인 물리pos2048 거부를 확인했다. [격리 CMake](../../extracted/cpp-fort-contents-load-20261024-isolated/CMakeLists.txt)만으로 **Release 경고·오류0,2검사/실패check0**이며 공용 라이브러리도 링크하지 않았다. SHA/로그는 [검증 기록](../../extracted/cpp-fort-contents-load-20261024-isolated/verification.json)에 둔다. 게임·GUI·OS 함수·전체SEH분배기를 실행하지 않았다.

## Session 연결 경계

같은 Storage의 Technology 선택 직후 고정한 `TemplateWriteStream` 생성수명을 사용해야 한다. 뒤 TypeNames 선택이 current를 바꾸어도 Contents는 원래 Technology의 같은 fields/버퍼/pos를 읽어야 한다. 현재 conversion은 Client 단일 `FortTypeConversionStorage`에서 빌리고 실제 `contained->Create`에 위6인자를 전달한다. parsed `FortContent.item.type`이나 저장 폭을 다시 추정한 목록은 이 경로에 사용하지 않는다.

Technology 부모는 원문처럼 Contents의 false 반환을 별도 실패로 승격하지 않고 현재 변환표 해제/UI/깊이 복원을 계속한다. 예외는 앞선 부분효과를 보존하고 기존 부모 cleanup을 따른다. 별도 [후속 연결 제안](cpp-editor-session-children-contents-proposed-20261024.md)은 기존 미완료 patch를 보존한 delta이며,전체Editor 외곽/private Snapshot/State/EditorPlayers/loaded 확정은 root의 별도 조립이다.
