# R0-4 다섯 번째 묶음 — 그래프 비표면 좌표 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 다섯 번째 묶음(`RawGraph` 비표면 좌표)을 처리했다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj (HEAD `73506bcb` 기준)

| cpppj | C# | 내용 |
| --- | --- | --- |
| `src/o/RawGraph.cpp` | `Netstorm.Core/Original/RawGraph.cs` | `RegionCoordinate` 신설: 비표면 영역 통지는 0쪽으로 절삭한 칸에서 탐색하고 범위 밖·비유한 값만 거부한다. `Coordinate`는 정수 검사를 유지한 채 범위 검사를 `RegionCoordinate`에 위임한다. `Region`·`Calculate`의 Pop 구간·`CalculatePostDestroy`는 표면 타입이면 `Coordinate`, 비표면이면 `RegionCoordinate`를 쓴다(소유자색·생산·미리보기 칸과 같은 원본 판정). 삭제 연결은 원문 첫 마스크의 거부·두 번째 코드 보정을 실행한 뒤 필요한 치환 방향만 요구한다(진단 메시지에 양쪽 좌표·타입·프레임을 담는다) |
| `src/o/RawGraph.h` | (같은 파일) | `RawGraphStorage`·공유 생성자·`ResetRawGraphTable`은 장기 물리 수명 분리라 R4 세션 조립 때 잇는다. 이번에는 손대지 않았다 |

미적용(연기): `island-population-x86.tsv`는 `RawIslandPopulation` 일가(런타임·FinishOwner·맵)의 것으로 `RawGraph`와 무관함을 확인했다.
별도 섬 모듈 단위에서 다룬다. CD 판본·세션 연결은 R4에서 잇는다.

## 2. 연결한 fixture와 검사

- 기존 연결 재실행: `rawgraph-x86.tsv`(1796행)·`island-graph-frame-x86.tsv`(24행)를 포함한 그래프·파괴 전체 단위가 그대로 통과한다.
- 신규 계약 6개(`RawGraphTests`): 비표면 소수 통지 절삭·표면 소수 거부·범위 밖/음수/비유한 거부(영역·Pop 예측·삭제 후 재계산 경로).
- `PORT_MAP.tsv`는 새 행 없이 `RawGraph.cs`의 base만 HEAD로 올렸다(바뀐 쌍 27개 유지).

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,425개(Assets 363 + Core 1017 + Client 45) 통과**.
- 변별력: 영역 셀 선택을 구형으로 되돌리는 변이에 신규 검사 2종 실패를 확인하고 원복했다.
- 삭제 연결 진단 분기는 기존 L1(1796·24·파괴 전 Suite) 통과로 회귀 없음을 확인했다. fixture가 변경 전(10-07/10-08)에 만들어져
  분기 자체는 코드 대조로 옮겼으며 전용 신규 검사는 두지 않았다.
