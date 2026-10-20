# 원본 월드 코어 W02 일곱째 조각: 프레임 지정

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 일곱째 조각이다. [여섯째 조각](dotnet-original-core-owner-20261020.md)의 다음 지시(프레임)를 수행했다. 근거는 커밋된 cpppj 소스·보고서([프레임 재구성](exe/cpp-setframe-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawFrame.cs` | 프레임 지정(원본 `004acee0`의 10.78 경로). `SquidFrameHooks`(크기·갱신·Unpop·Pop)·`SquidFrame`(플래그 검사→레벨 저장·비교→일치면 갱신 2회·현재면 레벨만·아니면 Unpop·쓰기·Pop or 0x50). `Frame()`·`StoreLevel()` 공개 | `SquidFrame.cpp` |
| `RawSquidHash.cs` | `ObjectLevel()` 추가(섬·다리 0, 큰 쪽 크기 2·4 경계로 1·2·3, 비유한·음수 거부) | `SquidHash.cpp` |
| `X86RawFrameTests.cs` | `setframe-x86.tsv` 10.78 몫 **1,920회** 재생(효과 순서·슬롯 50바이트) + Thunk 4건·보호 2종(빠진 효과·표 밖 프레임) | `SetFrameTests.cpp` |
| `RawFrameTests.cs` | 보호·분기 계약 4개(생성자·레벨 판정·갱신 2회·Unpop·Pop·4바이트 쓰기·최소 단계) | `SetFrameTests.cpp` 의 보호·합성 검사 |

순서는 원본 그대로 "플래그 assert → 부호 바이트 레벨·현재 프레임 읽기 → 레벨 저장 → 일치면 갱신·쓰기·갱신, 현재와 같으면 종료, 아니면 Unpop·쓰기·현재 위치 Pop(|0x50)"이다. 레벨 바이트는 부호 있게 읽는다. 기준 판본은 10.78 뿐이다(CD 10.72·10.37은 프레임·레벨 바이트 위치와 쓰기 폭이 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 프레임 변경 재생 | `setframe-x86.tsv` 10.78 몫 **1,920회**(종류 4종·레벨 0·1·2·3·255·플래그 0·0x2000) | 효과 순서(D·E·U·P + 현재 프레임 번호)·슬롯 50바이트 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,158개 통과**(Assets 363 + Core 795; 이번 +6).

**변이 확인:** 구현 19곳을 하나씩 망가뜨려 처음에 16곳이 실패로 잡혔다. 잡히지 않은 3곳은 검사를 3건 보강한 뒤(큰 프레임 4바이트 쓰기·최소 단계 직접 계약·타입 범위 예외 종류) 17곳이 잡혔다. 남은 2곳은 관측 동작이 같다 — 레벨 바이트 부호 읽기(계산 레벨 0~3은 255와 −1 어느 쪽과도 같아질 수 없음), 타입 범위 검사 생략(인터페이스 인덱서가 쓰기 전에 같은 `ArgumentOutOfRangeException`을 던짐). 모두 방어용 중복으로 유지한다.

## 3. 이번에 하지 않은 것 (경계)

- 실제 Unpop·Pop 연결 검사(`set_frame_real_unpop_and_pop_move_hash_level_on_next_change` — 실제 `SquidPop`·`SquidUnpop`·`SquidFactory` 연결과 Pop API 정합이 필요해서 L2 단계로 미룸). 표시 레벨 계산·효과 순서는 이 조각의 재생·합성 검사로 끝냈다.
- 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide`를 구현)·이웃·탐색, Graph 계열(`IRawPostPopGraph` 구현체), CD 10.72·10.37.
- 파생 분배 허브(`SetAssetPostPop`·`HandlesAsset` 등, `HandlesDerived`는 여전히 거짓)는 파생 몸체가 모인 뒤 별도 조각에서 한다.
- `cpp_sync.py` 의 반영 커밋을 `2856544d`→**`d5dc54ac`** 로 올렸다(사이 dotnetpj 커밋 1건이 이 조각의 전제이며 cpppj 변경은 없다).

## 4. 다음 조각

건물 공간(`buildingspace`)·이웃·탐색 → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
