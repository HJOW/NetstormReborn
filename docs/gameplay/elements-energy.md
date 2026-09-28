# 원소·원소 에너지·발전기

> 분석 상태: **사용자 설명(2026-09-28) + `.type` 데이터 대조**. exe 의 에너지 판정 함수는 미확인.
> 에너지 규칙 질문 A~F 는 모두 사용자 답변으로 확정됨 (5절). 공급 범위는 일반 30칸·튜토리얼 2 14칸 — 측정과 exe(전투 옵션 Generator Range) 모두 확인 (6절). 남은 것은 에너지 판정 함수 검증.

## 1. 원소 (사용자 확인)

* 원소는 **Rain · Wind · Thunder** 세 가지다. `.type` 의 `theme` 값 `rain`·`wind`·`thunder`.
* **Sun 은 "공용" 원소**다 (`theme = sun`). 신전·발전기가 없고, Sun 에너지는 다른 세 원소 중 어느 것의 에너지로도 대신 공급할 수 있다 (3절).
* `fireVortex`(Fire Temple, `theme = Fire`) 는 로딩 목록 밖의 미사용 타입이다.

## 2. 워크샵과 덱 등록 (사용자 확인)

* 원소마다 워크샵이 있다: Sun Workshop(`sunFactory`), Rain Workshop(`rainFactory`), Wind Workshop(`windFactory`), Thunder Workshop(`thunderFactory`).
* 워크샵에서는 **그 원소의 유닛만** 사이드바 덱에 등록할 수 있다 ([workshop-deck.md](workshop-deck.md)).
* **예외: Sun Workshop** 은 Sun 원소 발전기가 없으므로 **다른 원소들의 Generator 도 등록할 수 있다**.
  The War Begins! 의 Sun Workshop 목록이 Rain Generator(비 원소 발전기) · Sun Cannon · Whirlibase 였던 이유다.

## 3. 유닛과 원소 에너지 (사용자 확인)

* 이 게임에서는 **템플(신전)·워크샵·알타(Altar)를 제외한 작은 건물도 "유닛"** 이라 부른다 (포대·방벽·탑·발전기 등). 이동 유닛(골렘·게·비행선 등)도 유닛이다.
* 건물형 유닛을 지을 수 있는 곳 ([island-ownership.md](island-ownership.md) 규칙 6):
  1. 내 섬 (내 템플이 있는 섬)
  2. 내 섬과 **다리로 연결된 빈 섬**(아무도 소유하지 않은 섬)
  3. **내 다리의 끝**
  * 남의 섬(다른 플레이어 템플이 있는 섬)에는 일체 지을 수 없다.
* 유닛을 건설·생산할 때는 **Storm Power(돈)** 와 함께 그 유닛이 요구하는 **원소 에너지**가, 짓는 **위치에 공급되고 있어야** 한다.
  * 예: **Ice Cannon** 은 **Rain 1 + Sun 1 = 에너지 2개**가 필요하다.
  * 예: **Sun Cannon** 은 **Sun 1** 이 필요하다. Sun 은 다른 원소로 대신할 수 있으므로 **Rain Generator 하나만 근처에 있어도** 지을 수 있다 (Wind·Thunder 발전기여도 된다).
  * 예: **Vander Tower**(Thunder) 는 **Thunder 2 + Sun 1** 이 필요하다 → 짓는 위치를 **Thunder 공급원 2개 + 아무 원소 공급원 1개, 모두 서로 다른 3개**가 함께 덮어야 한다.
* **충족 판정 (사용자 확인, 2026-09-28)**:
  * 필요한 에너지 1개마다 **서로 다른 공급원(Generator 또는 Temple) 1개**가 필요하다. 공급원 하나는 에너지 1개로만 센다.
  * 그 공급원들의 범위가 **짓는 위치에서 모두 겹쳐야** 한다 (교집합 다이어그램처럼).
  * 원소 에너지는 같은 원소 공급원만 채울 수 있고, **Sun 에너지는 아무 원소 공급원**으로 채운다.
  * 공급원은 **자기 또는 동맹 소유**면 된다.
  * **Temple 은 Generator 를 대신할 수 있다** (같은 원소 공급원 1개로 센다).
  * **소모 개념이 아니다**: 같은 공급원 범위 안에 유닛을 여러 개 지을 수 있다.
  * 조건은 **건설(생산) 순간에만** 본다. 나중에 공급원이 파괴돼도 지은 유닛은 제 역할을 한다.
  * 이동 유닛도 **생산하려는 지점**에 에너지가 공급돼야 한다.
  * 판정 예 (Vander Tower): Thunder Temple + Thunder Generator + Rain Generator 가 모두 덮으면 가능. Thunder Generator 1 + Rain Generator 2 는 Thunder 가 1개뿐이라 불가.
* 에너지 공급원은 **Temple 과 Generator** 다.
  * Generator 는 **꽤 넓은 범위** 안에 **자기 원소 에너지 1개분**을 공급한다.
  * Temple 은 원소별로 1종씩(Rain·Wind·Thunder Temple) 있고, **Generator 와 똑같이 자기 원소 에너지 1개분을 같은 범위에** 공급한다 (사용자 확인).
  * 템플이나 Generator 를 선택하면 **그 원소 모양 아이콘**(비 = 물방울, 바람 = 조개껍데기 모양 등)이 둘레를 돌며 공급 범위를 보여 준다. **노란 별은 공급 범위가 아니라 건물형 유닛의 공격 범위** 표시다 (사용자 확인).
  * **공급 범위 = 반지름 30칸 원 (칸 좌표 거리² ≤ 900)**. 일반 미션에서는 템플·Generator 모두 같다. 튜토리얼 2(Secret Workshop)만 14칸으로 축소된다. 반지름은 전투 옵션 Generator Range 값이다 (6절).

## 4. 유닛별 필요 에너지

**규칙 (사용자 확인, 2026-09-28)**: 필요 에너지 개수 = `.type` 의 `level`, 구성 = **`자기 원소 × (level − 1) + Sun × 1`**.
Sun 은 아무 원소 공급원으로 채우므로, level 1 유닛(Generator 포함)은 원소와 관계없이 **아무 공급원 1개**, Sun 원소 유닛은 **아무 공급원 level 개**가 필요하다.
사용자가 확인한 예: Sun Cannon(Sun 1), Ice Cannon(Rain 1 + Sun 1), Vander Tower(Thunder 2 + Sun 1), Generator(아무 1), Balloon·Whirlibase(아무 2).

아래 표는 이 규칙을 `.type` 데이터에 적용한 것이다. 필요 공급원 = "원소 공급원 개수 + 아무 공급원 개수".

| 원소 | 유닛 (타입) | level | 비용 | 필요 공급원 |
|---|---|---|---|---|
| sun | Sun Disc Thrower `sunArcher` | 1 | 300 | 아무 1 |
| sun | Stone Tower `sunBlocker` | 1 | 400 | 아무 1 |
| sun | **Sun Cannon** `suncannon` | 1 | 400 | 아무 1 |
| sun | Sun Barricade `sunFence` | 1 | 300 | 아무 1 |
| sun | Balloon `sunBalloon` | 2 | 600 | 아무 2 |
| sun | Whirlibase `sunaviary` | 2 | (cost 속성 없음) | 아무 2 |
| rain | **Rain Generator** `rainBattery` | 1 | 400 | 아무 1 |
| rain | Crystal Crab `rainwalker` | 1 | 500 | 아무 1 |
| rain | **Ice Cannon** `raincannon` | 2 | 600 | Rain 1 + 아무 1 |
| rain | Acid Barricade `rainFence` | 2 | 400 | Rain 1 + 아무 1 |
| rain | Ice Tower `rainBlocker` | 2 | 800 | Rain 1 + 아무 1 |
| rain | Man o'War Pool `rainaviary` | 3 | 600 | Rain 2 + 아무 1 |
| rain | Cloud Floater `rainBalloon` | 3 | 1000 | Rain 2 + 아무 1 |
| wind | **Wind Generator** `windBattery` | 1 | 400 | 아무 1 |
| wind | Sail Skater `windwalker` | 2 | 600 | Wind 1 + 아무 1 |
| wind | Crossbow `windArcher` | 2 | 550 | Wind 1 + 아무 1 |
| wind | Wind Tower `windBlocker` | 2 | 800 | Wind 1 + 아무 1 |
| wind | Devil Maker `windaviary` | 3 | 800 | Wind 2 + 아무 1 |
| wind | Air Ship `windBalloon` | 3 | 1200 | Wind 2 + 아무 1 |
| thunder | **Thunder Generator** `thunderBattery` | 1 | 400 | 아무 1 |
| thunder | Bulf `bulf` | 1 | 500 | 아무 1 |
| thunder | Arc Spire `thunderFence` | 1 | 400 | 아무 1 |
| thunder | Bulwark `thunderBlocker` | 2 | 800 | Thunder 1 + 아무 1 |
| thunder | Thunder Cannon `thundercannon` | 2 | 1200 | Thunder 1 + 아무 1 |
| thunder | **Vander Tower** `thunderArcher` | 3 | 600 | Thunder 2 + 아무 1 |

* 템플(5000)·워크샵(800~1000)·알타·가이저(2000)는 유닛이 아니며 level 이 없다.
* 이동 유닛(Crab·Skater·Bulf·Balloon·Floater·Air Ship 등)도 **생산하려는 지점**에 같은 에너지가 필요하다 (사용자 확인).
* 사용자가 직접 확인한 5종 외에는 규칙을 적용한 결과이므로, exe 의 판정 함수로 전 유닛을 검증한다.
* 발전기 3종: `class = Source of Energy`, **`minUsage = maxUsage = -100`** (음수 = 공급으로 보임).
* 포대·방벽: `minUsage 20` / `maxUsage 200`(포대) 또는 `20`(방벽). 건설 판정은 공급원 개수·범위 겹침(3절)이고 소모 개념이 없으므로, 이 값은 건설 조건이 아닌 다른 용도일 수 있다. exe 확인 필요.
* `techBit` 은 기술(지식) 번호로 보인다 (미션 헤더 `myTech` 와의 관계는 미확인).

### 원판 매뉴얼과의 차이 (2026-09-29, [sources/game-manual.md](../sources/game-manual.md) 7절)

* 원판 매뉴얼(`GAME.HLP`)의 유닛별 "Energy to Build" 는 위 규칙과 대부분 같다 (Sun 유닛 = Sun × level, Generator = Sun 1, level 2 원소 유닛 = 원소 1 + Sun 1, level 3 = 원소 2 + Sun 1).
* **다른 점: level 1 원소 유닛** — 매뉴얼은 Bulf = **Thunder 1**, Sail Skater = Wind 1, Acid Barricade = Rain 1 로 자기 원소를 요구한다 (매뉴얼 규칙: 원소 = max(1, level − 1), Sun = 나머지, Generator 는 예외로 Sun).
  현재 규칙대로면 level 1 은 아무 공급원 1개다. 패치판에서 level 1 인 원소 유닛(Bulf·Arc Spire·Crystal Crab)에 무엇이 필요한지 **사용자·exe 확인 필요**.
  * exe 조사 진행 상황 (2026-09-29, 중단): `Rifttype.cpp` 로더가 `theme` 을 타입 구조체 **+0x98**(첫 글자로 표 `0x592990` 에서 번호 변환), `level` 을 **+0x94 에 level − 1** 로 저장한다.
    +0x94·+0x98 을 함께 읽는 함수는 `0044ac30`, `00473d00`, `0049b0d0`(타입 플래그 파생), `004cb320`·`004cb3f0`(`Ui.cpp` — 원소·레벨로 유닛 목록을 세는 메뉴용) 다섯 개다.
    에너지 판정 후보는 아직 읽지 않은 **`0044ac30`·`00473d00`** 이며, 다음에 이 둘부터 확인한다.
* 레벨·비용 자체도 원판과 패치판이 다르다 (예: Sail Skater 원판 level 1 → 패치 level 2, Acid Barricade 1 → 2, Arc Spire 2 → 1, Crystal Crab 2 → 1). 구현은 패치판 `.type` 을 따른다.

## 5. 질문과 답 (사용자 확인, 2026-09-28)

* **A. 섬 소유권 규칙과의 관계** → 배치 불가는 **남의 섬**(다른 플레이어 템플이 있는 섬). 빈 섬은 내 섬과 다리로 연결되면 건물형 유닛 건설 가능, 워크샵·알타는 사제가 도달하기만 하면 가능 — [island-ownership.md](island-ownership.md). 다리가 끊겨도 이미 지은 유닛은 계속 동작한다.
* **B. 신전의 공급량·범위** → Generator 와 같이 자기 원소 1개분, 같은 범위.
* **C. 필요 에너지 구성** → `자기 원소 × (level − 1) + Sun × 1` (4절). Generator 도 아무 공급원 1개, Sun level 2 유닛은 아무 공급원 2개가 필요하다.
* **D. 공급 1개분의 의미** → 필요 에너지 1개 = 서로 다른 공급원 1개, 범위가 모두 겹쳐야 함. **소모 개념이 아니다** — 같은 공급원 범위 안에 유닛을 여러 개 지을 수 있다.
* **E. 조건 확인 시점** → **건설(생산) 순간에만** 필요하다. 공급원이 나중에 파괴돼도 지은 유닛은 제 역할을 한다.
* **F. 이동 유닛** → 이동 유닛도 **생산하려는 지점**에 에너지 공급이 필요하다.

## 6. 공급 범위 측정 (스크린샷·영상, 2026-09-28)

선택한 템플·Generator 둘레를 도는 원소 아이콘의 위치를 캡처에서 찾아, 칸 좌표(가로 16px, 세로 11px)의 원으로 맞췄다.
모든 장면에서 아이콘이 한 원 위에 놓인다 → **범위는 칸 좌표 기준 원(화면에서는 가로:세로 = 16:11 타원)** 이다.

| 자료 | 대상 | 아이콘 | 개수 | 반지름 | 반지름² | 잔차(칸) |
|---|---|---|---|---|---|---|
| `Normal Mission - Temple - Generating Range.png` (The War Begins!) | Rain Temple | 물방울 | 5 | **30.0칸** | 899.5 | ≤ 0.06 |
| `Normal Mission - Generator - Generating Range.png` (The War Begins!) | Rain Generator (배치 중, 400) | 물방울 | 6 | **30.0칸** | 900.3 | ≤ 0.07 |
| Early Missions 영상 606초 (튜토리얼, 우클릭 메뉴 열림) | Wind Temple | 조개껍데기 | 4 | **30.0칸** | ≈ 902 | ≤ 0.01 |
| `Tutorial - Temple - Generating Range.png` (튜토리얼 2 Secret Workshop) | Wind Temple (배치 중, 5000) | 조개껍데기 | 7 | **약 14칸** (exe: 14) | ≈ 198 | ≤ 0.23 |

* **일반 공급 범위 = 30칸 (거리² ≤ 900)**. 사용자 설명대로 일반 미션에서는 템플·Generator 가 모두 같다.
* **튜토리얼 2 만 축소**: Secret Workshop(튜토리얼 2) 의 Wind Temple 은 약 14칸 (exe 로 14칸 확정). 다른 튜토리얼은 일반 30칸이다 (Early Missions 영상 606초의 튜토리얼 장면도 30칸).
* **exe 확인 (2026-09-29, [battle-options.md](../exe/battle-options.md))**: 반지름은 전투 옵션 **Generator Range** 로 정해진다.
  범위 표 `{14, 22, 30, 38}`칸(Short·Normal·Long·Very Long)에서 고른 뒤 **14~30칸으로 제한**하고 제곱값으로 비교한다.
  기본값은 Very Long(38) → 상한에 걸려 **30칸** = 일반 미션 측정값. **튜토리얼 2 처리 함수만** 첫 단계에서 Short(**14칸**)로 바꾼다 → 튜토리얼 2 측정값(약 14칸)의 정확한 값은 **14칸**.
  (도움말 원판은 short/normal/long 3단계라고 설명하며, 패치판 exe 는 4단계 이름과 30칸 상한을 가진다.)
* 범위 표시는 `rangeDisplayProcessType` 프로세스로 보인다 (exe 추적 필요).

### 노란 별 = 공격 범위 (사용자 확인)

| 자료 | 대상 | 별 개수 | 반지름 | 반지름² |
|---|---|---|---|---|
| Dissolved Alliance 영상 900초 | 다리 끝 작은 섬의 선택된 유닛 ("800" 표시) | 5 | 17.32칸 | ≈ 300 |

* 대상 유닛과 `.type` 의 `range` 값(예: Crossbow 16, Vander Tower 15, Sun Cannon 20)의 관계는 미확인. 공격 범위 규칙은 전투 분석(4단계)에서 다룬다.

재현 방법: 아이콘 근처에서 하늘 배경과 가장 다른 픽셀 무리의 무게중심을 구하고,
`u = x/16`, `v = y/11` 로 바꿔 원 `(u−cu)² + (v−cv)² = R²` 을 최소제곱으로 맞춘다.
