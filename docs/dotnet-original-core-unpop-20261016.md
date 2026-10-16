# 원본 월드 코어 W02 둘째 조각: 공간 해제(Unpop)와 공간 해시

2026-10-16, 작업 PC **DESKTOP-HJOW**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W02(원본 월드 코어 ① 생성·등록)** 의 둘째 조각이다. [첫 조각](dotnet-original-core-20261016.md)(슬롯 풀·생성)에 이어 `dotnetpj/src/Netstorm.Core/Original/`에 공간 해제와 4단계 공간 해시를 옮겼다. 이 PC는 마지막 전체 정밀 디컴파일 PC(VM-W11-Codex, 2026-10-16)와 달라 **디컴파일 산출물을 읽지 않았다.** 근거는 커밋된 cpppj 소스·보고서([cpp-unpop-reconstruction.md](exe/cpp-unpop-reconstruction.md))와 원본 기계어 fixture다. 새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없고 `cpppj/`·공통 `LEFT_JOBS.md`는 수정하지 않았다. 이 조각은 **아직 세션에 연결하지 않았다**(L1 모듈 단계). 게임 플레이는 달라지지 않는다.

## 1. 구성

| 파일 | 내용 | 원본 / cpppj 근거 |
| --- | --- | --- |
| `RawSquidHash.cs` | 4단계 공간 해시. 단계 0~3의 한 버킷이 월드 1·2·4·16칸을 맡고 배열 한 변은 256·128·64·16. `Bucket(단계, x, y)`는 머리를 **참조**로 돌려줘 대입하면 바뀐다. 좌표는 0 방향 절삭 후 단계 크기로 나눈다. 유한하지 않거나 범위 밖이면 예외 | `004b28f0`/`004b2a90`, `SquidHash.cpp` |
| `RawUnpop.cs` | 공통 Unpop(`004afe50`)의 일반 자산·매몰 경로. `IRawUnpop`을 구현해 `RawSquidFactory`의 non-void Take가 쓴다. 연결 지점 3개(`IRawUnpopDisplay`·`IRawBuildingSide`·`IRawMinimapQueue`) | `004afe50`, `SquidUnpop.cpp` |
| `OriginalUnpopDisplay.cs` + `.Data.cs` | 공통 표시 경로(update88/update8c)를 쓰는 가상 표 83개(10.78)와 지원 비트. `.Data.cs`는 [`gen_original_unpop_vtables.py`](../dotnetpj/tools/gen_original_unpop_vtables.py)가 cpppj 목록에서 만든다 | `UnpopVtables.inc`, `004ad500`/`004ad670` |
| `OriginalTypeRecord.cs` (확장) | `FootX`·`FootY`(발자국, 기본 1)와 `Surface`·`BuildingMask`·`SpotWriteMask` 상수 | `RiftType.h` |
| `RawSquidFactory.cs` (확장) | `IRawUnpop.Pool`을 더해 팩토리가 **같은 풀**의 해제 모듈만 받는다(다른 풀이면 생성자가 거부) | `SquidFactory.cpp` |

## 2. 동작 순서

활성(free·void·contained 아님, dead는 무관) 오브젝트를 풀 때의 순서는 원본과 같다.

1. **변경 전 검사:** 번호 0·자산 타입(70) 밖·contained는 거부. free/void면 아무것도 바꾸지 않고 반환. 건물군인데 옆면 몸체가 없거나, 가상 표가 지원 표에 없거나, 좌표가 유한하지 않거나, 발자국이 잘못됐거나, 체인이 손상됐으면(순환·자기 next·범위 밖 next·제거 뒤 자기를 가리키는 next) **풀·점유 지도·해시 머리를 한 바이트도 바꾸기 전에** 예외로 거부한다. 10.78은 체인에서 못 찾으면 원본이 assert로 멈추므로 같은 조건을 예외로 둔다(원본의 부분 변경과 오류 창은 재현하지 않는다).
2. 표시 검증(연결된 경우) → **void 비트 설정** → 섬·다리(플래그2 & 6)이고 매몰이 아니면 미니맵 큐에 이전 위치 통지.
3. 점유 해제: 발자국 칸을 위쪽 행부터 왼쪽 칸 순서로 `spot &= ~genus`(genus는 `SpotRules.EffectiveGenus`의 하위 바이트, 다른 비트 보존). 건물군은 칸마다 점유를 지운 **다음** 맨 위·맨 아래 행을 뺀 왼쪽·오른쪽 열에서 옆면을 푼다.
4. 체인 제거: 단계 0→3 순으로 찾은 체인에서 머리 또는 앞 슬롯의 next를 바꾼다. 제거 대상의 next·좌표·저장 단계·HP·섬 번호는 보존한다.
5. 표시 갱신(연결된 경우).

CD 10.72는 옮기지 않았다(기준 판본 10.78). CD는 체인 미발견을 조용히 넘기고 슬롯 배치가 달라 필요해지면 따로 낸다.

## 3. 검증 (원본 fixture의 10.78 행)

| 대상 | fixture | 일치한 것 |
| --- | --- | --- |
| `RawUnpop`·Take·반납·`FirstPopFlags` | `unpop-x86.tsv` 10.78 세션 9개·호출 **1,809**(Unpop 768·Take 384·FirstPop 448·Release 192·Create 17) | 반환 값, 풀 카운터·머리/꼬리, **풀 전체 바이트·삭제 기록·네 단계 해시 86,272머리·65,536칸 점유 지도의 Adler-32**, 대상 슬롯 50바이트 전부 |
| 표시 지원 표 | `unpop-vtables.tsv` 10.78 **83**행 | 가상 표별 일반/소용돌이 지원 비트(소용돌이 후보 주소 4개 포함), 표 밖 값은 미지원 |

첫 조각이 미뤘던 **`FirstPopFlags`의 원본 대조**(448회)와 **non-void Take의 공간 해제**(Take 384회)도 이 조각에서 원본 행으로 확인됐다.

[`RawUnpopTests`](../dotnetpj/tests/Netstorm.Core.Tests/RawUnpopTests.cs) 9개는 보호 경로(원본 행이 닿지 않는 거부 경로)와 연결 지점 호출 순서, 해시 주소 계산을 확인한다. **연결 지점의 호출 순서는 cpppj 구현의 계약**이며 해당 효과를 가진 모듈이 아직 없어 원본 행으로 증명된 것이 아니다.

**변이 확인:** 구현 15곳(void 비트 생략, 점유를 0 대입·OR로 바꿈, 체인 단계 0~2만 검색, 머리를 교체 대신 지움, 앞 슬롯 next를 엉뚱한 슬롯에 씀, 매몰 비트 위치, dead 슬롯 건너뜀, 미니맵 항상 통지, 표시 지원 검사 생략, 소용돌이 비트 반전, 해시 3단계 배율, 해시 x/y 순서, 발자국 왼쪽 계산, 옆면 맨 위 행 포함)을 하나씩 망가뜨리자 모두 하나 이상의 검사가 실패했다. 처음에는 "미니맵 항상 통지" 하나를 놓쳐서 섬·다리가 아닌 건물과 매몰 오브젝트에서 통지가 **없음**을 확인하는 검사를 더한 뒤 잡았다. 원복한 뒤 전체 통과를 다시 확인했다.

## 4. 이번 조각에서 하지 않은 것 (경계)

- **연결 지점의 구현체.** 표시(`IRawUnpopDisplay`)·건물 옆면(`IRawBuildingSide`)·미니맵 큐(`IRawMinimapQueue`)는 인터페이스만 있다. 이 효과를 가진 오브젝트(섬·다리·건물)의 Unpop은 구현체가 연결돼야 한다. 건물 몸체가 없으면 거부한다.
- **파생 Unpop·삭제.** 파생 클래스의 해제 효과, 의존 객체, 참조/수신 수명, 신전 삭제는 W03(삭제·수명)에서 옮긴다.
- **체인 삽입(Pop).** 이 조각은 제거만 한다. 삽입·공간 해시 조회·단계 선택은 다음 조각(Pop)이다.
- **세션 연결.** `BattleMap`·`BridgeGrid`는 여전히 기존 `SidPool`·`SpotMap`을 쓴다.

## 5. 다음 조각

`Pop`/공간 해시 삽입·spot 등록(`pop`·`spatial`, 기존 `SpotMap`·`SpotRules` 재사용 검토) → `PostPop`·`assetpostpop` → 소유자(`owner`)·프레임(`setframe`) → 건물 공간·이웃·탐색 → Graph 계열. 건물 옆면 몸체(`buildingspace`)는 Pop과 같은 몸체를 쓰므로 Pop 뒤에 만든다. 시작할 때 `python dotnetpj/tools/cpp_sync.py`를 먼저 실행하고, 쓸 fixture를 `--update --add`로 등록한다.
