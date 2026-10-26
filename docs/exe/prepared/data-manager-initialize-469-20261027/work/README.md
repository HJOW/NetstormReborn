# 공유 이름표·빈 Image 초기화 구현 제안 인수인계

이 work는 **제품 미적용·C++ 미빌드·검사 미실행** 상태다. 사용자 중지 지시에 따라 현재 작성한 범위만 정리해 Root에 인계한다. 후속 NewEditor/모달/실제 GUI 작업은 착수하지 않았다. 제품 Source5와 기존 baseline/fixture, 보호 원본·정밀 디컴파일은 수정하지 않았다. 원본 게임·클론 게임·빌드 실행은 모두0회다.

현재 호스트와 마지막 전체 정밀 디컴파일 호스트는 HJOW-Athlon/2026-10-23로 일치한다. AGENTS 및 공통/닷넷 인수인계를 다시 확인했다. 동결한 부모 NewFort1400행/모듈은 `../../cpp-new-fort-469-staging` 그대로다. 이 work의 근거는 상위 `../fixtures`의 **Native76행(64+12)**이며 그 파일들을 바꾸거나 다시 생성하지 않았다. 실제 검증 범위는 상위 README와 `initialize-native.json`을 따른다.

## 제안 범위

| 준비 파일 | 역할 |
| --- | --- |
| `src/o/RawSnart.h/.cpp` | 기존 RawDataManagerImage::Name 몸체를 공통화한다. 제자리 slash→NUL/끝NUL→FF, 같은 포인터/현재 이름 조회를 유지한다. |
| `src/o/RawDataManagerInitialize.h/.cpp` | root 진단→현재 이름표 한 번 조회→같은 표의 가변 이름 재조회→고정 manager Create→마지막 빈 이름 반환이다. Create에는 사전 strlen 대신 같은 물리 span을 전달한다. |
| `src/o/RawTemplateSectionNames.h/.cpp` |005c8aa4의 최초512할당·포인터 공개·기본136BYTE/20회서식/마지막277BYTE 작성과 비영 현재 포인터 재사용이다. 나머지235BYTE를 읽거나 쓰지 않는다. |
| `src/client/ClientTemplateSectionNames.h/.cpp` | 실제 `new(std::nothrow) uint8_t[512]`의 **값 초기화 없는** 한 할당을 Client 수명에 소유한다. 실제 실패는 예외이며 성공한 빈 표로 바꾸지 않는다. 검사에서만 A5로 알려진 allocator를 주입한다. |
| `src/client/CampaignTemplateStorage.h/.cpp` | 공유 owner 주입/ReadFile의 지역 이름 vector 제거/실제 빈 Image allocate→원문 ctor→고정 manager 초기화의 분리 API다. |
| `src/client/SoloMapCatalog.h/.cpp`, `src/client/UberGump.cpp` | 두 임시 MapProbe도 주 Storage의 같은 이름표를 빌리게 하는 좁은 연결이다. Uber.h에 중복 owner는 만들지 않는다. |
| `tests/DataManagerInitializeTests.cpp` | Native64/12 재생, lazy·가변표·미정끝 보존/할당0, 실제 빈 Image와 기존 보호 TEST01 reader의 같은 표, 실제 목록 두 Probe의 같은 표 검사 초안6개다. **미컴파일·미실행**이다. |

빈 Image는 ctor 전 실제 manager 할당을 별도 소유한다. metadata의 크기 확장이 SyncStreams에서 아직 만들지 않은 노드를 선행 생성하지 않도록 초기 seeded를 true로 둔다. 실제35개 노드는 RawDataManagerInitialize의 반복에서 Image의 기존 RawDataManagerResize Create 부모로만 생성한다. 기존파일 Install/LoadFile의 동작을 새 빈 파일 조립으로 대신하지 않는다.

Shared owner는 주 `CampaignTemplateStorage`가 생성하고, `SectionNamesOwner()`를 통해 `SoloMapCatalogEffects.sectionNames`로 전달한다. 독립 catalog는 자기 수명에 owner 하나만 만들고 두 Probe가 공유한다. Session별 reset/재변환을 추가하지 않았다. 이름표 첫 할당 이후 파일 reader/신규 초기화/다음 파일 reader는 같은 주소와 변환 상태를 보게 된다.

원문 `004437f0`은 실제 빈 RET이므로 별도 빈성공 callback을 요구하지 않았다. Native 재생 trace에서는 이 **`baseNoop` 항목 하나만 제외**한다. 서식20회는 raw 부모의 선택된 실제 hook으로 남겼고 Client는 원문 선택 범위0..19의 `Terr%02d` ASCII/NUL을 정확한7BYTE로 작성한다. Native trace의 `at`은 공개 helper 안에 observer를 넣지 않기 위해 Create 진입/마지막 정상 반환에서 같은 순서의 무효과 관측으로 기록한다. 전체512BYTE·manager·현재 token·실제 Create 문자열도 독립 기대값과 대조하도록 준비했다.

## Storage API와 NewFort 바인딩

- `AllocateEmptyManager(276)`은 실제 미생성 Image를 소유 표에 넣고 token/0을 반환하며 header를 쓰지 않는다.
- `ConstructEmptyManager(token,name)`는 같은 manager276BYTE와 기존 공유 SerializationVersion에 원문 ctor를 실행한다. header 공개는 RawNewFort 부모가 맡는다.
- `ManagerBytes(token)`는 이미 고른 같은 manager 물리를 빌린다. 파일명 getter는 기존 `TemplateDataManagerFileName`을 그대로 사용할 수 있다.
- `InitializeEmptyManager(token,assertion)`은 그 고정 manager에 실제 초기화 부모를 실행한다. root/이름 separator 진단이 선택되면 callback을 요구한다.
- `ManagerLifetime()`은 같은 기존 Effects의 실제 destroy/release를 나누어 빌린다. 임시 manager와 최종 manager를 별도 owner에 조립하지 않는다.

Root가 NewFort를 연결할 때 RawNewFort hook의 allocate/construct/fileName/destroy/release/initialize에 위 API를 바인딩한다. assertion/exists/erase는 실제 작업본 `EditorFiles` 장치가 제공해야 한다. 이미 열린 manager를 임의 Clear하거나 같은 이름의 기존 파일 삭제 재조회를 제거하지 않는다.

state_recovery의 별도 `cpp-fort-subscriber-write-469-staging`에는 `CreateManagerSection(manager,name)`가 있다. 이 work는 그 API를 중복 추가하지 않았다. 두 patch의 Storage hunk를 통합할 때 둘 모두 유지하고 전체 파일을 덮어쓰지 않는다. Subscriber는 captured ID/물리 handle을 그대로 전달하고 create 반환 typed stream을 고정해야 한다.

## 기준·patch·정적 확인

`baseline/`은 준비 당시 제품의6개 파일을 읽어 만든 **새 기준 복사본**이다. 기존 Native baseline과 구별한다. `storage-baselines.json`, `client-wiring-baselines.json`에는 최초 제품 SHA와 최초 제안 SHA를 기록했고, 최종 제안/현재 제품 SHA는 `final-changes.json`이 기준이다. Source5 등 Root의 후속 변경이 있으면 전체 파일 복사를 하지 않고 각 hunk를 검토한다.

`data-manager-initialize-469.patch`가 신규8개 모듈파일/검사1개/기존6개파일 hunk를 합친 최종 제안이다. `existing-files.patch`와 `client-wiring.patch`는 작성 중간 분할본이며 **최종 patch 대신 적용하지 않는다.** 최종 기준 확인 결과와 SHA는 `manifest.json` 및 아래 재개 절차에서 확인한다.

준비 도구는 제품을 읽기만 하고 work 안의 새 기준/제안만 기록했다. UTF-8/Python AST 확인과 `git apply --check --whitespace=error`는 제품 적용이나 C++ 검증이 아니다. CMake 등록과 C++ warning/실행 결과는 아직 없다. 미정235BYTE는 기본 owner에서 읽어 검사하지 않으며 알려진 A5 allocator를 주입한 검사에서만 전체 범위를 비교한다.

최종 patch SHA256은 `e66295fe635cb4bcf9f16bdaea3b2c2dcd8ce35b9f4763651912cfdddbae5792`이며 현재 제품에서 `git apply --check --whitespace=error` 종료0을 확인했다. 최초 확인은 생성 작업이 아직 반환하지 않아 파일을 찾지 못했으며, 생성 완료를 기다린 뒤 위 최종 patch에 재실행하여 통과했다. 실제 적용은 하지 않았다.

## 재개 순서

1. AGENTS/LEFT_JOBS/닷넷 인수인계와 호스트 기록을 다시 확인한다. 실제 제품과 `final-changes.json`의 기준 SHA를 대조한다. manifest와 상위 Native76행 SHA가 그대로인지 확인한다.
2. Root의 기존 Source5 및 단위468 변경을 먼저 보존한다. 최종 patch를 `git apply --check --whitespace=error`로 다시 읽기 검토한다. 충돌 시 최신 제품에 hunk를 옮기며 baseline/Native 기대값을 바꾸지 않는다.
3. RawSnart/RawDataManagerInitialize/RawTemplateSectionNames를 `netstorm`, ClientTemplateSectionNames를 Client 소스에 등록한다. 검사 한 파일과 아래 두 새 fixture 경로 정의를 등록한다. 현재 동결된 상위 fixture를 BYTE 그대로 새 제품 fixture로 복사할 수 있다.

```text
NETSTORM_DATA_MANAGER_INITIALIZE_FIXTURE = .../data-manager-initialize-x86.tsv
NETSTORM_TEMPLATE_SECTION_NAMES_FIXTURE = .../template-section-names-x86.tsv
```

4. 최초 C++ 빌드로 span/생성자/API/경고를 확인한다. 집중6개와 기존 DataManagerRead/Create/TemplateLoad/Lifetime/CampaignTemplateLoadFile/SoloMapCatalog 검사를 수행한다. 이름표 수명 변경이 기존 실제 파일 읽기에 주는 회귀를 먼저 해결한다.
5. 별도 Subscriber 저장/RawNewFort 부모와 실제 작업본 장치를 통합한다. NewEditor 전체 설치 부모·외곽/tail/State 확정/AddIsland 큐는 이 patch에 없으므로 남은 범위를 명시한 채 다음 단위에서 작업한다. C++ 통과 전 사용자 GUI 성공이나 NewFort 완료로 기록하지 않는다.

사용자 요청에 따라 이 인수인계 후 추가 후속 작업은 진행하지 않는다. 인수인계 문서 반영·커밋·목표 일시중지는 Root가 수행한다.
