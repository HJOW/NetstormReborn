# 원본 월드 코어 W02 열한째 조각: 다리 사건과 칸 변환

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열한째 조각이다. [열째 조각](dotnet-original-core-neighbor-20261020.md)이 남긴 다리 사건 경유 재생을 수행했다. 근거는 커밋된 cpppj 소스·보고서([다리 사건 재구성](exe/cpp-bridge-events-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawBridgeEvent.cs` | 다리 사건 처리기(원본 `00422740`)와 칸 변환(원본 `004215d0`)의 10.78 경로. `BridgeEventState`(권한·진단·다리 타입)·`BridgeEventHooks`(7개)·`Handle`(0x2691 자기 삭제·0x2692 칸 변환·그 밖은 payload)·`ConvertEnd`(끝 칸 판정→임시 프레임 이웃 탐색→프레임 복구→수명·알림→하드 프레임 중단→새 객체 복사·Pop) | `RawBridgeEvents.cpp` |
| `X86RawBridgeEventTests.cs` | `neighbor-x86.tsv` Event 행 10.78 몫 **288회** 재생(반환 비트·효과 순서·다리/새 객체 슬롯 50바이트) | `NeighborTests.cpp` |
| `RawBridgeEventTests.cs` | 보호·분기 계약 12개(효과 누락·미처리 사건·권한·진단·form·프레임 범위·이웃 예외 복구·이웃 없음·하드 프레임·방향 글자·글자 없음·단어 알림·필드 복사) | `NeighborTests.cpp` 의 보호·합성 검사 |

순서는 원본 그대로 "권한 → 사건 분기 → 칸 변환(진단·권한 → 타입·프레임 검사 → 방향 글자 → 임시 프레임 이웃 탐색 → 프레임 복구 → 이웃 없으면 삭제 → 수명 비트·알림 → 하드 중단 → 새 객체 생성·필드 복사·이전 삭제·새 칸 Pop)"이다. 기준 판본은 10.78 뿐이다(CD 10.72 는 프레임·소유자·플래그 위치가 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 사건 경유 재생 | `neighbor-x86.tsv` Event 10.78 몫 **288회** | 반환 비트·효과 순서(임시 프레임·이웃·알림·생성·소유자·삭제·Pop)·다리 슬롯 50바이트·새 객체 슬롯 50바이트 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,195개 통과**(Assets 363 + Core 832; 이번 +13).

**변이 확인:** 구현 20곳을 하나씩 망가뜨려 **19곳이 실패로 잡혔다.** 처음에 3곳(형식 타입·프레임 범위·ftol 하위 절삭)이 잡히지 않았고, 앞의 2곳은 예외 종류·`ParamName`을 명시하도록 검사를 강화해 잡혔다. 남은 1곳(ftol 하위 절삭)은 관측 동작이 같다 — C#의 `(uint)` 실수 변환이 이미 64비트 절삭 뒤 하위 DWORD와 같은 값을 내고, 64비트 밖 값은 별도 검사가 0으로 막는다. 소스는 모두 원복했고 강제 전체 빌드로 잔재 없음을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- `MakeBridgeRegularHandler`(부모 가상 표로 처리기를 고르는 분배)는 C# Regular 처리기 타입이 아직 없어(W04 시간·Kernel·Process) 옮기지 않았다. 사건 본체는 이 조각으로 끝났다.
- Graph 계열(`IRawPostPopGraph` 구현체), CD 10.72·10.37, 실제 세션 연결과 파생 분배 허브는 여전히 미연결이다.
- `cpp_sync.py` 반영 커밋을 `ae6649e9`→**`0abdf33d`** 로 올린다(사이 dotnetpj 커밋 1건, 새 fixture 등록 없음).

## 4. 다음 조각

Graph 계열(`graph`·`rawgraph`·`regiongraph` 등) → 실제 세션 연결 준비. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
