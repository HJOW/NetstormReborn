# 원본 월드 코어 W02 다섯째 조각 후반: 자산 전용 후처리

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 다섯째 조각 후반부다. [전반부(운반체 Damageable 몸체)](dotnet-original-core-carrierpostpop-20261020.md)에 이어 자산 전용 후처리 본체를 옮겼다. 근거는 커밋된 cpppj 소스·보고서([자산 후처리 복원](exe/cpp-asset-postpop.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawAssetPostPop.cs` | 자산 전용 후처리(10.78). `AssetPostPopState`(전역·목록·소유자별 신전 기록)·19개 `AssetPostPopHooks`·`BindPlayerRecords`(실제 9기록 1,620바이트 묶음, 미묶음이면 state 사본). 분배: 제네레이터·Gun 4종·장식·거주 구역·바리케이드·간헐천·신전 3종·전용 4종(훨리베이스·맨 오 워 풀·썬 바리케이드·너겟). 공통·Damageable 장부를 각각 정확히 한 번 호출한다 | `RawAssetPostPop.cpp` |
| `X86RawAssetPostPopTests.cs` | `assetpostpop-x86.tsv` 10.78 몫 **818회** 재생(효과 순서·슬롯 50바이트·세 목록·Player 신전·주변 받침). 파생 Activate 의 깊이 증가·공통 감소를 `Depth++`→본체→0 확인으로 재현한다. 묶은 기록 재생(신전 DWORD 가 묶은 바이트에만 가는지 전체 바이트 대조) 포함 | `AssetPostPopTests.cpp` |
| `RawAssetPostPopTests.cs` | 보호 계약 6개(생성자 풀·타입 표·지원 표·Gun 필수 효과·간헐천 비유한 비용·전용 4종 분배·보스 영역 삭제) | `AssetPostPopTests.cpp` 의 보호 검사 |

순서는 원본 그대로 "종류 판정 → 공통 검사 → 분기별 필수 효과 확인(Validate) → 전용 몸체(Gun·거주 구역·간헐천·신전 공통 몸체 후 원소 포장) → Damageable·공통 몸체"다. 잘못된 가상 표·타입 번호·미연결 효과·범위 밖 소유자·비유한 비용은 풀·목록·기록을 바꾸기 전에 거부한다. 기준 판본은 10.78 뿐이다(CD 10.72 는 가상 표·추가 바이트 폭·거주 구역 예약·소용돌이 좌표가 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 전용 몸체 재생 | `assetpostpop-x86.tsv` 10.78 몫 **818회**(종류 12종·플래그 0·1·2·7·전투·로딩·보스·내용물·목록 0·4) | 효과 순서·대상 슬롯 50바이트·세 목록(개수·전체 바이트)·Player 신전 4개·주변 받침 워드·공통 깊이 0 |
| 묶은 기록 재생 | 같은 818회(9기록 stale 0xa5·신전 16바이트 0·초기값 901~904) | 묶은 전체 1,620바이트·state 사본 불변·깊이 0 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,148개 통과**(Assets 363 + Core 785; 이번 +10).

**변이 확인:** 구현 20곳(Gun 분기·썬 캐논 추가 조건·거주 구역 소유자·사건 3·하위 니블·간헐천 비용·받침 프레임·신전 개수·초기 신전·충격음·추가 0x130·포장 0x134·종류표·종류 검사 생략·보스 삭제 생략·영역 상수 참·로딩 생산·내용물 검사 생략·기록 stride·너겟 필수 효과)을 하나씩 망가뜨려 모두 실패로 잡혔다. 처음에 2곳(종류 검사 생략·영역 상수 참)이 잡히지 않아 검사를 2건 보강했다(비최초 미지원 표의 사전 거부, 영역이 다를 때 삭제 금지). 소스는 모두 원복했고 강제 전체 빌드로 잔재 없음을 확정했다. 도달 가능 입력으로 잡히지 않는 곳은 없다.

## 3. 이번에 하지 않은 것 (경계)

- 소유자(`owner`, `owner-x86.tsv` 3,000행)·프레임(`setframe`, `setframe-x86.tsv` 5,777행), 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`를 구현)·이웃·탐색, Graph 계열(`IRawPostPopGraph` 구현체), CD 10.72. 가변 소유자 DWORD 포인터(`BindPlayerRecords` 둘째 인자)의 실제 battleOwner 변경 재생(`asset-player-x86.tsv` 288행)은 다음 조각에서 한다.
- 공통 후처리 쪽의 파생 분배 허브(`SquidPostPop::SetAssetPostPop`·`HandlesAsset` 등) — C# `RawPostPop.HandlesDerived` 는 여전히 거짓이다. 이 조각의 재생은 본체를 직접 호출하고 Activate 깊이만 재현한다. 허브 연결은 파생 몸체가 모인 뒤 별도 조각에서 한다.
- `cpp_sync.py` 의 반영 커밋을 `2cdcf81e`→**`6559604e`** 로 올렸다(사이 dotnetpj 커밋 1건이 이 조각의 전제이며 cpppj 변경은 없다).

## 4. 다음 조각

소유자(`owner`)·프레임(`setframe`, `asset-player-x86.tsv`의 가변 소유자 재생 포함) → 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`를 구현)·이웃·탐색 → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
