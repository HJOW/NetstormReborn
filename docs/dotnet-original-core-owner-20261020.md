# 원본 월드 코어 W02 여섯째 조각: 소유자 지정과 가변 소유자 재생

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 여섯째 조각이다. [다섯째 조각](dotnet-original-core-assetpostpop-20261020.md)의 다음 지시(소유자·프레임) 가운데 소유자 지정부터 옮기고, 묶은 Player 기록의 가변 소유자 재생도 함께 했다. 근거는 커밋된 cpppj 소스·보고서([소유자 재구성](exe/cpp-owner-reconstruction.md), [자산 후처리 복원](exe/cpp-asset-postpop.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawOwner.cs` | 소유자 지정(원본 `004adf00`·CD `004aefa0`의 10.78 경로). `SquidOwnerMode`(편집·전투·도전)·`SquidOwner.Set`(번호 범위 검사→이전 목록 제거→바이트 쓰기→빈·매립이 아닌 새 목록 추가) | `SquidOwner.cpp` |
| `X86RawOwnerTests.cs` | `owner-x86.tsv` 10.78 몫 **1,000회** 재생(소유자 바이트·9개 목록·슬롯 Adler-32) | `OwnerTests.cpp` |
| `RawOwnerTests.cs` | 보호 계약 3개(범위 밖 번호의 쓰기 전 거부·타입 표·작업장 목록 이동·빈·매립·모드 밖·가득 참) | `OwnerTests.cpp` 의 보호 검사 |
| `X86RawAssetPostPopTests.cs` | `AssetBodiesMatchOriginalX86WithMutablePlayers` 추가. `asset-player-x86.tsv` 10.78 몫 **288회**(충격음·소유자 쓰기 뒤에 바뀌는 battleOwner DWORD 로 신전을 다시 읽는다) | `AssetPostPopTests.cpp` 의 ReplayMutablePlayers |

순서는 원본 그대로 "번호 범위 검사(일반 0~8·도전 1~39, 목록 번호 0~8) → 이전 목록 제거 → 바이트 쓰기 → 빈·매립이 아니면 새 목록 추가"다. 소유자 0은 목록에 들지 않고, 자리·개수 밖의 stale 값은 보존한다. 기준 판본은 10.78 뿐이다(CD 10.72·10.37은 소유자·추가 바이트 위치가 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 소유자 지정 재생 | `owner-x86.tsv` 10.78 몫 **1,000회**(모드 6조합·부류 8종·상태·추가·이전 소유자) | 소유자 바이트·9개 목록(개수·전체 바이트)·슬롯 Adler-32 |
| 가변 소유자 재생 | `asset-player-x86.tsv` 10.78 몫 **288회**(효과 중 battleOwner 3→1·8 변경) | 효과 순서·슬롯 50바이트·전체 기록 1,620바이트·최종 소유자·공통 깊이 0 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,152개 통과**(Assets 363 + Core 789; 이번 +4).

**변이 확인:** 구현 16곳(소유자·추가 오프셋·작업장 마스크·모드 조건·이전 목록 조건·빈·매립·0번 추가·제거·추가 생략·목록 번호 검사·도전 0 허용·도전 상한·일반 상한·타입 수·가변 소유자 무시)을 하나씩 망가뜨려 **14곳이 실패로 잡혔다.** 소스는 모두 원복했고 강제 전체 빌드로 잔재 없음을 확정했다. 남은 2곳(도전 상한 `40` 허용·일반 상한 `9` 허용)은 loosened 첫 검사가 새로 받아들이는 번호를 두 번째 목록 검사가 쓰기 전에 그대로 거부하므로 관측 동작이 같다(방어용 중복 검사로 유지).

## 3. 이번에 하지 않은 것 (경계)

- 프레임(`setframe`, `setframe-x86.tsv` 5,777행·표준 1,920회) — `SquidHash::ObjectLevel`·타입 genus 판정·실제 Unpop·Pop 연결이 필요해서 다음 조각에서 한다.
- 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`를 구현)·이웃·탐색, Graph 계열(`IRawPostPopGraph` 구현체), CD 10.72·10.37.
- 공통 후처리 쪽의 파생 분배 허브(`SetAssetPostPop`·`HandlesAsset` 등, `HandlesDerived`는 여전히 거짓)는 파생 몸체가 모인 뒤 별도 조각에서 한다.
- `cpp_sync.py` 의 반영 커밋을 `6559604e`→**`2856544d`** 로 올렸다(사이 dotnetpj 커밋 1건이 이 조각의 전제이며 cpppj 변경은 없다).

## 4. 다음 조각

프레임(`setframe`) → 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`를 구현)·이웃·탐색 → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
