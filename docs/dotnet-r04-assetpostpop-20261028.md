# R0-4 마지막 묶음 — 자산 후처리 4종과 호출 시점 TYPE·기술 비트 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 마지막 묶음(`RawAssetPostPop` 4종)을 처리했다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj (HEAD `9ed8bc63` 기준)

| cpppj | C# | 내용 |
| --- | --- | --- |
| `src/o/RawAssetPostPop.cpp` | `Netstorm.Core/Original/RawAssetPostPop.cs` | 분배표 20→24개. 데빌메이커·아크 스파이어·애시드 바리케이드·성장형 아이스 타워 종류를 추가하고, 호출 시점의 TYPE을 `Type(sid)`로 다시 읽는다(Gun 2곳·간헐천 비용). 스파이어 2종은 최초 관문만 Regular 예약을 요구하고 전용 PostPop으로 분배한다 |
| `src/o/RawArcSpire.cpp` | `Netstorm.Core/Original/RawArcSpire.cs`(신규) | `PostPopArcSpire`만 이식. 비추가·비매립 최초 호출에 사건 0을 1초 뒤로 예약하고 항상 같은 Damageable 후처리로 이어진다. 전체 주기·빔 몸체는 R4에서 잇는다 |
| `src/o/RawAcidFence.cpp` | `Netstorm.Core/Original/RawAcidFence.cs`(신규) | `PostPopAcidFence`만 이식. 최초 호출에 사건 0을 0.3초 뒤로 예약하고 항상 같은 Damageable 후처리로 이어진다. 전용 주기·포획 몸체는 R4에서 잇는다 |
| `src/o/RiftType.h` | `Netstorm.Core/Original/OriginalTypeRecord.cs` | `TechBitIndex`(기본 -1, PATCH +0x1ec·CD +0x1cc)를 추가하고 복사본에 보존한다 |

미적용(연기): `AssetPostPopState` 공유 저장소 접근자(`Boss()`·`Residences()` 등)와 `types_` span 전환은 C#이 이미 참조 의미(`IReadOnlyList` 바깥 소유)로 들고 있어 동작이 같으므로 손대지 않았다. 공유 포인터·세션 연결은 R4에서 잇는다(앞 묶음의 `RawGraphStorage`와 같은 판정).
`arc-spire`·`acid-fence`·`ice-tower` fixture는 전체 주기·빔·성장 몸체(RawArcSpire·RawAcidFence·RawIceTower 클래스)를 대조하므로 이번에 연결하지 않았다. R4 세션 조립(동맹·편집기·송신·Zap·프레임 표 필요)에서 새 모듈 단위로 이식한다.
CD 판본·세션 연결도 R4에서 잇는다.

## 2. 연결한 fixture와 검사

- 기존 연결 재실행: `assetpostpop-x86.tsv`(10-13 생성, 12종·818행 중 10.78 몫)를 포함한 Core 전 Suite가 그대로 통과한다. fixture가 24표 변경(10-22)보다 먼저 만들어져 새 4종은 신규 계약 검사로 고정했다.
- 신규 계약 5개(`RawAssetPostPopTests`): 데빌메이커·성장형 아이스 분배와 필수 효과 요구(예외 구분 포함), 스파이어 2종 최초 예약(아크 1초·애시드 0.3초)과 Damageable 연결, 최초 관문에서만 Regular 요구(추가 비트·비최초는 통과), 외부 효과가 바꾼 슬롯 타입의 호출 시점 재조회(표 밖은 명시적 범위 오류), 기술 비트 기본 -1과 복사 보존.

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,430개(Assets 363 + Core 1022 + Client 45) 통과**.
- 변별력: 분배표에서 새 4행을 지우는 변이에 신규 검사 3종 실패를 확인하고 원복했다.
- 스파이어 정적 함수의 `reserve`·`regular` 지연 검사(C++ `Need` 의미)는 계약 검사로 고정했다. 비최초·추가 비트 경로에서 뭉치가 비어도 건드리지 않음을 확인했다(`SpireBodiesRequireRegularOnlyOnFirstPostPop`).
- `PORT_MAP.tsv`는 91행이 됐다(새 행 2개, base 갱신 2개, 바뀐 쌍 27→25개).
