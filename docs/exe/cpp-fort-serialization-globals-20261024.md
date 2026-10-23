# Fort 직렬화 버전 DWORD의 Storage·Catalog 공유

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS·공통/닷넷 인수인계를 재독했다. 이번 하위 작업은 실제 직렬화 전역 소유권과 Storage·Catalog 연결, 새 검사와 이 문서를 소유한다. 원본/CD/디컴파일 변경과 새 게임/GUI 실행, 공유 빌드, CMake/Session/Client/기존 검사/LEFT/git 변경은 수행하지 않았다. Session·Client 바인딩과 공유 빌드·실행 검증은 root의 별도 통합 범위다.

**최종 검증 결과:** root의최종제품/검사빌드는종료0/관측경고0,관련68검사/실패0,전체CTest는 **2446검사/실패check0·종료0·331.93초(CTest전체331.98초)**다. 같은게임제품의Early1자연완주와Early6/훈련9/캠페인16/Demo3 **34/34진입**을확인했다. 게임SHA는 `e679808354853f6ccd8895fcc8db8ffb943f268f50415ff699ffb36ce191e43e`,최종검사exe SHA는 `aaf723b33c6028d65a444c68024872b0319fdd549b8b55d61bf218cb654cd887`다. 아래단독구문검사는하위작업자의당시범위이며이최종공유실행과구별한다.

## 원본의 같은 DWORD

10.78 `004bcc60`의 현재 정밀 원문은 `Netstorm.c:150406`이다. 실제 PE를 읽기 전용으로 역어셈블해 `004bcc89: mov dword ptr [0x005c89c0],ebx`를 확인했다. EBX는 그 앞 `004bcc67`에서 0으로 만든다. 순서는 manager+4/+8=0 → vptr → +10c=0/+110=ffffffff → **같은005c89c0=0** → +c 파일명 첫 NUL까지 복사이다. 기존 `ConstructTemplateDataManager`의 쓰기 순서는 변경하지 않았다.

전체 Objects 부모 `004be430`의 DWORD2 쓰기, Terr/Chaff와 단일 객체/내용물의 현재 버전 읽기, 객체 로더의 현재 버전 쓰기는 모두 같은005c89c0을 사용한다. 기존 `CampaignTemplateStorage::managerGlobal_[4]`와 Session의 값형 `objectState.version`은 물리적으로 다른 저장소였다. 더구나 목록 분류용·메타용 `MapProbe`도 각각 다른 Storage를 만들어, 메인 Storage/Session만 연결하면 임시 manager 생성의0쓰기 효과가 전달되지 않았다.

CD 10.72의 관련 객체 버전 주소는 기존 분석의005178dc이다. 이번 새 소유권 연결은10.78 Client 경로이며 CD manager 생성자/목록의 실행 동일성까지 확장하지 않는다.

## 소유권과 연결 API

`o/FortSerializationGlobals.h`는 **실제 `std::uint32_t version` 한 개**만 소유한다. 과거 BYTE배열을 DWORD 참조로 역캐스팅하지 않는다. Image 생성자에 실제 DWORD 참조를 전달하고 그 객체의4BYTE 뷰를 기존 `ConstructTemplateDataManager`에 빌려주므로, 기존 원본 몸체가 쓰는0이 바로 같은 version 객체를 갱신한다.

- `CampaignTemplateStorage(shared_ptr<o::FortSerializationGlobals> = {})`: 공급된 한 객체를 소유권 공유한다. 생략한 독립 Storage는 자기 객체를 한 개 만든다.
- `CampaignTemplateStorage::SerializationVersion() -> uint32_t&`: 같은 객체의 현재 DWORD 참조를 반환한다. Storage/조회 생성자나 접근자가 공급된 version을 임의로0으로 만들지 않는다.
- `SoloMapCatalogEffects::serializationGlobals`: Client의 같은 shared_ptr를 공급한다. 생략한 독립 Catalog는 자신의 Impl 수명에 한 객체를 만들고, 분류용 `Kind`와 행 메타용 두 `MapProbe::storage`에 똑같이 전달한다.

Storage의 `Install`/`InstallFile`이 실제 새 Image를 생성할 때만 기존 manager 생성자의0쓰기가 발생한다. 파일 파싱이 그 뒤 실패해도 앞선0쓰기를 되돌리지 않는다. 반면 `CreateSection`/`WriteSection`/`RenameSection`/`Borrow`/`Clear`/`ClearAll`/C++ 소멸은 manager 생성자를 추가 호출하지 않으며 version을 임의 초기화하지 않는다. 단위377의 동적 Temp/rename/노드 generation·해제 검사 구현은 바꾸지 않았다.

root는 같은 Client의 globals를 주 Storage와 Catalog에 공급하고, `FortObjectLoadState`의 필수 version 참조를 Storage의 `SerializationVersion()`에 연결한다. 그 전역을 값으로 복사하거나 저장 전후에 동기화하는 방법으로 대신하지 않는다. 이 문서에서 직접 수행한 변경은 Storage·Catalog 다섯 제품 파일이며 root의 바깥 소비자 변경은 별도로 읽기 검토했다.

## 새 검사 계약과 현재 증거

`TemplateSerializationGlobalsTests.cpp`는 fixture 매크로 없이 네 검사를 등록한다. 원본 파일을 사용하지 않는 합성35섹션 Fort를 **실행 cwd의 `test-data/template-serialization-globals/shared.fort`**에만 만든다.

| 검사 | 직접 확인하는 계약 |
|---|---|
| SharedAddressAndManagerConstruction | 두Storage와globals의실제참조주소동일,Storage생성시비0보존,Install/InstallFile의0쓰기,파일거부뒤앞선효과,외부shared_ptr반납뒤수명 |
| SectionsAndClearPreserveVersion | Temp→Terr00동일노드/버퍼생성수명,오래된대상거부,Create/쓰기/rename/빌림/Clear/소멸의버전보존 |
| CatalogBothTemporaryManagers | 분류와행메타각Image생성직전에다른sentinel을쓰고직후같은ClientDWORD의0을관측,직접Kind와캐시재사용구별 |
| SeparateClientsAndDefaultOwners | 별도Client/기본독립Storage주소와값분리,별도기본Catalog비간섭,파일부재로Image생성에미도달한경로의비0보존 |

Catalog 검사는 제품 내부값을 노출하거나 테스트용 접근자를 추가하지 않는다. 기존 실제 `fileSpecKey`/`typeCount`/`userName` 경계에서 같은 Client 참조를 관측한다. 분류 생성 직전 `0x22222222`, 메타 생성 직전 `0x44444444`를 쓰므로 두 임시 Storage 중 어느 하나라도 다른 전역을 사용하면 다음 관측이 실패한다. 메타의0 확인 뒤 `0x55555555`를 남겨 조회/캐시/임시 소멸의 불필요한 재초기화도 구별한다. 기본 Catalog의 한 객체 배치는 구현을 읽어 확인했으며 private 임시 Storage의 주소를 직접 관측했다고 주장하지 않는다.

하위작업자는새검사.cpp만 `extracted/cpp-fort-serialization-globals-20261024-isolated`에서MSVC19.51 x64 `/std:c++20 /utf-8 /W4 /WX /O2 /c`로구문컴파일해종료0/관측경고0을확인했다. `compile.rsp/.bat/.log/.json`을보존했고기존ABI정적라이브러리를새참조ABI와혼용하지않았다. 이후root가아래공유실행을완료했다. 원본PE재실행fixture를새로만들지않았으며원본근거는정적PE/정밀C와기존`TemplateLoadTests`의생성자검사다.

전체 snapshot/Money/Mission/Badges/Notify, generic Editor/작업본/메뉴 연결·사용자 전체 완주와 외관/음향 동일성은 이 전역 연결만으로 완료되지 않는다.

## 단위379 최종 공유 실행

root의새Release제품 `build-game1`은종료0/관측경고·오류0이고SHA-256은 `e679808354853f6ccd8895fcc8db8ffb943f268f50415ff699ffb36ce191e43e`다. 같은제품의 `extracted/cpp-fort-version-20261024-natural1/results.json`을읽어상태주입없는Early1완주재생을확인했다. A~G/SP0→200→400→600/사제21위치/이동명령1회/완료게임99.894초/다음Tutorial2·Guide4·SP10000이고실행173.28초/종료0/상한미도달이다. 초기Done1→1보존,`initialProgressInjected=false`,`persistedProgressVerified=false`,원본1394파일manifest동일이다. 새진행저장·다음미션완주·전체동일성증거로확대하지않는다. 이GUI는root가실행했으며하위작업에서새로게임을실행하지않았다.

최초관련68검사는67통과/새`SectionsAndClearPreserveVersion`1실패였다. 원본tail압축(capacity2)뒤pos2에2BYTE를append한검사입력이실제용량진단을일으켰다. root는제품을바꾸지않고`SelectSection("Terr00",1)`의실제pos0뒤동일옛참조로덮어쓰도록그입력만교정했다. `build-tests2`종료0/경고0뒤새4검사/실패0과기존64검사를합쳐관련고유68/0을확인했다. 최초실패/교정로그는보존한다.

최종전체CTest로그 `extracted/cpp-fort-version-20261024-ctest.log`는종료0/331.93초(전체331.98초)이고`...-ctest-internal.log`의내부 **2446 test(s),0 failed check(s)**도직접읽어확인했다. `...-solo-all/results.json`의Early6/훈련9/캠페인16은31/31,`...-demos/results.json`의Demo3은3/3으로 **합계34/34**다. 양보고서모두complete/originalsUnchanged=true이고모든실행의제품SHA도위와같았다. 이34진입과단독Early1자연완주를전체사용자완주·전체UI/음향동일성으로확대하지않는다.
