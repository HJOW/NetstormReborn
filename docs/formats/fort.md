# 요새 / 맵 파일 (`.fort`)

> 분석 상태: **컨테이너·오브젝트 레코드 해독 완료** — 원본 `.fort` 463개(느슨한 파일 431 + 아카이브 32) 전부 섹션 끝까지 정확히 해석됨
> 도구: [`tools/fort.py`](../../tools/fort.py) (`dump`: 파일 하나 요약, `verify`: 전체 검증)
> 근거: `Netstorm.exe` 의 `Template.cpp`(요새 = "Template" 클래스), `Datamanager.cpp`(섹션 컨테이너), `Rifttype.cpp`(타입 플래그)

## 위치와 용도

* 느슨한 파일 `originals/d/*.fort` (대부분 팬 제작 미션용), TAFF 아카이브 `\d\*.fort` (공식 미션, XOR 복호화 후 같은 형식)
* 미션 스크립트와 같은 이름으로 짝을 이룬다 (`tutorial1.english` ↔ `tutorial1.fort`)
* **커스텀 맵**(AGENTS.md 2026-10-03): `.fort`는 게임 안 Edit 메뉴로 생성·수정하고, 짝이 되는 `.english`는 사용자가 텍스트 편집기로 직접 작성한다. 사용자가 만든 예는 `originals/d/TEST01.fort` + `TEST01.english`다 → [커스텀 맵 만들기](mission-script.md#커스텀-맵-만들기-agentsmd-2026-10-03-test01-예)
* 파일 경로는 설정으로 정해진다 (`setup.cfg`): `fortSpec`/`SOLOSpec = "{DataDir}\{local.1}.fort"`,
  `battleFortSpec = "{InstallDir}{DataDir}\{local.1}.player{local.2}"`, `archiveFortSpec = "...archive{local.2}"`
* 로드 함수 `FUN_004bf550(경로)`, 저장 함수 `FUN_004bfba0`

## 1. 파일 구조

모든 정수는 리틀 엔디언.

```
[0x46]            매직 ('F')  — 다르면 파일을 버린다
[0x00 | 0xFE]     플래그 (0xFE 이면 별도 플래그 설정, 의미 미확정. 원본 6개 파일)
[섹션] × N        길이 접두 섹션
```

### 섹션

| 크기 | 내용 |
|---|---|
| 2 | 길이 L (u16, **길이 필드 자신 포함**) |
| L-2 | 섹션 데이터 |

섹션은 **이름 순서로** 저장되며, 이름 목록은 실행 파일(0x5146B0)에 고정되어 있다. 로더는 목록의 35개만 읽고 나머지는 무시한다
(원본 97개 파일이 36개 이상 섹션을 가짐 — 팬 도구가 붙인 것으로 추정).

| 번호 | 이름 | 내용 |
|---|---|---|
| 0 | `Subscriber` | 소유자 ID u32 + 문자열(u16 길이 + 글자) — 요새 이름 (예: "Unnamed") |
| 1 | `State` | 고정 28바이트 상태 구조체 (필드 의미 미확정) |
| 2 | `Mission` | 미션 상태 (대부분 비어 있음) |
| 3 | `CoreData` | 핵심 데이터 (대부분 비어 있음, 미해석) |
| 4 | `Chaff` | **월드 전체 청크 오브젝트** (아래 4절) |
| 5 | `Badges` | 배지 (미해석) |
| 6 | `Territory` | 영역 표 120바이트 (6바이트 × 20), 모양·방향·생성 플래그·청크 위치 해석 완료 ([분석](../exe/territory-layout.md)) |
| 7 | `TypeNames` | 타입 번호 변환표 (아래 2절) |
| 8 | `CompressedData` | (대부분 비어 있음, 미해석) |
| 9 | `Technology` | 타입별 저장 상태 목록 (아래 3절) |
| 10 | `Money` | f32 — **Storm Power** (게임 내 재화) |
| 11 | `Deck` | 항목 수 u8 + 타입·확률 가중치·위력·남은 횟수 4바이트 항목 (아래 3절) |
| 12~14 | `Reserved2`~`Reserved4` | 예약 |
| 15~34 | `Terr00`~`Terr19` | **영역별 청크 오브젝트** (아래 4절) |

## 2. `TypeNames` — 타입 번호 변환표

```
[개수 N u8] [해시 i32] × N
```

* 파일 안의 타입 번호 i 는 저장 당시 실행 파일의 i 번째 타입이다. 해시로 현재 타입을 찾아 번호를 바꾼다 (`FUN_004bd800`).
* 해시: 이름의 각 글자(부호 있는 8비트)를 `(i % 4) * 8` 비트 왼쪽으로 밀어 모두 더한다.
  (2026-10-05 정정) 20바이트로 자르지 않는다. 타입 구조체의 이름 필드를 C 문자열로 읽기 때문에, 내장 프로세스 타입은 20글자로 잘려 들어가 있고, 이름이 20글자 이상인 `.type` 타입(`fakeThreeByThreeSurface`)은 뒤의 설명 필드와 이어져 `fakeThreeByThreeSurffake3x3Surface`로 읽힌다 — [cpp-fort-reconstruction.md](../exe/cpp-fort-reconstruction.md).
* 현재 실행 파일의 타입 번호 체계 (N = 188):

| 번호 | 내용 |
|---|---|
| 0 | 없음 |
| 1~9 | 내장 타입: 1 `DependForm`, 2 `ProcessForm`, 3 `GumpForm`, 4 `PlayerSquid`, 6 `ContentForm`. 5·7·8·9 는 이름 없음 (2026-10-05 확인) |
| 10~62 | 내장 프로세스 타입 53개 (`evilRaySystemProcessType` … `thunderAviaryProcessType`, 실행 파일 0x540FA0) |
| 63~69 | 쓰지 않음 (이름 없음) |
| 70~185 | `.type` 타입 116개 = **70 + 로딩 순서** ([shp.md](shp.md) `TYPE_LOAD_ORDER`) |
| 186, 187 | 쓰지 않음 (이름 없음) |

CD판(10.72)은 N = 171: 프로세스 타입 52개(10~61), `.type` 타입 101개(70~170).
`originals/d` 의 낱개 파일 대부분(415개)은 이 CD판 번호 체계로 저장되어 있다.

* 섹션이 비어 있으면 변환 없이 현재 번호를 그대로 쓴다.

## 3. `Deck` / `Technology` — 덱과 타입 상태

### `Deck`

```text
[항목 수 N u8] { [타입 번호 u8] [chance u8] [power i8] [numRemaining u8] } × N
```

* `Template.cpp`의 저장 `004befd0`과 읽기 `004bf190`이 항목 네 필드를 이 순서로 처리한다.
  `Deck.cpp`의 `0044f160`은 `power`를 부호 있는 8비트로 검사하고,
  `0044f510`은 양수인 `power`의 `chance`를 합산해 가중 추첨에 사용한다.
* 타입 번호는 `TypeNames` 변환표로 현재 `.type` 정의에 연결한다. 내장 타입이나
  변환할 수 없는 번호는 원본 번호를 보존하고 해석된 타입을 비워 둔다.
* 원본 463개 파일의 `Deck` 길이가 모두 `1 + 4 × N`과 일치한다. 1개 파일은 N=0이며,
  나머지에서 N은 28·37·38·43·46·48이다. 원본 항목의 `power`는 모두 1,
  `numRemaining`은 모두 255다. 이 값들이 일반 게임에서 어떤 범위까지 변하는지는 미검증이다.
* Save the Island!의 첫 항목 `47 04 01 FF`는 `sunArcher`, chance 4, power 1,
  numRemaining 255다. The War Begins!는 같은 타입의 chance가 3이다.

`FortFile.Deck`에서 저장 순서와 원본 타입 번호·해석된 타입·네 필드를 확인할 수 있다.

게임에서는 워크샵 우클릭 메뉴로 유닛을 화면 왼쪽 사이드바 "덱"에 등록해야 생산·건설할 수 있다 (사용자 확인).
이 섹션이 그 덱(등록 목록과 등장 확률)을 저장하는 것으로 추정한다 — [workshop-deck.md](../gameplay/workshop-deck.md) 3절.

### `Technology`

```text
[항목 수 N u8] { [타입 번호 u8] [saveQA u8]? [saveQB i16]? [목록 플래그 u8] [내용물 목록]? } × N
```

* `Template.cpp`의 저장 `004bcea0`과 읽기 `004bd130`을 따른다. 타입 번호는 `TypeNames`로
  변환하고, 해당 타입의 `saveQA`·`saveQB`·`container` 플래그에 따라 뒤의 값을 읽는다.
  내용물 목록은 `Chaff`의 중첩 내용물과 같은 형식이다.
* 원본은 타입 구조체의 +0x100 필드와 생성 문맥의 +0xFC 필드가 모두 0일 때 목록 플래그
  1바이트를 저장한다. 보유한 원본 463개 파일의 모든 항목은 이 경로를 사용한다.
  다른 경로의 저장 데이터는 아직 검증하지 않았다.
* 원본 파일의 항목 수는 0·2·3·4·5·26이다. 전체 10,180개 항목에서 목록 플래그는 4,
  `saveQA`·`saveQB` 값은 없으며 `container` 내용물은 비어 있다. 두 파일
  (`capturethepriest.fort`, `tacticalcombat.fort`)에는 항목 뒤에 의미가 확인되지 않은
  0바이트가 하나 더 있어 `FortFile`은 이 경우를 허용하고 원본 섹션 바이트를 보존한다.
* Save the Island!와 The War Begins!는 각각 26개 항목이다. 첫 항목은 `sunArcher`,
  목록 플래그 4이고 마지막 항목은 `rainWalker`다. 원본 전체 `.fort`와 공식 맵 두 개를
  대조한 검사 172개가 통과했다.

`FortFile.Technology`와 `tools/fort.py dump`의 `technology` 항목은 타입 번호와 해석된
타입·목록 플래그·내용물을 원본 순서로 제공한다. Python 도구의 `verify`도 원본 463개를
모두 해석했다.
플레이어의 보유 기술과 이 목록의 관계는 게임 로직 분석 후 확인해야 한다.

## 4. `Chaff` / `TerrNN` — 청크 오브젝트

```
[버전 u8]                 원본: 2 (구버전 1 도 21개 섹션)
[청크 레코드] × 청크 수
```

* 월드는 **16 × 16 청크**, 청크 하나는 16 × 16 칸으로 보인다 (원본 463개 전부 `Chaff` 청크 256개).
* `Chaff`: 모든 청크를 y 바깥, x 안쪽 순서로 저장 (**청크 번호 i → x = i % 16, y = i / 16**, 원본 화면 대조로 확정). 월드 칸 좌표 = 청크 좌표 × 16 + 위치 바이트.
* 화면 배율: 칸 하나 = **가로 16px × 세로 11px**, 미니맵 1px = 2칸. 오브젝트 칸 좌표를 변환한 화면 점에 스프라이트 기준점(0,0)을 그린다 ([save-the-island-start.md](../screens/save-the-island-start.md) 4절)
* `TerrNN`: 월드 청크를 y 바깥·x 안쪽으로 훑으며 그 영역에 속한 청크만 저장 (예: 4청크 영역 = 2×2 블록, 순서 왼위·오른위·왼아래·오른아래). 영역 번호와 플레이어 번호는 무관 (Save the Island! 은 플레이어가 Terr07)
* `TerrNN`: `Territory`의 모양 패턴을 적용하고 빈 칸을 제외한 최종 월드 청크 좌표를 y·x 순으로 정렬한다. 위치 바이트는 **하위=x, 상위=y**이며 미션 저장 좌표의 기준 원점은 (1,1). [영역 배치 분석](../exe/territory-layout.md)과 C# `FortMap` 참고.

### 청크 레코드

```
[0x63 ('c')] [오브젝트 수 u16] [오브젝트 레코드] × 오브젝트 수
```

### 오브젝트 레코드 (`FUN_004bdc60`)

| 순서 | 크기 | 조건 | 내용 |
|---|---|---|---|
| 1 | u8 | 항상 | 청크 안 위치: **상위 4비트 = x, 하위 4비트 = y** (원본 화면 미니맵 대조로 확정) |
| 2 | u8 | 항상 | 타입 번호 t (아래 특수값 참고) |
| 3 | u8 | 플래그1 `saveFrame`(0x20) | 애니메이션 프레임 |
| 4 | u8 | 플래그1 `saveQA`(0x1000) | 추가 값 A |
| 5 | u16 | 플래그1 `saveQB`(0x2000) | 추가 값 B |
| 6 | u8 | 플래그2 `bridge`(0x4) | 각 칸의 다리 클러스터 번호 (배치용 복합 패턴 번호가 아님) |
| 7 | u8 | 버전 > 1 이고 플래그2 `factory`(0x4000) | 작업장 상태 |
| 8 | u8 | 버전 > 0 이고 플래그2 & 0x5D77CF00 | 소유 플레이어 (1~8) |
| 9 | 가변 | 플래그1 `container`(0x20000) | 내용물 목록 (아래) |

특수 타입 번호:

| t | 처리 |
|---|---|
| 0 | 뒤의 1바이트(0 이어야 함)만 읽고 끝 |
| 0xF6 ~ 0xFF | **다리 조각 약식 표기**: 타입 = bridge, 소유자 = 0xFF - t, 이후 bridge 규칙대로 읽음 |
| island (94) | 뒤의 1바이트만 건너뜀 |

### 내용물 목록 (`FUN_004bd130`)

```
[개수 u8] { [타입 u8] [saveQA u8]? [saveQB u16]? [중첩 내용물]? } × 개수
```

### 타입 플래그

레코드 해석은 타입의 플래그1(타입 구조체 +0xE8)·플래그2(+0xEC)에 달려 있다.
`.type` 의 `typeflags` 단어가 비트를 켜고(`Rifttype.cpp`), 로드 후 후처리가 파생 비트를 추가로 켠다.

* 주요 단어 → 비트: `saveFrame` F1 0x20, `saveQA` F1 0x1000, `saveQB` F1 0x2000, `container` F1 0x20000, `bridge` F2 0x4,
  `factory` F2 0x4000, `walker` F2 0x10000, `balloon` F2 0x20000, `emplacement` F2 0x40000, `priest` F2 0x200000 등 — 전체 표는 `tools/fort.py` 의 `FLAG1_BITS`, `FLAG2_BITS`
* 파생 규칙 (해석에 영향 있는 것):
  * F2 & 0x34200 (vortex·factory·walker·balloon) → F1 `container` — 그래서 프리스트·신전 등은 내용물 개수 바이트를 가진다
  * F2 `buried`(0x2000) → F1 `saveQA`
  * (2026-10-05 확정) `.type` 의 `group` 속성(이름 표: archer 0, cannon 1, blocker 2, aviary 3, flyer 4, battery 5, fence 6, walker 7, balloon 8, misc 9, 없으면 10)에 따라
    battery → F2 0x800, archer·cannon → F2 0x8000000, blocker → F2 0x4000000. 이 비트들도 소유자를 저장하는 마스크에 들어 있다.

## 5. 원본 통계

* 오브젝트 상위: `noIsland` 175,616 (**가이저를 받치는 떠 있는 작은 바위 3×3칸** — 가이저가 있는 청크마다 9개, 원본 화면에서 확인), `bridge` 90,290, `geyser` 17,090, `priest` 2,038, 각종 포대·발전기·작업장·신전 …
* 36개 이상 섹션: 97개 파일 / 섹션 뒤 남는 바이트: 3개 파일 / 플래그 0xFE: 6개 파일

## 5-1. 2026-10-05 원본 코드 대조로 추가 확인한 것

C++ 로더(`cpppj/src/o/Template.cpp`)를 만들면서 확인했다 — [cpp-fort-reconstruction.md](../exe/cpp-fort-reconstruction.md).

* 타입 바이트는 **먼저 변환표로 바꾼 뒤** 0 인지 본다. 보유한 파일에는 타입 바이트가 0 인 오브젝트가 없다.
* 버전 0 섹션은 모든 타입 뒤에 상태 1바이트가 있다(보유한 파일에는 없는 버전).
* 내용물 항목의 목록 플래그는 "타입 +0x100 → 그릇 타입 +0xFC → 둘 다 0 이면 파일의 1바이트" 순서다. `Technology` 는 그릇이 플레이어라 항목마다 1바이트가 있고, bomb 타입 항목에는 없다.
* `saveQB` 는 부호 있는 16비트다. 섹션 길이도 부호 있는 16비트로 읽는다.
* 저장된 소유자 바이트는 0 이거나 8보다 크면 1로 바꾼다. 실제로 쓰는지는 실행 상태에 달려 있다.

## 6. 남은 일

* `State`, `Badges`, `CoreData`, `Mission`, `CompressedData` 섹션 내부 구조, `Territory` 외관·나머지 플래그
* 회전된 영역 파일의 실제 동작 및 일반 전투의 섬 재배치 (보유 원본 활성 영역은 모두 방향 0)
* 섬 지형(영역의 섬 모양)의 생성 방식 — 지형은 오브젝트로 저장되지 않음 (`Territory` 값으로 생성 추정)
