# 원본 월드 코어 W02 다섯째 조각 전반: 운반체 Damageable 몸체

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 다섯째 조각 중 전반부다. [넷째 조각](dotnet-original-core-postpop-20261020.md)의 다음 지시(`assetpostpop`, 운반체 Damageable 몸체와 함께) 가운데, 자산 후처리의 전제가 되는 운반체 몸체부터 옮겼다. 근거는 커밋된 cpppj 소스·보고서([자산 후처리 복원](exe/cpp-asset-postpop.md), [운반체 후처리 복원](exe/cpp-carrier-postpop-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawCarrierPostPop.cs` | 운반체 직접 호출(`00427720`)·Damageable 직접 호출(`0044bf80`)의 10.78 경로. `CarrierPostPopState`(보스·로딩 깊이·간헐천 타입)·`CarrierPostPopHooks`(확인·지면 갱신·공통 검사·공통 몸체)·`MakeCarrierPostPopHooks`(같은 풀의 `RawPostPop`에 연결). Validate→(비최초·비보스면 +0xCC 확인)→RunDamageable(로딩 중 조건이면 지면 갱신→공통 몸체) | `RawCarrierPostPop.cpp` |
| `X86RawCarrierPostPopTests.cs` | `carrierpostpop-x86.tsv` 10.78 몫(판본별 2,448행 중 `originals` 2,448회: 직접 호출·Damageable 호출) 재생. 효과 순서와 대상 슬롯 50바이트 대조 | `CarrierPostPopTests.cpp` |
| `RawCarrierPostPopTests.cs` | 보호 경로·연결 지점 2개(손상 거부·직접 공통 호출의 장부·가드) | `CarrierPostPopTests.cpp` 의 보호 검사 |

순서는 원본 그대로 "상태·타입 검사→공통 검사(ValidateBase)→비최초·비보스 +0xCC 확인→로딩 중 조건의 지면 갱신→공통 몸체"다. CD 10.72·10.37 행은 기준 판본이 아니므로 읽지 않는다(C++는 세 판본을 모두 재생하지만 C# 코어는 10.78 전용이며, 이전 조각과 같다). 잘못된 빈 번호·자산 밖 타입·내용물 상태·미연결 효과는 풀·효과를 바꾸기 전에 예외로 거부한다.

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 직접·Damageable 호출 재생 | `carrierpostpop-x86.tsv` 10.78 몫 **2,448회**(직접 1,968·Damageable 480) | 효과 순서·대상 슬롯 50바이트 |
| 직접 공통 호출의 장부·가드 | 합성 입력(사제 158번·비용 123.75·로컬 1번·장부 10·깊이 1) | 지면 갱신 1회·깊이 1→0·비용 10→133·로컬/전체 통계·AI 부착 거부·억제 경로 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,138개 통과**(Assets 363 + Core 775; 이번 +3).

**변이 확인:** 구현 15곳(보스 반전·최초 플래그 반전·로딩 우회·간헐천 예외 삭제·abstract·매립 검사 삭제·지면 비트 변경·소유자·추가·X 오프셋 변경·상태 마스크 축소·공통 검사 생략·기본 몸체 플래그 변조·타입 수 검사·효과 null 검사·풀 불일치 검사를 각각 무력화)를 하나씩 망가뜨려 **15곳 모두 실패로 잡혔다.** 소스는 모두 원복했고 강제 전체 빌드로 잔재 없음을 확정했다. 자산 타입 하한(`FirstAssetType`) 변이는 도달 가능한 입력이 없어 잡히지 않는다 — fixture 타입은 82·122·158뿐이고 70 미만 입력은 빈 비트·타입 검사가 항상 먼저 거부한다(방어용 중복 검사로 유지, 넷째 조각의 남은 1곳과 같은 부류).

## 3. 이번에 하지 않은 것 (경계)

- `RawAssetPostPop` 본체(Gun·거주 구역·간헐천·신전 분배, 19개 효과 연결) — 다음 조각에서 이 모듈의 Damageable 경로에 연결한다. 자산 fixture(`assetpostpop-x86.tsv`)는 아직 연결하지 않았다.
- 소유자(`owner`, `owner-x86.tsv` 3,000행)·프레임(`setframe`, `setframe-x86.tsv` 5,777행), 건물 공간·이웃·탐색, Graph 계열, CD 10.72·10.37. 사제 접두·프로세스 호스트와 엮는 결합 검사는 사제 후처리(W08)가 없어서 하지 않는다.
- `carrierpostpop` 은 계획상 W09(경로·운반) 소속이지만, W02 자산 후처리의 전제이므로 먼저 가져왔다(다리 접두를 PostPop 조각에서 먼저 가져온 것과 같은 방식). W09 이행 때 이 모듈을 다시 열지 않고 fixture 재생을 재사용한다.
- `cpp_sync.py` 의 반영 커밋을 `65674f52`→**`2cdcf81e`** 로 올렸다(사이 cpppj 커밋 1건은 공통 객체 명령 추가로, 연결 fixture·이식 모듈에 영향 없음).

## 4. 다음 조각

`assetpostpop`(자산 전용 후처리, 이 조각의 Damageable 몸체에 연결)·소유자(`owner`)·프레임(`setframe`) → 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`를 구현)·이웃·탐색 → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
