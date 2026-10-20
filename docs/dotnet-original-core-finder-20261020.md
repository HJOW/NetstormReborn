# 원본 월드 코어 W02 아홉째 조각: 탐색(원시 찾기)

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 아홉째 조각이다. [여덟째 조각](dotnet-original-core-buildingspace-20261020.md)의 다음 지시(이웃·탐색) 가운데 탐색부터 옮겼다. 근거는 커밋된 cpppj 소스·보고서([원시 찾기 재구성](exe/cpp-rawfinder-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawFinder.cs` | 원시 찾기(원본 `004b16d0`·`004b1810`의 10.78 경로). `SquidSearchArea`·`SquidSearchState`·`RawSquidFinder`(Begin 사각형·플래그 검사·단계 설정 → Next y·x·단계 회귀, 매립 제외·체인 순환·풀 범위 검사) | `RawSquidFinder.cpp` |
| `RawSquidHash.cs` | `Cell()` 추가(버킷 번호 머리 참조. 탐색기·이웃·재생 검사가 쓴다) | `SquidHash.cpp` |
| `X86RawFinderTests.cs` | `rawfinder-x86.tsv` 10.78 몫 **584회** 재생(회귀 커서 15개·방문 순서, 회귀 중 슬롯 직접 변경 포함) | `RawFinderTests.cpp` |
| `RawFinderTests.cs` | 보호·생애주기 계약 6개(시작 전·지원 밖 플래그·자기 순환·풀 범위·필터·읽기 전용·발자국·타입 범위·보드 밖 좌표) | `RawFinderTests.cpp` 의 보호·생애주기 검사 |

순서는 원본 그대로 "플래그·확장 높이 검사 → 사각형 고정 → 단계 설정(표면 생략이면 1) → 머리 조회 → Next 회귀(다음 번호 선확보·매립 제외·겹침 판정·필터)"다. 무효 좌표는 건너뛰고, 타입·발자국 손상은 거부한다. 기준 판본은 10.78 뿐이다(CD 10.72 는 좌표 assert·추가 바이트 위치가 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 탐색 회귀 재생 | `rawfinder-x86.tsv` 10.78 몫 **584회**(영역 6종·플래그 0·1·4·5·회귀 중 4종 변경) | 커서 15개·방문 순서 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,170개 통과**(Assets 363 + Core 807; 이번 +7).

**변이 확인:** 구현 17곳을 하나씩 망가뜨려 모두 실패로 잡혔다. 처음에 4곳이 잡히지 않아 검사를 4건 보강했다(발자국 범위·보드 밖 좌표 제외·타입 범위 예외 종류·최소 단계 직접 계약). 소스는 모두 원복했다. 도달 가능 입력으로 잡히지 않는 곳은 없다. 체인 순환 검사 제거 변이는 무한 순환을 일으키므로 역조건(항상 멈춤)으로 바꿔 검출을 확인했다.

**주의(빌드 잔재):** 변이 확인의 빠른 쓰기·빌드·원복 반복 뒤에는 강제 전체 빌드를 다시 하고 전체 검사를 통과해야 한다. 이번에 중간 전체 검사 1회가 변이 잔재 이진(용량 검사 제거판)으로 실패했으며, 강제 전체 빌드 뒤 같은 전체 검사가 전부 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- 이웃(`neighbor`, `neighbor-x86.tsv` 576행 — `RawSquidNeighbors`·`Bridge::Connects`·프레임 표와 Pop·소유자·파괴 생애주기 연결이 필요해서 다음 조각에서 한다. `RawSquidNeighborWalk` 포함).
- 생애주기 Pre·Post 재생(`rawfinder` 4+4행 — 다리 생애주기 `RawBridgeLifecycle` 연결이 필요해서 미룸).
- Graph 계열(`IRawPostPopGraph` 구현체), CD 10.72·10.37.
- 파생 분배 허브는 여전히 미연결이다. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.
- `cpp_sync.py` 의 반영 커밋을 `748bcbd1`→**`e5c9005d`** 로 올렸다(사이 dotnetpj 커밋 1건이 이 조각의 전제이며 cpppj 변경은 없다).

## 4. 다음 조각

이웃(`neighbor`, `RawSquidNeighbors`·`RawSquidNeighborWalk`, 프레임·연결·생애주기 포함) → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
