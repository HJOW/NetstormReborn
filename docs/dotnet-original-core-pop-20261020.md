# 원본 월드 코어 W02 셋째 조각: 공간 등록(Pop)

2026-10-20, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 셋째 조각이다. [둘째 조각](dotnet-original-core-unpop-20261016.md)(공간 해제·해시)에 이어 `dotnetpj/src/Netstorm.Core/Original/`에 공간 등록 Pop 을 옮겼다. 근거는 커밋된 cpppj 소스·보고서([cpp-pop-reconstruction.md](exe/cpp-pop-reconstruction.md))와 원본 기계어 fixture 다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다. 이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawPop.cs` | 공간 등록 Pop(`004b02d0`)의 10.78 경로. 결과는 `RawPopResult`(Unchanged·Registered·Overlap). 연결 지점 4개(`IRawUnpopDisplay` 공유·`IRawPopBuildingSide`·`IRawPopMetadisplay`·`IRawPopPostPop`). 순서는 원본 그대로 "유효 검사 → 좌표·화면 좌표 쓰기 → (섬·다리면) 미니맵 알림 → 발자국 spot 등록(겹치면 Overlap) → 단계·next 쓰기와 머리 연결 → (0단계 다리가 아니면) 섬 번호 127 → 표시 갱신 → 최초 Pop 비트·void 해제 → 파생 Activate" | `004b02d0`, `SquidPop.cpp` |
| `OriginalPopDisplay.cs` + `.Data.cs` | 파생 firstPop/postPop 지원 여부. 마나 대체 주소 3개는 앞 분기(0x50C3B0·0x509810 은 원본 표시 지원의 무조건 지원 분기로 true, 0x5070C0 은 공통 표시 지원), 표 26개는 공통 표시 지원으로 가린다. `.Data.cs` 는 [`gen_original_pop_vtables.py`](../dotnetpj/tools/gen_original_pop_vtables.py)가 cpppj 목록에서 생성 | `PopVtables.inc`, `SquidPop.cpp` |
| `RawSquidFactory.cs`·`RawUnpop.cs`·`OriginalTypeRecord.cs` | 변경 없음. Pop 은 팩토리·Unpop 과 같은 풀·해시·점유 지도를 공유한다 | — |

판정 과정에서 원본 상수 `0x50444200` 의 셋째 바이트(`0x42`)가 14번 비트(`0x4000`)를 포함함을 탐침 프로그램으로 확인했다. 처음에는 암산으로 마스크를 잘못 읽어 cpppj 보호 검사의 첫 예외(건물 공간 미연결)를 설명하지 못했으나, 빌드한 cpppj 라이브러리에 연결한 탐침이 실제 예외(`Pop 건물 부착 공간이 연결되지 않았습니다`)를 재현해 읽기를 바로잡았다. 코드는 처음부터 상수를 그대로 옮겼으므로 수정이 필요 없었다.

## 2. 검증 결과 (원본 fixture 의 10.78 몫)

| 검사 | fixture | 맞춘 값 |
| --- | --- | --- |
| `RawPop`·`RawUnpop`·생성·반납 재생 | `pop-x86.tsv` 10.78 몫 4개 Begin·**2,456회**(Create 376·Pop 1,016·Unpop 688·Release 376; Pop 은 Registered 688·Unchanged 320·Overlap 8) | 반환 번호·풀 전체 바이트·**네 단계 해시 머리·65,536칸 점유 지도 Adler-32**, 대상 슬롯 50바이트 전부 |
| 파생 지원 주소 | `pop-vtables.tsv` 10.78 **83**행 | 표 26개 일치·마스크 0인 3개 중 마나 대체 주소는 지원(true), 나머지는 미지원. `Supports(v, 0)` 와 `Supports(v, 0x2000)` 이 같다 |

[`RawPopTests`](../dotnetpj/tests/Netstorm.Core.Tests/RawPopTests.cs) 8개가 보호 경로·연결 지점 순서·경계를 확인한다: 미지원 가상 표·건물 옆면 미연결(원본 상수에 14번 비트가 들어가는 `0x4000` 경우)·0이 되는 발자국 칸·빈/전송 중 슬롯·유한하지 않은 좌표·해시 머리 충돌은 풀·점유 지도·해시 머리를 바꾸기 전에 거부하고, 겹침은 좌표·화면 좌표까지만 쓰고 체인에 넣지 않으며, 표시 Validate→파생 Validate→미니맵 알림→건물 옆면→표시 Update→파생 Activate 순서와 두 번째 등록의 파생 플래그(`0x10`만)를 고정한다.

**변이 확인:** 구현 19곳(void 조기 반환 제거·transmitting 검사 제거·매몰 건물 검사 제거·마나 분기를 표시 표 조회로·표 항목 1개 제거·발자국 왼쪽 1 미만 판정을 0 미만으로·정규화 항상-OR·미니맵 매몰 스킵 제거·겹침 반환 제거·건물 옆면 호출 생략·단계 바이트 고정·섬 번호 조건 반전·최초 Pop 비트 쓰기 생략·void 해제를 설정으로·파생 Activate 비트 생략·표시 Update에 정규화 플래그·파생 Validate에 원본 플래그·화면 배율·화면 편향)을 하나씩 망가뜨려 모두 실패로 잡았다. 정규화 변이는 처음에 잡히지 않아 파생 Validate 기록을 검사하는 단언과 추가 비트 1이 있는 경우를 넣어 잡았다. 처음에 테스트 설정 쓰기(가상 표 덮기)를 스냅샷보다 먼저 두어 "바뀌지 않는다" 검사가 실패했고, 빈 슬롯이 예외가 아니라 Unchanged 임을 C++ 코드 순서(void 검사가 먼저)로 바로잡은 뒤 잡았다. 표 항목 변이는 생성기로 파일을 다시 만들어 원복했다. 소스는 모두 원복했다.

## 3. 이번에 하지 않은 것 (경계)

- **연결 지점의 구현체**(표시·건물 옆면·미니맵 큐·파생 firstPop/postPop) — 해당 효과를 가진 섬·다리·건물의 Pop 은 구현체가 연결돼야 하며 지금은 건물 옆면이 없으면 거부된다. 연결 지점의 호출 순서는 cpppj 구현의 계약이고 원본 행으로 증명된 것이 아니다.
- 파생 Unpop·삭제·의존 객체·수명(W03), `PostPop`·`assetpostpop`, 소유자(`owner`)·프레임(`setframe`), 건물 공간(`buildingspace`)·이웃·탐색, Graph 계열, CD 10.72. `ValidateFrameBinding`·`ValidateGraph` 같은 상위 묶음 검사는 옮기지 않았다.
- **Unpop 쪽의 표시 지원 무조건 분기**(원본 `SquidUnpop::SupportsDisplay` 의 0x50C3B0·0x509810 true)는 Unpop C# port 에 아직 없다. Pop 은 원본 `SquidPop::Supports` 그대로를 따르므로 이 두 주소는 true 로 반영했다. Unpop 쪽을 고치는 일은 W02 완료 뒤에 따로 판단한다.

## 4. 다음 조각

`PostPop`·`assetpostpop`(등록 효과, `IRawPopPostPop` 을 구현)·소유자(`owner`)·프레임(`setframe`) → 건물 공간(`buildingspace`, `IRawPopBuildingSide`·`IRawBuildingSide` 를 구현)·이웃·탐색 → Graph 계열. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
