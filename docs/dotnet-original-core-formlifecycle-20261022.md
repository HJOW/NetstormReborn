# 원본 월드 코어 W03 열한째 조각 — Form 수명(SquidForms·ContentFormHost·DependFormHost)

2026-10-22, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 열한째 조각이다.
열째 조각까지는 종속 조회·삭제(`RawContainedLookup`)까지 이식했다. 이번 조각은 [현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의
"바로 다음 작업 2번 — W03 Form 의존성" 가운데 **공통 Form 생성·지정 Take·부착·공통 Pop/Unpop**, **생산 목록 ContentForm**,
**감시 쌍(DependForm)** 세 몸체를 옮긴 것이다. `RawFormPacket`(form-packet)과 `ContentFormLifetime`(contentlifetime)은 다음 조각으로 남긴다.

근거는 커밋된 cpppj 소스(`SquidForms.cpp`·`ContentFormHost.cpp`·`DependFormHost.cpp`)와 원본 fixture
(`formtake-x86.tsv`·`formlifecycle-x86.tsv`)뿐이다. 새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다.
현재 PC는 공통 기록의 마지막 전체 정밀 디컴파일 PC(**VM-W11-Codex, 2026-10-22**)와 다르지만, 디컴파일 소스를 분석하지 않고
커밋된 cpppj 소스와 기대값만 대조했으므로 재디컴파일 대상이 아니다. 마지막 디컴파일 PC 기록은 그대로다.
`cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawSquidForms.cs` (신규) | `Create`·`Take`·`Attach`·`ValidatePop`·`Pop`·`Unpop`·`Parent`·`Previous`, 10.78 생성자 가상 표 4개 | `SquidForms.cpp` 의 004af530(Create)·004af610(Take)·004ac560(Pop)·004ae140(Unpop) |
| `RawContentFormHost.cs` (신규) | `ContentFormState`·`ContentFormNotice`·`ContentPriestNotice`, `Attach`·`Unpop`·`Robust`·`Linked`·`Damage` | `ContentFormHost.cpp` 의 004aec10(정렬 부착)·004ae080(연결 검사)·+0x5c(재부착) |
| `RawDependFormHost.cs` (신규) | `DependFormPair`·`DependFormNotice`, `Create`·`Has`·`Remove`·`Watcher`·`Notify`·`Destroy`·`Hooks` | `DependFormHost.cpp` 의 004b1510(쌍 생성)·004ae900(조회)·004ae780(제거)·+0x1c(통지) |
| `X86RawSquidFormsTests.cs` (신규) | `formtake` 10.78 몫 300행 재생 1건 + 합성 5건 | `FormTakeTests.cpp` 의 `FormTake_IndependentPeFixture` 및 계약 검사 |
| `X86RawFormLifecycleTests.cs` (신규) | `formlifecycle` 10.78 몫 456행 재생 1건 + 합성 4건 | `FormLifecycleTests.cpp` 의 `FormLifecycle_IndependentPeFixture` 및 계약 검사 |
| `Netstorm.Core.Tests.csproj` | fixture 링크 2건, `cpp-fixture-manifest.tsv` 갱신(51→**53**개) | `cpp_sync.py --update --add formtake:Core --add formlifecycle:Core` |

### 공통 Form (`RawSquidForms`)

원본은 Form 슬롯의 +0 가상 표, +10 타입, +11 상태(free·dead·void·contained), +4 다음 종속, +6 부모 종속 머리,
+14 이전 종속, +16 부모를 쓴다. 이식한 순서와 규칙은 다음과 같다.

- `Create(type, flags)`: 지원 타입(1·2·3·6)의 가상 표 값을 먼저 확인하고, `Allocate` 뒤 가상 표(DWORD)와 타입(BYTE)만 쓴다.
  생성 뒤 postCreate 는 비어 있다.
- `Take(type, sid)`: 서버 영역 번호만 받는다. 살아 있는 슬롯은 타입이 같을 때만 공통 Unpop 을 먼저 수행하고,
  그 알림이 바꾼 현재 슬롯을 다시 읽어 free/dead 만 끄고 void 를 켠다. 패치 판본은 타입→void 순서다.
- `ValidatePop`: void 가 아니거나 dead·0x10 상태면 슬롯을 바꾸기 전에 거부한다. 서버 풀에서 client Form 은 0x10,
  서버 Form 은 client 비트(8)를 보완하며, 전송 묶음(0x50)이 없으면 전송 경계를 요구한다.
- `Pop`: void 를 먼저 끄고, 서버 전송이 필요하면 임시 0x10 을 세워 전송한 뒤 되돌린다.
- `Attach`: 부모 0 은 10.78 처럼 무동작, ContentForm 은 정렬 부착기를 요구한다. 부모·종속 머리·자기 참조를 검사한 뒤
  부모 머리 앞에 넣고 contained 를 켜며 `Pop(flags|0x100)` 로 전송을 건너뛴다.
- `Unpop`: void 를 켠 뒤 부모 free 면 링크를 보존한 채 돌아가고, dead 면 체인을 건드리지 않고 끝낸다.
  살아 있는 부모에서 빠진 ContentForm 만 부모 통지를 보낸 뒤 이전/다음 링크와 머리를 고친다.
- 프로세스 타입(10~62)은 생성자 가상 표(ProcessForm)로 확인한다.

### ContentForm (`RawContentFormHost`)

- 같은 목록 그룹(플래그 `0xfcff`) 안에서 순서 번호(+23)가 처음으로 커지는 자리 앞에 삽입한다.
  끝까지 가면 꼬리에 붙인다. 삽입 위치의 이전 항목 갱신과 부모 머리 갱신을 원본 순서대로 한다.
- 부착 뒤 `0x100` 전송을 건너뛴 공통 Pop 을 호출하고 **부모 통지(1 = 추가)** 를 보낸다.
- 전투 상태·검사 상태가 연결되어 있으면 부모 통지/전송 **뒤에** 현재 내용물 타입·부모 genus 를 다시 읽어
  사제 포획 알림을 요구한다(연결 검사 중이면 실제 체인 연결을 다시 확인).
- `Robust` 는 WORLD(0x80) 목록을 원본 assert 경로로 거부하고 0x34 비트 유무를 돌려준다.
- `Linked` 는 상태·타입 필터 없이 부모 체인의 next 만 따라가며, 범위 밖 next 를 만나면 false 로 끝난다.
- `Damage` 는 0·음수면 값을 그대로 돌려주고, 양수면 Unpop → 순서 변경 표시(0x100) 해제 → 0x10 로컬 재부착 뒤 0 을 돌려준다.

### 감시 쌍 (`RawDependFormHost`)

- `Create(watched, watcher)`: 어느 한 부모라도 client 번호면 두 Form 을 client 영역에 만든다. watcher 쪽 Form 을 먼저
  watcher 에 붙이고, watched 쪽 Form 을 watched 에 붙인다. 반환은 `(watchedForm, watcherForm)` 이다.
- `Has`: watched 체인의 타입 1 Form 중 `Watcher(form)` 이 watcher 와 같은 첫 항목을 찾는다. 후보를 반환하기 전에 next 를 저장한다.
- `Remove`: watched 가 이미 dead 면 부모 검사보다 먼저 돌아간다. 부모 타입 계약은 watcher 가 DependForm 이 아니고
  watched 가 자산이어야 한다.
- `Notify`: +18 watcherForm 이 0 이면 무동작, 0 이 아니면 그 부모를 읽어 client 쌍·서버 권한일 때만 통지하고
  통지 뒤 +20 watchedFid 가 0 인지 다시 확인한다.
- `Hooks`: 타입 1 만 처리하고 나머지는 fallback 으로 넘긴다. 사전 삭제는 **watched → watcher** 순서이며,
  두 Form 이 서로를 재삭제해도 dead 에서 즉시 돌아온다. Pre/Post 완료 호출로 공통 깊이를 반납한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,288개 통과**(Assets 363 + Core 925; 이번 +11).

| 검사 | 내용 |
| --- | --- |
| `formtake` 10.78 몫 300행 재생 | 매 행마다 반환 번호·부모 제거 통지 문자열·지정 슬롯 전체 바이트·풀 크기/빈 수/예측 커서/머리/꼬리/Adler-32 를 대조한다 |
| `formlifecycle` 10.78 몫 456행 재생 | 매 행마다 반환 값·외부 통지 순서·지정 슬롯 전체 바이트·풀 8개 값·공통 삭제 Pre/Post 깊이를 대조한다 |
| 합성 9건 | Take 타입 불일치·서버 영역 경계·부모 dead Unpop·Pop 플래그 보완/전송 경계·Attach 거부·ContentForm 통지 시점·감시 방향/재진입·감시 생성 권한/영역·사전 삭제 순서·ContentForm 순서/피해 재부착 |

- `formtake-x86.tsv` 는 총 600행(2 판본 × 300행)이며 10.78 몫 300행만 재생했다. 실행 연산은 Create 28·Take 168·Unpop 96·Reset 8 이고,
  부모 제거 통지는 16행에서 잡힌다. 서버 풀 152행·client 풀 148행이 모두 쓰인다.
- `formlifecycle-x86.tsv` 는 총 912행(2 판본 × 456행)이며 10.78 몫 456행만 재생했다. 실행 연산은 Robust 256·Attach 136·Has 16·Destroy 12·
  Pair 11·Remove 9·Notify 8·Damage 8 이고, ContentForm 통지 152건·감시 통지 6건·서버 전송 2건이 실제로 잡힌다.
  순서 번호를 덮어쓰는 변형 4행도 포함된다.
- `originalCD` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

**변이 확인:** 구현 13곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| Take 의 살아 있는 슬롯 타입 불일치 검사 제거 | 합성(Take 타입 불일치) |
| ContentForm 부모 제거 통지 제거 | 원본 재생 |
| Unpop 이웃 이전 링크 갱신 제거 | 원본 재생 |
| Unpop 부모 free 조기 반환 제거 | 원본 재생 |
| 서버 Pop 전송 제거 | 원본 재생 |
| ContentForm 순서 번호 위치 23→22 | 원본 재생 |
| ContentForm 삽입 비교 `<`→`<=` | 원본 재생 |
| 목록 그룹 마스크 0xfcff→0xffff | 원본 재생 |
| 재부착 순서 변경 표시 해제 제거 | 원본 재생 |
| 감시 쌍 할당 영역 서버 고정 | 원본 재생 |
| 감시 사전 삭제 순서 반전 | 합성(사전 삭제 순서) |
| Remove 의 watched dead 조기 반환 제거 | 원본 재생 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **Form 패킷 직렬화(`RawFormPacket`, `form-packet`)는 아직 옮기지 않았다.** 가상 +0x54/+0x58 writer 와 헤더 상태를
  같은 조각에서 연결해야 하므로 다음 조각으로 미뤘다.
- **내용물 수명(`ContentFormLifetime`, `contentlifetime`)과 contained 생성기 연결(`BindCreate`)은 옮기지 않았다.**
  `RawContainedCreate`·`SquidFactory`·표면 별도 알림을 함께 요구한다.
- `ContentFormState` 의 전투 사제 포획 경로는 구현했지만 기대값 fixture 가 없어 이번 행으로 검증하지 않았다(합성 검사도 넣지 않았다).
- `SquidFormDispatch`·`GumpFormHost`·Form 공통 Pop 전송 헤더·파생 Form 분배 허브는 옮기지 않았다.
- 신전 삭제, 삭제 보상(SP·지갑·샘 풀) 계산, CD 10.72·10.37 몫, 실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.
- 동기화 목록의 반영 커밋은 **`f95a8db2` 그대로** 두었다. HEAD(`eb739b80`) 사이 cpppj 커밋 65개 가운데 이번에 이식한 Form 모듈을
  바꾼 커밋은 없지만, 새 fixture 27개(AI·워커·클라우드 플로터·wind aviary 등)가 아직 이식되지 않은 기능을 담고 있어
  ["미반영 동작이 남으면 반영 커밋을 무조건 HEAD 로 올리지 않는다"](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)는 규칙을 지켰다.
  작업 트리의 미커밋 cpppj 변경(`dissolve-frame` 등, 다른 세션 작업물)은 쓰지 않았다.

## 4. 다음 조각

Form 패킷(`form-packet`·`RawFormPacket`) → 내용물 수명(`contentlifetime`·`RawContainedCreate`) →
신전 삭제 → 삭제 보상(SP·지갑·샘 풀) 계산 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
