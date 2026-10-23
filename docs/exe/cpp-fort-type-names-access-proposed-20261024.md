# TypeNames의 Storage 동기 빌림 제안

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 단위382 `3b7de358` 이후 읽기 검토/제안이며 기존 Session/Storage/RawFortTypeNamesLoad/CMake/LEFT/공유 build/lib/git/원본·CD는 하위 작업자가 변경하지 않았다. GUI/게임 실행은 없다. root가 허용한 별도 `extracted` 복사본과 **382 netstorm.lib 읽기 링크**만 사용했다.

**결론:** `LoadFortTypeNames`의 선택된 stream/body를 실제 `CampaignTemplateStorage::AccessSection` 안에서 실행하는 optional `access(token,body)` 확장이 타당하다. 필드16BYTE와 data를 복사해 돌려주는 방식은 같은 실제 alias가 아니다. 제안 patch는 `extracted/cpp-fort-type-names-access-20261024-proposed.patch`, SHA-256 `fb53bf8ee93c0495f16fae89a9ae0af0940e9c61c176bfa027ed9abe715a6a0f`다. 기존 두 RawFortTypeNamesLoad 파일만 변경 제안한다.

격리8검사 뒤 **root가 이 동결 patch를 공유 RawFortTypeNamesLoad.h/.cpp에 적용하고 새6검사를 공용에 복사·등록했다.** 현재 root의 NetstormCpp 공용 빌드가 진행 중이며 그 결과와 현재 Session 검증은 아직 이 격리 기록에 포함되지 않는다. 아래 공유원문 비변경은 하위 복사본 준비 당시 범위이며, 적용 뒤 이전 공유 SHA를 다시 강제하지 않았다. 공유 lib도 격리 당시 전후 기록을 동결하고 현재 파일에 과거 SHA를 다시 요구하지 않는다.

## 최소 API와 순서

`RawMemStream.h`를 포함하고 hooks 마지막에 다음 optional 필드를 붙인다. 기존 stream/buffer 경로와 그 실제 배열을 쓰는 독립664 fixture를 유지한다.

```cpp
std::function<void(std::uint32_t,
    const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access;
```

진입 순서는 **현재 depth 증가→고정 slot의 manager→Select(manager,"TypeNames",1)→반환 token 고정** 그대로다. 그 뒤 access가 있으면 `access(token,[&](fields,memory){ 기존본문(fields,memory.buffer); })`를 실행하며 기존 hooks.stream/buffer는 부르지 않는다. access가 없으면 원래 stream(token)/buffer(data)로 같은 본문을 실행한다. 기존 본문은 들여쓰기와 `Need(hooks.buffer)`의 인수 치환 외 기계적으로 동일함을 별도 확인했다.

정상 depth 감소는 **access가 성공하여 반환한 뒤**다. 실제 Storage의 완료 처리까지 예외가 나면 증가한 깊이가 남고 상위 prepare가 실패한 본문에만 별도 `UnwindFortTypeNamesLoad`를 한 번 적용한다. 정상 감소 뒤 중복cleanup이나 진입값 복원은 하지 않는다. Select 자체의 예외에도 access를 부르지 않고 기존 증가 깊이를 보존한다.

실제 연결은 token registry가 고정한 `TemplateWriteStream`을 해석하여 `storage.AccessSection(resolve(token),operation)`으로 전달한다. operation은 정확히 한 번, 동기적으로 실행해야 하며 fields/data/memory/operation을 반환 뒤 보관하면 안 된다. 해당 scope의 `memory.buffer`는 Storage의 현재 같은 실제 할당을 읽는다. fields16 사본·`fort.Section()`의 별도 복사 payload·guard가 이미 끝난 때 받은 span으로 교체하지 않는다.

## 실제 Storage 계약과 남는 제약

현재 Storage는 `Stream::accessing`으로 같은 stream의 중첩 Access를 거부하고 Create/Rename의 `RequireNoAccess`가 현재 manager의 구조 변경을 막는다. TypeNames 도중 `FindSection("Scratch")`로 manager.current가 바뀌어도 고정한 기존 token/generation과 실제 fields는 유지된다. 따라서 body 전체를 같은 AccessSection에 넣으면 TYPE/new/진단 callback 동안 pos/end/raw data를 다시 읽는 원문 동작을 같은 실제 저장소에 적용할 수 있다.

이 guard가 모든 파괴를 허용하는 것은 아니다. 현 `Clear/Install→Image::Destroy`에는 같은 RequireNoAccess 검사가 없으므로 provider가 로드 중 해당 manager를 파괴/재설치하면 안 된다. 임의 파괴 callback까지 안전하다는 주장은 하지 않는다. 이 제안은 Storage 정책을 바꾸지 않는다. 또한 원문이 한번 캡처한 hash-data 할당은 뒤에 stream.data가 바뀌어도 검색이 끝날 때까지 살아야 한다. 기존 pointer를 free하고 복사 배열을 반환하는 방식으로 수명을 가장하지 않는다.

## 격리 검증과 실제 검사 초안

전용 경로는 `extracted/cpp-fort-type-names-access-20261024-copy`다. 두 공유 소스의 최초 전체 바이트를 `before/`에 보존했고, 제안은 같은 경로의 사본에서만 적용했다. `prepare.py`가 원래 본문 동일성과 patch를 확인한다. 기존 fixture SHA는 `4c824ec764b85b73ef1d2387c7e6043611b767e983f66abc3f1a98da2d0ef86c`로 그대로다. 원문 실행 도구/fixture/report를 재생성하거나 수정하지 않았다.

최종 build3/run3은 **MSVC C++20/UTF8/MD/W4/WX/O2, 종료0·경고오류0·8검사/실패0**이다. 공유 lib는 읽기만 했고 실제 Storage 구현은382 lib를 사용했다. 공용 빌드나 현재 Session을 링크한 검사가 아니다.

- 기존 원문664 본문/38 별도cleanup 대조와 선택 경계/부분상태의2검사를 사본으로 그대로 실행했다.
- 같은664 fixture의 기대열/trace를 바꾸지 않고 optional access로 실제 두 stream/data 배열을 동기 전달한 별도1검사도 통과했다.664를 새 native 관측으로 중복 합산하지 않는다.
- `FortTypeNamesStorageAccessTests.cpp`의 실제 Storage5검사가 통과했다. 두 헤더가 같은 manager를 빌린 상태에서 Select 뒤 depth/current 순서, 실제 fields/data 주소, current 교체 후 고정 stream pos1, 중첩 Access/Create/Rename 거부, 할당 callback의 실제 end/count 변경 뒤 캡처 count 보존, TYPE callback이 바꾼 실제 hash 재조회, 부분실패의 pos/변환/증가depth 및 guard 반납 뒤 별도cleanup, 빈 섹션의 기존 pointer를 free하지 않는0쓰기를 확인했다.

공용에 추가할 **새6검사**의 사본은 같은 격리 경로의 `AccessFortTypeNamesFixtureTests.cpp`(1개)와 `FortTypeNamesStorageAccessTests.cpp`(5개)다. 첫 파일의 공용 파일명은 `FortTypeNamesAccessFixtureTests.cpp` 등으로 정하고 기존과 같은 `NETSTORM_FORT_TYPE_NAMES_LOAD_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-type-names-load-x86.tsv"` 정의를 붙인다. 두번째 파일은 추가 macro가 없다. 일반 src/tests include 경로만 필요하다. 격리 전용 `LegacyFortTypeNamesLoadTests.cpp`/`FixturePath.h`/`Main.cpp`는 공용 추가 대상이 아니다. 이미 있는 기존2검사와 새6을 합하면8이다.

검사 장치의 `activeFields`와 `activeMemory`는 AccessSection 안에서만 보관하고 성공/예외 양쪽에서 **반환 전에 비운다**. 주소 관측은 비교만 하며 guard 밖에서 역참조하지 않는다. 255BYTE 변환 할당의 검사 소유자는 별도 명시 장치로서 Client 전역 연결 성공을 뜻하지 않는다. 첫 빌드의 forced-include 검색 실패와 두번째 MD/MT 링크 불일치는 각각 build1/build2 로그에 보존했다. 독립 장치의 include 경로와382 lib와 같은 `/MD`만 교정했고 제품 제안/원문 기대는 바꾸지 않았다.

## Client 전체 conversion 소유자는 별도

이 격리 검토 당시 모듈은 `FortTypeNamesLoadGlobals{loadingDepth&,conversion&}`와 allocate(255)/conversionBuffer(token)를 빌릴 뿐 **Client 전체 할당 소유자는 제공하지 않았다**. 격리 완료 뒤 root가 실제 `client/FortTypeConversionStorage`와 `Client::FortTypeConversion()` 수명·services 주입을 작성했다. 읽은 API는 Current/Allocate/Bytes/Release 분리가 아래 계약에 맞지만, **그 새 Client/Session 소유자 코드는 이382 lib 격리 실행에 포함되지 않는다.** 후속 공용 검증에서 TypeNames/Objects/Technology/Deck가 같은 현재 DWORD와 할당을 읽는지 확인해야 한다. Session별 bool/배열/포인터값 복제로 대신하지 않는다. loadingDepth 역시 해당 로더들이 쓰는 같은 실제 DWORD여야 한다.

소유 API는 `Current()->uint32_t&`, `Allocate(255)->token`, `Bytes(token)->실제span`, `Release(token)`처럼 분리하는 방향이 적합하다. Allocate가 임의로 Current를 바꾸거나 Release가 자동으로 Current=0을 쓰면 원문의 대입/free/clear 순서가 달라진다. 각 부모가 원래 위치에서 현재token을 쓰고 free 뒤 무조건0을 쓰는 효과를 구별해야 한다. 빈 TypeNames는 기존 pointer를 free하지 않고0으로 덮으므로 그 이전 할당을 편의상 자동 반납하지 않는다. 기존 pointer 진단608 뒤 new가 계속되는 경우도 이전 할당을 새 배열로 덮어 alias를 잃는 방식이면 안 된다. 이 소유자와 부모별 free/clear 연결은 이번 patch 범위 밖이다.

## parsed 타입 사본의 별도 경계

`FortTemplate::Parse`는 그 시점의 `hasTypeConversion/typeConversion[255]`로 `FortContent.type`과 `FortObject.type`을 **이미 정수 사본으로 계산**한다. `MakeFortMetaContentsHooks`는 현재 원시 Technology span 동일성은 확인하지만, 실제생성에는 이미 파싱된 `fort.technology`를 넘긴다. `RawFortObjectLoad::Contents`는 item.type, `Object`는 object.type을 사용하므로 새 현재 conversion pointer/내용이 바뀌어도 자동 재변환되지 않는다.

`storedType`을 보관했다는 것만으로 해결되지 않는다. 현재 타입에 따라 수량/flags/중첩 목록의 해석 폭과 타입0 조기중단도 정해지므로 마지막 Create 직전에 type 정수만 바꾸는 방식은 원본 전체 읽기 순서를 보장하지 않는다. 후속은 원시 stream을 **현재 conversion으로 해당 부모의 실제 소비 시점에 읽는 경로**를 연결하거나, 사본 경로의 사용 조건/다시 파싱 시점을 명시해야 한다. `FortMetaLoadState::hasTypeConversion` bool을 실제 pointer 소유자나 현재 할당으로 표현해서도 안 된다. 이번 동기 access 완료를 현재 TypeNames 전역·Objects/Technology/Deck 전체 동일성이나 generic Editor 완료로 확대하지 않는다.
