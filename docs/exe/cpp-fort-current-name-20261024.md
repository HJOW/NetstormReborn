# 현재 Fort 이름 부모와 동일 manager 파일명 setter

2026-10-24 **신규 독립 준비·미등록** 자료다. 작업 전 AGENTS 및 공통/닷넷 인계 상단을 재확인했고 현재 PC/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. 보호된10.78 PE와 기존 정밀 C만 읽었으며 원본/CD/AGENTS/정밀 산출물을 변경하지 않았다. 게임·GUI·OS 파일 함수·공유 빌드·git을 실행하지 않았다. 신규 격리 경로에서 콘솔 단위 검사만 실행했다. 실제 Client·Storage·CMake·LEFT 등록은 후속 담당 범위다.

## 최초 원문 관측과 실행 범위

제품 소스를 만들기 **전에** [도구](../../tools/decomp_fort_current_name.py)로 [최초 fixture](../../cpppj/tests/fixtures/fort-current-name-x86.tsv)와 [보고서](../../cpppj/fort-current-name-native.json)를 고정했다. 이후 제품/검사를 대조했으며 fixture를 제품 결과에 맞춰 다시 만들지 않았다. 도구는 기존 결과 덮어쓰기를 거부하고 `--verify`에서 PE/정밀 C/functions/helper/tool/fixture SHA를 확인한다.

| 항목 | 결과 |
|---|---|
| 원문 진입 |00460980~00460ba6,551BYTE /004bf630~004bf68a,91BYTE |
| 독립 입력/관측 |438입력×x87 027f/037f =876관측,정밀도 차이0 |
| 정상/명시 중단 |720정상/156중단,총792,378명령 |
| 물리 중단 |manager0 24/manager명256BYTE 28/oldpath256BYTE 56/current256BYTE 32 |
| callback 중단 |snapshot1/Notify/snapshot2/삭제/제목/State/config1/점 진단,각2 |
| fixture SHA256 |`4c89096cbf763420fde7f95977c2d1d505ac01efb7c62f728a453c1de1869604` |
| 보호 PE SHA256 |`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236` |

원문 부모·setter와 실제 `strchr`004e63e0/C로케일 `stricmp`004ef510 코어를 Unicorn에서 실행했다. CRT004e65ed의 OS TLS/로케일 선택만 C로케일 코어로 명시 전달했다. 실제 OS/게임/파일 삭제는 호출하지 않았다. PathSpec004410f0/전체 snapshot004bfac0/Notify004bd360/삭제0041aef0/제목00460730/State004bd550/config004414d0/진단004e0620은 명시 하위 경계다. 전체 snapshot·Notify의 내부 저장/진단/실제 디스크 효과를 이 fixture가 검증했다고 주장하지 않는다.

원문 current는00460650 부근의 실제 `operator_new(0x100)`(`Netstorm.c:78131`), manager명은+c에서256BYTE, 부모 지역 oldpath는256BYTE다. 원문은 초과 검사 없이 복사하지만 oracle와 제품은 해당 물리 범위를 넘는 쓰기 직전에 **명시 중단**한다. 앞서 쓴 BYTE/헤더 값은 유지한다. 이는 원문 자체의 정상 거부나 원문 예외를 복원한 것이 아니다. manager0 진단이 복귀해도 실제0+c에는 쓰지 않고 별도 물리 중단한다.

특히 초기 manager0이면 지역 oldpath는 원문에서 초기화되지 않는다. 이후 진단 callback이 manager를 만들면 미정 지역값을 비교/삭제 경계에 넘길 수 있다. 최초 입력은 `Prior\0`+250개의cd BYTE를 명시 공급했다. 제품 API도256BYTE scratch를 필수 입력으로 받고 임의0이나 새 이름으로 채우지 않는다. 미정 메모리의 실제 값이나 모든 미정 실행 결과를 확정한 것은 아니다.

## 원문 순서와 현재값 재조회

기존 정밀 C는 `Netstorm.c:78378~78461`/`152592~152615`, PE와 functions.tsv의 해당 두 범위를 함께 확인했다.

1. 최초 **현재 owner00540cac**의 Template 헤더005c89f0+owner×20 첫DWORD가 비영이면 현재 current0055b490 첫BYTE를 검사한다. 비었으면 Fortspec.cpp39 진단 후 계속한다. current 포인터를 다시 읽고 FileSpec 포인터005327e0를005327ac로 선택한 다음 PathSpec(FileSpec,current,0,0)의 경로를 지역 oldpath에 NUL까지 전방 복사한다.
2. 새 이름의 첫NUL 앞 점만 `strchr`로 찾고 있으면 Fortspec.cpp88 진단한다. 진단 뒤 **현재 owner**의 manager와 헤더+8을 다시 읽는다. manager비영/+8=0일 때만 첫004bfac0을 호출한다. 그 뒤 현재 current 포인터를 다시 읽고 **원래 입력 주소의 그 시점 BYTE**를 NUL까지 복사한다. 새 이름을 미리 복제하지 않는다.
3. 다시 현재 owner의 manager를 읽는다. 비영이면 그때 헤더+8을ESI에 캡처하고 동일 헤더+8에0을 쓴다. 빈 current39 진단 뒤 current를 재조회해 두 번째 PathSpec을 부른다. **PathSpec 복귀 뒤 현재 owner를 다시 읽어** setter004bf630의 this를 선택한다.
4. setter는 고정 this의 manager0이면 Template.cpp429 진단, 그 뒤 같은 this+4가 비영이면430 진단한다. 두 진단은 독립적이며 복귀하면 계속한다. 그 뒤 같은 this의 **현재 manager**를 재조회하고 manager+c에 전달받은 경로를 NUL까지 복사한 후 **동일 this의 Notify004bd360**을 호출한다. current owner를 setter의 this로 바꿔 읽지 않는다.
5. setter 복귀 뒤 부모는 다시 현재 owner의004bfac0을 호출한다. 앞서 캡처한+8이0이면 세 번째 PathSpec(FileSpec,원래 입력 주소,0,0)을 부르고 oldpath와 대소문자 무시 비교한다. 다를 때만 이전 경로의 삭제를 **요청**한다. callback이 나중에 바꾼+8을 이 삭제 관문에 재사용하지 않는다.
6. 제목00460730→dirty00542678 DWORD 증가→Fort00594fb8/Editor005c85a4/Mission00594fa0 관문이다. Fort비영/Editor0/Mission0/현재manager비영이면 현재 슬롯의 State004bd550을 얻고 반환+0x11 BYTE가0일 때 현재이름으로 `lastMultiplayerFort`를 쓴다. 마지막으로 **현재 Editor를 다시 읽어** 비영이면 그 시점 현재이름으로 `lastEditFort`를 쓴다. 앞선 config callback이 Editor를 켜면 두 설정이 모두 실행된다. Fort비영/Editor비영은 바로 lastEditFort로 가고 Mission 관문을 건너뛴다.

FileSpec 재선택은 **포인터만 쓰기**다. callback이 고정 버퍼 내용을SOLOSpec으로 바꾸면 뒤 PathSpec도 바뀐 내용을 읽는다. setter는 점/빈 경로를 별도 검증하거나 확장자를 덧붙이지 않는다. 빈 이름/점 진단은 반환하면 계속하며 임의 이름 정규화나 기본 이름을 추가하지 않는다.

Readonly/+8이1이어도 current/manager명 변경·두 번째 snapshot은 실행되고+8은0으로 바뀐다. 첫 snapshot과 이전 파일 삭제만 원래 관문에 따라 생략된다. 이름이 같거나 대소문자만 달라 경로가 같아도 나머지 호출을 임의 생략하지 않는다. 원문004bfac0은 Battle/Fort의 **두 독립 if**인 [SnapshotCurrentTemplate](../../cpppj/src/o/RawFortSnapshotParent.h)이다. 이 둘을 합치거나 setter Notify와 snapshot Notify를 한 번으로 줄이지 않는다.

## 신규 API와 소유 수명

[RawFortCurrentName.h](../../cpppj/src/o/RawFortCurrentName.h)/[구현](../../cpppj/src/o/RawFortCurrentName.cpp)은 다음 두 진입을 제공한다.

```cpp
SetFortManagerFileName(slot, pathNulSpan, hooks);
SetCurrentFortName(newNameNulSpan, oldPathScratch256, globals, hooks);
```

`FortCurrentNameGlobals`는 owner/fort/editor/mission의 **순수 현재 getter**와 실제 `uint32_t& dirty`다. `FortCurrentNameHooks`는 고정header/currentName/managerName,FileSpec선택/PathSpec,진단/snapshot/Notify/경로비교/삭제요청/제목/State/config를 분리한다. 도달한 경계만 필수이며 빈 성공 callback을 제공하지 않는다. 하위 callback 예외는 그대로 전달하고 rollback하지 않는다.

Template 헤더는 슬롯별 실제20BYTE 주소가 진단 중 유지되어야 한다. managerName은 재조회된 manager토큰의 실제+c256BYTE를 빌리고, currentName은 그 시점0055b490의 실제 쓰기 주소를 빌린다. 새 이름 span은 동기 부모가 끝날 때까지 같은 실제 주소를 유지해야 한다. 같은 current 버퍼를 새 이름으로 넘기는 자기참조도 허용하며 전방 BYTE 복사의 효과를 보존한다. PathSpec 반환은 다음 PathSpec 전까지 유효해야 하고 State는 반환 직후+17 조회까지 유효해야 한다. 물리 span을 임의 확장하거나 복제 캐시를 실제 주소로 가장하지 않는다.

[검사](../../cpppj/tests/FortCurrentNameTests.cpp)는 입력열만으로 실행한 뒤 기대열과 비교한다.9×20헤더/두276BYTE manager/두256BYTE current/512BYTE 새 입력/FileSpec9BYTE/지역256BYTE까지 전체 raw를 대조한다. NUL뒤점과 바이트 보존,255/256경계,owner0/1/8,현재 포인터/owner/manager/readonly/입력/FileSpec의 callback 변경,빈현재 진단 뒤 owner변경,manager0·중복 진단 뒤 manager교체,설정 사이 모드변경,dirty DWORD wrap,선택적 미연결과 중단 전 효과를 포함한다.

## 후속 Client·작업 파일 연결

이 모듈은 이름 부모만 복원했다. [이름Gump/Client 후속 설계](cpp-editor-client-next-20261023.md)의 입력40자·제출 정규화/동명 존재 거부는 상위004cf9a0 범위로 남는다. 새 이름에 포함된 점을 이 부모에서 조용히 제거하거나 원본과 다른 기본 성공을 넣지 않는다. `.english` 복사/이름변경/삭제는 원문 부모에 없다.

현재 `CampaignTemplateStorage::Headers()`와 State/실제 stream을 재사용하되 새로 **현재 manager+c 이름쓰기 빌림**을 연결해야 한다. `Name(slot)`은 `Image.name` 문자열을 반환하고 저장 장치는 실제 manager+c를 읽으므로 둘의 동기성/일원 소유를 먼저 닫아야 한다. 이름만 바꾸려고 `InstallFile`로 manager·State·노드/stream 수명을 교체하지 않는다. `RenameSection`도 manager 파일명 변경과 다른 동작이다. 같은 setter Notify는 필수 경계이고 전체 snapshot은 원본 wrapper를 실제 child/Storage에 연결해야 한다.

`EditorFiles`는 현재 Prepare/Write/MakeWritable와 내부 Checked만 제공하며 실제 Delete/Exists는 없다. 후속에서 **동작 직전 동일 Checked 규칙을 통과하는 작업루트 전용 삭제/존재 경계**를 확장한다. 기존파일을 덮지 않는Prepare로 작업본을 마련하고 current/PathSpec/manager명/저장/목록/재로드가 같은 작업 namespace를 읽게 한다. 원본/CD와 그 hard link를 쓰거나 삭제에 넘기지 않는다. 이 모듈의 deleteRequest는 파일조작을 전혀 수행하지 않는다. 실제 파일 저장 실패 뒤 상위로 예외가 전달되면 이후 삭제로 진행하지 않는다.

첫 통합 검사는 임시 작업루트의 같은 실제 manager에 대해 이름A→B,다시C를 적용하고 **두 snapshot·setter Notify·실제 새 파일명 저장·old 작업Fort 삭제조건·제목/dirty/lastEditFort**를 관측한다. 대소문자만 다른 이름/동일PathSpec/+8=1은 삭제가 생략되어야 한다. Notify·snapshot·쓰기실패 주입 시 원본 순서의 부분효과를 기록하고 보호 경로/원본해시를 대조한다. 실제 GUI는 입력Gump Save as→목록·재실행/재로드와 Test Battle/Return까지 후속 전체 Editor 작업에서 수행하며 이번 준비 검사의 성공으로 대체하지 않는다.

## 격리 검증과 비교판

`extracted/fort-current-name-20261024-isolated`에 신규 runner/obj/exe와 build1.log/tests1.log를 두고 MSVC14.51 x64 `/std:c++20 /EHsc /utf-8 /W4`로 신규 제품1/검사1/runner만 컴파일했다. **최초 빌드 종료0/관측 경고0,검사2개/실패0**이다. 공용 빌드/CMake/기존 제품/LEFT/git은 변경하지 않았다. 검증 후 최초 SHA·UTF-8·Python AST·신규 문서 링크도 확인한다.

10.72는 기존 정밀 C의 대응 표기 **004ca6a0(이름 부모)/004274b0(setter)**만 기록한다. 이번 독립 실행/입력/제품 대조는10.78 대상이며 CD 동작 동일성을 주장하지 않는다. 전체 Edit/저장/메뉴/HUD와 사용자 캠페인·데모 완주는 계속 별도 목표다.
