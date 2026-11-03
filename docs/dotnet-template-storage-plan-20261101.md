# dotnetpj Template 저장소 이식 계획 (2026-11-01)

cpppj `client/CampaignTemplateStorage`와 그 소비자를 dotnetpj로 옮기는 여러 세션짜리 작업의 순서와 완료 기준이다. 전체 단계 순서는 [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)을 따르고, 이 문서는 그중 R4의 Template 항목만 다룬다.

단계를 마칠 때마다 4절 표의 상태 칸을 고친다. 재개할 때는 4절에서 첫 `미착수` 단계를 찾으면 된다.

## 1. 현재 위치

| 항목 | 값 |
| --- | --- |
| 이식 완료 | 0.128 원문 계층(`RawMemStream`, `RawDataManager*`, `RawTemplate*`, `RawFortImage`, `RawFortCurrentName`, `RawPlayerTemplateState`, `FortTemplate` 일부), 0.129 `ClientTemplateSectionNames`, 0.130 TS0(`RawSectionBuffer`, 섹션 버퍼 전환·`Clone`, 커서 3종, hook 묶음 setter), 0.131 TS1(`CampaignTemplateStorage`와 `Image` 골격·설치·선택), 0.132 TS2(동적 섹션·State 재생성·최소 확장·쓰기 참조), 0.133 TS3(파일 읽기·Load·저장·이름 부모 연결과 433개 왕복), 0.134 TS4(빈 manager 5개 메서드·`CloneSectionFrom`·`Dump`·`SaveImage`·`ReadLoginImage`와 Core `RawMissionLoginPackets`, `fort-image-packets` 송신·예약 103행), 0.135 TS5(`ExistingEditorTemplateValid`, 0.127에서 미룬 Editor 파일 검사 10개, 한글 작업 경로를 manager에 UTF-8로 보관하고 장치 경계에서 변환), 0.136 TS6(새 맵 부모 `RawNewFort`의 원본 x86 1,400행 일치, `NewFortManagerDevices`·`ClientFortGlobals`·`FortCurrentNameDevices`, 실제 저장소를 쓰는 편집 맵 목록 `SoloMapCatalog`, `HostFortNameConfirmation` 주입 지점), **0.137 F1(코드·검사 작성만, 검사 미실행)**: `RiftTypeFrames`·`RiftTypeDefinition`(.type 파서)·`RiftTypeLoadOrder`·`RiftTypeTable`(패치/CD)·`OriginalConstructors` CD 주소 표·`FortTemplate.Parse` 및 해석 레코드, 0.138 F1 확정(집중 21개·Core 1,146·Client 841, 변이 31종 중 29 검출·2 등가), **0.139 F2a**(Money·Meta 로더와 `Sid`, 집중 8개·Core 1,154·Client 841, 변이 22종 전부 검출), **0.140 F2b**(TypeNames·Deck·Contents·Subscriber 로더, 집중 11개·Core 1,165·Client 841, 변이 20종 중 18 검출·2 등가), **0.141 F2c-1**(TerritoryRecover·PaletteTransition, 집중 4개·Core 1,169·Client 841, 대표 변이 4종+무한 1종 검출·개발 2건), **0.142 F2c-2**(객체 적재 본체와 `MakeFortObjectLoadHooks`, 집중 5개·Core 1,174·Client 841·Assets 366, 변이 4종 검출·개발 3건), **0.143 F2c-3**(객체 스트림과 `MakeFortMetaContentsHooks`, 집중 4개·Core 1,178·Client 841·Assets 366, 변이 4종 검출·개발 6건), **0.144 F2c-4**(저장 5종 MoneySave·MissionSave·RegionSave·SnapshotBasics·SnapshotParent, 집중 11개·Core 1,189·Client 841·Assets 366, 변이 4종 검출·개발 1건), **0.145 F2c-5**(저장 3종 ContentSave·ObjectSave·ObjectsSave, 집중 6개·Core 1,195·Client 841·Assets 366, 변이 4종 검출·빌드 오류 6건 수정), **0.146 F2c-6**(저장 5종 BadgesGlobals·BadgesSave·CoreWrite·SubscriberWrite·SubscriberChange, 집중 15개·Core 1,209·Client 842·Assets 366, 변이 4종 검출·BadgesSave Currents 갱신 누락 1건 수정), **0.147 F2c-7**(저장 1종 TerritorySave와 `ChunkMap` 자료·조회, 집중 6개·Core 1,213·Client 844·Assets 366, 변이 4종 검출·개발 1건) |
| 저장소 본체 | TS0~TS6로 이식 완료(소비자 장치까지). `CampaignTemplateStorage.cpp` 581줄, `.h` 106줄의 저장소와 이를 쓰는 소비자 장치가 검사로 확인된 상태다. 실제 Template을 설치하는 주체(`RawCampaignSession`)가 아직 없어 화면 변화는 F3 이후에 생긴다 |
| 기준 커밋 | 0.128 원문 계층은 `d48ea756`. 2026-11-01 현재 HEAD까지 해당 cpppj 파일과 저장소 파일의 변경은 없다. TS6의 새 맵·이름·목록 파일은 PORT_MAP의 기준 커밋(`51e2d8f4`·`f4493776`·`2c7b3437`·`c4b75d7a`)을 따른다 |
| 이 단계의 원본 fixture | 없다. 저장소는 cpppj가 만든 호스트 어댑터라 원본 x86 대조 대상이 아니다. 그래서 cpppj 검사 직역과 3절의 433개 왕복 검사가 근거다 |

## 2. 범위

이 계획의 TS0~TS6이 끝나면 저장소와 작은 소비자 장치가 검사로 확인된 상태가 된다. **화면에서 보이는 변화는 거의 없다.** 실제 Template을 설치하는 주체는 `RawCampaignSession`(4,609줄)이므로 다음은 F3까지 끝나야 동작한다.

- 미션 Fort 적재
- Edit 진입(Create New Map, 목록 행 선택 뒤 편집)
- Save as의 실제 저장

cpppj 쪽 검사 중 저장소에 의존하는 것은 수작업 분류 기준 약 150개다. 정규식과 include로 분류한 근사치이며, 각 단계에서 실제 목록을 다시 확인한다.

| 묶음 | 검사 수 | 옮기는 단계 |
| --- | --- | --- |
| 저장소 자체 | 33 | TS1~TS4 |
| 소비자 장치 | 26 | TS5·TS6 |
| Fort 적재·저장 모듈 | 약 9 | F2 |
| 세션 통합 | 약 80 | F3 |

이전 문서의 "약 30개"는 저장소 자체 33개만 센 값이었다.

## 3. 설계 결정

### 3.1 버퍼형 `RawSectionBuffer` (2026-11-01 사용자 확정, 7절 1번)

저장소는 `std::vector<uint8_t>`의 `capacity()`를 원본 MemStream의 할당 크기로 쓴다. 이 값은 세 곳에 영향을 준다.

- `SyncStreams`: 용량이 0이면 data 토큰이 0이고, 이것이 `present`(섹션 없음과 빈 섹션의 구별)를 정한다. 파일로 읽은 빈 섹션은 용량 1이라 존재하고, 값형 설치의 빈 섹션은 용량 0이라 없다.
- `CompactTail`: `end==alloc`이면 압축을 건너뛴다.
- `AccessSection`: 쓰기 전에 크기를 alloc까지 다시 늘린다.

cpppj 검사 4개도 `capacity()`를 직접 확인한다. 따라서 크기와 용량을 따로 갖는 형이 필요하다. `Netstorm.Core/Original/RawSectionBuffer.cs`에 둔다.

| 멤버 | 대응 | 규칙 |
| --- | --- | --- |
| `Count`, `Capacity` | `size()`, `capacity()` | `Capacity`는 내부 배열 길이다 |
| `Span`, `Memory` | `data()`+`size()` | 앞 `Count`바이트만 빌려 준다 |
| `Resize(n)` | `resize` | 용량 안이면 재할당하지 않고 늘어난 구간만 0으로 채운다. 넘으면 `max(용량+용량/2, n)`(MSVC 규칙)으로 재할당한다 |
| `Reserve(n)` | `reserve` | 크기를 유지한다 |
| `Clear()` | `clear` | 크기만 0, 용량 유지 |
| `Release()` | `vector{}.swap(v)` | 크기·용량 모두 0 |
| `TakeFrom(other)` | 이동 대입 | 배열을 넘기고 `other`는 용량 0이 된다 |
| `FromBytes(bytes)` | 범위 생성 | 용량이 크기와 같다 |

검토한 대안은 다음과 같다.

- `List<byte>`: `Memory<byte>`를 얻을 수 없어 커서를 연결하지 못한다.
- 저장소가 용량을 별도 표로 관리: `Template(slot)`의 `sections`와 어긋나고 검사가 직접 만드는 입력을 표현하지 못한다.

cpppj 제품 코드에서 `.sections`를 고치는 곳은 `Template.cpp`와 저장소뿐이고, `RawFortImage`와 `app/main.cpp`는 읽기만 한다. 그 밖의 변경은 검사 입력이다. 저장소 코드를 읽어 본 바로는 재할당이 용량 0에서만 일어나므로 1.5배 규칙은 검사 입력에서만 작동할 것으로 본다. TS0에서 그대로 옮겨 두면 이 판단이 틀려도 결과가 같다.

### 3.2 C++와 달라지는 곳

| C++ | C# | 이유·주의 |
| --- | --- | --- |
| `std::map<uint32,...> streams` | `SortedDictionary<uint, Stream>` | `SyncStreams`·`LoadCursor`가 핸들 오름차순 순회에 의존한다 |
| `FortTemplate` 값 전달 | 소유권 이전, 필요한 곳만 `Clone()` | C#은 참조라 같은 객체를 두 번 설치하면 공유된다 |
| 커서 구조체 참조 반환 | 클래스 인스턴스를 Image가 평생 소유 | 검사가 `&a==&b`로 동일성을 확인한다. 교체하지 않고 필드만 초기화한다 |
| `hooks.list=ListEffects()` 등 묶음 대입 | 묶음 속성 8개를 `{ get; set; }`으로 연다 | 0.128에서 get 전용으로 만들었다. `DataManagerDumpHooks.Selection`, `DataManagerImageHooks.Streams`, `DataManagerFileHooks.Image`, `DataManagerListHooks.Lifetime`, `DataManagerResizeHooks`의 3개, `TemplateLoadHooks.Lifetime` |
| 람다의 `span` 캡처 | 전용 대리자, 또는 진입 시 배열로 복사 | C#은 Span을 캡처할 수 없다. `WriteStream`의 입력 복사는 입력이 대상 버퍼와 겹치지 않을 때만 동등하므로 소비자를 옮길 때 겹침 여부를 확인한다 |
| `.at()`의 `out_of_range` | 명시 검사 뒤 `ArgumentOutOfRangeException` | 사전 조회 실패가 `KeyNotFoundException`으로 바뀌지 않게 한다 |
| 무효화된 span은 UB | 옛 배열이 살아 있어 쓰기가 조용히 사라진다 | C#이 더 위험하다. 재할당을 일으킬 수 있는 호출을 넘어 Span을 들고 있지 않는다 |
| 파일 이름 `std::string`(UTF-8 바이트) | 호스트에는 실제 문자열(UTF-16), manager+c에는 UTF-8 바이트 | Core `RawTemplateLoad`는 문자 하나가 바이트 하나인 표기(Latin1)라 한글·이모지 경로를 그대로 담지 못한다. 저장소의 `TemplateFileNames`가 장치로 나가는 지점(읽기·readonly·이름 비교·서명 거부 요청·권한·쓰기)과 들어오는 지점(설치·Load 요청)에서 변환한다. 이름 한도는 UTF-8 바이트 수 256 미만이다. 짝이 안 맞는 UTF-8(옛 ANSI 이름)은 왕복하지 않는다 |

토큰 값(`0x10000000+핸들` 버퍼, `0x20000000+핸들` 임시, `0x30000` 파일 입력, `0x50000` 이미지)은 그대로 쓴다. 파일은 `partial class`로 나누고 위치는 `TemplateFileHooks.cs`와 같은 `Netstorm.Client/Host/`다.

### 3.3 `FortTemplate.Parse` 없이 진행

저장소는 파싱된 `FortTemplate`을 받기만 하고 `Parse`를 호출하지 않는다. 저장소 전용 검사 33개 중 12개가 실제 fort를 `Parse`로 만들지만 해석 필드(Money·Chaff·Deck 등)를 읽지 않음을 확인했다. 이 12개는 `FortTemplate.ReadSections`로 대체한다.

## 4. 단계

한 단계가 커밋 단위 하나다. 앞 단계가 끝나야 다음 단계를 시작한다.

| 단계 | 내용 | 검사 | 상태 |
| --- | --- | --- | --- |
| **TS0 선행** | `RawSectionBuffer`와 단위 검사, `FortTemplate.Sections`를 `List<RawSectionBuffer>`로 전환(`ReadSections`·`Section`·`RawFortImage`·`Clone`), hook 묶음 8개 개방, 커서 `FortTerritoryStream`·`FortMoneyCursor`·`FortMetaStream` | 기존 Core 1,101개 유지 + 버퍼 규칙 직접 검사 | **완료(0.130)**: Core 1,117개(새 16)·Client 731개, 변이 24종 검출 |
| **TS1 설치·선택** | `Image` 골격(노드·`Buffer`·`SyncStreams`·`PublishCursor`·`RetireCursor`·`Destroy`), `Install`·`Borrow`·`Clear`·`ClearAll`·`Template`·`State`·`Territory`·`Name`, `GrowState`, 모든 `Select*`, `Money`·`Badges`·`Technology`, `SectionOrder`·`CurrentSection`·`ReadSubscriberName`·`FileHeader` | cpppj 6개 | **완료(0.131)**: Client 19개(cpp 검사 7 + 직접 12), 변이 36종 중 35 검출·1 등가 |
| **TS2 동적 섹션** | `WithMemory`·`WriteStream`·`AccessSection`·`CompactTail`, `CreateSection`·`InitializeState`·`RenameSection`·`EnsureStateSize`, 쓰기 참조의 생성 수명 | cpppj 10개(용량 확인 4개 포함) | **완료(0.132)**: Client 18개(cpp 검사 11 + 직접 7), 변이 33종 중 31 검출·2 등가 |
| **TS3 파일** | `ReadFile` 두 경로·`BindFileStreams`, `InstallFile`·`LoadFile`·`SaveFile`·`BindCurrentName`. 가장 크므로 필요하면 읽기와 저장으로 나눈다 | cpppj 14개 + 433개 fort 왕복 | **완료(0.133)**: Client 19개(cpp 검사 13 + 왕복 1 + 직접 5), 433개 모두 왕복 일치, 변이 23종 중 19 검출·4 등가 |
| **TS4 빈 manager·로그인** | `AllocateEmptyManager` 계열, `CloneSectionFrom`·`Dump`·`ReadLoginImage`, `RawMissionLoginPackets` | cpppj 3개 + `fort-image-packets` 송신·예약 103행 + 패킷 경계 1개 | **완료(0.134)**: Core 2개(송신·예약 103행 + 경계)·Client 8개(cpp 검사 4 + 직접 4), 103행 모두 일치, 변이 34종 중 30 검출·4 등가 |
| **TS5 Editor 파일** | `ExistingEditorTemplateValid`, `EditorFortFileDevices.BindLoad`의 실제 사용, 한글 작업 경로를 manager에 UTF-8로 보관(`TemplateFileNames`) | 0.127에서 미룬 10개(`EditorFileWriter` 1, `EditorFiles` 5, `EditorFortFileDevices` 4) + 직접 4개 | **완료(0.135)**: Client 14개(cpp 검사 10 + 직접 4), 변이 31종 중 28 검출·3 등가 |
| **TS6 이름·새 맵·목록** | `RawNewFort`·`NewFortManagerDevices`, `ClientFortGlobals`·`FortCurrentNameDevices`, `SoloMapCatalog.Probe`를 실제 저장소로 교체, `HostFortNameConfirmation` 연결 범위 판정 | cpp 검사 19개(`NewFortManagerDevices` 5, `FortCurrentNameDevices` 7, 목록 6, `EditorCatalogFiles` 1) + `RawNewFort` 원본 x86 4종 1,400행 + 직접 15개 | **완료(0.136)**: Core 6개(x86 1,400행 일치 4 + 직접 2)·Client 32개(cpp 검사 19 + 직접 13), 변이 66종 중 63 검출·3 등가 또는 관측 불가 |

**433개 왕복 검사(TS3):** 검사 데이터 폴더의 `d/*.fort` 전체(`originals/d` 기준 433개)를 `InstallFile`로 읽고 `SaveImage`로 다시 쓴 뒤 `ReadSections`로 섹션과 flag를 비교한다. 기대값이 입력 파일 자체에서 나오므로 cpppj 없이 전 파일을 덮는다. 어긋나는 파일은 실패로 묶지 말고 원인을 먼저 분류한다(뒤꼬리 바이트, 비표준 flag, 0x8000 초과 등).

**TS1이 큰 이유:** 선택 없이는 노드를 관측할 수 없어 설치와 선택을 한 단계로 묶었다. `Borrow`·`SectionOrder`·`CurrentSection`·`FileHeader`·`GrowState`는 cpppj 제품 코드에 호출자가 없지만 검사가 쓰므로 옮긴다.

**TS6의 결과·한계(0.136):** `Client` 전체(`ClientMain`)는 옮기지 않았다(F3). 이름 장치 검사는 `ClientFortGlobals`·설정·저장소·파일 장치를 장면이 직접 조립해 7개를 모두 옮겼다. 목록 검사 3개(`DiskEnumerationAndArchiveRead`·`CurrentSubscriberSection`·`TellMetadataUsesActualFilesAndCurrentNames`)는 같은 경계를 이미 다른 C# 검사가 확인해 옮기지 않았다(사유는 단위 기록 0.136). `HostFortNameConfirmation`은 세션이 정하는 Template 소유자 헤더와 현재 이름 설정 장치를 선택 인자로 받게 했고 실제 연결은 F3이다. `TemplateDiagnosticClientTests` 1개와 `CampaignNewFort*Tests.inc` 4개는 `ClientDiagnostics`·`RawCampaignSession`이 필요해 F3으로 넘긴다.

### 뒤이은 트랙

TS 단계 다음이며 R4의 본체다. 이 문서는 범위만 적고 세부는 착수할 때 정한다.

| 트랙 | 내용 | 검증 |
| --- | --- | --- |
| **F1 타입 표·Parse** | `RiftTypeTable`(347줄)·`RiftType.cpp`(69줄)·`.type` 파서 `TypeParser.cpp`(244줄)·`TypeLoadOrder.cpp`, `FortTemplate.Parse`와 객체·내용물·덱 해석 | 원본 x86 fixture(기준점·기본 비용·techBit·생성자 주소), 실제 .type/.fort 전체 대조, 기존 경로 파서와의 교차 비교. **0.137에서 코드와 검사를 작성했으나 검사는 아직 실행하지 않았다(재개 지점은 검사 첫 실행).** 나중에 cpppj `--dump-types`·`--inspect-fort` 출력과 비교할 수 있다 |
| **F2 Fort 적재·저장** | `o/RawFort*.cpp` 29개 2,083줄 중 남은 26개 1,897줄(Object·World·Money·Meta·TypeNames·Snapshot·Subscriber·Territory 등). F2a에서 Money·Meta 2개, F2b에서 TypeNames·Deck·Contents·Subscriber 4개, 0.141에서 TerritoryRecover·PaletteTransition 2개, 0.142에서 ObjectLoad 본체·`MakeFortObjectLoadHooks`, 0.143에서 ObjectStream·`MakeFortMetaContentsHooks`, 0.144에서 F2c-4 저장 5종(MoneySave·MissionSave·RegionSave·SnapshotBasics·SnapshotParent), 0.145에서 F2c-5 저장 3종(ContentSave·ObjectSave·ObjectsSave) 완료(아래). 0.146에서 F2c-6 저장 5종(BadgesGlobals·BadgesSave·CoreWrite·SubscriberWrite·SubscriberChange) 완료(아래). 0.147에서 F2c-7 저장 1종(TerritorySave)과 `ChunkMap` 자료·조회(`ChunkEntry`·`IslandList`·`Bounds` 포함 전체) 완료(아래). 나머지는 F2c 나머지·F2d(World·저장 나머지)부터 | 각 모듈의 원본 fixture, 저장소 의존 검사 약 9개 |
| **F3 세션 연결** | `RawCampaignSession`에서 Template 경로, `ClientStateWorld*`의 `ClearAll`, `UberGump`의 로드·이름표·진단 연결 | 세션 통합 검사 약 80개, 0.5의 이정표 M1 |

F1은 TS와 독립이라 먼저 해도 된다. (0.137에서 F1을 TS6 다음에 작성했다. 4절 표대로 실행 확인 전이라 완료로 세지 않는다.) 판본은 패치/CD 둘을 옮겼다(CD 몫이 작고 fixture에 CD 행이 있다). 기존 `TypeCatalog` 어댑터는 만들지 않았고 기존 경로의 `.type` 파서는 교차 비교용으로만 읽는다. 다만 F2가 F1과 TS4까지를 모두 요구하므로 전체 순서는 바뀌지 않는다.

## 5. 매 단계 공통 절차

1. `python dotnetpj/tools/cpp_sync.py`와 `--ports`로 cpppj 변경을 확인한다. 다른 세션이 cpppj를 수정 중일 수 있으므로 HEAD 커밋만 기준으로 삼는다.
2. C#을 옮기고 cpppj 검사를 직역한다. 빠뜨린 검사는 사유와 함께 단위 기록에 남긴다.
3. 변이 검사를 한다. 0.128에서 10종 중 3종이 처음에 미검출이었으므로, fixture가 없는 이 계층에서는 분기마다 직접 검사가 있는지 특히 확인한다.
4. Core·Client 전체 Release 검사를 돌린다.
5. `PORT_MAP.tsv`(원본 fixture가 없으므로 상태는 `port`), 이 문서 4절, `LEFT_JOBS.dotnetpj.md`의 0.1·0.9와 단위 기록을 갱신하고 커밋한다.

## 6. 하지 않는 것

- `cpppj/`와 `LEFT_JOBS.md`를 고치지 않는다. 필요한 것은 사용자에게 요청한다.
- 기존 `Netstorm.Assets.FortFile`은 건드리지 않는다. 은퇴는 F2 이후다.
- 저장소를 C#답게 재설계하지 않는다(결정 2). 3.2절의 차이만 허용한다.

## 7. 확정된 결정 (2026-11-01 사용자 결정)

계획서가 제시한 기본값 네 가지를 모두 그대로 진행하기로 확정했다. 아래 내용을 바꾸려면 사용자 확인이 다시 필요하다. TS0는 이 결정에 따라 0.130에서 마쳤다.

1. **`RawSectionBuffer` 설계(3.1절) — 확정.** 3.1절의 형식대로 만든다. TS0에서 `FortTemplate.Sections`를 `List<byte[]>`에서 `List<RawSectionBuffer>`로 바꾼다.
2. **F1의 타입 표 경로 — 확정.** `RiftTypeTable`을 직역한다. 기존 `TypeCatalog`로 `Parse`를 먼저 완성하는 어댑터는 만들지 않는다(타입 표의 근거가 둘이 되어 구조 결정 2·3과 어긋난다).
3. **`FortTemplate`의 위치 — 확정.** F1에서 `RiftTypeTable` 위치를 정할 때 함께 확정하고, 그때까지 `Netstorm.Core/Original`에 그대로 둔다.
4. **저장소의 cpppj 대조(L2) — 확정.** cpppj 변경을 요청하지 않고 TS3의 433개 왕복 검사로 대신한다. cpppj에 manager·노드 덤프 명령이 생기면 그때 같은 입력의 상태 비교를 더한다.
