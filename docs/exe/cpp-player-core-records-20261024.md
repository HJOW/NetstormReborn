# Core9기록 metadata와 두 CRT 초기화

2026-10-24 **후속386 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 공유 기존 소스·CMake·LEFT·git·공용 build/lib·GUI는 변경하거나 실행하지 않았다. 원본/CD는 읽기만 했다. 신규 `RawPlayerCoreRecords.h/.cpp`와 검사, 기존 Client 소유자의 **사본 patch**만 준비했다.

## 먼저 PE로 확정한 귀속

**0048f350/44BYTE**는 실제 `00595360+owner×16`에 **subscriber DWORD→Core DWORD0→DWORD4→WORD8**을 쓴다. 기록 크기는16BYTE지만 실제 저장은 **ID4+Core10=14BYTE**이고 끝2BYTE는 보존한다. 원문에 owner0..8 검사나 전체 기록 지우기는 없다. `0048fb30`은 이 장부의 ID를 현재 Player ID와 진단하고 Core10을 Player+6c에 복원하는 별도298BYTE 소비자다. 이번에는 그 큰 복원/Chat 부모를 새로 구현하지 않았다.

**0042ce70/16BYTE**는 같은 Core에 DWORD0/4와 WORD8만0으로 쓴다. **004feac0/39BYTE** CRT 부모는9회 `record+4의42ce70→record.subIdDWORD0→다음16BYTE` 순서다. 실제 CRT 표0052815c가 이 부모를 가리킨다. 다음 슬롯00528160의 **004feaf0/10BYTE**는 별도의 lastKnownCoreData005953f0에42ce70으로 tail-jump한다.9기록144BYTE와 lastKnown10을 같은 캐시라고 부르지 않는다. PE 가상zero 영역에서 시작하므로 실제 첫 CRT 뒤 padding도0이지만, CRT 몸체는 기존 padding을 지우지 않는다.

**0043b540/91BYTE는 이 장부와 무관하다.**00531b08의9DWORD에0..8을 쓰는 소유자 색 번호 표 초기화다. 정확한 기존 저장소는 `RawBridgeConnect.h`의 **BridgeConnectState::ownerColors**이며 43ca70/기존 ChangePlayerColor와4421a0/OwnerColorFrame이 같은 표를 사용한다. cleanup의 Clear43b540을 Core 초기화나 새 장부 생성으로 연결하면 틀리다. 이 함수의6관측은 귀속 감사로만 보존하며 신규 Core 모듈에 포함하지 않는다. literal scan의 sourceHits만으로 실제 연결 완료를 판단하지 않았다.

## 제품 전 Native와 새 모듈

[관측 도구](../../tools/decomp_player_core_records.py)·[최초TSV](../../cpppj/tests/fixtures/player-core-records-x86.tsv)·[보고서](../../extracted/cpp-player-core-records-20261024-native.json)는 신규 제품 세 파일이 없는 상태에서 **96입력×양x87모드=192관측·3090명령**을 고정했다. 위 다섯 실제 PE 몸체만 실행했고 외부 성공 장치/게임/OS는 없다. 양 정밀도 차이는0이다.

Core 관련186관측은 **장부 앞64/기록144/뒤48을 포함한256BYTE 전체, 별도 지역Core16BYTE, 별도 색표44BYTE**를 C++와 대조해 모두 일치했다. 색표6관측은 별도 귀속으로 분류했다. 직접CPU store의 주소/폭/값 trace도 원시로 남겼다. C++ API에 원문에 없는 쓰기 callback을 추가하지 않았으므로 **제품의 trace 동일대조라고 주장하지 않으며**, store 순서는 소스 검토와 부분 물리 검사로 함께 확인한다.

입력은 초기0/a5/ff 패턴, owner0/1/8/9/10000000/ffffffff/fffffffe, ID0/ffffffff/12345678, 서로 다른 Core 값 인자를 포함한다. 넓은 명시 물리에서 owner9의 인접 lastKnown 덮기와 음수 비트 owner의 앞 인접 쓰기, owner10000000의32비트 shift 감김을 관측했다. 이 결과를 정상 게임의 잘못된 owner가 안전하다는 의미로 확대하지 않는다.

[공개 API](../../cpppj/src/o/RawPlayerCoreRecords.h)는 `InitializePlayerCore(span)`, `InitializePlayerCoreRecords(PlayerCoreRecordsView)`, `WritePlayerCoreMetadata(view,owner,subscriber,array10)`다. view는 **같은 실제 bytes와 장부 origin 상대위치**를 빌린다. 정상Client는144BYTE/origin0이고 넓은 Native 인접물리 관측은256BYTE/origin64다. 주소 산술은 unsigned32 감김으로 계산한 뒤 선택한 store 전체폭에서 host span을 검사한다. 물리 밖은 예외이며 앞선 store를 되돌리지 않는다. 원문의 전체잘못된 가상주소를 host 포인터로 꾸미지 않는다.

metadata의 Core는 원문의 DWORD/DWORD/WORD 값 인자처럼 **이미 복제한 array10**이다. 사본은 새 현재 장부가 아니라 호출 인자다. 저장 중 현재 장부를 다시 읽어 이 인자를 바꾸지 않는다. CRT는 각 Core 초기화가 완료되기 전에는 그 record의 subId0을 앞당기지 않는다. 새 모듈은 현재 Core/active/Player/장부 owner를 소유하거나 Session별로 reset하지 않는다.

## Client 장기 소유자 사본과 SavedReader 접합

[확장 patch](../../extracted/cpp-client-core-records-20261024-proposed.patch)는 **기존 ClientCoreDataGlobals.h/.cpp 두 파일과 새 ClientCoreRecordsStorageTests.cpp**만 포함한다. 현재 공유 파일에는 적용하지 않았다. 기존 owner에 고정144BYTE `records_`와 `Records()` 두뷰, `StoreMetadata(owner,id,array10)`를 추가한다. 생성자에서9기록 CRT 후 별도 lastKnown 초기화를 실제 신규 helper로 한 번 수행한다. current/active/lastKnown/빌린 Player의 기존 주소와 선택 API 계약은 유지한다.

후속 root의 SavedReader는 `initializeCore`를 `InitializePlayerCore`에, `metadata`를 **같은 Client::CoreDataGlobals owner의 StoreMetadata**에 연결하면 된다. ReadSavedState 자체의 array10 복제 시점은 이미 원문대로다. Session마다 새 records144를 만들거나 lastKnown10에metadata를 쓰면 안 된다. 실제 reader/File/Player/Core owner의 전체 수명과48fb30 복원/Chat·cleanup은 별도이며 이 patch로 완료했다고 세지 않는다.

이번 patch는 ClientMain/ColorSort 경로를 수정하지 않는다. 앞선 [ColorSort→Chat/Core 적용 오판의 교정](cpp-client-chat-core-merge-correction-20261024.md)과 구별한다. 이 patch의 적용검사는 저장소ROOT의 읽기전용 `git apply --check`이며 사본 결과파일은 실제 격리 컴파일/실행에 사용했다. 종료0만으로 사본 적용을 완료했다고 판단하지 않는다.

## 격리 검증과 등록

첫 **격리 build1 종료0·/W4 관측 경고오류0**, **5검사/실패check0·test1 종료0**이다. 신규 장부3검사와 Client 소유자 **사본2검사**를 실행했고 공용lib는 읽거나 링크하지 않았다. 기존 `RawPlayerCoreData.cpp`만 같은 source로 격리 컴파일했다. Client 전체 생성/Run/GUI/실제 SavedReader 저장복구를 실행한 결과는 아니다.

장부 검사는186 전체 물리 대조, 짧은 Core의4/4/2 store 부분효과·CRT Core 완료 전 ID 보존, metadata 값복제/padding/선택 물리오류를 확인한다. Client 사본 검사는144와lastKnown10의 주소 분리, 실제 metadata의 같은144 갱신, Select0 뒤 lastKnown만 바뀌고9기록은 보존됨, 새 owner와 기존 owner의 분리를 확인한다. 원문/제품 기대를 고치기 위한 실패는 없었다.

다른 작업자가 신규 모듈/독립3검사/Client 사본2검사를 정밀 C 다섯몸체와 읽기 대조해 명백한 의미·수명 오류가 없음을 확인했다. 추가 Native/빌드/실행을 수행한 검토가 아니다.

모든 SHA/최초원문 불변/사본 입력/최종 로그는 [manifest](../../extracted/cpp-player-core-records-20261024-isolated/verification.json)에 보존한다. 공용 등록은 신규 `src/o/RawPlayerCoreRecords.cpp`와 `tests/PlayerCoreRecordsTests.cpp`, **NETSTORM_PLAYER_CORE_RECORDS_FIXTURE**에 최초TSV 절대경로를 공급한다. Client 사본 patch는 root가 따로 적용하고 그 새2검사를 등록한다. 최초 도구/fixture/report와 현재 사용 중인 Core/SavedReader/Cleanup Native 자료는 수정하지 않았다.
