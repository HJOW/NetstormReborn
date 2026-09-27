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

* 이름: 영문자 1개 이상 + 숫자 (`N00`, `L01`, `AA01`). 첫 글자가 같은 클러스터끼리 한 동작(애니메이션 시퀀스)을 이루는 것으로 보인다.
  예) altar: `P` = 빈 상태, `O` = 몸부림, `N` = 붕괴, `M` = 투명, `L` = 희생
* 클러스터 플래그 (실행 파일에서 확인): `default`(기본 프레임), `help`(도움말 그림), `baseframe`, `gumpframe`,
  `fringe`, `suck`, `rim`, `lit`, `unlit`, `cracked`, `hard` (지형 타일 속성)
* 그림 참조는 레이어 순서(0 = 본체, 1 = 그림자). **런타임에는 GIF 이름을 쓰지 않고** 클러스터 순번으로 `_shapes.shp` 프레임을 찾는다.

## `typeflags` (빈도순)

| 플래그 | 개수 | 추정 의미 |
|---|---|---|
| `shadow` | 37 | 그림자 레이어 있음 |
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
| `flyershadow` | 6 | 비행 유닛 그림자 |
| `factory`, `vortex`, `altar`, `dais`, `residence`, `geyser`, `edgefarm`, `fence`, `fencePost`, `bridge`, `nugget`, `mog`, `tree`, `buried`, `container` | | 오브젝트 종류 |
| `surface`, `island`, `islandThreeByThree`, `rim`, `fringe` | | 지형 |
| `saveFrame`, `randframe`, `matchframe`, `predictable`, `opaquecollide`, `saveQa` | | 프레임·동기화 관련 |

## 속성 (빈도순, 47종)

| 키 | 개수 | 의미 (추정 포함) |
|---|---|---|
| `zorder` | 116 | 그리기 레이어 (`"zoEMPLACEMENTS"`, `"zoISLAND"`, `"zoFLARES"` …) |
| `foot_x`, `foot_y` | 103 | 차지하는 칸 수 (가로×세로) |
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
| `hotFootRatioX`, `hotFootRatioY` | 28 | 기준점 위치 비율 |
| `constructionRate` | 26 | 건설 속도 |
| `techBit` | 23 | 기술 비트 번호 |
| `casttime`, `praytime`, `manacost` | 19 | 주문 시전 시간 / 기도 시간 / 마나 비용 |
| `speed`, `turningSpeed` | 16 / 13 | 이동·회전 속도 |
| `minUsage`, `maxUsage` | 12 | 에너지 사용량 범위 |
| `orientations` | 10 | 방향 수 |
| `hpPerSec` | 10 | 초당 피해량 |
| `height` | 9 | 높이(비행 고도 등) |
| `spawns`, `crew`, `mana`, `effecttime`, `delayBetweenShots`, `airdamage`, `useairdamage`, `artifactFrame`, `helpText` | 소수 | |

설계자 주석에 밸런스 조정 이력이 남아 있다 (예: sunCannon `cost` 200→400, `range` 축소, `hpPerSec` 조정).
