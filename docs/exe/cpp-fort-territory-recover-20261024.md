# Territory의 Battle 표시 회복

2026-10-24 **신규 독립 준비·미등록**이다. AGENTS/공통·닷넷 인계 상단을 재확인했고 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치한다. 기존 정밀 C와 보호10.78 PE를 읽었으며 원본/CD/AGENTS/정밀 산출물은 변경하지 않았다. 기존 제품·공유빌드·CMake·LEFT·git·게임·GUI 실행은 없고 신규 격리 콘솔 검사만 실행했다.

## 먼저 고정한 원문 관측

[도구](../../tools/decomp_fort_territory_recover.py)는 제품 작성 **전에** [최초 fixture](../../cpppj/tests/fixtures/fort-territory-recover-x86.tsv)와 [native 보고서](../../cpppj/fort-territory-recover-native.json)를 생성했다. 이후 제품에 맞춰 수정하거나 재생성하지 않았다. 최초 결과가 있으면 덮어쓰기를 거부하며 `--verify`는 PE/C/functions/helper/tool/fixture SHA를 확인한다.

| 항목 | 결과 |
|---|---|
|실행 본문|회복004b5ec0의167BYTE, writer004bd600의93BYTE, 일반Write0044cce0의96BYTE|
|입력/관측|487입력×027f/037f 두정밀도=974관측, 정밀도차이0|
|정상/명시중단|464정상/510중단|
|실제 명령|358,048개|
|하위 진입|getter774/로그8308/Create964/Write538/State402/진단780회|
|물리 중단|현재manager0 424회, NULL source66회, 반환State0 2회|
|callback 중단|read/log1/log2/Create/Write/State/진단36/521/522, 각각2회|
|fixture SHA256|`120da4096de32e509054bae7b3f18495d5cf17f8cb0996dea0b94477fa181ff8`|
|보호 PE SHA256|`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`|

Unicorn은 회복·writer·일반Write의 실제 원문 명령만 실행한다. Territory getter004bd660/State004bd550/Create0044d1b0/로그004c2630/진단004e0620은 명시 하위 장치다. manager0/NULL source/State0은 원문이 접근하기 **직전의 명시 물리 중단**이며 원본의 정상 거부나 실제 OS 예외라고 주장하지 않는다. getter/State 전체 본문·실제 새 섹션/파일·게임·GUI 실행 결과는 아니다.

## 정확한 호출 순서

정밀 C `Netstorm.c:144619~144655`와 `150997~151014`,functions.tsv 및 PE를 대조했다. 회복 부모는 불연속 범위004b5ec0~004b5f0c/004b5f10~004b5f69다.

1. 지역120BYTE를 전부0으로 채운다. **현재 owner00540cac**를 읽어 같은 슬롯의004bd660으로120BYTE를 읽는다.
2. 20개×6BYTE 기록의 두 번째 BYTE를 앞에서부터 확인한다. bit4가 있으면 **29번** `Territory recovered from Battle!\n` 로그를 호출한 뒤 **같은 flags BYTE의 현재 값을 다시 읽고** `&0xfb`한다. 로그 callback이 같은 BYTE를128로 바꾸면128이 유지된다. 다음 flags를0으로 바꾸면 그 다음 로그도 자연 생략한다. 로그가 owner를 바꿔도 이미 읽은 같은 지역120BYTE를 계속 순회한다.
3. flags 변경이 없어도 **현재 owner를 다시 읽어**004bd600에 같은120BYTE를 항상 쓴다.
4. writer는 고정 this 슬롯의 현재 manager가0이면 Template.cpp521, 입력 pointer가0이면522 진단한다. 진단이 복귀하면 계속한다. **현재 manager를 다시 읽어** Create(manager,"Territory")를 호출하고 반환값은 버린다. **고정 this의 현재 manager를 다시 읽어** 그 manager+8의 현재 stream에 일반Write120을 호출한다. Create가 owner만 바꾸어도 writer의 this는 바뀌지 않는다. manager를 바꾸면 바뀐 manager.current를 쓴다.
5. 일반Write는 signed capacity와 현재pos+120을 비교해 MemStream.cpp36 진단한 뒤 **현재 data/pos**를 읽는다.30DWORD를 앞에서부터 복사하고 현재pos를120 증가시킨 뒤 signed end가 작을 때만 end를 증가시킨다. 진단 뒤 data/pos가 달라지는 경우와 end=ffffffff도 독립 관측에 포함했다.
6. writer가 정상 복귀한 뒤 **현재 owner를 다시 읽어**004bd550을 호출한다. 반환한 **같은 State 주소**의 첫BYTE에서 bit4만 지운다. State callback이 owner를 바꾸어도 반환 주소는 유지하고, 다른 State 주소를 반환하면 그 반환 주소를 쓴다. 앞선 새 Territory 쓰기/로그/다른 flags를 rollback하지 않는다.

회복 부모 자체에는 mode/기록 변경 여부/manager 진단의 추가 관문이 없다. getter와 State의 실제 진단·선택·초기화·최소확장은 각 하위 본문의 책임이다.10.72는 정밀 C에 대응 회복00433020/writer00427a50/getter00427c00으로 기록되어 있으나 이번 원문 실행과 제품 대조 대상은10.78이다.

## API와 실제 설치 경계

[RawFortTerritoryRecover.h](../../cpppj/src/o/RawFortTerritoryRecover.h)/[구현](../../cpppj/src/o/RawFortTerritoryRecover.cpp)은 다음 두 본문을 제공한다.

```cpp
RecoverFortTerritory(recoverHooks);
WriteFortTerritoryRecords(slot, optionalRecords120, writerHooks);
```

`FortTerritoryRecoverHooks`는 현재 owner/실제 getter/조건부 로그/실제 writer/반환 State를 분리한다. read는 부모가0으로 채운 고정120BYTE를 동기적으로 빌리며 그 span을 호출 수명 밖에 보관해서는 안 된다. write의 같은120BYTE도 동기 호출 동안만 유효하다. State는 반환 후 첫BYTE 쓰기까지 실제 할당이 유지되어야 한다. 도달한 필수 경계가 없으면 명시 실패하며 기본 성공을 채우지 않는다. bit4가 하나도 없으면 로그 장치는 요구하지 않는다.

`FortTerritoryWriteHooks`는 고정 슬롯의 현재 manager/원문번호 진단/Create/현재 stream/일반 writer다. Create는 반환 stream을 사용하지 않으므로 반환형이void다. `optional<FortTerritoryRecords>`의 없음은 원본NULL 입력 진단522를 표현한다. 진단이 복귀하면 없는 입력도 길이120으로 일반 writer에 전달한다. 실제 NULL 접근 거부는 일반 writer 장치가 맡으며 빈 기록 쓰기로 가장하지 않는다.

후속 [기존 Editor 설치](cpp-generic-editor-install-20261024.md)의 `recoverEditorTerritory`는 같은 현재 Template owner로 이 회복 부모를 호출해야 한다. 읽기는 기존 `RawFortWorldLoad::ReadTerritory`/Storage의`SelectTerritory`로, State는 같은 `RawPlayerTemplateState::Get`으로 연결한다. 새 Territory 쓰기는 현재 헤더 manager→Storage의`CreateSection(slot,"Territory")`→Create뒤 같은 헤더 재조회→`CurrentWriteStream(slot)` 순서로 연결하고, 반환한 실제 노드/세대 토큰을 `AccessSection`에 빌려 기존 `WriteMemStream`으로120BYTE 기록한다. Create반환 stream을 그대로 쓰는 어댑터는 원문과 다르다. manager/slot 대응 검사는 현재 실제 헤더를 기준으로 하고 다른 owner나 최초 manager를 편의상 캐시하지 않는다.

일반 SnapshotTerritory는 섬/seed를 다시 만드는 별도 부모다. 회복을 그 부모로 치환하면 기존120BYTE의 비flags 값/호출 순서가 바뀐다. 이 회복은 State 첫BYTE를 바꿀 뿐 Notify·파일 저장·전체Snapshot을 호출하지 않는다. 로그와 진단의 실제 소비자, getter/State의 장치 수명, 저장 작업본 보호는 후속 Session/Client 통합 범위다.

## C++ 대조와 인계

[FortTerritoryRecoverTests.cpp](../../cpppj/tests/FortTerritoryRecoverTests.cpp)는 기대열을 사용하기 전에 입력열만으로 제품을 실행하고, completion/현재 owner와 여섯 호출수/전체9헤더·두manager·세stream·세256BYTE출력·9State·입력120·지역120/trace를 대조한다. 정상 종료 뒤 지역span을 역참조하지 않고 마지막 State 경계나 명시 중단 **안에서** 지역120BYTE를 소유 배열에 복사해 수명 문제를 피한다.

입력은 owner0/1/8,manager0/11/22,flags 없음/전체/짝수/마지막/255,State첫BYTE0/4/255,출력pos0/4,end0/ffffffff,capacity0/256,NULL 원문을 교차한다. callback은 owner/고정 manager/current stream/같은 flags/진단 뒤 output data·pos/반환State를 바꾸며,37BYTE getter 부분쓰기 후 중단과 각 선택 경계 중단도 보존한다. 모든 임의 callback 조합이나 전체 장치 구현을 검증한 것은 아니다.

격리 경로 `extracted/fort-territory-recover-20261024-isolated`에서 `/std:c++20 /EHsc /utf-8 /W4`로 신규 모듈/검사와 기존 MemStream·DataManager 소멸 primitive만 별도 콘솔 빌드했다. **빌드1 종료0·관측 경고0,2검사/실패0**이며 `build1.log`/`test1.log`를 보존한다. 두 번째 검사는 flags 없음에서도 쓰기/State 실행, 미선택 로그의 생략, 선택된 State 누락/빈 물리범위의 앞선 쓰기 보존,Create뒤 manager22의 현재 stream 선택을 확인한다. 최초 fixture/report/tool의 `--verify`도 종료0이다.

공용 등록과 실제 Session 연결은 root 소유다. 이 독립 모듈 완료를 기존 Fort Editor 설치·실제 메뉴/NewFort·변경/전체 저장/재로드·사용자 플레이 성공으로 확대하지 않는다.
