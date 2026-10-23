# 전체 Territory/Chaff 청크 순회 복원 (단위377)

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. AGENTS·공통/닷넷 인수인계를 다시 확인했다. 기존 원본·CD·AGENTS·디컴파일 산출물을 수정하거나 원본/클론 GUI를 실행하지 않았다. 이 문서는 신규 하위 순회 모듈과 독립 검증의 범위다. CMake/실제 Session 연결·공유 빌드·전체 Objects/Editor 저장 완료는 루트 통합 작업과 구별한다.

## 원문과 API

[RawFortRegionSave.h](../../cpppj/src/o/RawFortRegionSave.h), [몸체](../../cpppj/src/o/RawFortRegionSave.cpp)는 보호 10.78 `004be230`(Territory)과 `004be350`(Chaff)을 복원한다. PE 전체 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`다. 정밀 C151564/151638행을 찾은 뒤 실제 PE 명령 순서도 별도로 확인했다. CD 대응은 각각 `00428960`/`00428aa0`이나 이번 실제 실행·검증은 10.78만 대상으로 한다.

```cpp
SaveFortTerritoryChunks(stream, owner, region, sharedScope, hooks);
SaveFortChaffChunks(stream, sharedScope, hooks);
```

`sharedScope`는 `std::uint32_t&`이며 기존 단일 객체 writer가 읽는 같은 `005c89b0`을 빌려야 한다. 종료 시 원래 값으로 되돌리지 않는다. `FortRegionSaveHooks`는 현재 signed DWORD를 반환하는 `chunksX/chunksY`, 실제 청크 조회 효과와 BYTE를 반환하는 `chunkRegion(x,y)`, `allowed(signedByte,owner,region)`의 DWORD 판정, 기존 `chunk(stream,x,y)` writer다. 전체 Objects 부모는 이 두 함수를 Territory/Chaff 하위 경계로 연결하고, 실제 stream/영역 선택·Temp 수명은 부모가 책임진다.

실제 Session 연결 대상으로 확인한 값은 `FortObjectLoadState.chunksX/chunksY/scope`, `chunks.At(x,y).island`와 같은 섬 표 판정, 기존 `WriteFortObjectChunk`다. 이 문서의 격리 검증에서는 그 하위들을 명시 장치로 둔다. 청크 writer와 섬 판정 본체를 이 모듈의 fixture가 함께 실행했다고 주장하지 않는다.

## PE에서 확인한 읽기·쓰기 순서

1. **최초 height를 먼저 읽는다.** Territory `004be233`, Chaff `004be353`의 MOV 다음에 scope1/2를 쓴다. C의 변수 정리만 보고 최초 width를 먼저 읽거나 scope를 최초 조회보다 앞에 쓰면 효과 순서가 바뀐다.
2. 최초 height가0 이하이면 scope만 새 값으로 남기고 반환하며 width/지도/판정/writer는 읽지 않는다.
3. 양수 height 관문 뒤 width를 한 번 읽는다. 각 행 x는0으로 시작하되 행 입구에서 width를 다시 읽지 않는다.
4. 각 셀에서 `00432180`을 실행한다. Territory는 반환 포인터의 BYTE를 `MOVSX`로 signed 확장하여 `0046f200(signedByte,owner,region)`을 실행하고 반환 DWORD가0이 아닌 청크만 `004be020`으로 저장한다. Chaff는 지도 조회 반환값을 사용하지 않지만 조회 효과를 생략하지 않고 모든 셀에서 청크를 저장한다.
5. 각 셀의 판정/저장 뒤 현재 width를 읽고 x를 증가·비교한다. 다음 행 입구는 마지막 셀 끝에서 읽은 **캐시된 width**를 사용한다. 행 사이 width가 바뀌는 callback이 있어도 임의의 추가 getter로 이를 앞당기지 않는다.
6. 각 행 끝에서 현재 height를 읽은 뒤 y를 증가·비교한다. 청크/판정/getter가 크기나 scope를 바꾸면 이후 원본 읽기가 같은 공유 값을 본다. 최초 고정 stream/owner/region 인자를 callback 뒤 다른 값으로 대체하지 않는다.

좌표 증가도 signed overflow의 C++ 미정의 동작 대신 원본 DWORD INC 감김으로 구현했다. 도달하지 않은 외부 장치에는 임의의 기본 성공값이나 가짜 청크를 공급하지 않는다. 반대로 빈 지도에서 미도달 callback을 강제로 요구하지도 않는다.

## 독립 x86 관측

[tools/decomp_fort_region_save.py](../../tools/decomp_fort_region_save.py)는 제품 소스·실행 파일을 읽지 않고 보호 PE의 두 몸체를 Unicorn에서 실행한다. 코드 범위는 **`004be230–004be2b1`(130바이트), `004be350–004be3b8`(105바이트)**다. MOV가 읽은 실제 레지스터 값을 그 다음 명령에서 관측해, getter 외부효과가 이미 캡처한 값을 소급 변경하지 않게 했다. 본체의 모든 분기/반복/공유scope 쓰기는 실제 명령이다. `00432180`/`0046f200`/`004be020` CALL만 명시 외부 입력으로 대체했다. OS·파일 쓰기·게임 진입·GUI는 실행하지 않는다.

- 독립 입력 **930개**, x87 제어어 `027f`/`037f`에서 각각 실행한 **1,860회**다. 정상 RET1,820회와 입력에서 의도한 외부 경계 중단40회를 구분한다. 실제 원문 명령은133,422개다.
- 음수/0/양수 크기, 모든 BYTE0..255, 영역 판정의0/1/최상위비트/전체비트, raw owner/region 인자, getter/셀/판정/청크 callback의 크기·scope 변경 및 부분 중단을 포함한다. Chaff도 모든 BYTE 입력과 조회 효과를 관측한다.
- `y2`에서 width를 줄이거나 늘리는 입력, 최초 width0, 다음행의 캐시된 width가 현재 공유 width와 다른 입력으로 행 입구의 불필요한 getter를 드러낸다.
- 정상 반환의 cdecl 스택·callee-save/x87 제어어·빈 x87 스택, 두 원문 몸체 전체 바이트 불변, 허용 범위 밖 코드/메모리 쓰기 부재도 확인했다.

최초 fixture는 [fort-region-save-x86.tsv](../../cpppj/tests/fixtures/fort-region-save-x86.tsv), SHA-256 **`5d50f51052e7cfcd39697062fdcfd959fa4d8edb3ef951c0e736cba7ddc25d3b`**다. [원문 보고서](../../cpppj/fort-region-save-native.json)에 PE/정밀 C/도구/fixture SHA·호스트·정상 반환·경계 수를 보존했다. `python tools/decomp_fort_region_save.py --verify`는 종료0으로 최초 근거를 확인했다. 기본 생성은 기존 fixture/보고서 덮어쓰기를 거부한다.

## C++ 격리 검증과 통합 경계

[FortRegionSaveTests.cpp](../../cpppj/tests/FortRegionSaveTests.cpp)는 fixture의 앞10열만 장치 입력으로 공급하고 뒤8열의 실제 원본 결과와 대조한다. 모든 getter/하위 순서·signed BYTE 인자·현재 폭/높이/scope·정상/부분 중단이930행 모두 일치했다. 별도 검사로 빈 지도/미도달 장치, 누락 장치의 부분 상태, Chaff가 반환값을 버리면서 조회의 scope/width 변경 효과를 보존하는지도 확인한다.

공유 `cpppj/build`를 사용하지 않고 `extracted/cpp-fort-region-save-20261023-isolated`에 MSVC19.51 x64 `/std:c++20 /utf-8 /W4 /WX /O2`로 새 모듈·검사와 작은 진입점만 컴파일/링크했다. **빌드 종료0/관측 컴파일·링크 경고·오류0**, 격리 실행 **3검사/실패check0·종료0·stderr0바이트**다. 격리 실행 파일 SHA-256은 `18d41e0ca8ba316dbe2cc2674df8966058cd22dfadb6e7ada880dbcda52676a0`이며 빌드/실행 로그·메타데이터를 같은 경로에 보존했다. PowerShell이 컴파일러 배너의 stderr를 `NativeCommandError` 형식으로 감싼 기록은 실제 CL 실패와 구별한다.

루트 통합에서 필요한 fixture 매크로는 **`NETSTORM_FORT_REGION_SAVE_FIXTURE`**이며 경로는 위 TSV다. 추가 파일은 모듈2개/검사1개/fixture/원문 도구·보고서/이 문서다. 이 하위 순회만으로 전체 Objects 부모의 Temp 생성·선택·rename, 실제 저장소/Session 연결, 전체 스냅샷·파일 저장·Editor UI·사용자 완주를 완료 처리하지 않는다. 기존 파일·CMake·LEFT·git 작업은 이 하위 작업에서 수행하지 않았다.
