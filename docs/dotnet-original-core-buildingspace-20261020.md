# 원본 월드 코어 W02 여덟째 조각: 건물 공간

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 여덟째 조각이다. [일곱째 조각](dotnet-original-core-frame-20261020.md)의 다음 지시(건물 공간·이웃·탐색) 가운데 건물 공간부터 옮겼다. 근거는 커밋된 cpppj 소스·보고서([건물 공간 재구성](exe/cpp-buildingspace-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawBuildingSpace.cs` | 건물 옆면 공간(원본 `0040da00`·`0046f680`·`004214a0`의 10.78 경로). `BuildingSpaceState`(섬·다리 타입·진단)·`Surface`·`HasIsland`·`PopSide`·`UnpopSide`·`NotifySurface`·`ValidateSpace`. `IRawPopBuildingSide`·`IRawBuildingSide`를 구현해 Pop·Unpop 에 연결한다 | `RawBuildingSpace.cpp` |
| `AssemblyInfo.cs` | 테스트 어셈블리에 internal 공개(미할당 슬롯 raw 심기용). 실행 경로에 영향 없음 | — |
| `X86RawBuildingSpaceTests.cs` | `buildingspace-x86.tsv` 10.78 몫 **64 Begin·123 Step** 재생(Pop 결과·풀 전체·해시 머리·점유 지도 체크섬·앞 192슬롯 바이트) | `BuildingSpaceTests.cpp` |
| `RawBuildingSpaceTests.cs` | 보호 계약 4개(생성자·연결·표면·섬·진단·옆면 범위) | `BuildingSpaceTests.cpp` 의 보호 검사 |

순서는 원본 그대로 "표면 조회(지도 밖 0) → Pop 옆면(표면 아니면 둠·섬이면 bit2·아니면 bit1·2 지움·진단) → Unpop 옆면(bit1·2 지움·진단) → 다리 수명 nibble 7 초과면 진단 모드에서 멈춤"이다. 절삭은 0 방향이며 범위를 벗어나면 거부한다. 기준 판본은 10.78 뿐이다(CD 10.72 는 표면 شرط·진단 방식이 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 건물 등록·해제 재생 | `buildingspace-x86.tsv` 10.78 몫 **64 Begin·123 Step**(Pop 64·Unpop 59) | Pop 결과·풀 전체 Adler-32·해시 86,272머리 Adler-32·점유 65,536칸 Adler-32·앞 192슬롯 바이트 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,163개 통과**(Assets 363 + Core 800; 이번 +5).

**변이 확인:** 구현 18곳을 하나씩 망가뜨려 처음에 14곳이 실패로 잡혔다. 잡히지 않은 4곳은 검사를 2건 보강한 뒤(풀 범위 표면 번호·매립/비다리 진단 제외) 16곳이 잡혔다. 남은 2곳은 관측 동작이 같다 — 빈 번호 검사 생략(0번 슬롯 타입이 섬 번호와 같아질 수 없음), 표면 타입 범위 검사 생략(인터페이스 인덱서가 쓰기 전에 같은 `ArgumentOutOfRangeException`을 던짐). 모두 방어용 중복으로 유지한다.

## 3. 이번에 하지 않은 것 (경계)

- 이웃·탐색(`neighbor`·`rawfinder` fixture) → 다음 조각에서 한다.
- Graph 계열(`IRawPostPopGraph` 구현체 — `SurfaceGraph` 어댑터는 Graph 조각에서), CD 10.72.
- 실제 세션 연결(`BattleMap`·`BridgeGrid`는 아직 기존 규칙을 쓰며, Pop·Unpop 의 건물 옆면은 이 조각의 재생 검사에서만 연결한다). 파생 분배 허브는 여전히 미연결이다.
- `cpp_sync.py` 의 반영 커밋을 `d1d5643c`→**`748bcbd1`** 로 올렸다(사이 dotnetpj 커밋 2건은 점검·보완이며 cpppj 변경은 없다).

## 4. 다음 조각

이웃·탐색(`neighbor`·`rawfinder`) → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
