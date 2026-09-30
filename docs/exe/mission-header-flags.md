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
  이 표가 "지은 개수"라는 점은 단계 조건(`sunArcher > 0`, `> 1`, `> 3`)과 스크립트 문구("두 개 더 지으면")로 추정하며, 누적 값인지 현재 개수인지는 확인하지 못했다.

| 단계 | 스크립트 | 매 프레임 하는 일 | 다음 단계로 가는 조건 |
|---|---|---|---|
| A | `[A.]` "Buildings", "Create a Temple" | 전투 옵션을 덮어쓴다: `DAT_0052f7e4 = 0`(Generator Range = Short), `DAT_0052f7e2 = 2`(Unit Rate = Fast), `FUN_004b4860`(반지름 재계산) ([battle-options.md](battle-options.md)) | vortex(템플)를 지은 개수 > 0 |
| **B** | `[B.]` "Temple Complete", `[B1.]` "Create a Workshop" | **`FUN_004c23e0(sunFactory, 허용)`** — 표에서 Sun Workshop을 허용으로 바꾼다 | factory(워크샵) 개수 > 0 |
| C | `[C.]` "Production" | 템플(0x200)을 선택한 채 일정 시간이 지나면 `[NotVortex]` 보정 안내를 띄운다(대기 시간 상수는 미확인) | 워크샵에 sunArcher를 등록(`FUN_0044f970`) |
| D | `[D.]`, `[D1.]` | — | sunArcher 개수 > 0 |
| E | `[E.]` | — | sunArcher 개수 > 1 |
| F | `[F.]`, `[F1.]` | 템플을 선택하면 타이머를 시작한다(상수 `0x506588`, 미확인) | 타이머가 지남 |
| G | `[G.]` | — | sunArcher 개수 > 3 ("두 개 더") |
| **H** | `[H.]`, `[H1.]` "Salvaging" | **`DAT_00595078 = 0`** — 회수 금지를 푼다 | 회수 명령 이벤트(`DAT_0054286c`) 뒤 타이머 |
| I | `[I.]` "Mission Accomplished!" | — | 결과 창 |

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

* [`TechPermissions`](../../src/Netstorm.Core/Rules/MissionStart.cs)를 **변경 가능한 표**로 바꿨다(`Set`·`SetAll` = `FUN_004c23e0`·`FUN_004c23c0`). 머리 값은 시작 상태다.
* [`BattleSession`](../../src/Netstorm.Core/Simulation/BattleSession.cs)의 `DenySalvage`는 머리 `denySalvage`로 시작하는 **변하는 상태**다.
  켜져 있으면 회수 명령은 `SalvageDenied`로 거부된다(원본의 "DenySalvage 섹션 Tell"에 대응, 화면 안내는 없음).
* 기술 허용 표는 사제 Construct 판정(`CheckBuilding`), 지식 등록(`RegisterKnowledgeCommand`), 덱 배치(`CheckUnit`)에서 확인한다 — 원본이 확인하는 곳(3.2)과 같은 수준이다.
* **아직 없는 것**: 튜토리얼 단계 처리 코드 자체. 지금은 테스트가 단계 B·H의 효과(`Tech.Set("sunFactory", true)`, `DenySalvage = false`)를 손으로 재현한다
  ([BattleSessionTests](../../tests/Netstorm.Core.Tests/BattleSessionTests.cs)). 단계 C·F의 조건은 UI 선택 상태(우클릭 메뉴·선택한 오브젝트)와 타이머 상수에 의존해서
  UI와 함께 구현해야 한다. 이때 단계 처리는 세션 명령·이벤트를 보고 표를 바꾸는 별도 객체로 두면 된다.
* 맵 뷰어의 배치 시험 모드는 `EnforceProductionRules = false`라 표·회수 금지·덱을 모두 무시한다.

## 6. 한계·미확인

* 표를 **명령 실행 경로**(배치 커서·건설 실행)에서 다시 확인하는지는 못 찾았다. 메뉴에서 항목이 빠지면 사용자가 명령을 낼 수 없으므로 실질적으로 같지만, 네트워크 등 다른 경로의 검증은 알 수 없다.
* `FUN_004cf960`이 없는 섹션 이름을 받았을 때의 동작, 단계 C·F의 타이머 상수(`0x506588`, `0x506590`), `DAT_005c98d0`의 정확한 의미(누적/현재), `FUN_004c2500`이 합하는 대상은 확인하지 못했다.
* `FUN_00484ab0`(`tech` 키 파서)을 쓰는 미션 유형은 확인하지 못했다.
* `FUN_004c2b20`이 언제·누가 부르는지(가상 함수 표 경유로 보이며 직접 호출자가 없다)와 `FUN_004c3290`·`FUN_004c2b20`의 실행 순서는 확인하지 못했다.

## 7. Ghidra 전체 디컴파일에서 빠진 함수 (재현 방법 포함)

`extracted/decomp/Netstorm.c`(함수 4,506개)에는 **위 세 함수(`FUN_00484ab0`, `FUN_004c2b20`, `FUN_004c3290`)가 없다.**
Ghidra 자동 분석이 이 주소를 함수로 인식하지 못했기 때문이다(직접 호출자가 없거나 가상 함수 표로만 호출되는 코드로 보인다).
`grep`으로 `denySalvage` 문자열이 안 나온 것도 이 때문이다.

* 빠진 함수는 더 있을 수 있다. INT3(0xCC) 패딩 직후에서 흔한 함수 프롤로그로 시작하지만 디컴파일 목록에 없는 주소가 **약 528개**(그중 진입점 간격이 16바이트 이상인 것 525개)이며 위 세 함수도 여기에 들어간다.
  이 수는 휴리스틱 후보이고 실제 누락 수는 검증하지 않았다. **디컴파일 결과에서 `push 주소`로만 쓰이는 문자열(설정 키 등)이 검색되지 않으면 이런 누락 함수를 의심한다.**
* 누락 함수 찾기: 문자열의 주소를 exe 바이트에서 `push 주소`(`68 xx xx xx xx`)로 검색해 참조 위치를 얻고, 그 앞의 INT3 패딩 다음을 함수 시작으로 잡는다.
* 누락 함수 디컴파일: `tools/ghidra/decompile_at.ps1`(`DecompileAt.java`)이 이미 만든 프로젝트를 **읽기 전용**으로 열고, 함수가 없으면 그 주소에서 역어셈블해 함수를 만든 뒤 디컴파일한다(프로젝트는 바뀌지 않는다).

```powershell
powershell -ExecutionPolicy Bypass -File tools\ghidra\decompile_at.ps1 -Addresses 484ab0,4c2b20,4c3290
# 결과: extracted\decomp-at\originals.c (Git 제외, 약 1분)
```
