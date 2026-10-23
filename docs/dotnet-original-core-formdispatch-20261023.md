# 원본 월드 코어 W03 열다섯째 조각 — Form 파생 분배와 GumpForm 호스트(RawSquidFormDispatch·RawGumpFormHost)

2026-10-23, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 열다섯째 조각이다.
열네째 조각에서 신전 삭제 전처리를 옮겼고, 이번에는 [현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 4-b2" 가운데
**파생 Form 분배(`SquidFormDispatch`)·GumpForm 호스트(`GumpFormHost`)·`ContentFormHost.BindCreate`** 를 옮겼다.
선택·커서·Graph·장부·Kernel 프로세스 호스트를 묶는 실제 삭제 수명주기 조립은 아직이다.

근거는 커밋된 cpppj 소스(`SquidFormDispatch.h/.cpp`·`GumpFormHost.h/.cpp`·`ContentFormHost.h/.cpp`·`SquidForms.cpp`·`SquidFactory.cpp`)와
기존 cpppj 검사(`ContentLifetimeTests.cpp` 의 혼합 체인 시험)뿐이다. 새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다.
현재 PC는 공통 기록의 마지막 전체 정밀 디컴파일 PC(**VM-W11-Codex, 2026-10-23**)와 같으나 디컴파일 산출물을 읽지 않았으므로 재디컴파일 대상이 아니다.
마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawGumpFormHost.cs` (신규) | `GumpFormNotice`·`RawGumpFormHost`(`Create`·`Take`·`IsGumpForm`·`Owner`·`Parent`·`SetOwner`·`Attach`·`Unpop`·`Notify`·`Destroy`·`Hooks`) | `GumpFormHost.h/.cpp`, 부착 004af160, Unpop 004ae140 ↔ CD 004af270, 가상 +0x1c/+0x78 알림, 가상 +0x10 삭제, 소유 ID 004644e0 |
| `RawSquidFormDispatch.cs` (신규) | `SquidAssetDestroy`·`RawSquidFormDispatch`(`Register`·`Bind` 3종·`SetAssets`·`Destroy`·`Hooks`·`Route`·`EmitAssets`) | `SquidFormDispatch.h/.cpp` |
| `RawSquidFactory.cs` (기존 확장) | `CreateGumpForm(uint flags = 2)` — 내장 Form 생성기로 타입 3 생성 | `SquidFactory.cpp` CreateGumpForm, 004af530 계열 |
| `RawContentFormHost.cs` (기존 확장) | `BindCreate(factory, hooks)` — contained 생성의 부착 경계만 정렬 부착기로 교체 | `ContentFormHost.cpp` BindCreate |
| `RawGumpFormHostTests.cs` (신규) | GumpForm 계약 6건 | `GumpFormHost` 합성 경계 |
| `RawSquidFormDispatchTests.cs` (신규) | 혼합 체인·등록 거부·경계 교체·자산 삭제 거부 4건 | `ContentLifetime_SharedMixedFormDispatch` 와 L1 경계 |
| `RawContainedCreateTests.cs` (기존 확장) | `BindCreate` 계약 1건 | `ContentFormHost::BindCreate` |

### GumpForm 호스트 (`RawGumpFormHost`)

10.78 GumpForm(타입 3)의 생성·소유·부착·통지·삭제를 원본 순서대로 옮겼다.

1. `Create` 는 `RawSquidFactory.CreateGumpForm` 으로 타입 3 을 만든다(기본 flags 2 = 클라이언트 영역).
2. `Attach` 는 부모의 **첫 종속 항목**으로 넣고(`+4` next, `+6` head, `+14` prev), contained 비트를 세우며 void 를 끈다.
   부모·머리·목록 genus 검사를 쓰기 전에 끝내고, 10.78 처럼 **부모 0 부착은 조용히 넘긴다**.
   서버 Form 의 실제 Pop 직렬화·전송은 미복원이므로 서버 영역 Form 부착은 쓰기 전에 거부한다.
3. `Owner`/`SetOwner` 는 슬롯 `+0x12` 의 **정렬되지 않은 DWORD 전체**를 읽고 쓴다. WORD 로 절단하거나 포인터로 해석하지 않는다.
4. `Notify` 는 소유 ID 가 0 이면 아무것도 하지 않고, 아니면 **호출 인자 대신 현재 Form 부모**를 읽어 알린다.
   소유 ID·부모를 모두 읽은 뒤 알리며 알림 뒤에는 Form 을 다시 읽지 않는다(콜백이 슬롯을 바꿀 수 있다).
5. `Hooks` 는 GumpForm 만 이 몸체가 처리하고 나머지 타입은 fallback 으로 넘긴다. GumpForm 자신의 삭제에는
   소유 UI 알림이 없고, 부모 삭제의 `ReleaseDependent` 에서만 한 번 알린다.

### Form 파생 분배 (`RawSquidFormDispatch`)

원본 `SquidFormDispatch` 는 타입 번호마다 등록된 가상 삭제를 골라 같은 SID 풀·공통 삭제 깊이로 분배한다.

1. 생성 시 분배기·공통 삭제 본체가 **같은 풀**인지, 자산 공통 훅에 효과·선택 콜백이 있는지 검사한다.
2. `Bind(RawDependFormHost)`(타입 1)·`Bind(RawGumpFormHost)`(타입 3)·`Bind(RawContentLifetime)`(타입 6)는
   같은 풀·자산 타입 미만·중복 아님·효과/Unpop 경계 존재를 모두 확인한 뒤 등록한다.
3. `Destroy` 는 슬롯의 현재 타입이 자산 타입(70 이상)이고 가상 삭제 재정의가 있으면 그쪽으로 보내고,
   Form 뿌리는 공통 본체로 지운다. 재정의의 **거부 반환 값을 그대로 보존**한다.
4. `Hooks` 의 `Emit` 은 전송·선택 해제를 자산 경계로, 종속 파괴를 루트 재진입으로, 일반 Form 효과는
   현재 raw 타입의 등록 훅으로 보낸다. 콜백 뒤 다음 효과는 타입을 다시 읽는다.
5. `SetAssets` 는 현재 자산 훅·가상 삭제 경계를 통째로 교체한다. 새로 만든 훅은 교체된 경계를 읽는다.

C# 에는 가상 표가 없으므로 자산 뿌리의 가상 삭제 재정의는 `SquidAssetDestroy` 델리게이트로 분리했다.
프로세스 Form(타입 10~62) 등록은 실제 Kernel 프로세스 호스트가 아직 이식되지 않아 **비워 두고 기록**했다.

### contained 생성 연결 (`RawContentFormHost.BindCreate`)

`RawContainedCreate.ConnectFactory` 로 생성 콜백을 실제 팩토리의 contained 전용 생성자에 연결하고,
부착 콜백만 이 몸체의 정렬 부착기(`Attach`)로 바꾼다. 생성기는 슬롯 필드만 채우고 부모 알림·목록 순서는
ContentForm 규칙을 따르게 된다. 다른 풀의 팩토리는 연결 시점에 거부한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,329개 통과**(Assets 363 + Core 966; 이번 +11).

| 검사 | 내용 |
| --- | --- |
| `MixedChainDestroysEveryFormThroughDispatch` | 자산 뿌리 + ContentForm + GumpForm + 감시 쌍을 한 부모에 붙이고 한 번 삭제한다. 뿌리는 자산 재정의를 1회 거치고, 감시·Gump 통지는 각 1회, ContentForm 부모 제거 통지는 0회(부모 dead), 모든 Form 과 뿌리가 같은 풀에서 반납되며 사전/사후 깊이는 0 이다 |
| `RejectsWrongPoolDuplicateAndIncompleteRegistration` | 다른 풀의 본체·호스트, 효과/선택 콜백 누락, 같은 타입 중복 등록을 쓰기 전에 거부한다 |
| `SetAssetsReplacesCurrentBoundaryForRegisteredHooks` | 교체 뒤 Form 삭제는 새 선택 조회·새 효과 경계를 쓰고, 자산 뿌리는 새 삭제 재정의로 간다. 불완전한 교체는 거부하고 기존 경계를 유지한다 |
| `AssetDestroyRejectionIsPreserved` | 자산 재정의가 거짓을 돌려주면 분배기도 거짓을 보존하고 슬롯·효과·깊이를 바꾸지 않는다. Form 뿌리는 자산 재정의를 거치지 않는다 |
| `RawGumpFormHostTests` 6건 | 생성 가상 표·void, 부모 0 무동작, 첫 종속 부착, DWORD 소유 ID·현재 부모 통지, 통지 안 중첩 삭제(선저장 next), 소유자 알림 없는 직접 삭제, 잘못된 계약 거부 |
| `BindCreateReplacesOnlyAttachBoundary` | 실제 contained 생성 뒤 정렬 부착·부모 알림 1회·`Linked` 참, 기존 Attach 콜백 미호출, 다른 풀 팩토리 거부 |

혼합 체인 시험은 cpppj `ContentLifetime_SharedMixedFormDispatch` 와 같은 장면에서 **프로세스 Form 만 뺀** 구성이다.
프로세스 호스트가 없으므로 `Kernel.Size()`·프로세스 카운터 대신 나머지 Form·자산 반납과 깊이를 대조한다.
GumpForm·분배기·`BindCreate` 는 cpppj 전용 x86 fixture 가 없어 합성 계약으로만 검증했다(경계 기록).

**변이 확인:** 구현 8곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 자산 분기 조건 `>=` → `<` | 혼합 체인·경계 교체·자산 거부 |
| 중복 등록 검사 제거 | 등록 거부 |
| `SetAssets` 의 현재 자산 대입 제거 | 경계 교체 |
| 종속 파괴 → 종속 해제로 바꿈 | 혼합 체인 |
| GumpForm 소유 ID 0 조기 반환 제거 | GumpForm 통지 |
| GumpForm 부모 0 무동작 제거 | GumpForm 부착 |
| `BindCreate` 의 부착 경계 교체 제거 | `BindCreate` 계약 |
| `CreateGumpForm` 의 클라이언트 비트 무시 | GumpForm 생성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**동기화:** 시작 검토 HEAD 는 **`954acb92`(20261023-26 DN)**, 목록의 반영 커밋은 **`f95a8db2` 그대로**다.
`cpp_sync.py` 종료 0, 연결 fixture **58개**와 생성 자료 3개가 모두 일치했다. 종료 시점 cpppj HEAD 는
**`9eb899e4`(20261023-27 CP)** 이며, cpppj의 `*-x86.tsv` **508개 중 연결된 것은 55개**다.
반영 이후 cpppj 관련 커밋은 **91개**, 새 fixture **51개**(수정 0·삭제 0)다. 이 가운데 이번에 옮긴
`GumpFormHost`·`SquidFormDispatch`·`ContentFormHost`·`SquidForms`·`SquidFactory`·`RawSquidDestroy`·
`DependFormHost`·`ContentFormLifetime`·`RawContainedCreate` 를 바꾼 커밋은 없었다.
미이식 기능이 남아 있으므로 규칙대로 반영 커밋을 HEAD 로 올리지 않았다
([계획 7절 7번](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)).
작업 트리의 미커밋 cpppj 변경(`SoloMapCatalog`·`FortSelection`, 추적되지 않은 `fort-snapshot-basics-x86.tsv` 등)은 쓰지 않았다.

## 3. 이번에 하지 않은 것 (경계)

- **실제 삭제 수명주기 조립**(선택·커서·Graph·장부·Kernel 프로세스 호스트)은 아직이다.
  이번 모듈은 파생 분배와 GumpForm 수명만 소유하며, 실제 월드 분배기에 꽂지 않았다.
- **프로세스 Form(타입 10~62) 등록**은 Kernel 프로세스 호스트가 없어 비워 두었다. 원본 분배기의 등록 표를
  흉내 내어 빈 훅을 넣지 않았다.
- **자산 뿌리의 가상 삭제**는 C# 에 가상 표가 없어 `SquidAssetDestroy` 델리게이트로 위임했다.
  실제 자산 타입별 삭제 재정의(파생 클래스)는 별도 이식 대상이다.
- GumpForm 의 **서버 Pop 직렬화·전송**은 미복원이므로 서버 영역 Form 부착을 거부한다.
- CD 10.72·10.37 몫은 옮기지 않았다(10.78 기준).
- 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

삭제 수명주기 조립(선택·커서·Graph·장부·Kernel 프로세스 호스트·실제 세션 연결) → L2 시작 상태.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
