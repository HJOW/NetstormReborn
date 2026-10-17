# 게임 표시 이름과 소스 타입명 대응표

2026-10-16부터 분석 중 확인한 이름을 이 문서에 계속 추가한다. **영어 표시 이름은 원본10.78의 `.type` 속성 `description`, 내부 타입명과 번호는 실제 등록 TYPE 표**를 기준으로 한다. 클래스명·함수명·미션의 기술 키에는 내부 이름이 남아 있으므로 두 이름을 구별한다.

예를 들어 `sunFence`는 **Sun Barricade(썬 바리케이드)**다. `sunBlocker`는 **Stone Tower(스톤 타워)**이며 서로 다른 건물이다. `sunAviary`도 사용자 표시 이름은 **Whirlibase(훨리베이스)**다. 앞으로 설명에는 표시 이름을 사용하고 처음 등장할 때 내부 이름을 함께 적는다. 기존 코드 심벌과 과거 작업 기록의 식별자는 유지한다. **`sunWalker`도 사용자 표시 이름은 Golem(골렘)**이다. “썬 워커”는 내부 식별자를 읽은 표현이며, 앞으로 사용자에게 유닛을 설명할 때는 “골렘”으로 적는다.

## 확인한 건물·유닛·자원

한국어 표기는 [공유 명칭 표](../assets/unit-names.korean.json)를 따른다. 영어 표시 이름을 한국어로 적은 프로젝트 표기이며 원본에 공식 한국어 UI가 있었다는 뜻은 아니다.

| 10.78 TYPE | 내부 타입명 | 원본 영어 표시 이름 | 한국어 표기 |
| ---: | --- | --- | --- |
| 71 | `sunArcher` | Sun Disc Thrower | 썬 디스크 발사기 |
| 72 | `sunAviary` | Whirlibase | 훨리베이스 |
| 73 | `sunFlyer` | Whirligig | 훨리기그 |
| 75 | `windBattery` | Wind Generator | 윈드 제네레이터 |
| 76 | `rainBattery` | Rain Generator | 레인 제네레이터 |
| 77 | `thunderBattery` | Thunder Generator | 썬더 제네레이터 |
| 78 | `sunBlocker` | Stone Tower | 스톤 타워 |
| 79 | `thunderBlocker` | Bulwark | 불워크 |
| 82 | `bridge` | Bridge | 다리 |
| 84 | `emptyGeyser` | Empty Storm Geyser | 고갈된 스톰 간헐천 |
| 85 | `sunFactory` | Sun Workshop | 썬 워크샾 |
| 86 | `windFactory` | Wind Workshop | 윈드 워크샾 |
| 87 | `rainFactory` | Rain Workshop | 레인 워크샾 |
| 88 | `thunderFactory` | Thunder Workshop | 썬더 워크샾 |
| 89 | `residence` | Residence | 거주 구역 |
| 111 | `sunBalloon` | Balloon | 벌룬 |
| 112 | `buried` | Obelisk | 오벨리스크 |
| 122 | `geyser` | Storm Geyser | 스톰 간헐천 |
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
| 148 | `thunderArcher` | Vander Tower | 반더 타워 |
| 149 | `thunderFence` | Arc Spire | 아크 스파이어 |
| 154 | `nugget` | Storm Crystal | 스톰 크리스탈 |
| 156 | `rainWalker` | Crystal Crab | 크리스탈 크랩 |
| 158 | `priest` | High Priest | 사제 |
| 170 | `fenceShield` | Force Field | 포스 필드 |

`Battery`·`Factory`·`Aviary`·`Walker`·`Archer`·`Fence`·`Blocker`·`Vortex`는 내부 구현상의 이름이다. 이를 그대로 사용자 명칭으로 번역하지 않는다. 특히 모든 `Blocker`가 바리케이드이거나 모든 `Archer`가 아처라는 뜻이 아니다.

## `sunWalker`는 골렘이다

2026-10-17 사용자 요청에 따라 원본10.78을 다시 확인했다. **TYPE133 `sunWalker` = Golem = 골렘**이며 위 대응표의 기존 행이 맞다. 별도 유닛인 “Sun Walker”를 뜻하지 않는다.

| 확인 근거 | 원문에서 확인한 값 |
| --- | --- |
| 보호 원본 `originals/netstorm.tarc` 안의 `d/sunwalker.type` | `typename sunwalker`, `description="Golem"`, `class="Ground Transport"` |
| 같은 `.type`의 그래픽·음향 참조 | `golem.gif`·`s_golem.gif`, `golemMove1.wav`·`golemPickup.wav`·`golemDrop.wav` |
| `NetstormCpp.exe --dump-types originals` | TYPE133의 등록 이름 `sunWalker`, 그룹 `walker` |
| [공유 한국어 명칭 표](../assets/unit-names.korean.json) | `"Golem": "골렘"` |

보호 아카이브의 항목은 `NetstormCpp.exe --read-archive originals/netstorm.tarc d/sunwalker.type`으로 직접 읽었다. 내부 이름의 대소문자 차이(`sunwalker`/`sunWalker`)는 같은 대응으로 읽는다. 설명·작업 인계에는 **골렘(내부 `sunWalker`, TYPE133)**을 사용하고 코드 심벌·미션 기술 키·과거 근거 식별자는 보존한다. 이번 재확인은 현재 PC **HJOW-Athlon**, 마지막 정밀 디컴파일 **HJOW-Athlon / 2026-10-16**을 확인한 뒤 자산/등록표를 읽기 전용으로 대조했다. 새 디컴파일이나 게임 창 실행은 없으며, 확인 범위는10.78이다.

## 이름이 같아도 구별할 내부 객체

| 10.78 TYPE | 내부 타입명 | 원본 `description` | 취급 |
| ---: | --- | --- | --- |
| 164 | `altar` | Altar | 실제 제단 계열 객체. |
| 165 | `dais` | Altar | 내부 다이스 객체. TYPE164와 합치지 않는다. |
| 166 | `rune` | Altar | 내부 룬 객체. TYPE164/165와 합치지 않는다. |
| 167 | `forceField` | High Priest | 사제 관련 내부 객체. 실제 사제 TYPE158 및 표시 이름이 Force Field인 TYPE170과 구별한다. |

이 표의 `description` 존재만으로 해당 보조 객체가 독립적인 생산/선택 UI를 가진다고 판단하지 않는다. 주문·효과·보조 객체는 실제 사용 위치를 확인하면서 계속 추가한다.

## 색 변경·아비어리 분석에서 확인한 내부 보조 타입

2026-10-16 같은10.78 등록 덤프와 원본0043ca70/0045ec50/0045ed10/0045edf0에서 확인했다. 아래 타입에는 별도 사용자 표시 이름을 붙이지 않는다. 건물·유닛 자체와 이를 조작하는 프로세스는 서로 다른 객체다.

| 10.78 TYPE | 실제 등록 이름 | 소스/분석에서 쓰는 전체 이름 | 대응 또는 역할 |
| ---: | --- | --- | --- |
| 31 | `baseFlyerProcessType` | `baseFlyerProcessType` | 플라이어 공통 동작 프로세스. |
| 32 | `sunAviaryProcessType` | `sunAviaryProcessType` | 훨리베이스(TYPE72·sunAviary)의 생산 동작 프로세스. |
| 47 | `soundProcessType` | `soundProcessType` | 플라이어가 이동 중 제거하는 소리 프로세스. 무기 프로세스와 구별한다. |
| 59 | `windAviaryProcessTyp` | `windAviaryProcessType` | 데빌 메이커(TYPE139·windAviary)의 생산 동작 프로세스. |
| 60 | `rainAviaryProcessTyp` | `rainAviaryProcessType` | 맨 오 워 풀(TYPE143·rainAviary)의 생산 동작 프로세스. |
| 94 | `island` | `island` | 섬 받침. 지형 본체인 TYPE97·isle과 구별한다. |
| 95 | `islandStalag` | `islandStalag` | 섬 받침 아래 종유석. 받침과 함께 소유자 색 프레임이 바뀐다. |

프로세스의 TYPE 이름 필드는20바이트여서 TYPE59/60의 끝 `e`가 잘린다. 전체 소스 이름과 등록 덤프의 문자열 차이를 별도 유닛으로 오인하지 않는다. TYPE94/95의 `island` 명칭도 사용자 유닛 이름을 번역할 근거는 아니다.

2026-10-16에 TYPE144 `rainFlyer`의 실제0050a388+20→0045fac0도 확인했다. 이 맨 오 워는 공통 **타입31** 프로세스를 만든 뒤 outer00509f60/inner00509f40의 전용 동작 표로 바꾼다. 별도 `rainFlyerProcessType` 번호를 추정해 추가하지 않는다. 그림자도 같은 TYPE144를 사용하고 전체 프레임 수만큼 프레임을 옮긴다. [최초 후처리/프로세스 생성 복원](exe/cpp-flyer-postpop-20261016.md)을 참고한다.

### 비행체별 수명과 종료를 구별한다

2026-10-16 사용자가 **훨리기그는 수명 만료 시 소속 훨리베이스로 귀환·충전 후 다시 출격하고, 맨 오 워는 수명 만료 시 소멸하며 전투 파괴와 그래픽·효과음이 다르다**고 알려 주었다. 이를 같은 플라이어 공통 동작으로 합치지 않는다. 훨리기그의 귀환·충전·재출격 전체와 두 종료 효과의 실제 재생 결과는 원본 대조가 필요하다.

현재 정적 분석으로 확인한 맨 오 워의 `0045d630`은 첫 수명 기한을 현재 시각+60초로 설정하고, 기한 만료 시 `Destroy(0x200000)`을 호출한다. 일반적인 `Destroy(0)`와 플래그를 구별한다. 그림자 삭제·파생 PreDestroy·공통 피해 종료 효과까지 이어지는 복원과 확인은 별도이며, 이 플래그 확인만으로 그래픽·음향 재현 완료를 선언하지 않는다. 공통 `0045c9d0`의 홈 복귀·착륙 처리와 맨 오 워의 수명 wrapper도 서로 구별한다.

## 썬 바리케이드에서 구별할 두 연결 객체

2026-10-16 원본004566d0/00455b20과 실제 TYPE 로딩 순서를 대조했다. **TYPE90 `fencemark`**는 끝점 SID를 QA 및 `(QB<<8)|graphBYTE`로 저장하는 내부 연결 표식이다. 별도 사용자 표시명을 붙이지 않는다. **TYPE170 `fenceShield`**는 위 표의 **Force Field/포스 필드**이며 화면에 표시하는 연결선이다. 원본 전역도 각각 `005411c0=90`, `00541300=170`으로 다르다.

기존 Session은 두 번째 전역을90으로 연결하는 복원 오류가 있어170으로 정정했다. 내부 `fencemark`라는 이름만 보고 포스 필드나 Sun Barricade(TYPE136) 자체와 합치지 않는다. [실제 사건/표식 삭제 복원](exe/cpp-fence-regular-20261016.md)을 참고한다.

## 근거와 갱신 방법

- 보호 원본 [netstorm.tarc](../originals/netstorm.tarc)의 `.type` 텍스트에서 `typename`과 `description`을 대조했다. [수치표](gameplay/types.md)도 같은 속성을 표시하지만 자동 생성 문서이므로 이 명칭 확인 작업에서 직접 수정하지 않는다.
- `NetstormCpp.exe --dump-types originals`의 실제 등록188TYPE 중 이름이 있는 행과 대조했다. `.type` 원문의 `sunaviary`/`Priest`처럼 대소문자만 다른 표기는 실제 등록 이름과 구별해 읽는다. 표에는 등록 TYPE 이름을 적었다.
- 원본 [BC9Sun_Bars.english](../originals/d/BC9Sun_Bars.english)의 기술 `sunFence`, 그림 키 `sunfence.*`, 표시 문구 `Sun Barricade`도 같은 대응을 뒷받침한다.
- 이번 확인은 현재/마지막 전체 정밀 디컴파일 PC **VM-W11-Codex / 2026-10-16**에서 읽기 전용으로 수행했다. 추출 JSON/TYPE 덤프는 `extracted/unit-name-{types,ids}-20261016.*`에 보존했다. 게임 창 실행과 보호 원본 변경은 없다.
- 이후 새 명칭은 **판본·내부 타입/번호·원문 표시 이름·근거 경로·확인 범위**를 함께 기록한다. 미션/번역 설정이 이름을 바꾸면 기본 `description`과 실제 설정 후 이름을 구별한다. CD10.72의 번호/이름은 별도 확인 전10.78과 같다고 가정하지 않는다.

2026-10-16 [한국어 명칭 규칙](korean-unit-names.md)과 공유 JSON에 훨리기그·훨리베이스·제단·거주 구역·간헐천을 반영했다. 영어 원문·내부 식별자·과거 검증 기록은 유지한다.
