# 원소·원소 에너지·발전기

> 분석 상태: **사용자 설명(2026-09-28) + `.type` 데이터 대조**. exe 의 에너지 판정 함수는 미확인.
> "확인 필요" 표시는 사용자 설명만으로 정해지지 않은 부분이다 (5절).

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
* 에너지 공급원은 **Temple 과 Generator** 다.
  * Generator 는 **꽤 넓은 범위** 안에 **자기 원소 에너지 1개분**을 공급한다.
  * Temple 은 원소별로 1종씩(Rain·Wind·Thunder Temple) 있고, **Generator 와 똑같이 자기 원소 에너지 1개분을 같은 범위에** 공급한다 (사용자 확인).
  * 템플이나 Generator 를 **우클릭하면 노란 별 표시가 빙글빙글 돌며 공급 범위를 보여 준다** (사용자 확인). 범위 측정은 6절.

## 4. `.type` 데이터 대조

`level` 이 **필요한 에너지 개수**와 맞아 보인다 (Sun Cannon 1, Ice Cannon 2 — 사용자 설명과 일치). 확인 필요 C.

| 원소 | level 1 | level 2 | level 3 |
|---|---|---|---|
| sun | Sun Disc Thrower `sunArcher` 300, Stone Tower `sunBlocker` 400, **Sun Cannon** `suncannon` 400, Sun Barricade `sunFence` 300 | Balloon `sunBalloon` 600, Whirlibase `sunaviary` (cost 속성 없음) | |
| rain | **Rain Generator** `rainBattery` 400, Crystal Crab `rainwalker` 500 | **Ice Cannon** `raincannon` 600, Acid Barricade `rainFence` 400, Ice Tower `rainBlocker` 800 | Man o'War Pool `rainaviary` 600, Cloud Floater `rainBalloon` 1000 |
| wind | **Wind Generator** `windBattery` 400 | Sail Skater `windwalker` 600, Crossbow `windArcher` 550, Wind Tower `windBlocker` 800 | Devil Maker `windaviary` 800, Air Ship `windBalloon` 1200 |
| thunder | **Thunder Generator** `thunderBattery` 400, Bulf `bulf` 500, Arc Spire `thunderFence` 400 | Bulwark `thunderBlocker` 800, Thunder Cannon `thundercannon` 1200 | Vander Tower `thunderArcher` 600 |

(숫자는 `cost` = Storm Power. 워크샵 800~1000, 신전 5000, 가이저 2000 은 level 없음)

* 발전기 3종: `class = Source of Energy`, **`minUsage = maxUsage = -100`** (음수 = 공급으로 보임).
* 포대·방벽: `minUsage 20` / `maxUsage 200`(포대) 또는 `20`(방벽). 에너지를 **양(量)으로 소비**하는 모델일 가능성이 있다 → 확인 필요 D.
* `techBit` 은 기술(지식) 번호로 보인다 (미션 헤더 `myTech` 와의 관계는 미확인).

## 5. 확인 필요 (사용자에게 질문, 2026-09-28)

* ~~**A. 섬 소유권 규칙과의 관계**~~ → 해결 (사용자 확인, 2026-09-28): 배치 불가는 **남의 섬**(다른 플레이어 템플이 있는 섬). 빈 섬은 내 섬과 다리로 연결되면 건물형 유닛 건설 가능, 워크샵·알타는 사제가 도달하기만 하면 가능 — [island-ownership.md](island-ownership.md).
  다리가 끊겨도 이미 지은 유닛은 계속 동작한다 (사용자 확인).
* ~~**B. 신전의 공급량·범위**~~ → 해결 (사용자 확인): Generator 와 같이 자기 원소 1개분, 같은 범위.
* **C. 필요 에너지 구성**: `level` = 필요 에너지 개수로 보면, level 3 (예: Vander Tower) 은 Thunder 몇 개 + Sun 몇 개인가? "자기 원소 1 + Sun (level−1)" 규칙인가? 발전기(level 1) 자체도 짓는 데 에너지 1개가 필요한가?
* **D. 공급 1개분의 의미**: 발전기 하나의 범위 안에 유닛 여러 개를 지을 수 있는가(범위 안이면 모두 충족), 아니면 발전기 하나가 유닛 하나분만 채우는가?
  또 Ice Cannon(Rain+Sun) 은 **Rain Generator 하나로 충족**되는가, **발전기 2개**(예: Rain + 아무 원소)가 필요한가?
* **E. 조건 확인 시점**: 에너지는 건설 순간에만 필요한가, 이후 발전기가 파괴되면 유닛이 멈추는가?
* **F. 이동 유닛**: 골렘·게·비행선 등도 같은 규칙으로 "짓는 위치"에 에너지가 필요한가?

## 6. 공급 범위 측정 (영상, 2026-09-28)

템플·Generator 를 우클릭(선택)하면 노란 별들이 대상 둘레를 돌며 범위를 보여 준다. 별 위치를 영상 프레임에서 찾아
칸 좌표(가로 16px, 세로 11px)의 원으로 맞췄다. 두 장면 모두 별이 하나의 원 위에 오차 0.01칸 수준으로 정확히 놓인다
→ **범위는 칸 좌표 기준 원(화면에서는 가로:세로 = 16:11 타원)** 이다.

| 장면 | 선택 대상 | 별 개수 | 반지름 | 반지름² |
|---|---|---|---|---|
| Early Missions (튜토리얼) 606초 | **Wind Temple** (우클릭 메뉴 열림) | 4 | **30.0칸** | ≈ 900 |
| Dissolved Alliance 900초 | 다리 끝 작은 섬의 유닛 (흰 사각 테두리, "800" 표시, 원소 아이콘으로 보이는 표식 3개) | 5 | **17.32칸** | ≈ 300 |

* 사용자 설명(2026-09-28): **Early Missions(튜토리얼)에서는 신전의 에너지 공급 범위가 대폭 축소된 경우가 있었다.**
  → 튜토리얼 606초의 30칸이 일반 범위인지 축소된 범위인지는 아직 모른다. 일반 캠페인에서 템플·Generator 를 우클릭한 장면을 찾아 같은 방법으로 재야 한다.
* Dissolved Alliance 의 17.32칸 원은 템플·Generator 가 아니라 선택된 유닛 둘레라서, 공급 범위가 아니라 그 유닛에 관한 다른 범위(공격 범위 등)일 수 있다. 대상 유닛 확인 필요.
* 튜토리얼 미션 머리 값에만 있는 키: `aiNoTemple`, `aiOff`, `denyAscend`, `denySalvage`, `myProd`, `techAllowed`, `tutorialNumber` — 범위를 직접 정하는 키는 없다.
  `options.cfg`·`setup.cfg` 에도 범위 키가 없다. `.type` 의 `rangeConst`·`rangeVar` 는 로더가 assert 로 막는 옛 속성이다.
* exe: 별 표시는 `rangeDisplayProcessType` 프로세스로 보인다. 반지름을 어디서 가져오는지(고정값·타입별·미션별)는 프로세스 타입 번호 표를 풀어 추적해야 한다.

재현 방법: 프레임 추출(`tools/videoframes.py frame`) 후 별 주변에서 가장 밝은 점의 무게중심을 구하고,
`u = x/16`, `v = y/11` 로 바꿔 원 `(u−cu)² + (v−cv)² = R²` 을 최소제곱으로 맞춘다.
