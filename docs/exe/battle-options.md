# 전투 옵션 (`globalBattleOptions`)과 에너지 공급 범위

> 분석일: 2026-09-29. 근거: `Netstorm.exe` 데이터(옵션 표 VA `0x52f5d0`~, 범위 표 `0x5425f4`)와 디컴파일
> `004b4860`·`004b4990`(State.cpp)·`004a7xxx`·`004c3bb0`·`004c35xx`(Totalmade.cpp). 도움말 근거: `GAME.HLP` "BattleMasters" ([help-manual.md](../gameplay/help-manual.md)).

## 1. 옵션 저장 위치

* 전투 옵션은 **`0x52f7e0` 부터 옵션 번호 순서로 1바이트씩** 저장된다 (`globalBattleOptions`, 최대 0x57 바이트).
  `State.cpp` `004b4990` 이 네트워크·저장 자료에서 이 영역을 통째로 복사하고(`z->dataLen <= sizeof(globalBattleOptions)` assert), 복사 직후 `004b4860`(공급 범위 갱신)을 부른다.
* exe 안의 초기값은 모두 0 이며, 아래 표의 기본값은 실행 중에 채워지는 것으로 보인다 (적용 위치는 미확인).

## 2. 옵션 표 (VA `0x52f5d0`, 24바이트 레코드)

레코드 = `[이름 문자열][선택지 문자열 목록][최대 인덱스][최소 인덱스][기본 인덱스][옵션 번호]`.

| 번호 | 이름 | 최소~최대 | 기본 | 선택지 (인덱스 0부터) |
|---|---|---|---|---|
| 1 | Bridge Slots | 0~2 | 2 | 2, 4, 6 |
| 2 | Unit Rate | 1~2 | 2 | Slow, Medium, Fast |
| 3 | Knowledge | 1~1 | 0 | Individual, Shared |
| **4** | **Generator Range** | **1~3** | **3** | **Short, Normal, Long, Very Long** |
| 5 | Map Mode | 0~1 | 0 | Archipelago, Single Island |
| 6 | Map Size | 0~3 | 2 | Small, Medium, Large, Extra Large |
| 7 | Map Density | 0~3 | 1 | Low, Medium, High, Extreme |
| 8 | Spells | 0~1 | 1 | No, Yes |
| 9 | Island Dynamics | 0~3 | 0 | Off, Low, Medium, High |
| 10 | Living World | 0~1 | 0 | Off, On |
| 11 | Alliances | 0~1 | 1 | No, Yes |
| 12 | Alliances Breakable | 1~1 | 0 | No, Yes |
| 13 | Allow Sending Money | 0~1 | 1 | No, Yes |
| 14 | Starting Cash | 0~2 | 1 | 5000, 6500, 7000 |
| 15 | Kill Reward | 1~5 | 2 | 0%, 25%, 50%, 75%, 100%, 150% |
| 16 | Money per Geyser | 0~3 | 2 | 1000, 2000, 3000, 5000 |
| 17 | Geyser Amount | 0~3 | 1 | Low, Medium, High, Extreme |
| 18 | Geysers Placement | 0~2 | 2 | None, Random, Even |
| 19 | Geysers Respawns | 0~4 | 2 | None, 30 Sec, 1 Min, 2 Min, 5 Min |
| 20 | Resource Injections | 0~5 | 0 | None, 30 Sec, 1 Min, 2 Min, 5 Min, 10 Min |
| 21 | Injection Value | 0~3 | 0 | 500 SP, 1000 SP, 2500 SP, 5000 SP |

* 번호 0 은 `Game Type`(Netstorm Standard/Classic/Flexible/Geyserless/Player Customizable, 레코드 `(4, 0, 0, 1)` — 앞 레코드와 필드 배치가 달라 해석 확인 필요).
* 선택지 목록은 문자열 포인터 배열이며 끝 표시가 없으므로, 개수는 최대 인덱스+1 로 본다 (위 표는 그 기준. Kill Reward 는 최대 5 → 6개).
* 도움말(원판)은 Bridge Slots 2/4/6, Unit Rate slow/medium/fast, **Generator Range short/normal/long**, Kill Reward 0/25/50/100%, SP per Geyser 1000/2000/3000 을 설명한다. 패치판 exe 는 선택지가 더 많다 (Very Long, 75%·150%, 5000 등).
* Unit Rate의 실제 생산 창 재충전 시간과 요새 모드 예외는 [production-refresh.md](production-refresh.md)에 정리했다.

## 2-1. 처치 보상 비율 (2026-10-01 확인)

* 보상 계산 `FUN_0044c2f0`은 `DAT_005424bc × 비용 / 100`을 처치한 플레이어에게 준다.
* `DAT_005424bc`는 전투 초기화 `FUN_004b2df0`에서 **0x19(25%)**로 정해진다. 옵션 15 표(`FUN_0041ca70`: 0·25·50·75·100·150%)로 바꾸는 곳은 네트워크·저장 옵션 복사 `FUN_004b4900`뿐이다.
* 따라서 **캠페인·튜토리얼은 25%**, 멀티플레이는 옵션 값(기본 인덱스 2 = 50%)이다. 사용자 설명(적 템플 파괴 보상 = 비용의 25%)과 일치한다. 클론: `BattleOptions.ApplySinglePlayerKillReward()`.

## 3. 에너지 공급 범위 (`004b4860`)

```c
// 범위 표 (float, VA 0x5425f4): {14, 22, 30, 38}
range = table[options[4]];               // options[4] = Generator Range 인덱스
if (range > table[2]) range = table[2];  // 30 칸 초과는 30 으로
if (range < table[0]) range = table[0];  // 14 칸 미만은 14 로
rangeSq = range * range;                 // DAT_0052f48c, 거리² 비교용
```

| 인덱스 | 이름 | 표 값 | 실제 반지름 |
|---|---|---|---|
| 0 | Short | 14 | **14칸** (옵션 화면 최소가 1 이라 일반적으로 선택 불가 — 튜토리얼 2 가 강제) |
| 1 | Normal | 22 | 22칸 |
| 2 | Long | 30 | 30칸 |
| 3 | Very Long (기본) | 38 | **30칸** (상한에 걸림) |

* **일반 미션 30칸**(스크린샷 측정 30.0칸, [elements-energy.md](../gameplay/elements-energy.md) 6절)은 기본 인덱스 3 이 상한 30 으로 제한된 값과 일치한다.
* 범위 비교에는 `rangeSq` 를 쓰는 것으로 보인다 (칸 좌표 거리² ≤ 900 과 일치).

## 4. 튜토리얼 2 의 범위 축소 (`Totalmade.cpp`)

* 미션 제어 객체는 이름으로 등록된 두 종류뿐이다: **`Normal`**(`00484e60`) 과 **`Tutorial`**(`004c4710` → 생성자 `004c2990`). 미션 머리 값 `missionType` 과 대응하는 것으로 보인다.
* `Tutorial` 객체는 `DAT_005ca8e4`(튜토리얼 번호, 머리 값 `tutorialNumber` 로 추정)로 단계 처리 함수를 고른다: 1 `004c3a20`, **2 `004c3bb0`**, 3 `004c3f00`, 4 `004c40f0`, 5 `004c42b0`, 6 `004c45e0`.
  단계 문자 `DAT_005ca8e8` 는 생성 시 `'A'` 에서 시작해 `004c33f0` 이 1씩 올린다.
  단계 처리 함수는 전투 옵션뿐 아니라 **기술 허용 표(sunFactory 허용)와 회수 금지(`denySalvage`)도 실행 중에 바꾼다** → [mission-header-flags.md](mission-header-flags.md).
* **튜토리얼 2 의 단계 'A' 만** `options[4] = 0`(Short = **14칸**) 과 `options[2] = 2`(Unit Rate Fast) 로 바꾸고 `004b4860` 을 다시 부른다.
  다른 튜토리얼 처리 함수에는 Generator Range 를 바꾸는 코드가 없다.
  → 사용자 설명 "튜토리얼 일부에서 범위가 대폭 축소", 캡처 `Tutorial - Temple - Generating Range.png`(튜토리얼 2, 약 14칸) 와 일치한다.
  `GAME.HLP` 의 튜토리얼 2 안내도 "Wind Temple 의 범위가 적 섬까지 닿지 않으니 Wind Generator 를 줄지어 놓으라"고 설명한다.
* 캠페인 미션도 `missionType = "Tutorial"` 이지만 `tutorialNumber` 가 없으므로 단계 처리 함수가 선택되지 않는 것으로 보인다 (번호 0 → `switch` 해당 없음).

## 5. 클론 구현 방침 (제안)

* 에너지 공급 반지름은 설정값으로 둔다: 기본 30칸, 튜토리얼 2 는 14칸. 멀티플레이 옵션에서 22·30 을 고를 수 있게 하되, 원본 패치판처럼 38 은 30 으로 제한할지는 결정 사항이다.
* 판정은 칸 좌표 거리² ≤ 반지름² 으로 한다.
