# 원본 월드 코어 W02 열두째 조각: 미니맵 갱신 큐

2026-10-20, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 열두째 조각이다. [열한째 조각](dotnet-original-core-bridgeevent-20261020.md)이 다음 시작점으로 남긴 Graph 계열 가운데, 먼저 Pop·Unpop 의 연결 지점을 실제로 채우는 `RawMetadisplayDirty`(미니맵 갱신 큐)를 옮겼다. Graph 계열은 이 조각 뒤에도 남는다. 근거는 커밋된 cpppj 소스·보고서([Sky Overview 미니맵 dirty 큐](exe/cpp-metadisplay-dirty.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawMetadisplayDirty.cs` | 10.78 의 미니맵 큐·Refresh(원본 등록 `00481510`, 화면별 처리 `00481320`). `MetadisplayDirtyState`(표시 유무·활성 비트·128×128 bitmap·100칸 큐·count·포화·초기화·캐시 view·프레임)·`MetadisplayDirtyCell`·`MetadisplayDirtyView`·`MetadisplayDirtyHooks`(rebuild `00480270`, 셀 그리기 `004808d0`). `IRawPopMetadisplay`·`IRawMinimapQueue` 를 함께 구현해 Pop·Unpop 의 기존 연결 지점에 바로 붙는다 | `RawMetadisplayDirty.cpp`, `cpp-metadisplay-dirty.md` |
| `X86RawMetadisplayDirtyTests.cs` | `metadisplaydirty-x86.tsv` 10.78 몫 **520행** 재생(입력 13 DWORD·결과 15 DWORD) | `MetadisplayDirtyTests.cpp` |
| `RawMetadisplayDirtyTests.cs` | 경계·연결 검사 3개(같은 풀·현재 슬롯 좌표·다른 풀 거부, 비활성 bitmap 확보·필수 효과, 실제 Pop/Unpop 통지 시점·겹침 순서) | 같은 파일의 보호·합성 검사 |

순서는 원본 그대로 "표시 활성 검사 → 상태 검사 → 4×4 칸 번호 변환 → 같은 칸 중복 검사 → 꽉 차면 전체 갱신 요구, 아니면 추가"이고, Refresh 는 "bitmap 최초 확보(128×128, 전체 갱신 요구) → 비활성 반환 → 초기화 비트 → rebuild 조건(큐·포화·카메라·사각형) → 프레임 증가 → 전체 64×64 또는 큐의 부분 셀 그리기 → count·포화 지우기"다. 칸 번호는 float 를 0 쪽으로 절삭한 뒤 4로 나눈 signed 정수라 -3.9 는 0, -4.0 은 -1 이다(floor 아님). 기준 판본은 10.78 뿐이다(CD 10.72 의 NaN 카메라 동등 처리는 옮기지 않았다).

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| 큐 등록·Refresh 재생 | `metadisplaydirty-x86.tsv` 10.78 몫 **520행**(등록 384·Refresh 136) | 큐 count·포화 표시·bitmap 폭·초기화·프레임·캐시 카메라 2·캐시 사각형 4·**큐 100칸/bitmap/관찰 사건 FNV-1a**·사건 수 |
| 같은 칸 중복·현재 좌표 | 합성 | -7.99 → -1, 8.25 → 2, 7.99 → 1, 같은 칸 재통지 무시, 무한 좌표 거부, 다른 풀 거부 |
| Pop·Unpop 연결 | 합성 | 겹침 Pop 도 좌표를 쓴 뒤 통지·Registered 중복 없음·매몰/비섬 타입/void 재해제는 통지 없음·다른 풀 연결 거부 |

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,199개 통과**(Assets 363 + Core 836; 이번 +4).

**변이 확인:** 구현 19곳(칸 나눗셈 4→2, 절삭을 floor 로, 중복 검사에서 Y 생략, 포화 표시 값 2, bitmap 최초 확보 생략, 초기화 비트 1 생략, rebuild 조건에서 큐/포화/카메라/사각형 각각 생략, 전체 셀 그리기 순서 x·y 교환, 부분 그리기에서 0번 칸 고정, 부분 루프에서 count 미갱신, 통지 함수의 풀 검사 생략, 표시 활성 비트 검사 생략, 프레임 증가 생략, count 조기 초기화, 두 번째 초기화의 사각형 초기화 생략, 전체 루프 x 경계 -1)을 하나씩 넣어 **19곳 모두 실패로 잡혔다.** 소스는 모두 원복했고, 되돌린 뒤 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **Graph 계열**(`graph`·`rawgraph`·`regiongraph`·`graphremove`·`graphrebuild`·`graphrecovery`·`graphlookup`·`graphforms`·`graph-reset`)은 여전히 미이식이다. `SurfaceGraph`(Bridges)가 cpppj `Graph` 의 표·flood·할당·반납을 이미 옮겼으므로 다음 조각은 그 위의 `RawGraph` 래퍼(원본 슬롯→표·영역 무효화·삭제·재구성)다.
- 미니맵 bitmap 을 실제로 그리는 표시 모듈, 화면별 tick 연결, 실제 세션 연결은 하지 않았다.
- CD 10.72·10.37, 실제 세션 연결, 파생 분배 허브는 그대로 미연결이다.

## 4. 다음 조각

`RawGraph`(원본 `RawGraph.cpp` — 표 저장소·`ResetTable`·`Free`·표면 영역 무효화·추가/flood·삭제 준비·전체 재구성·`PostDestroy`) → 실제 세션 연결 준비. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
