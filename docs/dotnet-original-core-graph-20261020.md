# 원본 월드 코어 W02 열셋째 조각: 그래프 표 저장소와 초기화

2026-10-20, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열셋째 조각이다. [열두째 조각](dotnet-original-core-metadisplay-20261020.md)이 다음 시작점으로 남긴 `RawGraph` 가운데, 뒤따르는 모든 그래프 연산의 바탕이 되는 **표 저장소·초기화·반납·공간/타입 일치 검사**를 먼저 옮겼다. 근거는 커밋된 cpppj 소스와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawGraph.cs` | 그래프 표 저장소(255 레코드 × 6바이트 = 1,530바이트, 4,096개 flood 스택 DWORD)와 `ResetTable`·`Free`/`ValidateFree`·`Records`/`FloodStack`·`ValidateSpace`·`ValidateTypes`·`Pool`. 레코드·스택은 호출자 입력을 복사해 소유하고, 레코드를 주지 않으면 254 sentinel 만 0x7dfd 로 채운다 | `RawGraph.cpp` 의 `ResetTable`·`Free`·`ValidateFree`·생성자, `Graph.h` 의 `kCount`/`kTableSize`/`kFloodSize`/`kInvalid` |
| `X86RawGraphTests.cs` | `graph-reset-x86.tsv` 17행(헤더 1 + 재생 16) — 입력 1,530바이트(byte×37+seed)로 표를 채우고 `ResetTable` 뒤 표 전체 바이트·풀 슬롯 5·스택 4,096개 불변을 대조 | `GraphResetTests.cpp` |
| `RawGraphTests.cs` | 경계 4개(저장소 크기 거부 / 초기화가 251~253·예약 WORD·스택·풀을 보존 / Free 번호 범위와 sentinel 보존 / 공간·타입 일치) | `RawGraph.cpp` 의 크기·범위 검사 |

초기화 순서는 원본 그대로 "앞 251 레코드의 표면 수·사용 여부만 0 → 0번 사용=1, 예약 WORD 하위 BYTE=1(윗 BYTE 보존) → 254 sentinel 의 표면 수·사용 여부=0x7dfd"다. 251~253 은 원본 표의 틈이라 건드리지 않고, 모든 레코드의 예약 WORD 나머지 비트·스택·풀도 보존한다. `Free` 는 0~250 과 254 만 받고 254 는 sentinel 을 그대로 두며, 유효 번호는 표면 수·사용 여부만 지운다.

## 2. 검증 결과 (원본 fixture)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 표 초기화 재생 | `graph-reset-x86.tsv` **16행** | 초기화 뒤 표 전체 **1,530바이트**(0번 사용·예약 WORD 하위 BYTE, 251~253 틈, 254 sentinel 0x7dfd)가 전부 일치, 풀 슬롯 5 와 스택 4,096개 불변 |
| 저장소 크기 | 합성 | 타입 187개·점유 지도 100바이트·레코드 254개·스택 100개를 만들기 전에 거부 |
| 초기화 보존 범위 | 합성 | 251~253·예약 WORD 윗 BYTE·스택 흔적·풀 카운터 보존, 254 sentinel 값 |
| Free·일치 검사 | 합성 | 251~253·255 거부, 254 무변경, 같은 공간/같은 내용 타입만 허용 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,204개 통과**(Assets 363 + Core 841; 이번 +5).

**변이 확인:** 구현 13곳(초기화 지우기 범위 `Count`→`TableSize`, 0번 사용 비트 생략, 예약 WORD 하위 BYTE 1 생략, 예약 WORD 윗 BYTE 덮어쓰기, sentinel 사용 여부 생략, sentinel 값 0x7DFE, Free 가 예약 WORD 지움, Free 범위 `Count`→`TableSize`, 레코드 복사 누락, 스택 마지막 칸 복사 누락, 공간 일치를 크기 비교로, 타입 비교에서 플래그2 생략, 생성자 sentinel 누락)을 하나씩 넣어 **13곳 모두 실패로 잡혔다.** 소스는 모두 원복했고 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `RawGraph` 의 **영역 무효화(`Region`/`InvalidateRegion`)·추가/flood(`Add`/`Flood`)·`PostPop`·삭제 준비(`Detach`)·전체 재구성(`Rebuild`)·`Allocate`·`PostDestroy`·`Frame`** 은 옮기지 않았다. 이들은 표 위의 계획(`Calculate`)과 `SurfaceFinder`/`SurfaceGraph`, 타입별 `FrameCode`, 그리고 `rawgraph`/`regiongraph`/`graphremove`/`graphrebuild`/`graphrecovery`/`graphlookup` fixture 의 장면(공통 postPop·표시 효과·Totalmade 상태)을 함께 요구한다. 다음 조각에서 그 순서대로 얹는다.
- `graph-reset` 이외의 그래프 fixture 는 아직 연결하지 않았다(장면 의존). CD 10.72·10.37, 실제 세션 연결, 파생 분배 허브도 그대로 미연결이다.

## 4. 다음 조각

`RawGraph` 의 `Region`(원본 `00462d40`)·`Add`/`Flood`(원본 `004633c0`/`00462f10`)와 그 계획 반영(`Calculate`/`Commit`) → `PostPop` → `Detach`/`Rebuild`/`Allocate` → `PostDestroy`. `rawgraph-x86.tsv`(1,536 호출)를 먼저 재생하려면 장면 준비(공통 postPop·표시)를 함께 옮겨야 한다. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
