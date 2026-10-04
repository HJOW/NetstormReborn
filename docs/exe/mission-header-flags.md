# 미션 머리 값 `techAllowed`·`denySalvage`·`denyAscend` — 시작 값과 튜토리얼 단계 처리

> 분석일: 2026-09-30. 기준: 보유 `originals/Netstorm.exe`(10.78)의 Ghidra 정적 디컴파일(`extracted/decomp/Netstorm.c`)과
> 그 디컴파일에 없는 함수를 따로 디컴파일한 결과(7절), capstone 어셈블리 대조, 튜토리얼 2 사용자 직접 조작 관찰(2026-09-30).
> 함수 이름은 Ghidra 자동 이름(`FUN_주소`)이며, 파일 이름(`Totalmade.cpp` 등)은 exe 안 assert 문자열에서 복원한 것이다.

## 1. 의문

튜토리얼 2(`tutorial2.english`)의 머리에는 다음 값이 있다.

```
denySalvage = 1
techAllowed = "deny;all;allow;windVortex;sunArcher"
```

그런데 사용자가 원본을 직접 조작한 결과(2026-09-30)는 머리 값과 어긋나 보인다.

| 머리 값이 말하는 것 | 실제로 관찰한 것 |
|---|---|
| `techAllowed`에 `sunFactory`가 없다 | Wind Temple 건설 뒤 **Sun Workshop을 지을 수 있었다** |
| `denySalvage = 1` (회수 금지) | 튜토리얼 마지막에 **유닛 하나를 회수(Salvage)할 수 있었고** 75 SP(원가 300의 25%)를 받았다 |

## 2. 결론

**머리 값은 미션의 "시작 상태"일 뿐이고, 튜토리얼은 진행 단계마다 이 상태를 실행 중에 바꾼다.**
원본은 튜토리얼 번호마다 단계 처리 함수(튜토리얼 2 = `FUN_004c3bb0`)를 매 프레임 호출하며, 이 함수가 다음을 한다.

- **단계 B**: 기술 허용 표에서 `sunFactory`(런타임 타입 번호 85)를 허용으로 바꾼다 (`FUN_004c23e0(DAT_005411ac, 1)`).
  스크립트 `[B.]`("Temple Complete") ~ `[B1.]`("Create a Workshop … Build Sun Workshop")가 뜨는 바로 그 단계다.
- **단계 H**: 회수 금지 전역 변수를 0으로 바꾼다 (`DAT_00595078 = 0`). 스크립트 `[H1.]`("Try it now! Right-click … Salvage")가 뜨는 단계다.

따라서 머리 값과 관찰은 **모순이 아니다**. 머리 값만 읽어 규칙을 "항상 강제"하면 오히려 원본과 달라진다.
(이전 구현 메모에서 `techAllowed`를 "만들 수 있는 기술"로만 적어 두었던 해석은 시작 상태라는 점을 빠뜨렸다.)

## 3. 근거

### 3.1 머리 값을 읽는 코드

머리 값은 `FUN_00482d50(키, 번호)`(`;` 목록의 번호째 토큰)와 `FUN_00482ce0(키, 기본값)`(값 하나)으로 읽는다.
`denySalvage`·`denyAscend`·`myStartMoney`·`myTech` 문자열은 exe에서 모두 `push 주소` 형태(예: `push 0x50da74`)로만 쓰여
간접 참조로 보이지만, 바이트 검색으로 참조 위치를 찾을 수 있다. **읽는 함수 세 개는 Ghidra 전체 디컴파일에 없다**(7절).

| 주소 | 하는 일 | 읽는 키 |
|---|---|---|
| `0x4c3290` `FUN_004c3290` | 미션 로드 때 Totalmade 객체를 초기화하는 함수로 보인다. `loadFort`로 맵을 정한 **다음** 마지막에 `FUN_00482eb0`(techAllowed 해석)을 부른다 | `tutorialNumber`(→ `DAT_005ca8e4`, 0이 아니면 `DAT_005ca8f0 = 1`), `missionNumber`, `loadFort` |
| `0x482eb0` `FUN_00482eb0` (Mission.cpp) | 기술 허용 표를 머리 값으로 채운다 (3.2) | `techAllowed` |
| `0x4c2b20` `FUN_004c2b20` (Totalmade 계열) | 시작 조건 읽기 | `denySalvage`(→ `DAT_00595078`), `denyAscend`(→ `DAT_0059507c`), `allowAnyCapture`(→ `DAT_005453e4`), `moreGeysers`(→ `DAT_005950ac`, 값이 0이면 1), `demoRestartTimer`, `tellAtList`, `myStartMoney`(→ 플레이어 Storm Power), `myTech`(→ `FUN_004915c0`, 시작 지식), `randGeysers`, 플레이어 1~8의 `ai%s…` 설정(`FUN_004c2d40`) |
| `0x484ab0` `FUN_00484ab0` | `Normal` 미션 객체(생성 `0x484e60`, [battle-options.md](battle-options.md) 4절)의 초기화로 보인다. `tech` 키(같은 deny/allow/all 규칙)와 `denySalvage`·`denyAscend`·`deadline`·`buildCriteria`·`researchCriteria`·`moneyCriteria`·`initialMoney`·`successDelay`·`stormGeysers`를 읽는다 | 위 목록 |

* 원본 미션 스크립트(아카이브 + `originals/d`) 중 `tech =` 키를 쓰는 것은 **없다**. `techAllowed`는 6개(튜토리얼 1~6), `denySalvage = 1`은 튜토리얼 1·2·3, `denyAscend = 1`은 6개다.
  튜토리얼은 `FUN_00482eb0`(`techAllowed`) 쪽만 쓴다. `FUN_00484ab0`을 어떤 미션이 쓰는지는 확인하지 못했다.
* 전역 변수 초기화·해제: `FUN_004c3210`(Totalmade 소멸)은 `FUN_004c23c0(1)`(전부 허용), `DAT_00595078 = 0`, `DAT_0059507c = 0`, 단계 변수 0으로 되돌린다.
  `FUN_004c2990`(Totalmade 생성)은 단계 글자 `DAT_005ca8e8`을 `0x41`('A')로 시작한다.

### 3.2 기술 허용 표 (`techAllowed`)

* 표는 `DAT_005ca0d0`부터의 255칸이다. **값 1 = 금지, 0 = 허용**이다.
  * `FUN_004c23c0(모드)` (Totalmade.cpp): 표 전체를 `(모드 == 0)`으로 다시 채운다.
  * `FUN_004c23e0(타입, 모드)`: 한 칸을 `(모드 == 0)`으로 바꾼다.
  * `FUN_004c2400(타입)`: `표[타입] == 0`(허용 여부)을 돌려준다. 타입 번호가 1 이상 `numTypes`(`DAT_00541348`) 미만이 아니면 assert.
* `FUN_00482eb0`은 `;` 토큰을 차례로 읽는다. `deny`/`allow`는 모드(처음 허용)를 바꾸고, `all`은 `FUN_004c23c0(모드)`, 그 밖의 이름은 `FUN_0049a860(이름)`으로 타입 번호를 찾아 `FUN_004c23e0(번호, 모드)`를 부른다.
  → `"deny;all;allow;windVortex;sunArcher"` = 전부 금지 후 windVortex·sunArcher만 허용. (기존 `TechPermissions.Parse`와 같은 순서.)
* **표를 확인하는 곳은 메뉴와 덱이다.** 명령을 실행하는 경로에서 다시 확인하는 곳은 찾지 못했다.

  | 함수 | 하는 일 |
  |---|---|
  | `FUN_00461cf0` (`0x461cf0`) | 사제의 Construct 메뉴 항목 "Build %s%s"("Build Level %d …")를 만든다. **허용되지 않은 타입이면 항목을 만들지 않고 돌아간다** |
  | `FUN_004752b0` (`0x4752b0`) | 워크샵의 "Current Production"·생산 목록 메뉴. 허용된 타입만 항목으로 넣는다 |
  | 생산 창 항목 처리 (Combatgump, 디컴파일 44,527행 부근) | 항목의 타입이 허용이면 다음 처리(수량·시간 비교)를 하고, **허용이 아니면 그 항목 객체의 가상 함수(+0x10)를 호출한다**(생산 창에서 지우는 것으로 추정, 세부 미확인) |
  | 덱 재구성 (Deck.cpp, `FUN_0044f8xx`, 디컴파일 55,075행~) | 덱을 비우고 내 오브젝트를 훑는다. 템플이면 **`FUN_004c2400(sunWalker)`가 허용일 때만** 골렘 항목을 넣는다 → 튜토리얼에서 골렘이 없는 이유 |

### 3.3 회수 금지 (`denySalvage`)

`DAT_00595078`을 읽는 곳은 두 군데다.

1. **Salvage 메뉴 항목**(`FUN_0044ca00`, `0x44ca00`): 내 오브젝트의 우클릭 메뉴에 "Salvage gains N" / "Salvage costs N" 항목을 만든다.
   `DAT_00595078 != 0`이면 항목은 그대로 나오되 **명령이 `"DenySalvage"`로 바뀐다**(정상은 회수 명령, 템플은 `SalvageVortex`, 워크샵은 `SalvageInf`).
2. **Salvage 실행**(`FUN_0044c420`, `0x44c4e9`): 내 오브젝트이고 `DAT_00595078 != 0`이며 타입이 `DAT_005412c0`(= 154, **nugget**)이 아니면
   회수를 하지 않고 `FUN_004cf960("DenySalvage")`(`0x507b94`의 문자열)로 끝난다. 이 함수는 미션 스크립트의 **섹션 이름을 알리는(Tell) 함수**다
   (`FUN_00482de0`도 같은 함수를 부른다). 정상 경로는 `FUN_004437c0(오브젝트, 0x200000)`으로 회수한다.
   * 원본 미션 스크립트에는 `[DenySalvage]` 섹션이 **하나도 없다**(아카이브 + `originals/d` 전수 검색). 그래서 튜토리얼 1·3에서 회수를 시도하면 화면에는 아무 안내도 뜨지 않고 회수만 안 되는 것으로 보인다(없는 섹션을 Tell했을 때의 동작은 확인하지 못했다).

`DAT_00595078`에 쓰는 곳: `FUN_004c2b20`·`FUN_00484ab0`(머리 값), `FUN_004c3210`·`0x484628`(초기화 0), **`FUN_004c3bb0` 단계 H(0)**.
튜토리얼 3·4·5·6 처리 함수에는 쓰는 곳이 없다 → 튜토리얼 1·3은 머리 값 1이 끝까지 유지되고, 회수를 가르치는 것은 튜토리얼 2뿐이다.

### 3.4 승천 금지 (`denyAscend`)

`DAT_0059507c`는 게임 메뉴 막대를 만드는 함수(`FUN_004cbb80`, 디컴파일 135,179행~)에서 읽는다.
켜져 있으면 전투 중 **"Leave Battle Honorably" / "Leave Battle In Shame"** 항목 묶음(`LeaveNormal`·`LeaveEarly`)을 건너뛴다.
튜토리얼 6개가 모두 1로 시작하고 이 값을 바꾸는 코드는 없다.

### 3.5 튜토리얼 2 단계 처리 (`FUN_004c3bb0`)

* 단계 글자 `DAT_005ca8e8`은 `'A'`(0x41)부터 시작하고 `FUN_004c33f0`이 한 글자씩 올린다(`A`→`B`→…). 단계마다 스크립트 `[A.]`, `[B.]` … 섹션을 Tell한다(`FUN_004c2a90`이 `"글자."` 이름을 만든다).
* 튜토리얼 번호(`DAT_005ca8e4`)로 함수를 고른다: 1 `004c3a20`, **2 `004c3bb0`**, 3 `004c3f00`, 4 `004c40f0`, 5 `004c42b0`, 6 `004c45e0` ([battle-options.md](battle-options.md)).
* 타입 번호는 70 + 로딩 순서다: `DAT_005411ac` = 85 = **sunFactory**, `DAT_00541174` = 71 = **sunArcher**, `DAT_0054126c` = 133 = sunWalker, `DAT_005412c0` = 154 = nugget.
* "개수"는 `FUN_004c24c0(타입)`(= `DAT_005c98d0[타입]`), 플래그별 합은 `FUN_004c2500(플래그)`가 구한다(vortex = `0x200`, factory = `0x4000`).
  * **세 개수 표의 의미 (2026-09-30 확인, `Totalmade.cpp` 증감 함수와 호출자)**: `FUN_004c2450`이 세 표를 0으로 지운다.

    | 표 | 읽기 | 올리는 곳 | 내리는 곳 | 의미 |
    |---|---|---|---|---|
    | `DAT_005c94d0` | `FUN_004c2480` | `FUN_004c25e0`(출생 콜백 `FUN_004b0d30`, **내 플레이어** 것만) | `FUN_004c25c0`(소멸 `FUN_004b0950`·전체 삭제 `FUN_004c27c0`) | **현재 개수**(내 것) |
    | `DAT_005c98d0` | `FUN_004c24c0`·`FUN_004c2500` | `FUN_004c25e0`(같은 곳) | `FUN_004c25d0` — **`FUN_004c27c0`(전체 삭제)에서만** | **지은 누적 수**(내 것). 파괴·회수는 줄이지 않는다 |
    | `DAT_005c9cd0` | `FUN_004c2560` | `FUN_004c25b0`(출생 콜백, 모든 소유자) | `FUN_004c25a0`(소멸) | 현재 개수(전체) |

    튜토리얼 단계 A·B·D·E·G의 조건(`FUN_004c2500`·`FUN_004c24c0`)은 **누적 수**를 본다. 단계 G의 `> 3`은 회수·파괴와 상관없이 네 번째로 놓는 순간이다.
  * 출생 콜백(`FUN_004b0d30`)이 건설 시작 때인지 완공 때인지는 코드로 확정하지 못했다. 단계 B의 안내 제목이 "Temple Complete"이고 본문이 "템플이 완공되자 섬 가장자리 색이 바뀐 것을 보셨을 겁니다"인 점,
    단계 C 안내가 "워크샵을 우클릭하라"인 점으로 **완공 시점**으로 본다 (클론 구현도 완공에 센다).

| 단계 | 스크립트 | 매 프레임 하는 일 | 다음 단계로 가는 조건 |
|---|---|---|---|
| A | `[A.]` "Buildings", "Create a Temple" | 전투 옵션을 덮어쓴다: `DAT_0052f7e4 = 0`(Generator Range = Short), `DAT_0052f7e2 = 2`(Unit Rate = Fast), `FUN_004b4860`(반지름 재계산) ([battle-options.md](battle-options.md)) | vortex(템플)를 지은 개수 > 0 |
| **B** | `[B.]` "Temple Complete", `[B1.]` "Create a Workshop" | **`FUN_004c23e0(sunFactory, 허용)`** — 표에서 Sun Workshop을 허용으로 바꾼다 | factory(워크샵) 개수 > 0 |
| C | `[C.]` "Production" | 선택한 오브젝트(`DAT_005caea0`)가 템플(0x200)이면 타이머(+2.0초)를 걸고, 지나면 `[NotVortex]` 보정 안내를 띄운 뒤 선택을 푼다(`FUN_004d5c90`·`FUN_004d5c60`) | 워크샵에 sunArcher를 등록(`FUN_0044f970`: 내 생산 창 목록에 타입이 있는지) |
| D | `[D.]`, `[D1.]` | — | sunArcher 개수 > 0 |
| E | `[E.]` | — | sunArcher 개수 > 1 |
| F | `[F.]`, `[F1.]` | 타이머가 없을 때 템플을 선택하면 타이머(+4.0초)를 시작한다 | 타이머가 지남 (선택을 푼다) |
| G | `[G.]` | — | sunArcher 개수 > 3 ("두 개 더") |
| **H** | `[H.]`, `[H1.]` "Salvaging" | **`DAT_00595078 = 0`** — 회수 금지를 푼다(프레임마다) | 처리 중인 명령이 회수 명령(`DAT_0054286c`)이면 타이머(+2.0초)를 걸고, 지나면 다음 단계 |
| I | `[I.]` "Mission Accomplished!" | — | 결과 창 |

* **타이머 상수(2026-09-30, exe 바이트)**: `0x506588` = **4.0**초(단계 F), `0x506590` = **2.0**초(단계 C의 `NotVortex`·단계 H), `0x500460` = 0.0("타이머 없음" 표시), `0x506580` = 60.0. 현재 시각 `DAT_0055b4d0`은 게임 시각(초, double)이다.
* **단계 넘김과 잠금(`FUN_004c33f0`·프레임 함수 `FUN_004c34c0`)**: 단계를 넘기면 글자를 올리고 타이머(`+0x98`)를 0으로 지우며 카운터 `+0x84`를 10으로 둔다. 카운터가 0이 되기 전에는 단계 처리 함수를 부르지 않고(`+0x84 == 0`일 때만 호출),
  카운터가 다 세어지면 `FUN_004c2a90`이 그 단계의 스크립트 섹션을 알린다. 모달 창이 떠 있으면(`FUN_00460de0`) 단계를 넘기지 않는다. 클론은 창이 없어 "다음 틱부터 검사"로 근사한다.
* 단계 처리는 세션 명령(선택·등록·배치·회수)과 이벤트만 보고 상태를 바꾼다: 클론의 [`TutorialStages`](../../dotnetpj/src/Netstorm.Core/Simulation/TutorialStages.cs) — [core-rules.md](../core-rules.md).

튜토리얼 1의 처리 함수(`FUN_004c3a20`)는 기술 허용 표나 `DAT_00595078`을 바꾸지 않는다. 튜토리얼 1은 머리 값(`windVortex`만 허용, 회수 금지)이 미션 내내 유지된다.

## 4. 관찰과의 대조

사용자가 직접 확인한 두 가지가 exe 규칙과 맞는다.

| 관찰(사용자, 2026-09-30) | exe 규칙 |
|---|---|
| 템플을 지은 **뒤** Sun Workshop을 지을 수 있었다 | 템플 개수 > 0 → 단계 A→B → `FUN_004c23e0(sunFactory, 허용)` → Construct 메뉴에 Sun Workshop 항목이 생긴다 |
| 튜토리얼 마지막에 유닛 하나를 회수(판매)할 수 있었다(75 SP = 원가 300의 25%) | 단계 H가 `DAT_00595078 = 0` → 정상 회수 경로(비용 25%) |

아래는 exe 규칙에서 **유도한 예측**이며 게임에서 확인하지 않았다: 템플 건설 전(단계 A)에는 Construct 메뉴에 Sun Workshop 항목이 없어야 하고,
단계 H 전(D~G)에 유닛을 회수하려 하면 회수되지 않아야 한다. 확인하려면 튜토리얼 2를 다시 조작해 봐야 한다.

## 5. 클론 구현 반영

* [`TechPermissions`](../../dotnetpj/src/Netstorm.Core/Rules/MissionStart.cs)를 **변경 가능한 표**로 바꿨다(`Set`·`SetAll` = `FUN_004c23e0`·`FUN_004c23c0`). 머리 값은 시작 상태다.
* [`BattleSession`](../../dotnetpj/src/Netstorm.Core/Simulation/BattleSession.cs)의 `DenySalvage`는 머리 `denySalvage`로 시작하는 **변하는 상태**다.
  켜져 있으면 회수 명령은 `SalvageDenied`로 거부된다(원본의 "DenySalvage 섹션 Tell"에 대응, 화면 안내는 없음).
* 기술 허용 표는 사제 Construct 판정(`CheckBuilding`), 지식 등록(`RegisterKnowledgeCommand`), 덱 배치(`CheckUnit`)에서 확인한다 — 원본이 확인하는 곳(3.2)과 같은 수준이다.
* **튜토리얼 2 단계 처리 구현(2026-09-30)**: [`TutorialStages`](../../dotnetpj/src/Netstorm.Core/Simulation/TutorialStages.cs)가 단계 A~I를 재현한다. 세션 명령·이벤트만 보고 표·회수 금지·전투 옵션을 바꾸고
  단계마다 스크립트 섹션 이름을 `TutorialTell` 이벤트로 알린다(안내 창은 아직 없다). 단계 C·F가 읽는 "선택한 오브젝트"는 `SelectEntityCommand`로 세션 상태가 되었다.
  [TutorialStagesTests](../../dotnetpj/tests/Netstorm.Core.Tests/TutorialStagesTests.cs)가 명령만으로 A→I를 끝까지 걷는다. 이전 테스트([BattleSessionTests](../../dotnetpj/tests/Netstorm.Core.Tests/BattleSessionTests.cs))의 손 재현은 규칙만 따로 확인하려고 남겼다.
  튜토리얼 1은 수집 경제(가이저·사제 결정 운반)가 필요해 아직 구현하지 않았고, 3~6은 전투가 필요하다.
* 맵 뷰어의 배치 시험 모드는 `EnforceProductionRules = false`라 표·회수 금지·덱을 모두 무시한다.

## 6. 한계·미확인

* 표를 **명령 실행 경로**(배치 커서·건설 실행)에서 다시 확인하는지는 못 찾았다. 메뉴에서 항목이 빠지면 사용자가 명령을 낼 수 없으므로 실질적으로 같지만, 네트워크 등 다른 경로의 검증은 알 수 없다.
* `FUN_004cf960`이 없는 섹션 이름을 받았을 때의 동작, 출생 콜백이 건설 시작 때인지 완공 때인지(3.5절 — 완공으로 추정), 단계 넘김 잠금 카운터의 정확한 의미(`+0x80` 지연값, `FUN_004c8e90`·`FUN_00460de0`의 뜻)는 2026-09-30에 확인했다: 잠금 카운터(`+0x84`)는 생성 때 10이며 **다이얼로그 gump가 없는 프레임이 연속 10번**이어야 단계 섹션이 Tell된다, `+0x80`은 단계 진행 뒤 다음 단계까지의 지연(기본 0.5초), `FUN_004c8e90` = 열린 다이얼로그 gump 없음(`DAT_00565dd4 == 0`), `FUN_00460de0` = 게임 시계 정지 중 → [dialog-pause.md](../gameplay/dialog-pause.md).
  (타이머 상수 4.0·2.0초와 `DAT_005c98d0` = 지은 누적 수는 2026-09-30에 확인했다.)
* `FUN_00484ab0`(`tech` 키 파서)을 쓰는 미션 유형: **`Normal` 클래스**의 가상 함수 표(`0x50d948`) 슬롯 4다(2026-09-30 확인). `Normal`은 `mission1~5.german`(옛 독일어 미션)에서만 쓰이므로 영어 미션에는 관계없다.
* `FUN_004c2b20`이 언제·누가 부르는지와 `FUN_004c3290`·`FUN_004c2b20`의 실행 순서: 둘 다 "Tutorial" 미션 클래스 가상 함수 표(`0x5149ec`)의 슬롯이다(`FUN_004c3290` = 슬롯 2 "미션 읽기", `FUN_004c2b20` = 슬롯 4 "시작 조건 읽기", 2026-09-30 확인 — 슬롯 표는 [dialog-pause.md](../gameplay/dialog-pause.md)). 표를 통해 부르는 쪽(`call [reg+8]`·`call [reg+0x10]`)과 실행 순서는 아직 확인하지 못했다. 영어 미션은 모두 이 클래스이고 `Normal` 클래스(옛 독일어 `mission1~5`)는 슬롯 4가 `FUN_00484ab0`(`tech` 키 파서)이다.

## 7. Ghidra 전체 디컴파일에서 빠진 함수 (재현 방법 포함)

`extracted/decomp/Netstorm.c`(함수 4,506개)에는 **위 세 함수(`FUN_00484ab0`, `FUN_004c2b20`, `FUN_004c3290`)가 없다.**
Ghidra 자동 분석이 이 주소를 함수로 인식하지 못했기 때문이다(직접 호출자가 없거나 가상 함수 표로만 호출되는 코드로 보인다).
`grep`으로 `denySalvage` 문자열이 안 나온 것도 이 때문이다.

* **(2026-10-05 갱신) 빠진 함수는 정밀 디컴파일이 한꺼번에 복구한다 — [decompile-reliability.md](decompile-reliability.md).** 아래 "약 528개"는 그 이전의 휴리스틱 추정이며, 실제로는 패치판 1,031개·CD판 2,065개가 복구됐다. 위 세 함수와 `FUN_004b1e80` 도 `extracted/refined/originals/Netstorm.c` 에 들어 있다.
* 빠진 함수는 더 있을 수 있다. INT3(0xCC) 패딩 직후에서 흔한 함수 프롤로그로 시작하지만 디컴파일 목록에 없는 주소가 **약 528개**(그중 진입점 간격이 16바이트 이상인 것 525개)이며 위 세 함수도 여기에 들어간다.
  이 수는 휴리스틱 후보이고 실제 누락 수는 검증하지 않았다. **디컴파일 결과에서 `push 주소`로만 쓰이는 문자열(설정 키 등)이 검색되지 않으면 이런 누락 함수를 의심한다.**
* 누락 함수 찾기: 문자열의 주소를 exe 바이트에서 `push 주소`(`68 xx xx xx xx`)로 검색해 참조 위치를 얻고, 그 앞의 INT3 패딩 다음을 함수 시작으로 잡는다.
* 누락 함수 디컴파일: `tools/ghidra/decompile_at.ps1`(`DecompileAt.java`)이 이미 만든 프로젝트를 **읽기 전용**으로 열고, 함수가 없으면 그 주소에서 역어셈블해 함수를 만든 뒤 디컴파일한다(프로젝트는 바뀌지 않는다).

```powershell
powershell -ExecutionPolicy Bypass -File tools\ghidra\decompile_at.ps1 -Addresses 484ab0,4c2b20,4c3290
# 결과: extracted\decomp-at\originals.c (Git 제외, 약 1분)
```

Linux 에서는 PowerShell 대신 같은 일을 하는 `tools/ghidra/decompile_at.sh`(2026-09-30 추가, `analyzeHeadless -process … -readOnly` 사용)를 쓴다.

```bash
bash tools/ghidra/decompile_at.sh 4b1e80 4b23e0     # 결과: extracted/decomp-at/originals.c (약 5초)
# GHIDRA_DIR=~/Tools/ghidra_12.1.4_PUBLIC OUT_FILE=/tmp/at.c 로 경로를 바꿀 수 있다
```

* 누락 함수의 실제 사례: 다리 이웃 탐색기의 후보 필터 `FUN_004b1e80`(탐색기 가상 함수 표 `0x513098`의 슬롯 0)은 표로만 호출되어 전체 디컴파일에 없었다. 이 함수가 두 프레임의 **연결 방향 일치**(`FUN_00441e40`)를 검사한다는 것이 다리 붕괴 알고리즘의 열쇠였다([bridge-pieces.md](bridge-pieces.md) 8.1절). **가상 함수 표(`.rdata`)에서 함수 주소가 4바이트 값으로 나열된 곳을 보고, 디컴파일에서 `(**(code **)*param_1)(…)` 같은 간접 호출이 나오면 그 표를 읽어 슬롯 함수를 이 도구로 디컴파일한다.**

### 7.1 누락 함수 일괄 복구 → 정밀 디컴파일 (2026-10-05)

2026-10-04 에 만든 읽기 전용 복구 도구(`recover_missing.ps1`, `merge_decomp.py`, `Netstorm.all.c`)는 **정밀 디컴파일 파이프라인으로 대체되어 삭제했다.** 정밀 디컴파일은 누락 함수를 Ghidra 프로젝트에 실제로 만들고, `switch` 분기 블록을 함수 몸체에 포함시키고, 근거 기반으로 호출 규약을 지정하고, 패치판과 CD판의 함수를 서로 대응시켜 함수마다 신뢰도 등급을 붙인다.

```powershell
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1   # 두 판본 전부, 약 20분
python tools/decomp_refine.py --show 4b1e80                            # 함수와 상대 판본의 짝을 차례로 출력
```

결과는 `extracted/refined/originals/Netstorm.c`(패치판 5,549개 함수)와 `extracted/refined/originalCD/NETSTORM.c`(CD판 5,823개 함수)다. 방법·검증·읽는 법은 [decompile-reliability.md](decompile-reliability.md)에 있다.

이전 도구에서 확인했던 두 가지 오탐은 정밀 디컴파일에서 해소됐다.

* **메인 프레임 함수 `FUN_004d62b0` 안의 가짜 함수 26개**: `switch` 분기 블록이 함수 몸체에서 빠져 있어 별개 함수로 복구됐던 것이다. 지금은 분기 블록이 몸체에 포함되어 16,945바이트짜리 한 함수다. (디컴파일 결과 자체에는 처음부터 `switch` 3개와 `case` 48개가 들어 있었다 — 빠져 있던 것은 리스팅의 함수 몸체뿐이다.)
* **`afterret` 출처의 꼬리 조각**: 호출자·포인터·상대 판본의 짝이 없으면 D 등급으로 표시된다(예: `FUN_004a59da`).

**`FUN_004d62b0` 전체 디컴파일 성공(2026-10-04).** `DECOMPILE_TIMEOUT_SEC`를 60초에서 3600초로 올려 전체 디컴파일을 다시 돌리자 이 함수가 `Netstorm.c` 141,241~143,296행(약 2,056줄)에 복구되었고 전체가 **성공 4,506·실패 0**이 되었다(이전 결과는 이 함수만 타임아웃). 그 앞부분(1~141,241행)은 이전 결과와 같고, 143,297행 이후는 2,053줄 뒤로 밀렸다. 문서가 인용한 줄 번호는 모두 141,241행보다 앞이라 영향이 없다. `DecompileAt.java`도 같은 제한 시간(3600초)으로 바꿨다.
