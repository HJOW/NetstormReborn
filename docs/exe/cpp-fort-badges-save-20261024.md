# Badges 저장 부모와 실제 장부 getter

2026-10-24 후속 독립 복원이다. AGENTS·공통/닷넷 인수인계를 재확인했고 현재 PC/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 기존 코드·장부 소유자·공용 CMake·LEFT·공유 빌드·GUI·git·보호 원본·정밀 디컴파일은 변경하지 않았다. 아래 신규 모듈은 자체 격리 검사만 완료했으며 실제 Client 장부 초기화·사용자 Edit 저장 완료가 아니다.

## 실제 원문과 최초 고정 자료

[10.78 정밀 C](../../extracted/refined/originals/Netstorm.c)의151852~151907행 **004be7f0 저장 부모234BYTE**와101987~102009행 **0047d7c0 getter54BYTE**를 독립 관측했다. getter의3BYTE 정렬은 실행 범위에서 제외했다. 저장 부모의10.72 후보는00428ed0, getter는00475630이나 이번 관측·제품은10.78 전용이다.

[도구](../../tools/decomp_fort_badges_save.py)는 보호 PE를 가상 x86에 복사해 **저장 부모·28개linear검색 getter를 모두 실제 실행**한다. Create/진단 보고만 명시 외부 장치다. getter 반환 뒤·진단 callback의 data/pos/count 변경은 입력에 명시한 효과이며 원본 getter 자체의 부작용으로 주장하지 않는다. CPU 쓰기는 스택/두stream/두물리버퍼로 제한하고 장부는 실제6160BYTE 위치에 입력으로 준비한다. 장부의 padding A5·타입/수량 프로필은 독립 입력이지 Client CRT 초기화 결과가 아니다.

제품 소스를 작성하기 전에 [최초 fixture](../../cpppj/tests/fixtures/fort-badges-save-x86.tsv)와 [보고서](../../cpppj/fort-badges-save-native.json)를 고정했다. **676입력×두x87제어어=1,352관측, 정상1,272/명시중단80,29,662,486명령,정밀도차이0**이다. 실제 getter 반환167,834회·Create710회·진단3,788회다. 기존 fixture/보고서가 있으면 재생성을 거부하며 `--verify`에서 모든 최초 SHA만 확인한다.

fixture SHA256은 **ec75999aa1489573f259d3c5257a7ed6fb954f6c57f6afc2f8d08c439eadda2f**다. 각 행은 입력 kind/profile/query/count/manager/pos/end/capacity/mutation, 정상/중단, 현재장치7DWORD, **Template20+stream32+출력512BYTE 전체**, **장부6160BYTE 전체RLE**, 실제getter반환/처음counterBYTE/진단순서를 담는다. type0/254/255/256/ffffffff, 첫 중복0/비0, count0/1/255/256/ffffffff/80000000/12345678, signed end/capacity, 두 번째stream 반환/다른current, 진단 후 manager/data/pos/end/capacity/장부count 변경과 부분 중단을 포함한다.

## 원본의 정확한 저장 순서

1. 고정 this의 manager0이면 Template.cpp **0x412**를 진단한다. 복귀 뒤 manager를 다시 읽고 **Create("Badges")0044d1b0**의 반환 stream을 보관한다. 다른 current로 재조회하지 않는다.
2. 새 stream의 **기존 data+기존 pos 포인터를 보관**한다. pos를 DWORD1 진행하고 signed 비교로 end를 진행한 다음 **보관 포인터에0BYTE**를 쓴다. 이 첫 BYTE에는 용량 진단을 추가하지 않는다.
3. **type0..254 전체255회**0047d7c0을 조회한다. 반환 count DWORD가0이면 넘어가고, 음수 비트나256도 비0이면 항목을 쓴다. type255는 이 저장 루프에서 조회하지 않는다.
4. 반환 DWORD를 보관한 상태에서 signed `pos+1 > capacity`이면 MemStream **0x24**를 진단한다. 복귀 뒤 **현재 pos/data**로 type 낮은BYTE를 쓴다. 쓰기 뒤 현재 pos/end를 다시 읽어 pos1/end를 진행한다.
5. 그때 보관한 진행 위치+1과 현재capacity를 비교하고 필요하면 다시 진단한다. 복귀 뒤 **현재 data/pos**로 **앞서 조회한 count의 낮은BYTE**를 쓴다. 장부를 다시 읽거나 진단이 바꾼 새 count로 대체하지 않는다. 현재 pos/end를 진행한다.
6. 마지막에 **처음 보관한 count 포인터의BYTE를1 감김 증가**한다. 현재 stream.data나 새로운 pos로 count 위치를 바꾸지 않는다. type/count 쓰기가 그 BYTE를 덮어쓴 경우에도 같은 포인터를 증가한다.

`ReadFortBadgeCount`의 원문은 전역 **[00593160,00594970),28×220BYTE**에서 처음 같은 **type DWORD**를 찾고 해당 **+216 count DWORD**를 반환한다. 첫 같은 타입의 count가0이어도 뒤 중복을 찾지 않는다. 미일치0이며 타입을BYTE로 절삭하지 않는다. type/count 사이212BYTE는 그대로 보존한다. 실제 Client CRT004fcbe0의28번째 기록은 type0인220BYTE 전부0이므로 type0을 제외한 편의 목록으로 바꾸지 않는다.

## 공개 API와 기존 Meta loader 재사용

[RawFortBadgesSave.h](../../cpppj/src/o/RawFortBadgesSave.h), [구현](../../cpppj/src/o/RawFortBadgesSave.cpp)의 API는 다음과 같다.

- `ReadFortBadgeCount(span<const uint8_t> badges,uint32_t type)`: 같은 실제6160BYTE를 읽는0047d7c0이다. 장부를 소유하거나 초기화하지 않는다. 공개 상수 `kFortBadgeRecordCount/Stride/CountOffset`은28/220/216이다.
- `SaveFortBadges(int32_t slot,const FortBadgesSaveHooks&)`: manager/assertion/create/count/access로 위 부모를 실행한다. enum 진단은 실제0x412/0x24다. `access`는 기존 `CampaignTemplateStorage::AccessSection`과 같은16BYTE/`MemStreamHooks` 빌림 형식이다. 최초 buffer span+offset을 보관해 두 버퍼의 수명을 유지한 data 교체 뒤에도 같은 count BYTE에 쓴다. 빌린 최초 할당은 저장 완료까지 살아 있어야 한다.

실제 연결자는 **장기 live 장부6160BYTE 한 개**를 소유해야 한다. 기존 `RawFortMetaLoad`를 생성할 때 그 mutable span을 넘기고, 새 `hooks.count`에는 **같은 span을 `ReadFortBadgeCount`에 넘기는 lambda**를 설치한다. 기존 `RawFortMetaLoad::SetBadge/FindBadge/LoadBadgesSelected`가 쓰는 기록과 새 저장 getter가 읽는 기록이 같아진다. 기존 클래스의 private count 읽기를 노출하거나 별도 장부 사본을 만들 필요는 없다. 이번에는 기존 loader·장부 소유자 코드를 수정하지 않았다.

장부의 실제 CRT 초기화004fcbe0/초기 count 재설정0047d860, MetaDisplay 수량 갱신·실제 Client 소유권과 generic Fort 설치는 후속이다. 로더 API와 새 writer의 존재를 이 경로 연결 완료로 계산하지 않으며 **과거 Fort의 원시 Badges 섹션을 현재 live 장부로 가장하지 않는다**. 전체 부모004bf9a0은 이 모듈을 Badges child로 호출하고, Battle004bf880의 CoreData004beb30은 별도다. [전체 부모](cpp-fort-snapshot-parent-20261023.md)와 [상태 전역 후속 설계](cpp-editor-snapshot-next-20261023.md)를 따른다.

## 자체 검증과 남은 통합

[FortBadgesSaveTests.cpp](../../cpppj/tests/FortBadgesSaveTests.cpp)는 최초 입력열만으로 장치·장부를 구성하고 제품 실행 뒤 모든 관측열을 비교한다. 원본과 같은1,352행/80중단/167,834getter/3,788진단/710Create가 모두 있어야 통과한다. 별도 검사는 누락 count 장치에서 **이미 생성한 섹션·초기count0·pos1이 보존되는 실패**, 정확한6160BYTE 장부 계약을 확인한다. 등록 매크로는 **NETSTORM_FORT_BADGES_SAVE_FIXTURE**다.

`extracted/cpp-fort-badges-save-20261024-isolated/`에서 공용 라이브러리 없이 신규CPP/검사/콘솔진입만 MSVC19.51·C++20·`/W4 /utf-8 /permissive- /EHsc`로 빌드했다. **Release 종료0·관측 경고/오류0,2검사·실패check0**이다. `build.log`, `tests.log`에 보존했다. UTF-8·PythonAST·최초SHA·문서링크·공백과 원문 fixture가 제품 작성보다 앞섰음을 정적으로 확인한다. 공용 등록/Session/장부 소유/실제 파일 저장은 수행하지 않았다.
