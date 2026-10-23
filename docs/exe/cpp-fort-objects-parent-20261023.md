# 전체 Objects 저장 부모004be430 — 단위377

**단위377 루트 통합 갱신:** 공용 CMake·실제 Storage·RawCampaignSession에 등록했고 관련63검사에서 부모714관측 및 실제 두Terr/Chaff의 두차례 파일 저장·별도Session재로드를 통과했다. 전체 CTest/GUI와 남은 Editor 경계는 [현재 실행 기록](cpp-fort-objects-runtime-20261023.md)을 따른다. 아래 작성자 범위/준비 상태는 최초 독립 관측을 인계한 시점의 기록이다.

최초 하위 작업자는 빌드/제품 검사/게임/GUI를 실행하지 않았으며 기존 공용 소스·빌드 디렉터리·LEFT_JOBS·git을 변경하지 않았다. 신규 독립 모듈/도구/fixture/검사/문서만 작성했다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 일치하며 원문 산출물은 변경하지 않았다.

## 실제 원본과 복원 범위

대상은 [10.78 정밀 원문](../../extracted/refined/originals/Netstorm.c)의151702행 `FUN_004be430`, 실제 **471바이트**다. 함수 범위는004be430–004be4ac 및004be4b0–004be609다. 중간3바이트 정렬은 실행 범위에 넣지 않았다. 같은 원문의 CD 대응 후보는00428ba0이지만 이번 복원/관측은10.78만 대상으로 한다.

원본의 실제 호출·쓰기 순서는 다음과 같다.

1. 고정 this의 manager가0이면 Template.cpp957을 진단한다. 현재00540cac와 고정 this 슬롯이 다르면958을 진단한다. 진단 복귀 뒤 현재 값을 계속 사용한다. 소유자 번호는 고정 this에서 계산한 슬롯을1..8만 유지하고 나머지는0으로 clamp한다.
2. 전역005c89c0에 **DWORD2**를 한 번 쓴다. 루프 전체에서 버전이 항상2라고 가정하지 않는다.
3. 영역0..19마다 `Terr%02d`를 현재 manager에서 Find0044ce60한다. 없으면 현재 manager를 다시 읽어 같은 이름을 Create0044d1b0한다.
4. 현재 manager를 다시 읽고 정확한 대소문자의 **Temp**를 Create한다. 반환 stream은 이후 current 변경과 별개로 고정한다. 생성 복귀 뒤 현재 version의 낮은 BYTE를 캡처한다.
5. 원본 인라인 MemStream 쓰기로 그 BYTE를 쓴다. signed `alloc < pos+1`이면 MemStream36을 진단한다. 진단이 version/data/pos/end를 바꾸어도 쓰는 BYTE는 진단 전에 보관한 값이며 주소는 복귀 뒤의 현재 data/pos다. 현재 pos+1을 쓰고 signed 비교로 end를 진행한다.
6. **영역 writer004be230(stream,clampedOwner,region)**을 호출한다. 이 하위는 기존 단일 청크004be020이 아니라 해당 영역의 전체 청크 순회다.
7. 현재 manager를 다시 읽어 **Select0044ceb0(Temp,1)**한다. 선택 결과의 data와end가 모두 비영일 때만 현재 manager를 다시 읽어 **Rename0044d160(destination=TerrNN,source=Temp)**한다. pos/end 관계나 저장 객체 개수를 새 조건으로 추가하지 않는다.
8.20개 영역이 끝난 뒤 현재00594fb8을 딱 한 번 읽는다. 비0이면 현재 manager에서 **Chaff**를 Create하고, 생성 복귀 뒤의 현재 version 낮은 BYTE를 같은 인라인 쓰기로 기록한 뒤 **Chaff writer004be350(stream)**을 호출한다.

원문은 새 Temp의 version BYTE 때문에 객체가 없어도 일반적으로end가 비영이다. ‘저장할 객체0개’와 ‘선택한 stream의 data/end가0’은 다른 조건이다. 기존Terr를 먼저 삭제하거나 비어 있는 영역을 임의로 보존하는 새로운 정책을 넣지 않는다. 실제 rename의 대상 제거·원본 노드 이름 변경은 아래 명시 DataManager 경계에 속한다.

## 신규 API

[RawFortObjectsSave.h](../../cpppj/src/o/RawFortObjectsSave.h), [구현](../../cpppj/src/o/RawFortObjectsSave.cpp)에 다음 API를 둔다.

```cpp
SaveFortObjects(slot, FortObjectsSaveGlobals{templateOwner, version, fortGetter}, hooks);
```

templateOwner와version은 실제 같은 장기DWORD 참조다. fortGetter는 마지막 Chaff 관문에서 한 번 호출하는 현재 getter다. 실제 Session의 `core.ownerMode.fort`는bool이므로 DWORD 변환 임시값의 참조를 보관하지 않는다. 예를 들어 `[&] { return core.ownerMode.fort ? 1U : 0U; }`로 연결한다. version은 기존 객체 writer가 읽는005c89c0과 같은 저장소여야 한다.

**현재 수명 경계:** 부모와 단일객체 writer는 Session의 같은 `objectState.version`을 공유할 수 있다. 그러나 Storage의 `managerGlobal_`(DataManager 생성자가0을 쓰는005c89c0)와 Session의 이version필드는 아직 전체수명이 물리적으로 통합되지 않았다. 이번Objects의CreateSection은DataManager생성자를부르지않아해당경로안에서서로간섭하지않는다. 이 사실을 전체생성자/전역수명복원으로 확대하지 않는다. generic Editor/NewFort에서는 생성자의 같은전역쓰기까지 공유해야 하며 별도version복제나임의초기값으로가장하지않는다.

| hook | 연결할 원본 경계 |
|---|---|
| manager(slot) | 고정 this 슬롯의 현재 manager DWORD |
| assertion(kind) | MissingTemplate957 / NotMine958 / WriteCapacity36 |
| find(manager,name) | 0044ce60 |
| create(manager,name) | 0044d1b0; 없는Terr/새Temp/Chaff 생성 수명 |
| select(manager,name,writable) | 0044ceb0; 현재Temp/1 선택 |
| rename(manager,destination,source) | 0044d160; TerrNN,Temp의 원본 인자 순서 |
| territory(stream,owner,region) | 004be230 전체영역 writer |
| chaff(stream) | 004be350 전체지도 writer |
| access(stream,operation) | 같은 실제16BYTE stream 및 물리버퍼를 동기적으로 빌림 |

access와 MemStreamHooks::buffer는 원본의 별도 게임 효과가 아닌 호스트 저장소 빌림이다. stream의 current를 바꾸거나, manager를 선택하거나, 소유 버퍼를 새로 할당하는 효과를 넣지 않는다. version 쓰기 뒤의 territory가 current를 바꿔도 반환된 같은 stream에 써야 한다. 선택되지 않는 Chaff hook은 요구하지 않으며 실제 도달한 누락 hook만 선행 효과를 보존하고 오류로 보고한다.

이번 신규 모듈은20영역/Temp/Chaff의 **부모**다. 영역의 실제 지도 순회·Scope1, Chaff 전체 순회·Scope2, 단일 청크 writer, DataManager의 unlink/free/rename 및 실제 파일 쓰기를 재구현하거나 검증했다고 주장하지 않는다. 기존 [WriteFortObjectChunk](../../cpppj/src/client/RawCampaignSession.cpp)의 상위에004be230/004be350의 실제 순회를 각각 연결해야 한다.

## 제품 작성 전 동결한 독립 x86 관측

[decomp_fort_objects_parent.py](../../tools/decomp_fort_objects_parent.py)는 위471바이트를 실제 PE에서 가상 x86으로 실행했다. 기본 PE 복사·격리메모리·외부CALL 복귀 도구만 재사용한다. 실제 CPU 쓰기는 원본 스택, 명시16BYTE stream/64BYTE물리 출력, version전역으로 제한한다. 원본DataManager/영역/Chaff/서식·진단은 명시 하위 경계다. 제품CPP나 제품 기대 목록을 읽지 않았으며 제품 작성 전에 결과를 저장했다.

[최초 fixture](../../cpppj/tests/fixtures/fort-objects-parent-x86.tsv)는 **357고유입력×두x87제어어 =714관측**이다. **정상666회/명시중단48회/1,038,830명령/정밀도차이0**을 보존한다. 원문은 정수 부모지만 두 제어어에서 실제 별도 실행했다. slot−2/0/1/8/9와 clamp, manager0/11, isMine 일치/불일치, fort0/1, Terr 없음/전체 있음/일부 있음, 선택된Temp의data0/end0, 비영 초기pos/end, signed alloc/end, callback의 manager/version/fort 변경, 진단 이후 현재pos/end 변경과 부분 중단을 포함한다.

각 행의 입력은control 및 `slot,manager,mine,fort,pattern,pos,end,alloc,mutation`이다. 결과는 정상반환 여부, 현재manager/mine/version/fort 및 세manager의현재선택, **21개stream의모든16BYTE와21개64BYTE전체출력**, 모든Find/Create/영역/Select/Rename/Chaff/진단의 실제 인자/순서다. 이름서식은 원본이 요청한Terr%02d만 명시 공급했다. fixture의 stream token은 호스트 포인터가 아닌 독립 불투명 번호다.

영역/Chaff 하위 장치는 부모의 version 쓰기와 구분되는 입력1BYTE만 추가한다. 실제 청크·전체게임 생성물이라고 간주하지 않는다. 명시중단은 외부 진입에서 emu_stop하고 부분원문을 읽은 결과이며 원본 예외 unwind를 검증한 것이 아니다.

[실행 보고서](../../cpppj/fort-objects-parent-native.json)에 보호PE·기존 정밀원문·도구·fixtureSHA를 기록했다. 기존 fixture/보고서가 있으면 재생성을 거부하며 `--verify`는 최초SHA만 읽기전용 검사한다.

- 보호PE SHA256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`
- fixture SHA256: `df0dc804a10b5f6f802bb8b4501c05b99148e9112325caddc5e98eea79eaf232`

## 검사 준비 상태와 다음 통합

[FortObjectsSaveTests.cpp](../../cpppj/tests/FortObjectsSaveTests.cpp)의 `FortObjectsSave_IndependentNative714`는 입력열로 명시 장치만 준비하고 제품 실행 뒤 전체 raw/현재전역/trace/정상·부분중단을 최초 결과와 대조하도록 작성했다. `FortObjectsSave_ExplicitUnselectedAndMissingBoundary`는 fort0의미선택Chaff생략과누락Find에서선행version2가보존된오류를확인한다. fixture 매크로는 **NETSTORM_FORT_OBJECTS_PARENT_FIXTURE**다.

이 작업자 범위에서는 C++를 **미빌드·미실행** 상태로 인계한다. Python AST·엄격UTF-8·문서로컬링크·원문SHA보존을 읽기전용 확인했다.714행은 원본x86관측이며 아직 제품714행통과 판정이 아니다. 루트가 등록/실제Storage·Session연결 뒤 실제 제품 검사를 실행한다.

Storage 작업자는 동적Temp와실제Find/Create/Select/Rename·generation수명·버퍼빌림을같은manager에연결한다. Session 작업자는 이 부모의 territory/chaff hook에 현재 지도/지역Matches 및 기존 실제 WriteFortObjectChunk를 연결하고 version/Scope를공유한다. 통합검증은 **서로다른두Terr와Chaff의현재객체변경→전체Objects저장→실제작업본파일→재로드**를 확인해야 한다. 지금의 고정35개 원시섹션 재저장이나 한청크만 바뀐 결과를 그 증거로 쓰지 않는다.

전체snapshot의 Money/Mission/Badges/Notify, generic Editor초기설치, 사용자Edit진입/SaveAs/NewFort/시나리오왕복은 [Edit잔여감사](cpp-editor-gap-audit-20261023.md)의별도후속이다. 단위377 준비모듈만으로전체사용자저장이나전체게임목표를완료했다고판정하지않는다.
