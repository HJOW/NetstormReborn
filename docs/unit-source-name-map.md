# 게임 표시 이름과 소스 타입명 대응표

2026-10-16부터 분석 중 확인한 이름을 이 문서에 계속 추가한다. **영어 표시 이름은 원본10.78의 `.type` 속성 `description`, 내부 타입명과 번호는 실제 등록 TYPE 표**를 기준으로 한다. 클래스명·함수명·미션의 기술 키에는 내부 이름이 남아 있으므로 두 이름을 구별한다.

예를 들어 `sunFence`는 **Sun Barricade(썬 바리케이드)**다. `sunBlocker`는 **Stone Tower(스톤 타워)**이며 서로 다른 건물이다. `sunAviary`도 사용자 표시 이름은 **Whirlibase(휘리베이스)**다. 앞으로 설명에는 표시 이름을 사용하고 처음 등장할 때 내부 이름을 함께 적는다. 기존 코드 심벌과 과거 작업 기록의 식별자는 유지한다.

## 확인한 건물·유닛·자원

한국어 표기는 [공유 명칭 표](../assets/unit-names.korean.json)를 따른다. 영어 표시 이름을 한국어로 적은 프로젝트 표기이며 원본에 공식 한국어 UI가 있었다는 뜻은 아니다.

| 10.78 TYPE | 내부 타입명 | 원본 영어 표시 이름 | 한국어 표기 |
| ---: | --- | --- | --- |
| 71 | `sunArcher` | Sun Disc Thrower | 썬 디스크 발사기 |
| 72 | `sunAviary` | Whirlibase | 휘리베이스 |
| 73 | `sunFlyer` | Whirligig | 휘리기그 |
| 75 | `windBattery` | Wind Generator | 윈드 제네레이터 |
| 76 | `rainBattery` | Rain Generator | 레인 제네레이터 |
| 77 | `thunderBattery` | Thunder Generator | 썬더 제네레이터 |
| 78 | `sunBlocker` | Stone Tower | 스톤 타워 |
| 79 | `thunderBlocker` | Bulwark | 불워크 |
| 82 | `bridge` | Bridge | 다리 |
| 84 | `emptyGeyser` | Empty Storm Geyser | 엠프티 스톰 가이저 |
| 85 | `sunFactory` | Sun Workshop | 썬 워크샾 |
| 86 | `windFactory` | Wind Workshop | 윈드 워크샾 |
| 87 | `rainFactory` | Rain Workshop | 레인 워크샾 |
| 88 | `thunderFactory` | Thunder Workshop | 썬더 워크샾 |
| 89 | `residence` | Residence | 레지던스 |
| 111 | `sunBalloon` | Balloon | 벌룬 |
| 112 | `buried` | Obelisk | 오벨리스크 |
| 122 | `geyser` | Storm Geyser | 스톰 가이저 |
| 123 | `windVortex` | Wind Temple | 윈드 신전 |
| 124 | `rainVortex` | Rain Temple | 레인 신전 |
| 125 | `thunderVortex` | Thunder Temple | 썬더 신전 |
| 126 | `outpost` | Outpost | 아웃포스트 |
| 128 | `sunCannon` | Sun Cannon | 썬 캐논 |
| 129 | `rainCannon` | Ice Cannon | 아이스 캐논 |
| 131 | `thunderCannon` | Thunder Cannon | 썬더 캐논 |
| 133 | `sunWalker` | Golem | 골렘 |
| 135 | `bulf` | Bulf | 벌프 |
| 136 | `sunFence` | Sun Barricade | 썬 바리케이드 |
| 137 | `windWalker` | Sail Skater | 세일 스케이터 |
| 138 | `windFlyer` | Dust Devil | 더스트 데빌 |
| 139 | `windAviary` | Devil Maker | 데빌 메이커 |
| 140 | `windArcher` | Crossbow | 크로스보우 |
| 141 | `windBalloon` | Air Ship | 에어 쉽 |
| 142 | `windBlocker` | Wind Tower | 윈드 타워 |
| 143 | `rainAviary` | Man o'War Pool | 맨 오 워 풀 |
| 144 | `rainFlyer` | Man o'War | 맨 오 워 |
| 145 | `rainFence` | Acid Barricade | 애시드 바리케이드 |
| 146 | `rainBalloon` | Cloud Floater | 클라우드 플로터 |
| 147 | `rainBlocker` | Ice Tower | 아이스 타워 |
| 148 | `thunderArcher` | Vander Tower | 밴더 타워 |
| 149 | `thunderFence` | Arc Spire | 아크 스파이어 |
| 154 | `nugget` | Storm Crystal | 스톰 크리스탈 |
| 156 | `rainWalker` | Crystal Crab | 크리스탈 크랩 |
| 158 | `priest` | High Priest | 사제 |
| 170 | `fenceShield` | Force Field | 포스 필드 |

`Battery`·`Factory`·`Aviary`·`Walker`·`Archer`·`Fence`·`Blocker`·`Vortex`는 내부 구현상의 이름이다. 이를 그대로 사용자 명칭으로 번역하지 않는다. 특히 모든 `Blocker`가 바리케이드이거나 모든 `Archer`가 아처라는 뜻이 아니다.

## 이름이 같아도 구별할 내부 객체

| 10.78 TYPE | 내부 타입명 | 원본 `description` | 취급 |
| ---: | --- | --- | --- |
| 164 | `altar` | Altar | 실제 알타 계열 객체. |
| 165 | `dais` | Altar | 내부 다이스 객체. TYPE164와 합치지 않는다. |
| 166 | `rune` | Altar | 내부 룬 객체. TYPE164/165와 합치지 않는다. |
| 167 | `forceField` | High Priest | 사제 관련 내부 객체. 실제 사제 TYPE158 및 표시 이름이 Force Field인 TYPE170과 구별한다. |

이 표의 `description` 존재만으로 해당 보조 객체가 독립적인 생산/선택 UI를 가진다고 판단하지 않는다. 주문·효과·보조 객체는 실제 사용 위치를 확인하면서 계속 추가한다.

## 근거와 갱신 방법

- 보호 원본 [netstorm.tarc](../originals/netstorm.tarc)의 `.type` 텍스트에서 `typename`과 `description`을 대조했다. [수치표](gameplay/types.md)도 같은 속성을 표시하지만 자동 생성 문서이므로 이 명칭 확인 작업에서 직접 수정하지 않는다.
- `NetstormCpp.exe --dump-types originals`의 실제 등록188TYPE 중 이름이 있는 행과 대조했다. `.type` 원문의 `sunaviary`/`Priest`처럼 대소문자만 다른 표기는 실제 등록 이름과 구별해 읽는다. 표에는 등록 TYPE 이름을 적었다.
- 원본 [BC9Sun_Bars.english](../originals/d/BC9Sun_Bars.english)의 기술 `sunFence`, 그림 키 `sunfence.*`, 표시 문구 `Sun Barricade`도 같은 대응을 뒷받침한다.
- 이번 확인은 현재/마지막 전체 정밀 디컴파일 PC **VM-W11-Codex / 2026-10-16**에서 읽기 전용으로 수행했다. 추출 JSON/TYPE 덤프는 `extracted/unit-name-{types,ids}-20261016.*`에 보존했다. 게임 창 실행과 보호 원본 변경은 없다.
- 이후 새 명칭은 **판본·내부 타입/번호·원문 표시 이름·근거 경로·확인 범위**를 함께 기록한다. 미션/번역 설정이 이름을 바꾸면 기본 `description`과 실제 설정 후 이름을 구별한다. CD10.72의 번호/이름은 별도 확인 전10.78과 같다고 가정하지 않는다.

기존 [한국어 명칭 규칙](korean-unit-names.md)과 공유 JSON은 그대로 사용한다. 이 문서는 내부 이름과 표시 이름의 관계를 기록하며 게임 UI나 공유 번역 자료를 변경하지 않는다.
