# 전체 Fort/Battle Snapshot 부모와 현재칸 wrapper

2026-10-23 후속 준비다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. AGENTS·공통/닷넷 인수인계를 확인했다. 기존 제품·공용 CMake·LEFT·공유 빌드·GUI·git·보호 원본·정밀 디컴파일은 변경하지 않았다. 신규 모듈은 **단위377 이후 통합 대상**이며 아래 자체 격리 검사 통과를 실제 Editor 저장 완료로 해석하지 않는다.

## 원문 범위와 최초 독립 관측

[정밀 C](../../extracted/refined/originals/Netstorm.c):152737~152884와 보호10.78 PE의 다음 **684BYTE**를 관측했다. 각 끝 주소는 포함하지 않는다.

| 진입 | 범위 | 실제 역할 |
|---|---|---|
| 004bf880 | [004bf880,004bf991) | Battle 전체 부모 |
| 004bf9a0 | [004bf9a0,004bfab8) | Fort 전체 부모 |
| 004bfac0 | [004bfac0,004bfafb) | 현재 모드별 두 독립 if |
| 004bfb50 | [004bfb50,004bfb98) | 최초 현재칸 manager 존재 관문 뒤 두 분기 |

정밀 원문의10.72 대응 후보는 Battle00429880/Fort00429920/현재칸004299d0이다.004bfb50은 대응 후보가 없다. 이번 관측·제품은 **10.78 전용**이며 CD 동작을 검증하지 않았다.

[최초 도구](../../tools/decomp_fort_snapshot_parent.py)는 부모의 실제 명령·nested CALL·tail jump와 **실제 CPU의 manager/owner/fort/battle 메모리 읽기**를 실행한다. child writer·DataManager Create·현재 기술 getter·진단 보고는 이름이 있는 명시 외부 장치다. CPU 쓰기는 스택에만 허용한다. OS·게임 실행·제품 CPP/기대값 읽기는 없다.

제품을 작성하기 전에 [fixture](../../cpppj/tests/fixtures/fort-snapshot-parent-x86.tsv)와 [보고서](../../cpppj/fort-snapshot-parent-native.json)를 고정했다. **1,364입력×두x87제어어=2,728관측, 정상2,350/명시중단378,187,894명령,정밀도차이0**이다. 도구는 기존 fixture/보고서가 있으면 생성을 거부하고 `--verify`에서 최초 모든 SHA만 읽기 전용 확인한다.

입력은 kind/고정slot/manager/현재owner/두모드/명시callback변경이다. slot−2/0/1/8/9, manager0, 각 진단, 두 모드 모두0/모두1, child가 바꾸는 manager/owner/모드/기술 부모/현재stream, 반환0과 중단을 포함한다. 관측은 정상·중단 여부, 현재전역8DWORD, **12개Template의 전체240BYTE**, 모든 실제 메모리 읽기와 child 인자/순서다. 원문SEH/unwind를 관측한 것이 아니라 지정 child 진입에서 `emu_stop`한 부분 상태다.

## 제품 API와 원문 순서

[RawFortSnapshotParent.h](../../cpppj/src/o/RawFortSnapshotParent.h), [구현](../../cpppj/src/o/RawFortSnapshotParent.cpp)의 API는 다음 네 개다.

- `SnapshotFortTemplate(slot,globals,hooks)`: manager0x530 → fort0x531 → isMine0x532 진단 후 **TypeNames→기술→Objects→Territory→Deck→Money→Mission→Badges→Notify**.
- `SnapshotBattleTemplate(slot,globals,hooks)`: manager0x50e → battle0x50f → isMine0x510 진단 후 **기술→Deck→Objects→Territory→Money→Mission→CoreData→Notify**. TypeNames/Badges를 호출하지 않는다.
- `SnapshotCurrentTemplate(globals,hooks)`: battle getter → 켜졌으면 그때 owner getter/전체Battle → **그 뒤 fort getter** → 켜졌으면 그때 owner getter/전체Fort다. `else if`가 아니다.
- `SnapshotCurrentTemplateIfPresent(globals,hooks)`: owner getter/현재칸 manager를 먼저 읽고0이면 종료한다. battle getter가 켜졌을 때만 전체Battle 뒤 owner를 다시 읽는다. 이후 fort getter/전체Fort는 그때 보관한 owner를 사용한다. **Battle이 생략되면 owner를 두 번 읽지 않고, Battle 뒤 manager 존재 관문도 반복하지 않는다.**

`FortSnapshotParentGlobals`는00540cac의 현재 Template owner 및00594fb8/00594fbc의 fort/battle getter다. Player owner00540c70과 혼동하지 않는다. getter는 원본 전역 조회이며 callback 전후 동기화 사본이 아니다. Hooks의 manager(slot)는 고정 this의 현재 첫DWORD다. 각 child마다 현재owner로 this를 바꾸지 않는다.

`FortSnapshotParentHooks`는 manager/assertion/typeNames/technologyParent/create/technology/objects/territory/deck/money/mission/badges/coreData/notify다. 기술은 manager0x4a6 진단 → **technologyParent() 반환 캡처 → 현재manager → create(manager,"Technology") 반환stream 캡처 → technology(parent,stream)**이다. Money는 현재manager/create("Money")/money(stream)다. Mission은 manager0x1e8 진단 → **현재manager 재조회 → create("Mission") → mission(stream)**이다. current 선택이나 기술 getter 반환을 Create 뒤 다시 읽지 않는다. stream0도 원문처럼 child에 전달하며 임의 유효성 관문을 더하지 않는다.

isMine은 원본의 **DWORD 주소 비교**를 보존한다.20×owner의 감김 때문에 owner+0x40000000/0x80000000/0xc0000000이 같은 Template 주소를 만들 때도 진단하지 않는다. wrapper는 같은 PE 주소를 호스트 slot으로 변환한다. 실제20BYTE 기록 경계 사이를 가리키는 입력은 호스트 물리 경계 오류로 거부한다. 정상 Player0..8과 관측한 동일 주소 별칭은 그대로 지원하며 임의32비트 잘못된 포인터 메모리까지 복원했다는 뜻은 아니다.

도달한 진단 callback은 복귀 뒤 처리와 현재 재조회를 허용한다. 선택되지 않는 child는 필수로 요구하지 않으며, 선택된 누락 hook은 앞선 효과를 유지한 채 명시 오류다. 마지막 Notify의 파일 저장 관문과 Money/Mission/Badges/CoreData 본체는 별도 모듈이다. 각각의 기능을 임의 성공 callback으로 바인딩하지 않는다.

## 격리 검사와 통합 경계

[FortSnapshotParentTests.cpp](../../cpppj/tests/FortSnapshotParentTests.cpp)의 `FortSnapshotParent_IndependentNative2728`은 최초 fixture 입력열로 장치만 구성하고 제품 실행 뒤 모든 관측열을 비교한다. `FortSnapshotParent_SelectedAndMissingDevices`는 Fort의 미선택CoreData/Battle의 미선택TypeNames·Badges, Create 누락의 앞선 TypeNames/기술getter 효과, manager0 wrapper의 미선택 전체child를 검사한다. 등록 매크로는 **NETSTORM_FORT_SNAPSHOT_PARENT_FIXTURE**다.

자체 빌드는 `extracted/cpp-fort-snapshot-parent-20261023-isolated/`다. 공유 netstorm.lib도 링크하지 않고 신규CPP/검사/콘솔진입3개만 MSVC19.51·C++20·`/W4 /utf-8 /permissive- /EHsc`로 빌드했다. **Release 종료0/관측 경고·오류0,2검사/실패check0**이다. 로그는 `build.log`, `tests.log`이며 실제 게임창·원본실행은 없다. CMake 제품 등록·Session 연결은 수행하지 않았다.

다음 root 통합은 이미 있는 같은 Session의 TypeNames/Technology/Objects/Territory/Deck child를 재사용하고 새 Money/Mission/Badges·실제 Notify를 이 hook에 연결한다. **기술 부모/inlineCreate는 이 부모가 담당**하므로 기존 `SnapshotFortTechnology` 전체를 다시 호출하면 Technology를 두 번 생성한다. root는 기술 내용물만 기존 writer에 연결하거나 이 부모의 technology 구간을 실제 동일 저수준 API로 바인딩해야 한다. Mission도 자기가 새 섹션을 다시 만들지 않는다. 원본 전체 상태의 단독 소유/005c89c0·live Badges·실제작업본 namespace는 [후속 설계](cpp-editor-snapshot-next-20261023.md)를 따른다.
