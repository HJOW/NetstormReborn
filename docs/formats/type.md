# 오브젝트 정의 스크립트 (`.type`)

> 분석 상태: **문법 확인 완료**, 각 속성·플래그의 게임 내 의미는 일부 추정 (4단계에서 확정)
> 위치: TAFF 아카이브 `\d\*.type` (119개) — [taff.md](taff.md)
> 도구: [`tools/typefile.py`](../../tools/typefile.py) (`json`: 전체 해석 결과, `table`: [수치표](../gameplay/types.md) 생성)

## 개요

건물·유닛·주문·지형·효과 등 게임 오브젝트 하나를 정의하는 텍스트 스크립트 (Windows-1252, CRLF).
실행 파일(`Rifttype.cpp`)이 고정된 순서의 116개 타입을 읽는다 ([shp.md](shp.md) "블록 ↔ 타입 대응").
아카이브에 있지만 로딩 목록에 없는 타입 3개(`fireVortex`, `fireWalker`, `efarm` 등 구버전/미사용)도 있다.

## 문법

```
typename <이름> [constructor | client constructor]
typeflags <플래그> <플래그> ... ;
{
    키 = 값;          // 값: 정수, 실수, "문자열"
    ...
}

// 클러스터(애니메이션 프레임) 정의
<클러스터이름> : <클러스터 플래그들> : "<그림>.gif" #<번호> [: "<그림>.gif" #<번호>] ;
```

* `//` 줄 주석. 키 이름의 대소문자가 파일마다 섞여 있다(`hotFootRatioX` / `hotfootratiox`) → **대소문자 무시**로 처리.
* `typename` 과 `typeflags` 가 한 줄에 붙어 있는 경우도 있다.

### 클러스터

* 이름: 영문자 1~2개 + 숫자 (`N00`, `L01`, `AA01`).
  예) altar: `P` = 빈 상태, `O` = 몸부림, `N` = 붕괴, `M` = 투명, `L` = 희생
* 클러스터 플래그 (실행 파일에서 확인): `default`(기본 프레임), `help`(도움말 그림), `baseframe`, `gumpframe`,
  `fringe`, `suck`, `rim`, `lit`, `unlit`, `cracked`, `hard` (지형 타일 속성). 그 밖의 단어(`dirt`, `solid`)는 원본이 무시한다.
* 그림 참조는 레이어 순서(0 = 본체, 1 = 그림자). **런타임에는 GIF 이름을 쓰지 않고** 클러스터 순번으로 `_shapes.shp` 프레임을 찾는다.
  셰이프 블록은 레이어별로 클러스터 전체를 차례로 담는다 (프레임 = 레이어 × 클러스터 수 + 클러스터 번호).

### 원본 프레임 코드 표 (2026-09-28, `Rifttype.cpp` 로더 `0049d0xx` 후반부 확인)

로더는 클러스터 줄마다 4바이트 코드를 만들어 타입 구조체(500바이트)의 `+0x124` 배열에 저장하고, 개수를 `+0x114` 에 둔다.

| 바이트 | 내용 | 원본 규칙 |
|---|---|---|
| 0 | 측면(assert 이름 `sideOrientation`) | 이름 첫 글자. `'A'~'Z'` 가 아니면 assert |
| 1 | 변형 | 이름 둘째 글자. 한 글자 이름(`N00`)이면 `'P'` |
| 2 | 번호 | 글자 뒤 숫자 토큰을 실수로 읽어 `_ftol`(`004e49c0`) 로 정수화, char 로 저장 (원본 데이터의 최댓값 99) |
| 3 | 플래그 비트 | `fringe` 0x01, `suck` 0x02, `rim` 0x04, `lit` 0x08, `unlit` 0x10, `cracked` 0x20, `hard` 0x40 (대소문자 무시) |

특수 프레임 번호 (초기값은 타입 표 생성 함수 `0049ebb0` 에서 확인):

| 오프셋 | 의미 | 규칙 | 초기값 |
|---|---|---|---|
| `+0x118` | 기본 프레임 | `default` 가 나올 때마다 덮어씀 → **마지막 default** | 0 |
| `+0x11c` | gump 프레임 | `gumpframe` | 0 |
| `+0x120` | 도움말 프레임 | `help` 가 있으면 마지막 help, 없으면 마지막 default | -1 |
| `+0x128` | base 프레임 | `baseframe` | -1 |

로딩 목록의 116개 타입에서는 default 가 둘 이상인 타입이 없어 "첫 default" 와 결과가 같다 (로딩 목록 밖의 `Bird` 만 28개).

프레임 검색 함수 (모두 앞에서부터 순차 검색, 첫 일치):

| 함수 | 조건 | 실패 시 |
|---|---|---|
| `0049a940` | 측면·변형·번호 일치 | assert 후 -1 |
| `0049a9a0` | 측면·변형·번호 일치 | -1 |
| `0049a9e0` | 측면·변형·번호 일치 + (플래그 & 마스크) ≠ 0 | -1 |
| `0049aa30` | 측면·변형 일치 + 플래그가 정확히 같음 | assert 후 -1 |
| `0049aa90` | 측면·변형(+플래그 조건) 후보를 모아 난수 또는 구간으로 하나 선택. 없으면 변환 표(`0041cd20`) → 변형 `'A'` → 측면만 순으로 대체 | -1 |

다리 코드에서는 번호 +10 을 금 간(cracked) 프레임으로 찾는 사용 예가 있다 (`00421b60`, `00421bd0`).
따라서 **한 동작 = 측면·변형이 같은 클러스터 묶음**이며, `dude` 는 측면 A~H(8방향) × 걷기 8프레임이다.
동작별 재생 속도·틱은 아직 확인하지 않았다 (4단계 게임 틱 분석).
C# 구현: `TypeFrameTable` (`TypeDefinition.Frames`), 테스트 `TypeFrameTableTests`.

## `typeflags` (빈도순)

| 플래그 | 개수 | 추정 의미 |
|---|---|---|
| `shadow` | 37 | 둘째 레이어를 고르게 어두운 그림자로 그린다 |
| `dontSave` | 31 | 저장하지 않는 임시 오브젝트 |
| `default_hotspot` | 29 | 기본 기준점 사용 |
| `mayDropOnRim` / `mayDropOnIsle` | 26 / 5 | 섬 가장자리 / 작은 섬에 배치 가능 |
| `createsisland` | 23 | 설치 시 섬 조각을 만든다 (포대류) |
| `not_real` / `notreal` | 22 / 3 | 실체 없는 오브젝트(효과 등) |
| `emplacement` | 21 | 고정 포대/건물 |
| `bomb` | 19 | 주문(폭탄) |
| `dropBlocking`, `walkBlocking`, `shotblocking`, `yuckWalk` | 17 / 1 / 13 / 14 | 배치·이동·사격 차단 |
| `not_selectable` | 11 | 선택 불가 |
| `walker`, `flyer`, `balloon`, `ship`, `guy`, `priest` | | 이동 유닛 종류 |
| `flyershadow` | 6 | 둘째 레이어를 체크무늬(한 픽셀 건너) 그림자로 그린다 (풍선·비행체) |
| `factory`, `vortex`, `altar`, `dais`, `residence`, `geyser`, `edgefarm`, `fence`, `fencePost`, `bridge`, `nugget`, `mog`, `tree`, `buried`, `container` | | 오브젝트 종류 |
| `surface`, `island`, `islandThreeByThree`, `rim`, `fringe` | | 지형 |
| `saveFrame`, `randframe`, `matchframe`, `predictable`, `opaquecollide`, `saveQa` | | 프레임·동기화 관련 |

## 속성 (빈도순, 47종)

| 키 | 개수 | 의미 (추정 포함) |
|---|---|---|
| `zorder` | 116 | 그리기 레이어 (`"zoEMPLACEMENTS"`, `"zoISLAND"`, `"zoFLARES"` …) |
| `foot_x`, `foot_y` | 103 | 차지하는 칸 수 (가로×세로). 원본 `Rifttype.cpp FUN_0049b0d0`은 로딩 때 `foot_y=6`을 **8로 보정**한다. Sun Workshop 원문 7×6 → 실제 7×8. 클론 `Footprint.TypeHeight`에 반영([근거](../gameplay/test02-clone-parity-20261004.md)) |
| `description` | 92 | 표시 이름 (영어, 번역 대상) |
| `class` | 70 | 분류 (`Shooter`, `Defense`, `Production`, `Energy`, `Ground Transport`, `Aerial Attack`, `Offensive Spell` …) |
| `cost` | 62 | 비용 (Storm Power) |
| `level` | 51 | 기술 레벨 |
| `maxHitPoints` | 48 | 최대 체력 |
| `theme` | 48 | 원소 (`sun`, `rain`, `wind`, `thunder`) |
| `threat` | 38 | AI 위협도 |
| `range` | 37 | 사거리 |
| `damageEffect` | 34 | 피해 연출 |
| `activeSound`, `fireSound`, `pickupSound`, `moveSound`, `dropSound`, `buildDoneSound`, `impactSound` | | 효과음 파일 이름 |
| `group` | 28 | 그룹 (`"cannon"` 등) |
| `hotFootRatioX`, `hotFootRatioY` | 28 | 그림 기준점 이동 비율. 원본은 칸 기준점에서 **비율 × 한 칸 크기(16 × 11)** 만큼 왼쪽·위에 그린다(소수점 버림). 주석의 `hotFootX = 1`·`hotFootY = 8` ↔ 비율 0.0625·0.5 처럼 16분의 n 단위다. 2026-10-03 녹화에서 네 타입으로 확인 — [근거](../videos/test01-visuals-20261003.md) 4절 |
| `constructionRate` | 26 | 건설 속도 |
| `techBit` | 23 | 기술 비트 번호 |
| `casttime`, `praytime`, `manacost` | 19 | 주문 시전 시간 / 기도 시간 / 마나 비용 |
| `speed`, `turningSpeed` | 16 / 13 | 이동·회전 속도 |
| `minUsage`, `maxUsage` | 12 | 에너지 사용량 범위 |
| `orientations` | 10 | 방향 수 |
| `hpPerSec` | 10 | 초당 피해량 |
| `height` | 9 | 그림의 키(칸). 들고 있는 유닛의 발자국이 커서 칸보다 `1 + height` 행 아래에 놓인다(Thunder Cannon 2 → 3행 아래). 사제·골렘 1, 방벽 2~3 — [근거](../videos/test01-visuals-20261003.md) 9절. 다른 쓰임(비행 고도 등)은 미확인 |
| `spawns`, `crew`, `mana`, `effecttime`, `delayBetweenShots`, `airdamage`, `useairdamage`, `artifactFrame`, `helpText` | 소수 | `mana`는 타입의 건설 에너지 요구 문자열을 직접 지정한다 ([exe 분석](../exe/energy-requirements.md)) |

설계자 주석에 밸런스 조정 이력이 남아 있다 (예: sunCannon `cost` 200→400, `range` 축소, `hpPerSec` 조정).
