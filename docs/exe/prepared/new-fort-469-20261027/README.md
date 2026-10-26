# NewFort 단위469 독립 준비본

이 디렉터리는 **NewFort 구현 완료물이 아니다.** `004bfba0` 새 manager 부모, `004b3920` 신규 자료 부모, `004b5ca0` 파일 부재 분기의 독립 C++ 초안과 추가 Native 관측을 보관한다. 제품 `src/tests/CMake`에는 등록하지 않았으며 **C++ 빌드·검사·GUI 실행은 하지 않았다.** Root가 단위468을 검증하는 동안 제품 동결을 유지한 작업이다. 기존 fixture와 원시 Early3 시행 두 개는 변경하지 않았다.

`AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 다시 읽었다. 현재 호스트와 마지막 전체 정밀 디컴파일 호스트는 모두 **HJOW-Athlon**, 마지막 전체 디컴파일 날짜는 **2026-10-23**이다. 작업은 2026-10-26에 시작했고 재검증 시 세션 날짜가 2026-10-27로 넘어갔다. 원본 게임·클론 게임 실행은 각각0회, 실제 OS 파일 삭제·할당·클립보드·창 호출도0회다. 보호 원본과 정밀 디컴파일은 읽기만 했다.

## 파일과 재검증

| 파일 | 역할 |
| --- | --- |
| `src/o/RawNewFort.h/.cpp` | 세 실제 원문 부모의 부분 쓰기·호출 순서·현재 owner 재조회다. 선택된 실제 하위 callback이 없으면 예외로 남긴다. |
| `tools/decomp_new_fort.py` | 보호 PE에서 실제 세 부모와 manager ctor/fileName/Core clear를 CPU 에뮬레이션한다. 큰 외부 하위는 명시한 장치 입력이다. |
| `fixtures/new-fort-manager-x86.tsv` | 기존 ptr/존재/argument/두 할당 성공 여부48행이다. 두 manager276BYTE와 header20BYTE·version·fault PC·순서를 전부 보관한다. |
| `fixtures/new-fort-data-x86.tsv` | Editor/known/SP 비트/seed/난수 입력·13개 외부 재진입 효과1248행이다. 두 handle·Core16BYTE·State288BYTE·Territory120BYTE·seed·순서를 보관한다. |
| `fixtures/new-fort-missing-x86.tsv` | owner1/8×외부 효과13종의 파일 부재 정상 반환26행이다. |
| `tools/decomp_new_fort_missing_bits.py`, `fixtures/new-fort-missing-bits-x86.tsv` | 기존 A5 입력의 bit0이 이미1인 한계를 보강한다. bit0=0인 State 전체0/A4/FE×동일26입력78행에서 실제 쓰기 owner를 확인한다. 앞의 세 fixture는 읽거나 쓰지 않는다. |
| `new-fort-native.json`, `new-fort-missing-bits-native.json` | 원본/도구 SHA, 행 수, 관측 범위와 명시 하위의 한계다. |
| `parents-x86.txt`, `native-code-digests.json` | 실제 실행 허용 PE 범위의 BYTE·disassembly·SHA다. 정밀 C에서 분리한 `004b3a30`은 실제 NewData의 연속 tail이다. |
| `tests/NewFortTests.cpp` | Native 전체 BYTE와 trace를 재생하는4개 검사 초안이다. manager ctor는 기존 `ConstructTemplateDataManager`를 실제 호출한다. **미빌드 상태다.** |

보호 입력 SHA256은 다음과 같이 재확인했다.

- `originals/Netstorm.exe`: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`.
- `extracted/refined/originals/Netstorm.c`: `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`.

두 도구를 각각 최초 생성 후 `--verify`로 재실행하여 전체 fixture BYTE가 일치했다.

| fixture | 행 | SHA256 |
| --- | ---: | --- |
| manager | 48 | `edb8e5bb6ef8b048b738d41dcabd74435383f3257313644f1b9d1859d1f23938` |
| data | 1248 | `6119e88b96262dbbe319df077d72802f03d4d9540d1142a96d92abf78de45ce3` |
| missing | 26 | `c01e26f982c3fb9eae1e3e9636e32e2f205d6f5a748d11dc8e4348e17d12001e` |
| missing-bits | 78 | `1a78e64220673bdf374b6c712b4d769930bb35d949dfd854238336a35553f2c4` |

실행 명령은 저장소 루트에서 다음과 같다. 둘 다 실제 게임 프로세스를 만들지 않는다.

```text
python extracted/cpp-new-fort-469-staging/tools/decomp_new_fort.py --verify
python extracted/cpp-new-fort-469-staging/tools/decomp_new_fort_missing_bits.py --verify
```

최초 도구 준비 중 정밀 `functions.tsv`에 없는4BYTE leaf `004bcca0` 때문에 한 번 중단했고, NULL initialize 관측의 허용 범위가 너무 짧아 한 번 중단했다. 두 경우 fixture는 아직 생성되지 않았다. 이후 같은 보호 BYTE leaf와 `0044d870`의 최초9BYTE만 명시 허용하여 실제 NULL 접근 PC를 얻었다. 에뮬레이터의 vtable 첫 DWORD에 넣는 `004bcca0`은 원본 PE `00514690`의 실제 값과 동일하다. 원본 파일 패치는 없다.

## 원문 부모와 정확한 의미

`docs/exe/cpp-new-fort-next-20261026.md`의 신규 자료3·5번 설명을 이 준비본에서 정정한다. **`0042b200`은 현재 CoreData의10BYTE clear이고 기술/Deck 초기화가 아니다. `004beec0`은 Money 섹션 저장이며 Deck 저장이 아니다.** 기존 문서는 동결 범위 밖으로 수정하지 않았다.

| 부모·하위 | 정밀 C 위치 | 실제 PE와 확인 사항 |
| --- | --- | --- |
| NewManager | `Netstorm.c:152892` 이후 | `004bfba0..004bfc54`: header 부분 쓰기·exists 재조회·임시276·삭제·소멸/free·최종276 공개·초기화다. |
| NewData | `Netstorm.c:142702..142774` | `004b3920..004b3aef` 중 실제 허용 두 범위다. 현재 전역 재조회와 kind 캡처를 x86로 대조했다. |
| Missing-file | `Netstorm.c:144537` 이후 | `004b5ca0`의 exists0 분기에서 `004b5e80` 공통 tail을 지나 정상 RET까지 실제 실행했다. |
| manager ctor/name | 기존 `RawTemplateLoad` | `004bcc60`·`004bcca0`: manager276의 지정 필드/파일명과 공유005c89c0=0이다. |
| manager initialize | `Netstorm.c:62573` | `0044d870`: root(+4) 진단0xc9→가상+4 이름 목록→`004a8850(list,index)`가 빈 이름을 돌려줄 때까지 `0044d1b0`로 실제 섹션 생성이다. 이번 manager 관측에서 큰 하위 경계다. |
| 현재 Core getter/clear | `Netstorm.c:114606`, `34563` | `0048f0d0`이 현재00595350을 돌려주고 `0042b200`이 DWORD+0/+4와 WORD+8을0으로 쓴다. Getter 진단은 실제 `ClientCoreDataGlobals::Read`가 맡는다. |
| Money | `Netstorm.c:152219`, `105807` | `004beec0`: Create("Money")→`004832c0`; 후자는 같은 게임 난수 `Next(0x3f2)`와333.0f를 합쳐 실제 float32 4BYTE를 쓴다. |

### NewManager `004bfba0`

1. 고정 header.manager가 비영이면 Template.cpp:0x172 진단을 호출한다. **기존 manager의 반납은 없다.** 앞에서 임의 Clear를 실행하면 원문과 달라진다.
2. header+4=0, +0x10=argument를 쓴다. +8/+0xc의 stale DWORD는 그대로 둔다.
3. 같은 이름에 대한 `0041ab10` 반환이 비영이면276BYTE를 할당하고 ctor를 실행한 임시 manager를 사용한다. 가상+0으로 읽은 **임시 manager의 현재 이름**으로 `0041aef0` 삭제→`0044da60` 소멸→CRT free 순서다. 사전 이름 창에서 파일 부재를 확인했더라도 이 재조회/삭제 경계를 없애지 않는다.
4. 최종276BYTE를 할당하고 ctor를 실행한다. 반환된 token을 header.manager에 먼저 공개한 뒤 같은 token으로 `0044d870`을 호출한다.
5. 임시 할당0은 실제 `004bfbff`의 `[esi]`에서 중단된다. 최종 할당0은 header.manager=0 공개 뒤 실제 initialize 최초 `[edi+4]`, `0044d875`에서 중단된다. 초안은 전자를 물리 접근 예외로 표현하고 후자는 initialize callback에0을 그대로 전달한다.0을 정상 성공으로 숨기지 않는다.

### NewData `004b3920`

1. 같은 MySubscriber00540c74에−1을 먼저 쓴다. Unnamed 번역이 반환된 다음 현재 handle 포인터00540ca8을 다시 읽어 NUL까지 앞으로 복사한다. 설정 `fort.mySubHandle`→제목→등록 마감004b38d0이다.
2. 로컬20×6BYTE를0으로 지운다. **그 시점 Editor가0일 때만** 첫 레코드 BYTE0의 c7마스크/7, PrepareRandom→Next(254)+1의 lowBYTE, BYTE1|1, BYTE2=0x11을 쓴다. 첫 제목 callback에서 Editor가 바뀌는 Native 입력도 있다.
3. 현재 known00595344가 비영일 때만 현재 localOwner00540c70의 실제 SP getter를 호출한다. 아니면 ROM00500fc4의+0.0을 사용한다. 실제 AddSp에 그 음수를 넘기며−0.0도 그대로다. AddSp가 읽을 owner는 SP getter 전에 고정하지 않는다.
4.00595348=1→현재 Core getter0048f0d0→실제10BYTE clear→현재 templateOwner00540cac의 Core writer004beb30이다. Editor의 자연 skip 여부는 **실제 Core writer 부모**가 판단한다.
5. PrepareRandom 후 Next(65536)의 lowWORD를 **0일 때도 같은005c89b4에 써서** 비영이 될 때까지 반복한다. 매 난수 callback 반환 뒤 현재 상위WORD를 유지한다. callback이 상위WORD를 바꾸는 관측과 CoreWrite가 DWORD를 바꾸는 관측을 포함한다.
6. x86 `004b3a48..004b3a65`는 현재 handle **포인터만** 먼저 읽고, 현재 templateOwner, 그 다음 MySubscriber 값을 캡처하여004bd400에 전달한다. 초안 Subscriber hook은 문자열 길이를 미리 계산하지 않고 물리 span을 전달한다.004bd400의 등록helper가 같은 버퍼 내용을 바꾸면 그 뒤 실제 문자열 writer가 새 NUL을 읽어야 한다. 별도 문자열 복사나 진입 전 strlen으로 바꾸지 않는다.
7. 현재 handle 설정→제목→현재 owner의 Territory120BYTE 저장→현재 owner의 **Money 저장**이다. 이 단계들 사이 owner가 바뀌면 다음 호출에서 새 현재값을 읽는다. Money writer가 소비할 `Next(1010)`을 포함한 전체 소비 횟수는 통합 검사에서 확인해야 한다.
8. 현재 Editor를 읽고 현재 owner를 캡처한 뒤 State getter를 호출한다. State+0x11에는 캡처한 Editor가 비영이면2, 아니면0을 쓴다. State callback이 Editor/owner를 바꿔도 이 값과 쓰기 대상은 이미 캡처한 값이다. Notify는 **다시 읽은 현재 owner**로 실행한다.

등록 helper는 기존 `FinishFortSubscriberRegistration`을 재사용할 수 있다. 원문 읽기 키는 `registeredSubId`, 새 ID 설정식 키는 오탈자인 `registerdSubId`다. 현재 기존 모듈이 이 차이를 이미 보존한다.

### 파일 부재 분기 `004b5ca0`

Path→exists0→Path 재조회→현재 owner의 NewManager(path,0)→현재 State 첫BYTE|1→다시 읽은 owner Notify→실제 NewData→**NewData 반환 후 alternate=1**→현재 owner의 Subscriber 읽기004bd450→현재 handle 설정→제목 순서다.

정밀 C의 `00460680(0)`은 인자 추정 오류다. x86은 `push 0; call Path; push eax; call NewManager`로,0은 Path의 인자가 아니라 NewManager의 두 번째 스택 인자로 남는다. 새 API의 `path()`에는 인자가 없다.

초안 `InstallMissingNewFort`의 bool은 **외부 dispatcher의 선택 표시**다. exists 비영이면 false를 돌려 기존 파일 부모에 계속 맡긴다. 원본 전체004b5ca0이 기존 파일에서 false를 반환한다는 의미가 아니다. 기존/잘못된 파일/demo/재시도 부모는 이번 분리 모듈에 포함하지 않았다.

## Root가 연결할 실제 소유자와 남은 child

1. **실제 manager 생성/초기화 공개가 먼저 필요하다.** `CampaignTemplateStorage`의 private `Image`가 실제276BYTE/stream 목록/metadata와 동적 버퍼를 소유한다. 현재 `Image(...,fileImage=true)`는 원문 ctor만 수행하고 빈 목록을 유지하므로 새 manager 할당/ctor 경계에 재사용할 수 있다. 최종 header 공개는 RawNewFort가 해야 하며 callback이 먼저 header를 바꾸지 않는다. 임시 manager도 같은 실제 소유 표에서 소멸→반납해야 한다. 생성자는 같은 `SerializationVersion()`을 빌린다.
2. **0044d870을 빈 callback으로 두지 않는다.** 실제 root 진단→가상 이름 목록→빈 이름까지 순차 Create가 필요하다. `FortTemplate::SectionNames()`의 canonical35개와 기존 `Image::CreateSection`/RawDataManagerResize를 연결할 수 있지만, 원문 이름 목록/반복과 실제 current·head·버퍼 할당·섹션 순서를 새 Native 관측으로 고정해야 한다. 단순 `Install(FortTemplate{})` 또는 `LoadFile`에 조립한 빈 파일을 전달하는 방식은 생성/초기화 순서를 우회한다.
3. 현재 `EditorFiles` 작업본 namespace의 존재·삭제·권한·쓰기를 반드시 빌린다. 보호 원본을 실제 NewManager의 erase에 넘기지 않는다. 실제 삭제 직전 작업본 경로/보호 경계를 다시 확인한다. 이름 창의 exists와 이 부모의 exists는 서로 다른 실제 조회 시점이다.
4. NewData 전역은 `ClientSubscriberGlobals::MySubscriber`, 현재 Session의 실제 handle/PlayerWorld editor·known·두 owner, `ClientCoreDataGlobals::Active/Read`, `ClientRandomGlobals::FortCoreSeed/Game/GameMode`의 **같은 저장소**를 빌린다. 값 복사/고정 seed/Session별 새 GameRandom을 만들지 않는다. SP는 현재 `ClientSpGlobals`와 실제 getter/AddSp 경계를 사용한다.
5. CoreWrite는 기존 `WriteFortCoreData`/실제 Session 바인딩을 재사용한다. Getter0048f0d0의 현재 포인터 진단과 writer의 manager·metamap 진단/Editor 자연 반환을 생략하지 않는다. Territory는 기존 `WriteFortTerritoryRecords`, Money는 `SnapshotFortMoney`, missing tail은 `LoadFortSubscriber`의 실제 저장소 바인딩을 재사용할 수 있다.
6. **Subscriber writer004bd400은 현재 제품에서 별도 복원이 필요하다.** `RawFortSubscriberLoad`는 읽기/등록 마감이며 저장 부모가 아니다. 원문은 고정this의 현재 manager0 진단0x1cf→등록helper004b38d0 재호출→**그 뒤 현재 manager**로 Create("Subscriber")→진입 때 캡처한 ID4BYTE의 일반 writer→004bcbf0 문자열 writer 순서다. 이 문자열 하위는 기존 `RawFortMissionSave`의 실제 문자열 writer를 재사용할 수 있다. ID는 재등록 뒤 현재 global ID로 바꾸지 않는다. 문자열은 같은 입력 포인터에서 등록 후 NUL 길이를 읽는다. 이 짧은 부모를 독립 Native fixture로 고정하는 것이 다음 최소 단위다.
7. **전체 신규 Editor 설치 외곽은 여전히 미연결이다.** 현재 `UberEditor.inc::InstallExistingEditorState`는 먼저 Read/Parse하고 `CampaignInstallMode`는 Campaign/ExistingEditor뿐이다. 없는 파일을 여기에서 가짜 Parse로 통과시키거나 `RawExistingEditorInstallOuter`의 NewFort guard를 완화하지 않는다. 실제 State1 prefix 뒤의 명시 NewEditor 분기와 prepared World/현재 manager·지형 로더 계약이 필요하다. alternate1의 실제 Island0 Exists/populate, 모든 공통 외곽/tail/startupStateCheck, State144BYTE 확정→다음 Poll4→섬count0의 AddIsland **큐 예약**까지 연결해야 사용자 기능이 된다.

## 등록과 다음 검증

제품 채택 시 Root가 `RawNewFort.h/.cpp`를 `cpppj/src/o`에 복사하고 `target_sources(netstorm PRIVATE ...)`에 등록한다. 검사 초안은 `cpppj/tests/NewFortTests.cpp`, 네 fixture는 기존 baseline과 구분된 새 이름으로 `cpppj/tests/fixtures`에 복사한다. `netstorm_tests`에는 다음 정의가 필요하다. 실제 CMake는 이 준비 작업에서 수정하지 않았다.

```text
NETSTORM_NEW_FORT_MANAGER_FIXTURE = .../fixtures/new-fort-manager-x86.tsv
NETSTORM_NEW_FORT_DATA_FIXTURE = .../fixtures/new-fort-data-x86.tsv
NETSTORM_NEW_FORT_MISSING_FIXTURE = .../fixtures/new-fort-missing-x86.tsv
NETSTORM_NEW_FORT_MISSING_BITS_FIXTURE = .../fixtures/new-fort-missing-bits-x86.tsv
```

집중 검사 이름은 `NewFort_ManagerNative48`, `NewFort_DataNative1248`, `NewFort_MissingNative26`, `NewFort_MissingStateBitNative78`이다. 첫 빌드에서는 C++ API/span 변환/경고부터 확인해야 하며 현재4개가 통과했다는 주장은 하지 않는다. Native 재생은 외부 하위의 제품 바인딩 검증을 대신하지 않는다.

이어질 실장 검사는 실제 Storage 초기 canonical 목록/State 확장과 같은 manager alias, 실제 등록ID/Subscriber 저장·읽기, Territory120BYTE, Money의 추가 난수 소비, 실제 Core10BYTE/상위seed 보존, 작업본 파일 생성/기존 대상 재조회 삭제/보호 원본 차단을 함께 확인해야 한다. callback 중단에서는 header 공개·SP 차감·Core clear·seed·State 등의 그 이전 부분효과와 alternate 미확정을 보존한다.

최종 GUI 검증은 메인 Edit→Create New Map→새 이름→원문 NewEditor 설치→자동 Add Island 큐 소비→실제 섬 추가→저장→메뉴→같은 작업본 재로드다. 현재 staging 준비와 Native1322+78행 관측은 이 GUI 성공이나 전체 NewFort/AddIsland 완료로 합산하지 않는다.
