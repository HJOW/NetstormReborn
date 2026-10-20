# 원본 월드 코어 W02 넷째 조각: 공통 후처리(PostPop)와 다리 접두

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 넷째 조각이다. [셋째 조각](dotnet-original-core-pop-20261020.md)(공간 등록 Pop)에 이어 `dotnetpj/src/Netstorm.Core/Original/` 에 공통 후처리 PostPop 과 다리 접두를 옮겼다. 근거는 커밋된 cpppj 소스·보고서([cpp-postpop-reconstruction.md](exe/cpp-postpop-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawPostPop.cs` | 공통 후처리(`004b0d30`)의 10.78 경로. `RawPostPopList`(원본 확보 배열+개수, 가득 차면 생략)·`RawPostPopState`(비용·알림·깊이·세 통계표·공급원/작업장/소유자별 목록·억제/Graph/보류 배치·로컬 소유자·AI 부착)·`IRawPostPopGraph`(표면 Graph 연결 지점, 미연결이면 영역·표면 등록 거부)·`RawPostPop`(Validate→ValidateBase→PostPopBase→Activate, 패치판 비용 편향 `타입×23` 누적, 다리 접두 `BridgePrefix`). 파생 몸체가 없으므로 `HandlesDerived` 는 항상 거짓이다 | `004b0d30`, `SquidPostPop.cpp` |
| `OriginalTypeRecord.cs` | `Cost`(float, 원본 타입 +0xc4) 추가. 공통 후처리의 전역 비용 집계가 읽는다 | `RiftTypeRecord::cost` |
| `X86RawPostPopTests.cs` | `postpop-x86.tsv` 10.78 몫(직접 PostPop 384·Pop 72·Unpop 36)과 `bridgepostpop-x86.tsv` 10.78 몫(80행) 재생. 표시 대역은 고정 SHP 모양 2프레임·640×480 viewport 의 일반 경로를 이미 검증된 `DisplayBounds` 로 재현하고, 변경 표 수집은 원본 `DirtyRegions::Add` 의 병합 규칙(합집합 면적 1/8 허용치)으로 합친다 | `PostPopTests.cpp` |
| `RawPostPopTests.cs` | 보호 경로·연결 지점 계약 8개(손상 거부·AI/보류 거부·비용 범위·억제·최초 등록 중복 금지·Graph 요구·Graph 위임·다리 접두) | `PostPopTests.cpp` 의 보호 검사 |

순서는 원본 그대로 "상태 검사 → 파생 지원 검사 → Graph 검사 → 비용·목록·통계 검사(Validate) → Graph 생성 → 비용 누적 → 공급원·작업장 목록 → 소유자 조건 통계 → 깊이 감소(PostPopBase)"이며, 억제(`suppressed`)면 검사 뒤 깊이만 줄인다. 직접 `PostPop` 은 깊이를 1 줄이고, `Activate` 는 1 늘렸다가 공통 몸체에서 1 줄여 net 0 이다(Pop 경로의 깊이 보존). 원본은 손상된 목록 개수·미복원 효과·범위 밖 소유자·AI 부착·보류 배치·비유한 비용을 **풀·상태를 바꾸기 전에** 예외로 거부한다.

작업 중 원본 테스트(`PostPopTests.cpp`)의 `Fill` 분기가 슬롯 재작성 때 해시를 리셋(`hash->Reset()`, spot 은 유지)함을 확인해 C# 재생에도 그대로 넣었다. 이를 빠뜨리면 연속 State 블록의 두 번째 Pop 이 "해시 머리 오류"로 깨진다.

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 공통 후처리·Pop·Unpop 재생 | `postpop-x86.tsv` 10.78 몫 2개 Begin·**492회**(PostPop 384·Pop 72·Unpop 36) | 비용·알림·깊이·**세 통계표 256×3 Adler-32**·목록 개수 11개·**목록 전체 바이트(stale 포함) Adler-32**·대상 슬롯 50바이트·표시 변경 표(사각형·플래그·순서) |
| 다리 접두 | `bridgepostpop-x86.tsv` 10.78 **80**행 | 추가 비트 결과·연결 효과 호출(추가 비트 쓰기 뒤 호출) |

[`RawPostPopTests`](../dotnetpj/tests/Netstorm.Core.Tests/RawPostPopTests.cs) 8개가 보호 경로·연결 지점 순서·경계를 확인한다: 손상 상태·미지원 가상 표·범위 밖 소유자·손상 목록은 상태·풀을 바꾸기 전에 거부하고, AI 부착·보류 배치는 최초 등록만 거부하며(flags 0 호출은 깊이만 줄인다), 억제 경로는 AI 부착 검사도 건너뛰고, 최초 등록의 재등록은 비용을 중복 집계하지 않으며(직접 호출은 다시 집계), Graph 파괴 비트는 연결돼 있어도 assert 로 멈추고, 다리 접두는 flag 1 의 추가 비트 쓰기→연결 호출 순서를 지킨다.

**변이 확인:** 구현 20곳(void 조기 반환·지원 표·억제 조기 반환·파괴 assert·Graph 요구·최초 등록 조기 반환·소유자 범위·AI 부착·보류 배치·NaN 검사·비용 편향·공급원 중복 검사·작업장 검사·작업장 덧붙임·로컬 통계·억제 깊이·다리 연결 효과 누락·다리 추가 비트·다리 연결 조건·NoGraph 무효 번호)을 하나씩 망가뜨려 19곳이 실패로 잡혔다. 소스는 모두 원복했다. 남은 1곳(void 번호 `<5` 검사)은 도달 가능한 입력이 없으므로 잡히지 않는다 — 0~4번 슬롯은 빈 비트·타입 바이트 검사를 절대 통과하지 못해 뒤의 검사가 항상 먼저 거부한다(방어용 중복 검사로 유지). 변이 실행 중에는 스크립트 대신 강제 전체 빌드(`--no-incremental`) 뒤 건별 대조로 진행했다. 스크립트 일괄 실행은 빌드 캐시·파일 잠금 탓에 거짓 BUILD-FAIL 과 오래된 DLL 대조를 내어 신뢰할 수 없었다(해당 시행의 결과는 파기하고 건별로 다시 확인했다).

## 3. 이번에 하지 않은 것 (경계)

- **파생 후처리 몸체 전부**(사제·보호막·운반체·자산·공장·룬·다이스·Dude·Player·매립·비행체·벌룬·다리 연결·섬 받침 접두) — `HandlesDerived` 는 항상 거짓이며 해당 가상 표의 등록은 거부된다. 자산 후처리(`assetpostpop-x86.tsv`, 1,636행)는 운반체(Damageable) 몸체가 필요하므로 다음 조각에서 한다.
- 소유자(`owner`, `owner-x86.tsv`)·프레임(`setframe`, `setframe-x86.tsv`), 건물 공간(`buildingspace`)·이웃·탐색, Graph 계열(`IRawPostPopGraph` 구현체 — `SurfaceGraph` 어댑터는 Graph 조각에서), CD 10.72(소유자·추가 비트 위치와 비용 누적 방식이 다름).
- `ValidateFrameBinding`·`ValidateGraph` 같은 상위 묶음 검사와 소용돌이 신전 inset·마나 WORD0 표시 특수 경로는 옮기지 않았다(표시 대역은 기본 가상 표의 일반 경로만 재현하며, 이 fixture 의 가상 표는 항상 base 다).
- `cpp_sync.py` 의 반영 커밋을 `1530e0af`→**`65674f52`** 로 올렸다(사이 cpppj 커밋 2건은 건설 setOwner 배선·사전 점검 문서로 연결 fixture·이식 모듈에 영향 없음). 별도 도구 `gen_*` 추가는 없다(새 표 없음).

## 4. 다음 조각

`assetpostpop`(자산 전용 후처리, 운반체 Damageable 몸체와 함께)·소유자(`owner`)·프레임(`setframe`) → 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`·`IRawPopPostPop` 을 구현)·이웃·탐색 → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
