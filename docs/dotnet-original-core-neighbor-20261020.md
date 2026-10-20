# 원본 월드 코어 W02 열째 조각: 이웃

2026-10-20, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열째 조각이다. [아홉째 조각](dotnet-original-core-finder-20261020.md)의 다음 지시(이웃) 가운데 첫 이웃 탐색을 옮겼다. 근거는 커밋된 cpppj 소스·보고서([이웃 재구성](exe/cpp-neighbor-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cppjp 근거 |
| --- | --- | --- |
| `RawNeighbor.cs` | 이웃 탐색(원본 `004b23e0`·`004ade70`·`004adea0`·`0041ce90`의 10.78 경로). `NeighborFlags`(1·2·4)·`SquidFootprint`·`RawSquidNeighbors`(발자국·교차·결합·죽음·abstract·spot 판정)·`RawSquidNeighborWalk`(회귀). 결합 판정은 기존 `BridgeSurfaceRules.Connects`를 재사용한다 | `RawSquidNeighbors.cpp` |
| `X86RawNeighborTests.cs` | `neighbor-x86.tsv` 10.78 몫 **288회** 재생(첫 이웃 번호) | `NeighborTests.cpp` |
| `RawNeighborTests.cs` | 보호·기하 계약 11개(미방문 손상 중단·번호 범위·방향·발자국·abstract·죽음·spot 편향·범위·순서·타입·보드 밖) | `NeighborTests.cpp` 의 보호·기하 검사 |

순서는 원본 그대로 "발자국 읽기·안쪽 판정 → 사각형 확장 탐색 → 교차(정확 겹침만 제외)·결합(NoJoint 아니면)·죽음·abstract·spot(0.9999 절삭) 순서의 후보 검사"다. 방향·결합 글자 판정은 기존 다리 표면 규칙을 그대로 쓴다. 기준 판본은 10.78 뿐이다(CD 10.72 는 추가·프레임 위치가 달라서 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 첫 이웃 재생 | `neighbor-x86.tsv` 10.78 몫 **288회**(발자국·표면·결합·spot 포함 장면) | 첫 이웃 번호 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,182개 통과**(Assets 363 + Core 819; 이번 +12).

**변이 확인:** 구현 30곳을 하나씩 망가뜨려 모두 실패로 잡혔다. 처음에 6곳이 잡히지 않아 검사를 6건 보강했다(abstract·죽음·spot 편향·범위 종류·타입 범위·보드 밖 좌표). 소스는 모두 원복했다. 체인 순환 제거는 무한 순환을 일으키므로 역조건으로 바꿔 검출을 확인했고, 빈 번호 검사는 같은 `ArgumentOutOfRangeException`을 던지므로 방어용 중복으로 유지한다. 변이 반복 뒤에는 강제 전체 빌드를 다시 하고 전체 검사를 통과했다.

## 3. 이번에 하지 않은 것 (경계)

- 다리 사건 경유 재생(`neighbor` Event 288회 — `RawBridgeEvents` 연결이 필요해서 다리 작업과 함께 별도 조각에서 한다).
- Graph 계열(`IRawPostPopGraph` 구현체), CD 10.72·10.37.
- 실제 세션 연결과 파생 분배 허브는 여전히 미연결이다.
- `cpp_sync.py` 의 반영 커밋을 `e5c9005d`→**`ae6649e9`** 로 올린다. 시작 HEAD가 전제 커밋과 같아 사이 cpppj 변경 검토가 없고, 쓸 fixture를 새로 1개 등록했다(연결 fixture 31→**32**개).

## 4. 다음 조각

다리 사건(`RawBridgeEvents`, Event 288회 재생) → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
