# 원본 월드 코어 W03 자산 삭제 분배 조각 — 다리 가상 삭제 재정의(RawSquidDestroyDispatch)

2026-10-23, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의
열여섯째 조각이다. 열다섯째 조각에서 Form 파생 분배·GumpForm 호스트를 옮겼고, 이번에는
[현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 4-b2" 가운데
**자산 타입별 가상 삭제 재정의(`RawSquidDestroyDispatch`)** 의 첫 자산 종류인 다리를 옮겼다.
선택·커서·Graph·장부·Kernel 프로세스 호스트를 묶는 실제 삭제 수명주기 조립은 아직이다.

근거는 커밋된 cpppj 소스(`RawSquidDestroyDispatch.h/.cpp`·`RawSquidDestroy.cpp`·`SquidUnpop.cpp`·`RawSquidNeighbors.h`·`RawGraph.cpp`·`Bridge.h`)와
cpppj 검사(`DestroyDispatchTests.cpp`)뿐이다. 새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다.
현재 PC는 공통 기록의 마지막 전체 정밀 디컴파일 PC(**VM-W11-Codex, 2026-10-23**)와 같으나 디컴파일 산출물을 읽지 않았으므로 재디컴파일 대상이 아니다.
마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawSquidDestroyDispatch.cs` (신규) | `RawSquidDestroyDispatch`(`Destroy`·`Pool`·`Graph`, 고정 상태/상태 공급자 생성자) | `RawSquidDestroyDispatch.h/.cpp`, 원본 004220f0 |
| `RawSquidDestroy.cs` (기존 확장) | `ValidateGraph` 가 같은 공간 해시·같은 점유 지도까지 확인 | `SquidUnpop::ValidateGraph` → `RawGraph::ValidateSpace` |
| `RawUnpop.cs` (기존 확장) | `Hash`·`Spots` 읽기 전용 접근자 | `SquidUnpop` 의 공간 대조 |
| `RawNeighbor.cs` (기존 확장) | `Types` 를 내부 전용에서 공개로 넓힘 | `RawSquidNeighbors::Types()` |
| `RawSquidDestroyDispatchTests.cs` (신규) | cpppj 계약 5건 + Form 분배 합성 1건 | `DestroyDispatchTests.cpp` |

### 다리 가상 삭제 재정의 (`RawSquidDestroyDispatch`)

원본 자산 루트의 삭제는 가상 표 `+0x10` 의 재정의를 먼저 거친다. 이 몸체는 10.78 의 다리 재정의 하나를 옮겼다.

1. 슬롯 앞 4바이트가 **10.78 다리 가상 표**(`0x005034C8`)일 때만 재정의를 검사한다. 다른 자산·Form 은 그대로 공통 본체로 간다.
2. 편집기이거나 권한이 없거나 추가 비트에 **abstract(1)·매립(8)** 이 켜져 있으면,
   무효 Graph 번호·범위 밖 프레임을 읽기 **전에** 단락한다. 원본의 단락 평가 순서를 그대로 지킨다.
3. Graph 를 연결했으면 슬롯 `+30` 의 Graph 번호를 읽는다. **254(무효)** 는 원본이 NULL 을 역참조하는 지점이므로
   읽기 전에 예외로 표시하고, 표 범위(255) 밖도 쓰기 전에 거부한다. Graph 를 연결하지 않았으면 표면 수를 0 으로 본다.
4. 표면 수가 **5 미만**이면 현재 프레임을 읽지 않고 공통 본체로 넘긴다.
5. 표면 수가 5 이상이면 슬롯 `+36` 의 4바이트 프레임을 **signed int32** 로 읽고,
   타입의 SHP 프레임 목록 범위를 확인한 뒤 `BridgeDecayRules.DestroyProceeds` 로 판정한다.
   단단한(hard) 프레임이거나 디버그 유지(`DebugKeep`)이면 **거짓을 돌려주고 아무것도 바꾸지 않는다**.
6. 그 밖에는 공통 삭제 본체를 호출하고 참을 돌려준다. 거부 반환 값을 그대로 보존하므로
   `RawSquidFormDispatch` 의 `SquidAssetDestroy` 경계에 그대로 꽂을 수 있다.

실행 상태(`BridgeDecayMode`)는 C# 레코드의 init 속성이라 도중에 바꿀 수 없으므로,
상태 공급자(`Func<BridgeDecayMode>`)를 받는 생성자를 두어 **호출마다 현재 상태를 다시 읽는다**.
고정 상태만 쓰는 호출자는 기존 형태의 생성자를 쓸 수 있다.

### 공간 대조 (`RawSquidDestroy.ValidateGraph`)

원본 `RawSquidDestroy::ValidateGraph` 는 `SquidUnpop::ValidateGraph` 에 위임하고, 그것이 `RawGraph::ValidateSpace` 로
**같은 공간 해시·같은 점유 지도 배열**인지 확인한다. 기존 C# 판은 풀 동일성만 봤으므로,
`RawUnpop.Hash`·`RawUnpop.Spots` 접근자를 더하고 `RawGraph.ValidateSpace` 까지 부르게 했다.
그래서 다른 공간의 Graph 를 가진 분배기는 첫 삭제 전에 거부된다(원본 `destroy_dispatch_rejects_mismatched_pool_and_graph_space`).

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,335개 통과**(Assets 363 + Core 972; 이번 +6).

| 검사 | 내용 |
| --- | --- |
| `BridgeThresholdHardDebugAndDisabledGraph` | 표면 수 4(통과)·5(거부)·-1(통과, signed), 보통 프레임(통과), `DebugKeep`(거부), Graph 비활성(통과) |
| `ShortCircuitsGraphAndFrameReads` | editor·권한 없음·abstract·매립 각각이 무효 Graph 번호와 프레임 99 를 읽지 않고 통과한다. 표면 4 도 프레임을 읽지 않는다 |
| `ReadsLiveVtableFrameAndKeepsDirectBaseCall` | 같은 슬롯의 프레임을 1→0 으로 바꾸면 거부→통과로 바뀐다. 일반 가상 표는 재정의를 건너뛴다. 공통 본체 직접 호출은 분배를 거치지 않는다 |
| `InvalidGraphAndFramePreserveRawState` | 무효 Graph(254)·범위 밖 Graph(255)·범위 밖 프레임(99)은 슬롯 바이트·효과 횟수·깊이를 바꾸지 않고 예외로 남는다 |
| `RejectsMismatchedPoolAndGraphSpace` | 다른 풀의 이웃/Graph, 같은 풀·같은 타입 표라도 다른 공간 해시의 Graph 를 생성 시 거부한다 |
| `ComposesWithFormDispatchAsAssetOverride` | `RawSquidFormDispatch` 의 자산 재정의로 그대로 꽂힌다. 거부는 거짓으로 보존되고 효과가 0회, 통과하면 pre/post 두 번·실제 반납·깊이 0 |

각 장면은 cpppj `DispatchScene` 과 같이 공통 pre/post 깊이만 완료하고, 거부 시 **슬롯 전체 바이트·빈 번호 수·Graph 표·flood 스택·콜백 횟수**가
그대로인지 대조한다. 이 몸체는 cpppj 전용 x86 fixture 가 없어 **합성 계약**으로만 검증했다(경계 기록).

**변이 확인:** 구현 5곳을 하나씩 망가뜨려 4곳이 실패로 잡혔다.

| 변이 | 결과 |
| --- | --- |
| 표면 수 경계 `>=` → `>` | 경계 검사가 실패로 잡힘 |
| abstract·매립 단락 조건 제거 | 단락 검사가 실패로 잡힘 |
| 거부 반환 `false` → `true` | 경계·단락·합성 검사 3건이 실패로 잡힘 |
| `ValidateGraph` 의 공간 대조 제거 | 다른 공간 Graph 거부가 실패로 잡힘 |
| 프레임 상한 검사 제거 | **동등 변이** — 배열 인터페이스 인덱서가 같은 `ArgumentOutOfRangeException` 을 내어 계약(쓰기 전 예외)이 같다. 구현은 원본 규칙(명시적 범위 거부)을 유지 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**동기화:** 시작 검토 HEAD 는 **`c7459e61`(20261023-31 CP)**, 목록의 반영 커밋은 **`f95a8db2` 그대로**다.
`cpp_sync.py` 종료 0, 연결 fixture **58개**와 생성 자료 3개가 모두 일치했다. 종료 시점 cpppj HEAD 는
**`77156a6c`(목표 프롬프트 내용 추가)** 이며, 이번 작업 중 들어온 커밋은 `TODO.md` 한 줄뿐이다.
반영 이후 cpppj 관련 커밋은 **77개**(전체 118개), 새 fixture **56개**(추가 0→56, 수정·삭제 0)다.
cpppj의 `*-x86.tsv` **514개 중 연결된 것은 55개**다.
이 가운데 `RawSquidDestroy*`·`SquidUnpop`·`RawSquidNeighbors`·`SquidFormDispatch` 를 바꾼 커밋은 없고,
`RawGraph.cpp` 를 바꾼 `7738c65a`(비표면 예측 그림의 소수 영역 연결)는 이 분배기의 표면 수·프레임 판정에 영향이 없다.
미이식 기능이 남아 있으므로 규칙대로 반영 커밋을 HEAD 로 올리지 않았다
([계획 7절 7번](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)).
작업 트리의 미커밋 cpppj 변경(`SoloMapCatalog` 등)과 추적되지 않은 `fort-chunk-save-x86.tsv`·`fort-object-save-x86.tsv` 는 쓰지 않았다.

## 3. 이번에 하지 않은 것 (경계)

- **삭제 수명주기 조립**(선택·커서·Graph·장부·Kernel 프로세스 호스트)은 아직이다. 이 몸체는 자산 하나의 재정의만 소유한다.
- **다리 외 자산 타입의 가상 삭제 재정의**는 아직 옮기지 않았다(사제·워크샾·공격체·신전·얼음 타워 등).
- **프로세스 Form(타입 10~62) 등록**은 Kernel 프로세스 호스트가 없어 `RawSquidFormDispatch` 와 같이 비워 두었다.
- `RawGraph.cpp` 의 비표면 소수 영역 연결(`7738c65a`)은 이 조각에서 이식하지 않았다. 기존 `RawGraph` 포팅 범위 그대로다.
- CD 10.72·10.37 몫은 옮기지 않았다(10.78 기준).
- 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

삭제 수명주기 조립(선택·커서·Graph·장부·Kernel 프로세스 호스트·실제 세션 연결) → L2 시작 상태.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
